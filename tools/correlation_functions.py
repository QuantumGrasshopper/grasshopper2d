# given a configuration computes the nearest neighbor correlation function
# general correlation function (distance input) is via correlation_functions.cpp
# creates spatial representation of correlations and computes averages

import numpy as np
import matplotlib.pyplot as plt

# data input -----------------------------------------------------------------

inputfile = input("Name of file containing spin configuration: ")

finconf = np.loadtxt(inputfile, dtype = int)
numberspins = len(finconf)
cellwidth = 1 / np.sqrt(numberspins)

with open("result.dat") as f:
    lines = f.readlines()
    jump = float(lines[3].strip().split(" ")[-1])
    gridsize = int(lines[5].strip().split(" ")[-1])
    delta = int(lines[8].strip().split(" ")[-1])

unraveled = np.unravel_index(finconf, (gridsize, gridsize))
grid = np.zeros((gridsize, gridsize), dtype=int)
grid[unraveled] = 1

# plot configuration ---------------------------------------------------------

#plt.imshow(grid, interpolation='None', cmap='Greens')
#plt.show()

# energy contribution function -----------------------------------------------

def contributionEnergy(have, comparewith):

    contribution=0
    absdist = abs(have-comparewith)/cellwidth
    
    if absdist <= 2:
        if delta==1:
            if absdist<1:
                contribution = 17/48 + np.sqrt(3)*np.pi/108 + absdist/4 - absdist*absdist/4 + (1-2*absdist)*np.sqrt(1+12*absdist*(1-absdist))/16 - np.sqrt(3)*np.arcsin(np.sqrt(3)*(2*absdist-1)/2)/12
            else:
                contribution = 55/48 - np.sqrt(3)*np.pi/108 - 13*absdist/12+absdist*absdist/4 + (2*absdist-3)*np.sqrt(36*absdist-23-12*absdist*absdist)/48 + np.sqrt(3)*np.arcsin(np.sqrt(3)*(2*absdist-3)/2)/36
        else:
            contribution=(1 + np.cos(np.pi*absdist/2))/4
    
    return contribution

# compute correlation function -----------------------------------------------


corr_grid = np.zeros((gridsize, gridsize), dtype=float)

# Using NumPy roll for neighbor computation in 2D
corr_grid += np.roll(grid, shift=1, axis=0)  # down
corr_grid += np.roll(grid, shift=-1, axis=0)  # up
corr_grid += np.roll(grid, shift=1, axis=1)  # left
corr_grid += np.roll(grid, shift=-1, axis=1)  # right

# doesn't really make sense to save grid data here because only the boundary is different
# so essentially we get the same as the configuration plot, with a slightly fuzzier boundary
np.savetxt(f"d{jump:.2f}_nn_correlation.txt", grid * corr_grid)

# this will be close to 4 at low temperature
# only boundary points have fewer than 4 neighbors
print(np.sum(grid * corr_grid)/numberspins)

# plot correlation grid ------------------------------------------------------

plt.imshow(corr_grid, interpolation='None', cmap='Greens')
plt.show()

plt.imshow(grid * corr_grid/numberspins, interpolation='None', cmap='Greens')
plt.show()













