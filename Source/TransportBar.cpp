#include "TransportBar.h"

TransportBar::TransportBar()
{
    for (auto* button : { &playButton, &stopButton, &recordButton, &audioButton })
    {
        button->addListener(this);
        addAndMakeVisible(button);
    }

    tempoSlider.setRange(40.0, 280.0, 0.1);
    tempoSlider.setValue(120.0, juce::dontSendNotification);
    tempoSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 58, 20);
    tempoSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    tempoSlider.addListener(this);
    addAndMakeVisible(tempoSlider);

    tempoLabel.setText("BPM", juce::dontSendNotification);
    tempoLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(tempoLabel);

    positionLabel.setJustificationType(juce::Justification::centred);
    positionLabel.setFont(juce::FontOptions(20.0f));
    positionLabel.setColour(juce::Label::textColourId, juce::Colour(0xffc8f7cf));
    positionLabel.setColour(juce::Label::backgroundColourId, juce::Colour(0xff10140f));
    addAndMakeVisible(positionLabel);
}

void TransportBar::setPlaying(bool playing)
{
    playButton.setButtonText(playing ? "Pause" : "Play");
    playButton.setColour(juce::TextButton::buttonColourId,
                          playing ? juce::Colour(0xff24362b) : juce::Colour(0xff2a2a2a));
}

void TransportBar::setRecording(bool recording)
{
    recordButton.setColour(juce::TextButton::buttonColourId,
                            recording ? juce::Colour(0xff4a2222) : juce::Colour(0xff2a2a2a));
}

void TransportBar::setBeat(double beat)
{
    const int bar = (int) std::floor(beat / 4.0) + 1;
    const int step = ((int) std::floor(beat)) % 4 + 1;
    const int tick = ((int) std::floor(beat * 4.0)) % 4 + 1;
    positionLabel.setText(juce::String(bar) + "." + juce::String(step) + "." + juce::String(tick),
                          juce::dontSendNotification);
}

void TransportBar::setBpm(double bpm)
{
    tempoSlider.setValue(bpm, juce::dontSendNotification);
}

void TransportBar::paint(juce::Graphics& g)
{
    juce::ColourGradient wash(juce::Colour(0xff2a2a2a), 0.0f, 0.0f,
                              juce::Colour(0xff1a1a1a), 0.0f, (float) getHeight(), false);
    g.setGradientFill(wash);
    g.fillAll();
    g.setColour(juce::Colour(0xff0a0a0a));
    g.fillRect(0, getHeight() - 1, getWidth(), 1);
    g.setColour(juce::Colours::white.withAlpha(0.04f));
    g.fillRect(0, 0, getWidth(), 1);
}

void TransportBar::resized()
{
    auto area = getLocalBounds().reduced(10, 8);
    playButton.setBounds(area.removeFromLeft(42));
    area.removeFromLeft(4);
    stopButton.setBounds(area.removeFromLeft(42));
    area.removeFromLeft(4);
    recordButton.setBounds(area.removeFromLeft(42));
    area.removeFromLeft(8);
    audioButton.setBounds(area.removeFromLeft(64));
    area.removeFromLeft(16);
    positionLabel.setBounds(area.removeFromLeft(118).reduced(0, 1));
    area.removeFromLeft(12);
    tempoLabel.setBounds(area.removeFromLeft(36));
    tempoSlider.setBounds(area.removeFromLeft(210));
}

void TransportBar::buttonClicked(juce::Button* button)
{
    if (button == &playButton && onPlay) onPlay();
    if (button == &stopButton && onStop) onStop();
    if (button == &recordButton && onRecord) onRecord();
    if (button == &audioButton && onAudio) onAudio();
}

void TransportBar::sliderValueChanged(juce::Slider* slider)
{
    if (slider == &tempoSlider && onTempoChanged)
        onTempoChanged(tempoSlider.getValue());
}
