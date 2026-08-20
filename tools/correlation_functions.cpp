#include "../interactions.hpp"
#include "../output.hpp"
#include "../utilities.hpp"

#include <cmath>
#include <cstddef>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

unsigned int totalNumSpins;
double cellSize;
unsigned int gridSize;
unsigned int gridArea;
int deltaOption;

namespace {

constexpr const char* resultFilename = "result.dat";
constexpr const char* outputFilename = "correlations.dat";

struct CorrelationOptions {
    std::string configurationFilename;
    std::optional<double> distance;
};

struct SimulationMetadata {
    unsigned int totalNumSpins;
    double hoppingDistance;
    unsigned int gridSize;
    int deltaOption;
};

double parsePositiveDistance(const std::string& text) {
    std::size_t parsed = 0;
    double value = 0.0;

    try {
        value = std::stod(text, &parsed);
    }
    catch (const std::exception&) {
        throw std::invalid_argument("Invalid value for -r: " + text);
    }

    if (parsed != text.size() || !std::isfinite(value) || value <= 0.0) {
        throw std::invalid_argument("-r must be a finite value greater than zero: " + text);
    }
    return value;
}

unsigned int parseUnsignedValue(const std::string& text,
                                const std::string& fieldName) {
    if (text.empty()
        || text.find_first_not_of("0123456789") != std::string::npos) {
        throw std::runtime_error("Invalid " + fieldName + ": " + text);
    }

    unsigned long long value = 0;
    try {
        std::size_t parsed = 0;
        value = std::stoull(text, &parsed, 10);
        if (parsed != text.size()) {
            throw std::runtime_error("Invalid " + fieldName + ": " + text);
        }
    }
    catch (const std::exception&) {
        throw std::runtime_error("Invalid " + fieldName + ": " + text);
    }

    if (value > std::numeric_limits<unsigned int>::max()) {
        throw std::runtime_error(fieldName + " is outside the supported range: " + text);
    }
    return static_cast<unsigned int>(value);
}

CorrelationOptions parseOptions(int argc, char* argv[]) {
    CorrelationOptions options;
    bool haveConfiguration = false;
    std::set<std::string> seenOptions;
    const std::set<std::string> recognizedOptions{"-config", "-r"};

    for (int index = 1; index < argc; ++index) {
        const std::string option = argv[index];
        if (recognizedOptions.count(option) == 0) {
            throw std::invalid_argument("Unknown option: " + option);
        }
        if (!seenOptions.insert(option).second) {
            throw std::invalid_argument("Duplicate option: " + option);
        }
        if (index + 1 >= argc
            || recognizedOptions.count(argv[index + 1]) != 0) {
            throw std::invalid_argument("Missing value for " + option + ".");
        }

        const std::string value = argv[++index];
        if (option == "-config") {
            if (value.empty()) {
                throw std::invalid_argument("-config requires a nonempty filename.");
            }
            options.configurationFilename = value;
            haveConfiguration = true;
        }
        else if (option == "-r") {
            options.distance = parsePositiveDistance(value);
        }
    }

    if (!haveConfiguration) {
        throw std::invalid_argument("Missing required option -config.");
    }
    return options;
}

void assignMetadataValue(std::optional<unsigned int>& destination,
                         const std::string& valueText,
                         const std::string& fieldName,
                         const std::string& filename) {
    if (destination.has_value()) {
        throw std::runtime_error("Duplicate " + fieldName
                                 + " field in " + filename + ".");
    }
    destination = parseUnsignedValue(valueText, fieldName);
}

void assignDistance(std::optional<double>& destination,
                    const std::string& valueText,
                    const std::string& filename) {
    if (destination.has_value()) {
        throw std::runtime_error("Duplicate Hopping distance field in "
                                 + filename + ".");
    }

    std::size_t parsed = 0;
    double value = 0.0;
    try {
        value = std::stod(valueText, &parsed);
    }
    catch (const std::exception&) {
        throw std::runtime_error("Invalid hopping distance in " + filename
                                 + ": " + valueText);
    }
    if (parsed != valueText.size() || !std::isfinite(value) || value <= 0.0) {
        throw std::runtime_error("Hopping distance must be a finite value greater "
                                 "than zero in " + filename + ": " + valueText);
    }
    destination = value;
}

SimulationMetadata readMetadata(const std::string& filename) {
    std::ifstream input(filename);
    if (!input.is_open()) {
        throw std::runtime_error("Failed to open metadata file " + filename + ".");
    }

    const std::string spinLabel = "Total number of spins: ";
    const std::string distanceLabel = "Hopping distance: ";
    const std::string gridLabel = "Size of grid: ";
    const std::string deltaLabel =
        "Option for delta-function discretization: ";
    std::optional<unsigned int> parsedSpins;
    std::optional<double> parsedDistance;
    std::optional<unsigned int> parsedGridSize;
    std::optional<unsigned int> parsedDelta;
    std::string line;

    while (std::getline(input, line)) {
        if (line.compare(0, spinLabel.size(), spinLabel) == 0) {
            assignMetadataValue(parsedSpins, line.substr(spinLabel.size()),
                                "total number of spins", filename);
        }
        else if (line.compare(0, distanceLabel.size(), distanceLabel) == 0) {
            assignDistance(parsedDistance, line.substr(distanceLabel.size()),
                           filename);
        }
        else if (line.compare(0, gridLabel.size(), gridLabel) == 0) {
            assignMetadataValue(parsedGridSize, line.substr(gridLabel.size()),
                                "grid size", filename);
        }
        else if (line.compare(0, deltaLabel.size(), deltaLabel) == 0) {
            assignMetadataValue(parsedDelta, line.substr(deltaLabel.size()),
                                "delta option", filename);
        }
    }
    if (input.bad()) {
        throw std::runtime_error("Failed to read metadata file " + filename + ".");
    }

    if (!parsedSpins.has_value()) {
        throw std::runtime_error("Missing Total number of spins field in "
                                 + filename + ".");
    }
    if (!parsedGridSize.has_value()) {
        throw std::runtime_error("Missing Size of grid field in "
                                 + filename + ".");
    }
    if (!parsedDistance.has_value()) {
        throw std::runtime_error("Missing Hopping distance field in "
                                 + filename + ".");
    }
    if (!parsedDelta.has_value()) {
        throw std::runtime_error(
            "Missing Option for delta-function discretization field in "
            + filename + ".");
    }
    if (*parsedSpins == 0) {
        throw std::runtime_error("Total number of spins must be greater than zero in "
                                 + filename + ".");
    }
    if (*parsedGridSize == 0) {
        throw std::runtime_error("Grid size must be greater than zero in "
                                 + filename + ".");
    }
    if (*parsedDelta > 1) {
        throw std::runtime_error("Delta option must be exactly 0 or 1 in "
                                 + filename + ".");
    }

    return {*parsedSpins, *parsedDistance, *parsedGridSize,
            static_cast<int>(*parsedDelta)};
}

std::vector<unsigned char> readConfiguration(const std::string& filename) {
    std::ifstream input(filename);
    if (!input.is_open()) {
        throw std::runtime_error("Failed to open configuration file " + filename + ".");
    }

    std::vector<unsigned char> occupation(gridArea, 0);
    for (unsigned int index = 0; index < totalNumSpins; ++index) {
        std::string coordinateText;
        if (!(input >> coordinateText)) {
            throw std::runtime_error("Configuration file " + filename
                                     + " contains fewer than "
                                     + std::to_string(totalNumSpins)
                                     + " coordinates.");
        }

        if (coordinateText.empty()
            || coordinateText.find_first_not_of("0123456789")
               != std::string::npos) {
            throw std::runtime_error("Invalid coordinate in " + filename
                                     + ": " + coordinateText);
        }

        unsigned long long coordinate = 0;
        try {
            std::size_t parsed = 0;
            coordinate = std::stoull(coordinateText, &parsed, 10);
            if (parsed != coordinateText.size()) {
                throw std::runtime_error("Invalid coordinate");
            }
        }
        catch (const std::exception&) {
            throw std::runtime_error("Invalid coordinate in " + filename
                                     + ": " + coordinateText);
        }

        if (coordinate >= gridArea) {
            throw std::runtime_error("Coordinate is outside the grid in "
                                     + filename + ": " + coordinateText);
        }
        if (occupation[static_cast<unsigned int>(coordinate)] != 0) {
            throw std::runtime_error("Duplicate coordinate in " + filename
                                     + ": " + coordinateText);
        }
        occupation[static_cast<unsigned int>(coordinate)] = 1;
    }

    std::string trailingData;
    if (input >> trailingData) {
        throw std::runtime_error("Configuration file " + filename
                                 + " contains trailing data: " + trailingData);
    }
    if (input.bad()) {
        throw std::runtime_error("Failed to read configuration file "
                                 + filename + ".");
    }
    return occupation;
}

} // namespace

int main(int argc, char* argv[]) {
    try {
        const CorrelationOptions options = parseOptions(argc, argv);
        const SimulationMetadata metadata = readMetadata(resultFilename);
        const double requestedDistance =
            options.distance.value_or(metadata.hoppingDistance);

        const unsigned long long selectedGridArea =
            static_cast<unsigned long long>(metadata.gridSize) * metadata.gridSize;
        if (selectedGridArea > std::numeric_limits<unsigned int>::max()
            || selectedGridArea > std::numeric_limits<int>::max()) {
            throw std::runtime_error("Grid area is outside the supported range.");
        }
        if (metadata.totalNumSpins >= selectedGridArea) {
            throw std::runtime_error("Total number of spins must be smaller than the grid area.");
        }

        totalNumSpins = metadata.totalNumSpins;
        gridSize = metadata.gridSize;
        gridArea = static_cast<unsigned int>(selectedGridArea);
        deltaOption = metadata.deltaOption;
        cellSize = 1.0 / std::sqrt(static_cast<double>(totalNumSpins));

        const std::vector<unsigned char> occupation =
            readConfiguration(options.configurationFilename);
        if (requestedDistance <= 2.0 * cellSize) {
            std::cerr << "Warning: requested distance r <= 2*cellSize; the smeared "
                      << "interaction is not reliable at distances comparable to "
                      << "the grid spacing\n";
        }
        const GrasshopperInteractionTable interactionTable =
            buildInteractionTable(requestedDistance);
        const std::vector<double> interactionGrid =
            buildGrasshopperInteractionGrid(occupation.data(), interactionTable);
        const double totalInteraction =
            totalGrasshopperInteraction(occupation.data(), interactionGrid);

        std::vector<unsigned int> nearestNeighborCounts(gridArea);
        long long nearestNeighborBonds = 0;
        for (unsigned int cell = 0; cell < gridArea; ++cell) {
            nearestNeighborCounts[cell] =
                nearestNeighborCount(cell, occupation.data());
            if (occupation[cell] != 0) {
                nearestNeighborBonds += nearestNeighborCounts[cell];
            }
        }
        nearestNeighborBonds /= 2;

        std::ofstream output(outputFilename);
        if (!output.is_open()) {
            throw std::runtime_error("Failed to open output file "
                                     + std::string(outputFilename) + ".");
        }
        output << std::setprecision(std::numeric_limits<double>::max_digits10);
        output << "# cell x y occupation local_grasshopper_probability nn_fraction\n";

        for (unsigned int cell = 0; cell < gridArea; ++cell) {
            output << cell << ' '
                   << xcoord(cell) << ' '
                   << ycoord(cell) << ' '
                   << static_cast<unsigned int>(occupation[cell]) << ' '
                   << normalizeGrasshopperInteraction(interactionGrid[cell],
                                                      requestedDistance) << ' '
                   << normalizeNearestNeighborCount(nearestNeighborCounts[cell])
                   << '\n';
        }
        finishOutputFile(output, outputFilename);

        std::cout << std::setprecision(std::numeric_limits<double>::max_digits10)
                  << "Requested distance: " << requestedDistance << '\n'
                  << "Global grasshopper probability: "
                  << normalizeGrasshopperEnergy(totalInteraction,
                                                requestedDistance) << '\n'
                  << "Global nearest-neighbor probability: "
                  << nearestNeighborProbability(nearestNeighborBonds) << '\n'
                  << "Output file: " << outputFilename << '\n';
        return 0;
    }
    catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}
