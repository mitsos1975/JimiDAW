#include "AudioEngine.h"

AudioEngine::AudioEngine()
{
    deviceManager.initialise(2, 2, nullptr, true);
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

void AudioEngine::startRecording()
{
    const double rate = sampleRate > 1000.0 ? sampleRate : 44100.0;
    recordBuffer.setSize(1, (int) (rate * 180.0), false, true, true);
    recordBuffer.clear();
    recordWrite.store(0);
    recordStartBeat = beat.load();
    recording.store(true);
    setPlaying(true);
}

Clip AudioEngine::stopRecording()
{
    recording.store(false);
    const int count = juce::jlimit(0, recordBuffer.getNumSamples(), recordWrite.load());

    Clip clip;
    clip.startBeat = recordStartBeat;
    clip.name = "Record";
    clip.sampleRate = sampleRate > 1000.0 ? sampleRate : 44100.0;
    if (count > 0)
    {
        clip.audio = std::make_shared<juce::AudioBuffer<float>>(1, count);
        clip.audio->copyFrom(0, 0, recordBuffer, 0, 0, count);
        clip.lengthBeats = juce::jmax(0.25, (count / clip.sampleRate) * (bpm.load() / 60.0));
    }
    recordWrite.store(0);
    return clip;
}

void AudioEngine::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    sampleRate = device != nullptr ? device->getCurrentSampleRate() : 44100.0;
}

void AudioEngine::audioDeviceStopped()
{
    clickSamplesLeft = 0;
    recording.store(false);
}

float AudioEngine::renderSample(float input)
{
    const bool running = playing.load() || recording.load();
    if (project == nullptr || !running)
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
        sample += (float) (0.12 * std::sin(clickPhase));
        clickPhase += juce::MathConstants<double>::twoPi * freq / sampleRate;
        --clickSamplesLeft;
    }

    if (recording.load())
    {
        const int index = recordWrite.load();
        if (index < recordBuffer.getNumSamples())
        {
            recordBuffer.setSample(0, index, input);
            recordWrite.store(index + 1);
        }
        else
        {
            recording.store(false);
        }
    }

    const bool soloing = project->anySolo();
    for (size_t i = 0; i < project->tracks.size(); ++i)
    {
        const auto& track = project->tracks[i];
        if (track.mute || (soloing && !track.solo))
            continue;

        for (const auto& clip : track.clips)
        {
            if (now < clip.startBeat || now >= clip.startBeat + clip.lengthBeats)
                continue;

            if (clip.audio != nullptr && clip.audio->getNumSamples() > 1)
            {
                const double seconds = (now - clip.startBeat) * 60.0 / bpm.load();
                const double position = seconds * clip.sampleRate;
                const int index = (int) position;
                if (index >= 0 && index + 1 < clip.audio->getNumSamples())
                {
                    const float fraction = (float) (position - index);
                    const float a = clip.audio->getSample(0, index);
                    const float b = clip.audio->getSample(0, index + 1);
                    sample += (a + (b - a) * fraction) * track.volume;
                }
            }
            else
            {
                const double freq = 110.0 * std::pow(2.0, (double) i / 12.0);
                const double phase = std::fmod(now, 1.0) * juce::MathConstants<double>::twoPi * (freq / (bpm.load() / 60.0));
                sample += (float) (std::sin(phase) * 0.06 * track.volume);
            }
        }
    }

    beat.store(now + beatsPerSample);
    return juce::jlimit(-1.0f, 1.0f, sample);
}

void AudioEngine::audioDeviceIOCallbackWithContext(const float* const* inputChannelData, int numInputChannels,
                                                   float* const* outputChannelData, int numOutputChannels,
                                                   int numSamples, const juce::AudioIODeviceCallbackContext&)
{
    for (int i = 0; i < numSamples; ++i)
    {
        const float input = (numInputChannels > 0 && inputChannelData[0] != nullptr) ? inputChannelData[0][i] : 0.0f;
        const float value = renderSample(input);
        for (int channel = 0; channel < numOutputChannels; ++channel)
            outputChannelData[channel][i] = value;
    }
}
