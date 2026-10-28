#pragma once
#include <JuceHeader.h>
#include "Project.h"

class ArrangementView : public juce::Component
{
public:
    void setProject(Project* newProject);
    void setBeat(double newBeat);
    void paint(juce::Graphics&) override;

    static constexpr int header = 28;

private:
    Project* project = nullptr;
    double beat = 0.0;
    double pixelsPerBeat = 32.0;
};
