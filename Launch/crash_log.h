#ifndef CRASH_LOG_H
#define CRASH_LOG_H

#include <string>
#include <vector>
#include <chrono>
#include <fstream>

#ifdef _WIN32
#include <OleCtl.h>
struct CodePageRestorer {
    UINT old;
    CodePageRestorer() { old = GetConsoleOutputCP(); SetConsoleOutputCP(65001); }
    ~CodePageRestorer() { SetConsoleOutputCP(old); }
};
#else
struct CodePageRestorer {};
#endif

bool generateCrashLog(const std::string& fullPath, const std::vector<std::string>& programOutput);
bool runProgramWithCrashLogging(const std::string& relativePath, const std::string& programName);

#endif // CRASH_LOG_H
