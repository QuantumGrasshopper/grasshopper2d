import numpy as np
import matplotlib.pyplot as plt 

# this script creates an average representative configuration
# for code that was run at fixed temperature

data = np.loadtxt("config.dat", dtype = float)

filesize = len(data)

with open("result.dat") as f:
    lines = f.readlines()
    gridsize = int(lines[5].strip().split(" ")[-1])
    
# first we determine when the system has thermalized
# criterion: look at last 10% of the data and determine mean & std
# find cutoff time when energy is within 1std of mean

energies = []

for i in range(filesize):
    row = data[i]
    energies.append(row[-1])
    
cutoff = filesize//10
    
energymean = np.mean(energies[-cutoff:])
energystd = np.std(energies[-cutoff:])
print(energymean, energystd)

for i in range(filesize):
    if abs(energies[i]-energymean)<energystd:
        cutoff=i
        break

print(cutoff)    
config_average = np.zeros((gridsize,gridsize))

for i in range(cutoff,filesize):
    row = data[i]
    for j in row[:-1]:    # last element is energy
        y = int(j)//gridsize
        x = int(j) - y*gridsize
        config_average[x,y] += 1
  
plt.imshow(config_average/(filesize-cutoff), interpolation='None', cmap='Greens')
plt.show()
