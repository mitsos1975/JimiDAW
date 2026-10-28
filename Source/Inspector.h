#pragma once
#include <JuceHeader.h>
#include "Project.h"

class Inspector : public juce::Component
{
public:
    std::function<void()> onChanged;
    std::function<void()> onLoadPlugin;
    std::function<void()> onShowPlugin;
    std::function<void()> onClearPlugin;
    juce::CriticalSection* lock = nullptr;

    Inspector();
    void setProject(Project* newProject);
    void refresh();
    void resized() override;
    void paint(juce::Graphics&) override;

private:
    void bind(juce::Slider& slider, float Track::* field, double start, double end, double step);

    Project* project = nullptr;
    bool updating = false;
    juce::Label title;
    juce::Slider volume, pan, low, mid, high, threshold, ratio;
    juce::TextButton pluginButton { "Load VST3" };
    juce::TextButton showButton { "Show plugin" };
    juce::TextButton clearButton { "Clear plugin" };
};
