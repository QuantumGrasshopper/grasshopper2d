#include "common.hpp"

// Benchmarking code for the grasshopper
// Setup: square grid, default delta function discretization, random config 
//        fixed temperature=1, fixed number of MC updates
//        compare MC time after neighborslist initiated
//        also note time to create the neighborslist
//        TODO need to benchmark specifically regimes with low acceptance rates

// Keeping track of energy contributions of each grid point
// Only need to change these if an update was accepted

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
    
    bool grid[gridArea];
    double energyGrid[gridArea];
	int spinArray[totalNumSpins];
	int noSpinArray[gridArea-totalNumSpins];
	vector< pair<int,double> > dNeighbourTemplate;
	vector< pair<int,double> > dNeighbourTable[gridArea];	//for each grid point: list of grid points that are its d-neighbours with corresponding energies

    // construct generic neighbor list
    auto begin = chrono::high_resolution_clock::now();
    
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
		
    // check that grid is right
    //for(unsigned int j=0;j<dNeighbourTable[405].size();j++) cout << dNeighbourTable[405][j].first << '\t' << dNeighbourTable[405][j].second << endl;
    
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
        
    // check that this was done right by checking energy of initial system
// 	double energy=0;
// 	for(int i=0;i<totalNumSpins;i++)
// 		{
// 		for(int j=i;j<totalNumSpins;j++)
// 			{
// 			energy+=contributionEnergy(euclideanDistance(findPosition(spinArray[i]),findPosition(spinArray[j])),d);
// 			}
// 		}
// 	cout << "Full energy of initial system (counting all pairs of spins): " << energy << endl;
// 	energy=0;
// 	for(int i=0;i<totalNumSpins;i++)
// 		{
// 		for(unsigned int j=0;j<dNeighbourTable[spinArray[i]].size();j++)
// 			{
// 			if(grid[dNeighbourTable[spinArray[i]][j].first]==true)
// 				{energy+=dNeighbourTable[spinArray[i]][j].second;}
// 			}
// 		}
// 	energy=energy/2;
// 	cout << "Initial energy using d-neighbour table: " << energy << endl;
// 	energy = 0;
//     for(unsigned int i=0;i<gridArea;i++)
//         {
//         energy += energyGrid[i]*grid[i];
//         }
//     cout << "Initial energy using energy grid: " << energy/2 << endl;
    

    // perform a set of X regular MC updates, let's say we only accept improvements
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

    // benchmark the time  
    auto end = chrono::high_resolution_clock::now();
    cout << "Benchmark time: " << chrono::duration_cast<chrono::milliseconds>(end-now).count() << "ms" << endl;
    cout << "Acceptance ratio: " << acceptance_counter/double(steps) << endl;
    
    return 0;
}
