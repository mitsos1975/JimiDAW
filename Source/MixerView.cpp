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
    sends.clear();
    if (project == nullptr)
        return;

    for (size_t i = 0; i < project->tracks.size(); ++i)
    {
        auto* send = sends.add(new juce::Slider());
        send->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        send->setRange(0.0, 1.0, 0.01);
        send->setValue(project->tracks[i].send, juce::dontSendNotification);
        send->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        send->setTooltip("Send");
        send->onValueChange = [this, i, send]
        {
            if (project != nullptr && i < project->tracks.size())
            {
                project->tracks[i].send = (float) send->getValue();
                if (onChanged)
                    onChanged();
            }
        };
        addAndMakeVisible(send);

        auto* pan = pans.add(new juce::Slider());
        pan->setTooltip("Pan");
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
        slider->setColour(juce::Slider::thumbColourId, project->tracks[i].colour);
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
    g.fillAll(juce::Colour(0xff23262b));
    g.setColour(juce::Colour(0xff1b1e22));
    g.fillRect(0, 0, gutter, getHeight());
    g.setColour(juce::Colour(0xffd0d0d0));
    g.setFont(juce::FontOptions(12.0f));
    g.drawText("Visibility", 10, 6, gutter - 16, 18, juce::Justification::centredLeft);
    if (project == nullptr)
        return;

    for (size_t i = 0; i < project->tracks.size(); ++i)
    {
        const int y = 30 + (int) i * 18;
        if (y > getHeight() - 20)
            break;
        const bool selected = project->selectedTrack == (int) i;
        if (selected)
        {
            g.setColour(juce::Colour(0xff31404c));
            g.fillRect(4, y - 1, gutter - 8, 18);
        }
        g.setColour(project->tracks[i].colour);
        g.fillEllipse(10.0f, (float) y + 4.0f, 8.0f, 8.0f);
        g.setColour(juce::Colour(0xffd5d5d5));
        g.setFont(juce::FontOptions(11.0f));
        g.drawText(project->tracks[i].name, 24, y, gutter - 32, 16, juce::Justification::centredLeft, true);
    }

    const char* rows[] = { "ROUTING", "INSERTS", "EQ", "STRIP", "SENDS" };
    for (int row = 0; row < 5; ++row)
    {
        const int y = 4 + row * 20;
        g.setColour(juce::Colour(0xff8e949c));
        g.setFont(juce::FontOptions(10.0f));
        g.drawText(rows[row], gutter + 4, y, 52, 16, juce::Justification::centredLeft);
        for (size_t i = 0; i < project->tracks.size(); ++i)
        {
            const float x = (float) (gutter + (int) i * columnWidth + 28);
            const bool on = (row == 2) || (row == 4 && project->tracks[i].send > 0.01f) || (row == 1 && project->tracks[i].pluginPath.isNotEmpty());
            g.setColour(on ? juce::Colour(0xff3d8bfd) : juce::Colour(0xff3a3e44));
            g.fillEllipse(x, (float) y + 3.0f, 10.0f, 10.0f);
        }
    }

    const int columns = (int) project->tracks.size() + 1;
    for (int i = 0; i < columns; ++i)
    {
        const bool master = i == (int) project->tracks.size();
        const int x = gutter + i * columnWidth;
        auto well = juce::Rectangle<int>(x + 4, zone + 56, columnWidth - 8, getHeight() - zone - 92);
        g.setColour(juce::Colour(0xff101114));
        g.fillRect(well);
        g.setColour(juce::Colour(0xff3a3e44));
        g.drawRect(well);

        if (!master)
        {
            auto buttons = juce::Rectangle<int>(x + 8, zone + 34, columnWidth - 16, 16);
            auto mute = buttons.removeFromLeft(buttons.getWidth() / 2).reduced(1, 0);
            auto solo = buttons.reduced(1, 0);
            g.setColour(project->tracks[(size_t) i].mute ? juce::Colour(0xffc4a21a) : juce::Colour(0xff2c3036));
            g.fillRect(mute);
            g.setColour(project->tracks[(size_t) i].solo ? juce::Colour(0xff2f8fd0) : juce::Colour(0xff2c3036));
            g.fillRect(solo);
            g.setColour(juce::Colour(0xfff0f0f0));
            g.setFont(juce::FontOptions(10.0f));
            g.drawText("M", mute, juce::Justification::centred);
            g.drawText("S", solo, juce::Justification::centred);
        }

        paintMeter(g, juce::Rectangle<int>(x + columnWidth - 16, zone + 60, 6, getHeight() - zone - 100),
                   master ? masterLevel : levels[i]);

        auto nameBar = juce::Rectangle<int>(x + 2, getHeight() - 28, columnWidth - 4, 24);
        auto tone = master ? juce::Colour(0xff3a3e44) : project->tracks[(size_t) i].colour.withMultipliedBrightness(0.55f);
        g.setColour(tone);
        g.fillRect(nameBar);
        g.setColour(juce::Colours::white);
        g.setFont(juce::FontOptions(10.0f));
        g.drawText(master ? "Stereo Out" : project->tracks[(size_t) i].name, nameBar, juce::Justification::centred, true);
    }
}

void MixerView::resized()
{
    for (int i = 0; i < faders.size(); ++i)
    {
        const int x = gutter + i * columnWidth;
        pans[i]->setBounds(x + 20, zone + 2, 28, 28);
        sends[i]->setBounds(x + 42, 80, 22, 20);
        faders[i]->setBounds(x + 10, zone + 60, 28, getHeight() - zone - 104);
    }
}

void MixerView::mouseDown(const juce::MouseEvent& event)
{
    if (project == nullptr)
        return;
    if (event.x < gutter)
    {
        const int index = (event.y - 30) / 18;
        if (index >= 0 && index < (int) project->tracks.size())
        {
            project->selectedTrack = index;
            repaint();
            if (onChanged)
                onChanged();
        }
        return;
    }
    if (event.y < zone + 32 || event.y > zone + 52)
        return;
    const int index = (event.x - gutter) / columnWidth;
    if (index < 0 || index >= (int) project->tracks.size())
        return;
    auto& track = project->tracks[(size_t) index];
    if (((event.x - gutter) % columnWidth) < columnWidth / 2)
        track.mute = !track.mute;
    else
        track.solo = !track.solo;
    repaint();
    if (onChanged)
        onChanged();
}
