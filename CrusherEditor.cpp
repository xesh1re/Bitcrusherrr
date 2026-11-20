#include "CrusherEditor.h"

//==============================================================================
CrusherLookAndFeel::CrusherLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xffd8dde6));
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Label::textColourId, juce::Colour (0xff9aa4b2));

    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff1b2027));
    setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff2c333d));
    setColour (juce::ComboBox::textColourId, juce::Colour (0xffd8dde6));
    setColour (juce::ComboBox::arrowColourId, accent());

    setColour (juce::ToggleButton::textColourId, juce::Colour (0xffd8dde6));
    setColour (juce::ToggleButton::tickColourId, accent());
    setColour (juce::ToggleButton::tickDisabledColourId, juce::Colour (0xff2c333d));
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
    g.setColour (juce::Colour (0xff1b2027));
    g.fillEllipse (centre.x - bodyRadius, centre.y - bodyRadius, bodyRadius * 2.0f, bodyRadius * 2.0f);

    // Riel
    juce::Path track;
    track.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                         rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (juce::Colour (0xff2c333d));
    g.strokePath (track, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));

    // Valor
    juce::Path value;
    value.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                         rotaryStartAngle, angle, true);
    g.setColour (accent());
    g.strokePath (value, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));

    // Aguja
    juce::Path pointer;
    const float len = bodyRadius * 0.55f;
    pointer.addRoundedRectangle (-1.5f, -bodyRadius + 4.0f, 3.0f, len, 1.5f);
    pointer.applyTransform (juce::AffineTransform::rotation (angle).translated (centre.x, centre.y));
    g.setColour (juce::Colour (0xfff2f4f8));
    g.fillPath (pointer);
}

//==============================================================================
CrusherEditor::CrusherEditor (CrusherProcessor& p)
    : juce::AudioProcessorEditor (&p), processor (p)
{
    setLookAndFeel (&lookAndFeel);

    const char* ids[8]   = { "drive", "bits", "rate", "jitter", "dither", "tone", "mix", "output" };
    const char* names[8] = { "DRIVE", "BITS", "RATE", "JITTER", "DITHER", "TONE", "MIX", "OUTPUT" };

    for (size_t i = 0; i < knobs.size(); ++i)
    {
        auto& k = knobs[i];

        k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 90, 18);
        k.slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f,
                                      juce::MathConstants<float>::pi * 2.8f, true);
        addAndMakeVisible (k.slider);

        k.label.setText (names[i], juce::dontSendNotification);
        k.label.setJustificationType (juce::Justification::centred);
        k.label.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::bold)));
        addAndMakeVisible (k.label);

        k.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            processor.apvts, ids[i], k.slider);
    }

    modeLabel.setText ("MODE", juce::dontSendNotification);
    modeLabel.setJustificationType (juce::Justification::centred);
    modeLabel.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::bold)));
    addAndMakeVisible (modeLabel);

    modeBox.addItemList (processor.apvts.getParameter ("mode")->getAllValueStrings(), 1);
    addAndMakeVisible (modeBox);
    modeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        processor.apvts, "mode", modeBox);

    aaButton.setButtonText ("ANTI-ALIAS");
    addAndMakeVisible (aaButton);
    aaAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        processor.apvts, "aa", aaButton);

    setSize (640, 400);
}

CrusherEditor::~CrusherEditor()
{
    setLookAndFeel (nullptr);
}

void CrusherEditor::paint (juce::Graphics& g)
{
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff171b21), 0.0f, 0.0f,
                                             juce::Colour (0xff0c0e12), 0.0f, (float) getHeight(), false));
    g.fillAll();

    auto header = getLocalBounds().reduced (24, 0).removeFromTop (70);

    g.setColour (CrusherLookAndFeel::accent());
    g.setFont (juce::Font (juce::FontOptions (30.0f, juce::Font::bold)));
    g.drawText ("BROU CRUSHER", header.removeFromLeft (320), juce::Justification::centredLeft);

    g.setColour (juce::Colour (0xff6b7480));
    g.setFont (juce::Font (juce::FontOptions (13.0f)));
    g.drawText ("bit & sample rate destroyer", header, juce::Justification::centredRight);

    g.setColour (juce::Colour (0xff2c333d));
    g.drawLine (24.0f, 70.0f, (float) getWidth() - 24.0f, 70.0f, 1.0f);
}

void CrusherEditor::resized()
{
    auto area = getLocalBounds().reduced (20, 0);
    area.removeFromTop (78);
    area.removeFromBottom (16);

    auto row1 = area.removeFromTop (area.getHeight() / 2);
    auto row2 = area;

    const int colW = row1.getWidth() / 5;

    auto placeKnob = [] (Knob& k, juce::Rectangle<int> cell)
    {
        k.label.setBounds (cell.removeFromTop (18));
        k.slider.setBounds (cell.reduced (4, 0));
    };

    for (size_t i = 0; i < 5; ++i)
        placeKnob (knobs[i], row1.removeFromLeft (colW));

    for (size_t i = 5; i < 8; ++i)
        placeKnob (knobs[i], row2.removeFromLeft (colW));

    auto modeCell = row2.removeFromLeft (colW);
    modeLabel.setBounds (modeCell.removeFromTop (18));
    modeBox.setBounds (modeCell.withSizeKeepingCentre (colW - 24, 30));

    aaButton.setBounds (row2.withSizeKeepingCentre (colW - 16, 30));
}
