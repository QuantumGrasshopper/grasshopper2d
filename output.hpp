#pragma once

#include <chrono>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <ostream>
#include <string>
#include <type_traits>
#include <vector>

void prepareOutputFiles(bool overwrite, bool preserveInitialConfiguration);
void checkOutputStream(const std::ostream& stream,
                       const std::string& filename,
                       const char* operation);
void finishOutputFile(std::ofstream& stream, const std::string& filename);

template<typename T>
T get_option(int inputN, char *inputV[], const char *was)
    {
    char option[20];
    std::sprintf(option, "-%s", was);
    for (int n = 1; n < (inputN - 1); n++)
        {
        if (std::strcmp(inputV[n], option) == 0)
            {
            const char* value = inputV[n + 1];
            // Use double for all numerical types (double, int, bool, etc)
            if constexpr (std::is_same_v<T, std::string>) return std::string(value);
            else return static_cast<T>(std::strtod(value, nullptr));  // Convert directly to double, then cast to T
            }
        }
    // Default values
    if constexpr (std::is_same_v<T, std::string>) return "";
    else return static_cast<T>(0.0);
}

class BufferedFileWriter {
private:
    std::ofstream file;
    std::string filename;
    std::vector<std::string> buffer;
    std::size_t bufferLimit;
    std::chrono::steady_clock::time_point lastFlushTime;
    std::chrono::milliseconds flushInterval;
    bool finishAttempted;

public:
    BufferedFileWriter(const std::string& filename, std::size_t limit,
                       std::chrono::milliseconds interval);
    void write(const std::string& data);
    void flush();
    void finish();
    ~BufferedFileWriter() noexcept;
};
