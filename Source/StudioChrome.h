#pragma once
#include <JuceHeader.h>
#include "Project.h"

class MediaRack : public juce::Component
{
public:
    std::function<void(int)> onTile;
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
};

class ToolStrip : public juce::Component
{
public:
    int tool = 0;
    std::function<void(int)> onCommand;
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
};

class ChannelEq : public juce::Component
{
public:
    std::function<void()> onChanged;
    std::function<void()> onLoadPlugin;
    juce::CriticalSection* lock = nullptr;

    ChannelEq();
    void setProject(Project* newProject);
    void refresh();
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void bind(juce::Slider& slider, float Track::* field, double start, double end);

    Project* project = nullptr;
    bool updating = false;
    juce::Slider low, mid, high, send, volume;
    juce::TextButton pluginButton { "Load VST3" };
};
