#include "Session.h"

namespace
{
bool writeMonoWav(const juce::File& file, const juce::AudioBuffer<float>& buffer, double sampleRate)
{
    file.deleteFile();
    auto stream = std::unique_ptr<juce::FileOutputStream>(file.createOutputStream());
    if (stream == nullptr || stream->failedToOpen())
        return false;

    juce::WavAudioFormat format;
    juce::StringPairArray meta;
    std::unique_ptr<juce::AudioFormatWriter> writer(format.createWriterFor(stream.release(),
                                                                           sampleRate,
                                                                           1,
                                                                           16,
                                                                           meta,
                                                                           0));
    if (writer == nullptr)
        return false;

    return writer->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples());
}
}

std::shared_ptr<juce::AudioBuffer<float>> Session::readMonoWav(const juce::File& file, double& sampleRate)
{
    juce::AudioFormatManager manager;
    manager.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader(manager.createReaderFor(file));
    if (reader == nullptr)
        return {};

    sampleRate = reader->sampleRate;
    auto buffer = std::make_shared<juce::AudioBuffer<float>>(1, (int) reader->lengthInSamples);
    reader->read(buffer.get(), 0, buffer->getNumSamples(), 0, true, false);
    return buffer;
}

juce::Result Session::save(const Project& project, const juce::File& file)
{
    auto audioDir = file.getParentDirectory().getChildFile(file.getFileNameWithoutExtension() + "_audio");
    audioDir.createDirectory();

    auto* root = new juce::DynamicObject();
    root->setProperty("bpm", project.bpm);
    root->setProperty("loopEnabled", project.loopEnabled);
    root->setProperty("loopStart", project.loopStart);
    root->setProperty("loopEnd", project.loopEnd);
    root->setProperty("metronome", project.metronome);

    juce::Array<juce::var> tracks;
    for (size_t ti = 0; ti < project.tracks.size(); ++ti)
    {
        const auto& track = project.tracks[ti];
        auto* obj = new juce::DynamicObject();
        obj->setProperty("name", track.name);
        obj->setProperty("colour", track.colour.toDisplayString(true));
        obj->setProperty("volume", track.volume);
        obj->setProperty("pan", track.pan);
        obj->setProperty("mute", track.mute);
        obj->setProperty("solo", track.solo);
        obj->setProperty("armed", track.armed);
        obj->setProperty("lowDb", track.lowDb);
        obj->setProperty("midDb", track.midDb);
        obj->setProperty("highDb", track.highDb);
        obj->setProperty("compThreshold", track.compThreshold);
        obj->setProperty("compRatio", track.compRatio);
        obj->setProperty("plugin", track.pluginPath);

        juce::Array<juce::var> clips;
        for (size_t ci = 0; ci < track.clips.size(); ++ci)
        {
            const auto& clip = track.clips[ci];
            auto* clipObj = new juce::DynamicObject();
            clipObj->setProperty("start", clip.startBeat);
            clipObj->setProperty("length", clip.lengthBeats);
            clipObj->setProperty("name", clip.name);
            clipObj->setProperty("sampleRate", clip.sampleRate);
            clipObj->setProperty("offset", clip.sourceOffset);
            if (clip.audio != nullptr && clip.audio->getNumSamples() > 0)
            {
                const auto wavName = juce::String(ti) + "_" + juce::String(ci) + ".wav";
                if (!writeMonoWav(audioDir.getChildFile(wavName), *clip.audio, clip.sampleRate))
                    return juce::Result::fail("Could not write " + wavName);
                clipObj->setProperty("file", wavName);
            }
            clips.add(juce::var(clipObj));
        }
        obj->setProperty("clips", clips);

        juce::Array<juce::var> notes;
        for (const auto& note : track.notes)
        {
            auto* noteObj = new juce::DynamicObject();
            noteObj->setProperty("start", note.startBeat);
            noteObj->setProperty("length", note.lengthBeats);
            noteObj->setProperty("note", note.note);
            noteObj->setProperty("velocity", note.velocity);
            notes.add(juce::var(noteObj));
        }
        obj->setProperty("notes", notes);

        juce::Array<juce::var> automation;
        for (const auto& point : track.volumeAutomation)
        {
            auto* pointObj = new juce::DynamicObject();
            pointObj->setProperty("beat", point.beat);
            pointObj->setProperty("value", point.value);
            automation.add(juce::var(pointObj));
        }
        obj->setProperty("volumeAutomation", automation);
        tracks.add(juce::var(obj));
    }
    root->setProperty("tracks", tracks);

    if (!file.replaceWithText(juce::JSON::toString(juce::var(root), true)))
        return juce::Result::fail("Could not write the project file");
    return juce::Result::ok();
}

juce::Result Session::load(Project& project, const juce::File& file)
{
    auto parsed = juce::JSON::parse(file.loadFileAsString());
    auto* root = parsed.getDynamicObject();
    if (root == nullptr)
        return juce::Result::fail("Not a JimiDAW project");

    auto audioDir = file.getParentDirectory().getChildFile(file.getFileNameWithoutExtension() + "_audio");
    Project loaded;
    loaded.bpm = (double) root->getProperty("bpm");
    if (loaded.bpm < 40.0)
        loaded.bpm = 120.0;
    loaded.loopEnabled = (bool) root->getProperty("loopEnabled");
    loaded.loopStart = (double) root->getProperty("loopStart");
    loaded.loopEnd = (double) root->getProperty("loopEnd");
    if (loaded.loopEnd <= loaded.loopStart)
        loaded.loopEnd = loaded.loopStart + 8.0;
    loaded.metronome = root->hasProperty("metronome") ? (bool) root->getProperty("metronome") : true;

    auto tracksVar = root->getProperty("tracks");
    if (auto* tracks = tracksVar.getArray())
    {
        for (int i = 0; i < tracks->size(); ++i)
        {
            auto* obj = (*tracks)[i].getDynamicObject();
            if (obj == nullptr)
                continue;
            Track track = Project::makeTrack(i);
            track.name = obj->getProperty("name").toString();
            track.colour = juce::Colour::fromString(obj->getProperty("colour").toString());
            track.volume = (float) (double) obj->getProperty("volume");
            track.pan = (float) (double) obj->getProperty("pan");
            track.mute = (bool) obj->getProperty("mute");
            track.solo = (bool) obj->getProperty("solo");
            track.armed = (bool) obj->getProperty("armed");
            track.lowDb = (float) (double) obj->getProperty("lowDb");
            track.midDb = (float) (double) obj->getProperty("midDb");
            track.highDb = (float) (double) obj->getProperty("highDb");
            track.compThreshold = obj->hasProperty("compThreshold") ? (float) (double) obj->getProperty("compThreshold") : 1.0f;
            track.compRatio = obj->hasProperty("compRatio") ? (float) (double) obj->getProperty("compRatio") : 1.0f;
            track.pluginPath = obj->getProperty("plugin").toString();

            if (auto* clips = obj->getProperty("clips").getArray())
            {
                for (const auto& clipVar : *clips)
                {
                    auto* clipObj = clipVar.getDynamicObject();
                    if (clipObj == nullptr)
                        continue;
                    Clip clip;
                    clip.startBeat = (double) clipObj->getProperty("start");
                    clip.lengthBeats = (double) clipObj->getProperty("length");
                    clip.name = clipObj->getProperty("name").toString();
                    clip.sampleRate = clipObj->hasProperty("sampleRate") ? (double) clipObj->getProperty("sampleRate") : 44100.0;
                    clip.sourceOffset = clipObj->hasProperty("offset") ? (double) clipObj->getProperty("offset") : 0.0;
                    const auto wavName = clipObj->getProperty("file").toString();
                    if (wavName.isNotEmpty())
                    {
                        double rate = clip.sampleRate;
                        clip.audio = readMonoWav(audioDir.getChildFile(wavName), rate);
                        if (clip.audio != nullptr)
                            clip.sampleRate = rate;
                    }
                    track.clips.push_back(std::move(clip));
                }
            }

            if (auto* notes = obj->getProperty("notes").getArray())
            {
                for (const auto& noteVar : *notes)
                {
                    auto* noteObj = noteVar.getDynamicObject();
                    if (noteObj == nullptr)
                        continue;
                    MidiNote note;
                    note.startBeat = (double) noteObj->getProperty("start");
                    note.lengthBeats = (double) noteObj->getProperty("length");
                    note.note = (int) noteObj->getProperty("note");
                    note.velocity = (float) (double) noteObj->getProperty("velocity");
                    track.notes.push_back(note);
                }
            }

            if (auto* automation = obj->getProperty("volumeAutomation").getArray())
            {
                for (const auto& pointVar : *automation)
                {
                    auto* pointObj = pointVar.getDynamicObject();
                    if (pointObj == nullptr)
                        continue;
                    AutomationPoint point;
                    point.beat = (double) pointObj->getProperty("beat");
                    point.value = (float) (double) pointObj->getProperty("value");
                    track.volumeAutomation.push_back(point);
                }
            }

            if (track.armed)
                loaded.armedTrack = (int) loaded.tracks.size();
            loaded.tracks.push_back(std::move(track));
        }
    }

    if (loaded.tracks.empty())
        return juce::Result::fail("The project has no tracks");

    project = std::move(loaded);
    return juce::Result::ok();
}
