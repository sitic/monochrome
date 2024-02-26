# Monochrome — Video viewer for scientific monochromatic videos

> ⚠️ An initial release is currently in preparation. The install instruction are not yet up-to-date.

Monochrome is a viewer for scientific monochromatic videos with high-dynamic range.

It is designed for viewing high-speed monochromatic fluorescence video data from scientific cameras and meet our spefic needs for cariac optical mapping data (together with [optimap](https://github.com/cardiacvision/optimap)):
* Support for high-dynamic range (16-bit, 32-bit float)
* Playback of multiple videos in sync
* High-speed playback (as fast as the hardware allows)
* Viewing of optical traces (average intensity in a region of interest over time)
* Rendering of layers on top of videos with transparency
* Rendering of point positions over time (e.g. for tracking, optical flow visualization)
* Exporting of videos as PNG images or MP4 videos with precise control over the frame rate
* Cross-platform (Linux, Windows, MacOS)

It is designed to be fast and lightweight, i.e. it uses memory-mapping to load video files to avoid copying the data into RAM. 

## Standalone Application and Python Library

Monochrome can be used as a standalone application or as a Python library. The standalone application is a simple video viewer with a minimalistic user interface. The Python library allows to load and play videos from Python scripts and Jupyter notebooks, see the [Python Quickstart](#python-quickstart).

For the standalone application, download the latest release from the [releases page](https://github.com/sitic/monochrome/-/releases) and run the executable. On Winodws you may need to install [Microsoft Visual C++ Redistributable 2019](https://aka.ms/vs/16/release/vc_redist.x86.exe).

For the Python library, install it with pip:

```bash
python -m pip install monochrome
```

## Native Video File Formats
Monochrome supports the following video formats:

* `.npy` (NumPy array) with shape (time, width, height). The data type can be float, integer (uint8, uint16, etc.), or boolean.
* `.dat` MultiRecorder format
* `.dat` (binary) with shape (time, width, height) and data type float32

Drag & drop the file into the window or associate the file extension with Monochrome.

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
python -m pip install monochrome
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

# Several videos can be shown at the same time, they will be played in sync if they have the same length
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

## Development
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
