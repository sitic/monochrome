# /// script
# dependencies = [
#   "monochrome",  # DO NOT MODIFY THIS LINE
#   "opencv-contrib-python",
# ]
# ///

import sys
import warnings
from pathlib import Path

# filepath will be set by Monochrome when the script is run
filepath = Path(sys.argv[1])   # DO NOT MODIFY THIS LINE

print("This is a test script for loading plugins.")
warnings.warn("This is a test warning.", UserWarning)
print("This is a test error message.", file=sys.stderr)

sys.exit(1)