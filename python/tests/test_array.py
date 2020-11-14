from quickVidViewer import show_array, show, ColorMap, BitRange
import numpy as np

shape = (100, 128, 256)
arr = np.random.rand(*shape).astype(dtype=np.float32)

show_array(arr, 'TestArray', 30, 500, "2020-04-29-13-10-27", "Test Comment", BitRange.PHASE, ColorMap.HSV)

show_array(arr[0], 'TestArray 2D')

arr = (np.random.rand(*shape) * 65535).astype(dtype=np.uint16)

show_array(arr, 'TestArray u16', 30, 500, "2020-04-29-13-10-27", "Test Comment", metaData={'Foo': 'Bar'})
show(arr, 'TestArray u16', 30, 500, "2020-04-29-13-10-27", "Test Comment", metaData={'Foo': 'Bar'})