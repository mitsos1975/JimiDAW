#include "MainComponent.h"

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
        engine.setPlaying(false);
        engine.setBeat(0.0);
        transport.setPlaying(false);
    };
    transport.onTempoChanged = [this](double bpm) { engine.setBpm(bpm); };

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

void MainComponent::timerCallback()
{
    transport.setBeat(engine.getBeat());
    arrangement.setBeat(engine.getBeat());
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
