#%%
import matplotlib.pyplot as plt
import numpy as np

# Get the turbo colormap
cmap_name = 'turbo'
cmap = plt.get_cmap(cmap_name)
original_colors = cmap(np.linspace(0, 1, 256))
print("const std::array<float, 3 * 256> turbo_colormapdata = {")
for i in range(256):
    r = original_colors[i][0]
    g = original_colors[i][1]
    b = original_colors[i][2]
    print(f"    {r}, {g}, {b},")  # Output in C++ array format
print("};")
#%%

# Get the tab10 colormap
cmap_name = 'tab10'
cmap = plt.get_cmap(cmap_name)
original_colors = cmap(np.linspace(0, 1, 10))
print("const std::array<std::array<float, 3>, 10> tab10_colors = {")
for i in range(10):
    r = original_colors[i][0]
    g = original_colors[i][1]
    b = original_colors[i][2]
    print(f"    {{ {r}, {g}, {b} }},")  # Output in C++ array format
print("};")