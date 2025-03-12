# /// script
# dependencies = [
#   "monochrome",  # DO NOT MODIFY THIS LINE
#   "imageio",
#   "SimpleITK",
# ]
# ///
import sys
from pathlib import Path

import monochrome as mc
import numpy as np
import imageio.v3 as iio

# filepath will be set by Monochrome when the script is run
filepath = Path(sys.argv[1])   # DO NOT MODIFY THIS LINE
print(f"Loading image from '{filepath}' ...")

image = iio.imread(filepath, plugin="ITK")

metadata = {
    "filepath": str(filepath),
    "shape": str(image.shape),
    "dtype": str(image.dtype),
}
if image.ndim > 2:
    if image.shape[-1] == 4:
        # RGBA image, remove alpha channel
        image = image[..., :3]
    if image.shape[-1] == 3:
        # RGB image, convert to grayscale
        convert = np.array([0.2989, 0.5870, 0.1140], dtype=np.float32)
        image = np.dot(image, convert)
if (image.ndim not in [2, 3]):
    msg = f"ERROR: Loaded image/video has unsupported shape {image.shape}. Monochrome only supports 2D or 3D arrays."
    print(msg)
    sys.exit(1)
mc.show(image, name=filepath.name, metadata=metadata)
