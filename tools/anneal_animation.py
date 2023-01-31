import numpy as np
import matplotlib.pyplot as plt 
import matplotlib.animation as anim

data = np.genfromtxt("config.dat", dtype = int)
gridsize = int(input("Grid size: "))

fig = plt.figure()

def animation_function(i):
    row = data[i]
    zvals = np.zeros((gridsize,gridsize))
    for j in row[:-1]:    # last element is energy
        y = j//gridsize
        x = j - y*gridsize
        zvals[x,y] = 1
    plt.imshow(zvals, interpolation='None', cmap='Greens')
  
animation = anim.FuncAnimation(fig, animation_function, interval = 2)

plt.show()
