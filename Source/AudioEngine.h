#pragma once
#include <JuceHeader.h>
#include "Project.h"

class AudioEngine : public juce::AudioIODeviceCallback
{
public:
    AudioEngine();
    ~AudioEngine() override;

    juce::AudioDeviceManager& getDeviceManager() { return deviceManager; }

    void setProject(Project* newProject);
    void setPlaying(bool shouldPlay);
    bool isPlaying() const { return playing.load(); }
    void setBpm(double newBpm);
    double getBeat() const { return beat.load(); }
    void setBeat(double newBeat);

    void startRecording();
    Clip stopRecording();
    bool isRecording() const { return recording.load(); }

    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;
    void audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                          int numInputChannels,
                                          float* const* outputChannelData,
                                          int numOutputChannels,
                                          int numSamples,
                                          const juce::AudioIODeviceCallbackContext& context) override;

private:
    float renderSample(float input);

    juce::AudioDeviceManager deviceManager;
    Project* project = nullptr;
    std::atomic<bool> playing { false };
    std::atomic<bool> recording { false };
    std::atomic<double> beat { 0.0 };
    std::atomic<double> bpm { 120.0 };
    std::atomic<int> recordWrite { 0 };
    double sampleRate = 44100.0;
    double recordStartBeat = 0.0;
    double clickPhase = 0.0;
    int clickSamplesLeft = 0;
    int lastWholeBeat = -1;
    juce::AudioBuffer<float> recordBuffer;
};
