import numpy as np
import monochrome as mc

vid = np.zeros((100, 128, 128), dtype=np.float32)
vid[:, :64:2, :64:2] = 1

flow = np.zeros((100, 128, 128, 2), dtype=np.float32)
for t in range(100):
    flow[t, :64, :64, 0] = t / 100
    flow[t, :64, :64, 1] = t / 100

mc.show_array(vid)
mc.show_flow(flow, "", color='blue')