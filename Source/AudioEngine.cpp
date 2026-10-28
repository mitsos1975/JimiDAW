#include "AudioEngine.h"

AudioEngine::AudioEngine()
{
    deviceManager.initialise(0, 2, nullptr, true);
    deviceManager.addAudioCallback(this);
}

AudioEngine::~AudioEngine()
{
    deviceManager.removeAudioCallback(this);
    deviceManager.closeAudioDevice();
}

void AudioEngine::setProject(Project* newProject)
{
    project = newProject;
    if (project != nullptr)
        bpm.store(project->bpm);
}

void AudioEngine::setPlaying(bool shouldPlay)
{
    if (shouldPlay && !playing.load())
        lastWholeBeat = (int) std::floor(beat.load()) - 1;
    playing.store(shouldPlay);
}

void AudioEngine::setBpm(double newBpm)
{
    bpm.store(juce::jlimit(40.0, 280.0, newBpm));
    if (project != nullptr)
        project->bpm = bpm.load();
}

void AudioEngine::setBeat(double newBeat)
{
    beat.store(juce::jmax(0.0, newBeat));
    lastWholeBeat = (int) std::floor(beat.load()) - 1;
}

void AudioEngine::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    sampleRate = device != nullptr ? device->getCurrentSampleRate() : 44100.0;
}

void AudioEngine::audioDeviceStopped()
{
    clickSamplesLeft = 0;
}

float AudioEngine::renderSample()
{
    if (project == nullptr || !playing.load())
        return 0.0f;

    const double beatsPerSample = bpm.load() / 60.0 / sampleRate;
    const double now = beat.load();
    const int whole = (int) std::floor(now);
    if (whole != lastWholeBeat)
    {
        lastWholeBeat = whole;
        clickSamplesLeft = (int) (0.03 * sampleRate);
        clickPhase = 0.0;
    }

    float sample = 0.0f;
    if (clickSamplesLeft > 0)
    {
        const double freq = (whole % 4 == 0) ? 1760.0 : 1174.0;
        sample += (float) (0.15 * std::sin(clickPhase));
        clickPhase += juce::MathConstants<double>::twoPi * freq / sampleRate;
        --clickSamplesLeft;
    }

    const bool soloing = project->anySolo();
    for (size_t i = 0; i < project->tracks.size(); ++i)
    {
        const auto& track = project->tracks[i];
        if (track.mute || (soloing && !track.solo))
            continue;

        bool inside = false;
        for (const auto& clip : track.clips)
            if (now >= clip.startBeat && now < clip.startBeat + clip.lengthBeats)
                inside = true;
        if (!inside)
            continue;

        const double freq = 110.0 * std::pow(2.0, (double) i / 12.0);
        const double phase = std::fmod(now, 1.0) * juce::MathConstants<double>::twoPi * (freq / (bpm.load() / 60.0));
        sample += (float) (std::sin(phase) * 0.08 * track.volume);
    }

    beat.store(now + beatsPerSample);
    return juce::jlimit(-1.0f, 1.0f, sample);
}

void AudioEngine::audioDeviceIOCallbackWithContext(const float* const*, int,
                                                   float* const* outputChannelData, int numOutputChannels,
                                                   int numSamples, const juce::AudioIODeviceCallbackContext&)
{
    for (int sample = 0; sample < numSamples; ++sample)
    {
        const float value = renderSample();
        for (int channel = 0; channel < numOutputChannels; ++channel)
            outputChannelData[channel][sample] = value;
    }
}
