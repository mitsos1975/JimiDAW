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

void ArrangementView::drawWaveform(juce::Graphics& g, const Clip& clip, juce::Rectangle<float> box)
{
    if (clip.audio == nullptr || clip.audio->getNumSamples() < 2)
        return;

    const int samples = clip.audio->getNumSamples();
    const int width = juce::jmax(1, (int) box.getWidth());
    juce::Path path;
    bool started = false;
    for (int x = 0; x < width; x += 2)
    {
        const int start = (int) ((double) x / width * samples);
        const int end = juce::jmin(samples, (int) ((double) (x + 2) / width * samples));
        float peak = 0.0f;
        for (int i = start; i < end; ++i)
            peak = juce::jmax(peak, std::abs(clip.audio->getSample(0, i)));

        const float y = box.getCentreY() - peak * box.getHeight() * 0.45f;
        if (!started)
        {
            path.startNewSubPath(box.getX() + (float) x, y);
            started = true;
        }
        else
        {
            path.lineTo(box.getX() + (float) x, y);
        }
    }
    g.setColour(juce::Colour(0xff111315).withAlpha(0.85f));
    g.strokePath(path, juce::PathStrokeType(1.0f));
}

void ArrangementView::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff16181b));
    g.setColour(juce::Colour(0xff2a2e32));
    for (int x = 0; x < getWidth(); x += (int) (pixelsPerBeat * 4))
        g.drawVerticalLine(x, (float) header, (float) getHeight());

    g.setColour(juce::Colour(0xff9aa0a6));
    g.setFont(11.0f);
    for (int bar = 0; bar < 64; ++bar)
        g.drawText(juce::String(bar + 1), (int) (bar * pixelsPerBeat * 4) + 4, 4, 40, header - 8,
                   juce::Justification::centredLeft);

    if (project == nullptr)
        return;

    for (size_t i = 0; i < project->tracks.size(); ++i)
    {
        const int y = header + (int) i * TrackList::rowHeight;
        g.setColour(juce::Colour(0xff22262a).withAlpha(i % 2 == 0 ? 1.0f : 0.65f));
        g.fillRect(0, y, getWidth(), TrackList::rowHeight - 1);

        for (const auto& clip : project->tracks[i].clips)
        {
            auto box = juce::Rectangle<float>((float) (clip.startBeat * pixelsPerBeat),
                                              (float) y + 8.0f,
                                              (float) (clip.lengthBeats * pixelsPerBeat),
                                              (float) TrackList::rowHeight - 16.0f);
            g.setColour(project->tracks[i].colour.withAlpha(clip.audio != nullptr ? 0.95f : 0.75f));
            g.fillRoundedRectangle(box, 4.0f);
            drawWaveform(g, clip, box);
            g.setColour(juce::Colours::black.withAlpha(0.35f));
            g.drawRoundedRectangle(box, 4.0f, 1.0f);
            g.setColour(juce::Colour(0xff111315));
            g.drawText(clip.name, box.reduced(6.0f, 0.0f).removeFromTop(16.0f), juce::Justification::centredLeft);
        }
    }

    const float playhead = (float) (beat * pixelsPerBeat);
    g.setColour(juce::Colour(0xfff0a202));
    g.drawLine(playhead, 0.0f, playhead, (float) getHeight(), 2.0f);
}

void ArrangementView::mouseDown(const juce::MouseEvent& event)
{
    if (onSeek)
        onSeek(juce::jmax(0.0, event.x / pixelsPerBeat));
}
