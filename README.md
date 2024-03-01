# Monochrome — Viewer for scientific monochromatic video data

> ⚠️ An initial release is currently in preparation. The install instruction are not yet up-to-date.

Monochrome is a viewer for scientific monochromatic videos with high-dynamic range.

It is designed for viewing high-speed monochromatic fluorescence video data from scientific cameras and meet our spefic needs for cardiac optical mapping data (together with [optimap](https://github.com/cardiacvision/optimap)):
* Support for high-dynamic range (uint16, 32-bit float)
* Playback of multiple videos in sync
* High-speed playback with precise frame-rate control
* Viewing of optical traces (average intensity in a region of interest over time)
* Rendering of layers on top of videos with transparency
* Rendering of point positions over time (e.g. for tracking or optical flow visualization)
* Exporting of videos as PNG images or MP4 videos with precise control over the frame rate
* Cross-platform (Linux, Windows, MacOS)

It is designed to be fast and lightweight, i.e. it uses memory-mapping to load video files to avoid copying the data into RAM. 

## Standalone Application and Python Library

Monochrome can be used as a standalone application or as a Python library. The standalone application is a simple video viewer with a minimalistic user interface. The Python library allows to load and play videos from Python scripts and Jupyter notebooks, see the [Python Quickstart](#python-quickstart).

For the standalone application, download the latest release from the [releases page](https://github.com/sitic/monochrome/-/releases) and run the executable. On Winodws you may need to install [Microsoft Visual C++ Redistributable 2019](https://aka.ms/vs/16/release/vc_redist.x86.exe).

For the Python library, install it with pip:

```bash
python -m pip install monochrome-viewer
```

## Native Video File Formats
Monochrome supports the following video file formats:

* `.npy` (NumPy array) with shape (time, width, height). The data type can be float, integer (uint8, uint16, etc.), or boolean.
* `.dat` (binary) with shape (time, width, height) and data type float32
* `.dat` MultiRecorder format

Drag & drop the file into the window or associate the file extension with Monochrome to open it with a double-click.

## Usage & Key Bindings

Click in a video to view optical traces (average intensity in a region of interest over time). Click and drag to move the region of interest. Right-click to remove the region of interest.

Keyboard shortcuts:

| Keybinding &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; | Action &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; |
| --- | --- |
| `Ctrl + q` | Quit Monochrome |
| `Esc` or `q` | Close focused recording |
| `Space` | Play/Pause |
| `Up` | Increase playback speed (frame skip) |
| `Down` | Decrease playback speed (frame skip) |
| `0` or `r` | Reset playback to beginning |
| `Left` | Skip to next frame |
| `Right` | Skip to previous frame |
| `Shift + Left` | 10x previous frame |
| `Shift + Right` | 10x next frame |
| `Ctrl + Left` | Previous frame in focused recording only |
| `Ctrl + Right` | Next frame in focused recording only |
| `Ctrl + Left + Shift` | 10x previous frame in focused recording only |
| `Ctrl + Right + Shift` | 10x next frame in focused recording only |
| `p` | Save screenshot of focused recording |
| `s` | Sync playback of all recordings |

## Python Quickstart

To install Monochrome with Python integration run the following command on Linux or macOS:
```bash
python -m pip install monochrome-viewer
```
On Windows you might need to run
```bash
py -m pip install monochrome-viewer
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

# Videos of type float or unsigned integer (uint8, uint16 etc.) are natively supported by Monochrome
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

### Dependencies

#### Linux

Install the following packages:
```bash
sudo apt install build-essential cmake mesa-utils xorg-dev
```

#### MacOS

Download [CMake](https://cmake.org/download/) and Xcode.

#### Windows

Download [CMake](https://cmake.org/) and a C++ compiler like Microsoft Visual C++ or MinGW.

### Compilation

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
