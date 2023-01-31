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

struct gridcell {
    int x;
    int y;
    double energy;    
};

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
    
    bool grid[gridSize][gridSize];
    double energyGrid[gridSize][gridSize];
	vector< pair<int,int> > spinArray(totalNumSpins);
	vector< pair<int,int> > noSpinArray(gridArea-totalNumSpins);
    
    vector< gridcell > dNeighbourTemplate;
	vector< gridcell > dNeighbourTable[gridSize][gridSize];	//for each grid point: list of grid points that are its d-neighbours with corresponding energies
    
    // construct generic neighbor list
    auto begin = chrono::high_resolution_clock::now();
    
    double thisEnergyContribution;
    pair<int,int> centerPosition=make_pair(gridSize/2,gridSize/2);        
    for(unsigned int i=0;i<gridSize;i++) 
        {
        for(unsigned int j=0;j<gridSize;j++)
            {
            if(isAround(d,euclideanDistance(centerPosition, make_pair(i,j)) ))
                {
                thisEnergyContribution=contributionEnergy(d, euclideanDistance(centerPosition,make_pair(i,j)) );
                if(thisEnergyContribution > EPS) 
                    {
                    gridcell this_cell;
                    this_cell.x = i;
                    this_cell.y = j;
                    this_cell.energy = thisEnergyContribution;
                    dNeighbourTemplate.push_back(this_cell);
                    }
                }
            }
        }
    
    // make list of neighbor lists for each grid cell
	for(unsigned int k=0;k<dNeighbourTemplate.size();k++)
		{
        int relx = dNeighbourTemplate[k].x - gridSize/2;
        int rely = dNeighbourTemplate[k].y - gridSize/2;
        for(unsigned int i=0;i<gridSize;i++) 
            {
            for(unsigned int j=0;j<gridSize;j++)
                {
                int gridLocationx = i + relx;
                int gridLocationy = j + rely;
                if(gridLocationx >= 0 && gridLocationy >= 0 && gridLocationx < gridSize && gridLocationy < gridSize)
                    {
                    gridcell this_cell;
                    this_cell.x = gridLocationx;
                    this_cell.y = gridLocationy;
                    this_cell.energy = dNeighbourTemplate[k].energy; 
                    dNeighbourTable[i][j].push_back(this_cell);
                    }
                }
            }
        }

    auto now = chrono::high_resolution_clock::now();
	cout << "Time to construct neighbors list: " <<chrono::duration_cast<chrono::milliseconds>(now-begin).count() << "ms" << endl;

    // initialize grid with random configuration
    for(unsigned int i=0;i<gridSize;i++) 
        {
        for(unsigned int j=0;j<gridSize;j++)
            {
            grid[i][j]=false;
            }
        }
    int newSpinCoord; unsigned int spincounter=0;
    while(spincounter<totalNumSpins)
        {
        bool create=true;
        while(create==true)
            {
            newSpinCoord=gsl_rng_uniform_int (RNG, gridArea);
            create=grid[xcoord(newSpinCoord)][ycoord(newSpinCoord)];
            }
        grid[xcoord(newSpinCoord)][ycoord(newSpinCoord)]=true;
        spinArray[spincounter] = make_pair(xcoord(newSpinCoord),ycoord(newSpinCoord));
        spincounter++;
        }
        
    unsigned int noSpinCounter=0;
    for(unsigned int i=0;i<gridArea;i++) 
        {
        if(grid[xcoord(i)][ycoord(i)]==false) {noSpinArray[noSpinCounter]=make_pair(xcoord(i),ycoord(i)); noSpinCounter++;}
        }
        
    // Fill the energy grid
    for(unsigned int i=0;i<gridSize;i++) 
        {
        for(unsigned int j=0;j<gridSize;j++)
            {
            energyGrid[i][j]=0;
            for(unsigned int k=0;k<dNeighbourTable[i][j].size();k++)
                {
                if(grid[dNeighbourTable[i][j][k].x][dNeighbourTable[i][j][k].y]==true)
                    {energyGrid[i][j] += dNeighbourTable[i][j][k].energy;}
                }
            }
        }
        
//             // check that this was done right by checking energy of initial system
//                         double energy=0;
//                         for(unsigned int i=0;i<totalNumSpins;i++)
//                             {
//                             for(unsigned int j=i;j<totalNumSpins;j++)
//                                 {
//                                 energy+=contributionEnergy(euclideanDistance(spinArray[i],spinArray[j]),d);
//                                 }
//                             }
//                         cout << "Full energy of initial system (counting all pairs of spins): " << energy << endl;
//                         energy=0;
//                         for(unsigned int i=0;i<totalNumSpins;i++)
//                             {
//                             for(unsigned int j=0;j<dNeighbourTable[spinArray[i].first][spinArray[i].second].size();j++)
//                                 {
//                                 if(grid[dNeighbourTable[spinArray[i].first][spinArray[i].second][j].x][dNeighbourTable[spinArray[i].first][spinArray[i].second][j].y]==true)
//                                     {energy+=dNeighbourTable[spinArray[i].first][spinArray[i].second][j].energy;}
//                                 }
//                             }
//                         energy=energy/2;
//                         cout << "Initial energy using d-neighbour table: " << energy << endl;
//                         energy = 0;
//                         for(unsigned int i=0;i<gridSize;i++)
//                             for(unsigned int j=0;j<gridSize;j++)
//                             {
//                             energy += energyGrid[i][j]*grid[i][j];
//                             }
//                         cout << "Initial energy using energy grid: " << energy/2 << endl;

    // perform a set of X regular MC updates, let's say we only accept improvements
    for(unsigned int counter=0;counter<steps;counter++)
        {
        //select random spins to destroy and to create
		int destroy=gsl_rng_uniform_int (RNG, totalNumSpins);
		pair<int,int> oldSpinCoord=spinArray[destroy];
		int create=gsl_rng_uniform_int (RNG, gridArea-totalNumSpins);
		pair<int,int> newSpinCoord=noSpinArray[create];
		
		//calculate energy difference
		double energyDifference=energyGrid[newSpinCoord.first][newSpinCoord.second]-energyGrid[oldSpinCoord.first][oldSpinCoord.second];
        if(isAround(d,euclideanDistance(newSpinCoord,oldSpinCoord)))
            {
            energyDifference -= contributionEnergy(d,euclideanDistance( newSpinCoord,oldSpinCoord ));
            }

		if(energyDifference>=0)
			{
			grid[oldSpinCoord.first][oldSpinCoord.second]=false; grid[newSpinCoord.first][newSpinCoord.second]=true;
			spinArray[destroy]=newSpinCoord; noSpinArray[create]=oldSpinCoord;
            gridcell thisCell;
        
            for(unsigned int j=0;j<dNeighbourTable[oldSpinCoord.first][oldSpinCoord.second].size();j++)
                {
                thisCell = dNeighbourTable[oldSpinCoord.first][oldSpinCoord.second][j];
                energyGrid[thisCell.x][thisCell.y] -= thisCell.energy;
                }
            for(unsigned int j=0;j<dNeighbourTable[newSpinCoord.first][newSpinCoord.second].size();j++)
                {
                thisCell = dNeighbourTable[newSpinCoord.first][newSpinCoord.second][j];
				energyGrid[thisCell.x][thisCell.y] += thisCell.energy;
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
