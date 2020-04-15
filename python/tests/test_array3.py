from quickVidViewer import open_array3
import numpy as np

shape = (100, 256, 256)
arr = np.random.rand(*shape).astype(dtype=np.float32)

open_array3(arr, 'TestArray')
