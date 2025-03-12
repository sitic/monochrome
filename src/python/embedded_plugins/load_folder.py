# /// script
# dependencies = [
#   "monochrome",  # DO NOT MODIFY THIS LINE
#   "scikit-image",
# ]
# ///
import re
import sys
from pathlib import Path

import monochrome as mc
import numpy as np
import skimage.io as sio

# filepath will be set by Monochrome when the script is run
filepath = Path(sys.argv[1])   # DO NOT MODIFY THIS LINE
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
    for extension in [".tif", ".tiff", ".TIF", ".TIFF", ".png", ".PNG", ".dcm", ".DCM"]:
        files.extend(path.glob(f"*{extension}"))
        if len(files) > 0:
            break
    if not files:
        msg = f"No .tif, .tiff or .png files found in folder '{path}'"
        raise ValueError(msg)

    files = sorted(files, key=_natural_sort_path_key)
    video = [sio.imread(file, as_gray=True) for file in files]
    return np.array(video)

video = load_image_folder(filepath)
if (video.ndim not in [2, 3]):
    msg = f"ERROR: Loaded video has unsupported shape {video.shape}. Monochrome only supports 2D or 3D arrays."
    print(msg)
    sys.exit(1)
mc.show_video(video, name=filepath.name, metadata={
    "filepath": str(filepath),
    "shape": str(video.shape),
    "dtype": str(video.dtype),
})
