#include "LookAndFeel.h"

DawLookAndFeel::DawLookAndFeel()
{
    setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(0xff1a1c1e));
    setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2e3236));
    setColour(juce::TextButton::textColourOffId, juce::Colour(0xffe8eaed));
    setColour(juce::Label::textColourId, juce::Colour(0xffe8eaed));
    setColour(juce::Slider::thumbColourId, juce::Colour(0xffd0d4d8));
    setColour(juce::Slider::trackColourId, juce::Colour(0xff3a3f44));
    setColour(juce::Slider::backgroundColourId, juce::Colour(0xff111315));
}

void DawLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                          const juce::Colour& backgroundColour,
                                          bool highlighted, bool down)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
    auto colour = backgroundColour.brighter(highlighted ? 0.12f : 0.0f).darker(down ? 0.18f : 0.0f);
    g.setColour(colour);
    g.fillRoundedRectangle(bounds, 4.0f);
    g.setColour(juce::Colours::black.withAlpha(0.45f));
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
}
