#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <cmath>

// Filtro biquad (forma transpuesta II) hecho a mano para no reservar memoria
// en el hilo de audio.
struct Biquad
{
    float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;
    float z1 = 0.0f, z2 = 0.0f;

    void reset() { z1 = z2 = 0.0f; }

    void setLowpass (float cutoff, float sampleRate, float q)
    {
        const double fc = juce::jlimit (10.0, 0.49 * (double) sampleRate, (double) cutoff);
        const double w0 = 2.0 * juce::MathConstants<double>::pi * fc / (double) sampleRate;
        const double cosw = std::cos (w0);
        const double sinw = std::sin (w0);
        const double alpha = sinw / (2.0 * (double) q);
        const double a0 = 1.0 + alpha;

        b0 = (float) (((1.0 - cosw) * 0.5) / a0);
        b1 = (float) ((1.0 - cosw) / a0);
        b2 = b0;
        a1 = (float) ((-2.0 * cosw) / a0);
        a2 = (float) ((1.0 - alpha) / a0);
    }

    float process (float x)
    {
        const float y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }
};

class CrusherProcessor : public juce::AudioProcessor
{
public:
    CrusherProcessor();
    ~CrusherProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Brou Crusher"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    juce::LinearSmoothedValue<float> smoothDrive  { 1.0f };
    juce::LinearSmoothedValue<float> smoothBits   { 8.0f };
    juce::LinearSmoothedValue<float> smoothRate   { 12000.0f };
    juce::LinearSmoothedValue<float> smoothJitter { 0.0f };
    juce::LinearSmoothedValue<float> smoothDither { 0.0f };
    juce::LinearSmoothedValue<float> smoothMix    { 1.0f };
    juce::LinearSmoothedValue<float> smoothOutput { 1.0f };

    // Anti-alias de 4 polos (dos biquads en serie) y filtro de tono final
    std::array<Biquad, 2> aaFilterA, aaFilterB, toneFilter;

    // Sample & hold
    double phase = 1.0;
    std::array<float, 2> held { 0.0f, 0.0f };

    // Bloqueador de DC (la cuantizacion por truncado mete DC)
    std::array<float, 2> dcX { 0.0f, 0.0f }, dcY { 0.0f, 0.0f };
    float dcCoeff = 0.999f;

    std::array<juce::Random, 2> ditherRng;
    juce::Random jitterRng;

    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CrusherProcessor)
};
