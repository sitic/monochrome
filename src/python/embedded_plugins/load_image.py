# /// script
# dependencies = [
#   "monochrome",  # DO NOT MODIFY THIS LINE
#   "imageio",
#   "numpy",
# ]
# ///
import sys
from pathlib import Path

import imageio.v3 as iio
import monochrome as mc

# filepath will be set by Monochrome when the script is run
filepath = Path(sys.argv[1])   # DO NOT MODIFY THIS LINE
print(f"Loading image from '{filepath}' ...")

image = iio.imread(filepath)
metadata={
    "filepath": str(filepath),
    "shape": str(image.shape),
    "dtype": str(image.dtype),
}
if (image.ndim not in [2, 3, 4]) or (image.ndim == 4 and image.shape[3] not in [3, 4]):
    msg = f"ERROR: Loaded image/video has unsupported shape {image.shape}."
    print(msg)
    sys.exit(1)
mc.show_image(image, name=filepath.name, metadata=metadata)
