#include "system_info.h"
#include <iostream>
#include "crash_log.h"

#ifdef _WIN32
#include <windows.h>
class CodePageRestorer {
public:
    CodePageRestorer() : oldCodePage(GetConsoleOutputCP()) {
        SetConsoleOutputCP(CP_UTF8);
    }
    ~CodePageRestorer() {
        SetConsoleOutputCP(oldCodePage);
    }
private:
    UINT oldCodePage;
};
#else
class CodePageRestorer {
public:
    CodePageRestorer() {}
};
#endif

int main()
{
    CodePageRestorer _; // 设置控制台代码页为UTF-8

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