#pragma once
#include <JuceHeader.h>
#include "AudioEngine.h"
#include "LookAndFeel.h"
#include "TransportBar.h"
#include "TrackList.h"
#include "ArrangementView.h"
#include "MixerView.h"
#include "Project.h"

class AudioSettingsWindow;

class MainComponent : public juce::Component,
                      private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void toggleRecord();
    void showAudioSettings();

    DawLookAndFeel lookAndFeel;
    Project project;
    AudioEngine engine;
    TransportBar transport;
    TrackList trackList;
    ArrangementView arrangement;
    MixerView mixer;
    std::unique_ptr<AudioSettingsWindow> audioSettings;
};
