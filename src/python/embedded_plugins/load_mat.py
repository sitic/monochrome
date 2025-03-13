# /// script
# dependencies = [
#   "monochrome",  # DO NOT MODIFY THIS LINE
#   "pymatreader",
#   "numpy",
# ]
# ///
import sys
import warnings
from pathlib import Path

import monochrome as mc
import numpy as np
import pymatreader

# filepath will be set by Monochrome when the script is run
filepath = Path(sys.argv[1])   # DO NOT MODIFY THIS LINE
print(f"Loading MATLAB file from '{filepath}' ...")

def load_MATLAB(filename):
    data = pymatreader.read_mat(filename)

    fields = [key for key in data.keys() if not key.startswith("__")]
    npyfields = [key for key in fields if isinstance(data[key], np.ndarray)]
    npyfields = [key for key in npyfields if data[key].ndim >= 2 and data[key].ndim <= 3]
    npyfields = list(sorted(npyfields, key=lambda x: data[x].ndim, reverse=True))
    if len(npyfields) == 0:
        msg = f"No video files found in file '{filename}'. Fields: {fields}"
        raise ValueError(msg)
    elif len(npyfields) == 1:
        video = data[npyfields[0]]
        if video.ndim == 3 and video.shape[-1] > video.shape[0] and video.shape[-1] > video.shape[1]:
            # Assume the last dimension is the channel dimension
            video = np.moveaxis(video, -1, 0)
    else:
        if "cmosData" in fields and "bgimage" in fields:
            # cardiac Rhythm data format from Efimov lab
            video = -np.moveaxis(data["cmosData"], -1, 0)
        else:
            warnings.warn(f"Multiple fields found in file '{filename}': {fields}. Loading field '{npyfields[0]}'", UserWarning)
            video = data[npyfields[0]]
            if video.ndim == 3 and video.shape[-1] > video.shape[0] and video.shape[-1] > video.shape[1]:
                # Assume the last dimension is the channel dimension
                video = np.moveaxis(video, -1, 0)
    return video

video = load_MATLAB(filepath)
if (video.ndim not in [2, 3, 4]) or (video.ndim == 4 and video.shape[3] not in [3, 4]):
    msg = f"ERROR: Loaded image/video has unsupported shape {video.shape}."
    print(msg)
    sys.exit(1)
mc.show_video(video, name=filepath.name, metadata={
    "filepath": str(filepath),
    "shape": str(video.shape),
    "dtype": str(video.dtype),
})
