#include "common.hpp"

// Benchmarking code for the grasshopper storage implementations.
//
// This is based directly on grasshopper_energygrid.cpp. The numerical
// algorithm and Monte Carlo update are intentionally kept the same. Only
// storage declarations, parameterization, timing, validation, and post-run
// verification differ.

using namespace std;

#if (defined(STORAGE_VLA) + defined(STORAGE_UNIQUE_PTR) + defined(STORAGE_VECTOR)) != 1
#error "Define exactly one of STORAGE_VLA, STORAGE_UNIQUE_PTR, or STORAGE_VECTOR"
#endif

using Neighbor = pair<int,double>;
using NeighborList = vector<Neighbor>;

unsigned int totalNumSpins;
double cellsize;
unsigned int gridSize;
unsigned int gridArea;

struct BenchmarkParameters {
    double d;
    long unsigned int steps;
    long unsigned int seed;
};

static void printUsage(const char* program) {
    cerr << "Usage: " << program
         << " --N UINT --d DOUBLE --grid-size UINT --steps UINT --seed UINT\n";
}

static unsigned long long parseUnsigned(const string& text, const string& option) {
    if (text.empty() || text.front() == '-') {
        throw invalid_argument("Invalid value for " + option + ": " + text);
    }

    size_t parsed = 0;
    unsigned long long value = stoull(text, &parsed, 10);
    if (parsed != text.size()) {
        throw invalid_argument("Invalid value for " + option + ": " + text);
    }
    return value;
}

static double parseDouble(const string& text, const string& option) {
    size_t parsed = 0;
    double value = stod(text, &parsed);
    if (parsed != text.size() || !isfinite(value)) {
        throw invalid_argument("Invalid value for " + option + ": " + text);
    }
    return value;
}

static BenchmarkParameters parseCommandLine(int argc, char* argv[]) {
    bool haveN = false;
    bool haveD = false;
    bool haveGridSize = false;
    bool haveSteps = false;
    bool haveSeed = false;

    unsigned long long parsedN = 0;
    unsigned long long parsedGridSize = 0;
    unsigned long long parsedSteps = 0;
    unsigned long long parsedSeed = 0;
    double parsedD = 0;

    for (int i = 1; i < argc; i += 2) {
        if (i + 1 >= argc) {
            throw invalid_argument("Missing value for option " + string(argv[i]));
        }

        const string option = argv[i];
        const string value = argv[i + 1];

        if (option == "--N" && !haveN) {
            parsedN = parseUnsigned(value, option);
            haveN = true;
        }
        else if (option == "--d" && !haveD) {
            parsedD = parseDouble(value, option);
            haveD = true;
        }
        else if (option == "--grid-size" && !haveGridSize) {
            parsedGridSize = parseUnsigned(value, option);
            haveGridSize = true;
        }
        else if (option == "--steps" && !haveSteps) {
            parsedSteps = parseUnsigned(value, option);
            haveSteps = true;
        }
        else if (option == "--seed" && !haveSeed) {
            parsedSeed = parseUnsigned(value, option);
            haveSeed = true;
        }
        else {
            throw invalid_argument("Unknown or duplicate option: " + option);
        }
    }

    if (!(haveN && haveD && haveGridSize && haveSteps && haveSeed)) {
        throw invalid_argument("All benchmark options are required");
    }
    if (parsedN == 0 || parsedN > numeric_limits<unsigned int>::max()) {
        throw invalid_argument("N is outside the supported range");
    }
    if (parsedGridSize == 0 || parsedGridSize > numeric_limits<unsigned int>::max()) {
        throw invalid_argument("grid size is outside the supported range");
    }
    if (parsedD <= 0) {
        throw invalid_argument("d must be positive");
    }
    if (parsedSteps == 0 || parsedSteps > numeric_limits<unsigned int>::max()) {
        throw invalid_argument("steps must be between 1 and UINT_MAX");
    }
    if (parsedSeed > numeric_limits<long unsigned int>::max()) {
        throw invalid_argument("seed is outside the supported range");
    }

    const unsigned long long parsedGridArea = parsedGridSize * parsedGridSize;
    if (parsedGridArea > numeric_limits<unsigned int>::max()) {
        throw invalid_argument("grid area is outside the supported range");
    }
    if (parsedN >= parsedGridArea) {
        throw invalid_argument("N must be smaller than the grid area");
    }

    totalNumSpins = static_cast<unsigned int>(parsedN);
    gridSize = static_cast<unsigned int>(parsedGridSize);
    gridArea = static_cast<unsigned int>(parsedGridArea);
    cellsize = 1./sqrt(double(totalNumSpins));

    return BenchmarkParameters{
        parsedD,
        static_cast<long unsigned int>(parsedSteps),
        static_cast<long unsigned int>(parsedSeed)
    };
}

static void checksumMix(uint64_t& checksum, uint64_t value) {
    checksum ^= value;
    checksum *= UINT64_C(1099511628211);
}

int main(int argc, char* argv[]) {
    try {
        const BenchmarkParameters parameters = parseCommandLine(argc, argv);
        const double d = parameters.d;
        const long unsigned int steps = parameters.steps;
        long unsigned int acceptance_counter=0;

        // RNG
        gsl_rng * RNG = gsl_rng_alloc (gsl_rng_mt19937);
        if (RNG == nullptr) {
            throw runtime_error("Could not allocate the GSL random-number generator");
        }
        gsl_rng_set (RNG, parameters.seed);

        vector< pair<int,double> > dNeighbourTemplate;

        using BenchmarkClock = chrono::steady_clock;
        // allocation_seconds is not directly comparable between storage
        // representations because std::vector value-initializes its elements.
        // Overall setup cost is allocation + neighbor construction + initialization.
        const auto allocationBegin = BenchmarkClock::now();

#if defined(STORAGE_VLA)
        const char* storageName = "vla";
        bool grid[gridArea];
        double energyGrid[gridArea];
        int spinArray[totalNumSpins];
        int noSpinArray[gridArea-totalNumSpins];
        NeighborList dNeighbourTable[gridArea];
#elif defined(STORAGE_UNIQUE_PTR)
        const char* storageName = "unique_ptr";
        unique_ptr<bool[]> grid(new bool[gridArea]);
        unique_ptr<double[]> energyGrid(new double[gridArea]);
        unique_ptr<int[]> spinArray(new int[totalNumSpins]);
        unique_ptr<int[]> noSpinArray(new int[gridArea-totalNumSpins]);
        unique_ptr<NeighborList[]> dNeighbourTable(new NeighborList[gridArea]);
#elif defined(STORAGE_VECTOR)
        const char* storageName = "vector";
        vector<unsigned char> grid(gridArea);
        vector<double> energyGrid(gridArea);
        vector<int> spinArray(totalNumSpins);
        vector<int> noSpinArray(gridArea-totalNumSpins);
        vector<NeighborList> dNeighbourTable(gridArea);
#endif

        const auto allocationEnd = BenchmarkClock::now();

        // construct generic neighbor list
        const auto neighborBegin = BenchmarkClock::now();

        int center = gridSize*gridSize/2+gridSize/2;
        double thisEnergyContribution;
        pair<double,double> currentPosition=findPosition(center);
        for(unsigned int j=0;j<gridArea;j++)
            {
            if(isAround(d,euclideanDistance(currentPosition,findPosition(j))))//TODO no longer need this check
                {
                thisEnergyContribution=contributionEnergy(d,euclideanDistance(currentPosition,findPosition(j)));
                pair<int,double> thisPair(j,thisEnergyContribution);
                if(thisEnergyContribution > EPS) dNeighbourTemplate.push_back(thisPair);
                }
            }

        // make list of neighbor lists for each grid cell
        for(unsigned int j=0;j<dNeighbourTemplate.size();j++)
            {
            int coord = dNeighbourTemplate[j].first;
            int relx = xcoord(coord) - gridSize/2;
            int rely = ycoord(coord) - gridSize/2;
            double relativeEnergy = dNeighbourTemplate[j].second;
            for(unsigned int i=0;i<gridArea;i++)
                {
                int gridLocationx = xcoord(i) + relx;
                int gridLocationy = ycoord(i) + rely;
                if(gridLocationx >= 0 && gridLocationy >= 0 && gridLocationx < gridSize && gridLocationy < gridSize)
                    {
                    pair<int,double> thisPair(gridLocationx + gridLocationy*gridSize, relativeEnergy);
                    dNeighbourTable[i].push_back(thisPair);
                    }
                }
            }

        const auto neighborEnd = BenchmarkClock::now();

        // initialize grid with random configuration
        const auto initializationBegin = BenchmarkClock::now();

        for(unsigned int i=0;i<gridArea;i++)
            {
            grid[i]=false;
            }
        int newSpinCoord; unsigned int spincounter=0;
        while(spincounter<totalNumSpins)
            {
            bool create=true;
            while(create==true)
                {
                newSpinCoord=gsl_rng_uniform_int (RNG, gridArea);
                create=grid[newSpinCoord];
                }
            grid[newSpinCoord]=true;
            spinArray[spincounter]=newSpinCoord;
            spincounter++;
            }

        unsigned int noSpinCounter=0;
        for(unsigned int i=0;i<gridArea;i++)
            {
            if(grid[i]==false) {noSpinArray[noSpinCounter]=i; noSpinCounter++;}
            }

        // Fill the energy grid
        for(unsigned int i=0;i<gridArea;i++)
            {
            energyGrid[i]=0;
            for(unsigned int j=0;j<dNeighbourTable[i].size();j++)
                {
                if(grid[dNeighbourTable[i][j].first]==true)
                    {energyGrid[i] += dNeighbourTable[i][j].second;}
                }
            }

        const auto initializationEnd = BenchmarkClock::now();

        // perform a set of X regular MC updates, let's say we only accept improvements
        const auto mcBegin = BenchmarkClock::now();

        for(unsigned int counter=0;counter<steps;counter++)
            {
            //select random spins to destroy and to create
            int destroy=gsl_rng_uniform_int (RNG, totalNumSpins);
            int oldSpinCoord=spinArray[destroy];
            int create=gsl_rng_uniform_int (RNG, gridArea-totalNumSpins);
            int newSpinCoord=noSpinArray[create];

            //calculate energy difference
            double energyDifference=energyGrid[newSpinCoord]-energyGrid[oldSpinCoord];
            if(isAround(d,euclideanDistance(findPosition(newSpinCoord),findPosition(oldSpinCoord))))
                {
                energyDifference -= contributionEnergy(d,euclideanDistance( findPosition(newSpinCoord),findPosition(oldSpinCoord) ));
                }

            if(energyDifference>=0)
                {
                grid[oldSpinCoord]=false; grid[newSpinCoord]=true;
                spinArray[destroy]=newSpinCoord; noSpinArray[create]=oldSpinCoord;

                for(unsigned int j=0;j<dNeighbourTable[oldSpinCoord].size();j++)
                    {
                    energyGrid[dNeighbourTable[oldSpinCoord][j].first] -= dNeighbourTable[oldSpinCoord][j].second;
                    }
                for(unsigned int j=0;j<dNeighbourTable[newSpinCoord].size();j++)
                    {
                    energyGrid[dNeighbourTable[newSpinCoord][j].first] += dNeighbourTable[newSpinCoord][j].second;
                    }

                acceptance_counter++;
                }
            }

        const auto mcEnd = BenchmarkClock::now();

        // Post-run verification is intentionally outside all timed regions.
        double finalEnergy=0;
        unsigned int occupiedCount=0;
        size_t neighborTableEntries=0;
        for(unsigned int i=0;i<gridArea;i++)
            {
            finalEnergy += energyGrid[i]*grid[i];
            occupiedCount += static_cast<unsigned int>(grid[i] != false);
            neighborTableEntries += dNeighbourTable[i].size();
            }
        finalEnergy=finalEnergy/2;

        uint64_t finalStateChecksum = UINT64_C(1469598103934665603);
        checksumMix(finalStateChecksum, totalNumSpins);
        for(unsigned int i=0;i<totalNumSpins;i++)
            {
            checksumMix(finalStateChecksum, static_cast<uint64_t>(spinArray[i]));
            }
        checksumMix(finalStateChecksum, gridArea-totalNumSpins);
        for(unsigned int i=0;i<gridArea-totalNumSpins;i++)
            {
            checksumMix(finalStateChecksum, static_cast<uint64_t>(noSpinArray[i]));
            }

        const double allocationSeconds = chrono::duration<double>(allocationEnd-allocationBegin).count();
        const double neighborSeconds = chrono::duration<double>(neighborEnd-neighborBegin).count();
        const double initializationSeconds = chrono::duration<double>(initializationEnd-initializationBegin).count();
        const double mcSeconds = chrono::duration<double>(mcEnd-mcBegin).count();

        cout << setprecision(numeric_limits<double>::max_digits10)
             << "storage=" << storageName << '\n'
             << "N=" << totalNumSpins << '\n'
             << "grid_size=" << gridSize << '\n'
             << "grid_area=" << gridArea << '\n'
             << "d=" << d << '\n'
             << "steps=" << steps << '\n'
             << "seed=" << parameters.seed << '\n'
             << "allocation_seconds=" << allocationSeconds << '\n'
             << "allocation_note=not_directly_comparable_vector_value_initializes_elements\n"
             << "neighbor_construction_seconds=" << neighborSeconds << '\n'
             << "initialization_seconds=" << initializationSeconds << '\n'
             << "overall_setup_seconds=" << allocationSeconds+neighborSeconds+initializationSeconds << '\n'
             << "mc_seconds=" << mcSeconds << '\n'
             << "neighbor_template_entries=" << dNeighbourTemplate.size() << '\n'
             << "neighbor_table_entries=" << neighborTableEntries << '\n'
             << "accepted_moves=" << acceptance_counter << '\n'
             << "acceptance_ratio=" << acceptance_counter/double(steps) << '\n'
             << "occupied_count=" << occupiedCount << '\n'
             << "no_spin_count=" << noSpinCounter << '\n'
             << "final_energy=" << finalEnergy << '\n'
             << "final_state_checksum=" << finalStateChecksum << '\n';

        gsl_rng_free(RNG);
        return 0;
    }
    catch (const exception& error) {
        cerr << "Error: " << error.what() << '\n';
        printUsage(argv[0]);
        return 1;
    }
}
