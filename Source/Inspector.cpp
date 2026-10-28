#include "Inspector.h"

Inspector::Inspector()
{
    title.setText("Inspector", juce::dontSendNotification);
    addAndMakeVisible(title);
    bind(volume, &Track::volume, 0.0, 1.0, 0.01);
    bind(pan, &Track::pan, -1.0, 1.0, 0.01);
    bind(low, &Track::lowDb, -12.0, 12.0, 0.1);
    bind(mid, &Track::midDb, -12.0, 12.0, 0.1);
    bind(high, &Track::highDb, -12.0, 12.0, 0.1);
    bind(threshold, &Track::compThreshold, 0.05, 1.0, 0.01);
    bind(ratio, &Track::compRatio, 1.0, 12.0, 0.1);

    for (auto* button : { &pluginButton, &showButton, &clearButton })
        addAndMakeVisible(button);

    pluginButton.onClick = [this] { if (onLoadPlugin) onLoadPlugin(); };
    showButton.onClick = [this] { if (onShowPlugin) onShowPlugin(); };
    clearButton.onClick = [this] { if (onClearPlugin) onClearPlugin(); };
}

void Inspector::bind(juce::Slider& slider, float Track::* field, double start, double end, double step)
{
    slider.setRange(start, end, step);
    slider.setSliderStyle(juce::Slider::LinearHorizontal);
    slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 48, 18);
    slider.onValueChange = [this, &slider, field]
    {
        if (updating || project == nullptr || project->selectedTrack < 0 || project->selectedTrack >= (int) project->tracks.size())
            return;
        if (lock != nullptr)
        {
            juce::ScopedLock sl(*lock);
            project->tracks[(size_t) project->selectedTrack].*field = (float) slider.getValue();
        }
        else
            project->tracks[(size_t) project->selectedTrack].*field = (float) slider.getValue();
        if (onChanged)
            onChanged();
    };
    addAndMakeVisible(slider);
}

void Inspector::setProject(Project* newProject)
{
    project = newProject;
    refresh();
}

void Inspector::refresh()
{
    updating = true;
    if (project != nullptr && project->selectedTrack >= 0 && project->selectedTrack < (int) project->tracks.size())
    {
        const auto& track = project->tracks[(size_t) project->selectedTrack];
        title.setText(track.name, juce::dontSendNotification);
        volume.setValue(track.volume, juce::dontSendNotification);
        pan.setValue(track.pan, juce::dontSendNotification);
        low.setValue(track.lowDb, juce::dontSendNotification);
        mid.setValue(track.midDb, juce::dontSendNotification);
        high.setValue(track.highDb, juce::dontSendNotification);
        threshold.setValue(track.compThreshold, juce::dontSendNotification);
        ratio.setValue(track.compRatio, juce::dontSendNotification);
    }
    updating = false;
    repaint();
}

void Inspector::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1c1f22));
    g.setColour(juce::Colour(0xff9aa0a6));
    g.setFont(11.0f);
    const char* names[] = { "Vol", "Pan", "Low", "Mid", "High", "Thr", "Ratio" };
    juce::Slider* sliders[] = { &volume, &pan, &low, &mid, &high, &threshold, &ratio };
    for (int i = 0; i < 7; ++i)
        g.drawText(names[i], 8, sliders[i]->getY(), 42, sliders[i]->getHeight(), juce::Justification::centredLeft);
}

void Inspector::resized()
{
    auto area = getLocalBounds().reduced(8);
    title.setBounds(area.removeFromTop(24));
    for (auto* slider : { &volume, &pan, &low, &mid, &high, &threshold, &ratio })
    {
        auto row = area.removeFromTop(28);
        slider->setBounds(row.withTrimmedLeft(40));
    }
    area.removeFromTop(8);
    pluginButton.setBounds(area.removeFromTop(28));
    area.removeFromTop(4);
    showButton.setBounds(area.removeFromTop(28));
    area.removeFromTop(4);
    clearButton.setBounds(area.removeFromTop(28));
}
