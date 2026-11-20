#include "CrusherEditor.h"

//==============================================================================
CrusherLookAndFeel::CrusherLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xfff5e6e6));
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Label::textColourId, juce::Colour (0xffc9a8a8));

    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff2a0a0c));
    setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff5a1a1f));
    setColour (juce::ComboBox::textColourId, juce::Colour (0xfff5e6e6));
    setColour (juce::ComboBox::arrowColourId, accent());

    setColour (juce::ToggleButton::textColourId, juce::Colour (0xfff5e6e6));
    setColour (juce::ToggleButton::tickColourId, accent());
    setColour (juce::ToggleButton::tickDisabledColourId, juce::Colour (0xff5a1a1f));

    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff2a0a0c));
    setColour (juce::PopupMenu::textColourId, juce::Colour (0xfff5e6e6));
    setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xff8a1218));
}

void CrusherLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                           juce::Slider&)
{
    const auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (8.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    const float arcRadius = radius - 4.0f;
    const float bodyRadius = arcRadius - 8.0f;

    // Cuerpo de la perilla
    g.setColour (juce::Colour (0xff2a0a0c));
    g.fillEllipse (centre.x - bodyRadius, centre.y - bodyRadius, bodyRadius * 2.0f, bodyRadius * 2.0f);
    g.setColour (juce::Colour (0xff4a1419));
    g.drawEllipse (centre.x - bodyRadius, centre.y - bodyRadius, bodyRadius * 2.0f, bodyRadius * 2.0f, 1.5f);

    // Riel
    juce::Path track;
    track.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                         rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (juce::Colour (0xff4a1a1e));
    g.strokePath (track, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));

    // Valor, con brillo
    juce::Path value;
    value.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                         rotaryStartAngle, angle, true);
    g.setColour (accent().withAlpha (0.25f));
    g.strokePath (value, juce::PathStrokeType (10.0f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));
    g.setColour (accent());
    g.strokePath (value, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));

    // Aguja
    juce::Path pointer;
    const float len = bodyRadius * 0.55f;
    pointer.addRoundedRectangle (-1.5f, -bodyRadius + 4.0f, 3.0f, len, 1.5f);
    pointer.applyTransform (juce::AffineTransform::rotation (angle).translated (centre.x, centre.y));
    g.setColour (juce::Colour (0xfffff1f1));
    g.fillPath (pointer);
}

//==============================================================================
CrushDisplay::CrushDisplay (juce::AudioProcessorValueTreeState& s)
    : state (s)
{
    startTimerHz (30);
}

void CrushDisplay::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat().reduced (1.0f);

    g.setColour (juce::Colour (0xff120405));
    g.fillRoundedRectangle (b, 8.0f);
    g.setColour (juce::Colour (0xff5a1a1f));
    g.drawRoundedRectangle (b, 8.0f, 1.0f);

    auto inner = b.reduced (12.0f, 8.0f);

    auto textRow = inner.removeFromTop (14.0f);
    g.setColour (juce::Colour (0xffb89a9a));
    g.setFont (juce::Font (juce::FontOptions (11.0f)));
    g.drawText ("Asi se ve tu sonido: gris = original, rojo = destruido (segun Bits y Rate)",
                textRow, juce::Justification::centredLeft);

    const auto wave = inner.reduced (0.0f, 2.0f);
    const float midY = wave.getCentreY();
    const float halfH = wave.getHeight() * 0.5f * 0.9f;

    g.setColour (juce::Colour (0xff3a1014));
    g.drawHorizontalLine ((int) midY, wave.getX(), wave.getRight());

    const float bits = state.getRawParameterValue ("bits")->load();
    const float rate = state.getRawParameterValue ("rate")->load();

    const float levels = std::exp2 (bits - 1.0f);
    const float ratio = juce::jmin (1.0f, rate / 48000.0f);

    constexpr int numPoints = 360;
    juce::Path original, crushed;
    double phase = 1.0;
    float held = 0.0f;

    for (int i = 0; i < numPoints; ++i)
    {
        const float t = (float) i / (float) (numPoints - 1);
        const float xPos = wave.getX() + t * wave.getWidth();
        const float s = std::sin (juce::MathConstants<float>::twoPi * 2.0f * t) * 0.9f;

        phase += (double) ratio;
        if (phase >= 1.0)
        {
            phase -= std::floor (phase);
            held = juce::jlimit (-1.0f, 1.0f, std::floor (s * levels + 0.5f) / levels);
        }

        const float yOriginal = midY - s * halfH;
        const float yCrushed = midY - held * halfH;

        if (i == 0)
        {
            original.startNewSubPath (xPos, yOriginal);
            crushed.startNewSubPath (xPos, yCrushed);
        }
        else
        {
            original.lineTo (xPos, yOriginal);
            crushed.lineTo (xPos, yCrushed);
        }
    }

    g.setColour (juce::Colour (0xff8a7474));
    g.strokePath (original, juce::PathStrokeType (1.5f));

    g.setColour (CrusherLookAndFeel::accent().withAlpha (0.25f));
    g.strokePath (crushed, juce::PathStrokeType (8.0f, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));
    g.setColour (CrusherLookAndFeel::accent());
    g.strokePath (crushed, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));
}

//==============================================================================
CrusherEditor::CrusherEditor (CrusherProcessor& p)
    : juce::AudioProcessorEditor (&p), processor (p), display (p.apvts)
{
    setLookAndFeel (&lookAndFeel);

    const char* ids[8]   = { "drive", "bits", "rate", "jitter", "dither", "tone", "mix", "output" };
    const char* names[8] = { "DRIVE", "BITS", "RATE", "JITTER", "DITHER", "TONE", "MIX", "OUTPUT" };

    const char* tips[8] =
    {
        "Sube el volumen ANTES de destruir el sonido. Mas drive = mas distorsion.",
        "Cuantos escalones de volumen puede usar el audio. Menos bits = sonido mas roto y con ruido.",
        "Cuantas veces por segundo se escucha el audio. Menos = suena mas viejo, metalico y de consola.",
        "Hace que el reloj tiemble. Mas jitter = sonido inestable, como un aparato roto.",
        "Agrega un poquito de ruido para suavizar la distorsion cuando usas pocos bits.",
        "Corta los agudos. Girala a la izquierda = sonido mas oscuro y menos filoso.",
        "Mezcla entre el sonido original (izquierda) y el destruido (derecha).",
        "Volumen final del plugin. Subelo si el efecto queda muy bajito."
    };

    addAndMakeVisible (display);

    for (size_t i = 0; i < knobs.size(); ++i)
    {
        auto& k = knobs[i];

        k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 90, 18);
        k.slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f,
                                      juce::MathConstants<float>::pi * 2.8f, true);
        k.slider.setTooltip (tips[i]);
        addAndMakeVisible (k.slider);

        k.label.setText (names[i], juce::dontSendNotification);
        k.label.setJustificationType (juce::Justification::centred);
        k.label.setFont (juce::Font (juce::FontOptions (13.0f, juce::Font::bold)));
        k.label.setColour (juce::Label::textColourId, juce::Colour (0xfff5e6e6));
        addAndMakeVisible (k.label);

        k.desc.setText (tips[i], juce::dontSendNotification);
        k.desc.setJustificationType (juce::Justification::centredTop);
        k.desc.setFont (juce::Font (juce::FontOptions (11.0f)));
        addAndMakeVisible (k.desc);

        k.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            processor.apvts, ids[i], k.slider);
    }

    // Mode
    modeLabel.setText ("MODE", juce::dontSendNotification);
    modeLabel.setJustificationType (juce::Justification::centred);
    modeLabel.setFont (juce::Font (juce::FontOptions (13.0f, juce::Font::bold)));
    modeLabel.setColour (juce::Label::textColourId, juce::Colour (0xfff5e6e6));
    addAndMakeVisible (modeLabel);

    modeDesc.setText ("Round = normal. Truncate = mas sucio. Wrap = caos: al pasarse del limite el sonido da la vuelta.",
                      juce::dontSendNotification);
    modeDesc.setJustificationType (juce::Justification::centredTop);
    modeDesc.setFont (juce::Font (juce::FontOptions (11.0f)));
    addAndMakeVisible (modeDesc);

    modeBox.addItemList (processor.apvts.getParameter ("mode")->getAllValueStrings(), 1);
    modeBox.setTooltip ("Elige como se redondea el sonido: Round (normal), Truncate (mas sucio) o Wrap (caos).");
    addAndMakeVisible (modeBox);
    modeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        processor.apvts, "mode", modeBox);

    // Anti-alias
    aaLabel.setText ("ANTI-ALIAS", juce::dontSendNotification);
    aaLabel.setJustificationType (juce::Justification::centred);
    aaLabel.setFont (juce::Font (juce::FontOptions (13.0f, juce::Font::bold)));
    aaLabel.setColour (juce::Label::textColourId, juce::Colour (0xfff5e6e6));
    addAndMakeVisible (aaLabel);

    aaDesc.setText ("Encendido = crunch mas limpio. Apagado = ruido metalico y aspero (el sonido clasico).",
                    juce::dontSendNotification);
    aaDesc.setJustificationType (juce::Justification::centredTop);
    aaDesc.setFont (juce::Font (juce::FontOptions (11.0f)));
    addAndMakeVisible (aaDesc);

    aaButton.setButtonText ("Activar");
    aaButton.setTooltip ("Filtra el sonido antes de bajarle la calidad, para que suene menos aspero.");
    addAndMakeVisible (aaButton);
    aaAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        processor.apvts, "aa", aaButton);

    setSize (820, 620);
}

CrusherEditor::~CrusherEditor()
{
    setLookAndFeel (nullptr);
}

void CrusherEditor::paint (juce::Graphics& g)
{
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff1e0709), 0.0f, 0.0f,
                                             juce::Colour (0xff070203), 0.0f, (float) getHeight(), false));
    g.fillAll();

    auto header = getLocalBounds().reduced (24, 0).removeFromTop (62);

    g.setColour (CrusherLookAndFeel::accent());
    g.setFont (juce::Font (juce::FontOptions (32.0f, juce::Font::bold)));
    g.drawText ("BROU CRUSHER", header.removeFromLeft (320), juce::Justification::centredLeft);

    g.setColour (juce::Colour (0xffb89a9a));
    g.setFont (juce::Font (juce::FontOptions (12.0f)));
    g.drawFittedText ("Un bitcrusher baja la calidad del audio a proposito: menos escalones de volumen (bits) y menos muestras por segundo (rate).",
                      header, juce::Justification::centredRight, 2);

    g.setColour (CrusherLookAndFeel::accent().withAlpha (0.6f));
    g.fillRect (24.0f, 64.0f, (float) getWidth() - 48.0f, 2.0f);
}

void CrusherEditor::resized()
{
    auto area = getLocalBounds().reduced (20, 0);
    area.removeFromTop (66);

    display.setBounds (area.removeFromTop (118).reduced (0, 4));
    area.removeFromTop (6);
    area.removeFromBottom (10);

    auto row1 = area.removeFromTop (area.getHeight() / 2);
    auto row2 = area;

    const int colW = row1.getWidth() / 5;

    auto placeKnob = [] (Knob& k, juce::Rectangle<int> cell)
    {
        k.label.setBounds (cell.removeFromTop (18));
        k.desc.setBounds (cell.removeFromBottom (66).reduced (4, 0));
        k.slider.setBounds (cell.reduced (6, 0));
    };

    for (size_t i = 0; i < 5; ++i)
        placeKnob (knobs[i], row1.removeFromLeft (colW));

    for (size_t i = 5; i < 8; ++i)
        placeKnob (knobs[i], row2.removeFromLeft (colW));

    auto modeCell = row2.removeFromLeft (colW);
    modeLabel.setBounds (modeCell.removeFromTop (18));
    modeDesc.setBounds (modeCell.removeFromBottom (66).reduced (4, 0));
    modeBox.setBounds (modeCell.withSizeKeepingCentre (colW - 24, 30));

    auto aaCell = row2.removeFromLeft (colW);
    aaLabel.setBounds (aaCell.removeFromTop (18));
    aaDesc.setBounds (aaCell.removeFromBottom (66).reduced (4, 0));
    aaButton.setBounds (aaCell.withSizeKeepingCentre (colW - 24, 30));
}
