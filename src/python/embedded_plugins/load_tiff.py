# /// script
# dependencies = [
#   "monochrome",  # DO NOT MODIFY THIS LINE
#   "tifffile[all]",
# ]
# ///
import sys
from pathlib import Path

import monochrome as mc
from tifffile import imread

# filepath will be set by Monochrome when the script is run
filepath = Path(sys.argv[1])  # DO NOT MODIFY THIS LINE
print(f"Loading tiff file '{filepath}' ...")

def sizeof_fmt(num, suffix="B"):
    for unit in ("", "Ki", "Mi", "Gi", "Ti", "Pi", "Ei", "Zi"):
        if abs(num) < 1024.0:
            return f"{num:3.1f}{unit}{suffix}"
        num /= 1024.0
    return f"{num:.1f}Yi{suffix}"

video = imread(filepath)
metadata = {
    "filepath": str(filepath),
    "shape": str(video.shape),
    "dtype": str(video.dtype),
    "size": sizeof_fmt(video.nbytes),
}
if (video.ndim not in [2, 3, 4]) or (video.ndim == 4 and video.shape[3] not in [3, 4]):
    msg = f"ERROR: Loaded image/video has unsupported shape {video.shape}."
    print(msg)
    sys.exit(1)
mc.show_video(video, name=filepath.name, metadata=metadata)
