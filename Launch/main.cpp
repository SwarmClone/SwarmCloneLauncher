#include "crash_log.h"

#include <iostream>

int main()
{
    std::string relativePath = "launcher/";
#ifdef _WIN32
    std::string programName = "SwarmCloneLauncher.exe";
#else
    std::string programName = "SwarmCloneLauncher";
#endif

    bool success = runProgramWithCrashLogging(relativePath, programName);

    if (success) {
        std::cout << "程序正常完成" << std::endl;
    } else {
        std::cout << "程序异常终止" << std::endl;
    }

    return 0;
}