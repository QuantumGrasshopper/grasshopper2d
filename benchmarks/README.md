# Benchmarks

The benchmark programs record some of the implementation choices that led to the current code design. They are mainly for reference and separate from the supported production and test builds.

## Algorithm development sequence

1. `archive/grasshopper_old_setup.cpp` (untracked) constructs every site's d-neighbor list independently by checking every pair of grid sites. Each MC proposal scans the old and new neighbor lists. This is very inefficient.
2. `grasshopper_relative_neighbors.cpp` (tracked) replaces the quadratic d-neighbor list construction with one central template, subsequently translated into a full d-neighbor table for each grid site. Each MC proposal scans the old and new neighbor lists.
3. `grasshopper_neighbor_template.cpp` (tracked) keeps only the d-neighbor template at the central site and translates it on demand to the appropriate grid sites during each proposed MC move. This requires less memory, since only one template is stored, but at the cost of substantially slower updates compared to the version above.
4. `archive/grasshopper_energygrid.cpp` (untracked) keeps track of current grasshopper energy contributions of each grid point. This makes MC acceptance probability evaluation almost instantaneous. The energy field only needs to be changed when a proposal is accepted. This is very efficient and the production simulation uses this design.
5. `archive/grasshopper_storage_benchmark.cpp` (untracked) compared the efficiency of runtime-sized stack arrays, `std::unique_ptr<T[]>`, and `std::vector`. The numerical results matched, and repeated runs confirmed there was almost no performance difference, supporting the production choice of `std::vector`.

## Active benchmarks

- `grasshopper_relative_neighbors.cpp` constructs a translated full neighbor table and scans the affected site lists directly for each proposed move.
- `grasshopper_neighbor_template.cpp` stores only relative neighbor offsets and performs translation and boundary checks for each proposed move.

These two benchmark programs are retained provisionally to compare the runtime and memory tradeoff between precomputed per-site tables and template-only translation.
`common.cpp` and `common.hpp` provide their shared benchmark-local geometry and interaction routines. In a production simulation, they would both be used together with a field storing current energy contributions, cf. buildGrasshopperInteractionGrid(...). This is not part of the benchmark to better assess the relative performance difference.

## Archived benchmarks

Files under `archive/` are retained as design-history documentation (untracked in the repository and not maintained) and are not part of the supported benchmark build:

- `grasshopper_old_setup.cpp`: all-pairs neighbor-table construction baseline;
- `grasshopper_energygrid.cpp`: introduction of the cached interaction field;
- `grasshopper_storage_benchmark.cpp`: VLA, unique-pointer, and vector storage comparison;
- `run_storage_benchmarks.sh`: runner used for the storage comparison.


