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
    void mouseDown(const juce::MouseEvent&) override;

    static constexpr int columnWidth = 92;

private:
    void rebuild();

    Project* project = nullptr;
    juce::OwnedArray<juce::Slider> faders;
};
