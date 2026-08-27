# Grasshopper2D

Grasshopper2D is a C++ implementation of simulated annealing for the planar Euclidean grasshopper problem on a square grid. The lawn has unit area and is represented by `N` occupied grid cells. The code searches for configurations that maximize the discretized grasshopper success probability for a fixed jump distance `d`, with an optional nearest-neighbor interaction.

## Citation

If you use Grasshopper2D in published work, please cite the software release (see [`CITATION.cff`](CITATION.cff)) and the paper introducing the grasshopper problem and numerical method:

O. Goulko and A. Kent, *The grasshopper problem*, 
Proc. R. Soc. A **473**, 20170494 (2017),
https://doi.org/10.1098/rspa.2017.0494.

## Funding

This work was supported by the National Science Foundation under Grant Nos. PHY-2112738 ("CQIS: The Grasshopper Problem") and OSI-2328774 ("ExpandQISE: Track 2: EQUIP-UMB").

## Related publications

- D. Llamas, J. Kent-Dobias, K. Chen, A. Kent, and O. Goulko,
  *Origin of Symmetry Breaking in the Grasshopper Model*,
  Phys. Rev. Research **6**, 023235 (2024),
  https://doi.org/10.1103/PhysRevResearch.6.023235.
  
## Related software

- A broader Python implementation supporting both spherical and Euclidean grasshopper models is available at https://github.com/llamas7/grasshopper.

## Prerequisites and compilation

- make
- GSL library
- C++17 compiler (GCC and Clang tested)

To compile the code, type `make` in the shell while in the folder containing the code. This uses the default GCC compiler. To use a different compiler, for example Clang, type `make CXX=clang++`, etc.

Optional performance benchmarks and their build instructions are documented in [`benchmarks/README.md`](benchmarks/README.md).

## Testing

Run the unit tests with

> `make test`

run the integration tests with

> `make integration-test`

or run the complete test suite with

> `make check`

## Running the code

The code uses the following command-line options:

| option          | comment |
| --------------- | ------------------ |
| `-d`            | required positive grasshopper jump distance in regular length units (unit in which the lawn area is 1), e.g. 0.1 or 0.5 or 2 or 10 |
| `-N`            | total number of grid points with spin 1 (10000 is default) |
| `-gridsize`     | square-grid edge length in cells (automatically sized if omitted); must satisfy `floor((gridsize - 1)/2) >= ceil(d/cellSize) + 1`, where `cellSize=1/sqrt(N)`; undersized grids are rejected |
| `-hours`        | how many hours the code should run (can be less than 1 hour), e.g. 0.1, 0.5, 2, 48, can also be 0 (default value) if you only want to look at the initial configuration |
| `-steps`        | how many steps the code should maximally run (`1e12` by default), but code will terminate earlier if maximal time is reached |
| `-tempsteps`    | initial number of steps before first temperature decrease (defaults to `N`; the number of steps between decreases goes up with each round) |
| `-inittemp`     | initial temperature (20 by default) |
| `-fintemp`      | final temperature (0.01 by default; need to run long enough to reach it) |
| `-annealsteps`  | number of simulated annealing steps between initial and final temperature (1000 by default) |
| `-configoutput` | strict maximum number of configurations in `config.dat`, see below; default is `0` (no output) |
| `-initconf`     | how to initialise the system: currently implemented: `random` (default), `disk`, or `load` (load configuration from file called `initconf.dat`) |
| `-delta`        | choice of delta-function discretization: exactly `0` (default) or `1`; see Goulko and Kent (2017) for their definitions |
| `-NNint`        | nearest neighbor interaction coefficient (0 by default); positive values favor occupied nearest-neighbor bonds |
| `-randomseed`   | unsigned initial value for the random number generator (if omitted or set to `0`, a seed is generated from the system clock) |
| `-overwrite`    | output overwrite policy: exactly `0` (default, reject if an output artifact exists) or `1` (remove old output artifacts before starting) |

- Required options: `d`
- Recommended options: `N`, `gridsize`, `hours`

Every option accepts exactly one value and may be supplied at most once. Unknown options, missing values, malformed or out-of-range numbers, and non-finite floating-point values are rejected. The number of spins must satisfy `0 < N < gridsize^2` after automatic or explicit grid sizing.

The code uses the following standard output files:

file name          | comment
---------------    | ---------------
`result.dat`       | general info about the simulation and parameters
`initconf.dat`     | initial spin configuration; with `-initconf load`, this is an input and is never removed or overwritten
`finconf.dat`      | final spin configuration
`bestconf.dat`     | best spin configuration over the whole run
`energies.dat`     | every annealing round the energy (normalized MC objective, which equals the grasshopper probability if NNint=0) value is written to this file
`temperatures.dat` | every annealing round prints the counter, the current temperature, and the current acceptance ratio
`config.dat`       | stores selected spin configurations and energies across the annealing trajectory for animation or analysis (not output by default)

For `-configoutput`, `0` disables `config.dat` and `1` stores only the actual final configuration. Values of `2` or more reserve rows for the initial and actual final configurations, with up to the remaining maximum distributed approximately uniformly over cooling stages (and therefore log temperature). Stages are not duplicated when more rows are requested than the annealing schedule provides. If a run terminates early, its actual final configuration is still the last row.

By default, a run is rejected before creating files if any standard output file listed above already exists. With `-overwrite 1`, the complete standard output set is removed before the run so that stale files, including `config.dat`, do not survive when the corresponding output is disabled. The only exception is `initconf.dat` with `-initconf load`, which is preserved as the run's input. A cleanup error aborts the run before simulation output begins.

Example of command to run the code:

> `./grasshopper -N 10000 -initconf random -gridsize 200 -d 0.3 -hours 0.2 -inittemp 20.0 -fintemp 0.05 `

## Plotting tools

Python scripts for plotting spin configurations and other tools can be found in the subfolder `tools`.

- `config_plot.py` plots a grasshopper spin configuration in 2D, such as `initconf.dat` or `bestconf.dat` etc. Input is the name of the configuration file; everything else is read automatically from `result.dat`, which must be present.
- `anneal_animation.py` generates an animation of the simulated annealing process from the file `config.dat`, which is output by the main code. The size of the grid is read automatically from `result.dat`, which must be present.
- `anneal_frame.py` plots individual frames from `config.dat` rather than creating the full animation.
- `extract_boundary.py` extracts the boundary of a cogwheel shape (read from `finconf.dat`) and returns it in the polar coordinate representation (rho vs. phi). The files `finconf.dat` and `result.dat` must be present.

## Spatial correlation analysis

Build the C++ correlation tool explicitly with:

> `make correlation-tool`

This target is separate from the normal simulation build and from `make test`.
Analyze a saved configuration with:

> `./tools/correlations -config finconf.dat [-r 0.3]`

The `-config FILE` option is required. The tool reads simulation metadata from `result.dat` and writes `correlations.dat`, potentially overwriting a previously existing file with that name. The optional `-r DISTANCE` selects the correlation distance; if omitted, the hopping distance from `result.dat` is used. The distance must be positive and must satisfy the same interaction-table reach condition as a simulation hopping distance. Distances that are not larger than two grid cells are accepted with a warning because they do not resolve the full `+/- 2`-cell delta-function smearing away from zero distance.

The output contains one flattened row-major row for every grid site, including unoccupied sites, with this schema:

```text
# cell x y occupation local_grasshopper_probability nn_fraction
```

`local_grasshopper_probability` is the raw local grasshopper interaction divided by `2*pi*r*sqrt(N)`.
`nn_fraction` is the nearest-neighbor count divided by four.
The tool also prints the selected distance, global grasshopper probability, global nearest-neighbor probability, and output filename to standard output.

The plot-only Python script reads this spatial output without repeating any correlation calculations:

> `python3 tools/correlation_plot.py correlations.dat`

It requires NumPy and Matplotlib. By default it plots occupation, normalized local grasshopper probability over the full grid, and the same probability masked to occupied cells. Use repeated `--field` options to plot only selected fields, and `--save FILE` to write the figure instead of opening an interactive window. Normal simulation builds and C++ unit tests do not require these Python plotting dependencies.

## License

This software is distributed under the GNU General Public License version 3 or, at your option, any later version (GPL-3.0-or-later). See [LICENSE](LICENSE) for details.

Third-party components retain their respective licenses.
