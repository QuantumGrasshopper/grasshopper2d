#include "annealing.hpp"
#include "interactions.hpp"
#include "output.hpp"
#include "parameters.hpp"
#include "setup.hpp"
#include "utilities.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <gsl/gsl_rng.h>

unsigned int totalNumSpins;
double cellSize;
unsigned int gridSize;
unsigned int gridArea;
double tempScaling;
int deltaOption;

using namespace std; 

int main(int inputN,char *inputV[]) {
    try {
    const int outputPrecision=numeric_limits<double>::max_digits10;
    auto formatDouble = [](double value) {
        ostringstream buffer;
        buffer << setprecision(outputPrecision) << value;
        return buffer.str();
    };
    
    // SETUP -------------------------------------------------------------------------------------
	
    // read parameters from command-line arguments and define derived quantities

    const SimulationParameters parameters = parseSimulationParameters(inputN, inputV);
    const double d = parameters.hoppingDistance;
    totalNumSpins = parameters.totalNumSpins;
    cellSize=1./sqrt(double(totalNumSpins));

    if (parameters.gridSize.has_value()) {
        gridSize = *parameters.gridSize;
    }
    else {
        const double automaticHalfGridSize =
            sqrt(double(totalNumSpins)) + 3*d/cellSize + EPS;
        if (!isfinite(automaticHalfGridSize)
            || automaticHalfGridSize > numeric_limits<int>::max()) {
            throw invalid_argument("Automatically selected grid size is outside the supported range.");
        }
        gridSize=2*static_cast<unsigned int>(static_cast<int>(automaticHalfGridSize));
    }

    const unsigned long long selectedGridArea =
        static_cast<unsigned long long>(gridSize) * gridSize;
    if (selectedGridArea > numeric_limits<unsigned int>::max()) {
        throw invalid_argument("Grid area is outside the supported range.");
    }
    if (static_cast<unsigned long long>(totalNumSpins) >= selectedGridArea) {
        throw invalid_argument("N must be smaller than the grid area.");
    }
    gridArea = static_cast<unsigned int>(selectedGridArea);
    validateInteractionTableReach(d);
    
    double maxtime=parameters.hours;
    maxtime=60*60*maxtime*1000;
    
    long unsigned int maxsteps=parameters.maxSteps;
    
	long unsigned int temproundsteps=parameters.temperatureRoundSteps.value_or(0UL);
    if(temproundsteps>maxsteps) temproundsteps=maxsteps/1000UL;
    if(temproundsteps<10) temproundsteps=totalNumSpins;
    
	double temperature=parameters.initialTemperature;
	double finaltemperature=parameters.finalTemperature;
    
    int numberannealingsteps=parameters.annealingSteps;
    tempScaling=pow((finaltemperature/temperature),1./double(numberannealingsteps));
    int configOutputs = parameters.configurationOutputs; // maximal number of configuration snapshots to save for animation
	const int plannedIntermediateSnapshots = configOutputs >= 2
	    ? min(configOutputs - 2, numberannealingsteps - 1) : 0;
	int coolingStageIndex=0;
	int nextIntermediateSnapshot=1;
	int configurationsWritten=0;
	string pendingIntermediateSnapshot;
	
    string initconf=parameters.initialConfiguration;
    deltaOption=parameters.deltaOption;

    double NNint = parameters.nearestNeighborInteraction;

    prepareOutputFiles(parameters.overwriteExistingOutputs, initconf == "load");

	gsl_rng * RNG = gsl_rng_alloc (gsl_rng_mt19937);
    long unsigned int seed=parameters.randomSeed.value_or(0UL);
    // Use system clock if no random seed supplied (seed is always output)
    if(seed < EPS) seed=chrono::duration_cast<chrono::seconds>(chrono::system_clock::now().time_since_epoch()).count();
	gsl_rng_set (RNG, seed);

    ofstream result("result.dat");
	if (!result.is_open()) {
		throw runtime_error("Failed to open output file result.dat.");
	}
	result << setprecision(outputPrecision);
	
	unsigned long temproundcounter=0;
	double accratio;
    long unsigned int counter=0; long accepted=0; long accepted_current=0;
    
    result << "2D Grasshopper with Simulated Annealing, Euclidean metric\n\n"
           << "Total number of spins: " << totalNumSpins << '\n'
           << "Hopping distance: " << d << '\n'
           << "Nearest Neighbor interaction coefficient: " << NNint << '\n'
           << "Size of grid: " << gridSize << '\n'
           << "Size of cell: " << cellSize << "\n\n"
	       << "Random seed: " << seed << '\n'
           << "Option for delta-function discretization: " << deltaOption << '\n'
	       << "Initial temperature: " << temperature << '\n'
	       << "Final temperature: " << finaltemperature << '\n'
	       << "Temperature scaling factor: " << tempScaling << '\n'
	       << "Number of annealing steps: " << numberannealingsteps << '\n'
	       << "Initial number of steps before temperature scaling: " << temproundsteps << "\n\n";
	checkOutputStream(result, "result.dat", "write");

    // CONSTRUCT NEIGHBOR LIST ---------------------------------------------------------------------------  

    auto begin = chrono::high_resolution_clock::now();

    auto interactionTable = buildInteractionTable(d);
		
    auto now = chrono::high_resolution_clock::now();
    auto timeDiff = chrono::duration_cast<chrono::milliseconds>(now-begin).count();
	result << "Time to construct neighbors list: " << timeDiff << "ms" << endl;
	checkOutputStream(result, "result.dat", "write");
    
    // INITIAL SPIN CONFIGURATION ------------------------------------------------------------------------
    
    std::vector<unsigned char> grid(gridArea);          //true if spin=1 at this grid point
	std::vector<int> spinArray(totalNumSpins);          //grid point where any spin is
	std::vector<int> noSpinArray(gridArea-totalNumSpins); //complementary to above: grid point where no spin is
    
    initialize(grid.data(), spinArray.data(), RNG, initconf);

    auto energyGrid = buildGrasshopperInteractionGrid(grid.data(), interactionTable);
    double grasshopperEnergy = totalGrasshopperInteraction(grid.data(), energyGrid);
    
    unsigned int noSpinCounter=0;
    long long nearestNeighborBonds = 0;
	for(unsigned int i=0;i<gridArea;i++)
		{
		if(grid[i]==false) {noSpinArray[noSpinCounter]=i; noSpinCounter++;}
		// NN contributions
		else {nearestNeighborBonds+=nearestNeighborCount(i, grid.data());}
		}

    nearestNeighborBonds /= 2;
    double energy = grasshopperEnergy + NNint*nearestNeighborBonds;
		
    int bufferLimit = 10000;
    int flushInterval = 60*1000;
    BufferedFileWriter energies("energies.dat", bufferLimit, chrono::milliseconds(flushInterval));
	energies.write(formatDouble(normalizeGrasshopperEnergy(energy,d)));
    BufferedFileWriter temperatures("temperatures.dat", bufferLimit, chrono::milliseconds(flushInterval));
	ofstream configuration;
	auto buildConfigurationSnapshot = [&]() {
		ostringstream buffer;
		for(unsigned int i=0;i<totalNumSpins;i++) buffer << spinArray[i] << " ";
		buffer << setprecision(outputPrecision) << normalizeGrasshopperEnergy(energy,d) << endl;
		return buffer.str();
	};
	if(configOutputs > 0)
        {
        configuration.open("config.dat");
		if (!configuration.is_open()) {
			throw runtime_error("Failed to open output file config.dat.");
		}
		if(configOutputs >= 2) {
			configuration << buildConfigurationSnapshot();
			checkOutputStream(configuration, "config.dat", "write");
			configurationsWritten++;
		}
        }
    
	std::vector<int> bestSpinArray(totalNumSpins);  //the overall best spin array during the whole run
	for(unsigned int i=0;i<totalNumSpins;i++) bestSpinArray[i]=spinArray[i];
	double bestenergy=energy;
		
    // MAIN LOOP ------------------------------------------------------------------------------------------
		
    while( (timeDiff<maxtime) && (counter<maxsteps) )
        {
		counter++; temproundcounter++;
		
        // MC update
		auto destroy=gsl_rng_uniform_int (RNG, totalNumSpins);
		unsigned int oldSpinCoord=spinArray[destroy];
		auto create=gsl_rng_uniform_int (RNG, gridArea-totalNumSpins);
		unsigned int newSpinCoord=noSpinArray[create];
		
		double grasshopperDifference=energyGrid[newSpinCoord]-energyGrid[oldSpinCoord];
        if(isAround(d,euclideanDistance(findPosition(newSpinCoord),findPosition(oldSpinCoord))))//NOTE more efficient to keep this check explicit
            {
            grasshopperDifference -= contributionEnergy(d,euclideanDistance( findPosition(newSpinCoord),findPosition(oldSpinCoord) ));
            }
        // Nearest neighbor contributions
        int nearestNeighborDifference = nearestNeighborBondDifference(oldSpinCoord, newSpinCoord, grid.data());

		bool accept;
        double energyDifference = grasshopperDifference + NNint*nearestNeighborDifference;
		if(energyDifference>=0) accept=true;
		else accept=acceptreject(energyDecreaseProbDistr(energyDifference,temperature),RNG);
		
		if(accept==true)
			{
			grid[oldSpinCoord]=false; grid[newSpinCoord]=true;
			spinArray[destroy]=newSpinCoord; noSpinArray[create]=oldSpinCoord;
            grasshopperEnergy += grasshopperDifference;
            nearestNeighborBonds += nearestNeighborDifference;
            energy = grasshopperEnergy + NNint*nearestNeighborBonds;

            //update energy grid
            for(unsigned int j=0;j<interactionTable[oldSpinCoord].size();j++)
                {
                energyGrid[interactionTable[oldSpinCoord][j].first] -= interactionTable[oldSpinCoord][j].second;
                }
            for(unsigned int j=0;j<interactionTable[newSpinCoord].size();j++)
                {
				energyGrid[interactionTable[newSpinCoord][j].first] += interactionTable[newSpinCoord][j].second;
                }
			accepted_current++;
			if(energy>bestenergy)
				{
                bestenergy=energy; 
                for(unsigned int i=0;i<totalNumSpins;i++) bestSpinArray[i]=spinArray[i];
                }
			}
        
        // cooling step
		if(temproundcounter==temproundsteps)
			{
			if(temperature>finaltemperature) 
				{
				temperature=temperatureDecrease(temperature);
				coolingStageIndex++;

				if(!pendingIntermediateSnapshot.empty()) {
					configuration << pendingIntermediateSnapshot;
					checkOutputStream(configuration, "config.dat", "write");
					configurationsWritten++;
					pendingIntermediateSnapshot.clear();
				}

				if(nextIntermediateSnapshot <= plannedIntermediateSnapshots) {
					const int intermediateTarget = static_cast<int>(
						static_cast<long long>(nextIntermediateSnapshot)
						* numberannealingsteps
						/ (plannedIntermediateSnapshots + 1));
					if(coolingStageIndex == intermediateTarget) {
						// Hold this row until a later cooling stage so an early stop
						// replaces it with the actual final configuration instead.
						pendingIntermediateSnapshot = buildConfigurationSnapshot();
						nextIntermediateSnapshot++;
					}
				}
				}
			accratio=accepted_current/double(temproundcounter);
            temperatures.write(to_string(counter) + '\t' + formatDouble(temperature)
                               + '\t' + formatDouble(accratio));
			energies.write(formatDouble(normalizeGrasshopperEnergy(energy,d)));
			accepted+=accepted_current; accepted_current=0;
			temproundcounter=0;
			temproundsteps=stepIncrease(temproundsteps);
			}
		
		now = chrono::high_resolution_clock::now();
		timeDiff = chrono::duration_cast<chrono::milliseconds>(now-begin).count();
		}
		
    // WRAP UP --------------------------------------------------------------------------------------------
    
    temperatures.finish();
    energies.finish();

    const long totalAccepted = accepted + accepted_current;
    const double averageAcceptanceRatio =
    counter > 0 ? totalAccepted / double(counter) : 0.0;
    
	result << "\nSimulation took " << timeDiff/60./1000 << " minutes" << '\n'
	       << "Finished after " << counter << " steps" << '\n'
	       << "Final temperature: " << temperature << '\n'
	       << "Average acceptance ratio: " << averageAcceptanceRatio << "\n\n"

           << "final grasshopper energy: " << grasshopperEnergy << '\n'
           << "final nearest neighbor bonds: " << nearestNeighborBonds << '\n'
           << "final total MC objective: " << energy << '\n'
           << "best total MC objective: " << bestenergy << '\n'
           << "final grasshopper probability: " << normalizeGrasshopperEnergy(grasshopperEnergy, d) << '\n'
           << "final nearest neighbor probability: " << nearestNeighborProbability(nearestNeighborBonds) << '\n'
        // normalized MC objective; equals grasshopper probability when NNint == 0
           << "final normalized MC objective: " << normalizeGrasshopperEnergy(energy, d) << '\n'
           << "best normalized MC objective: " << normalizeGrasshopperEnergy(bestenergy, d) << "\n\n";

	checkOutputStream(result, "result.dat", "write");

	if (configOutputs > 0) {
		if(configurationsWritten < configOutputs
		   && (configurationsWritten == 0 || counter > 0)) {
			configuration << buildConfigurationSnapshot();
			checkOutputStream(configuration, "config.dat", "write");
			configurationsWritten++;
		}
		finishOutputFile(configuration, "config.dat");
	}
	finishOutputFile(result, "result.dat");
    
    saveConfig(spinArray.data(), "finconf.dat");
    saveConfig(bestSpinArray.data(), "bestconf.dat");
    
    return 0;
    }
    catch (const exception& error) {
        cerr << "Error: " << error.what() << endl;
        return 1;
    }
}
