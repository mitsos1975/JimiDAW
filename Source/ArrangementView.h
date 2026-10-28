#pragma once
#include <JuceHeader.h>
#include "Project.h"

class ArrangementView : public juce::Component
{
public:
    std::function<void(double)> onSeek;
    std::function<void()> onChanged;
    juce::CriticalSection* lock = nullptr;

    void setProject(Project* newProject);
    void setBeat(double newBeat);
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

    static constexpr int header = 28;

private:
    void drawWaveform(juce::Graphics&, const Clip&, juce::Rectangle<float>);
    int trackAt(int y) const;
    int clipAt(int trackIndex, int x, bool& edge) const;
    double xToBeat(int x) const;

    Project* project = nullptr;
    double beat = 0.0;
    double originBeat = 0.0;
    double pixelsPerBeat = 32.0;
    int dragTrack = -1;
    int dragClip = -1;
    bool dragEdge = false;
    double dragOrigin = 0.0;
};
