import numpy as np
import matplotlib.pyplot as plt

inputfile = input("Name of file containing spin configuration: ")
gridsize = int(input("Grid size: "))

data = np.genfromtxt(inputfile, dtype = int)
numberspins = len(data)

zvals = np.zeros((gridsize,gridsize))
for i in data:
    y = i//gridsize
    x = i - y*gridsize
    zvals[x,y] = 1

plt.imshow(zvals, interpolation='None', cmap='Greens')
plt.show()
