#pragma once
#include <JuceHeader.h>
#include "Project.h"

class MixerView : public juce::Component
{
public:
    std::function<void()> onChanged;

    void setProject(Project* newProject);
    void resized() override;
    void paint(juce::Graphics&) override;

private:
    void rebuild();

    Project* project = nullptr;
    juce::OwnedArray<juce::Slider> faders;
};
