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

data = np.atleast_2d(np.loadtxt("config.dat", dtype=float))

print("Entering -1 will end the loop")

while(True):

    rownum = int(input("Number of config to plot: "))
    if rownum == -1:
        break

    fig = plt.figure()

    row = data[rownum]
    zvals = np.zeros((gridsize,gridsize))
    for j in row[:-1]:    # last element is energy
        y = int(j)//gridsize
        x = int(j) - y*gridsize
        zvals[x,y] = 1
    
    plt.imshow(zvals, interpolation='None', cmap='Greens')

    plt.show()
