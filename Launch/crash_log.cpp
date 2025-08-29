#include "crash_log.h"
#include "system_info.h"

#include <iostream>

#ifdef _WIN32
    #include <windows.h>
    #include <tchar.h>
    #include <tlhelp32.h>
    #include <iphlpapi.h>
    #include <comdef.h>
    #include <wbemidl.h>
    #pragma comment(lib, "wbemuuid.lib")
    #pragma comment(lib, "iphlpapi.lib")
    #pragma comment(lib, "comsuppw.lib")
    #pragma comment(lib, "ole32.lib")
    #pragma comment(lib, "oleaut32.lib")
#else
    #include <unistd.h>
    #include <sys/wait.h>
    #ifdef __APPLE__
        #include <sys/types.h>
        #include <sys/sysctl.h>
    #endif
#endif


// 生成崩溃日志文件
bool generateCrashLog(const std::string& fullPath, const std::vector<std::string>& programOutput) {
    std::string crashLogName = "crashlog_" + getCurrentTimestamp() + ".log";
    std::ofstream crashLog(crashLogName);

    if (crashLog.is_open()) {
        crashLog << "（" << fullPath << "）于（" << getFormattedTime() << "）遇到严重问题而崩溃。请将本日志提交给软件维护人员，方便我们解决问题。\n";
        crashLog << "--------------------\n";

        // 系统信息
        #ifdef _WIN32
            crashLog << "系统版本：" << getWindowsVersion() << "\n";
        #else
            crashLog << "系统版本：" << getUnixVersion() << "\n";
        #endif
        crashLog << "处理器：" << getCpuInfo() << "\n";
        crashLog << "运行内存：" << getMemoryInfo() << "\n";
        crashLog << "显卡：\n";
        std::vector<std::string> gpus = getGpuInfo();
        for (size_t i = 0; i < gpus.size(); i++) {
            crashLog << "GPU" << i << "：" << gpus[i] << "\n";
        }
        crashLog << "系统类型：" << getSystemType() << "\n";
        crashLog << "--------------------\n";
        crashLog << "以下是自程序启动后到崩溃前输出的全部信息：\n";

        // 程序输出
        for (const auto& line : programOutput) {
            crashLog << line;
        }

        crashLog.close();
        std::cout << "崩溃日志已生成: " << crashLogName << std::endl;
        return true;
    } else {
        std::cerr << "无法创建崩溃日志文件" << std::endl;
        return false;
    }
}

// 运行程序并处理崩溃
bool runProgramWithCrashLogging(const std::string& relativePath, const std::string& programName) {
    std::string fullPath = relativePath + "/" + programName;

    #ifdef _WIN32
        // 在 Windows 上，我们不显示控制台输出，而是将输出重定向到文件
        std::ofstream logFile("launcher_log.txt");
        std::streambuf* coutBuf = std::cout.rdbuf();
        std::streambuf* cerrBuf = std::cerr.rdbuf();
        std::cout.rdbuf(logFile.rdbuf());
        std::cerr.rdbuf(logFile.rdbuf());
    #endif

    std::cout << "准备运行程序: " << fullPath << std::endl;

    std::vector<std::string> programOutput;

#ifdef _WIN32
    // Windows实现
    HANDLE hReadPipe, hWritePipe;
    SECURITY_ATTRIBUTES sa;

    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) {
        std::cerr << "创建管道失败" << std::endl;
        #ifdef _WIN32
            std::cout.rdbuf(coutBuf);
            std::cerr.rdbuf(cerrBuf);
            logFile.close();
        #endif
        return false;
    }

    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.hStdError = hWritePipe;
    si.hStdOutput = hWritePipe;
    si.dwFlags |= STARTF_USESTDHANDLES;
    ZeroMemory(&pi, sizeof(pi));

    // 创建进程
    if (!CreateProcess(
        NULL,                   // 应用程序名称
        const_cast<LPSTR>(fullPath.c_str()), // 命令行
        NULL,                   // 进程安全属性
        NULL,                   // 线程安全属性
        TRUE,                   // 句柄继承选项
        CREATE_NO_WINDOW,       // 创建标志 - 不显示窗口
        NULL,                   // 环境变量
        relativePath.c_str(),   // 当前目录
        &si,                    // STARTUPINFO
        &pi))                   // PROCESS_INFORMATION
    {
        std::cerr << "CreateProcess failed (" << GetLastError() << ")." << std::endl;
        CloseHandle(hReadPipe);
        CloseHandle(hWritePipe);
        #ifdef _WIN32
            std::cout.rdbuf(coutBuf);
            std::cerr.rdbuf(cerrBuf);
            logFile.close();
        #endif
        return false;
    }

    CloseHandle(hWritePipe);

    // 读取程序输出
    char buffer[4096];
    DWORD bytesRead;
    while (true) {
        if (!ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL) || bytesRead == 0) {
            if (GetLastError() == ERROR_BROKEN_PIPE) {
                break; // 管道已断开
            }
        }
        buffer[bytesRead] = '\0';
        programOutput.push_back(buffer);
        std::cout << buffer; // 输出到日志文件
    }

    // 等待进程结束
    WaitForSingleObject(pi.hProcess, INFINITE);

    // 获取退出代码
    DWORD exitCode;
    if (GetExitCodeProcess(pi.hProcess, &exitCode)) {
        std::cout << "程序退出代码: " << exitCode << std::endl;

        // 如果程序异常退出（崩溃）
        if (exitCode != 0) {
            generateCrashLog(fullPath, programOutput);

            // 关闭进程和线程句柄
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            CloseHandle(hReadPipe);

            #ifdef _WIN32
                std::cout.rdbuf(coutBuf);
                std::cerr.rdbuf(cerrBuf);
                logFile.close();
            #endif
            return false;
        }
    } else {
        std::cerr << "获取退出代码失败 (" << GetLastError() << ")." << std::endl;
    }

    // 关闭进程和线程句柄
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hReadPipe);

    #ifdef _WIN32
        std::cout.rdbuf(coutBuf);
        std::cerr.rdbuf(cerrBuf);
        logFile.close();
        // 删除日志文件，因为程序正常退出
        remove("launcher_log.txt");
    #endif

#else
    // Linux/macOS实现
    int stdoutPipe[2];
    if (pipe(stdoutPipe) == -1) {
        std::cerr << "创建管道失败" << std::endl;
        return false;
    }

    pid_t pid = fork();

    if (pid == 0) {
        // 子进程
        close(stdoutPipe[0]); // 关闭读端

        // 重定向标准输出和错误到管道
        dup2(stdoutPipe[1], STDOUT_FILENO);
        dup2(stdoutPipe[1], STDERR_FILENO);
        close(stdoutPipe[1]);

        // 切换到指定目录
        if (chdir(relativePath.c_str()) != 0) {
            std::cerr << "无法切换到目录: " << relativePath << std::endl;
            exit(EXIT_FAILURE);
        }

        // 执行程序
        execl(programName.c_str(), programName.c_str(), NULL);

        // 如果execl返回，说明出错了
        std::cerr << "执行程序失败: " << programName << std::endl;
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        // 父进程
        close(stdoutPipe[1]); // 关闭写端

        // 读取程序输出
        char buffer[4096];
        ssize_t bytesRead;
        while ((bytesRead = read(stdoutPipe[0], buffer, sizeof(buffer) - 1)) > 0) {
            buffer[bytesRead] = '\0';
            programOutput.push_back(buffer);
            std::cout << buffer; // 实时输出到控制台
        }

        close(stdoutPipe[0]);

        int status;
        waitpid(pid, &status, 0);

        if (WIFEXITED(status)) {
            int exitCode = WEXITSTATUS(status);
            std::cout << "程序退出代码: " << exitCode << std::endl;

            if (exitCode != 0) {
                generateCrashLog(fullPath, programOutput);
                return false;
            }
        } else if (WIFSIGNALED(status)) {
            // 程序被信号终止
            int signal = WTERMSIG(status);
            std::cout << "程序被信号终止: " << signal << std::endl;

            generateCrashLog(fullPath, programOutput);
            return false;
        }
    } else {
        // fork失败
        std::cerr << "fork失败，无法创建子进程" << std::endl;
        return false;
    }
#endif

    return true;
}
