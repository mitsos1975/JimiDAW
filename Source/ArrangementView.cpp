#include "ArrangementView.h"
#include "TrackList.h"

void ArrangementView::setProject(Project* newProject)
{
    project = newProject;
    repaint();
}

void ArrangementView::setBeat(double newBeat)
{
    beat = newBeat;
    repaint();
}

double ArrangementView::xToBeat(int x) const
{
    return originBeat + x / pixelsPerBeat;
}

int ArrangementView::trackAt(int y) const
{
    if (project == nullptr || y < header)
        return -1;
    const int index = (y - header) / TrackList::rowHeight;
    return index >= 0 && index < (int) project->tracks.size() ? index : -1;
}

int ArrangementView::clipAt(int trackIndex, int x, bool& edge) const
{
    edge = false;
    if (project == nullptr || trackIndex < 0)
        return -1;
    const auto& clips = project->tracks[(size_t) trackIndex].clips;
    for (int i = (int) clips.size() - 1; i >= 0; --i)
    {
        const float left = (float) ((clips[(size_t) i].startBeat - originBeat) * pixelsPerBeat);
        const float right = left + (float) (clips[(size_t) i].lengthBeats * pixelsPerBeat);
        if (x >= left && x <= right)
        {
            edge = x >= right - 8.0f;
            return i;
        }
    }
    return -1;
}

void ArrangementView::drawWaveform(juce::Graphics& g, const Clip& clip, juce::Rectangle<float> box)
{
    if (clip.audio == nullptr || clip.audio->getNumSamples() < 2)
        return;
    const int samples = clip.audio->getNumSamples();
    const int width = juce::jmax(1, (int) box.getWidth());
    juce::Path path;
    for (int x = 0; x < width; x += 2)
    {
        const int start = (int) ((double) x / width * samples);
        const int end = juce::jmin(samples, start + juce::jmax(1, samples / width));
        float peak = 0.0f;
        for (int i = start; i < end; ++i)
            peak = juce::jmax(peak, std::abs(clip.audio->getSample(0, i)));
        const float y = box.getCentreY() - peak * box.getHeight() * 0.45f;
        if (x == 0)
            path.startNewSubPath(box.getX(), y);
        else
            path.lineTo(box.getX() + (float) x, y);
    }
    g.setColour(juce::Colour(0xff111315).withAlpha(0.85f));
    g.strokePath(path, juce::PathStrokeType(1.0f));
}

void ArrangementView::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff16181b));
    g.setColour(juce::Colour(0xff2a2e32));
    for (double bar = std::floor(originBeat / 4.0) * 4.0; (bar - originBeat) * pixelsPerBeat < getWidth(); bar += 4.0)
    {
        const int x = (int) ((bar - originBeat) * pixelsPerBeat);
        g.drawVerticalLine(x, (float) header, (float) getHeight());
        g.setColour(juce::Colour(0xff9aa0a6));
        g.setFont(11.0f);
        g.drawText(juce::String((int) (bar / 4.0) + 1), x + 4, 4, 40, header - 8, juce::Justification::centredLeft);
        g.setColour(juce::Colour(0xff2a2e32));
    }

    if (project == nullptr)
        return;

    for (size_t i = 0; i < project->tracks.size(); ++i)
    {
        const int y = header + (int) i * TrackList::rowHeight;
        g.setColour(juce::Colour(0xff22262a).withAlpha(i % 2 == 0 ? 1.0f : 0.65f));
        g.fillRect(0, y, getWidth(), TrackList::rowHeight - 1);
        for (int c = 0; c < (int) project->tracks[i].clips.size(); ++c)
        {
            const auto& clip = project->tracks[i].clips[(size_t) c];
            auto box = juce::Rectangle<float>((float) ((clip.startBeat - originBeat) * pixelsPerBeat),
                                              (float) y + 8.0f,
                                              (float) (clip.lengthBeats * pixelsPerBeat),
                                              (float) TrackList::rowHeight - 16.0f);
            if (box.getRight() < 0 || box.getX() > getWidth())
                continue;
            g.setColour(project->tracks[i].colour.withAlpha(clip.audio != nullptr ? 0.95f : 0.75f));
            g.fillRoundedRectangle(box, 4.0f);
            drawWaveform(g, clip, box);
            g.setColour(project->selectedTrack == (int) i && project->selectedClip == c ? juce::Colour(0xffffd166) : juce::Colours::black.withAlpha(0.35f));
            g.drawRoundedRectangle(box, 4.0f, project->selectedClip == c ? 2.0f : 1.0f);
            g.setColour(juce::Colour(0xff111315));
            g.drawText(clip.name, box.reduced(6.0f, 0.0f).removeFromTop(16.0f), juce::Justification::centredLeft);
        }

        for (const auto& note : project->tracks[i].notes)
        {
            auto box = juce::Rectangle<float>((float) ((note.startBeat - originBeat) * pixelsPerBeat),
                                              (float) y + TrackList::rowHeight - 18.0f,
                                              (float) juce::jmax(3.0, note.lengthBeats * pixelsPerBeat),
                                              8.0f);
            g.setColour(juce::Colour(0xffffd166).withAlpha(0.9f));
            g.fillRect(box);
        }
    }

    if (project->loopEnabled)
    {
        const float a = (float) ((project->loopStart - originBeat) * pixelsPerBeat);
        const float b = (float) ((project->loopEnd - originBeat) * pixelsPerBeat);
        g.setColour(juce::Colour(0xff3d8bfd).withAlpha(0.18f));
        g.fillRect(a, 0.0f, b - a, (float) header);
    }

    const float playhead = (float) ((beat - originBeat) * pixelsPerBeat);
    g.setColour(juce::Colour(0xfff0a202));
    g.drawLine(playhead, 0.0f, playhead, (float) getHeight(), 2.0f);
}

void ArrangementView::mouseDown(const juce::MouseEvent& event)
{
    grabKeyboardFocus();
    const int track = trackAt(event.y);
    bool edge = false;
    const int clip = clipAt(track, event.x, edge);
    if (project != nullptr && track >= 0)
        project->selectedTrack = track;

    if (event.mods.isPopupMenu() && clip >= 0)
    {
        if (lock != nullptr)
        {
            juce::ScopedLock sl(*lock);
            project->tracks[(size_t) track].clips.erase(project->tracks[(size_t) track].clips.begin() + clip);
        }
        project->selectedClip = -1;
        if (onChanged) onChanged();
        repaint();
        return;
    }

    if (clip >= 0)
    {
        project->selectedClip = clip;
        dragTrack = track;
        dragClip = clip;
        dragEdge = edge;
        const auto& item = project->tracks[(size_t) track].clips[(size_t) clip];
        dragOrigin = edge ? item.lengthBeats : item.startBeat;
        repaint();
        return;
    }

    dragClip = -1;
    if (project != nullptr)
        project->selectedClip = -1;
    if (onSeek)
        onSeek(juce::jmax(0.0, xToBeat(event.x)));
    if (onChanged) onChanged();
}

void ArrangementView::mouseDrag(const juce::MouseEvent& event)
{
    if (project == nullptr || dragClip < 0 || dragTrack < 0)
        return;
    auto& clips = project->tracks[(size_t) dragTrack].clips;
    if (dragClip >= (int) clips.size())
        return;
    const double delta = event.getDistanceFromDragStartX() / pixelsPerBeat;
    const double value = juce::jmax(dragEdge ? 0.25 : 0.0, std::round((dragOrigin + delta) * 4.0) / 4.0);
    if (lock != nullptr)
    {
        juce::ScopedLock sl(*lock);
        if (dragEdge)
            clips[(size_t) dragClip].lengthBeats = value;
        else
            clips[(size_t) dragClip].startBeat = value;
    }
    repaint();
}

void ArrangementView::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    if (event.mods.isShiftDown())
        originBeat = juce::jmax(0.0, originBeat - wheel.deltaY * 8.0);
    else
        pixelsPerBeat = juce::jlimit(6.0, 180.0, pixelsPerBeat * (wheel.deltaY > 0 ? 1.12 : 0.89));
    repaint();
}
