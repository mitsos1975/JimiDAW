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
        slider->setRange(0.0, 1.0, 0.01);
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
    g.fillAll(juce::Colour(0xff1c1f22));
    g.setColour(juce::Colour(0xff111315));
    g.drawLine(0.0f, 0.0f, (float) getWidth(), 0.0f);

    if (project == nullptr)
        return;

    g.setFont(11.0f);
    for (size_t i = 0; i < project->tracks.size(); ++i)
    {
        auto column = juce::Rectangle<int>((int) i * 72, 0, 72, getHeight()).reduced(4, 6);
        g.setColour(project->tracks[i].colour);
        g.fillRect(column.removeFromTop(4));
        g.setColour(juce::Colour(0xffe8eaed));
        g.drawText(project->tracks[i].name, column.removeFromBottom(18), juce::Justification::centred);
    }
}

void MixerView::resized()
{
    for (int i = 0; i < faders.size(); ++i)
        faders[i]->setBounds(i * 72 + 16, 16, 40, getHeight() - 42);
}
