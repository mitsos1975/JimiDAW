#include "StudioChrome.h"
#include <cmath>

void MediaRack::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff2a2d31));
    auto tabs = getLocalBounds().removeFromTop(28);
    g.setColour(juce::Colour(0xff1e2125));
    g.fillRect(tabs);
    g.setColour(juce::Colour(0xffd8d8d8));
    g.setFont(juce::FontOptions(12.0f));
    g.drawText("VSTI", 10, 0, 42, 28, juce::Justification::centredLeft);
    g.setColour(juce::Colour(0xff8e949c));
    g.drawText("Media     CR     Meter", 52, 0, getWidth() - 60, 28, juce::Justification::centredLeft);

    const char* names[] = { "VST Instruments", "VST Effects", "Loops & Samples", "Presets",
                            "User Presets", "File Browser", "Favorites" };
    auto grid = getLocalBounds().withTrimmedTop(36).reduced(8, 4);
    const int columns = 2;
    const int cellW = grid.getWidth() / columns;
    const int cellH = 92;
    for (int i = 0; i < 7; ++i)
    {
        const int col = i % columns;
        const int row = i / columns;
        auto cell = juce::Rectangle<int>(grid.getX() + col * cellW, grid.getY() + row * cellH, cellW, cellH).reduced(6, 6);
        g.setColour(juce::Colour(0xff3a3e44));
        g.fillRoundedRectangle(cell.toFloat(), 3.0f);
        g.setColour(juce::Colour(0xffcfcfcf));
        auto icon = cell.removeFromTop(cell.getHeight() - 22).reduced(18, 8);
        g.drawRoundedRectangle(icon.toFloat(), 3.0f, 1.4f);
        g.setFont(juce::FontOptions(12.0f));
        g.drawText(names[i], cell, juce::Justification::centred);
    }
}

void MediaRack::mouseDown(const juce::MouseEvent& event)
{
    if (event.y < 36 || !onTile)
        return;
    auto grid = getLocalBounds().withTrimmedTop(36).reduced(8, 4);
    const int cellW = juce::jmax(1, grid.getWidth() / 2);
    const int col = (event.x - grid.getX()) / cellW;
    const int row = (event.y - grid.getY()) / 92;
    if (col < 0 || col > 1 || row < 0)
        return;
    const int index = row * 2 + col;
    if (index >= 0 && index < 7)
        onTile(index);
}

void ToolStrip::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff2a2d31));
    g.setColour(juce::Colour(0xff17191c));
    g.fillRect(0, getHeight() - 1, getWidth(), 1);

    auto button = [&](int x, int w, const juce::String& text, bool on)
    {
        auto box = juce::Rectangle<int>(x, 4, w, getHeight() - 8);
        g.setColour(on ? juce::Colour(0xff3d5164) : juce::Colour(0xff34383d));
        g.fillRoundedRectangle(box.toFloat(), 2.0f);
        g.setColour(on ? juce::Colours::white : juce::Colour(0xffd0d0d0));
        g.setFont(juce::FontOptions(11.0f));
        g.drawText(text, box, juce::Justification::centred);
    };

    button(8, 28, "M", false);
    button(38, 28, "S", false);
    button(68, 28, "R", false);
    button(108, 54, "Select", tool == 0);
    button(164, 46, "Draw", tool == 1);
    button(212, 50, "Erase", tool == 2);
    button(274, 46, "Split", false);
    button(322, 42, "Quant", false);
    button(376, 28, "+", false);
    button(406, 28, "-", false);
    button(446, 52, "Grid", true);
    button(500, 42, "Bar", false);
    button(544, 42, "1/16", false);
    button(getWidth() - 118, 52, "Mix", false);
    button(getWidth() - 62, 52, "EQ", false);
}

void ToolStrip::mouseDown(const juce::MouseEvent& event)
{
    if (!onCommand)
        return;
    const int x = event.x;
    int id = -1;
    if (x >= 108 && x < 162) id = 0;
    else if (x >= 164 && x < 210) id = 1;
    else if (x >= 212 && x < 262) id = 2;
    else if (x >= 274 && x < 320) id = 3;
    else if (x >= 322 && x < 364) id = 4;
    else if (x >= 376 && x < 404) id = 5;
    else if (x >= 406 && x < 434) id = 6;
    else if (x >= getWidth() - 118 && x < getWidth() - 64) id = 7;
    else if (x >= getWidth() - 62) id = 8;
    if (id >= 0)
    {
        if (id < 3)
            tool = id;
        onCommand(id);
        repaint();
    }
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
