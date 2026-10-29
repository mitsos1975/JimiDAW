#include "TrackList.h"

void TrackList::setProject(Project* newProject)
{
    project = newProject;
    repaint();
}

void TrackList::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1c1e22));
    g.setColour(juce::Colour(0xff2a2d31));
    g.fillRect(0, 0, getWidth(), header);
    g.setColour(juce::Colour(0xffd0d0d0));
    g.setFont(juce::FontOptions(12.0f));
    g.drawText("Band Tracks", 28, 0, getWidth() - 36, header, juce::Justification::centredLeft);
    if (project == nullptr)
        return;

    for (size_t i = 0; i < project->tracks.size(); ++i)
    {
        const int y = header + (int) i * rowHeight;
        const bool selected = project->selectedTrack == (int) i;
        g.setColour(selected ? juce::Colour(0xff31404c) : juce::Colour(i % 2 ? 0xff23262a : 0xff1e2125));
        g.fillRect(0, y, getWidth(), rowHeight - 1);
        g.setColour(project->tracks[i].colour);
        g.fillRect(0, y, 6, rowHeight - 1);

        g.setColour(juce::Colour(0xff9aa0a6));
        g.setFont(juce::FontOptions(11.0f));
        g.drawText(juce::String((int) i + 1), 10, y, 18, rowHeight, juce::Justification::centredLeft);

        g.setColour(selected ? juce::Colours::white : juce::Colour(0xffd5d5d5));
        g.setFont(juce::FontOptions(12.5f));
        g.drawText(project->tracks[i].name, 30, y, getWidth() - 108, rowHeight, juce::Justification::centredLeft, true);

        auto drawKey = [&](int x, const juce::String& letter, bool active, juce::Colour on)
        {
            auto box = juce::Rectangle<int>(x, y + 6, 16, rowHeight - 12);
            g.setColour(active ? on : juce::Colour(0xff2c3036));
            g.fillRoundedRectangle(box.toFloat(), 2.0f);
            g.setColour(active ? juce::Colours::white : juce::Colour(0xff9aa0a6));
            g.setFont(juce::FontOptions(10.0f));
            g.drawText(letter, box, juce::Justification::centred);
        };
        drawKey(getWidth() - 62, "M", project->tracks[i].mute, juce::Colour(0xffc4a21a));
        drawKey(getWidth() - 42, "S", project->tracks[i].solo, juce::Colour(0xff2f8fd0));
        drawKey(getWidth() - 22, "R", project->tracks[i].armed, juce::Colour(0xffc44747));
    }
}

void TrackList::mouseDown(const juce::MouseEvent& event)
{
    if (project == nullptr || event.y < header)
        return;
    const int index = (event.y - header) / rowHeight;
    if (index < 0 || index >= (int) project->tracks.size())
        return;

    auto apply = [this, index, x = event.x]
    {
        auto& track = project->tracks[(size_t) index];
        project->selectedTrack = index;
        if (x >= getWidth() - 62 && x < getWidth() - 44)
            track.mute = !track.mute;
        else if (x >= getWidth() - 42 && x < getWidth() - 24)
            track.solo = !track.solo;
        else if (x >= getWidth() - 22)
        {
            for (auto& other : project->tracks)
                other.armed = false;
            track.armed = true;
            project->armedTrack = index;
        }
    };

    if (lock != nullptr)
    {
        juce::ScopedLock sl(*lock);
        apply();
    }
    else
        apply();

    repaint();
    if (onChanged)
        onChanged();
}
