#pragma once

#include <utility>

//grid geometry

int xcoord(int gridPoint);
int ycoord(int gridPoint);
std::pair<double,double> findPosition(int gridPoint);
double euclideanDistance(std::pair<double,double> point1, std::pair<double,double> point2);
double euclideanDistance(std::pair<int,int> point1, std::pair<int,int> point2);

//grasshopper interaction

bool isAround(double have, double comparewith);
double contributionEnergy(double have, double comparewith);
