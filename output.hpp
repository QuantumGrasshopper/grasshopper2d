// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Olga Goulko

#pragma once

#include <chrono>
#include <cstddef>
#include <fstream>
#include <ostream>
#include <string>
#include <vector>

void prepareOutputFiles(bool overwrite, bool preserveInitialConfiguration);
void checkOutputStream(const std::ostream& stream,
                       const std::string& filename,
                       const char* operation);
void finishOutputFile(std::ofstream& stream, const std::string& filename);

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
