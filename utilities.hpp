#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
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
#include <stdexcept>
#include <bits/stdc++.h>

#define PI 3.14159265358979323846264338328 
#define EPS 1e-8

// global simulation parameters

extern unsigned int totalNumSpins;
extern double cellSize;
extern unsigned int gridSize;
extern unsigned int gridArea;
extern double tempScaling;
extern int deltaOption;

// extern std::set<std::string> validOptions = {"-d", 
//                                              "-N", 
//                                              "-gridsize", 
//                                              "-hours", 
//                                              "-steps", 
//                                              "-tempsteps",
//                                              "-inittemp", 
//                                              "-fintemp",
//                                              "-annealsteps",
//                                              "-configoutput",
//                                              "-initconf", 
//                                              "-delta", 
//                                              "-NNint",
//                                              "-randomseed"    
//                                              };

// common functions

bool isAround(double have, double comparewith);
double contributionEnergy(double have, double comparewith);
int xcoord(int gridPoint);
int ycoord(int gridPoint);
std::pair<double,double> findPosition(int gridPoint);
double euclideanDistance(std::pair<double,double> point1, std::pair<double,double> point2);
double euclideanDistance(std::pair<int,int> point1, std::pair<int,int> point2);

// I/O routines

template<typename T>
T get_option(int inputN, char *inputV[], const char *was)
    {
    char option[20];
    sprintf(option, "-%s", was);
    for (int n = 1; n < (inputN - 1); n++)
        {
        if (strcmp(inputV[n], option) == 0)
            {
            const char* value = inputV[n + 1];
            // Use double for all numerical types (double, int, bool, etc)
            if constexpr (std::is_same_v<T, std::string>) return std::string(value);
            else return static_cast<T>(std::strtod(value, nullptr));  // Convert directly to double, then cast to T
            }
        }
    // Default values
    if constexpr (std::is_same_v<T, std::string>) return "";
    else return static_cast<T>(0.0);
}

class BufferedFileWriter {
private:
    std::ofstream file;
    std::vector<std::string> buffer;
    size_t bufferLimit;
    std::chrono::steady_clock::time_point lastFlushTime;
    std::chrono::milliseconds flushInterval;

public:
    BufferedFileWriter(const std::string& filename, size_t limit, std::chrono::milliseconds interval);
    void write(const std::string& data);
    void flush();
    ~BufferedFileWriter();
};
