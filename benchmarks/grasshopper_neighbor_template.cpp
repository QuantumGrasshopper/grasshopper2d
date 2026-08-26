// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Olga Goulko

#include "../interactions.hpp"
#include "../utilities.hpp"

#include <chrono>
#include <cmath>
#include <iostream>
#include <utility>
#include <vector>

#include <gsl/gsl_rng.h>

// Benchmarking code for the grasshopper
// Setup: square grid, default delta function discretization, random config 
//        fixed number of MC updates, accept only energy-increasing moves
//        compare MC time after neighborslist initiated
//        also note time to create the neighborslist

// Instead of having a neighborslist for each point we only have a generic template (center point)
// For a specific neighborslist we translate the neighbors template in real time

using namespace std;

unsigned int totalNumSpins = 10000;
double cellSize = 1./sqrt(double(totalNumSpins));
unsigned int gridSize = 200;
unsigned int gridArea = gridSize*gridSize;
int deltaOption = 0;

struct RelativeNeighbor {
    int dx;
    int dy;
    double contribution;
};

int main() {
    
    // test parameters
    double d = 0.3;
    long unsigned int steps = 1000000;
    long unsigned int acceptance_counter=0;
    const int signedGridSize = static_cast<int>(gridSize);
    std::vector<unsigned char> grid(gridArea);          //true if spin=1 at this grid point

    // RNG
	auto seed=12345;
	gsl_rng * RNG = gsl_rng_alloc (gsl_rng_mt19937);
	gsl_rng_set (RNG, seed);

    // construct generic neighbor list like in interactions.cpp
    auto begin = chrono::high_resolution_clock::now();

    vector< RelativeNeighbor > dNeighbourTemplate;
    
    int center = (gridSize/2) * gridSize + gridSize/2;
    double thisEnergyContribution;
    pair<double,double> currentPosition=findPosition(center);
    for(unsigned int j=0;j<gridArea;j++)
        {
        thisEnergyContribution=contributionEnergy(d,euclideanDistance(currentPosition,findPosition(j)));
        if(thisEnergyContribution > EPS) dNeighbourTemplate.push_back(RelativeNeighbor{
                    xcoord(static_cast<int>(j)) - signedGridSize / 2,
                    ycoord(static_cast<int>(j)) - signedGridSize / 2,
                    thisEnergyContribution
                });
        }
    
    auto now = chrono::high_resolution_clock::now();
	cout << "Time to construct neighbors list: " <<chrono::duration_cast<chrono::milliseconds>(now-begin).count() << "ms" << endl;

    // initialize random grid explicitly
    std::vector<int> spinArray(totalNumSpins);          //grid point where any spin is
	std::vector<int> noSpinArray(gridArea-totalNumSpins); //complementary to above: grid point where no spin is
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
    spincounter=0;
	for(unsigned int i=0;i<gridArea;i++)
		{
		if(grid[i]==false) {noSpinArray[spincounter]=i; spincounter++;}
		}

    // perform a set of X regular MC updates, let's say we only accept improvements
    now = chrono::high_resolution_clock::now();

    for(unsigned int counter=0;counter<steps;counter++)
        {
        //select random spins to destroy and to create
		auto destroy=gsl_rng_uniform_int (RNG, totalNumSpins);
		unsigned int oldSpinCoord=spinArray[destroy];
		auto create=gsl_rng_uniform_int (RNG, gridArea-totalNumSpins);
		unsigned int newSpinCoord=noSpinArray[create];
        double energyDifference = 0;
		
		//calculate energy difference by shifting the template
        for(unsigned int j=0;j<dNeighbourTemplate.size();j++)
            {
            auto neighbor = dNeighbourTemplate[j];

            double relativeEnergy = neighbor.contribution;
            int gridLocationx = xcoord(newSpinCoord) + neighbor.dx;
            int gridLocationy = ycoord(newSpinCoord) + neighbor.dy;
            int gridCoord = gridLocationx + gridLocationy*gridSize;
            if(gridLocationx >= 0 && gridLocationy >= 0 && gridLocationx < signedGridSize && gridLocationy < signedGridSize && grid[gridCoord]==true && gridCoord != oldSpinCoord)
                {
                energyDifference+=relativeEnergy;
                }
            gridLocationx = xcoord(oldSpinCoord) + neighbor.dx;
            gridLocationy = ycoord(oldSpinCoord) + neighbor.dy;
            gridCoord = gridLocationx + gridLocationy*gridSize;
            if(gridLocationx >= 0 && gridLocationy >= 0 && gridLocationx < signedGridSize && gridLocationy < signedGridSize && grid[gridCoord]==true)
                {
                energyDifference-=relativeEnergy;
                }
            }

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

    gsl_rng_free(RNG);
    
    return 0;
}
