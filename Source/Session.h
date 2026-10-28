#pragma once
#include <JuceHeader.h>
#include "Project.h"

struct Session
{
    static juce::Result save(const Project& project, const juce::File& file);
    static juce::Result load(Project& project, const juce::File& file);
    static std::shared_ptr<juce::AudioBuffer<float>> readMonoWav(const juce::File& file, double& sampleRate);
};
