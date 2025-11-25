import numpy as np
from PIL import Image   # pip install pillow

# 1) load data
vals = np.loadtxt("lena.dat", dtype=np.uint8)
N = vals.size
side = int(np.sqrt(N))
vals = vals.reshape((side, side))

# 2) make image and save
img = Image.fromarray(vals, mode="L")   # "L" = grayscale
img.save("lena.png")
print("Saved lena.png", img.size)
img.show()