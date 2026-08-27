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

// The neighbors grid is initialized by translating the center point neighbors list

using namespace std;

unsigned int totalNumSpins = 10000;
double cellSize = 1./sqrt(double(totalNumSpins));
unsigned int gridSize = 200;
unsigned int gridArea = gridSize*gridSize;
int deltaOption = 0;

int main() {
    
    // test parameters
    double d = 0.3;
    long unsigned int steps = 1000000;
    long unsigned int acceptance_counter=0;
    std::vector<unsigned char> grid(gridArea);          //true if spin=1 at this grid point
    
    // RNG
	auto seed=12345;
	gsl_rng * RNG = gsl_rng_alloc (gsl_rng_mt19937);
	gsl_rng_set (RNG, seed);

    // construct neighbor table using full routine from interactions.cpp
    auto begin = chrono::high_resolution_clock::now();

    auto dNeighbourTable = buildInteractionTable(d);

    auto now = chrono::high_resolution_clock::now();

    cout << "Time to construct neighbors table: " <<chrono::duration_cast<chrono::milliseconds>(now-begin).count() << "ms" << endl;
    
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
