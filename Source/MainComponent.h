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
#include "StudioChrome.h"
#include "Project.h"

class AudioSettingsWindow;
class PluginWindow;

class MainComponent : public juce::Component,
                      private juce::Timer,
                      private juce::MenuBarModel
{
public:
    MainComponent();
    ~MainComponent() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress&) override;

private:
    void timerCallback() override;
    juce::StringArray getMenuBarNames() override;
    juce::PopupMenu getMenuForIndex(int, const juce::String&) override;
    void menuItemSelected(int, int) override;
    void toggleRecord();
    void commitRecording();
    void showAudioSettings();
    void refreshEditors();
    void splitSelectedClip();
    void duplicateSelectedClip();
    void quantizeSelectedNotes();
    void addMarker();
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
    juce::TextButton autoButton { "Auto" };
    juce::TextButton splitButton { "Split" };
    juce::TextButton copyButton { "Copy" };
    juce::TextButton quantButton { "Quant" };
    juce::TextButton markButton { "Mark" };
    juce::TextButton mixButton { "Mix" };
    juce::TextButton eqButton { "EQ" };
    juce::MenuBarComponent menuBar { this };
    ToolStrip toolStrip;
    MediaRack mediaRack;
    ChannelEq channelEq;
    std::unique_ptr<AudioSettingsWindow> audioSettings;
    std::unique_ptr<PluginWindow> pluginWindow;
    std::unique_ptr<juce::FileChooser> chooser;
    bool showPiano = false;
    bool showMixer = false;
    bool showChannel = false;
    bool sawRecording = false;
};
