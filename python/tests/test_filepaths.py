import tempfile
from pathlib import Path
from time import sleep

import numpy as np
from monochrome import show, show_file, show_files

with tempfile.TemporaryDirectory() as tmpdir:
    shape = (100, 128, 256)
    array = np.random.rand(*shape).astype(dtype=np.float32)

    tmpfile = Path(tmpdir) / "test.npy"
    np.save(tmpfile, array)
    tmpfile = str(tmpfile)

    paths = [tmpfile]
    show_file(paths[0])
    show_files(paths)

    sleep(2)
