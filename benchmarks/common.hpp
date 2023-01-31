#pragma once

#include <iostream>
#include <fstream>
#include <stdio.h>
#include <math.h>
#include <float.h>
#include <cmath>
#include <cstdlib>
#include <cassert>
#include <iomanip>
#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>
#include <random>
#include <string.h>
#include <stdlib.h>
#include <vector>
#include <time.h>
#include <bits/stdc++.h>
#include <valarray>

#define PI 3.14159265358979323846264338328 
#define EPS 1e-8

extern unsigned int totalNumSpins;
extern double cellsize;
extern unsigned int gridSize;
extern unsigned int gridArea;

bool isAround(double have, double comparewith);
double contributionEnergy(double have, double comparewith);
int xcoord(int gridPoint);
int ycoord(int gridPoint);
std::pair<double,double> findPosition(int gridPoint);
double euclideanDistance(std::pair<double,double> point1, std::pair<double,double> point2);
double euclideanDistance(std::pair<int,int> point1, std::pair<int,int> point2);
