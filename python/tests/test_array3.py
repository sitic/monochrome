from quickVidViewer import open_array3, ColorMap, BitRange
import numpy as np

shape = (100, 128, 256)
arr = np.random.rand(*shape).astype(dtype=np.float32)

open_array3(arr, 'TestArray', 30, 500, "2020-04-29-13-10-27", "Test Comment", BitRange.PHASE, ColorMap.HSV)
