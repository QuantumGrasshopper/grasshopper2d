#include "annealing.hpp"
#include "doctest/doctest.h"

#include <cmath>
#include <limits>
#include <memory>
#include <stdexcept>

TEST_CASE("annealing helpers scale temperature, steps, and probability") {
    totalNumSpins = 4;
    cellSize = 0.5;
    gridSize = 4;
    gridArea = 16;
    tempScaling = 0.5;
    deltaOption = 0;

    CHECK(temperatureDecrease(20.0) == doctest::Approx(10.0));
    CHECK(stepIncrease(10) == 20);
    CHECK(energyDecreaseProbDistr(-2.0, 2.0) == doctest::Approx(std::exp(-1.0)));
}

TEST_CASE("temperature-round step growth rejects unsigned long overflow") {
    totalNumSpins = 4;
    cellSize = 0.5;
    gridSize = 4;
    gridArea = 16;
    tempScaling = 0.5;
    deltaOption = 0;

    CHECK_THROWS_AS(stepIncrease(std::numeric_limits<unsigned long>::max()),
                    std::overflow_error);

    tempScaling = 1.0;
    CHECK(stepIncrease(std::numeric_limits<unsigned long>::max())
      == std::numeric_limits<unsigned long>::max());
}

TEST_CASE("accept-reject handles the endpoint probabilities deterministically") {
    totalNumSpins = 4;
    cellSize = 0.5;
    gridSize = 4;
    gridArea = 16;
    tempScaling = 1.0;
    deltaOption = 0;

    using RngPointer = std::unique_ptr<gsl_rng, decltype(&gsl_rng_free)>;
    RngPointer rng(gsl_rng_alloc(gsl_rng_mt19937), &gsl_rng_free);
    REQUIRE(rng != nullptr);
    gsl_rng_set(rng.get(), 12345);

    CHECK_FALSE(acceptreject(0.0, rng.get()));
    CHECK(acceptreject(1.0, rng.get()));
}
