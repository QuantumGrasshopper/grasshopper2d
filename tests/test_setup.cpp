#include "doctest/doctest.h"
#include "setup.hpp"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

namespace {

void checkConfiguration(const std::vector<unsigned char>& grid,
                        const std::vector<int>& spins) {
    CHECK(grid.size() == gridArea);
    CHECK(spins.size() == totalNumSpins);
    CHECK(std::count(grid.begin(), grid.end(), static_cast<unsigned char>(1))
          == static_cast<std::ptrdiff_t>(totalNumSpins));
    CHECK(std::all_of(grid.begin(), grid.end(), [](unsigned char value) {
        return value == 0 || value == 1;
    }));

    std::set<int> uniqueSpins;
    for (int coordinate : spins) {
        REQUIRE(coordinate >= 0);
        REQUIRE(coordinate < static_cast<int>(grid.size()));
        CHECK(grid[coordinate] == 1);
        uniqueSpins.insert(coordinate);
    }
    CHECK(uniqueSpins.size() == totalNumSpins);
}

class ScopedTemporaryDirectory {
public:
    ScopedTemporaryDirectory()
        : originalDirectory_(std::filesystem::current_path()) {
        const auto identifier = std::chrono::steady_clock::now().time_since_epoch().count();
        const auto baseName = std::string("grasshopper2d-unit-tests-")
                            + std::to_string(identifier);

        for (unsigned int attempt = 0; attempt < 100; ++attempt) {
            const auto candidate = std::filesystem::temp_directory_path()
                                 / (baseName + "-" + std::to_string(attempt));
            if (std::filesystem::create_directory(candidate)) {
                temporaryDirectory_ = candidate;
                std::filesystem::current_path(temporaryDirectory_);
                return;
            }
        }

        throw std::runtime_error("Could not create a temporary test directory");
    }

    ~ScopedTemporaryDirectory() {
        std::error_code error;
        std::filesystem::current_path(originalDirectory_, error);
        std::filesystem::remove_all(temporaryDirectory_, error);
    }

    ScopedTemporaryDirectory(const ScopedTemporaryDirectory&) = delete;
    ScopedTemporaryDirectory& operator=(const ScopedTemporaryDirectory&) = delete;

private:
    std::filesystem::path originalDirectory_;
    std::filesystem::path temporaryDirectory_;
};

template<typename Operation>
void checkRuntimeErrorContains(Operation operation, const std::string& expectedText) {
    bool threw = false;
    try {
        operation();
    }
    catch (const std::runtime_error& error) {
        threw = true;
        CHECK(std::string(error.what()).find(expectedText) != std::string::npos);
    }
    CHECK(threw);
}

bool devFullAvailable() {
#ifdef __linux__
    std::ofstream probe("/dev/full");
    return probe.is_open();
#else
    return false;
#endif
}

} // namespace

TEST_CASE("random initialization is reproducible and internally consistent") {
    totalNumSpins = 5;
    cellSize = 1.0 / std::sqrt(static_cast<double>(totalNumSpins));
    gridSize = 8;
    gridArea = gridSize * gridSize;
    tempScaling = 1.0;
    deltaOption = 0;

    using RngPointer = std::unique_ptr<gsl_rng, decltype(&gsl_rng_free)>;
    RngPointer firstRng(gsl_rng_alloc(gsl_rng_mt19937), &gsl_rng_free);
    RngPointer secondRng(gsl_rng_alloc(gsl_rng_mt19937), &gsl_rng_free);
    REQUIRE(firstRng != nullptr);
    REQUIRE(secondRng != nullptr);
    gsl_rng_set(firstRng.get(), 2026);
    gsl_rng_set(secondRng.get(), 2026);

    std::vector<unsigned char> firstGrid(gridArea);
    std::vector<unsigned char> secondGrid(gridArea);
    std::vector<int> firstSpins(totalNumSpins);
    std::vector<int> secondSpins(totalNumSpins);

    initRandom(firstGrid.data(), firstSpins.data(), firstRng.get());
    initRandom(secondGrid.data(), secondSpins.data(), secondRng.get());

    CHECK(firstGrid == secondGrid);
    CHECK(firstSpins == secondSpins);
    checkConfiguration(firstGrid, firstSpins);
}

TEST_CASE("disk initialization produces a valid configuration on an even grid") {
    totalNumSpins = 5;
    cellSize = 1.0 / std::sqrt(static_cast<double>(totalNumSpins));
    gridSize = 4;
    gridArea = gridSize * gridSize;
    tempScaling = 1.0;
    deltaOption = 0;

    std::vector<unsigned char> grid(gridArea);
    std::vector<int> spins(totalNumSpins);

    initDisk(grid.data(), spins.data());

    checkConfiguration(grid, spins);
}

TEST_CASE("disk initialization is centered on an odd grid") {
    totalNumSpins = 5;
    cellSize = 1.0 / std::sqrt(static_cast<double>(totalNumSpins));
    gridSize = 5;
    gridArea = gridSize * gridSize;
    tempScaling = 1.0;
    deltaOption = 0;

    std::vector<unsigned char> grid(gridArea);
    std::vector<int> spins(totalNumSpins);

    initDisk(grid.data(), spins.data());

    checkConfiguration(grid, spins);
    const std::set<int> expectedSpins{7, 11, 12, 13, 17};
    CHECK(std::set<int>(spins.begin(), spins.end()) == expectedSpins);
}

TEST_CASE("valid configurations survive a save-load round trip") {
    totalNumSpins = 3;
    cellSize = 1.0 / std::sqrt(static_cast<double>(totalNumSpins));
    gridSize = 4;
    gridArea = gridSize * gridSize;
    tempScaling = 1.0;
    deltaOption = 0;

    ScopedTemporaryDirectory temporaryDirectory;
    std::vector<int> expectedSpins{0, 5, 15};
    saveConfig(expectedSpins.data(), "initconf.dat");

    std::ifstream savedFile("initconf.dat");
    REQUIRE(savedFile.is_open());
    const std::string savedContents((std::istreambuf_iterator<char>(savedFile)),
                                    std::istreambuf_iterator<char>());
    CHECK(savedContents == "0\n5\n15\n");

    std::vector<unsigned char> loadedGrid(gridArea, 1);
    std::vector<int> loadedSpins(totalNumSpins, -1);
    initLoad(loadedGrid.data(), loadedSpins.data());

    CHECK(loadedSpins == expectedSpins);
    checkConfiguration(loadedGrid, loadedSpins);
}

TEST_CASE("loading rejects duplicate coordinates") {
    totalNumSpins = 3;
    cellSize = 1.0 / std::sqrt(static_cast<double>(totalNumSpins));
    gridSize = 4;
    gridArea = gridSize * gridSize;
    tempScaling = 1.0;
    deltaOption = 0;

    ScopedTemporaryDirectory temporaryDirectory;
    {
        std::ofstream configurationFile("initconf.dat");
        REQUIRE(configurationFile.is_open());
        configurationFile << "0\n0\n5\n";
    }

    std::vector<unsigned char> grid(gridArea);
    std::vector<int> spins(totalNumSpins);
    CHECK_THROWS_AS(initLoad(grid.data(), spins.data()), std::runtime_error);
}

TEST_CASE("saving configurations reports output open failures") {
    totalNumSpins = 2;
    cellSize = 1.0 / std::sqrt(static_cast<double>(totalNumSpins));
    gridSize = 4;
    gridArea = gridSize * gridSize;
    tempScaling = 1.0;
    deltaOption = 0;

    ScopedTemporaryDirectory temporaryDirectory;
    std::vector<int> spins{0, 5};
    const std::string filename = "missing/config.dat";
    checkRuntimeErrorContains([&spins, &filename]() {
        saveConfig(spins.data(), filename);
    }, filename);
}

TEST_CASE("configuration write failures identify dev full"
          * doctest::skip(!devFullAvailable())) {
    totalNumSpins = 2;
    cellSize = 1.0 / std::sqrt(static_cast<double>(totalNumSpins));
    gridSize = 4;
    gridArea = gridSize * gridSize;
    tempScaling = 1.0;
    deltaOption = 0;

    std::vector<int> spins{0, 5};
    checkRuntimeErrorContains([&spins]() {
        saveConfig(spins.data(), "/dev/full");
    }, "/dev/full");
}
