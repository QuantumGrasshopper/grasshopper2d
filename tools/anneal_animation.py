import numpy as np
import matplotlib.pyplot as plt 
import matplotlib.animation as anim

def read_result_value(label, value_type):
    with open("result.dat") as f:
        for line in f:
            key, separator, value = line.partition(":")
            if separator and key.strip() == label:
                return value_type(value.strip())
    raise ValueError(f"Could not find '{label}' in result.dat")

gridsize = read_result_value("Size of grid", int)

data = np.atleast_2d(np.loadtxt("config.dat", dtype=float))

fig = plt.figure()

def animation_function(i):
    row = data[i]
    zvals = np.zeros((gridsize,gridsize))
    for j in row[:-1]:    # last element is energy
        y = int(j)//gridsize
        x = int(j) - y*gridsize
        zvals[x,y] = 1
    plt.imshow(zvals, interpolation='None', cmap='Greens')
  
animation = anim.FuncAnimation(fig, animation_function, frames=len(data), interval=2)

plt.show()
