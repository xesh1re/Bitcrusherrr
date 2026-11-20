#pragma once

#include "CrusherProcessor.h"
#include <array>
#include <memory>

// Aspecto oscuro con perillas de arco naranja
class CrusherLookAndFeel : public juce::LookAndFeel_V4
{
public:
    CrusherLookAndFeel();

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider& slider) override;

    static juce::Colour accent() { return juce::Colour (0xffff5a1f); }
};

class CrusherEditor : public juce::AudioProcessorEditor
{
public:
    explicit CrusherEditor (CrusherProcessor&);
    ~CrusherEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    CrusherProcessor& processor;
    CrusherLookAndFeel lookAndFeel;

    std::array<Knob, 8> knobs;

    juce::Label modeLabel;
    juce::ComboBox modeBox;
    juce::ToggleButton aaButton;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> aaAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CrusherEditor)
};
