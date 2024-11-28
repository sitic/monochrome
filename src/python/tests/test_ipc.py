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

# Do we want to mock the server?
HEADLESS_TEST = "pytest" in sys.modules or os.getenv('HEADLESS')

if HEADLESS_TEST:
    patcher.start()
    mc.ipc.ABSTRACT_DOMAIN_SOCKET_SUPPORTED = True  # Fix for MacOS

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
    mc.show_video(arr, 'TestArray', cmap='hsv', bitrange='float', comment="Test Comment")

    mc.show_video(arr[0], 'TestArray Image')

    arr = (np.random.rand(*shape) * 65535).astype(dtype=np.uint16)
    mc.show(arr, 'TestArray u16', metadata={'Foo': 'Bar'})
    mc.show_video((arr[0]*255).astype(np.uint8), 'TestArray Image uint8')


def test_overlay():
    shape = (100, 128, 256)
    arr = np.random.rand(*shape).astype(dtype=np.float32)
    mc.show_video(arr, 'TestArray', bitrange='float')
    
    overlay = arr.copy()
    overlay[32:96, 32:96] = np.nan

    mc.show_layer(overlay, parent="", name="Overlay Name", cmap='hsv')


def test_flow():
    vid = np.zeros((100, 128, 128), dtype=np.float32)
    vid[:, :64:2, :64:2] = 1

    flow = np.zeros((100, 128, 128, 2), dtype=np.float32)
    for t in range(100):
        flow[t, :64, :64, 0] = t / 100
        flow[t, :64, :64, 1] = t / 100

    mc.show_video(vid, "flows")
    mc.show_flow(flow, name="", color='blue')
    mc.show_flow(flow, name="2", color='yellow')


def test_points():
    vid = np.zeros((100, 128, 128), dtype=np.float32)
    def random_points(shape):
        points = []
        for _ in range(shape[0]):
            p = []
            for _ in range(random.randint(0, 5)):
                p.append([random.randint(0, shape[0]), random.randint(0, shape[1])])
            points.append(p)
        return points
    mc.show_video(vid, "points")
    points = random_points(vid.shape)
    mc.show_points(points, name="Foo", color='red', point_size=4)
    points = random_points(vid.shape)
    mc.show_points(points, name="Test", color='yellow', point_size=10)

def test_export_video():
    with tempfile.TemporaryDirectory() as tmpdir:
        shape = (30, 48, 48)
        array = np.random.rand(*shape).astype(dtype=np.float32)
        mc.show(array, 'TestArray')
        mc.export_video(Path(tmpdir) / "test.mp4", name='TestArray', fps=15, description="Test Description")

        if not HEADLESS_TEST:
            sleep(5)
            assert os.path.exists(tmpdir + "/test.mp4")

def test_close_video():
    array = np.random.rand(*(1, 48, 48)).astype(dtype=np.float32)
    mc.show(array, 'CloseArray')
    mc.close_video()
    mc.show(array, 'CloseArray2')
    mc.close_video('CloseArray2')

def test_quit():
    mc.quit()
    if not HEADLESS_TEST:
        sleep(3)

if __name__ == "__main__":
    test_filepaths()
    test_array()
    test_overlay()
    test_flow()
    test_points()
    test_export_video()
    test_close_video()
    test_quit()
