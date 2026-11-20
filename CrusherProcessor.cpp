#include "CrusherProcessor.h"
#include "CrusherEditor.h"
#include <cmath>

namespace
{
    // Saturador transparente: no toca nada por debajo de 0.5 y suaviza el exceso
    // hasta un maximo de 1.0, asi el drive no revienta el cuantizador.
    inline float softClip (float x)
    {
        constexpr float knee = 0.5f;
        const float a = std::abs (x);

        if (a <= knee)
            return x;

        const float e = knee + (1.0f - knee) * std::tanh ((a - knee) / (1.0f - knee));
        return x < 0.0f ? -e : e;
    }

    // Overflow tipo entero: lo que pasa de +1 reaparece desde -1
    inline float wrapSample (float x)
    {
        return x - 2.0f * std::floor ((x + 1.0f) * 0.5f);
    }
}

CrusherProcessor::CrusherProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    ditherRng[0].setSeed (1234);
    ditherRng[1].setSeed (5678);
    jitterRng.setSeed (42);
}

juce::AudioProcessorValueTreeState::ParameterLayout CrusherProcessor::createLayout()
{
    using P    = juce::AudioParameterFloat;
    using NR   = juce::NormalisableRange<float>;
    using Attr = juce::AudioParameterFloatAttributes;

    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<P> (juce::ParameterID { "drive", 1 }, "Drive",
                                           NR (0.0f, 36.0f, 0.1f), 0.0f,
                                           Attr().withLabel ("dB")));

    params.push_back (std::make_unique<P> (juce::ParameterID { "bits", 1 }, "Bits",
                                           NR (1.0f, 16.0f, 0.01f), 8.0f,
                                           Attr().withStringFromValueFunction ([] (float v, int)
                                           {
                                               return juce::String (v, 1) + " bit";
                                           })));

    params.push_back (std::make_unique<P> (juce::ParameterID { "rate", 1 }, "Rate",
                                           NR (500.0f, 48000.0f, 1.0f, 0.35f), 12000.0f,
                                           Attr().withStringFromValueFunction ([] (float v, int)
                                           {
                                               return juce::String (juce::roundToInt (v)) + " Hz";
                                           })));

    params.push_back (std::make_unique<P> (juce::ParameterID { "jitter", 1 }, "Jitter",
                                           NR (0.0f, 1.0f, 0.01f), 0.0f));

    params.push_back (std::make_unique<P> (juce::ParameterID { "dither", 1 }, "Dither",
                                           NR (0.0f, 1.0f, 0.01f), 0.0f));

    params.push_back (std::make_unique<P> (juce::ParameterID { "tone", 1 }, "Tone",
                                           NR (500.0f, 20000.0f, 1.0f, 0.35f), 20000.0f,
                                           Attr().withStringFromValueFunction ([] (float v, int)
                                           {
                                               return juce::String (juce::roundToInt (v)) + " Hz";
                                           })));

    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "aa", 1 },
                                                                  "Anti-alias", false));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "mode", 1 }, "Mode",
                                                                    juce::StringArray { "Round", "Truncate", "Wrap" }, 0));

    params.push_back (std::make_unique<P> (juce::ParameterID { "mix", 1 }, "Mix",
                                           NR (0.0f, 1.0f, 0.01f), 1.0f));

    params.push_back (std::make_unique<P> (juce::ParameterID { "output", 1 }, "Output",
                                           NR (-24.0f, 12.0f, 0.1f), 0.0f,
                                           Attr().withLabel ("dB")));

    return { params.begin(), params.end() };
}

void CrusherProcessor::prepareToPlay (double sampleRate, int)
{
    currentSampleRate = sampleRate;

    auto raw = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };

    smoothDrive.reset (sampleRate, 0.02);
    smoothBits.reset (sampleRate, 0.03);
    smoothRate.reset (sampleRate, 0.03);
    smoothJitter.reset (sampleRate, 0.02);
    smoothDither.reset (sampleRate, 0.02);
    smoothMix.reset (sampleRate, 0.02);
    smoothOutput.reset (sampleRate, 0.02);

    smoothDrive.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (raw ("drive")));
    smoothBits.setCurrentAndTargetValue (raw ("bits"));
    smoothRate.setCurrentAndTargetValue (raw ("rate"));
    smoothJitter.setCurrentAndTargetValue (raw ("jitter"));
    smoothDither.setCurrentAndTargetValue (raw ("dither"));
    smoothMix.setCurrentAndTargetValue (raw ("mix"));
    smoothOutput.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (raw ("output")));

    for (size_t ch = 0; ch < 2; ++ch)
    {
        aaFilterA[ch].reset();
        aaFilterB[ch].reset();
        toneFilter[ch].reset();
        held[ch] = 0.0f;
        dcX[ch] = 0.0f;
        dcY[ch] = 0.0f;
    }

    phase = 1.0;
    dcCoeff = std::exp (-juce::MathConstants<float>::twoPi * 10.0f / (float) sampleRate);
}

bool CrusherProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();

    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;

    return out == layouts.getMainInputChannelSet();
}

void CrusherProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numChannels = juce::jmin (buffer.getNumChannels(), 2);
    const int numSamples  = buffer.getNumSamples();

    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, numSamples);

    auto raw = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };

    const float rateTarget = raw ("rate");
    const float toneHz     = raw ("tone");
    const bool  aaOn       = raw ("aa") > 0.5f;
    const int   mode       = juce::jlimit (0, 2, (int) (raw ("mode") + 0.5f));
    const bool  toneOn     = toneHz < 19000.0f;

    smoothDrive.setTargetValue (juce::Decibels::decibelsToGain (raw ("drive")));
    smoothBits.setTargetValue (raw ("bits"));
    smoothRate.setTargetValue (rateTarget);
    smoothJitter.setTargetValue (raw ("jitter"));
    smoothDither.setTargetValue (raw ("dither"));
    smoothMix.setTargetValue (raw ("mix"));
    smoothOutput.setTargetValue (juce::Decibels::decibelsToGain (raw ("output")));

    const float sr = (float) currentSampleRate;

    // Anti-alias: Butterworth de 4to orden justo debajo de la mitad de la frecuencia reducida
    const float aaCutoff = juce::jlimit (20.0f, 0.45f * sr, 0.45f * rateTarget);
    for (size_t ch = 0; ch < 2; ++ch)
    {
        aaFilterA[ch].setLowpass (aaCutoff, sr, 0.5412f);
        aaFilterB[ch].setLowpass (aaCutoff, sr, 1.3066f);
        toneFilter[ch].setLowpass (toneHz, sr, 0.7071f);
    }

    float* channelData[2] = { nullptr, nullptr };
    for (int ch = 0; ch < numChannels; ++ch)
        channelData[ch] = buffer.getWritePointer (ch);

    for (int i = 0; i < numSamples; ++i)
    {
        const float drive   = smoothDrive.getNextValue();
        const float bits    = smoothBits.getNextValue();
        const float rateHz  = smoothRate.getNextValue();
        const float jitter  = smoothJitter.getNextValue();
        const float dither  = smoothDither.getNextValue();
        const float mix     = smoothMix.getNextValue();
        const float outGain = smoothOutput.getNextValue();

        // Niveles de cuantizacion: 2^(bits-1) por lado (acepta bits fraccionarios)
        const float levels = std::exp2 (bits - 1.0f);
        const float invLevels = 1.0f / levels;

        // Reloj de muestreo reducido, con jitter opcional
        const float ratio = juce::jmin (1.0f, rateHz / sr);
        const bool reducing = ratio < 0.9999f;
        bool tick = true;

        if (reducing)
        {
            float r = ratio;
            if (jitter > 0.0f)
                r *= 1.0f + jitter * 0.9f * (jitterRng.nextFloat() * 2.0f - 1.0f);
            r = juce::jmax (r, 1.0e-4f);

            phase += (double) r;
            tick = phase >= 1.0;
            if (tick)
                phase -= std::floor (phase);
        }

        for (int ch = 0; ch < numChannels; ++ch)
        {
            const size_t c = (size_t) ch;
            const float dry = channelData[ch][i];

            float x = dry * drive;

            // Modo Wrap deja pasar el overflow; los otros limitan suavemente
            if (mode != 2)
                x = softClip (x);

            // El anti-alias corre siempre que haya reduccion de rate (mantiene su estado limpio)
            if (aaOn && reducing)
                x = aaFilterB[c].process (aaFilterA[c].process (x));

            if (tick)
            {
                float s = x;

                if (mode == 2)
                    s = wrapSample (s);

                // Dither TPDF (hasta +-1 LSB)
                if (dither > 0.0f)
                    s += (ditherRng[c].nextFloat() - ditherRng[c].nextFloat()) * dither * invLevels;

                float q;
                if (mode == 1)
                    q = std::floor (s * levels);          // truncado: suena mas "sucio" y asimetrico
                else
                    q = std::floor (s * levels + 0.5f);   // redondeo

                held[c] = juce::jlimit (-1.0f, 1.0f, q * invLevels);
            }

            float wet = held[c];

            if (toneOn)
                wet = toneFilter[c].process (wet);

            // Bloqueo de DC
            const float y = wet - dcX[c] + dcCoeff * dcY[c];
            dcX[c] = wet;
            dcY[c] = y;
            wet = y;

            channelData[ch][i] = (dry * (1.0f - mix) + wet * mix) * outGain;
        }
    }
}

juce::AudioProcessorEditor* CrusherProcessor::createEditor()
{
    return new CrusherEditor (*this);
}

void CrusherProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void CrusherProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CrusherProcessor();
}
