# /// script
# dependencies = [
#   "monochrome==MONOCHROME_VERSION",
#   "pymatreader",
#   "scikit-image",
# ]
# ///
from pathlib import Path

import monochrome as mc
import skimage.io as sio

# filepath will be set by Monochrome when the script is run
filepath = Path(MC_FILEPATH)   # noqa: F821
print(f"Loading image from '{filepath}' ...")

image = sio.imread(filepath, as_gray=True)

mc.show_image(image, name=filepath.name, metadata={
    "filepath": str(filepath),
    "shape": str(image.shape),
    "dtype": str(image.dtype),
})