#include "MainComponent.h"
#include "Session.h"

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

class PluginWindow : public juce::DocumentWindow
{
public:
    explicit PluginWindow(juce::AudioProcessor& processor)
        : DocumentWindow(processor.getName(), juce::Colour(0xff23272b), DocumentWindow::closeButton)
    {
        setUsingNativeTitleBar(true);
        if (auto* editor = processor.createEditorIfNeeded())
            setContentOwned(editor, true);
        else
            setContentOwned(new juce::GenericAudioProcessorEditor(processor), true);
        setResizable(true, true);
        centreWithSize(juce::jmax(320, getWidth()), juce::jmax(200, getHeight()));
        setVisible(true);
    }

    void closeButtonPressed() override { setVisible(false); }
};

MainComponent::MainComponent()
    : project(Project::demo())
{
    setLookAndFeel(&lookAndFeel);
    setWantsKeyboardFocus(true);
    engine.setProject(&project);
    const auto lock = &engine.getLock();
    trackList.lock = lock;
    arrangement.lock = lock;
    piano.lock = lock;
    inspector.lock = lock;

    trackList.setProject(&project);
    arrangement.setProject(&project);
    mixer.setProject(&project);
    mixer.setEngine(&engine);
    piano.setProject(&project);
    inspector.setProject(&project);
    transport.setBpm(project.bpm);

    transport.onPlay = [this]
    {
        engine.setPlaying(!engine.isPlaying());
        transport.setPlaying(engine.isPlaying());
    };
    transport.onStop = [this]
    {
        if (engine.isRecording())
            commitRecording();
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
    trackList.onChanged = [this] { refreshEditors(); };
    arrangement.onChanged = [this] { refreshEditors(); };
    piano.onChanged = [this] { arrangement.repaint(); };
    inspector.onChanged = [this] { arrangement.repaint(); mixer.repaint(); };
    inspector.onLoadPlugin = [this]
    {
        pluginWindow.reset();
        chooseFile("Load VST3", "*.vst3",
                   juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::canSelectDirectories,
                   [this](const juce::File& file)
                   {
                       const auto error = engine.loadPlugin(project.selectedTrack, file);
                       if (error.isNotEmpty())
                           juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Plugin", error);
                       else
                           inspector.refresh();
                   });
    };
    inspector.onShowPlugin = [this]
    {
        pluginWindow.reset();
        if (auto* plugin = engine.getPlugin(project.selectedTrack))
            pluginWindow.reset(new PluginWindow(*plugin));
    };
    inspector.onClearPlugin = [this]
    {
        pluginWindow.reset();
        engine.clearPlugin(project.selectedTrack);
    };

    saveButton.onClick = [this]
    {
        chooseFile("Save project", "*.jimi.json", juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting,
                   [this](const juce::File& chosen)
                   {
                       auto file = chosen.hasFileExtension("jimi.json") ? chosen : chosen.withFileExtension("jimi.json");
                       const auto result = Session::save(project, file);
                       if (result.failed())
                           juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Save", result.getErrorMessage());
                   });
    };
    openButton.onClick = [this]
    {
        chooseFile("Open project", "*.jimi.json", juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                   [this](const juce::File& file)
                   {
                       Project loaded;
                       const auto result = Session::load(loaded, file);
                       if (result.failed())
                       {
                           juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Open", result.getErrorMessage());
                           return;
                       }
                       {
                           juce::ScopedLock sl(engine.getLock());
                           project = std::move(loaded);
                       }
                       engine.setProject(&project);
                       for (int i = 0; i < (int) project.tracks.size(); ++i)
                           if (project.tracks[(size_t) i].pluginPath.isNotEmpty())
                               engine.loadPlugin(i, juce::File(project.tracks[(size_t) i].pluginPath));
                       transport.setBpm(project.bpm);
                       refreshEditors();
                   });
    };
    importButton.onClick = [this]
    {
        chooseFile("Import audio", "*.wav;*.aiff;*.mp3;*.flac", juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                   [this](const juce::File& file)
                   {
                       double rate = 44100.0;
                       auto audio = Session::readMonoWav(file, rate);
                       if (audio == nullptr)
                           return;
                       Clip clip;
                       clip.startBeat = engine.getBeat();
                       clip.sampleRate = rate;
                       clip.audio = audio;
                       clip.name = file.getFileNameWithoutExtension();
                       clip.lengthBeats = juce::jmax(0.25, (audio->getNumSamples() / rate) * (project.bpm / 60.0));
                       const int track = juce::jlimit(0, (int) project.tracks.size() - 1, project.selectedTrack);
                       {
                           juce::ScopedLock sl(engine.getLock());
                           project.tracks[(size_t) track].clips.push_back(std::move(clip));
                       }
                       arrangement.repaint();
                   });
    };
    exportButton.onClick = [this]
    {
        chooseFile("Export WAV", "*.wav", juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting,
                   [this](const juce::File& chosen)
                   {
                       auto file = chosen.hasFileExtension("wav") ? chosen : chosen.withFileExtension("wav");
                       const auto result = engine.exportWav(file);
                       if (result.failed())
                           juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Export", result.getErrorMessage());
                   });
    };
    addTrackButton.onClick = [this]
    {
        {
            juce::ScopedLock sl(engine.getLock());
            project.tracks.push_back(Project::makeTrack((int) project.tracks.size()));
        }
        mixer.setProject(&project);
        refreshEditors();
    };
    pianoButton.onClick = [this]
    {
        showPiano = !showPiano;
        pianoButton.setColour(juce::TextButton::buttonColourId, showPiano ? juce::Colour(0xff3d5a80) : juce::Colour(0xff2e3236));
        resized();
    };
    loopButton.onClick = [this]
    {
        project.loopEnabled = !project.loopEnabled;
        if (project.loopEnd <= project.loopStart)
            project.loopEnd = project.loopStart + 8.0;
        loopButton.setColour(juce::TextButton::buttonColourId, project.loopEnabled ? juce::Colour(0xff3d5a80) : juce::Colour(0xff2e3236));
        arrangement.repaint();
    };
    clickButton.onClick = [this]
    {
        project.metronome = !project.metronome;
        clickButton.setColour(juce::TextButton::buttonColourId, project.metronome ? juce::Colour(0xff3d5a80) : juce::Colour(0xff2e3236));
    };
    autoButton.onClick = [this]
    {
        project.automationMode = !project.automationMode;
        autoButton.setColour(juce::TextButton::buttonColourId, project.automationMode ? juce::Colour(0xff3d5a80) : juce::Colour(0xff2e3236));
        arrangement.repaint();
    };
    splitButton.onClick = [this] { splitSelectedClip(); };
    copyButton.onClick = [this] { duplicateSelectedClip(); };
    quantButton.onClick = [this] { quantizeSelectedNotes(); };
    markButton.onClick = [this] { addMarker(); };
    mixButton.onClick = [this]
    {
        showMixer = !showMixer;
        if (showMixer)
        {
            showPiano = false;
            showChannel = false;
        }
        resized();
    };
    eqButton.onClick = [this]
    {
        showChannel = !showChannel;
        if (showChannel)
            showMixer = false;
        channelEq.refresh();
        resized();
    };

    for (auto* button : { &saveButton, &openButton, &importButton, &exportButton, &addTrackButton, &pianoButton, &loopButton, &clickButton, &autoButton, &splitButton, &copyButton, &quantButton, &markButton, &mixButton, &eqButton })
    {
        addAndMakeVisible(button);
        button->setVisible(false);
    }
    addAndMakeVisible(transport);
    addAndMakeVisible(trackList);
    addAndMakeVisible(arrangement);
    addAndMakeVisible(mixer);
    addAndMakeVisible(piano);
    addAndMakeVisible(inspector);
    addAndMakeVisible(menuBar);
    addAndMakeVisible(toolStrip);
    addAndMakeVisible(mediaRack);
    addAndMakeVisible(channelEq);
    channelEq.setVisible(false);
    channelEq.setProject(&project);
    channelEq.lock = &engine.getLock();
    channelEq.onChanged = [this] { refreshEditors(); };
    channelEq.onLoadPlugin = [this] { if (inspector.onLoadPlugin) inspector.onLoadPlugin(); };
    mediaRack.onTile = [this](int tile)
    {
        if ((tile == 0 || tile == 1) && inspector.onLoadPlugin)
            inspector.onLoadPlugin();
        else if (tile == 2 || tile == 5)
            importButton.triggerClick();
        else if (tile == 3)
            pianoButton.triggerClick();
        else if (tile == 6)
            eqButton.triggerClick();
    };
    toolStrip.onCommand = [this](int id)
    {
        if (id < 3)
            project.tool = id;
        else if (id == 3)
            splitSelectedClip();
        else if (id == 4)
            quantizeSelectedNotes();
        else if (id == 5)
            arrangement.nudgeZoom(1.25f);
        else if (id == 6)
            arrangement.nudgeZoom(0.8f);
        else if (id == 7)
            mixButton.triggerClick();
        else if (id == 8)
            eqButton.triggerClick();
    };
    piano.setVisible(false);
    clickButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff3d5a80));

    setSize(1440, 900);
    startTimerHz(30);
}

MainComponent::~MainComponent()
{
    setLookAndFeel(nullptr);
}

void MainComponent::chooseFile(const juce::String& title, const juce::String& filter, int flags,
                               const std::function<void(const juce::File&)>& use)
{
    chooser = std::make_unique<juce::FileChooser>(title, juce::File::getSpecialLocation(juce::File::userDocumentsDirectory), filter);
    chooser->launchAsync(flags, [use](const juce::FileChooser& dialog)
    {
        const auto file = dialog.getResult();
        if (file != juce::File())
            use(file);
    });
}

void MainComponent::refreshEditors()
{
    inspector.refresh();
    arrangement.repaint();
    trackList.repaint();
    piano.repaint();
    mixer.repaint();
    channelEq.refresh();
}


void MainComponent::splitSelectedClip()
{
    if (project.selectedTrack < 0 || project.selectedTrack >= (int) project.tracks.size())
        return;
    auto& clips = project.tracks[(size_t) project.selectedTrack].clips;
    if (project.selectedClip < 0 || project.selectedClip >= (int) clips.size())
        return;

    const double beat = engine.getBeat();
    juce::ScopedLock sl(engine.getLock());
    auto& clip = clips[(size_t) project.selectedClip];
    if (beat <= clip.startBeat + 0.05 || beat >= clip.startBeat + clip.lengthBeats - 0.05)
        return;

    Clip right = clip;
    const double leftLength = beat - clip.startBeat;
    right.startBeat = beat;
    right.lengthBeats = clip.lengthBeats - leftLength;
    right.name = clip.name + " b";
    right.sourceOffset = clip.sourceOffset + leftLength * 60.0 / juce::jmax(40.0, project.bpm);
    clip.lengthBeats = leftLength;
    clips.push_back(std::move(right));
    arrangement.repaint();
}


void MainComponent::duplicateSelectedClip()
{
    if (project.selectedTrack < 0 || project.selectedTrack >= (int) project.tracks.size())
        return;
    auto& clips = project.tracks[(size_t) project.selectedTrack].clips;
    if (project.selectedClip < 0 || project.selectedClip >= (int) clips.size())
        return;
    juce::ScopedLock sl(engine.getLock());
    Clip copy = clips[(size_t) project.selectedClip];
    copy.startBeat += copy.lengthBeats;
    copy.name = copy.name + " copy";
    clips.push_back(std::move(copy));
    arrangement.repaint();
}

void MainComponent::quantizeSelectedNotes()
{
    if (project.selectedTrack < 0 || project.selectedTrack >= (int) project.tracks.size())
        return;
    juce::ScopedLock sl(engine.getLock());
    for (auto& note : project.tracks[(size_t) project.selectedTrack].notes)
        note.startBeat = std::round(note.startBeat * 4.0) / 4.0;
    refreshEditors();
}

void MainComponent::addMarker()
{
    Marker marker;
    marker.beat = std::round(engine.getBeat() * 4.0) / 4.0;
    marker.name = "M" + juce::String((int) project.markers.size() + 1);
    project.markers.push_back(marker);
    arrangement.repaint();
}

void MainComponent::toggleRecord()
{
    if (engine.isRecording())
    {
        commitRecording();
        return;
    }
    engine.startRecording();
    transport.setRecording(true);
    transport.setPlaying(true);
    sawRecording = true;
}

void MainComponent::commitRecording()
{
    auto clip = engine.stopRecording();
    transport.setRecording(false);
    sawRecording = false;
    if (clip.audio == nullptr)
        return;
    const int track = juce::jlimit(0, (int) project.tracks.size() - 1, project.armedTrack);
    {
        juce::ScopedLock sl(engine.getLock());
        project.tracks[(size_t) track].clips.push_back(std::move(clip));
    }
    arrangement.repaint();
}

void MainComponent::showAudioSettings()
{
    if (audioSettings == nullptr)
        audioSettings.reset(new AudioSettingsWindow(engine.getDeviceManager()));
    audioSettings->setVisible(true);
    audioSettings->toFront(true);
}

bool MainComponent::keyPressed(const juce::KeyPress& key)
{
    if (key == juce::KeyPress::spaceKey)
    {
        engine.setPlaying(!engine.isPlaying());
        transport.setPlaying(engine.isPlaying());
        return true;
    }

    if (key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey)
    {
        juce::ScopedLock sl(engine.getLock());
        if (showPiano && project.selectedNote >= 0 && project.selectedTrack >= 0 && project.selectedTrack < (int) project.tracks.size())
        {
            auto& notes = project.tracks[(size_t) project.selectedTrack].notes;
            if (project.selectedNote < (int) notes.size())
                notes.erase(notes.begin() + project.selectedNote);
            project.selectedNote = -1;
        }
        else if (project.selectedClip >= 0 && project.selectedTrack >= 0 && project.selectedTrack < (int) project.tracks.size())
        {
            auto& clips = project.tracks[(size_t) project.selectedTrack].clips;
            if (project.selectedClip < (int) clips.size())
                clips.erase(clips.begin() + project.selectedClip);
            project.selectedClip = -1;
        }
        refreshEditors();
        return true;
    }

    const juce::String keys = "awsedftgyhujkolp";
    const int index = keys.indexOfChar(juce::CharacterFunctions::toLowerCase(key.getTextCharacter()));
    if (index >= 0 && project.selectedTrack >= 0 && project.selectedTrack < (int) project.tracks.size())
    {
        const int semis[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
        MidiNote note;
        note.note = 60 + semis[index];
        note.startBeat = std::round(engine.getBeat() * 4.0) / 4.0;
        note.lengthBeats = 1.0;
        {
            juce::ScopedLock sl(engine.getLock());
            project.tracks[(size_t) project.selectedTrack].notes.push_back(note);
        }
        refreshEditors();
        return true;
    }
    return false;
}

juce::StringArray MainComponent::getMenuBarNames()
{
    return { "File", "Edit", "Project", "Audio", "MIDI", "Transport", "Window", "Help" };
}

juce::PopupMenu MainComponent::getMenuForIndex(int index, const juce::String&)
{
    juce::PopupMenu menu;
    if (index == 0)
    {
        menu.addItem(1, "Save...");
        menu.addItem(2, "Open...");
        menu.addItem(3, "Import Audio...");
        menu.addItem(4, "Export WAV...");
        menu.addSeparator();
        menu.addItem(5, "Quit");
    }
    else if (index == 1)
    {
        menu.addItem(10, "Split at Playhead");
        menu.addItem(11, "Duplicate Clip");
        menu.addItem(12, "Quantize");
    }
    else if (index == 2)
        menu.addItem(20, "Add Track");
    else if (index == 3)
        menu.addItem(30, "Audio Connections...");
    else if (index == 4)
        menu.addItem(40, "Piano Roll");
    else if (index == 5)
    {
        menu.addItem(50, "Play");
        menu.addItem(51, "Stop");
        menu.addItem(52, "Record");
        menu.addItem(53, "Loop");
        menu.addItem(54, "Metronome");
        menu.addItem(55, "Add Marker");
    }
    else if (index == 6)
    {
        menu.addItem(60, "Project");
        menu.addItem(61, "MixConsole");
        menu.addItem(62, "Channel Settings");
    }
    else if (index == 7)
        menu.addItem(70, "About JimiDAW");
    return menu;
}

void MainComponent::menuItemSelected(int id, int)
{
    if (id == 1) saveButton.triggerClick();
    if (id == 2) openButton.triggerClick();
    if (id == 3) importButton.triggerClick();
    if (id == 4) exportButton.triggerClick();
    if (id == 5) juce::JUCEApplication::getInstance()->systemRequestedQuit();
    if (id == 10) splitSelectedClip();
    if (id == 11) duplicateSelectedClip();
    if (id == 12) quantizeSelectedNotes();
    if (id == 20) addTrackButton.triggerClick();
    if (id == 30) showAudioSettings();
    if (id == 40) pianoButton.triggerClick();
    if (id == 50) transport.onPlay();
    if (id == 51) transport.onStop();
    if (id == 52) transport.onRecord();
    if (id == 53) loopButton.triggerClick();
    if (id == 54) clickButton.triggerClick();
    if (id == 55) addMarker();
    if (id == 60) { showMixer = false; showChannel = false; resized(); }
    if (id == 61) mixButton.triggerClick();
    if (id == 62) eqButton.triggerClick();
    if (id == 70)
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, "JimiDAW",
            "JimiDAW is a Windows DAW with a Cubase-style project window, MixConsole and channel EQ. It is not Cubase.");
}

void MainComponent::timerCallback()
{
    transport.setBeat(engine.getBeat());
    arrangement.setBeat(engine.getBeat());
    if (sawRecording && !engine.isRecording())
        commitRecording();
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1b1d21));
    g.setColour(juce::Colour(0xff141618));
    g.fillRect(0, 54, getWidth(), 34);
    g.setColour(juce::Colour(0xff9aa0a6));
    g.setFont(juce::FontOptions(11.0f));
    g.drawText("Record Format   44.1 kHz - 24 bit      Project Pan Law  -3 dB",
               0, 54, getWidth(), 16, juce::Justification::centred);
    juce::String object = "No Object Selected";
    if (project.selectedClip >= 0)
        object = "Part";
    else if (project.selectedTrack >= 0 && project.selectedTrack < (int) project.tracks.size() && project.tool != 0)
        object = project.tracks[(size_t) project.selectedTrack].name;
    g.setColour(juce::Colour(0xffc8c8c8));
    g.drawText(object, 0, 70, getWidth(), 16, juce::Justification::centred);
}

void MainComponent::resized()
{
    auto area = getLocalBounds();
    menuBar.setBounds(area.removeFromTop(22));
    toolStrip.setBounds(area.removeFromTop(32));
    area.removeFromTop(34);
    transport.setBounds(area.removeFromBottom(40));

    const bool projectView = !showMixer;
    inspector.setVisible(projectView);
    mediaRack.setVisible(projectView && !showChannel);
    trackList.setVisible(projectView);
    arrangement.setVisible(projectView && !showChannel);
    channelEq.setVisible(projectView && showChannel);
    mixer.setVisible(showMixer);
    piano.setVisible(projectView && showPiano && !showChannel);

    if (showMixer)
    {
        mixer.setBounds(area);
        return;
    }

    inspector.setBounds(area.removeFromLeft(236));
    if (!showChannel)
        mediaRack.setBounds(area.removeFromRight(228));
    if (showPiano && !showChannel)
        piano.setBounds(area.removeFromBottom(240));
    trackList.setBounds(area.removeFromLeft(248));
    if (showChannel)
        channelEq.setBounds(area);
    else
        arrangement.setBounds(area);
}
