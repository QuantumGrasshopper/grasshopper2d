#include "setup.hpp"

using namespace std;
	
void initLoad(bool grid[], int spinArray[]) {
    
    // Load initial configuration file
    ifstream initconfin("initconf.dat");
    if (!initconfin.is_open()) {
        throw runtime_error("Error: Cannot open initconf.dat for reading.");
        }
     
    // Prep the grid
    for(unsigned int i=0;i<gridArea;i++)
        {
        grid[i]=false;
        }
        
    // Read in data checking that totalNumSpins and gridSize match
    // Note that if current grid is larger than the original grid, there will be no error message as long as totalNumSpins matches
    for (unsigned int i = 0; i < totalNumSpins; i++) {
        
        if (!(initconfin >> spinArray[i])) {
            throw runtime_error("Error: Invalid or insufficient data in initconf.dat.");
            }

        if (spinArray[i] < 0 || spinArray[i] >= static_cast<int>(gridArea)) {
            throw runtime_error("Error: spinArray[i] value is out of current grid bounds.");
            }

        grid[spinArray[i]] = true;
    }

    // Check if the configuration file has extra data
    int extraCheck;
    if (initconfin >> extraCheck) {
        throw runtime_error("Error: initconf.dat contains more data than expected.");
        }

    }
		

void initRandom(bool grid[], int spinArray[], gsl_rng* RNG)
	{
	for(unsigned int i=0;i<gridArea;i++)
		{
		grid[i]=false;
		}
	int newSpinCoord; 
    unsigned int spincounter=0;
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
	}
		
void initDisk(bool grid[], int spinArray[]) {
	
    // unless the number of spins matches perfectly the number required to shape the full disk 
    // the disk will not have complete shells
    // this is a small additional systematic error, but it will not substantially affect the end result
    
    double radius=1./sqrt(PI);
    pair<double,double> center=findPosition(gridSize*gridSize/2+gridSize/2);
	unsigned int spincounter=0;
    
    // fill inner part of the disk
    for(unsigned int i=0;i<gridArea;i++)
        {
        grid[i]=false;
        if(spincounter<totalNumSpins)
            {
            pair<double,double> thisPoint=findPosition(i);
            if(euclideanDistance(center,thisPoint)<radius)
                {
                grid[i]=true; 
                spinArray[spincounter]=i;
                spincounter++;
                }
            }
        }
        
    // put remaining spins (if any) into outside layer
    for(unsigned int i=0;i<gridArea;i++)
        {
        if( (spincounter<totalNumSpins) && (grid[i]==false) )
            {
            pair<double,double> thisPoint=findPosition(i);
            if(euclideanDistance(center,thisPoint)<radius+cellSize)
                {
                grid[i]=true; 
                spinArray[spincounter]=i;
                spincounter++;
                }
            }
        }
     
    if (spincounter != totalNumSpins)
        throw runtime_error("Error: Incorrect number of spins in disk initialization.");
    
}

void saveConfig(int *spinArray, const string& filename) {
    ofstream file;
    file.open(filename);
    if (!file.is_open()) {
        throw runtime_error("Error: Cannot open " + filename + " for writing.");
        }
    
    ostringstream buffer;
    for (unsigned int i = 0; i < totalNumSpins; i++) buffer << spinArray[i] << '\n';
    file << buffer.str();
}
    
    
void initialize(bool grid[], int spinArray[], gsl_rng* RNG, string initconf) {    
    try {
        if (initconf == "random") initRandom(grid, spinArray, RNG);
        else if (initconf == "load") initLoad(grid, spinArray);
        else if (initconf == "disk") initDisk(grid, spinArray);
        else throw logic_error("Warning: Initialization not specified or invalid.");
        }
    catch (const logic_error& e) {
        cerr << e.what() << " - Proceeding with random initialization." << endl;
        initRandom(grid, spinArray, RNG);
        }
        
	if(!(initconf=="load"))
		{
		saveConfig(spinArray, "initconf.dat");
		}
}
