// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Olga Goulko

#pragma once

#include <gsl/gsl_rng.h>

double temperatureDecrease(double temperature);                                 //simulated annealing cooling schedule
unsigned long stepIncrease(unsigned long temproundsteps);                       //increase number of MC steps between cooling rounds
double energyDecreaseProbDistr(double energyDifference, double temperature);    //Metropolis probability exp(Delta E/T) for unfavorable (Delta E < 0) moves
bool acceptreject(double probability, gsl_rng* RNG);
