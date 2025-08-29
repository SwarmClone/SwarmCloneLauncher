#include "main.h"

#include <iomanip>
#include <sstream>
#include <fstream>
#include <algorithm>
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
    #include <signal.h>
    #include <fcntl.h>
    #include <sys/utsname.h>
    #include <sys/sysinfo.h>
    #ifdef __APPLE__
        #include <sys/types.h>
        #include <sys/sysctl.h>
    #endif
#endif


// 获取当前时间戳字符串，格式：年月日时分秒
std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()) % 1000;

    std::stringstream ss;

    // 使用安全的 localtime_s 函数 (Windows) 或 localtime_r (Unix-like)
    #ifdef _WIN32
        struct tm time_info;
        localtime_s(&time_info, &in_time_t);
        ss << std::put_time(&time_info, "%Y%m%d%H%M%S");
    #else
        struct tm time_info;
        localtime_r(&in_time_t, &time_info);
        ss << std::put_time(&time_info, "%Y%m%d%H%M%S");
    #endif

    ss << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

// 获取格式化的时间字符串
std::string getFormattedTime() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;

    // 使用安全的 localtime_s 函数 (Windows) 或 localtime_r (Unix-like)
    #ifdef _WIN32
        struct tm time_info;
        localtime_s(&time_info, &in_time_t);
        ss << std::put_time(&time_info, "%Y年%m月%d日 %H:%M:%S");
    #else
        struct tm time_info;
        localtime_r(&in_time_t, &time_info);
        ss << std::put_time(&time_info, "%Y年%m月%d日 %H:%M:%S");
    #endif

    return ss.str();
}

#ifdef _WIN32
// Windows系统信息获取
std::string getWindowsVersion()
{
    HKEY hKey;
    char buf[128]{};
    DWORD sz;

    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                      R"(SOFTWARE\Microsoft\Windows NT\CurrentVersion)",
                      0, KEY_READ, &hKey) != ERROR_SUCCESS)
        return "Windows";

    std::string ver;

    sz = sizeof(buf);
    if (RegGetValueA(hKey, nullptr, "ProductName",
                     RRF_RT_REG_SZ, nullptr, buf, &sz) == ERROR_SUCCESS)
        ver = std::string(buf);

    sz = sizeof(buf);
    *buf = '\0';
    if (RegGetValueA(hKey, nullptr, "DisplayVersion",
                     RRF_RT_REG_SZ, nullptr, buf, &sz) == ERROR_SUCCESS)
        ver += "\n版本号" + std::string(buf);

    sz = sizeof(buf);
    *buf = '\0';
    if (RegGetValueA(hKey, nullptr, "CurrentBuildNumber",
                     RRF_RT_REG_SZ, nullptr, buf, &sz) == ERROR_SUCCESS)
    {
        int build = std::stoi(buf);

        DWORD ubr = 0;
        sz = sizeof(ubr);
        RegGetValueA(hKey, nullptr, "UBR",
                     RRF_RT_DWORD, nullptr, &ubr, &sz);

        ver += "\n操作系统版本 " + std::to_string(build) + "." + std::to_string(ubr);
    }

    RegCloseKey(hKey);
    return ver;
}

std::string getCpuInfo() {
    HKEY hKey;
    DWORD dwType = REG_SZ;
    CHAR buffer[256];
    DWORD dwSize = sizeof(buffer);
    std::string cpuInfo;

    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        if (RegQueryValueExA(hKey, "ProcessorNameString", NULL, &dwType, (LPBYTE)buffer, &dwSize) == ERROR_SUCCESS) {
            cpuInfo = buffer;
        }
        RegCloseKey(hKey);
    }

    return cpuInfo;
}

std::string getMemoryInfo() {
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);

    DWORDLONG totalPhysMem = memInfo.ullTotalPhys;
    double totalGB = totalPhysMem / (1024.0 * 1024.0 * 1024.0);

    std::stringstream ss;
    ss << std::fixed << std::setprecision(1) << totalGB << " GB";
    return ss.str();
}

std::vector<std::string> getGpuInfo() {
    std::vector<std::string> gpus;

    HRESULT hres;

    hres = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hres)) {
        return gpus;
    }

    hres = CoInitializeSecurity(
        NULL,
        -1,
        NULL,
        NULL,
        RPC_C_AUTHN_LEVEL_DEFAULT,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL,
        EOAC_NONE,
        NULL
    );

    IWbemLocator *pLoc = NULL;
    hres = CoCreateInstance(
        CLSID_WbemLocator,
        0,
        CLSCTX_INPROC_SERVER,
        IID_IWbemLocator,
        (LPVOID *) &pLoc
    );

    if (FAILED(hres)) {
        CoUninitialize();
        return gpus;
    }

    IWbemServices *pSvc = NULL;
    hres = pLoc->ConnectServer(
        _bstr_t(L"ROOT\\CIMV2"),
        NULL,
        NULL,
        0,
        NULL,
        0,
        NULL,
        &pSvc
    );

    if (FAILED(hres)) {
        pLoc->Release();
        CoUninitialize();
        return gpus;
    }

    hres = CoSetProxyBlanket(
        pSvc,
        RPC_C_AUTHN_WINNT,
        RPC_C_AUTHZ_NONE,
        NULL,
        RPC_C_AUTHN_LEVEL_CALL,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL,
        EOAC_NONE
    );

    IEnumWbemClassObject* pEnumerator = NULL;
    hres = pSvc->ExecQuery(
        bstr_t("WQL"),
        bstr_t("SELECT * FROM Win32_VideoController"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL,
        &pEnumerator
    );

    if (SUCCEEDED(hres)) {
        IWbemClassObject *pclsObj = NULL;
        ULONG uReturn = 0;

        while (pEnumerator) {
            HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);

            if (0 == uReturn) {
                break;
            }

            VARIANT vtProp;
            hr = pclsObj->Get(L"Name", 0, &vtProp, 0, 0);
            if (SUCCEEDED(hr)) {
                std::wstring wname(vtProp.bstrVal);
                std::string name(wname.begin(), wname.end());
                gpus.push_back(name);
                VariantClear(&vtProp);
            }
            pclsObj->Release();
        }
    }

    if (pSvc) pSvc->Release();
    if (pLoc) pLoc->Release();
    if (pEnumerator) pEnumerator->Release();
    CoUninitialize();

    return gpus;
}

std::string getSystemType() {
    SYSTEM_INFO si;
    GetNativeSystemInfo(&si);

    std::string systemType;
    if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64) {
        systemType = "64位操作系统，基于x64的处理器";
    } else if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL) {
        systemType = "32位操作系统，基于x86的处理器";
    } else {
        systemType = "未知架构";
    }

    return systemType;
}

#else
// Linux/macOS系统信息获取
std::string getUnixVersion() {
    struct utsname buf;
    if (uname(&buf) == 0) {
        return std::string(buf.sysname) + " " + std::string(buf.release) + " " + std::string(buf.machine);
    }
    return "Unknown Unix System";
}

std::string getCpuInfo() {
    std::string cpuInfo;

#ifdef __APPLE__
    // macOS 获取CPU信息
    char buffer[1024];
    size_t size = sizeof(buffer);
    if (sysctlbyname("machdep.cpu.brand_string", &buffer, &size, NULL, 0) == 0) {
        cpuInfo = buffer;
    } else {
        cpuInfo = "Apple Silicon (ARM架构)";
    }
#else
    // Linux 获取CPU信息
    std::ifstream cpuinfo("/proc/cpuinfo");
    if (cpuinfo.is_open()) {
        std::string line;
        while (std::getline(cpuinfo, line)) {
            if (line.find("model name") != std::string::npos) {
                size_t pos = line.find(":");
                if (pos != std::string::npos) {
                    cpuInfo = line.substr(pos + 2);
                    break;
                }
            }
        }
        cpuinfo.close();
    }
#endif

    if (cpuInfo.empty()) {
        cpuInfo = "Unknown CPU";
    }

    return cpuInfo;
}

std::string getMemoryInfo() {
#ifdef __APPLE__
    // macOS 获取内存信息
    uint64_t mem;
    size_t size = sizeof(mem);
    if (sysctlbyname("hw.memsize", &mem, &size, NULL, 0) == 0) {
        double totalGB = mem / (1024.0 * 1024.0 * 1024.0);
        std::stringstream ss;
        ss << std::fixed << std::setprecision(1) << totalGB << " GB";
        return ss.str();
    }
    return "Unknown";
#else
    // Linux 获取内存信息
    struct sysinfo info;
    if (sysinfo(&info) == 0) {
        double totalGB = info.totalram * info.mem_unit / (1024.0 * 1024.0 * 1024.0);
        std::stringstream ss;
        ss << std::fixed << std::setprecision(1) << totalGB << " GB";
        return ss.str();
    }
    return "Unknown";
#endif
}

std::vector<std::string> getGpuInfo() {
    std::vector<std::string> gpus;

#ifdef __APPLE__
    // macOS 获取GPU信息
    // 使用system_profiler命令获取GPU信息
    FILE* pipe = popen("system_profiler SPDisplaysDataType | grep \"Chipset Model:\" | sed 's/.*: //'", "r");
    if (pipe) {
        char buffer[256];
        while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
            std::string line(buffer);
            // 移除换行符
            if (!line.empty() && line[line.length()-1] == '\n') {
                line.erase(line.length()-1);
            }
            gpus.push_back(line);
        }
        pclose(pipe);
    }
#else
    // Linux 获取GPU信息
    FILE* pipe = popen("lspci | grep -i vga", "r");
    if (pipe) {
        char buffer[256];
        while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
            std::string line(buffer);
            // 提取GPU名称
            size_t pos = line.find(": ");
            if (pos != std::string::npos) {
                gpus.push_back(line.substr(pos + 2));
            }
        }
        pclose(pipe);
    }
#endif

    if (gpus.empty()) {
        gpus.push_back("Unknown GPU");
    }

    return gpus;
}

std::string getSystemType() {
    struct utsname buf;
    if (uname(&buf) == 0) {
        std::string arch(buf.machine);
        if (arch == "x86_64") {
            return "64位操作系统，基于x86_64的处理器";
        } else if (arch == "arm64" || arch == "aarch64") {
            return "64位操作系统，基于ARM架构的处理器";
        } else if (arch == "i386" || arch == "i686") {
            return "32位操作系统，基于x86的处理器";
        } else {
            return arch + " 位操作系统";
        }
    }
    return "Unknown System Type";
}
#endif

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
            std::string crashLogName = "crashlog_" + getCurrentTimestamp() + ".log";
            std::ofstream crashLog(crashLogName);

            if (crashLog.is_open()) {
                crashLog << "（" << fullPath << "）于（" << getFormattedTime() << "）遇到严重问题而崩溃。请将本日志提交给软件维护人员，方便我们解决问题。\n";
                crashLog << "--------------------\n";

                // 系统信息
                crashLog << "系统版本：" << getWindowsVersion() << "\n";
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
            } else {
                std::cerr << "无法创建崩溃日志文件" << std::endl;
            }

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
                std::string crashLogName = "crashlog_" + getCurrentTimestamp() + ".log";
                std::ofstream crashLog(crashLogName);

                if (crashLog.is_open()) {
                    crashLog << "（" << fullPath << "）于（" << getFormattedTime() << "）遇到严重问题而崩溃。请将本日志提交给软件维护人员，方便我们解决问题。\n";
                    crashLog << "--------------------\n";

                    // 系统信息
                    crashLog << "系统版本：" << getUnixVersion() << "\n";
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
                } else {
                    std::cerr << "无法创建崩溃日志文件" << std::endl;
                }
                return false;
            }
        } else if (WIFSIGNALED(status)) {
            // 程序被信号终止
            int signal = WTERMSIG(status);
            std::cout << "程序被信号终止: " << signal << std::endl;

            std::string crashLogName = "crashlog_" + getCurrentTimestamp() + ".log";
            std::ofstream crashLog(crashLogName);

            if (crashLog.is_open()) {
                crashLog << "（" << fullPath << "）于（" << getFormattedTime() << "）遇到严重问题而崩溃。请将本日志提交给软件维护人员，方便我们解决问题。\n";
                crashLog << "--------------------\n";

                // 系统信息
                crashLog << "系统版本：" << getUnixVersion() << "\n";
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
            } else {
                std::cerr << "无法创建崩溃日志文件" << std::endl;
            }
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

int main()
{
    std::string relativePath = "launcher/";
    std::string programName = "SwarmCloneLauncher.exe";

    bool success = runProgramWithCrashLogging(relativePath, programName);

    if (success) {
        std::cout << "程序正常完成" << std::endl;
    } else {
        std::cout << "程序异常终止" << std::endl;
    }

    return 0;
}