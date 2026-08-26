import numpy as np
import matplotlib.pyplot as plt

def read_result_value(label, value_type):
    with open("result.dat") as f:
        for line in f:
            key, separator, value = line.partition(":")
            if separator and key.strip() == label:
                return value_type(value.strip())
    raise ValueError(f"Could not find '{label}' in result.dat")

gridsize = read_result_value("Size of grid", int)

inputfile = input("Name of file containing spin configuration: ")
data = np.loadtxt(inputfile, dtype = int)

zvals = np.zeros((gridsize,gridsize))
for i in data:
    y = i//gridsize
    x = i - y*gridsize
    zvals[x,y] = 1

plt.imshow(zvals, interpolation='None', cmap='Greens')
plt.show()
