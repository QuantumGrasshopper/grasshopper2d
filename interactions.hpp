#pragma once

#include <utility>
#include <vector>

//grid geometry

int xcoord(int gridPoint);
int ycoord(int gridPoint);
std::pair<double,double> findPosition(int gridPoint);
double euclideanDistance(std::pair<double,double> point1, std::pair<double,double> point2);
double euclideanDistance(std::pair<int,int> point1, std::pair<int,int> point2);

//grasshopper interaction

bool isAround(double have, double comparewith);
double contributionEnergy(double have, double comparewith);

//interaction fields

void validateInteractionTableReach(double distance);
using GrasshopperInteractionTable = std::vector<std::vector<std::pair<int,double>>>;
GrasshopperInteractionTable buildInteractionTable(double distance);
std::vector<double> buildGrasshopperInteractionGrid(const unsigned char grid[], const GrasshopperInteractionTable& table);
double totalGrasshopperInteraction(const unsigned char grid[], const std::vector<double>& interactionGrid);

//nearest-neighbor contributions
unsigned int nearestNeighborCount(const unsigned int cell, const unsigned char grid[]);
