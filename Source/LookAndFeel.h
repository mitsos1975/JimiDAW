#pragma once
#include <JuceHeader.h>

class DawLookAndFeel : public juce::LookAndFeel_V4
{
public:
    DawLookAndFeel();
    void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
};
