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

Without that flag the app uses WASAPI, which is enough to hear playback.

## What this version does

- Opens an audio device and shows the device setup menu
- Play, stop, and tempo
- Moving playhead and metronome
- Eight tracks, clips, mute, solo, and faders
- A tone while the playhead is inside a clip

Not in this version yet: recording, audio files on clips, piano roll, or VST hosting.
