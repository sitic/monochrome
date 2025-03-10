# /// script
# dependencies = [
#   "monochrome==MONOCHROME_VERSION",
#   "pymatreader",
#   "scikit-image",
# ]
# ///
from pathlib import Path
import re

import monochrome as mc
import numpy as np
import skimage.io as sio

# filepath will be set by Monochrome when the script is run
filepath = Path(MC_FILEPATH)   # noqa: F821
print(f"Loading image folder from '{filepath}' ...")

def _natural_sort_path_key(path: Path, _nsre=re.compile("([0-9]+)")):
    return [
        int(text) if text.isdigit() else text.lower() for text in _nsre.split(path.name)
    ]

def load_image_folder(path):
    if not path.is_dir():
        msg = f"'{path}' is not a directory"
        raise ValueError(msg)

    files = []
    for extension in [".tif", ".tiff", ".TIF", ".TIFF", ".png", ".PNG"]:
        files.extend(path.glob(f"*{extension}"))
        if len(files) > 0:
            break
    if not files:
        msg = f"No .tif, .tiff or .png files found in folder '{path}'"
        raise ValueError(msg)

    files = sorted(files, key=_natural_sort_path_key)
    video = [sio.imread(file) for file in files]
    return np.array(video)

video = load_image_folder(filepath)
mc.show_video(video, name=filepath.name, metadata={
    "filepath": str(filepath),
    "shape": str(video.shape),
    "dtype": str(video.dtype),
})