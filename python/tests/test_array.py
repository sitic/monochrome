from quickVidViewer import open_array, ColorMap, BitRange
import numpy as np

shape = (100, 128, 256)
arr = np.random.rand(*shape).astype(dtype=np.float32)

open_array(arr, 'TestArray', 30, 500, "2020-04-29-13-10-27", "Test Comment", BitRange.PHASE, ColorMap.HSV)

open_array(arr[0], 'TestArray 2D')

arr = (np.random.rand(*shape) * 65535).astype(dtype=np.uint16)

open_array(arr, 'TestArray u16', 30, 500, "2020-04-29-13-10-27", "Test Comment")
