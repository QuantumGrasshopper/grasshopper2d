#include "doctest/doctest.h"
#include "utilities.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>

namespace {

class ScopedTemporaryPath {
public:
    explicit ScopedTemporaryPath(const std::string& label)
        : path_(std::filesystem::temp_directory_path()
                / ("grasshopper2d-" + label + "-"
                   + std::to_string(
                       std::chrono::steady_clock::now().time_since_epoch().count()))) {}

    ~ScopedTemporaryPath() {
        std::error_code error;
        std::filesystem::remove_all(path_, error);
    }

    const std::filesystem::path& path() const {
        return path_;
    }

private:
    std::filesystem::path path_;
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

TEST_CASE("output stream helpers finalize a successful file") {
    ScopedTemporaryPath temporaryPath("checked-output");
    const std::string filename = temporaryPath.path().string();
    std::ofstream output(filename);
    REQUIRE(output.is_open());

    output << "checked output\n";
    checkOutputStream(output, filename, "write");
    finishOutputFile(output, filename);

    CHECK_FALSE(output.is_open());
    std::ifstream input(filename);
    REQUIRE(input.is_open());
    const std::string contents((std::istreambuf_iterator<char>(input)),
                               std::istreambuf_iterator<char>());
    CHECK(contents == "checked output\n");
}

TEST_CASE("buffered output explicitly finishes all pending lines") {
    ScopedTemporaryPath temporaryPath("buffered-output");
    const std::string filename = temporaryPath.path().string();
    {
        BufferedFileWriter output(filename, 100, std::chrono::hours(1));
        output.write("first");
        output.write("second");
        output.finish();
    }

    std::ifstream input(filename);
    REQUIRE(input.is_open());
    const std::string contents((std::istreambuf_iterator<char>(input)),
                               std::istreambuf_iterator<char>());
    CHECK(contents == "first\nsecond\n");
}

TEST_CASE("buffered output reports its filename when opening fails") {
    ScopedTemporaryPath temporaryPath("missing-output-parent");
    const std::string filename =
        (temporaryPath.path() / "missing" / "output.dat").string();

    checkRuntimeErrorContains([&filename]() {
        BufferedFileWriter output(filename, 100, std::chrono::hours(1));
    }, filename);
}

TEST_CASE("post-open output failures identify dev full"
          * doctest::skip(!devFullAvailable())) {
    SUBCASE("direct output finalization") {
        std::ofstream output("/dev/full");
        REQUIRE(output.is_open());
        output << "data\n";
        checkRuntimeErrorContains([&output]() {
            finishOutputFile(output, "/dev/full");
        }, "/dev/full");
    }

    SUBCASE("buffered output finalization") {
        BufferedFileWriter output("/dev/full", 100, std::chrono::hours(1));
        output.write("data");
        checkRuntimeErrorContains([&output]() {
            output.finish();
        }, "/dev/full");
    }
}
