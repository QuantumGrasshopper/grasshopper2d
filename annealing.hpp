#pragma once

#include <gsl/gsl_rng.h>

double temperatureDecrease(double temperature);
unsigned long stepIncrease(unsigned long temproundsteps);
double energyDecreaseProbDistr(double energyDifference, double temperature);
bool acceptreject(double probability, gsl_rng* RNG);
