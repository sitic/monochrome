"""Monochrome: Viewer for monochromatic video and image data."""

from ._version import version as __version__
from .ipc import (
                  BitRange,
                  ColorMap,
                  OpacityFunction,
                  close_video,
                  export_video,
                  quit,
                  show,
                  show_file,
                  show_files,
                  show_flow,
                  show_image,
                  show_layer,
                  show_points,
                  show_video,
)
from .ipc import start_monochrome as launch

__all__ = [
    "__version__",
    "show",
    "show_video",
    "show_image",
    "show_layer",
    "show_points",
    "show_file",
    "show_files",
    "show_flow",
    "launch",
    "export_video",
    "close_video",
    "quit",
    "BitRange",
    "ColorMap",
    "OpacityFunction",
]
