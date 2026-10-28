#include "MixerView.h"

void MixerView::setProject(Project* newProject)
{
    project = newProject;
    rebuild();
}

void MixerView::rebuild()
{
    faders.clear();
    if (project == nullptr)
        return;

    for (size_t i = 0; i < project->tracks.size(); ++i)
    {
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

void MixerView::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1a1a1a));
    g.setColour(juce::Colour(0xff0a0a0a));
    g.fillRect(0, 0, getWidth(), 1);

    if (project == nullptr)
        return;

    g.setFont(juce::FontOptions(11.0f));
    for (size_t i = 0; i < project->tracks.size(); ++i)
    {
        auto column = juce::Rectangle<int>((int) i * columnWidth, 0, columnWidth, getHeight()).reduced(3, 4);
        g.setColour(juce::Colour(0xff101010));
        g.fillRect(column);
        g.setColour(juce::Colour(0xff2c2c2c));
        g.drawRect(column);

        const auto tone = project->tracks[i].colour.withSaturation(0.42f).withBrightness(0.62f);
        g.setColour(tone);
        g.fillRect(column.removeFromTop(5));

        g.setColour(juce::Colour(0xffd8d8d8));
        g.drawText(project->tracks[i].name, column.removeFromTop(18), juce::Justification::centred, true);

        auto buttons = column.removeFromBottom(22).reduced(6, 2);
        auto mute = buttons.removeFromLeft(buttons.getWidth() / 2).reduced(2, 0);
        auto solo = buttons.reduced(2, 0);
        g.setColour(project->tracks[i].mute ? juce::Colour(0xff8a3038) : juce::Colour(0xff2a2a2a));
        g.fillRect(mute);
        g.setColour(project->tracks[i].solo ? juce::Colour(0xff8a7030) : juce::Colour(0xff2a2a2a));
        g.fillRect(solo);
        g.setColour(juce::Colour(0xffe8e8e8));
        g.drawText("M", mute, juce::Justification::centred);
        g.drawText("S", solo, juce::Justification::centred);

        g.setColour(juce::Colour(0xff6a6a6a));
        g.setFont(juce::FontOptions(9.0f));
        const int marks[] = { 6, 0, -6, -12, -24 };
        for (int mark = 0; mark < 5; ++mark)
        {
            const int y = 28 + mark * (getHeight() - 78) / 4;
            g.drawText(juce::String(marks[mark]), column.getX(), y, 16, 10, juce::Justification::centredRight);
        }
    }
}

void MixerView::resized()
{
    for (int i = 0; i < faders.size(); ++i)
        faders[i]->setBounds(i * columnWidth + 28, 28, 36, getHeight() - 58);
}

void MixerView::mouseDown(const juce::MouseEvent& event)
{
    if (project == nullptr)
        return;
    const int index = event.x / columnWidth;
    if (index < 0 || index >= (int) project->tracks.size())
        return;
    if (event.y < getHeight() - 26)
        return;

    auto& track = project->tracks[(size_t) index];
    const int local = event.x - index * columnWidth;
    if (local < columnWidth / 2)
        track.mute = !track.mute;
    else
        track.solo = !track.solo;
    repaint();
    if (onChanged)
        onChanged();
}
