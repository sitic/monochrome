import numpy as np
from quickVidViewer import open_array3, open_flow

vid = np.zeros((100, 128, 128), dtype=np.float32)
vid[:, ::2, ::2] = 1

flow = np.zeros((100, 128, 128, 2), dtype=np.float32)
for t in range(100):
    flow[t, :, :, 0] = t / 100
    flow[t, :, :, 1] = t / 100

open_array3(vid)
open_flow(flow, "")