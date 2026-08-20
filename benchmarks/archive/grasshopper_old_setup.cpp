#include "common.hpp"

// Benchmarking code for the grasshopper
// Setup: square grid, default delta function discretization, random config 
//        fixed temperature=1, fixed number of MC updates
//        compare MC time after neighborslist initiated
//        also note time to create the neighborslist
//        TODO need to benchmark specifically regimes with low acceptance rates

// This code is a minimal version of the old setup for benchmarking

using namespace std;

unsigned int totalNumSpins = 10000;
double cellsize = 0.01;
unsigned int gridSize = 200;
unsigned int gridArea = gridSize*gridSize;

int main() {
    
    // test parameters
    double d = 0.3;
    long unsigned int steps = 1000000;
    long unsigned int acceptance_counter=0;
    
    // RNG
	auto seed=12345;
	gsl_rng * RNG = gsl_rng_alloc (gsl_rng_mt19937);
	gsl_rng_set (RNG, seed);
    
    //bitset<gridArea> grid;
    bool grid[gridArea];					//true if spin=1 at this grid point
	int spinArray[totalNumSpins];				//grid point where any spin is
	int noSpinArray[gridArea-totalNumSpins];		//complementary to above: grid point where no spin is
	vector< pair<int,double> > dNeighbourTable[gridArea];	//for each grid point: list of grid points that are its d-neighbours with corresponding energies

    // construct generic neighbor list
    auto begin = chrono::high_resolution_clock::now();
    
    // make list of neighbor lists for each grid cell (?)
    
    double thisEnergyContribution;
	for(unsigned int i=0;i<gridArea;i++)
		{
		pair<double,double> currentPosition=findPosition(i);
		for(unsigned int j=0;j<gridArea;j++)
			{
			if(isAround(d,euclideanDistance(currentPosition,findPosition(j))))
				{
				thisEnergyContribution=contributionEnergy(d,euclideanDistance(currentPosition,findPosition(j)));
				pair<int,double> thisPair(j,thisEnergyContribution);
				if(thisEnergyContribution > EPS) dNeighbourTable[i].push_back(thisPair);
				}
			}
		}
		
    // check that grid is right
    //for(unsigned int j=0;j<dNeighbourTable[10070].size();j++) cout << dNeighbourTable[10070][j].first << '\t' << dNeighbourTable[10070][j].second << endl;
    
    auto now = chrono::high_resolution_clock::now();
	cout << "Time to construct neighbors list: " <<chrono::duration_cast<chrono::milliseconds>(now-begin).count() << "ms" << endl;

    // initialize grid with random configuration
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

    // perform a set of X regular MC updates, let's say we only accept improvements
    for(unsigned int counter=0;counter<steps;counter++)
        {
        //select random spins to destroy and to create
		int destroy=gsl_rng_uniform_int (RNG, totalNumSpins);
		int oldSpinCoord=spinArray[destroy];
		int create=gsl_rng_uniform_int (RNG, gridArea-totalNumSpins);
		int newSpinCoord=noSpinArray[create];
		
		//calculate energy difference
		double energyDifference=0;
		for(unsigned int j=0;j<dNeighbourTable[oldSpinCoord].size();j++)
			{
			if(grid[dNeighbourTable[oldSpinCoord][j].first]==true)
				{energyDifference-=dNeighbourTable[oldSpinCoord][j].second;}
			}
		for(unsigned int j=0;j<dNeighbourTable[newSpinCoord].size();j++)
			{
			if( (grid[dNeighbourTable[newSpinCoord][j].first]==true) && (dNeighbourTable[newSpinCoord][j].first!=oldSpinCoord) )
				{energyDifference+=dNeighbourTable[newSpinCoord][j].second;}
			}
			
//cout << energyDifference << endl;

		if(energyDifference>=0)
			{
			grid[oldSpinCoord]=false; grid[newSpinCoord]=true;
			spinArray[destroy]=newSpinCoord; noSpinArray[create]=oldSpinCoord;
			acceptance_counter++;
			}
        }

    // benchmark the time  
    auto end = chrono::high_resolution_clock::now();
    cout << "Benchmark time: " << chrono::duration_cast<chrono::milliseconds>(end-now).count() << "ms" << endl;
    cout << "Acceptance ratio: " << acceptance_counter/double(steps) << endl;
    
    return 0;
}
