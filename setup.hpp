#pragma once

#include <string>

#include <gsl/gsl_rng.h>

void initLoad(unsigned char grid[], int spinArray[]);
void initRandom(unsigned char grid[], int spinArray[], gsl_rng* RNG);
void initDisk(unsigned char grid[], int spinArray[]);
void saveConfig(int *spinArray, const std::string& filename);
void initialize(unsigned char grid[], int spinArray[], gsl_rng* RNG, std::string initconf);
