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
    std::optional<unsigned long> temperatureRoundSteps;
    double initialTemperature = 20.0;
    double finalTemperature = 0.01;
    int annealingSteps = 1000;
    int configurationOutputs = 0;
    std::string initialConfiguration = "random";
    int deltaOption = 0;
    double nearestNeighborInteraction = 0.0;
    std::optional<unsigned long> randomSeed;
    bool overwriteExistingOutputs = false;
};

SimulationParameters parseSimulationParameters(int argc, char* argv[]);
