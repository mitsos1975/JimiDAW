#include "TrackList.h"

void TrackList::setProject(Project* newProject)
{
    project = newProject;
    repaint();
}

void TrackList::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1c1f22));
    g.setColour(juce::Colour(0xff9aa0a6));
    g.setFont(12.0f);
    g.drawText("Tracks", 8, 0, getWidth() - 16, header, juce::Justification::centredLeft);
    if (project == nullptr)
        return;

    for (size_t i = 0; i < project->tracks.size(); ++i)
    {
        auto row = juce::Rectangle<int>(0, header + (int) i * rowHeight, getWidth(), rowHeight);
        g.setColour(project->selectedTrack == (int) i ? juce::Colour(0xff2c333a) : (i % 2 == 0 ? juce::Colour(0xff22262a) : juce::Colour(0xff1a1d20)));
        g.fillRect(row);
        g.setColour(project->tracks[i].colour);
        g.fillRect(row.removeFromLeft(4));
        auto text = row.reduced(8, 6);
        g.setColour(juce::Colour(0xffe8eaed));
        g.setFont(14.0f);
        g.drawText(project->tracks[i].name, text.removeFromTop(20), juce::Justification::centredLeft);
        auto buttons = text.removeFromTop(18);
        g.setFont(12.0f);
        g.setColour(project->tracks[i].mute ? juce::Colour(0xffe85d75) : juce::Colour(0xff9aa0a6));
        g.drawText(project->tracks[i].mute ? "M" : "m", buttons.removeFromLeft(22), juce::Justification::centred);
        g.setColour(project->tracks[i].solo ? juce::Colour(0xfff0a202) : juce::Colour(0xff9aa0a6));
        g.drawText(project->tracks[i].solo ? "S" : "s", buttons.removeFromLeft(22), juce::Justification::centred);
        g.setColour(project->tracks[i].armed ? juce::Colour(0xffff4d4d) : juce::Colour(0xff9aa0a6));
        g.drawText(project->tracks[i].armed ? "R" : "r", buttons.removeFromLeft(22), juce::Justification::centred);
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
    {
        juce::ScopedLock sl(lock != nullptr ? *lock : juce::CriticalSection());
        if (lock == nullptr) {}
        auto& track = project->tracks[(size_t) index];
        project->selectedTrack = index;
        if (localY > 28)
        {
            if (event.x < 70)
                track.mute = !track.mute;
            else if (event.x < 92)
                track.solo = !track.solo;
            else if (event.x < 114)
            {
                for (auto& other : project->tracks)
                    other.armed = false;
                track.armed = true;
                project->armedTrack = index;
            }
        }
    }
    repaint();
    if (onChanged)
        onChanged();
}
