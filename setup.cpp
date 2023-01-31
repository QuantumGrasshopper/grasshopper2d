#include "setup.hpp"

using namespace std;
	
void initLoad(bool grid[], int spinArray[])
		{
		ifstream initconfin("initconf.dat");
		for(unsigned int i=0;i<gridArea;i++)
			{
			grid[i]=false;
			}
		for(unsigned int i=0;i<totalNumSpins;i++) 
			{
			initconfin >> spinArray[i];
			grid[spinArray[i]]=true;
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
		
		
void saveConfig(int *spinArray, ofstream& filename)
    {
    for(unsigned int i=0;i<totalNumSpins;i++) filename << spinArray[i] << endl;
    }
    
    
void initialize(bool grid[], int spinArray[], gsl_rng* RNG, string initconf)
{
	if(initconf=="random") initRandom(grid, spinArray, RNG);
    else if(initconf=="load") initLoad(grid, spinArray);
    else throw logic_error("Invalid initialization.");
        
	if(!(initconf=="load"))
		{
		ofstream initconfout("initconf.dat");
		saveConfig(spinArray, initconfout);
		}
}
