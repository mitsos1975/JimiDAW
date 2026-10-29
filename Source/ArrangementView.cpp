#include "ArrangementView.h"
#include <algorithm>
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
        const float half = peak * box.getHeight() * 0.42f;
        const float cx = box.getX() + (float) x;
        g.drawLine(cx, box.getCentreY() - half, cx, box.getCentreY() + half, 1.0f);
        juce::ignoreUnused(path);
    }
}

void ArrangementView::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff121212));
    g.setColour(juce::Colour(0xff1c1c1c));
    g.fillRect(0, 0, getWidth(), header);
    g.setColour(juce::Colour(0xff0a0a0a));
    g.drawLine(0.0f, (float) header - 1.0f, (float) getWidth(), (float) header - 1.0f);

    g.setFont(juce::FontOptions(11.0f));
    for (double bar = std::floor(originBeat / 4.0) * 4.0; (bar - originBeat) * pixelsPerBeat < getWidth(); bar += 1.0)
    {
        const int x = (int) ((bar - originBeat) * pixelsPerBeat);
        const bool downbeat = std::fmod(bar, 4.0) < 0.001;
        g.setColour(downbeat ? juce::Colour(0xff303030) : juce::Colour(0xff222222));
        g.drawVerticalLine(x, (float) header, (float) getHeight());
        if (downbeat)
        {
            g.setColour(juce::Colour(0xffc8c8c8));
            g.drawText(juce::String((int) (bar / 4.0) + 1), x + 4, 2, 36, header - 4, juce::Justification::centredLeft);
        }
    }

    if (project == nullptr)
        return;

    for (size_t i = 0; i < project->tracks.size(); ++i)
    {
        const int y = header + (int) i * TrackList::rowHeight;
        g.setColour(juce::Colour(i % 2 == 0 ? 0xff161616 : 0xff131313));
        g.fillRect(0, y, getWidth(), TrackList::rowHeight - 1);
        g.setColour(juce::Colour(0xff0e0e0e));
        g.drawLine(0.0f, (float) (y + TrackList::rowHeight - 1), (float) getWidth(), (float) (y + TrackList::rowHeight - 1));

        const auto tone = project->tracks[i].colour.withSaturation(0.40f).withBrightness(0.55f);
        for (int c = 0; c < (int) project->tracks[i].clips.size(); ++c)
        {
            const auto& clip = project->tracks[i].clips[(size_t) c];
            auto box = juce::Rectangle<float>((float) ((clip.startBeat - originBeat) * pixelsPerBeat),
                                              (float) y + 6.0f,
                                              (float) (clip.lengthBeats * pixelsPerBeat),
                                              (float) TrackList::rowHeight - 12.0f);
            if (box.getRight() < 0 || box.getX() > getWidth())
                continue;

            g.setColour(tone.darker(0.55f));
            g.fillRect(box);
            g.setColour(tone);
            g.fillRect(box.removeFromTop(14.0f));
            auto body = box;
            g.setColour(juce::Colour(0xffd8e2ea).withAlpha(0.85f));
            drawWaveform(g, clip, body.reduced(1.0f, 2.0f));

            const bool selected = project->selectedTrack == (int) i && project->selectedClip == c;
            g.setColour(selected ? juce::Colour(0xfff2f2f2) : juce::Colour(0xff000000).withAlpha(0.55f));
            g.drawRect(juce::Rectangle<float>((float) ((clip.startBeat - originBeat) * pixelsPerBeat),
                                              (float) y + 6.0f,
                                              (float) (clip.lengthBeats * pixelsPerBeat),
                                              (float) TrackList::rowHeight - 12.0f),
                       selected ? 1.5f : 1.0f);
            g.setColour(juce::Colour(0xff111111));
            g.setFont(juce::FontOptions(11.0f));
            g.drawText(clip.name,
                       juce::Rectangle<float>((float) ((clip.startBeat - originBeat) * pixelsPerBeat) + 6.0f,
                                              (float) y + 6.0f, box.getWidth(), 14.0f),
                       juce::Justification::centredLeft, true);
        }

        if (!project->tracks[i].volumeAutomation.empty() || project->automationMode)
        {
            auto points = project->tracks[i].volumeAutomation;
            std::sort(points.begin(), points.end(), [](const AutomationPoint& a, const AutomationPoint& b)
            {
                return a.beat < b.beat;
            });
            juce::Path curve;
            const float top = (float) y + 6.0f;
            const float bottom = (float) y + TrackList::rowHeight - 8.0f;
            auto pointY = [&](float value)
            {
                return bottom - juce::jlimit(0.0f, 1.0f, value) * (bottom - top);
            };
            if (points.empty())
            {
                g.setColour(juce::Colour(0xffd2b14a).withAlpha(0.45f));
                g.drawLine(0.0f, pointY(project->tracks[i].volume), (float) getWidth(), pointY(project->tracks[i].volume), 1.0f);
            }
            else
            {
                for (size_t n = 0; n < points.size(); ++n)
                {
                    const float x = (float) ((points[n].beat - originBeat) * pixelsPerBeat);
                    const float py = pointY(points[n].value);
                    if (n == 0)
                        curve.startNewSubPath(x, py);
                    else
                        curve.lineTo(x, py);
                    g.setColour(juce::Colour(0xfff2f2f2));
                    g.fillEllipse(x - 3.0f, py - 3.0f, 6.0f, 6.0f);
                }
                g.setColour(juce::Colour(0xffd2b14a).withAlpha(project->automationMode ? 0.95f : 0.55f));
                g.strokePath(curve, juce::PathStrokeType(1.5f));
            }
        }

        for (const auto& note : project->tracks[i].notes)
        {
            auto box = juce::Rectangle<float>((float) ((note.startBeat - originBeat) * pixelsPerBeat),
                                              (float) y + TrackList::rowHeight - 16.0f,
                                              (float) juce::jmax(3.0, note.lengthBeats * pixelsPerBeat),
                                              6.0f);
            g.setColour(tone.brighter(0.35f));
            g.fillRect(box);
        }
    }

    if (project->loopEnabled)
    {
        const float a = (float) ((project->loopStart - originBeat) * pixelsPerBeat);
        const float b = (float) ((project->loopEnd - originBeat) * pixelsPerBeat);
        g.setColour(juce::Colour(0xff8fb4ff).withAlpha(0.16f));
        g.fillRect(a, 0.0f, b - a, (float) header);
        g.setColour(juce::Colour(0xff8fb4ff));
        g.drawLine(a, 0.0f, a, (float) header, 2.0f);
        g.drawLine(b, 0.0f, b, (float) header, 2.0f);
    }

    for (const auto& marker : project->markers)
    {
        const float x = (float) ((marker.beat - originBeat) * pixelsPerBeat);
        g.setColour(juce::Colour(0xffe0b15a));
        g.fillRect(x, 0.0f, 2.0f, (float) header);
        g.setFont(juce::FontOptions(11.0f));
        g.drawText(marker.name, (int) x + 4, 2, 48, header - 4, juce::Justification::centredLeft, true);
    }

    const float playhead = (float) ((beat - originBeat) * pixelsPerBeat);
    g.setColour(juce::Colour(0xfff2f2f2));
    juce::Path flag;
    flag.addTriangle(playhead - 6.0f, 0.0f, playhead + 6.0f, 0.0f, playhead, 10.0f);
    g.fillPath(flag);
    g.drawLine(playhead, 0.0f, playhead, (float) getHeight(), 1.0f);
}

void ArrangementView::mouseDown(const juce::MouseEvent& event)
{
    grabKeyboardFocus();
    const int track = trackAt(event.y);
    dragPoint = -1;

    if (project != nullptr && event.y < header)
    {
        for (const auto& marker : project->markers)
        {
            const int x = (int) ((marker.beat - originBeat) * pixelsPerBeat);
            if (std::abs(x - event.x) <= 8)
            {
                if (onSeek)
                    onSeek(marker.beat);
                return;
            }
        }
    }

    if (project != nullptr && project->automationMode && track >= 0)
    {
        project->selectedTrack = track;
        auto& points = project->tracks[(size_t) track].volumeAutomation;
        const int rowY = header + track * TrackList::rowHeight;
        int nearest = -1;
        float best = 12.0f;
        for (int i = 0; i < (int) points.size(); ++i)
        {
            const float px = (float) ((points[(size_t) i].beat - originBeat) * pixelsPerBeat);
            const float py = (float) rowY + TrackList::rowHeight - 8.0f
                - juce::jlimit(0.0f, 1.0f, points[(size_t) i].value) * (TrackList::rowHeight - 14.0f);
            const float distance = std::hypot(px - (float) event.x, py - (float) event.y);
            if (distance < best)
            {
                best = distance;
                nearest = i;
            }
        }

        auto edit = [&]()
        {
            if (event.mods.isPopupMenu() && nearest >= 0)
            {
                points.erase(points.begin() + nearest);
                return;
            }
            if (nearest < 0)
            {
                AutomationPoint point;
                point.beat = juce::jmax(0.0, xToBeat(event.x));
                point.value = juce::jlimit(0.0f, 1.0f,
                    1.0f - (float) (event.y - rowY - 6) / (float) juce::jmax(1, TrackList::rowHeight - 14));
                points.push_back(point);
                nearest = (int) points.size() - 1;
            }
            dragPoint = nearest;
            dragTrack = track;
            dragClip = -1;
            dragOrigin = points[(size_t) nearest].beat;
            dragPointValue = points[(size_t) nearest].value;
        };

        if (lock != nullptr)
        {
            juce::ScopedLock sl(*lock);
            edit();
        }
        else
            edit();

        if (onChanged) onChanged();
        repaint();
        return;
    }

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

void ArrangementView::mouseUp(const juce::MouseEvent&)
{
    if (project == nullptr || dragPoint < 0 || dragTrack < 0 || dragTrack >= (int) project->tracks.size())
        return;
    auto& points = project->tracks[(size_t) dragTrack].volumeAutomation;
    std::sort(points.begin(), points.end(), [](const AutomationPoint& a, const AutomationPoint& b)
    {
        return a.beat < b.beat;
    });
    dragPoint = -1;
    repaint();
}

void ArrangementView::mouseDrag(const juce::MouseEvent& event)
{
    if (project != nullptr && dragPoint >= 0 && dragTrack >= 0 && dragTrack < (int) project->tracks.size())
    {
        auto& points = project->tracks[(size_t) dragTrack].volumeAutomation;
        if (dragPoint < (int) points.size())
        {
            const double beat = juce::jmax(0.0, dragOrigin + event.getDistanceFromDragStartX() / pixelsPerBeat);
            const float value = juce::jlimit(0.0f, 1.0f, dragPointValue - (float) event.getDistanceFromDragStartY() / (float) TrackList::rowHeight);
            if (lock != nullptr)
            {
                juce::ScopedLock sl(*lock);
                points[(size_t) dragPoint].beat = beat;
                points[(size_t) dragPoint].value = value;
            }
            else
            {
                points[(size_t) dragPoint].beat = beat;
                points[(size_t) dragPoint].value = value;
            }
        }
        repaint();
        return;
    }

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
