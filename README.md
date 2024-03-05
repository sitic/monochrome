# Monochrome: Viewer for Monochromatic Video Data
[![docs](https://readthedocs.org/projects/monochrome/badge/?version=latest&style=)](https://monochrome.readthedocs.org)
[![tests](https://github.com/sitic/monochrome/actions/workflows/build.yml/badge.svg)](https://github.com/sitic/monochrome/actions/workflows/build.yml)
[![PyPI](https://img.shields.io/pypi/v/monochrome-viewer.svg)](https://pypi.org/project/monochrome-viewer/)
[![Supported Python versions](https://img.shields.io/pypi/pyversions/monochrome-viewer.svg)](https://python.org)

Monochrome is a lightweight and fast video viewer for scientific monochromatic videos with high-dynamic range (float, uint16, etc.).

It is designed for viewing high-speed monochromatic fluorescence video data from scientific cameras and meet our specific needs for cardiac optical mapping data (together with [optimap](https://github.com/cardiacvision/optimap)):
* Support for high-dynamic range (uint16, 32-bit float) data with sliders to adjust intensity range
* Playback of multiple videos in sync
* High-speed playback with precise frame-rate control
* Viewing of optical traces (average intensity in a region of interest over time)
* Rendering of layers on top of videos with transparency
* Rendering of point positions over time (e.g. for tracking or optical flow visualization)
* Exporting videos as a sequence of PNG images or MP4 videos with control over frame rate and frame skipping
* Cross-platform (Linux, Windows, MacOS)

It is designed to be fast and lightweight, i.e. it uses memory-mapping to load video files to avoid copying the data into RAM.

## Installation

There are two ways to install Monochrome: as a standalone application and/or with its Python interface through pip.

In the standalone application, supported video files can be loaded by drag & drop them into the window or by associating the file extension with Monochrome to open them with a double-click. The Python interface allows to load and play videos from Python scripts and Jupyter notebooks.

### Standalone Application

Download the relevant executable (Windows, macOS, or Linux) from the latest [release page](https://github.com/sitic/monochrome/releases/latest). See the [installation instructions](https://monochrome.readthedocs.io/latest/installation_standalone/) for details.

### Python Library

The Python library includes all necessary files and does not require the installation of the standalone Monochrome application. Open a terminal window and run the following command:

```bash
python -m pip install monochrome-viewer
```

See the [Python installation guide](https://monochrome.readthedocs.io/latest/installation_python/) for further details. To start the viewer in standalone mode, run:
```bash
python -m monochrome
```

See the [tutorial](https://monochrome.readthedocs.io/latest/tutorial/) for an introduction to the Python library, here is a brief overview:

```python
import monochrome as mc
import numpy as np

# Create some video with shape (time, height, width) as a numpy array
video = np.random.rand(500, 256, 256)

# Display the video, see the tutorial for more details and options.
# Monochrome should automatically start and show the video in a loop.
mc.show_video(video, name="First Video", cmap='viridis', vmin=0, vmax=1)

# Play second video in sync with the first (note that the videos should have the same length)
video2 = (np.random.rand(500, 256, 256) * 65535).astype(dtype=np.uint16)
mc.show_video(video2, name="Second Video", comment="This is a uint16 video", bitrange="uint16")
# `bitrange` argument is optional, Monochrome will auto-detect the data type

# Layers can be added on top of video
overlay = np.random.rand(500, 256, 256)
overlay[:, 64:192, :] = np.nan # NaN values will be transparent pixels, see tutorial
mc.show_layer(overlay, parent="Second Video", cmap='PRGn', opacity='centered')

# List of functions:
# mc.show() is a shortcut for mc.show_video()/show_layer()/show_image()/show_file(),
#           it will try to auto-detect the input type and call the appropriate function.
# mc.show_video() to show videos
# mc.show_image() to show single images
# mc.show_layer() to show layers on top of videos/images
# mc.show_points() to visualize point positions over time over videos
# mc.show_flow() to visualize optical flow fields over time
# mc.show_file() to load videos from file in Monochrome
# mc.launch() to start Monochrome from Python
```

## Native Video File Formats
Monochrome supports loading the following video file formats:

* `.npy`, NumPy array with shape (time, width, height). The data type can be float (np.float32, np.float64), integer (uint8, uint16, etc.), or boolean.
* `.dat`, raw binary file with shape (time, width, height) and data type float32
* `.dat`, MultiRecorder file format (used in the cardiac optical mapping community)

Drag & drop the file into the window or associate the file extension with Monochrome to open it with a double-click.

## Usage & Key Bindings

Adjust settings for each video in the main control window. To view optical traces (average intensity in a region of interest over time), click in a video. Click and drag to move the region of interest. Right-click to remove the region of interest.

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

## Additional Resources

* [Documentation](https://monochrome.readthedocs.io)
* [Python Tutorial](https://monochrome.readthedocs.io/latest/tutorial/)
* [PyPI package](https://pypi.org/project/monochrome-viewer/)
* [GitHub repository](https://github.com/sitic/monochrome)

## License

Monochrome is licensed under the MIT License.
