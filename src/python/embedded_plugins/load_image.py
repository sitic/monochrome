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
import numpy as np

# filepath will be set by Monochrome when the script is run
filepath = Path(sys.argv[1])   # DO NOT MODIFY THIS LINE
print(f"Loading image from '{filepath}' ...")

image = iio.imread(filepath)
metadata={
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
mc.show_image(image, name=filepath.name, metadata=metadata)
