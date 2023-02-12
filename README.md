# Monochrome — Video viewer for scientific monochromatic videos

This is a simple viewer to play MultiRecorder and binary files. Drag & drop the file into the window or pass the paths as arguments.

## Key Bindings

| Keybinding | Action |
| --- | --- |
| `Ctrl + q` | Quit Monochrome |
| `Esc` or `q` | Close focused recording |
| `Space` | Play/Pause |
| `Up` | Increase playback speed |
| `Down` | Decrease playback speed |
| `0` or `r` | Reset to beginning |
| `Left` | Previous frame |
| `Right` | Next frame |
| `Shift + Left` | Previous frame (10x) |
| `Shift + Right` | Next frame (10x) |
| `p` | Save screenshot of focused recording |
| `s` | Sync playback of all recordings |
| `Ctrl + Left (+ Shift)` | Previous frame in focused recording only |
| `Ctrl + Right (+ Shift)` | Next frame in focused recording only |

## Python Quickstart

To install Monochrome with Python integration run the following command on Linux or macOS:
```bash
python3 -m pip install monochrome
```
On Windows you might need to run
```bash
py -m pip install monochrome
```
The installation of the standalone Monochrome application is *not* required.


```python
import monochrome as mc
import numpy as np

# Load some video with shape (time, width, height)
video = np.random.rand((500, 128, 128))

# Play the video in a loop, name it "Our Test Video"
mc.show(video, "Our Test Video", cmap='viridis')

# The video is copied to Monochrome and will play in a loop.
# It plays independently from the python process, it does not block it.
# The playback speed etc. can be controlled in Monochrome, as well as the export as png images or as mp4 video

# Videos of type float or integer (uint8, uint16 ect.) are natively supported by Monochrome
video2 = (np.random.rand((500, 128, 128)) * 65535).astype(dtype=np.uint16)

# Several videos can be shown at the same time, it does not block the python process
mc.show(video2, "Another Video", cmap='gray',
        # we can also add some metadata
        date="2020-04-29-13-10-27", comment="Blebbistatin", fps=500, duration_seconds=30)

# Layers can be added on top of video.
# Pixels with value NaN are not displayed, instead the layer/video below is shown
overlay = np.random.rand((500, 128, 128))
overlay[32:96, 32:96] = np.nan

mc.show_layer(overlay, "Overlay Name", cmap='hsv')

# By default, layers are added to the last video loaded, unless the parent name is specified.
mc.show_layer(overlay, "Phase", parent="Our Test Video", cmap='hsv')

```

## Standalone Application

* [Linux](https://gitlab.com/cardiac-vision/monochrome/builds/artifacts/master/file/build/Monochrome.AppImage?job=gcc%20Release)
  (mark as executable and run it)
* [Windows](https://gitlab.gwdg.de/lebert/monochrome/builds/artifacts/master/file/build/Release/Monochrome.exe?job=windows%20Release)
  (you may need to install [Microsoft Visual C++ Redistributable 2019](https://aka.ms/vs/16/release/vc_redist.x86.exe))
* [MacOS](https://gitlab.com/cardiac-vision/monochrome/builds/artifacts/master/file/dist/Monochrome-0.1.0-Darwin.dmg?job=macos)

## Build Requirements

### Linux

```bash
sudo apt install build-essential cmake mesa-utils xorg-dev
```

### MacOS

Download [CMake](https://cmake.org/download/) and Xcode.

### Windows

Download [CMake](https://cmake.org/) and a C++ compiler like Microsoft Visual C++ or MinGW.

## Compilation

This project uses CMake to generate cross-platform build configurations. Either use cmake-gui or the terminal from the project folder (where CMakeList is located):

```bash
# Create a subfolder (generally called build)
mkdir build
cd build

# Generate a project for the default platform
cmake ..

# Alternatively display the available platforms and generate the project for the platform of your choice
cmake --help    # (optional)
cmake .. -G "Xcode"    # (example)
cmake .. -G "Unix Makefiles"    # (example)
cmake .. -G "Visual Studio 15 2017 Win64"   # (example)

# Then use your IDE or use CMake to compile
cmake --build .

# Linux only: installation
sudo cmake --build . --target install
```
