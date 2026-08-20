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

    double averageLocalProbability = 0.0;
    for(unsigned int i=0; i<gridArea; i++)
        {
        if(grid[i])
            averageLocalProbability += normalizeGrasshopperInteraction(interactionGrid[i], distance);
        }

    averageLocalProbability /= totalNumSpins;

    CHECK(normalizeGrasshopperEnergy(energy, distance) == doctest::Approx(averageLocalProbability).epsilon(1e-12));
}

TEST_CASE("grasshopper energy difference agrees with full recomputation"){
    totalNumSpins = 4;
    cellSize = 0.25;
    gridSize = 20;
    gridArea = 400;
    tempScaling = 1.0;
    deltaOption = 0;

    const double distance = 1.0;

    std::vector<unsigned char> grid(gridArea, false);
    grid[0] = true;
    grid[210] = true;
    grid[214] = true;
    grid[290] = true;

    const auto interactionTable = buildInteractionTable(distance);
    const auto interactionGrid = buildGrasshopperInteractionGrid(grid.data(), interactionTable);

    const double energyBefore = totalGrasshopperInteraction(grid.data(), interactionGrid);

    const unsigned int oldCell = 214;
    const unsigned int newCell = 130;

    double energyDifference = interactionGrid[newCell] - interactionGrid[oldCell];

    const double moveDistance = euclideanDistance(findPosition(newCell), findPosition(oldCell));

    if(isAround(distance, moveDistance))
        energyDifference -= contributionEnergy(distance, moveDistance);

    auto movedGrid = grid;
    movedGrid[oldCell] = false;
    movedGrid[newCell] = true;

    const auto movedInteractionGrid = buildGrasshopperInteractionGrid(movedGrid.data(), interactionTable);

    const double energyAfter = totalGrasshopperInteraction(movedGrid.data(), movedInteractionGrid);

    CHECK(energyAfter == doctest::Approx(energyBefore + energyDifference).epsilon(1e-12));
}

TEST_CASE("nearest neighbor contribution"){
    totalNumSpins = 14;
    cellSize = 0.25;
    gridSize = 4;
    gridArea = 16;
    tempScaling = 1.0;
    deltaOption = 0;

    std::vector<unsigned char> grid(gridArea, true);
    grid[5] = false; grid[15] = false;

    CHECK(nearestNeighborCount(0, grid.data()) == 2U);   //corner, all occupied
    CHECK(nearestNeighborCount(15, grid.data()) == 2U);  //corner, all occupied, self empty
    CHECK(nearestNeighborCount(13, grid.data()) == 3U);  //side, all occupied
    CHECK(nearestNeighborCount(5, grid.data()) == 4U);   //bulk, all occupied
    CHECK(nearestNeighborCount(6, grid.data()) == 3U);   //bulk, one empty
    CHECK(normalizeNearestNeighborCount(0) == 0.0);
    CHECK(normalizeNearestNeighborCount(2) == 0.5);
    CHECK(normalizeNearestNeighborCount(4) == 1.0);

    CHECK(areNearestNeighbors(0,1) == true);
    CHECK(areNearestNeighbors(1,0) == true);
    CHECK(areNearestNeighbors(0,2) == false);
    CHECK(areNearestNeighbors(3,4) == false);
    CHECK(areNearestNeighbors(3,7) == true);
    CHECK(areNearestNeighbors(7,3) == true);
    CHECK(areNearestNeighbors(3,3) == false);

    CHECK(nearestNeighborBondDifference(13, 5, grid.data()) == 1);
    CHECK(nearestNeighborBondDifference(12, 5, grid.data()) == 2);
    CHECK(nearestNeighborBondDifference(10, 5, grid.data()) == 0);
    CHECK(nearestNeighborBondDifference(0, 15, grid.data()) == 0);
    CHECK(nearestNeighborBondDifference(10, 15, grid.data()) == -2);
    CHECK(nearestNeighborBondDifference(6, 5, grid.data()) == 0);
    CHECK(nearestNeighborBondDifference(11, 15, grid.data()) == -1);

}

TEST_CASE("nearest neighbor bond difference agrees with full recount"){
    totalNumSpins = 14;
    cellSize = 0.25;
    gridSize = 4;
    gridArea = 16;
    tempScaling = 1.0;
    deltaOption = 0;

    std::vector<unsigned char> grid(gridArea, true);
    grid[5] = false; grid[15] = false;

    auto countBonds = [](const std::vector<unsigned char>& configuration)
        {
        long long bonds = 0;

        for(unsigned int cell=0; cell<gridArea; cell++)
            {
            if(!configuration[cell]) continue;

            // right
            if(cell%gridSize != gridSize-1 && configuration[cell+1])
                bonds++;
            // up
            if(cell+gridSize < gridArea && configuration[cell+gridSize])
                bonds++;
            }

        return bonds;
        };

    const long long bondsBefore = countBonds(grid);

    for(unsigned int oldCell=0; oldCell<gridArea; oldCell++)
        {
        if(!grid[oldCell]) continue;

        for(unsigned int newCell=0; newCell<gridArea; newCell++)
            {
            if(grid[newCell]) continue;

            const int predictedDifference = nearestNeighborBondDifference(oldCell, newCell, grid.data());

            auto movedGrid = grid;
            movedGrid[oldCell] = false;
            movedGrid[newCell] = true;

            CHECK(countBonds(movedGrid) - bondsBefore == predictedDifference);
            }
        }
}

