#pragma once
#include <JuceHeader.h>
#include "Project.h"

class AudioEngine : public juce::AudioIODeviceCallback
{
public:
    AudioEngine();
    ~AudioEngine() override;

    juce::AudioDeviceManager& getDeviceManager() { return deviceManager; }
    juce::CriticalSection& getLock() { return lock; }

    void setProject(Project* newProject);
    void setPlaying(bool shouldPlay);
    bool isPlaying() const { return playing.load(); }
    void setBpm(double newBpm);
    double getBeat() const { return beat.load(); }
    void setBeat(double newBeat);

    void startRecording();
    Clip stopRecording();
    bool isRecording() const { return recording.load(); }

    juce::String loadPlugin(int trackIndex, const juce::File& file);
    void clearPlugin(int trackIndex);
    juce::AudioPluginInstance* getPlugin(int trackIndex);
    juce::Result exportWav(const juce::File& file);
    float getTrackPeak(int trackIndex) const;
    float getMasterPeak() const { return masterPeak.load(); }

    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;
    void audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                          int numInputChannels,
                                          float* const* outputChannelData,
                                          int numOutputChannels,
                                          int numSamples,
                                          const juce::AudioIODeviceCallbackContext& context) override;

private:
    void ensureDsp(int trackCount);
    void renderUnlocked(const float* input, float* const* outputs, int numOutputs, int numSamples,
                        bool live, bool withClick, bool allowLoop);
    void rememberPeak(int trackIndex, float peak);

    juce::AudioDeviceManager deviceManager;
    juce::AudioPluginFormatManager formatManager;
    juce::CriticalSection lock;
    Project* project = nullptr;
    std::atomic<bool> playing { false };
    std::atomic<bool> recording { false };
    std::atomic<double> beat { 0.0 };
    std::atomic<double> bpm { 120.0 };
    std::atomic<int> recordWrite { 0 };
    std::atomic<float> trackPeaks[64] {};
    std::atomic<float> masterPeak { 0.0f };
    double sampleRate = 44100.0;
    int blockSize = 512;
    double recordStartBeat = 0.0;
    double clickPhase = 0.0;
    int clickSamplesLeft = 0;
    int lastWholeBeat = -1;
    juce::AudioBuffer<float> recordBuffer;
    juce::AudioBuffer<float> trackAudio;
    juce::AudioBuffer<float> pluginBuffer;
    juce::AudioBuffer<float> sendBus;
    std::vector<float> delayL;
    std::vector<float> delayR;
    int delayPosL = 0;
    int delayPosR = 0;
    std::vector<juce::dsp::IIR::Filter<float>> lowFilters;
    std::vector<juce::dsp::IIR::Filter<float>> midFilters;
    std::vector<juce::dsp::IIR::Filter<float>> highFilters;
    std::vector<float> compEnv;
    std::vector<float> phases;
    std::vector<std::unique_ptr<juce::AudioPluginInstance>> plugins;
};
