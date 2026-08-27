# Benchmarks

The benchmark programs record some of the implementation choices that led to the current code design. They are mainly for reference and separate from the supported production and test builds.

## Algorithm development sequence

1. `archive/grasshopper_old_setup.cpp` (untracked) constructs every site's d-neighbor list independently by checking every pair of grid sites. Each MC proposal scans the old and new neighbor lists. This is very inefficient.
2. `grasshopper_relative_neighbors.cpp` (tracked) replaces the quadratic d-neighbor list construction with one central template, subsequently translated into a full d-neighbor table for each grid site. Each MC proposal scans the old and new neighbor lists.
3. `grasshopper_neighbor_template.cpp` (tracked) keeps only the d-neighbor template at the central site and translates it on demand to the appropriate grid sites during each proposed MC move. This requires less memory, since only one template is stored, but at the cost of slower updates compared to the version above.
4. `archive/grasshopper_energygrid.cpp` (untracked) keeps track of current grasshopper energy contributions of each grid point. This makes MC acceptance probability evaluation almost instantaneous. The energy field only needs to be changed when a proposal is accepted. This is very efficient and the production simulation uses this design.
5. `archive/grasshopper_storage_benchmark.cpp` (untracked) compares the efficiency of runtime-sized stack arrays, `std::unique_ptr<T[]>`, and `std::vector`. The numerical results match, and repeated runs confirm that there is almost no performance difference, supporting the production choice of `std::vector`.

## Active benchmarks

- `grasshopper_relative_neighbors.cpp` constructs a translated full neighbor table and scans the affected site lists directly for each proposed move.
- `grasshopper_neighbor_template.cpp` stores only relative neighbor offsets and performs translation and boundary checks for each proposed move.

These two benchmark programs are retained to compare the runtime and memory tradeoff between precomputed per-site tables and template-only translation. In a production simulation, they would both be used together with a field storing current energy contributions, cf. buildGrasshopperInteractionGrid(...). This field is not part of the benchmark to better assess the relative performance difference.

To build these benchmarks run `make` inside the `benchmarks/` subfolder. 

## Active benchmark results

Both active benchmarks were executed with the same random seed and initial configuration and ran 10^6 MC proposals. Only energy-increasing moves were accepted. Matching acceptance ratios were used as a simple check that the two implementations perform equivalent updates. Monte Carlo timings and memory usage reported below are medians of three runs; maximum resident set size (RSS) was measured with `/usr/bin/time -v`. For the scaling analysis, the jump length was kept fixed at $d=0.3$, while the number of spins and grid size were varied keeping `gridSize/sqrt(N)` approximately fixed. The latter implies that the total grid area is proportional to $N$. Since the number of d-neighbors of each point scales as `d/cellSize` or equivalently `d*sqrt(N)`, we expect the memory for storing the template to scale as $N^{1/2}$ and the memory for storing the full table to scale as $N^{3/2}$. 

|    (N) | grid | table setup | table MC | template MC | template/table MC | table RSS | template RSS |
| -----: | ---: | ----------: | -------: | ----------: | ----------------: | --------: | -----------: |
|  2,500 |  100 |       46 ms |  1.344 s |     2.395 s |             1.78× |     72 MB |       4.7 MB |
| 10,000 |  200 |      388 ms |  3.241 s |     5.658 s |             1.75× |    537 MB |       4.8 MB |
| 40,000 |  400 |     3566 ms |  6.440 s |    13.429 s |             2.09× |   3.99 GB |       5.3 MB |

Across all benchmark runs, the full neighbor table was consistently faster, reducing direct neighbor-processing time by approximately 43–52%. Its memory cost increased much more rapidly, from approximately 72 MB at N=2500 to 4.0 GB at N=40000, consistent with the expected scaling. The relative template remained near the process baseline in memory while requiring roughly 1.8–2.1 times the MC time. These results support the decision to use the full table for current 2D production calculations when memory permits, while retaining the template approach as a useful low-memory alternative.

## Archived benchmarks

Historical benchmark sources may be retained locally under the git-ignored `archive/` directory. They are not distributed with the repository or part of the supported benchmark build. Their purpose and conclusions are recorded here so that the algorithm-development history remains documented.

- `grasshopper_old_setup.cpp`: all-pairs neighbor-table construction baseline;
- `grasshopper_energygrid.cpp`: introduction of the cached interaction field;
- `grasshopper_storage_benchmark.cpp`: VLA, unique-pointer, and vector storage comparison;
- `run_storage_benchmarks.sh`: runner used for the storage comparison.


