#ifndef MAIN_H
#define MAIN_H

#include <string>
#include <vector>
#include <chrono>

std::string getCurrentTimestamp();
std::string getFormattedTime();
std::string getWindowsVersion();
std::string getCpuInfo();
std::string getMemoryInfo();
std::vector<std::string> getGpuInfo();
std::string getSystemType();
std::string getUnixVersion();
bool runProgramWithCrashLogging(const std::string& relativePath, const std::string& programName);

#endif // MAIN_H