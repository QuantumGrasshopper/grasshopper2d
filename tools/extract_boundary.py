# extract boundary of a low-temperature cogwheel configuration
# based on Nguyen Nguyen's code

import numpy as np
import matplotlib.pyplot as plt

finconf = np.loadtxt("finconf.dat", dtype = int)
numberspins = len(finconf)
cellwidth = numberspins**(-1/2)

with open("result.dat") as f:
    lines = f.readlines()
    jump = float(lines[3].strip().split(" ")[-1])
    gridsize = int(lines[5].strip().split(" ")[-1])

unraveled = np.unravel_index(finconf, (gridsize, gridsize))
grid = np.zeros((gridsize, gridsize), dtype=int)
grid[unraveled] = 1

lawn_points = []
outline_lawn_points = []

for index,lawn_value in np.ndenumerate(grid):
    if lawn_value == 1:
        index = np.array(index)
        lawn_points.append(index*cellwidth)
        
        if index[0] == gridsize-1 or index[1] == gridsize-1:
            outline_lawn_points.append(index*cellwidth)
        elif index[0] == 0 or index[1] == 0:
            outline_lawn_points.append(index*cellwidth)
        elif grid[index[0]-1,index[1]] == 0:
            outline_lawn_points.append(index*cellwidth)
        elif grid[index[0]+1,index[1]] == 0:
            outline_lawn_points.append(index*cellwidth)
        elif grid[index[0],index[1]-1] == 0:
            outline_lawn_points.append(index*cellwidth)
        elif grid[index[0],index[1]+1] == 0:
            outline_lawn_points.append(index*cellwidth)
            
lawn_points = np.array([x for x in lawn_points],dtype=float)
outline_lawn_points = np.array([x for x in outline_lawn_points],dtype=float)

# calculating the center of the cogwheels and shift the plot so that the center is at (0,0)

center = np.array([np.mean(lawn_points[:,0]),np.mean(lawn_points[:,1])])
lawn_points -= center
outline_lawn_points -= center

# plot the boundary to check if it is reasonable

plt.plot(outline_lawn_points[:,0],outline_lawn_points[:,1],'bo',markersize=2)
plt.gca().set_aspect('equal')
plt.show()

# convert boundary to polar coordinates

polar_coordinates = []

for i in range(len(outline_lawn_points)):
    x = outline_lawn_points[i,0]
    y = outline_lawn_points[i,1]
    polar_coordinates.append([np.arctan2(y,x),np.sqrt(x*x+y*y)])

polar_coordinates = np.array([x for x in polar_coordinates])
polar_coordinates = polar_coordinates[polar_coordinates[:, 0].argsort()]

# plot the boundary in polar coordinates

plt.plot(polar_coordinates[:,0],polar_coordinates[:,1],'bo',markersize=2)
plt.xlabel('phi')
plt.ylabel('r')

plt.show()

# save polar coordinates to file

np.savetxt(f"d{jump:.2f}_boundary.txt", polar_coordinates)


