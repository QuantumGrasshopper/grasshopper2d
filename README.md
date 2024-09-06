# Simulation of the Grasshopper Problem in 2d Euclidean space

Currently implemented for a square grid and with simulated annealing

## 1. Prerequisites and compilation

- make
- GSL library
- g++ compiler with c++17 (or another C++ compiler, which requires adjusting the Makefile)

To compile the code, type `make` in the shell while in the folder containing the code.

## 2. Code options (the order doesn't matter)

| option          | comment |
| --------------- | ------------------ |
| `-d`            | grasshopper jump distance in regular length units (unit in which the lawn area is 1), e.g. 0.1 or 0.5 or 2 or 10 |
| `-N`            | total number of grid points with spin 1 (10000 is default) |
| `-gridsize`     | for square grid length of square edge (number of cells) |
| `-hours`        | how many hours the code should run (can be less than 1 hour), e.g. 0.1, 0.5, 2, 48, can also be 0 (default value) if you only want to look at the initial configuration |
| `-steps`        | how many steps the code should maximally run (`1e12` by default), but code will terminate earlier if maximal time is reached |
| `-tempsteps`    | initial number of steps before first temperature decrease (the number of steps between decreases goes up with each round) |
| `-inittemp`     | initial temperature |
| `-fintemp`      | final temperature (need to run long enough to reach it) |
| `-annealsteps`  | number of simulated annealing steps between initial and final temperature |
| `-configoutput` | maximal number of configurations in `config.dat`, see below; default is `0` (no output) |
| `-initconf`     | how to initialise the system: currently implemented: `random` (default), `disk`, or `load` (load configuration from file called `initconf.dat`) |
| `-delta`        | choice of delta-function discretization (two options implemented, see code)
| `-NNint`        | nearest neighbor interaction coefficient (0 by default) |
| `-randomseed`   | initial value for the random number generator (if none specified will generate a random seed based on the system clock) |


Required options: `d`
Recommended options: `N`, `gridsize`, `hours`

You can also look into the source code to remind yourself of what the options do.

The code will generate the following output files:

file name          | comment
---------------    | ---------------
`result.dat`       | general info about the simulation and parameters
`initconf.dat`     | initial spin configuration
`finconf.dat`      | final spin configuration
`bestconf.dat`     | best spin configuration over the whole run
`energies.dat`     | every annealing round the energy (grasshopper probability) value is written to this file
`temperatures.dat` | every annealing round prints the counter, the current temperature, and the current acceptance ratio
`config.dat`       | stores the spin configuration and energy every certain number of steps, can be used to generate animations of the system evolution (no longer output by default)

Example of command to run the code:

> `./grasshopper -N 10000 -initconf random -gridsize 200 -d 0.3 -hours 0.2 -inittemp 20.0 -fintemp 0.05 `

## 3. Plotting spin configurations

Python scripts for plotting spin configurations and other tools can be found in the subfolder `tools`.

- `config_plot.py` plots a grasshopper spin configuration in 2d, such as `initconf.dat` or `bestconf.dat` etc. Input is the name of the configuration file; everthing else is read automatically from `result.dat`, which must be present.
- `anneal_animation.py` generates an animation of the simulated annealing process from the file `config.dat`, which is output by the main code. The size of the grid is read automatically from `result.dat`, which must be present.
- `anneal_frame.py` plots individual frames from `config.dat` rather than creating the full animation.
- `extract_boundary.py` extracts the boundary of a cogwheel shape (read from `finconf.dat`) and returns it in the polar coordinate representation (rho vs. phi). The files `finconf.dat` and `result.dat` must be present.
- `finite_temp_analysis.py` contains several routines for analyzing output from simulations that ran at constant temperature. It automatically generates several figures. The root directory must be manually adjusted.
