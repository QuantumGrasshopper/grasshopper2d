#include "simulation_parameters.hpp"

#include <cmath>
#include <limits>
#include <set>
#include <stdexcept>
#include <string>

namespace {

const std::set<std::string> recognizedOptions{
    "-d",
    "-N",
    "-gridsize",
    "-hours",
    "-steps",
    "-tempsteps",
    "-inittemp",
    "-fintemp",
    "-annealsteps",
    "-configoutput",
    "-initconf",
    "-delta",
    "-NNint",
    "-randomseed"
};

bool isRecognizedOption(const std::string& text) {
    return recognizedOptions.count(text) != 0;
}

double parseFiniteDouble(const std::string& text, const std::string& option) {
    std::size_t parsed = 0;
    double value = 0.0;

    try {
        value = std::stod(text, &parsed);
    }
    catch (const std::exception&) {
        throw std::invalid_argument("Invalid value for " + option + ": " + text);
    }

    if (parsed != text.size() || !std::isfinite(value)) {
        throw std::invalid_argument("Invalid value for " + option + ": " + text);
    }
    return value;
}

template<typename T>
T parseUnsignedInteger(const std::string& text, const std::string& option) {
    if (text.empty()
        || text.find_first_not_of("0123456789") != std::string::npos) {
        throw std::invalid_argument("Invalid value for " + option + ": " + text);
    }

    unsigned long long value = 0;
    try {
        std::size_t parsed = 0;
        value = std::stoull(text, &parsed, 10);
        if (parsed != text.size()) {
            throw std::invalid_argument("Invalid value for " + option + ": " + text);
        }
    }
    catch (const std::exception&) {
        throw std::invalid_argument("Invalid value for " + option + ": " + text);
    }

    if (value > static_cast<unsigned long long>(std::numeric_limits<T>::max())) {
        throw std::invalid_argument("Value for " + option
                                    + " is outside the destination type range: " + text);
    }
    return static_cast<T>(value);
}

int parseNonnegativeInt(const std::string& text, const std::string& option) {
    return parseUnsignedInteger<int>(text, option);
}

int parsePositiveInt(const std::string& text, const std::string& option) {
    const int value = parseNonnegativeInt(text, option);
    if (value == 0) {
        throw std::invalid_argument(option + " must be a positive integer");
    }
    return value;
}

} // namespace

SimulationParameters parseSimulationParameters(int argc, char* argv[]) {
    SimulationParameters parameters(0.0);
    std::set<std::string> suppliedOptions;
    bool hoppingDistanceSupplied = false;

    for (int argument = 1; argument < argc; argument += 2) {
        const std::string option = argv[argument];
        if (!isRecognizedOption(option)) {
            throw std::invalid_argument("Unknown option: " + option);
        }
        if (!suppliedOptions.insert(option).second) {
            throw std::invalid_argument("Duplicate option: " + option);
        }
        if (argument + 1 >= argc || isRecognizedOption(argv[argument + 1])) {
            throw std::invalid_argument("Missing value for option " + option);
        }

        const std::string value = argv[argument + 1];

        if (option == "-d") {
            parameters.hoppingDistance = parseFiniteDouble(value, option);
            if (parameters.hoppingDistance <= 0.0) {
                throw std::invalid_argument("-d must be finite and greater than zero");
            }
            hoppingDistanceSupplied = true;
        }
        else if (option == "-N") {
            parameters.totalNumSpins = parseUnsignedInteger<unsigned int>(value, option);
            if (parameters.totalNumSpins == 0) {
                throw std::invalid_argument("-N must be a positive integer");
            }
        }
        else if (option == "-gridsize") {
            const unsigned int parsedGridSize =
                parseUnsignedInteger<unsigned int>(value, option);
            if (parsedGridSize == 0) {
                throw std::invalid_argument("-gridsize must be a positive integer");
            }
            parameters.gridSize = parsedGridSize;
        }
        else if (option == "-hours") {
            parameters.hours = parseFiniteDouble(value, option);
            if (parameters.hours < 0.0) {
                throw std::invalid_argument("-hours must be finite and nonnegative");
            }
        }
        else if (option == "-steps") {
            parameters.maxSteps = parseUnsignedInteger<unsigned long>(value, option);
            if (parameters.maxSteps == 0) {
                throw std::invalid_argument("-steps must be a positive integer");
            }
        }
        else if (option == "-tempsteps") {
            const unsigned long parsedTemperatureRoundSteps =
                parseUnsignedInteger<unsigned long>(value, option);
            if (parsedTemperatureRoundSteps == 0) {
                throw std::invalid_argument("-tempsteps must be a positive integer");
            }
            parameters.temperatureRoundSteps = parsedTemperatureRoundSteps;
        }
        else if (option == "-inittemp") {
            parameters.initialTemperature = parseFiniteDouble(value, option);
            if (parameters.initialTemperature <= 0.0) {
                throw std::invalid_argument("-inittemp must be finite and greater than zero");
            }
        }
        else if (option == "-fintemp") {
            parameters.finalTemperature = parseFiniteDouble(value, option);
            if (parameters.finalTemperature <= 0.0) {
                throw std::invalid_argument("-fintemp must be finite and greater than zero");
            }
        }
        else if (option == "-annealsteps") {
            parameters.annealingSteps = parsePositiveInt(value, option);
        }
        else if (option == "-configoutput") {
            parameters.configurationOutputs = parseNonnegativeInt(value, option);
        }
        else if (option == "-initconf") {
            if (value != "random" && value != "disk" && value != "load") {
                throw std::invalid_argument(
                    "-initconf must be exactly random, disk, or load");
            }
            parameters.initialConfiguration = value;
        }
        else if (option == "-delta") {
            parameters.deltaOption = parseNonnegativeInt(value, option);
            if (parameters.deltaOption != 0 && parameters.deltaOption != 1) {
                throw std::invalid_argument("-delta must be exactly 0 or 1");
            }
        }
        else if (option == "-NNint") {
            parameters.nearestNeighborInteraction = parseFiniteDouble(value, option);
        }
        else if (option == "-randomseed") {
            const unsigned long parsedSeed = parseUnsignedInteger<unsigned long>(value, option);
            if (parsedSeed == 0) {
                parameters.randomSeed.reset();
            }
            else {
                parameters.randomSeed = parsedSeed;
            }
        }
    }

    if (!hoppingDistanceSupplied) {
        throw std::invalid_argument("Required option -d was not supplied");
    }

    return parameters;
}
