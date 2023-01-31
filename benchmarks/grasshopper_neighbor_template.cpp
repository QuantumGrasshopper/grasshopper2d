#include "common.hpp"

// Benchmarking code for the grasshopper
// Setup: square grid, default delta function discretization, random config 
//        fixed temperature=1, fixed number of MC updates
//        compare MC time after neighborslist initiated
//        also note time to create the neighborslist
//        TODO need to benchmark specifically regimes with low acceptance rates

// Instead of having a neighborslist for each point we only have a generic template (center point)
// The grid is implemented as 2d array 
// For a specific neighborslist we translate the neighbors template in real time => this is much slower, as expected

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
    bool grid[gridSize][gridSize];					//true if spin=1 at this grid point
	int spinArray[totalNumSpins];				//grid point where any spin is
	int noSpinArray[gridArea-totalNumSpins];		//complementary to above: grid point where no spin is
	// no longer using pairs: three vectors instead
	vector< int > dNeighbourTemplate_x;
    vector< int > dNeighbourTemplate_y;
    vector< double > dNeighbourTemplate_en;

    // construct generic neighbor list
    auto begin = chrono::high_resolution_clock::now();
    
    int center = gridSize*gridSize/2+gridSize/2;
    double thisEnergyContribution;
    pair<double,double> currentPosition=findPosition(center);
    for(int i=0;i<gridSize;i++) 
        {
        for(int j=0;j<gridSize;j++)
            {
            if(isAround(d,euclideanDistance(currentPosition, make_pair(i*cellsize,j*cellsize)) ))
                {
                thisEnergyContribution=contributionEnergy(d, euclideanDistance(currentPosition,make_pair(i*cellsize,j*cellsize)) );
                if(thisEnergyContribution > EPS) 
                    {
                    dNeighbourTemplate_x.push_back(i-gridSize/2);
                    dNeighbourTemplate_y.push_back(j-gridSize/2);
                    dNeighbourTemplate_en.push_back(thisEnergyContribution);
                    }
                }
            }
        }
    
    auto now = chrono::high_resolution_clock::now();
	cout << "Time to construct neighbors list: " <<chrono::duration_cast<chrono::milliseconds>(now-begin).count() << "ms" << endl;

    // initialize grid with random configuration
    for(int i=0;i<gridSize;i++) 
        {
        for(int j=0;j<gridSize;j++)
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
        spinArray[spincounter]=newSpinCoord;
        spincounter++;
        }
        
    unsigned int noSpinCounter=0;
    for(int i=0;i<gridArea;i++) 
        {
        if(grid[xcoord(i)][ycoord(i)]==false) {noSpinArray[noSpinCounter]=i; noSpinCounter++;}
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
		for(unsigned int j=0;j<dNeighbourTemplate_x.size();j++)
			{
            int x = dNeighbourTemplate_x[j]  +oldSpinCoord%gridSize;
            int y = dNeighbourTemplate_y[j]  +(oldSpinCoord-oldSpinCoord%gridSize)/gridSize;
            if(x >= 0 && y >= 0 && x < gridSize && y < gridSize)
                {
                if(grid[x][y]==true)
                    {energyDifference-=dNeighbourTemplate_en[j];}
                }
            x = dNeighbourTemplate_x[j] +newSpinCoord%gridSize;
            y = dNeighbourTemplate_y[j] +(newSpinCoord-newSpinCoord%gridSize)/gridSize;
            if(x >= 0 && y >= 0 && x < gridSize && y < gridSize)
                {
                if(grid[x][y]==true && x+gridSize*y != oldSpinCoord)
                    {energyDifference+=dNeighbourTemplate_en[j];}
                }
			}

		if(energyDifference>=0)
			{
			grid[oldSpinCoord%gridSize][(oldSpinCoord-oldSpinCoord%gridSize)/gridSize]=false; grid[newSpinCoord%gridSize][(newSpinCoord-newSpinCoord%gridSize)/gridSize]=true;
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
