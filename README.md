# JimiDAW

Native Windows DAW built with JUCE 8.

Repository: https://github.com/mitsos1975/JimiDAW

## Build

Install Visual Studio 2022 (Desktop development with C++) and CMake 3.22 or newer. Then, in the repository folder:

```bat
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

The program is `build/JimiDAW_artefacts/Release/JimiDAW.exe`.

The first configure downloads JUCE. That takes a few minutes.

## ASIO

ASIO is off until the Steinberg SDK is present. Copy it into `third_party/asiosdk` as described there, then:

```bat
cmake -S . -B build -DJIMIDAW_ASIO=ON
cmake --build build --config Release
```

Without that flag the app uses WASAPI.

## What this version does

- Audio device window (Audio button): input, output, sample rate, buffer size
- Play, stop, tempo, and click on the arrangement to move the playhead
- Metronome
- Eight tracks, mute, solo, arm, and faders
- Record the armed track (the red R) into a clip, then play that audio back
- Placeholder tones on clips that have no recording yet

## Record

1. Click Audio and choose the input that your microphone or interface is using.
2. Click the red R on the track you want.
3. Click Record. Click Record again, or Stop, to finish the clip.

Not in this version yet: saving a project, editing clip edges, a piano roll, or VST hosting.
