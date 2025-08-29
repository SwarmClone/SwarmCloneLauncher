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

#endif // SYSTEM_INFO_H