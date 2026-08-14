#include "doctest/doctest.h"
#include "output.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <system_error>

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
