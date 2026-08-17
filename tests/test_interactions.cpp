#include "doctest/doctest.h"
#include "interactions.hpp"
#include "utilities.hpp"

#include <cmath>
#include <utility>

TEST_CASE("coordinate conversion and physical positions") {
    totalNumSpins = 4;
    cellSize = 0.25;
    gridSize = 4;
    gridArea = 16;
    tempScaling = 1.0;
    deltaOption = 0;

    CHECK(xcoord(11) == 3);
    CHECK(ycoord(11) == 2);

    const auto position = findPosition(11);
    CHECK(position.first == doctest::Approx(0.75));
    CHECK(position.second == doctest::Approx(0.5));
}

TEST_CASE("Euclidean distance overloads use the configured cell size") {
    totalNumSpins = 4;
    cellSize = 0.25;
    gridSize = 4;
    gridArea = 16;
    tempScaling = 1.0;
    deltaOption = 0;

    CHECK(euclideanDistance(std::pair<double,double>{0.0, 0.0},
                            std::pair<double,double>{3.0, 4.0})
          == doctest::Approx(5.0));
    CHECK(euclideanDistance(std::pair<int,int>{0, 0},
                            std::pair<int,int>{3, 4})
          == doctest::Approx(1.25));
}

TEST_CASE("delta option zero has the expected compact support and weights") {
    totalNumSpins = 4;
    cellSize = 0.25;
    gridSize = 4;
    gridArea = 16;
    tempScaling = 1.0;
    deltaOption = 0;

    CHECK(isAround(0.0, 0.5));
    CHECK_FALSE(isAround(0.0, 0.500001));
    CHECK(contributionEnergy(0.0, 0.0) == doctest::Approx(0.5));
    CHECK(contributionEnergy(0.0, 0.25) == doctest::Approx(0.25));
    CHECK(std::abs(contributionEnergy(0.0, 0.5)) < 1e-15);
    CHECK(contributionEnergy(0.0, 0.500001) == 0.0);
    CHECK(contributionEnergy(0.25, 0.0) == contributionEnergy(0.0, 0.25));
}

TEST_CASE("delta option one evaluates both analytic branches") {
    totalNumSpins = 1;
    cellSize = 1.0;
    gridSize = 4;
    gridArea = 16;
    tempScaling = 1.0;
    deltaOption = 1;

    CHECK(contributionEnergy(0.0, 0.5)
          == doctest::Approx(0.46704998233983941).epsilon(1e-12));
    CHECK(contributionEnergy(0.0, 1.5)
          == doctest::Approx(0.03295001766016048).epsilon(1e-12));
    CHECK(contributionEnergy(0.0, 2.0) == 0.0);
    CHECK(contributionEnergy(1.5, 0.0) == contributionEnergy(0.0, 1.5));
}

TEST_CASE("interaction grid agrees with values from direct evaluation"){
    totalNumSpins = 4;
    cellSize = 0.25;
    gridSize = 20;
    gridArea = 400;
    tempScaling = 1.0;
    deltaOption = 0;

    double distance = 1; // 4 cells
    std::vector<unsigned char> grid(gridArea);
    for(unsigned int i=0;i<gridArea;i++)
        {
        grid[i]=false;
        }
    grid[210] = true; grid[214] = true; grid[290] = true; grid[0] = true;

    auto dNeighbourTable = buildInteractionTable(distance);
    auto interactionGrid = buildGrasshopperInteractionGrid(grid.data(), dNeighbourTable);

    double directEnergyContribution = 0;
    for(unsigned int i=0;i<gridArea;i++)
        {
        if(grid[i]) directEnergyContribution+=contributionEnergy(distance, euclideanDistance(findPosition(0),findPosition(i)));
        }
    CHECK(interactionGrid[0] == doctest::Approx(directEnergyContribution));  //occupied site without close d-neighbors

    directEnergyContribution = 0;
    for(unsigned int i=0;i<gridArea;i++)
        {
        if(grid[i]) directEnergyContribution+=contributionEnergy(distance, euclideanDistance(findPosition(210),findPosition(i)));
        }
    CHECK(interactionGrid[210] == doctest::Approx(directEnergyContribution)); //occupied site with close d-neighbors

    directEnergyContribution = 0;
    for(unsigned int i=0;i<gridArea;i++)
        {
        if(grid[i]) directEnergyContribution+=contributionEnergy(distance, euclideanDistance(findPosition(130),findPosition(i)));
        }
    CHECK(interactionGrid[130] == doctest::Approx(directEnergyContribution)); //unoccupied site with close d-neighbors

    //total energy
    int spinArray[] = {0,210,214,290};
    double energy = totalGrasshopperInteraction(grid.data(), interactionGrid);
    double directEnergy = 0;
	for(unsigned int i=0;i<totalNumSpins;i++)
		{
		for(unsigned int j=i+1;j<totalNumSpins;j++)
			{
			directEnergy+=contributionEnergy(euclideanDistance(findPosition(spinArray[i]),findPosition(spinArray[j])),distance);
			}
		}
    CHECK(energy == doctest::Approx(directEnergy));

	directEnergy = 0;
	for(unsigned int i=0;i<totalNumSpins;i++)
		{
		for(unsigned int j=0;j<dNeighbourTable[spinArray[i]].size();j++)
			{
			if(grid[dNeighbourTable[spinArray[i]][j].first]==true)
				{directEnergy+=dNeighbourTable[spinArray[i]][j].second;}
			}
		}
	CHECK(energy == doctest::Approx(directEnergy/2.));
}

TEST_CASE("nearest neighbor contribution"){
    totalNumSpins = 14;
    cellSize = 0.25;
    gridSize = 4;
    gridArea = 16;
    tempScaling = 1.0;
    deltaOption = 0;

    std::vector<unsigned char> grid(gridArea);
    for(unsigned int i=0;i<gridArea;i++)
        {
        grid[i]=true;
        }
    grid[5] = false; grid[15] = false;

    CHECK(nearestNeighborCount(0, grid.data()) == 2U);   //corner, all occupied
    CHECK(nearestNeighborCount(15, grid.data()) == 2U);  //corner, all occupied, self empty
    CHECK(nearestNeighborCount(13, grid.data()) == 3U);  //side, all occupied
    CHECK(nearestNeighborCount(5, grid.data()) == 4U);   //bulk, all occupied
    CHECK(nearestNeighborCount(6, grid.data()) == 3U);   //bulk, one empty

}


