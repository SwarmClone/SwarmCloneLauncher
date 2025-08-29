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

void ShowMessageBox(const std::string& message, const std::string& title) {
#ifdef _WIN32
    int len = MultiByteToWideChar(CP_UTF8, 0, message.c_str(), -1, NULL, 0);
    std::wstring wMessage(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, message.c_str(), -1, &wMessage[0], len);
    
    int len2 = MultiByteToWideChar(CP_UTF8, 0, title.c_str(), -1, NULL, 0);
    std::wstring wTitle(len2, 0);
    MultiByteToWideChar(CP_UTF8, 0, title.c_str(), -1, &wTitle[0], len2);
    
    MessageBoxW(NULL, wMessage.c_str(), wTitle.c_str(), MB_OK | MB_ICONERROR);
#elif __APPLE__
    // macOS使用osascript显示消息框
    std::string command = "osascript -e 'display alert \"" + title + "\" message \"" + message + "\"'";
    system(command.c_str());
#else
    // Linux使用zenity或xmessage显示消息框
    std::string command = "zenity --error --title=\"" + title + "\" --text=\"" + message + "\" 2>/dev/null";
    if (system(command.c_str()) != 0) {
        // 如果zenity不可用，尝试使用xmessage
        command = "xmessage -center \"" + title + "\\n" + message + "\" 2>/dev/null";
        system(command.c_str());
    }
#endif
}

int main()
{
    CodePageRestorer _; // 设置控制台代码页为UTF-8

    std::string relativePath = "launcher";
#ifdef _WIN32
    std::string programName = "SwarmCloneLauncher.exe";
#else
    std::string programName = "SwarmCloneLauncher";
#endif

    // 检查launcher目录和程序文件是否存在
    std::string fullPath = relativePath + "/" + programName;
    std::ifstream fileCheck(fullPath);
    if (!fileCheck.good()) {
        std::cerr << u8"错误：启动器文件损坏或缺失，建议您重新安装启动器。 " << fullPath << std::endl;
        std::string errorMsg = std::string(u8"启动器文件损坏或缺失，建议您重新安装启动器: ") + fullPath;
        ShowMessageBox(errorMsg, u8"启动错误");
        return 1;
    }
    fileCheck.close();

    bool success = runProgramWithCrashLogging(relativePath, programName);

    if (success) {
        std::cout << u8"程序正常完成" << std::endl;
    } else {
        std::cout << u8"程序异常终止" << std::endl;
    }

    return 0;
}