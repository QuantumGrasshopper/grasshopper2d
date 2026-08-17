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

// interaction fields

using GrasshopperInteractionTable = std::vector<std::vector<std::pair<int,double>>>;
GrasshopperInteractionTable buildInteractionTable(double distance);   //what currently constructs dNeighbourTemplate and dNeighbourTable
std::vector<double> buildGrasshopperInteractionGrid(const unsigned char grid[], const GrasshopperInteractionTable& table);  //Q_i(r), what is currently energyGrid
double totalGrasshopperInteraction(const unsigned char grid[], const std::vector<double>& interactionGrid);
