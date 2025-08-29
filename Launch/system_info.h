#ifndef SYSTEM_INFO_H
#define SYSTEM_INFO_H

#include <string>
#include <vector>
#include <chrono>
#include <fstream>

std::string getCurrentTimestamp();
std::string getFormattedTime();
std::string getWindowsVersion();
std::string getCpuInfo();
std::string getMemoryInfo();
std::vector<std::string> getGpuInfo();
std::string getSystemType();
std::string getUnixVersion();
bool generateCrashLog(const std::string& fullPath, const std::vector<std::string>& programOutput);
bool runProgramWithCrashLogging(const std::string& relativePath, const std::string& programName);

#endif // SYSTEM_INFO_H