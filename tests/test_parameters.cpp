// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Olga Goulko

#include "doctest/doctest.h"
#include "parameters.hpp"

#include <stdexcept>
#include <string>
#include <vector>

namespace {

SimulationParameters parse(std::vector<std::string> arguments) {
    std::vector<char*> argv;
    argv.reserve(arguments.size());
    for (std::string& argument : arguments) {
        argv.push_back(argument.data());
    }
    return parseSimulationParameters(static_cast<int>(argv.size()), argv.data());
}

void checkRejected(std::vector<std::string> arguments, const std::string& expectedText) {
    try {
        static_cast<void>(parse(std::move(arguments)));
        FAIL_CHECK("invalid command line was accepted");
    }
    catch (const std::invalid_argument& error) {
        CHECK(std::string(error.what()).find(expectedText) != std::string::npos);
    }
}

} // namespace

TEST_CASE("simulation parameter defaults are explicit") {
    const SimulationParameters parameters = parse({"grasshopper", "-d", "0.5"});

    CHECK(parameters.hoppingDistance == doctest::Approx(0.5));
    CHECK(parameters.totalNumSpins == 10000);
    CHECK_FALSE(parameters.gridSize.has_value());
    CHECK(parameters.hours == 0.0);
    CHECK(parameters.maxSteps == 1000000000000UL);
    CHECK_FALSE(parameters.temperatureRoundSteps.has_value());
    CHECK(parameters.initialTemperature == 20.0);
    CHECK(parameters.finalTemperature == 0.01);
    CHECK(parameters.annealingSteps == 1000);
    CHECK(parameters.configurationOutputs == 0);
    CHECK(parameters.initialConfiguration == "random");
    CHECK(parameters.deltaOption == 0);
    CHECK(parameters.nearestNeighborInteraction == 0.0);
    CHECK_FALSE(parameters.randomSeed.has_value());
    CHECK_FALSE(parameters.overwriteExistingOutputs);
}

TEST_CASE("simulation parser accepts every option in arbitrary order") {
    const SimulationParameters parameters = parse({
        "grasshopper",
        "-overwrite", "1",
        "-randomseed", "12345",
        "-NNint", "-0.75",
        "-delta", "1",
        "-initconf", "disk",
        "-configoutput", "4",
        "-annealsteps", "250",
        "-fintemp", "0.05",
        "-inittemp", "12.5",
        "-tempsteps", "20",
        "-steps", "400",
        "-hours", "1.5",
        "-gridsize", "32",
        "-N", "25",
        "-d", "0.3",
    });

    CHECK(parameters.hoppingDistance == doctest::Approx(0.3));
    CHECK(parameters.totalNumSpins == 25);
    REQUIRE(parameters.gridSize.has_value());
    CHECK(*parameters.gridSize == 32);
    CHECK(parameters.hours == doctest::Approx(1.5));
    CHECK(parameters.maxSteps == 400);
    REQUIRE(parameters.temperatureRoundSteps.has_value());
    CHECK(*parameters.temperatureRoundSteps == 20);
    CHECK(parameters.initialTemperature == doctest::Approx(12.5));
    CHECK(parameters.finalTemperature == doctest::Approx(0.05));
    CHECK(parameters.annealingSteps == 250);
    CHECK(parameters.configurationOutputs == 4);
    CHECK(parameters.initialConfiguration == "disk");
    CHECK(parameters.deltaOption == 1);
    CHECK(parameters.nearestNeighborInteraction == doctest::Approx(-0.75));
    REQUIRE(parameters.randomSeed.has_value());
    CHECK(*parameters.randomSeed == 12345);
    CHECK(parameters.overwriteExistingOutputs);

    CHECK_FALSE(parse({"grasshopper", "-d", "1", "-randomseed", "0"})
                    .randomSeed.has_value());
}

TEST_CASE("simulation parser rejects structural command-line errors") {
    checkRejected({"grasshopper"}, "Required option -d");
    checkRejected({"grasshopper", "-d", "1", "-unknown", "2"},
                  "Unknown option: -unknown");
    checkRejected({"grasshopper", "-d", "1", "-d", "2"},
                  "Duplicate option: -d");
    checkRejected({"grasshopper", "-d"}, "Missing value for option -d");
    checkRejected({"grasshopper", "-d", "-N", "2"},
                  "Missing value for option -d");
}

TEST_CASE("simulation parser rejects malformed and out-of-range numbers") {
    checkRejected({"grasshopper", "-d", "1abc"}, "Invalid value for -d");
    checkRejected({"grasshopper", "-d", "nan"}, "Invalid value for -d");
    checkRejected({"grasshopper", "-d", "inf"}, "Invalid value for -d");
    checkRejected({"grasshopper", "-d", "1", "-N", "-2"},
                  "Invalid value for -N");
    checkRejected({"grasshopper", "-d", "1", "-steps", "1.5"},
                  "Invalid value for -steps");
    checkRejected({"grasshopper", "-d", "1", "-N", "4294967296"},
                  "outside the destination type range");
    checkRejected({"grasshopper", "-d", "1", "-annealsteps", "2147483648"},
                  "outside the destination type range");
    checkRejected({"grasshopper", "-d", "1", "-randomseed",
                   "184467440737095516160"},
                  "Invalid value for -randomseed");
}

TEST_CASE("simulation parser enforces option-specific value rules") {
    checkRejected({"grasshopper", "-d", "0"}, "-d must be");
    checkRejected({"grasshopper", "-d", "1", "-N", "0"}, "-N must be");
    checkRejected({"grasshopper", "-d", "1", "-gridsize", "0"},
                  "-gridsize must be");
    checkRejected({"grasshopper", "-d", "1", "-hours", "-0.1"},
                  "-hours must be");
    checkRejected({"grasshopper", "-d", "1", "-steps", "0"},
                  "-steps must be");
    checkRejected({"grasshopper", "-d", "1", "-tempsteps", "0"},
                  "-tempsteps must be");
    checkRejected({"grasshopper", "-d", "1", "-inittemp", "0"},
                  "-inittemp must be");
    checkRejected({"grasshopper", "-d", "1", "-fintemp", "-1"},
                  "-fintemp must be");
    checkRejected({"grasshopper", "-d", "1", "-annealsteps", "0"},
                  "-annealsteps must be");
    checkRejected({"grasshopper", "-d", "1", "-configoutput", "-1"},
                  "Invalid value for -configoutput");
    checkRejected({"grasshopper", "-d", "1", "-initconf", "Random"},
                  "-initconf must be exactly");
    checkRejected({"grasshopper", "-d", "1", "-delta", "2"},
                  "-delta must be exactly");
    checkRejected({"grasshopper", "-d", "1", "-NNint", "infinity"},
                  "Invalid value for -NNint");
    checkRejected({"grasshopper", "-d", "1", "-overwrite", "2"},
                  "-overwrite must be exactly");
}

TEST_CASE("annealing step counts below one hundred remain parser-valid") {
    const SimulationParameters parameters =
        parse({"grasshopper", "-d", "1", "-annealsteps", "1"});
    CHECK(parameters.annealingSteps == 1);
}
