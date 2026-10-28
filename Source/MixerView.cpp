#include "MixerView.h"

void MixerView::setProject(Project* newProject)
{
    project = newProject;
    rebuild();
}

void MixerView::setEngine(AudioEngine* newEngine)
{
    engine = newEngine;
    startTimerHz(30);
}

void MixerView::timerCallback()
{
    if (engine == nullptr || project == nullptr)
        return;
    float peaks[64];
    const int count = juce::jmin(64, (int) project->tracks.size());
    for (int i = 0; i < count; ++i)
        peaks[i] = engine->getTrackPeak(i);
    setLevels(peaks, count, engine->getMasterPeak());
}

void MixerView::setLevels(const float* peaks, int count, float master)
{
    const int n = juce::jmin(count, 64);
    for (int i = 0; i < 64; ++i)
        levels[i] = i < n && peaks != nullptr ? peaks[i] : 0.0f;
    masterLevel = master;
    repaint();
}

void MixerView::rebuild()
{
    faders.clear();
    pans.clear();
    if (project == nullptr)
        return;

    for (size_t i = 0; i < project->tracks.size(); ++i)
    {
        auto* pan = pans.add(new juce::Slider());
        pan->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        pan->setRange(-1.0, 1.0, 0.01);
        pan->setValue(project->tracks[i].pan, juce::dontSendNotification);
        pan->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        pan->onValueChange = [this, i, pan]
        {
            if (project != nullptr && i < project->tracks.size())
            {
                project->tracks[i].pan = (float) pan->getValue();
                if (onChanged)
                    onChanged();
            }
        };
        addAndMakeVisible(pan);

        auto* slider = faders.add(new juce::Slider());
        slider->setSliderStyle(juce::Slider::LinearVertical);
        slider->setRange(0.0, 1.0, 0.001);
        slider->setValue(project->tracks[i].volume, juce::dontSendNotification);
        slider->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        slider->onValueChange = [this, i, slider]
        {
            if (project != nullptr && i < project->tracks.size())
            {
                project->tracks[i].volume = (float) slider->getValue();
                if (onChanged)
                    onChanged();
            }
        };
        addAndMakeVisible(slider);
    }
    resized();
}

void MixerView::paintMeter(juce::Graphics& g, juce::Rectangle<int> area, float level) const
{
    g.setColour(juce::Colour(0xff070707));
    g.fillRect(area);
    const float clamped = juce::jlimit(0.0f, 1.0f, level);
    const int filled = (int) (area.getHeight() * clamped);
    const int x = area.getX();
    const int width = area.getWidth();
    const int bottom = area.getBottom();
    for (int y = bottom - filled; y < bottom; ++y)
    {
        const float position = (float) (bottom - y) / (float) juce::jmax(1, area.getHeight());
        juce::Colour colour = position > 0.92f ? juce::Colour(0xffd24a4a)
                            : position > 0.75f ? juce::Colour(0xffd2b14a)
                                               : juce::Colour(0xff7dcea0);
        g.setColour(colour);
        g.fillRect(x, y, width, 1);
    }
}

void MixerView::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff161616));
    if (project == nullptr)
        return;

    const int columns = (int) project->tracks.size() + 1;
    g.setFont(juce::FontOptions(11.0f));
    for (int i = 0; i < columns; ++i)
    {
        const bool master = i == (int) project->tracks.size();
        auto column = juce::Rectangle<int>(i * columnWidth, 0, columnWidth, getHeight()).reduced(3, 4);
        g.setColour(juce::Colour(0xff101010));
        g.fillRect(column);
        g.setColour(juce::Colour(0xff2a2a2a));
        g.drawRect(column);

        auto tone = master ? juce::Colour(0xffd0d0d0) : project->tracks[(size_t) i].colour.withSaturation(0.42f).withBrightness(0.62f);
        g.setColour(tone);
        g.fillRect(column.getX(), column.getY(), column.getWidth(), 4);

        g.setColour(juce::Colour(0xffe4e4e4));
        g.drawText(master ? "Master" : project->tracks[(size_t) i].name,
                   column.getX(), column.getY() + 46, column.getWidth(), 16, juce::Justification::centred, true);

        if (!master)
        {
            auto buttons = juce::Rectangle<int>(column.getX() + 8, column.getBottom() - 22, column.getWidth() - 16, 16);
            auto mute = buttons.removeFromLeft(buttons.getWidth() / 2).reduced(2, 0);
            auto solo = buttons.reduced(2, 0);
            g.setColour(project->tracks[(size_t) i].mute ? juce::Colour(0xff8a3038) : juce::Colour(0xff2a2a2a));
            g.fillRect(mute);
            g.setColour(project->tracks[(size_t) i].solo ? juce::Colour(0xff8a7030) : juce::Colour(0xff2a2a2a));
            g.fillRect(solo);
            g.setColour(juce::Colour(0xfff0f0f0));
            g.drawText("M", mute, juce::Justification::centred);
            g.drawText("S", solo, juce::Justification::centred);
        }

        paintMeter(g, juce::Rectangle<int>(column.getRight() - 16, column.getY() + 66, 8, column.getHeight() - 96),
                   master ? masterLevel : levels[i]);
    }
}

void MixerView::resized()
{
    for (int i = 0; i < faders.size(); ++i)
    {
        pans[i]->setBounds(i * columnWidth + 34, 8, 36, 36);
        faders[i]->setBounds(i * columnWidth + 30, 66, 40, getHeight() - 96);
    }
}

void MixerView::mouseDown(const juce::MouseEvent& event)
{
    if (project == nullptr || event.y < getHeight() - 26)
        return;
    const int index = event.x / columnWidth;
    if (index < 0 || index >= (int) project->tracks.size())
        return;

    auto& track = project->tracks[(size_t) index];
    if ((event.x - index * columnWidth) < columnWidth / 2)
        track.mute = !track.mute;
    else
        track.solo = !track.solo;
    repaint();
    if (onChanged)
        onChanged();
}
