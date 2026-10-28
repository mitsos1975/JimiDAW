#pragma once
#include <JuceHeader.h>
#include <memory>
#include <vector>

struct Clip
{
    double startBeat = 0.0;
    double lengthBeats = 4.0;
    juce::String name { "Clip" };
    std::shared_ptr<juce::AudioBuffer<float>> audio;
    double sampleRate = 44100.0;
};

struct Track
{
    juce::String name;
    juce::Colour colour;
    float volume = 0.8f;
    bool mute = false;
    bool solo = false;
    bool armed = false;
    std::vector<Clip> clips;
};

struct Project
{
    double bpm = 120.0;
    int armedTrack = 0;
    std::vector<Track> tracks;

    static Project demo()
    {
        Project project;
        const juce::Colour colours[] = {
            juce::Colour(0xff3d8bfd), juce::Colour(0xff3ddc97), juce::Colour(0xfff0a202),
            juce::Colour(0xffe85d75), juce::Colour(0xffb388ff), juce::Colour(0xff4dd0e1),
            juce::Colour(0xffff8a65), juce::Colour(0xff90a4ae)
        };
        for (int i = 0; i < 8; ++i)
        {
            Track track;
            track.name = "Audio " + juce::String(i + 1);
            track.colour = colours[i];
            track.armed = i == 0;
            track.clips.push_back({ 0.0, 8.0, "Clip 1" });
            if (i % 2 == 0)
                track.clips.push_back({ 12.0, 4.0, "Clip 2" });
            project.tracks.push_back(std::move(track));
        }
        return project;
    }

    bool anySolo() const
    {
        for (const auto& track : tracks)
            if (track.solo)
                return true;
        return false;
    }
};
