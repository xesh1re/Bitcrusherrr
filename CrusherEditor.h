#pragma once

#include "CrusherProcessor.h"
#include <array>
#include <memory>

// Aspecto oscuro con perillas rojas
class CrusherLookAndFeel : public juce::LookAndFeel_V4
{
public:
    CrusherLookAndFeel();

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider& slider) override;

    static juce::Colour accent() { return juce::Colour (0xffff2d2d); }
};

// Pantalla que dibuja una onda de ejemplo: gris = original, rojo = destruida
// segun los valores actuales de Bits y Rate.
class CrushDisplay : public juce::Component, private juce::Timer
{
public:
    explicit CrushDisplay (juce::AudioProcessorValueTreeState& s);
    ~CrushDisplay() override { stopTimer(); }

    void paint (juce::Graphics&) override;

private:
    void timerCallback() override { repaint(); }

    juce::AudioProcessorValueTreeState& state;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CrushDisplay)
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
        juce::Label label;   // nombre
        juce::Label desc;    // explicacion en lenguaje simple
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    CrusherProcessor& processor;
    CrusherLookAndFeel lookAndFeel;
    juce::TooltipWindow tooltipWindow { this, 400 };

    CrushDisplay display;
    std::array<Knob, 8> knobs;

    juce::Label modeLabel, modeDesc;
    juce::ComboBox modeBox;

    juce::Label aaLabel, aaDesc;
    juce::ToggleButton aaButton;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> aaAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CrusherEditor)
};
