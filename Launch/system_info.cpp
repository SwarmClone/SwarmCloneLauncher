#include "system_info.h"

#include <iomanip>
#include <sstream>
#include <fstream>
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
    } else if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM) {
        systemType = "32位操作系统，基于ARM的处理器";
    } else if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM64) {
        systemType = "64位操作系统，基于ARM64的处理器";
    } else {
        systemType = "未知系统架构";
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
