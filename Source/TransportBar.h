#pragma once
#include <JuceHeader.h>

class TransportBar : public juce::Component,
                     private juce::Button::Listener,
                     private juce::Slider::Listener
{
public:
    std::function<void()> onPlay;
    std::function<void()> onStop;
    std::function<void(double)> onTempoChanged;

    TransportBar();
    void setPlaying(bool playing);
    void setBeat(double beat);
    void setBpm(double bpm);
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void buttonClicked(juce::Button*) override;
    void sliderValueChanged(juce::Slider*) override;

    juce::TextButton playButton { "Play" };
    juce::TextButton stopButton { "Stop" };
    juce::Slider tempoSlider;
    juce::Label tempoLabel;
    juce::Label positionLabel;
};
