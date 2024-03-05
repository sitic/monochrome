# Monochrome: Viewer for Monochromatic Video Data
[![docs](https://readthedocs.org/projects/monochrome/badge/?version=latest&style=)](https://monochrome.readthedocs.org)
[![tests](https://github.com/sitic/monochrome/actions/workflows/build.yml/badge.svg)](https://github.com/sitic/monochrome/actions/workflows/build.yml)
[![PyPI](https://img.shields.io/pypi/v/monochrome-viewer.svg)](https://pypi.org/project/monochrome-viewer/)
[![Supported Python versions](https://img.shields.io/pypi/pyversions/monochrome-viewer.svg)](https://python.org)

Monochrome is a lightweight and fast video viewer for scientific monochromatic videos with high-dynamic range.

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

## Installation

There are two ways to install Monochrome: as a standalone application and/or with it's Python interface through pip.

In the standalone application, supported video files can be loaded by drag & drop them into the window or by associating the file extension with Monochrome to open them with a double-click. The Python interface allows to load and play videos from Python scripts and Jupyter notebooks.

### Standalone Application

Download the relevant executable (Windows, macOS, or Linux) from the latest [release page](https://github.com/sitic/monochrome/releases/latest), see the [installation instructions](https://monochrome.readthedocs.io/latest/installation_standalone/) for details. 


### Python Library

The Python library includes all necessary files and does not require the installation of the standalone Monochrome application. Open a terminal window and run the following command:

```bash
python -m pip install monochrome-viewer
```

See the [python installation instructions](https://monochrome.readthedocs.io/latest/installation_python/) for further details. To start the viewer in standalone mode, run:
```bash
python -m pip -m monochrome
```

See the [tutorial](https://monochrome.readthedocs.io/latest/tutorial/) for an introduction to the Python library, here is a brief overview:

```python
import monochrome as mc
import numpy as np

# Create some video with shape (time, height, width) as a numpy array
video = np.random.rand(500, 256, 256)

# Show the video, see Tutorial for more details and options
mc.show_video(video, name="First Video", cmap='viridis', vmin=0, vmax=1)
# Monochrome should automatically start and show the video in a loop

# Several videos can be shown at the same time, they will be played in sync if they have the same length
video2 = (np.random.rand(500, 256, 256) * 65535).astype(dtype=np.uint16)
mc.show_video(video2, name="Second Video", comment="Blebbistatin", bitrange="uint16")
# Monochrome will auto-detect the data type and bitrange of the video and display it accordingly

# Layers can be added on top of video
overlay = np.random.rand(500, 256, 256)
overlay[:, 64:192, :] = np.nan # NaN values are not displayed, instead the video below is shown
mc.show_layer(overlay, parent="Second Video", cmap='PRGn', opacity='centered')

# Additional functions:
# mc.show() is a shortcut for mc.show_video()/show_layer()/show_image()/show_file(), it will try to auto-detect the input type and call the appropriate function
# mc.show_image() can be used to show single images
# mc.show_file() can be used to load videos from file
# mc.show_points() can be used to visualize point positions over time
# mc.show_flow() can be used to visualize optical flow fields over time
```

## Native Video File Formats
Monochrome supports loading the following video file formats:

* `.npy`, NumPy array with shape (time, width, height). The data type can be float (np.float32, np.float64), integer (uint8, uint16, etc.), or boolean.
* `.dat`, raw binary file with shape (time, width, height) and data type float32
* `.dat`, MultiRecorder file format (used in the cardiac optical mapping community)

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

## Additional Resources

* [Documentation](https://monochrome.readthedocs.io)
* [Python Tutorial](https://monochrome.readthedocs.io/latest/tutorial/)
* [PyPI package](https://pypi.org/project/monochrome-viewer/)
* [GitHub repository](https://github.com/sitic/monochrome)

## License

Monochrome is licensed under the MIT License.
