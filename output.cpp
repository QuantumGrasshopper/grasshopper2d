#include "output.hpp"

#include <array>
#include <filesystem>
#include <stdexcept>
#include <system_error>

using namespace std;

namespace {

const array<const char*, 7> outputFiles{
    "result.dat",
    "energies.dat",
    "temperatures.dat",
    "config.dat",
    "initconf.dat",
    "finconf.dat",
    "bestconf.dat"
};

} // namespace

void prepareOutputFiles(bool overwrite, bool preserveInitialConfiguration) {
    vector<string> existingOutputFiles;

    // First inspect the complete output set without changing anything.
    for (const char* filename : outputFiles) {
        if (preserveInitialConfiguration && string(filename) == "initconf.dat") {
            continue;
        }

        error_code error;
        const filesystem::file_status status =
            filesystem::symlink_status(filename, error);

        if (error) {
            if (error == errc::no_such_file_or_directory) {
                continue;
            }
            throw runtime_error("Cannot inspect output artifact "
                                + string(filename) + ": " + error.message());
        }

        if (!filesystem::exists(status)) {
            continue;
        }

        if (filesystem::is_directory(status)) {
            throw runtime_error("Output artifact is a directory: "
                                + string(filename));
        }

        if (!overwrite) {
            throw runtime_error("Output artifact already exists: "
                                + string(filename)
                                + ". Use -overwrite 1 to replace existing outputs.");
        }

        existingOutputFiles.push_back(filename);
    }

    // Only start removing files after the complete preflight succeeded.
    for (const string& filename : existingOutputFiles) {
        error_code error;
        const bool removed = filesystem::remove(filename, error);

        if (error || !removed) {
            const string detail =
                error ? error.message() : "file was not removed";
            throw runtime_error("Cannot remove output artifact "
                                + filename + ": " + detail);
        }
    }
}

void checkOutputStream(const ostream& stream,
                       const string& filename,
                       const char* operation) {
    if (!stream) {
        throw runtime_error("Failed to " + string(operation)
                            + " output file " + filename + ".");
    }
}

void finishOutputFile(ofstream& stream, const string& filename) {
    checkOutputStream(stream, filename, "write");
    stream.flush();
    checkOutputStream(stream, filename, "flush");
    stream.close();
    checkOutputStream(stream, filename, "close");
}

BufferedFileWriter::BufferedFileWriter(const string& filename,
                                       size_t limit,
                                       chrono::milliseconds interval)
        : filename(filename), bufferLimit(limit), flushInterval(interval),
          finishAttempted(false) {
        file.open(filename);
        if (!file.is_open()) {
            throw runtime_error("Failed to open output file " + filename + ".");
        }
        lastFlushTime = chrono::steady_clock::now();
    }

void BufferedFileWriter::write(const string& data) {
        buffer.push_back(data);

        auto now = chrono::steady_clock::now();
        if (buffer.size() >= bufferLimit || (now - lastFlushTime) >= flushInterval) {
            flush();
            lastFlushTime = now;
        }
    }

void BufferedFileWriter::flush() {
        for (const auto& line : buffer) {
            file << line << '\n';
        }
        checkOutputStream(file, filename, "write");
        file.flush();
        checkOutputStream(file, filename, "flush");
        buffer.clear();
    }

void BufferedFileWriter::finish() {
        finishAttempted = true;
        flush();
        file.close();
        checkOutputStream(file, filename, "close");
    }

BufferedFileWriter::~BufferedFileWriter() noexcept {
        if (finishAttempted) {
            return;
        }
        try {
            flush();
            if (file.is_open()) {
                file.close();
            }
        }
        catch (...) {
        }
    }
