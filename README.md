# JimiDAW

Native Windows DAW built with JUCE 8.

https://github.com/mitsos1975/JimiDAW

## Build

```bat
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

The program is `build/JimiDAW_artefacts/Release/JimiDAW.exe`.

The first configure downloads JUCE. Pull the latest `main` before building.

ASIO stays off until the Steinberg SDK is in `third_party/asiosdk`. Then configure with `-DJIMIDAW_ASIO=ON`.

## In this version

- Audio device window, WASAPI, and optional ASIO
- Play, stop, space bar, tempo, metronome, and loop
- Click, drag, and resize clips. Shift-wheel scrolls. Wheel zooms. Right-click or Delete removes a clip
- Record the armed track, import WAV, and export a stereo mix
- Save and open a `.jimi.json` project, with recordings stored beside it
- Piano roll, computer-keyboard notes, mute, solo, arm, volume, and pan
- Per-track low, mid, and high EQ, plus a compressor
- Load a VST3 on the selected track and open its editor
- Add tracks

This is not yet every function in Cubase or Logic. The next pieces are automation, MIDI file import, quantize, and a fuller mixer.
