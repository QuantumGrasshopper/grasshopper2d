// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Olga Goulko

#pragma once

#include <optional>
#include <string>

struct SimulationParameters {
    explicit SimulationParameters(double requiredHoppingDistance)
        : hoppingDistance(requiredHoppingDistance) {}

    double hoppingDistance;
    unsigned int totalNumSpins = 10000;
    std::optional<unsigned int> gridSize;
    double hours = 0.0;
    unsigned long maxSteps = 1000000000000UL;
    std::optional<unsigned long> temperatureRoundSteps;     //initial number of MC proposals between cooling updates
    double initialTemperature = 20.0;
    double finalTemperature = 0.01;
    int annealingSteps = 1000;                              //number of annealing steps between initial and final temperature
    int configurationOutputs = 0;
    std::string initialConfiguration = "random";
    int deltaOption = 0;                                    //selects smeared delta-function discretization (currently two options implemented)
    double nearestNeighborInteraction = 0.0;                //positive values favor occupied nearest-neighbor bonds
    std::optional<unsigned long> randomSeed;
    bool overwriteExistingOutputs = false;
};

SimulationParameters parseSimulationParameters(int argc, char* argv[]);
