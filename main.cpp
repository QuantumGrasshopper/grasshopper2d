#include "utilities.hpp"
#include "setup.hpp"
#include "annealing.hpp"

unsigned int totalNumSpins;
double cellSize;
unsigned int gridSize;
unsigned int gridArea;
double tempScaling;
int deltaOption;

using namespace std; 

int main(int inputN,char *inputV[]) {
    
    // SETUP -------------------------------------------------------------------------------------
    
    ofstream result("result.dat");
	
	double d=get_option(inputN,inputV,"d");					//grasshopper hopping distance
	double maxtime=get_option(inputN,inputV,"hours");
	long unsigned int maxsteps=get_option(inputN,inputV,"steps");
	long unsigned int temproundsteps=get_option(inputN,inputV,"tempsteps");
	double temperature=get_option(inputN,inputV,"inittemp");
	double finaltemperature=get_option(inputN,inputV,"fintemp");
	int numberannealingsteps=get_option(inputN,inputV,"annealsteps");
	totalNumSpins=get_option(inputN,inputV,"N");
	gridSize=get_option(inputN,inputV,"gridsize");
	long unsigned int seed=get_option(inputN,inputV,"randomseed");
	string initconf=get_string_option(inputN,inputV,"initconf");
    deltaOption=get_option(inputN,inputV,"delta");
    bool configOutputs = get_option(inputN,inputV,"configoutput");
    
    double NNint = get_option(inputN,inputV,"NNint");
	
	maxtime=60*60*maxtime*1000;
	if(totalNumSpins<100) totalNumSpins=5000;
	if(maxsteps==0) maxsteps=1e12;
	if(temproundsteps>maxsteps) temproundsteps=int(maxsteps/1000.);
	if(temproundsteps<10) temproundsteps=totalNumSpins;
	if(temperature<EPS) temperature=25.;
	if(finaltemperature<EPS) finaltemperature=0.1;
	if(numberannealingsteps<EPS) numberannealingsteps=1000;
	tempScaling=pow((finaltemperature/temperature),1./double(numberannealingsteps));
	int outputconfigbeforetherm=numberannealingsteps/100; int annealingcounter=0; int maxoutputconfigs=200;	//for output config dat
	
	cellSize=1./sqrt(double(totalNumSpins));
	if( (gridSize<10) || (totalNumSpins > gridSize*gridSize) ) gridSize=2*int(sqrt(double(totalNumSpins))+EPS)+2*int(3*d/cellSize+EPS);
	gridArea = gridSize*gridSize;
    
    // one factor of 1/2 is already taken care of by avoiding double counting
    double probabilityNormFactor = 1/PI/d/pow(double(totalNumSpins),3./2.);
    
	gsl_rng * RNG = gsl_rng_alloc (gsl_rng_mt19937);
    if(seed==0) seed=std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	gsl_rng_set (RNG, seed);
	
	unsigned int temproundcounter=0;
	double accratio;
    long unsigned int counter=0; long accepted=0; long accepted_current=0;

    result << "2D Grasshopper with Simulated Annealing, Euclidean metric" << endl;
	result << endl;
	result << "Total number of spins: " << totalNumSpins << endl;
	result << "Hopping distance: " << d << endl;
    result << "Nearest Neighbor interaction coefficient: " << NNint << endl;
	result << "Size of grid: " << gridSize << endl;
	result << "Size of cell: " << cellSize << endl;
	result << endl;
	result << "Random seed: " << seed << endl;
    result << "Option for delta-function discretization: " << deltaOption << endl;
	result << "Initial temperature: " << temperature << endl;
	result << "Final temperature: " << finaltemperature << endl;
	result << "Temperature scaling factor: " << tempScaling << endl;
	result << "Number of annealing steps: " << numberannealingsteps << endl;
	result << "Initial number of steps before temperature scaling: " << temproundsteps << endl;
	result << endl;

    auto begin = chrono::high_resolution_clock::now();
    
    // CONSTRUCT NEIGHBOR LIST ---------------------------------------------------------------------------  
    
    vector< pair<int,double> > dNeighbourTemplate;
	vector< pair<int,double> > dNeighbourTable[gridArea];	//for each grid point: list of points that are its d-neighbours with corresponding energies
    
    int center = gridSize*gridSize/2+gridSize/2;
    double thisEnergyContribution;
    pair<double,double> currentPosition=findPosition(center);
    for(unsigned int j=0;j<gridArea;j++)
        {
        thisEnergyContribution=contributionEnergy(d,euclideanDistance(currentPosition,findPosition(j)));
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
		
    auto now = chrono::high_resolution_clock::now();
    auto timeDiff = chrono::duration_cast<chrono::milliseconds>(now-begin).count();
	result << "Time to construct neighbors list: " << timeDiff << "ms" << endl;
    
    // INITIAL SPIN CONFIGURATION ------------------------------------------------------------------------
    
    bool grid[gridArea];					     //true if spin=1 at this grid point
    double energyGrid[gridArea];
	int spinArray[totalNumSpins];				 //grid point where any spin is
	int noSpinArray[gridArea-totalNumSpins];	 //complementary to above: grid point where no spin is
    
    initialize(grid, spinArray, RNG, initconf);
    
    unsigned int noSpinCounter=0;
    double energy = 0;
    double NNenergy = 0;
	for(unsigned int i=0;i<gridArea;i++)
		{
		if(grid[i]==false) {noSpinArray[noSpinCounter]=i; noSpinCounter++;}
		// NN contributions
		else {
            //down
            if(i>=gridSize) NNenergy += grid[i-gridSize];
            //up
            if(i<gridSize*(gridSize-1)) NNenergy += grid[i+gridSize];
            
            int gridLocationx = xcoord(i);
            //left
            if(gridLocationx != 0) NNenergy += grid[i-1];
            //right
            if(gridLocationx != gridSize-1) NNenergy += grid[i+1];
            }
        // Fill the energy grid
        energyGrid[i]=0;
        for(unsigned int j=0;j<dNeighbourTable[i].size();j++)
			{
			if(grid[dNeighbourTable[i][j].first]==true)
				{energyGrid[i] += dNeighbourTable[i][j].second;}
			}
        if(grid[i]==true) energy += energyGrid[i];
		}
    energy = energy/2.;
    NNenergy = NNenergy*NNint/2.;
    energy += NNenergy;
		
    ofstream energies("energies.dat");
	ofstream temperatures("temperatures.dat");
	energies << energy*probabilityNormFactor << endl;
	ofstream configuration("config.dat");
	if(configOutputs)
        {
        for(unsigned int i=0;i<totalNumSpins;i++) configuration << spinArray[i] << " ";
        configuration << energy*probabilityNormFactor << endl;
        }
    
	int bestSpinArray[totalNumSpins];	//the overall best spin array during the whole run
	for(unsigned int i=0;i<totalNumSpins;i++) bestSpinArray[i]=spinArray[i];
	double bestenergy=energy;
		
    // MAIN LOOP ------------------------------------------------------------------------------------------
		
    while( (timeDiff<maxtime) && (counter<maxsteps) )
        {
		counter++; temproundcounter++;
		
        // MC update
		int destroy=gsl_rng_uniform_int (RNG, totalNumSpins);
		int oldSpinCoord=spinArray[destroy];
		int create=gsl_rng_uniform_int (RNG, gridArea-totalNumSpins);
		int newSpinCoord=noSpinArray[create];
		
		double energyDifference=energyGrid[newSpinCoord]-energyGrid[oldSpinCoord];
        if(isAround(d,euclideanDistance(findPosition(newSpinCoord),findPosition(oldSpinCoord))))//NOTE more efficient to keep this check explicit
            {
            energyDifference -= contributionEnergy(d,euclideanDistance( findPosition(newSpinCoord),findPosition(oldSpinCoord) ));
            }
        // Nearest neighbor contributions
        //down
        if(newSpinCoord>=gridSize && newSpinCoord-gridSize != oldSpinCoord) energyDifference += NNint*grid[newSpinCoord-gridSize];
        if(oldSpinCoord>=gridSize && oldSpinCoord-gridSize != newSpinCoord) energyDifference -= NNint*grid[oldSpinCoord-gridSize];
        //up
        if(newSpinCoord<gridSize*(gridSize-1) && newSpinCoord+gridSize != oldSpinCoord) energyDifference += NNint*grid[newSpinCoord+gridSize];
        if(oldSpinCoord<gridSize*(gridSize-1) && oldSpinCoord+gridSize != newSpinCoord) energyDifference -= NNint*grid[oldSpinCoord+gridSize];
            
        int gridLocationx = xcoord(newSpinCoord);
        //left
        if(gridLocationx != 0 && newSpinCoord-1 != oldSpinCoord) energyDifference += NNint*grid[newSpinCoord-1];
        //right
        if(gridLocationx != gridSize-1 && newSpinCoord+1 != oldSpinCoord) energyDifference += NNint*grid[newSpinCoord+1];
        gridLocationx = xcoord(oldSpinCoord);
        //left
        if(gridLocationx != 0 && oldSpinCoord-1 != newSpinCoord) energyDifference -= NNint*grid[oldSpinCoord-1];
        //right
        if(gridLocationx != gridSize-1 && oldSpinCoord+1 != newSpinCoord) energyDifference -= NNint*grid[oldSpinCoord+1];

		bool accept;
		if(energyDifference>=0) accept=true;
		else accept=acceptreject(energyDecreaseProbDistr(energyDifference,temperature),RNG);
		
		if(accept==true)
			{
			grid[oldSpinCoord]=false; grid[newSpinCoord]=true;
			spinArray[destroy]=newSpinCoord; noSpinArray[create]=oldSpinCoord;
			energy+=energyDifference;
            //update energy grid
            for(unsigned int j=0;j<dNeighbourTable[oldSpinCoord].size();j++)
                {
                energyGrid[dNeighbourTable[oldSpinCoord][j].first] -= dNeighbourTable[oldSpinCoord][j].second;
                }
            for(unsigned int j=0;j<dNeighbourTable[newSpinCoord].size();j++)
                {
				energyGrid[dNeighbourTable[newSpinCoord][j].first] += dNeighbourTable[newSpinCoord][j].second;
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
				if(annealingcounter%outputconfigbeforetherm==0 && configOutputs) 
					{
                    for(unsigned int i=0;i<totalNumSpins;i++) configuration << spinArray[i] << " "; 
                    configuration << energy*probabilityNormFactor << endl;
                    }
				annealingcounter++;
				}
			else if(annealingcounter<maxoutputconfigs)
				{
				annealingcounter++; 
                if(configOutputs)
                    {
                    for(unsigned int i=0;i<totalNumSpins;i++) configuration << spinArray[i] << " "; 
                    configuration << energy*probabilityNormFactor << endl;
                    }
				}
			accratio=accepted_current/double(temproundcounter);
			temperatures << counter << '\t' << temperature << '\t' << accratio << endl;
			energies << energy*probabilityNormFactor << endl;
			accepted+=accepted_current; accepted_current=0;
			temproundcounter=0;
			temproundsteps=stepIncrease(temproundsteps);
			}
		
		now = chrono::high_resolution_clock::now();
		timeDiff = chrono::duration_cast<chrono::milliseconds>(now-begin).count();
		}
		
    // WRAP UP --------------------------------------------------------------------------------------------
    
    result << endl;
	result << "Simulation took " << timeDiff/60./1000 << " minutes" << endl;
	result << "Finished after " << counter << " steps" << endl;
	result << "Final temperature: " << temperature << endl;
	result << "Average acceptance ratio: " << accepted/double(counter) << endl;
	result << endl;
	result << "final energy: " << energy << endl;
	result << "best energy: " << bestenergy << endl;
    result << "final probability: " << energy*probabilityNormFactor << endl;
    result << "best probability: " << bestenergy*probabilityNormFactor << endl;
	result << endl;
    
    if(!configOutputs) {remove("config.dat");}
    ofstream finconf("finconf.dat");
    saveConfig(spinArray, finconf);
	ofstream bestconf("bestconf.dat");
    saveConfig(bestSpinArray, bestconf);
    
    return 0;
}
