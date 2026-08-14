#include "doctest/doctest.h"
#include "interactions.hpp"
#include "utilities.hpp"

#include <cmath>
#include <utility>

TEST_CASE("coordinate conversion and physical positions") {
    totalNumSpins = 4;
    cellSize = 0.25;
    gridSize = 4;
    gridArea = 16;
    tempScaling = 1.0;
    deltaOption = 0;

    CHECK(xcoord(11) == 3);
    CHECK(ycoord(11) == 2);

    const auto position = findPosition(11);
    CHECK(position.first == doctest::Approx(0.75));
    CHECK(position.second == doctest::Approx(0.5));
}

TEST_CASE("Euclidean distance overloads use the configured cell size") {
    totalNumSpins = 4;
    cellSize = 0.25;
    gridSize = 4;
    gridArea = 16;
    tempScaling = 1.0;
    deltaOption = 0;

    CHECK(euclideanDistance(std::pair<double,double>{0.0, 0.0},
                            std::pair<double,double>{3.0, 4.0})
          == doctest::Approx(5.0));
    CHECK(euclideanDistance(std::pair<int,int>{0, 0},
                            std::pair<int,int>{3, 4})
          == doctest::Approx(1.25));
}

TEST_CASE("delta option zero has the expected compact support and weights") {
    totalNumSpins = 4;
    cellSize = 0.25;
    gridSize = 4;
    gridArea = 16;
    tempScaling = 1.0;
    deltaOption = 0;

    CHECK(isAround(0.0, 0.5));
    CHECK_FALSE(isAround(0.0, 0.500001));
    CHECK(contributionEnergy(0.0, 0.0) == doctest::Approx(0.5));
    CHECK(contributionEnergy(0.0, 0.25) == doctest::Approx(0.25));
    CHECK(std::abs(contributionEnergy(0.0, 0.5)) < 1e-15);
    CHECK(contributionEnergy(0.0, 0.500001) == 0.0);
    CHECK(contributionEnergy(0.25, 0.0) == contributionEnergy(0.0, 0.25));
}

TEST_CASE("delta option one evaluates both analytic branches") {
    totalNumSpins = 1;
    cellSize = 1.0;
    gridSize = 4;
    gridArea = 16;
    tempScaling = 1.0;
    deltaOption = 1;

    CHECK(contributionEnergy(0.0, 0.5)
          == doctest::Approx(0.46704998233983941).epsilon(1e-12));
    CHECK(contributionEnergy(0.0, 1.5)
          == doctest::Approx(0.03295001766016048).epsilon(1e-12));
    CHECK(contributionEnergy(0.0, 2.0) == 0.0);
    CHECK(contributionEnergy(1.5, 0.0) == contributionEnergy(0.0, 1.5));
}
