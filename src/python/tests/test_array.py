import numpy as np
import monochrome as mc

shape = (100, 128, 256)
arr = np.random.rand(*shape).astype(dtype=np.float32)

mc.show_array(arr, 'TestArray', cmap='hsv', bitrange='float', duration_seconds=30, fps=500,
              date="2020-04-29-13-10-27", comment="Test Comment")

mc.show_array(arr[0], 'TestArray 2D')

arr = (np.random.rand(*shape) * 65535).astype(dtype=np.uint16)

mc.show(arr, 'TestArray u16', metadata={'Foo': 'Bar'})
