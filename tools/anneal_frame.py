import numpy as np
import matplotlib.pyplot as plt 

data = np.loadtxt("config.dat", dtype = float)

with open("result.dat") as f:
    lines = f.readlines()
    gridsize = int(lines[5].strip().split(" ")[-1])
    
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
