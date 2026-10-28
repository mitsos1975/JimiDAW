#pragma once
#include <JuceHeader.h>
#include <memory>
#include <vector>

struct MidiNote
{
    double startBeat = 0.0;
    double lengthBeats = 1.0;
    int note = 60;
    float velocity = 0.8f;
};

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
    juce::Colour colour { juce::Colour(0xff3d8bfd) };
    float volume = 0.8f;
    float pan = 0.0f;
    bool mute = false;
    bool solo = false;
    bool armed = false;
    float lowDb = 0.0f;
    float midDb = 0.0f;
    float highDb = 0.0f;
    float compThreshold = 1.0f;
    float compRatio = 1.0f;
    juce::String pluginPath;
    std::vector<Clip> clips;
    std::vector<MidiNote> notes;
};

struct Project
{
    double bpm = 120.0;
    int armedTrack = 0;
    int selectedTrack = 0;
    int selectedClip = -1;
    int selectedNote = -1;
    bool loopEnabled = false;
    bool metronome = true;
    double loopStart = 0.0;
    double loopEnd = 8.0;
    std::vector<Track> tracks;

    static Track makeTrack(int index)
    {
        const juce::Colour colours[] = {
            juce::Colour(0xff3d8bfd), juce::Colour(0xff3ddc97), juce::Colour(0xfff0a202),
            juce::Colour(0xffe85d75), juce::Colour(0xffb388ff), juce::Colour(0xff4dd0e1),
            juce::Colour(0xffff8a65), juce::Colour(0xff90a4ae)
        };
        Track track;
        track.name = "Track " + juce::String(index + 1);
        track.colour = colours[index % 8];
        return track;
    }

    static Project demo()
    {
        Project project;
        for (int i = 0; i < 8; ++i)
        {
            auto track = makeTrack(i);
            track.armed = i == 0;
            track.clips.push_back({ 0.0, 8.0, "Clip 1" });
            if (i % 2 == 0)
                track.clips.push_back({ 12.0, 4.0, "Clip 2" });
            if (i == 1)
            {
                track.notes.push_back({ 0.0, 1.0, 60, 0.8f });
                track.notes.push_back({ 1.0, 1.0, 64, 0.8f });
                track.notes.push_back({ 2.0, 1.0, 67, 0.8f });
                track.notes.push_back({ 3.0, 1.0, 72, 0.8f });
            }
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

    double getEndBeat() const
    {
        double end = 8.0;
        for (const auto& track : tracks)
        {
            for (const auto& clip : track.clips)
                end = juce::jmax(end, clip.startBeat + clip.lengthBeats);
            for (const auto& note : track.notes)
                end = juce::jmax(end, note.startBeat + note.lengthBeats);
        }
        return end;
    }
};
