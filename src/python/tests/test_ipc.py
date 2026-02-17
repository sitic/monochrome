import os
import random
import sys
import tempfile
from pathlib import Path
from time import sleep

import numpy as np
import monochrome as mc

# Run Monochrome in headless unit test mode
HEADLESS_TEST = "pytest" in sys.modules or os.getenv('HEADLESS')
if HEADLESS_TEST:
    mc.ipc.MONOCHROME_DEFAULT_ARGS = {'unit-test-mode': True}

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
        filename = Path(tmpdir) / "test.mp4"
        mc.export_video(filename, name='TestArray', fps=15, description="Test Description")

        if not HEADLESS_TEST:
            sleep(5)
            assert os.path.exists(filename)

def test_close_video():
    array = np.random.rand(*(1, 48, 48)).astype(dtype=np.float32)
    mc.show(array, 'CloseArray')
    if not HEADLESS_TEST:
        input("CloseArray loaded. Press ENTER to close it (default to last)...")
    mc.close_video()
    mc.show(array, 'CloseArray2')
    if not HEADLESS_TEST:
        input("CloseArray2 loaded. Press ENTER to close it by name...")
    mc.close_video('CloseArray2')

def test_speed():
    shape = (100, 128, 256)
    arr = np.random.rand(*shape).astype(dtype=np.float32)
    mc.show_video(arr, 'TestSpeed')
    
    if not HEADLESS_TEST:
        input("Video loaded. Press ENTER to set speed to 4.0...")
        mc.set_playback_speed(4.0)
        input("Speed set to 4.0. Press ENTER to set speed to 0.5...")
        mc.set_playback_speed(0.5)
        input("Speed set to 0.5. Press ENTER to set speed to 1.0...")
        mc.set_playback_speed(1.0)
        input("Speed set to 1.0. Press ENTER to continue...")
    else:
        mc.set_playback_speed(4.0)
        mc.set_playback_speed(0.5)
        mc.set_playback_speed(1.0)

def test_close_all():
    array = np.random.rand(*(1, 48, 48)).astype(dtype=np.float32)
    mc.show(array, 'Array1')
    mc.show(array, 'Array2')
    mc.show(array, 'Array3')
    
    if not HEADLESS_TEST:
        input("Three videos loaded. Press ENTER to close all...")
    
    mc.close_all_videos()
    
    if not HEADLESS_TEST:
        input("All videos should be closed. Press ENTER to continue...")

def test_playback():
    shape = (100, 64, 64)
    arr = np.random.rand(*shape).astype(dtype=np.float32)
    mc.show_video(arr, 'TestPlayback')
    
    if not HEADLESS_TEST:
        input("Video loaded. Press ENTER to pause...")
        mc.pause()
        input("Paused. Press ENTER to set frame to 50...")
        mc.set_frame(50)
        input("Frame set to 50. Press ENTER to set frame to 10...")
        mc.set_frame(10)
        input("Frame set to 10. Press ENTER to play...")
        mc.play()
        input("Playing. Press ENTER to continue...")
    else:
        mc.pause()
        mc.set_frame(50)
        mc.set_frame(10)
        mc.play()

def test_quit():
    mc.quit()
    if not HEADLESS_TEST:
        sleep(3)

if __name__ == "__main__":
    print("Testing filepaths...")
    test_filepaths()
    print("Testing array...")
    test_array()
    print("Testing overlay...")
    test_overlay()
    print("Testing flow...")
    test_flow()
    print("Testing points...")
    test_points()
    print("Testing export video...")
    test_export_video()
    print("Testing close all...")
    test_close_all()
    print("Testing close video...")
    test_close_video()
    print("Testing speed...")
    test_speed()
    print("Testing playback controls...")
    test_playback()
    print("Quitting...")
    if not HEADLESS_TEST:
        input("\n\n\nPress ENTER to quit Monochrome...\n")
    test_quit()
    print("Done.")
