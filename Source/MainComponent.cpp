#include "MainComponent.h"

class AudioSettingsWindow : public juce::DocumentWindow
{
public:
    explicit AudioSettingsWindow(juce::AudioDeviceManager& devices)
        : DocumentWindow("Audio devices", juce::Colour(0xff23272b), DocumentWindow::closeButton)
    {
        setUsingNativeTitleBar(true);
        setContentOwned(new juce::AudioDeviceSelectorComponent(devices, 0, 2, 0, 2, true, true, true, false), true);
        centreWithSize(540, 600);
        setResizable(true, true);
        setVisible(true);
    }

    void closeButtonPressed() override { setVisible(false); }
};

MainComponent::MainComponent()
    : project(Project::demo())
{
    setLookAndFeel(&lookAndFeel);
    engine.setProject(&project);
    trackList.setProject(&project);
    arrangement.setProject(&project);
    mixer.setProject(&project);
    transport.setBpm(project.bpm);

    transport.onPlay = [this]
    {
        engine.setPlaying(!engine.isPlaying());
        transport.setPlaying(engine.isPlaying());
    };
    transport.onStop = [this]
    {
        if (engine.isRecording())
            toggleRecord();
        engine.setPlaying(false);
        engine.setBeat(0.0);
        transport.setPlaying(false);
        arrangement.setBeat(0.0);
    };
    transport.onRecord = [this] { toggleRecord(); };
    transport.onAudio = [this] { showAudioSettings(); };
    transport.onTempoChanged = [this](double bpm) { engine.setBpm(bpm); };
    arrangement.onSeek = [this](double beat)
    {
        engine.setBeat(beat);
        arrangement.setBeat(beat);
    };
    trackList.onChanged = [this] { arrangement.repaint(); };

    addAndMakeVisible(transport);
    addAndMakeVisible(trackList);
    addAndMakeVisible(arrangement);
    addAndMakeVisible(mixer);

    setSize(1440, 900);
    startTimerHz(30);
}

MainComponent::~MainComponent()
{
    setLookAndFeel(nullptr);
}

void MainComponent::toggleRecord()
{
    if (engine.isRecording())
    {
        auto clip = engine.stopRecording();
        transport.setRecording(false);
        if (clip.audio != nullptr && project.armedTrack >= 0 && project.armedTrack < (int) project.tracks.size())
        {
            project.tracks[(size_t) project.armedTrack].clips.push_back(std::move(clip));
            arrangement.repaint();
        }
        return;
    }

    engine.startRecording();
    transport.setRecording(true);
    transport.setPlaying(true);
}

void MainComponent::showAudioSettings()
{
    if (audioSettings == nullptr)
        audioSettings.reset(new AudioSettingsWindow(engine.getDeviceManager()));
    audioSettings->setVisible(true);
    audioSettings->toFront(true);
}

void MainComponent::timerCallback()
{
    transport.setBeat(engine.getBeat());
    arrangement.setBeat(engine.getBeat());
    if (!engine.isRecording())
        transport.setRecording(false);
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1a1c1e));
}

void MainComponent::resized()
{
    auto area = getLocalBounds();
    transport.setBounds(area.removeFromTop(48));
    mixer.setBounds(area.removeFromBottom(210));
    trackList.setBounds(area.removeFromLeft(220));
    arrangement.setBounds(area);
}
