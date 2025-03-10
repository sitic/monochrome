# /// script
# dependencies = [
#   "monochrome==MONOCHROME_VERSION",
#   "tifffile[all]",
# ]
# ///
from pathlib import Path

import monochrome as mc
from tifffile import imread

# filepath will be set by Monochrome when the script is run
filepath = Path(MC_FILEPATH)   # noqa: F821
print(f"Loading tiff file '{filepath}' ...")

def sizeof_fmt(num, suffix="B"):
    for unit in ("", "Ki", "Mi", "Gi", "Ti", "Pi", "Ei", "Zi"):
        if abs(num) < 1024.0:
            return f"{num:3.1f}{unit}{suffix}"
        num /= 1024.0
    return f"{num:.1f}Yi{suffix}"

video = imread(filepath)
mc.show_video(video, name=filepath.name, metadata={
    "filepath": str(filepath),
    "shape": str(video.shape),
    "dtype": str(video.dtype),
    "size": sizeof_fmt(video.nbytes),
})