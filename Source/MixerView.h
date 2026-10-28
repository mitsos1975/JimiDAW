#pragma once
#include <JuceHeader.h>
#include "AudioEngine.h"
#include "Project.h"

class MixerView : public juce::Component,
                  private juce::Timer
{
public:
    std::function<void()> onChanged;

    void setProject(Project* newProject);
    void setEngine(AudioEngine* newEngine);
    void setLevels(const float* peaks, int count, float master);
    void resized() override;
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;

    static constexpr int columnWidth = 104;

private:
    void rebuild();
    void paintMeter(juce::Graphics&, juce::Rectangle<int>, float level) const;
    void timerCallback() override;

    Project* project = nullptr;
    AudioEngine* engine = nullptr;
    juce::OwnedArray<juce::Slider> faders;
    juce::OwnedArray<juce::Slider> pans;
    float levels[65] {};
    float masterLevel = 0.0f;
};
