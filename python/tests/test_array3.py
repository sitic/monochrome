# %%
import numpy as np
from quickVidViewer import send_array3

shape = (1000, 128, 128)
arr = np.zeros(shape, dtype=np.float32)

name = 'TestArray'
send_array3(name, arr)
