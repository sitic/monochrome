# %%
import numpy as np
from quickVidViewer import send_array3

shape = (100, 256, 256)
arr = np.random.rand(*shape).astype(dtype=np.float32)

name = 'TestArray'
send_array3(name, arr)
