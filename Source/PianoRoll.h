#pragma once
#include <JuceHeader.h>
#include "Project.h"

class PianoRoll : public juce::Component
{
public:
    std::function<void()> onChanged;
    juce::CriticalSection* lock = nullptr;

    void setProject(Project* newProject);
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    void edit(const std::function<void()>& change);
    int noteAt(juce::Point<int> position) const;

    Project* project = nullptr;
    int lowestNote = 48;
    double pixelsPerBeat = 48.0;
    int dragNote = -1;
    double dragOrigin = 0.0;
    static constexpr int keyWidth = 48;
    static constexpr int rowHeight = 16;
};
