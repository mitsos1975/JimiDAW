#include "AudioEngine.h"

namespace
{
juce::dsp::IIR::Coefficients<float>::Ptr shelfOrPass(bool engaged, const juce::dsp::IIR::Coefficients<float>::Ptr& engagedCoeffs)
{
    if (!engaged)
        return new juce::dsp::IIR::Coefficients<float>(1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
    return engagedCoeffs;
}
}

AudioEngine::AudioEngine()
{
    formatManager.addFormat(new juce::VST3PluginFormat());
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
    juce::ScopedLock sl(lock);
    project = newProject;
    if (project != nullptr)
    {
        bpm.store(project->bpm);
        ensureDsp((int) project->tracks.size());
    }
}

void AudioEngine::setPlaying(bool shouldPlay)
{
    if (shouldPlay && !playing.load())
        lastWholeBeat = (int) std::floor(beat.load()) - 1;
    playing.store(shouldPlay);
}

void AudioEngine::setBpm(double newBpm)
{
    const double value = juce::jlimit(40.0, 280.0, newBpm);
    bpm.store(value);
    if (project != nullptr)
        project->bpm = value;
}

void AudioEngine::setBeat(double newBeat)
{
    beat.store(juce::jmax(0.0, newBeat));
    lastWholeBeat = (int) std::floor(beat.load()) - 1;
}

void AudioEngine::ensureDsp(int trackCount)
{
    trackCount = juce::jmax(0, trackCount);
    if ((int) plugins.size() != trackCount)
        plugins.resize((size_t) trackCount);
    if ((int) lowFilters.size() == trackCount)
        return;

    lowFilters.resize((size_t) trackCount);
    midFilters.resize((size_t) trackCount);
    highFilters.resize((size_t) trackCount);
    compEnv.assign((size_t) trackCount, 0.0f);
    phases.assign((size_t) trackCount * 128, 0.0f);
    auto pass = new juce::dsp::IIR::Coefficients<float>(1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
    for (int i = 0; i < trackCount; ++i)
    {
        lowFilters[(size_t) i].coefficients = pass;
        midFilters[(size_t) i].coefficients = pass;
        highFilters[(size_t) i].coefficients = pass;
    }
}

void AudioEngine::startRecording()
{
    juce::ScopedLock sl(lock);
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
    juce::ScopedLock sl(lock);
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

juce::String AudioEngine::loadPlugin(int trackIndex, const juce::File& file)
{
    juce::OwnedArray<juce::PluginDescription> types;
    for (int i = 0; i < formatManager.getNumFormats(); ++i)
        formatManager.getFormat(i)->findAllTypesForFile(types, file.getFullPathName());
    if (types.isEmpty())
        return "That file does not contain a VST3 plugin.";

    juce::String error;
    auto instance = formatManager.createPluginInstance(*types[0], sampleRate, blockSize, error);
    if (instance == nullptr)
        return error.isNotEmpty() ? error : "Could not open the plugin.";

    instance->enableAllBuses();
    instance->prepareToPlay(sampleRate, blockSize);

    juce::ScopedLock sl(lock);
    const int needed = project != nullptr ? juce::jmax((int) project->tracks.size(), trackIndex + 1) : trackIndex + 1;
    ensureDsp(needed);
    if (trackIndex >= 0 && trackIndex < (int) plugins.size())
        plugins[(size_t) trackIndex] = std::move(instance);
    if (project != nullptr && trackIndex >= 0 && trackIndex < (int) project->tracks.size())
        project->tracks[(size_t) trackIndex].pluginPath = file.getFullPathName();
    return {};
}

void AudioEngine::clearPlugin(int trackIndex)
{
    juce::ScopedLock sl(lock);
    if (trackIndex >= 0 && trackIndex < (int) plugins.size())
        plugins[(size_t) trackIndex].reset();
    if (project != nullptr && trackIndex >= 0 && trackIndex < (int) project->tracks.size())
        project->tracks[(size_t) trackIndex].pluginPath.clear();
}

juce::AudioPluginInstance* AudioEngine::getPlugin(int trackIndex)
{
    juce::ScopedLock sl(lock);
    if (trackIndex < 0 || trackIndex >= (int) plugins.size())
        return nullptr;
    return plugins[(size_t) trackIndex].get();
}

float AudioEngine::getTrackPeak(int trackIndex) const
{
    if (trackIndex < 0 || trackIndex >= 64)
        return 0.0f;
    return trackPeaks[trackIndex].load();
}

void AudioEngine::rememberPeak(int trackIndex, float peak)
{
    if (trackIndex < 0 || trackIndex >= 64)
        return;
    const float previous = trackPeaks[trackIndex].load();
    trackPeaks[trackIndex].store(juce::jmax(peak, previous * 0.86f));
}

void AudioEngine::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    juce::ScopedLock sl(lock);
    sampleRate = device != nullptr ? device->getCurrentSampleRate() : 44100.0;
    blockSize = device != nullptr ? juce::jmax(32, device->getCurrentBufferSizeSamples()) : 512;
    lowFilters.clear();
    if (project != nullptr)
        ensureDsp((int) project->tracks.size());
    for (auto& plugin : plugins)
    {
        if (plugin != nullptr)
        {
            plugin->releaseResources();
            plugin->prepareToPlay(sampleRate, blockSize);
        }
    }
}

void AudioEngine::audioDeviceStopped()
{
    clickSamplesLeft = 0;
    recording.store(false);
}

void AudioEngine::renderUnlocked(const float* input, float* const* outputs, int numOutputs,
                                 int numSamples, bool live, bool withClick, bool allowLoop)
{
    if (outputs == nullptr || numOutputs <= 0 || numSamples <= 0)
        return;
    for (int channel = 0; channel < numOutputs; ++channel)
        if (outputs[channel] != nullptr)
            juce::FloatVectorOperations::clear(outputs[channel], numSamples);

    if (project == nullptr)
        return;

    const int trackCount = (int) project->tracks.size();
    ensureDsp(trackCount);
    if (trackAudio.getNumChannels() < trackCount || trackAudio.getNumSamples() < numSamples)
        trackAudio.setSize(juce::jmax(1, trackCount), numSamples, false, false, true);
    trackAudio.clear();

    const double rate = sampleRate > 1000.0 ? sampleRate : 44100.0;
    const double tempo = bpm.load();
    const double beatsPerSample = tempo / 60.0 / rate;
    double cursor = beat.load();
    const bool running = live ? (playing.load() || recording.load()) : true;
    const bool soloing = project->anySolo();

    if (running)
    {
        for (int trackIndex = 0; trackIndex < trackCount; ++trackIndex)
        {
            const auto& track = project->tracks[(size_t) trackIndex];
            lowFilters[(size_t) trackIndex].coefficients = shelfOrPass(std::abs(track.lowDb) >= 0.05f,
                juce::dsp::IIR::Coefficients<float>::makeLowShelf(rate, 120.0f, 0.707f, juce::Decibels::decibelsToGain(track.lowDb)));
            midFilters[(size_t) trackIndex].coefficients = shelfOrPass(std::abs(track.midDb) >= 0.05f,
                juce::dsp::IIR::Coefficients<float>::makePeakFilter(rate, 1000.0f, 0.707f, juce::Decibels::decibelsToGain(track.midDb)));
            highFilters[(size_t) trackIndex].coefficients = shelfOrPass(std::abs(track.highDb) >= 0.05f,
                juce::dsp::IIR::Coefficients<float>::makeHighShelf(rate, 8000.0f, 0.707f, juce::Decibels::decibelsToGain(track.highDb)));
        }

        for (int sample = 0; sample < numSamples; ++sample)
        {
            const double now = cursor;
            const int whole = (int) std::floor(now);
            float click = 0.0f;
            if (withClick && project->metronome)
            {
                if (whole != lastWholeBeat)
                {
                    lastWholeBeat = whole;
                    clickSamplesLeft = (int) (0.03 * rate);
                    clickPhase = 0.0;
                }
                if (clickSamplesLeft > 0)
                {
                    const double freq = (whole % 4 == 0) ? 1760.0 : 1174.0;
                    click = (float) (0.12 * std::sin(clickPhase));
                    clickPhase += juce::MathConstants<double>::twoPi * freq / rate;
                    --clickSamplesLeft;
                }
            }
            for (int channel = 0; channel < numOutputs; ++channel)
                outputs[channel][sample] += click;

            if (recording.load() && recordWrite.load() < recordBuffer.getNumSamples())
            {
                const int index = recordWrite.load();
                recordBuffer.setSample(0, index, input != nullptr ? input[sample] : 0.0f);
                recordWrite.store(index + 1);
            }

            for (int trackIndex = 0; trackIndex < trackCount; ++trackIndex)
            {
                const auto& track = project->tracks[(size_t) trackIndex];
                if (track.mute || (soloing && !track.solo))
                    continue;

                float mixed = 0.0f;
                for (const auto& clip : track.clips)
                {
                    if (now < clip.startBeat || now >= clip.startBeat + clip.lengthBeats)
                        continue;
                    if (clip.audio != nullptr && clip.audio->getNumSamples() > 1)
                    {
                        const double seconds = (now - clip.startBeat) * 60.0 / tempo + clip.sourceOffset;
                        const double position = seconds * clip.sampleRate;
                        const int index = (int) position;
                        if (index >= 0 && index + 1 < clip.audio->getNumSamples())
                        {
                            const float fraction = (float) (position - index);
                            const float a = clip.audio->getSample(0, index);
                            const float b = clip.audio->getSample(0, index + 1);
                            mixed += a + (b - a) * fraction;
                        }
                    }
                    else
                    {
                        const double freq = 110.0 * std::pow(2.0, trackIndex / 12.0);
                        const double phase = std::fmod(now, 1.0) * juce::MathConstants<double>::twoPi * (freq / (tempo / 60.0));
                        mixed += (float) (std::sin(phase) * 0.15);
                    }
                }

                if (plugins[(size_t) trackIndex] == nullptr)
                {
                    for (const auto& note : track.notes)
                    {
                        if (now < note.startBeat || now >= note.startBeat + note.lengthBeats)
                            continue;
                        const int midiNote = juce::jlimit(0, 127, note.note);
                        const double freq = 440.0 * std::pow(2.0, (midiNote - 69) / 12.0);
                        float& phase = phases[(size_t) trackIndex * 128 + (size_t) midiNote];
                        mixed += (float) (std::sin(phase) * 0.15 * note.velocity);
                        phase += (float) (juce::MathConstants<double>::twoPi * freq / rate);
                        if (phase > juce::MathConstants<float>::twoPi)
                            phase -= juce::MathConstants<float>::twoPi;
                    }
                }

                mixed = lowFilters[(size_t) trackIndex].processSample(mixed);
                mixed = midFilters[(size_t) trackIndex].processSample(mixed);
                mixed = highFilters[(size_t) trackIndex].processSample(mixed);

                const float threshold = juce::jlimit(0.05f, 1.0f, track.compThreshold);
                const float ratio = juce::jmax(1.0f, track.compRatio);
                float& envelope = compEnv[(size_t) trackIndex];
                const float level = std::abs(mixed);
                envelope = level > envelope ? level : envelope * 0.9995f;
                if (ratio > 1.01f && envelope > threshold)
                    mixed *= std::pow(threshold / envelope, 1.0f - 1.0f / ratio);

                trackAudio.setSample(trackIndex, sample, mixed);
            }

            cursor += beatsPerSample;
            if (allowLoop && project->loopEnabled && cursor >= project->loopEnd)
                cursor = project->loopStart;
        }
    }

    if (running)
    {
        const double blockStart = beat.load();
        for (int trackIndex = 0; trackIndex < trackCount; ++trackIndex)
        {
            auto& track = project->tracks[(size_t) trackIndex];
            if (track.mute || (soloing && !track.solo))
            {
                rememberPeak(trackIndex, 0.0f);
                continue;
            }

            auto* plugin = plugins[(size_t) trackIndex].get();
            if (plugin != nullptr)
            {
                const int channels = juce::jmax(2, plugin->getTotalNumInputChannels(), plugin->getTotalNumOutputChannels());
                if (pluginBuffer.getNumChannels() < channels || pluginBuffer.getNumSamples() < numSamples)
                    pluginBuffer.setSize(channels, numSamples, false, false, true);
                pluginBuffer.clear();
                for (int channel = 0; channel < channels; ++channel)
                    pluginBuffer.copyFrom(channel, 0, trackAudio, trackIndex, 0, numSamples);

                juce::MidiBuffer midi;
                for (const auto& note : track.notes)
                {
                    const double startSample = (note.startBeat - blockStart) / beatsPerSample;
                    const double endSample = (note.startBeat + note.lengthBeats - blockStart) / beatsPerSample;
                    if (startSample >= 0.0 && startSample < numSamples)
                        midi.addEvent(juce::MidiMessage::noteOn(1, note.note, note.velocity), (int) startSample);
                    if (endSample >= 0.0 && endSample < numSamples)
                        midi.addEvent(juce::MidiMessage::noteOff(1, note.note), (int) endSample);
                }
                plugin->processBlock(pluginBuffer, midi);
                trackAudio.copyFrom(trackIndex, 0, pluginBuffer, 0, 0, numSamples);
            }

            const float angle = (track.pan + 1.0f) * 0.5f * juce::MathConstants<float>::halfPi;
            const float leftPan = std::cos(angle);
            const float rightPan = std::sin(angle);
            float trackPeak = 0.0f;
            for (int sample = 0; sample < numSamples; ++sample)
            {
                const float value = trackAudio.getSample(trackIndex, sample);
                const float gain = track.volumeAt(blockStart + sample * beatsPerSample);
                trackPeak = juce::jmax(trackPeak, std::abs(value * gain));
                if (numOutputs > 0)
                    outputs[0][sample] += value * leftPan * gain;
                if (numOutputs > 1)
                    outputs[1][sample] += value * rightPan * gain;
                else if (numOutputs == 1)
                    outputs[0][sample] += value * 0.5f * (leftPan + rightPan) * gain;
            }
            rememberPeak(trackIndex, trackPeak);
        }
        if (live)
            beat.store(cursor);
    }

    float busPeak = 0.0f;
    for (int channel = 0; channel < numOutputs; ++channel)
        if (outputs[channel] != nullptr)
            busPeak = juce::jmax(busPeak, juce::FloatVectorOperations::findMaximum(outputs[channel], numSamples));
    masterPeak.store(juce::jmax(busPeak, masterPeak.load() * 0.86f));

    for (int channel = 0; channel < numOutputs; ++channel)
        if (outputs[channel] != nullptr)
            juce::FloatVectorOperations::clip(outputs[channel], outputs[channel], -1.0f, 1.0f, numSamples);
}

void AudioEngine::audioDeviceIOCallbackWithContext(const float* const* inputChannelData, int numInputChannels,
                                                   float* const* outputChannelData, int numOutputChannels,
                                                   int numSamples, const juce::AudioIODeviceCallbackContext&)
{
    juce::ScopedLock sl(lock);
    const float* input = (numInputChannels > 0 && inputChannelData != nullptr) ? inputChannelData[0] : nullptr;
    renderUnlocked(input, outputChannelData, numOutputChannels, numSamples, true, true, true);
}

juce::Result AudioEngine::exportWav(const juce::File& file)
{
    juce::ScopedLock sl(lock);
    if (project == nullptr)
        return juce::Result::fail("No project");

    const double rate = sampleRate > 1000.0 ? sampleRate : 44100.0;
    const double endBeat = juce::jmax(1.0, project->getEndBeat());
    const int total = juce::jlimit(1, (int) (rate * 60.0 * 20.0), (int) ((endBeat * 60.0 / bpm.load()) * rate));
    juce::AudioBuffer<float> rendered(2, total);

    const double savedBeat = beat.load();
    const bool savedPlaying = playing.load();
    const bool savedRecording = recording.load();
    recording.store(false);
    playing.store(true);
    beat.store(0.0);
    lastWholeBeat = -1;

    int done = 0;
    while (done < total)
    {
        const int count = juce::jmin(256, total - done);
        float* channels[] = { rendered.getWritePointer(0, done), rendered.getWritePointer(1, done) };
        renderUnlocked(nullptr, channels, 2, count, true, false, false);
        done += count;
    }

    beat.store(savedBeat);
    playing.store(savedPlaying);
    recording.store(savedRecording);

    file.deleteFile();
    auto stream = std::unique_ptr<juce::FileOutputStream>(file.createOutputStream());
    if (stream == nullptr)
        return juce::Result::fail("Could not create the WAV file");
    juce::WavAudioFormat format;
    juce::StringPairArray meta;
    std::unique_ptr<juce::AudioFormatWriter> writer(format.createWriterFor(stream.release(), rate, 2, 16, meta, 0));
    if (writer == nullptr || !writer->writeFromAudioSampleBuffer(rendered, 0, rendered.getNumSamples()))
        return juce::Result::fail("Could not write the WAV file");
    return juce::Result::ok();
}
