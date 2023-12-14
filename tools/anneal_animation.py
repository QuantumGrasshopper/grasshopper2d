import numpy as np
import matplotlib.pyplot as plt 
import matplotlib.animation as anim

data = np.loadtxt("config.dat", dtype = float)

with open("result.dat") as f:
    lines = f.readlines()
    gridsize = int(lines[5].strip().split(" ")[-1])

fig = plt.figure()

def animation_function(i):
    row = data[i]
    zvals = np.zeros((gridsize,gridsize))
    for j in row[:-1]:    # last element is energy
        y = int(j)//gridsize
        x = int(j) - y*gridsize
        zvals[x,y] = 1
    plt.imshow(zvals, interpolation='None', cmap='Greens')
  
animation = anim.FuncAnimation(fig, animation_function, interval = 2)

plt.show()
