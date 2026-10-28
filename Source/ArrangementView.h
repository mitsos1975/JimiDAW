#pragma once
#include <JuceHeader.h>
#include "Project.h"

class ArrangementView : public juce::Component
{
public:
    std::function<void(double)> onSeek;

    void setProject(Project* newProject);
    void setBeat(double newBeat);
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;

    static constexpr int header = 28;

private:
    void drawWaveform(juce::Graphics&, const Clip&, juce::Rectangle<float>);

    Project* project = nullptr;
    double beat = 0.0;
    double pixelsPerBeat = 32.0;
};
