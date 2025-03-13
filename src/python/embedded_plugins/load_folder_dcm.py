# /// script
# dependencies = [
#   "monochrome",  # DO NOT MODIFY THIS LINE
#   "imageio",
#   "SimpleITK",
# ]
# ///
import re
import sys
from pathlib import Path

import monochrome as mc
import numpy as np
import imageio.v3 as iio

# filepath will be set by Monochrome when the script is run
filepath = Path(sys.argv[1])   # DO NOT MODIFY THIS LINE
print(f"Loading image folder '{filepath}' ...")

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
    video = [iio.imread(file, plugin="ITK") for file in files]
    return np.squeeze(np.array(video))

video = load_image_folder(filepath)
if (video.ndim not in [2, 3, 4]) or (video.ndim == 4 and video.shape[3] not in [3, 4]):
    msg = f"ERROR: Loaded image/video has unsupported shape {video.shape}."
    print(msg)
    sys.exit(1)
metadata = {
    "filepath": str(filepath),
    "shape": str(video.shape),
    "dtype": str(video.dtype),
}
mc.show_video(video, name=filepath.name, metadata=metadata)
