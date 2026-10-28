#pragma once
#include <JuceHeader.h>
#include "AudioEngine.h"
#include "LookAndFeel.h"
#include "TransportBar.h"
#include "TrackList.h"
#include "ArrangementView.h"
#include "MixerView.h"
#include "PianoRoll.h"
#include "Inspector.h"
#include "Project.h"

class AudioSettingsWindow;
class PluginWindow;

class MainComponent : public juce::Component,
                      private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress&) override;

private:
    void timerCallback() override;
    void toggleRecord();
    void commitRecording();
    void showAudioSettings();
    void refreshEditors();
    void chooseFile(const juce::String& title, const juce::String& filter, int flags,
                    const std::function<void(const juce::File&)>& use);

    DawLookAndFeel lookAndFeel;
    Project project;
    AudioEngine engine;
    TransportBar transport;
    TrackList trackList;
    ArrangementView arrangement;
    MixerView mixer;
    PianoRoll piano;
    Inspector inspector;
    juce::TextButton saveButton { "Save" };
    juce::TextButton openButton { "Open" };
    juce::TextButton importButton { "Import" };
    juce::TextButton exportButton { "Export" };
    juce::TextButton addTrackButton { "+ Track" };
    juce::TextButton pianoButton { "Piano" };
    juce::TextButton loopButton { "Loop" };
    juce::TextButton clickButton { "Click" };
    std::unique_ptr<AudioSettingsWindow> audioSettings;
    std::unique_ptr<PluginWindow> pluginWindow;
    std::unique_ptr<juce::FileChooser> chooser;
    bool showPiano = false;
    bool sawRecording = false;
};
