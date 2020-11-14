from quickVidViewer import open_file, open_files, open
import tempfile
import numpy as np
from pathlib import Path
from time import sleep

with tempfile.TemporaryDirectory() as tmpdir:
    shape = (100, 128, 256)
    array = np.random.rand(*shape).astype(dtype=np.float32)

    tmpfile = Path(tmpdir) / "test.npy"
    np.save(tmpfile, array)
    tmpfile = str(tmpfile)

    paths = [tmpfile]
    open_file(paths[0])
    open_files(paths)

    sleep(2)
