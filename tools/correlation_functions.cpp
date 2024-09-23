// given a configuration computes different correlations functions
// somewhat similar to old code grasshopper_dneighbourscount.cpp
// creates spatial representation of correlations and computes averages

// compile with
// g++ -O3 -Wall -std=c++17 correlation_functions.cpp ../utilities.o -o correlations -lgsl -lgslcblas -lm

#include "../utilities.hpp"

unsigned int totalNumSpins;
double cellSize;
unsigned int gridSize;
unsigned int gridArea;
int deltaOption;

using namespace std; 

int main(int inputN,char *inputV[]) { 
    
    // SETUP -------------------------------------------------------------------------------------
    
    double d;
    double corrDist;
    string line;
    
    ifstream paramfile("result.dat");
    
    if (!paramfile.is_open()) {
        cerr << "Error: Could not open result.dat" << endl;
    }

    while (getline(paramfile, line)) {
        if (line.find("Total number of spins:") != string::npos) {
            sscanf(line.c_str(), "Total number of spins: %d", &totalNumSpins);
        }
        if (line.find("Hopping distance:") != string::npos) {
            sscanf(line.c_str(), "Hopping distance: %lf", &d);
        }
        if (line.find("Size of grid:") != string::npos) {
            sscanf(line.c_str(), "Size of grid: %d", &gridSize);
        }
        if (line.find("Option for delta-function discretization:") != string::npos) {
            sscanf(line.c_str(), "Option for delta-function discretization: %d", &deltaOption);
        }
    }

    paramfile.close();
    
    cout << "2D Grasshopper correlation function analysis\n\n"
         << "Total number of spins: " << totalNumSpins << '\n'
         << "Hopping distance: " << d << '\n'
         << "Size of grid: " << gridSize << '\n'
         << "Option for delta-function discretization: " << deltaOption << endl;
    
    cellSize=1./sqrt(double(totalNumSpins));
    gridArea = gridSize*gridSize;
       
    cout << "Distance for correlation function (in natural length units of the problem):" << endl;
    cin >> corrDist;
    
    // one factor of 1/2 is already taken care of by avoiding double counting
    // includes one division by N and one division by d (instead could divide by corrDist) <= think about this
    double probabilityNormFactor = 1/PI/d/pow(double(totalNumSpins),3./2.);
    
    // CONSTRUCT NEIGHBOR LIST ---------------------------------------------------------------------------  

    vector< pair<int,double> > dNeighbourTemplate;
	vector< pair<int,double> > dNeighbourTable[gridArea];	//for each grid point: list of points that are its d-neighbours with corresponding energies
    
    int center = gridSize*gridSize/2+gridSize/2;
    double thisEnergyContribution;
    pair<double,double> currentPosition=findPosition(center);
    for(unsigned int j=0;j<gridArea;j++)
        {
        thisEnergyContribution=contributionEnergy(corrDist,euclideanDistance(currentPosition,findPosition(j)));
        pair<int,double> thisPair(j,thisEnergyContribution);
        if(thisEnergyContribution > EPS) dNeighbourTemplate.push_back(thisPair);
        }
    
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
		
    // LOAD SPIN CONFIGURATION ---------------------------------------------------------------------------
    
    bool grid[gridArea];					     //true if spin=1 at this grid point
    double corrGrid[gridArea];
	int spinArray[totalNumSpins];				 //grid point where any spin is
    
    string configFilename;
    cout << "Enter the name of the file containing the configuration: ";
    cin >> configFilename;
    
    ifstream configfile(configFilename);
    
        if (!configfile.is_open()) {
        throw runtime_error("Error: Cannot open configuration file for reading.");
        }
     
    // Prep the grid
    for(unsigned int i=0;i<gridArea;i++)
        {
        grid[i]=false;
        }
        
    // Note that if current grid is larger than the original grid, there will be no error message as long as totalNumSpins matches
    for (unsigned int i = 0; i < totalNumSpins; i++) {
        
        if (!(configfile >> spinArray[i])) {
            throw runtime_error("Error: Invalid or insufficient data in configuration file.");
            }

        if (spinArray[i] < 0 || spinArray[i] >= static_cast<int>(gridArea)) {
            throw runtime_error("Error: spinArray[i] value is out of current grid bounds.");
            }

        grid[spinArray[i]] = true;
    }

    int extraCheck;
    if (configfile >> extraCheck) {
        throw runtime_error("Error: configuration contains more data than expected.");
        }
        
    // COMPUTE CORRELATIONS ------------------------------------------------------------------------------
    
    double correlation = 0;
    
    for(unsigned int i=0;i<gridArea;i++)
		{
        corrGrid[i]=0;
        for(unsigned int j=0;j<dNeighbourTable[i].size();j++)
			{
			if(grid[dNeighbourTable[i][j].first]==true)
				{corrGrid[i] += dNeighbourTable[i][j].second;}
			}
        if(grid[i]==true) correlation += corrGrid[i];
		}
		
    correlation = correlation/2.*probabilityNormFactor; // for correlation distance = d this will equal probability 
    
    // SAVE AND OUTPUT -----------------------------------------------------------------------------------
    
    cout << "The total normalized correlation function is: " << correlation << endl;
    
    //only outputting correlations at points that do have spins
    ofstream corrOutput("correlations.dat");
	for(int i=0;i<totalNumSpins;i++) corrOutput << corrGrid[spinArray[i]]*probabilityNormFactor << endl;
    
    // to plot in python
    
    // import numpy as np
    // import matplotlib.pyplot as plt
    // finconf = np.loadtxt("finconf.dat", dtype = int)
    // gridsize = 550
    // unraveled = np.unravel_index(finconf, (gridsize, gridsize))
    // grid = np.zeros((gridsize,gridsize))
    // data = np.loadtxt("correlations.dat")
    // grid[unraveled] = data
    // plt.imshow(grid, interpolation='None',cmap='Greens')
    // plt.show()

    
    
    return 0;
}
