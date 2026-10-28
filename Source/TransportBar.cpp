#include "TransportBar.h"

TransportBar::TransportBar()
{
    playButton.addListener(this);
    stopButton.addListener(this);
    addAndMakeVisible(playButton);
    addAndMakeVisible(stopButton);

    tempoSlider.setRange(40.0, 280.0, 1.0);
    tempoSlider.setValue(120.0, juce::dontSendNotification);
    tempoSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 56, 22);
    tempoSlider.addListener(this);
    addAndMakeVisible(tempoSlider);

    tempoLabel.setText("BPM", juce::dontSendNotification);
    addAndMakeVisible(tempoLabel);
    addAndMakeVisible(positionLabel);
}

void TransportBar::setPlaying(bool playing)
{
    playButton.setButtonText(playing ? "Pause" : "Play");
    playButton.setColour(juce::TextButton::buttonColourId,
                          playing ? juce::Colour(0xff2e7d4f) : juce::Colour(0xff2e3236));
}

void TransportBar::setBeat(double beat)
{
    const int bar = (int) std::floor(beat / 4.0) + 1;
    const int step = ((int) std::floor(beat)) % 4 + 1;
    positionLabel.setText(juce::String(bar) + "." + juce::String(step), juce::dontSendNotification);
}

void TransportBar::setBpm(double bpm)
{
    tempoSlider.setValue(bpm, juce::dontSendNotification);
}

void TransportBar::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff23272b));
    g.setColour(juce::Colour(0xff111315));
    g.drawLine(0.0f, (float) getHeight() - 1.0f, (float) getWidth(), (float) getHeight() - 1.0f);
}

void TransportBar::resized()
{
    auto area = getLocalBounds().reduced(8, 6);
    playButton.setBounds(area.removeFromLeft(72));
    area.removeFromLeft(6);
    stopButton.setBounds(area.removeFromLeft(72));
    area.removeFromLeft(16);
    positionLabel.setBounds(area.removeFromLeft(70));
    tempoLabel.setBounds(area.removeFromLeft(36));
    tempoSlider.setBounds(area.removeFromLeft(180));
}

void TransportBar::buttonClicked(juce::Button* button)
{
    if (button == &playButton && onPlay) onPlay();
    if (button == &stopButton && onStop) onStop();
}

void TransportBar::sliderValueChanged(juce::Slider* slider)
{
    if (slider == &tempoSlider && onTempoChanged)
        onTempoChanged(tempoSlider.getValue());
}
