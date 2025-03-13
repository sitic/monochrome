#%%
import imageio.v3 as iio
import matplotlib.pyplot as plt
import numpy as np

img = iio.imread("imageio:chelsea.png")
img = img[..., :3]

plt.imshow(img)
plt.show()
#%%

import monochrome as mc
mc.show(img)

# %%
