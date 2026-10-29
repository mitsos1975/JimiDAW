#include "StudioChrome.h"
#include <cmath>

void MediaRack::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff2a2c30));
    g.setColour(juce::Colour(0xff1a1c1e));
    g.fillRect(0, 0, getWidth(), 28);
    g.setColour(juce::Colour(0xffd0d0d0));
    g.setFont(juce::FontOptions(12.0f));
    g.drawText("VSTI", 8, 0, 48, 28, juce::Justification::centredLeft);
    g.setColour(juce::Colour(0xff8e949c));
    g.drawText("Media    CR    Meter", 56, 0, getWidth() - 64, 28, juce::Justification::centredLeft);

    const char* names[] = { "VST Instruments", "VST Effects", "Loops & Samples", "Presets",
                            "User Presets", "File Browser", "Favorites" };
    auto area = getLocalBounds().withTrimmedTop(40).reduced(12, 4);
    for (int i = 0; i < 7; ++i)
    {
        auto cell = area.removeFromTop(78).reduced(4, 4);
        g.setColour(juce::Colour(0xff3a3d42));
        g.fillRoundedRectangle(cell.toFloat(), 4.0f);
        g.setColour(juce::Colour(0xff4a4e54));
        g.drawRoundedRectangle(cell.toFloat(), 4.0f, 1.0f);
        g.setColour(juce::Colour(0xffd7d7d7));
        g.setFont(juce::FontOptions(13.0f));
        g.drawText(names[i], cell.removeFromBottom(22), juce::Justification::centred);
        g.setColour(juce::Colour(0xff9aa0a6));
        g.drawText(i == 6 ? "*" : "FX", cell, juce::Justification::centred);
    }
}

void MediaRack::mouseDown(const juce::MouseEvent& event)
{
    if (event.y < 36 || !onTile)
        return;
    const int index = (event.y - 40) / 78;
    if (index >= 0 && index < 7)
        onTile(index);
}

ChannelEq::ChannelEq()
{
    bind(low, &Track::lowDb, -12.0, 12.0);
    bind(mid, &Track::midDb, -12.0, 12.0);
    bind(high, &Track::highDb, -12.0, 12.0);
    bind(send, &Track::send, 0.0, 1.0);
    bind(volume, &Track::volume, 0.0, 1.0);
    for (auto* slider : { &low, &mid, &high, &send })
        slider->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    volume.setSliderStyle(juce::Slider::LinearVertical);
    pluginButton.onClick = [this] { if (onLoadPlugin) onLoadPlugin(); };
    addAndMakeVisible(pluginButton);
}

void ChannelEq::bind(juce::Slider& slider, float Track::* field, double start, double end)
{
    slider.setRange(start, end, 0.01);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
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
        repaint();
    };
    addAndMakeVisible(slider);
}

void ChannelEq::setProject(Project* newProject)
{
    project = newProject;
    refresh();
}

void ChannelEq::refresh()
{
    updating = true;
    if (project != nullptr && project->selectedTrack >= 0 && project->selectedTrack < (int) project->tracks.size())
    {
        const auto& track = project->tracks[(size_t) project->selectedTrack];
        low.setValue(track.lowDb, juce::dontSendNotification);
        mid.setValue(track.midDb, juce::dontSendNotification);
        high.setValue(track.highDb, juce::dontSendNotification);
        send.setValue(track.send, juce::dontSendNotification);
        volume.setValue(track.volume, juce::dontSendNotification);
        pluginButton.setButtonText(track.pluginPath.isEmpty() ? "No Insert" : juce::File(track.pluginPath).getFileNameWithoutExtension());
    }
    updating = false;
    repaint();
}

void ChannelEq::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff2b2e32));
    auto plot = getLocalBounds().reduced(168, 12).withTrimmedBottom(118).withTrimmedRight(86);
    g.setColour(juce::Colour(0xff101418));
    g.fillRect(plot);
    g.setColour(juce::Colour(0xff2c3944));
    for (int db = -24; db <= 24; db += 6)
    {
        const float y = plot.getBottom() - (db + 24) / 48.0f * plot.getHeight();
        g.drawHorizontalLine((int) y, (float) plot.getX(), (float) plot.getRight());
        g.setColour(juce::Colour(0xff8d98a3));
        g.setFont(juce::FontOptions(10.0f));
        g.drawText(juce::String(db), plot.getX() - 28, (int) y - 6, 26, 12, juce::Justification::centredRight);
        g.setColour(juce::Colour(0xff2c3944));
    }

    const char* freqs[] = { "20", "50", "100", "200", "500", "1k", "2k", "5k", "10k", "20k" };
    for (int i = 0; i < 10; ++i)
    {
        const float x = plot.getX() + plot.getWidth() * i / 9.0f;
        g.drawVerticalLine((int) x, (float) plot.getY(), (float) plot.getBottom());
        g.setColour(juce::Colour(0xff8d98a3));
        g.drawText(freqs[i], (int) x - 16, plot.getBottom() + 2, 36, 12, juce::Justification::centred);
        g.setColour(juce::Colour(0xff2c3944));
    }

    float lowDb = (float) low.getValue();
    float midDb = (float) mid.getValue();
    float highDb = (float) high.getValue();
    juce::Path curve, fill;
    for (int x = 0; x < plot.getWidth(); ++x)
    {
        const float freq = 20.0f * std::pow(1000.0f, (float) x / (float) juce::jmax(1, plot.getWidth() - 1));
        float db = lowDb * juce::jlimit(0.0f, 1.0f, (220.0f - freq) / 200.0f);
        db += midDb * std::exp(-std::pow(std::log(freq / 700.0f), 2.0f) * 1.6f);
        db += highDb * juce::jlimit(0.0f, 1.0f, (freq - 1800.0f) / 5000.0f);
        db = juce::jlimit(-24.0f, 24.0f, db);
        const float y = plot.getBottom() - (db + 24.0f) / 48.0f * plot.getHeight();
        const float px = (float) plot.getX() + x;
        if (x == 0)
        {
            curve.startNewSubPath(px, y);
            fill.startNewSubPath(px, (float) plot.getCentreY());
            fill.lineTo(px, y);
        }
        else
        {
            curve.lineTo(px, y);
            fill.lineTo(px, y);
        }
    }
    fill.lineTo((float) plot.getRight(), (float) plot.getCentreY());
    fill.closeSubPath();
    g.setColour(juce::Colour(0xff1c6d86).withAlpha(0.55f));
    g.fillPath(fill);
    g.setColour(juce::Colour(0xff5ec8e6));
    g.strokePath(curve, juce::PathStrokeType(2.0f));

    g.setColour(juce::Colour(0xffd0d4d8));
    g.setFont(juce::FontOptions(13.0f));
    g.drawText("Inserts", 12, 8, 140, 18, juce::Justification::centredLeft);
    g.drawText("Equalizer", plot.getX(), 8, 120, 18, juce::Justification::centredLeft);
    g.drawText("Sends", getWidth() - 150, 8, 70, 18, juce::Justification::centredLeft);
    g.drawText("1 LO", low.getX(), low.getBottom() + 2, low.getWidth(), 14, juce::Justification::centred);
    g.drawText("2 MF", mid.getX(), mid.getBottom() + 2, mid.getWidth(), 14, juce::Justification::centred);
    g.drawText("3 HI", high.getX(), high.getBottom() + 2, high.getWidth(), 14, juce::Justification::centred);
    g.drawText("Send", send.getX(), send.getBottom() + 2, send.getWidth(), 14, juce::Justification::centred);

    auto name = juce::String("No Track");
    if (project != nullptr && project->selectedTrack >= 0 && project->selectedTrack < (int) project->tracks.size())
        name = project->tracks[(size_t) project->selectedTrack].name;
    g.drawText(name, getWidth() - 86, getHeight() - 22, 78, 16, juce::Justification::centred);
}

void ChannelEq::resized()
{
    pluginButton.setBounds(12, 36, 140, 26);
    auto bottom = getLocalBounds().removeFromBottom(100).withTrimmedRight(90).withTrimmedLeft(170);
    auto knobs = bottom.removeFromLeft(juce::jmin(360, bottom.getWidth()));
    const int knob = 64;
    low.setBounds(knobs.getX(), knobs.getY(), knob, knob);
    mid.setBounds(knobs.getX() + 90, knobs.getY(), knob, knob);
    high.setBounds(knobs.getX() + 180, knobs.getY(), knob, knob);
    send.setBounds(getWidth() - 150, 48, 70, 70);
    volume.setBounds(getWidth() - 78, 36, 56, getHeight() - 64);
}
