#include "TrackList.h"

void TrackList::setProject(Project* newProject)
{
    project = newProject;
    repaint();
}

void TrackList::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff181818));
    g.setColour(juce::Colour(0xff8a8a8a));
    g.setFont(juce::FontOptions(11.0f));
    g.drawText("Band Tracks", 10, 0, getWidth() - 16, header, juce::Justification::centredLeft);
    g.setColour(juce::Colour(0xff0c0c0c));
    g.drawLine(0.0f, (float) header - 1.0f, (float) getWidth(), (float) header - 1.0f);
    if (project == nullptr)
        return;

    for (size_t i = 0; i < project->tracks.size(); ++i)
    {
        auto row = juce::Rectangle<int>(0, header + (int) i * rowHeight, getWidth(), rowHeight);
        const bool selected = project->selectedTrack == (int) i;
        g.setColour(selected ? juce::Colour(0xff2a3138) : juce::Colour(i % 2 == 0 ? 0xff1c1c1c : 0xff161616));
        g.fillRect(row);
        const auto tone = project->tracks[i].colour;
        g.setColour(tone);
        g.fillRect(row.removeFromLeft(8));
        if (selected)
        {
            g.setColour(juce::Colour(0xffd0d0d0).withAlpha(0.35f));
            g.drawRect(0, header + (int) i * rowHeight, getWidth(), rowHeight);
        }

        auto text = row.reduced(8, 6);
        g.setColour(juce::Colour(0xffe6e6e6));
        g.setFont(juce::FontOptions(13.0f));
        g.drawText(project->tracks[i].name, text.removeFromTop(20), juce::Justification::centredLeft);

        auto buttons = text.removeFromTop(18);
        auto drawKey = [&](juce::Rectangle<int> box, const juce::String& letter, bool active, juce::Colour activeColour)
        {
            g.setColour(active ? activeColour : juce::Colour(0xff2a2a2a));
            g.fillRect(box);
            g.setColour(juce::Colour(0xff111111));
            g.drawRect(box);
            g.setColour(active ? juce::Colour(0xfff4f4f4) : juce::Colour(0xff8d8d8d));
            g.setFont(juce::FontOptions(11.0f));
            g.drawText(letter, box, juce::Justification::centred);
        };
        drawKey(buttons.removeFromLeft(20), "M", project->tracks[i].mute, juce::Colour(0xff8a3038));
        buttons.removeFromLeft(4);
        drawKey(buttons.removeFromLeft(20), "S", project->tracks[i].solo, juce::Colour(0xff8a7030));
        buttons.removeFromLeft(4);
        drawKey(buttons.removeFromLeft(20), "R", project->tracks[i].armed, juce::Colour(0xff8a3030));
    }
}

void TrackList::mouseDown(const juce::MouseEvent& event)
{
    if (project == nullptr || event.y < header)
        return;
    const int index = (event.y - header) / rowHeight;
    if (index < 0 || index >= (int) project->tracks.size())
        return;

    const int localY = (event.y - header) - index * rowHeight;
    auto apply = [this, index, localY, x = event.x]
    {
        auto& track = project->tracks[(size_t) index];
        project->selectedTrack = index;
        if (localY > 28)
        {
            if (x < 70)
                track.mute = !track.mute;
            else if (x < 94)
                track.solo = !track.solo;
            else if (x < 118)
            {
                for (auto& other : project->tracks)
                    other.armed = false;
                track.armed = true;
                project->armedTrack = index;
            }
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
