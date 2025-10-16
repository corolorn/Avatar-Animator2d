# Avatar Animator (Win32 C++)

A single-exe Windows 10/11 app that switches between two images based on microphone level, with a chroma key background for OBS. No external frameworks.

## Features
- Load two images (Idle / Talking). PNG with alpha supported
- Microphone selection from Windows devices (MMDevice)
- Threshold with hysteresis; adjustable delay and crossfade
- Chroma key color picker; Topmost toggle
- Separate Canvas window without UI, for clean OBS capture

## Build (MSVC + CMake)
```bat
cd G:\projects\gav-gif
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```
Result: `build\Release\gav_gif.exe`

## Usage
1. Run `gav_gif.exe`
2. Load Idle and Talking images
3. Pick your microphone from the dropdown
4. Adjust Threshold, Delay (ms), Fade (ms), and Chroma
5. Click "Open Canvas" to open a clean window for OBS (Window Capture + Chroma Key)

## Tech
- Win32 API, C++17
- Audio: WASAPI (IMMDevice, IAudioClient, IAudioCaptureClient)
- Images: WIC -> 32bpp PBGRA -> AlphaBlend
- Rendering: GDI double buffering, per-pixel alpha, chroma fill

## License
MIT
