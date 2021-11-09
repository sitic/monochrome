from unittest import mock

patcher = mock.patch('socket.socket')

import os
import random
import sys
import tempfile
from pathlib import Path
from time import sleep

import numpy as np

import monochrome as mc

# Do want to mock the server? Check if we are in a CI server or if env 'HEADLESS' is set
HEADLESS_TEST = ("pytest" in sys.modules or os.getenv('HEADLESS')
                 or (os.name == 'posix' and "DISPLAY" not in os.environ))

if HEADLESS_TEST:
    patcher.start()


def test_filepaths():
    with tempfile.TemporaryDirectory() as tmpdir:
        shape = (100, 128, 256)
        array = np.random.rand(*shape).astype(dtype=np.float32)

        tmpfile = Path(tmpdir) / "test.npy"
        np.save(tmpfile, array)
        tmpfile = str(tmpfile)

        paths = [tmpfile]
        mc.show_file(paths[0])
        mc.show_files(paths)

        if not HEADLESS_TEST:
            sleep(2)


def test_array():
    shape = (100, 128, 256)
    arr = np.random.rand(*shape).astype(dtype=np.float32)
    mc.show_array(arr, 'TestArray', cmap='hsv', bitrange='float', duration_seconds=30, fps=500,
                  date="2020-04-29-13-10-27", comment="Test Comment")

    mc.show_array(arr[0], 'TestArray Image')

    arr = (np.random.rand(*shape) * 65535).astype(dtype=np.uint16)
    mc.show(arr, 'TestArray u16', metadata={'Foo': 'Bar'})
    mc.show_array((arr[0]*255).astype(np.uint8), 'TestArray Image uint8')


def test_flow():
    vid = np.zeros((100, 128, 128), dtype=np.float32)
    vid[:, :64:2, :64:2] = 1

    flow = np.zeros((100, 128, 128, 2), dtype=np.float32)
    for t in range(100):
        flow[t, :64, :64, 0] = t / 100
        flow[t, :64, :64, 1] = t / 100

    mc.show_array(vid, "flows")
    mc.show_flow(flow, "", color='blue')


def test_points():
    vid = np.zeros((100, 128, 128), dtype=np.float32)
    points = []
    for _ in range(vid.shape[0]):
        p = []
        for _ in range(random.randint(0, 5)):
            p.append([random.randint(0, 128), random.randint(0, 128)])
        points.append(p)
    mc.show_array(vid, "points")
    mc.show_points(points, color='red', point_size=10)


if __name__ == "__main__":
    test_filepaths()
    test_array()
    test_flow()
    test_points()
