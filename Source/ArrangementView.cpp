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

void ArrangementView::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff16181b));
    g.setColour(juce::Colour(0xff2a2e32));
    for (int x = 0; x < getWidth(); x += (int) (pixelsPerBeat * 4))
        g.drawVerticalLine(x, (float) header, (float) getHeight());

    g.setColour(juce::Colour(0xff9aa0a6));
    g.setFont(11.0f);
    for (int bar = 0; bar < 32; ++bar)
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
            g.setColour(project->tracks[i].colour.withAlpha(0.85f));
            g.fillRoundedRectangle(box, 4.0f);
            g.setColour(juce::Colours::black.withAlpha(0.35f));
            g.drawRoundedRectangle(box, 4.0f, 1.0f);
            g.setColour(juce::Colour(0xff111315));
            g.drawText(clip.name, box.reduced(6.0f, 0.0f), juce::Justification::centredLeft);
        }
    }

    const float playhead = (float) (beat * pixelsPerBeat);
    g.setColour(juce::Colour(0xfff0a202));
    g.drawLine(playhead, 0.0f, playhead, (float) getHeight(), 2.0f);
}
