#pragma once
#include <JuceHeader.h>
#include "Project.h"

class TrackList : public juce::Component
{
public:
    std::function<void()> onChanged;
    juce::CriticalSection* lock = nullptr;

    void setProject(Project* newProject);
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;

    static constexpr int rowHeight = 64;
    static constexpr int header = 28;

private:
    Project* project = nullptr;
};
