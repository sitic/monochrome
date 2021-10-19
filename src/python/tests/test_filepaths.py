import tempfile
from pathlib import Path
from time import sleep

import numpy as np
import monochrome as mc

with tempfile.TemporaryDirectory() as tmpdir:
    shape = (100, 128, 256)
    array = np.random.rand(*shape).astype(dtype=np.float32)

    tmpfile = Path(tmpdir) / "test.npy"
    np.save(tmpfile, array)
    tmpfile = str(tmpfile)

    paths = [tmpfile]
    mc.show_file(paths[0])
    mc.show_files(paths)

    sleep(2)
