#include <iostream>
#include <string>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <sstream>

// ============================================================================
// 跨平台头文件包含
// ============================================================================
#if defined(_WIN32)
    #include <intrin.h>
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <winsock2.h>
    #include <ws2tcpip.h>
#elif defined(__GNUC__) || defined(__clang__)
    #include <cpuid.h>
#endif

#if defined(__linux__)
    #include <fstream>
    #include <sys/sysinfo.h>
    #include <sys/types.h>
    #include <dirent.h>
    #include <unistd.h>
    #include <netinet/in.h>
    #include <sys/ioctl.h>
    #include <net/if.h>
    #include <arpa/inet.h>
    #include <sys/socket.h>
    #include <sys/stat.h>
#elif defined(__APPLE__)
    #include <sys/sysctl.h>
    #include <net/if.h>
    #include <net/if_dl.h>
    #include <sys/socket.h>
    #include <ifaddrs.h>
    #include <mach/mach.h>
    #include <IOKit/IOKitLib.h>
    #include <IOKit/ps/IOPSKeys.h>
    #include <IOKit/ps/IOPowerSources.h>
#endif

// CPUID 封装函数
void cpuid(uint32_t func, uint32_t& eax, uint32_t& ebx, uint32_t& ecx, uint32_t& edx) {
#if defined(_WIN32)
    int cpuInfo[4];
    __cpuid(cpuInfo, func);
    eax = cpuInfo[0]; ebx = cpuInfo[1]; ecx = cpuInfo[2]; edx = cpuInfo[3];
#elif defined(__GNUC__) || defined(__clang__)
    __get_cpuid(func, &eax, &ebx, &ecx, &edx);
#endif
}

std::string getCPUVendor() {
    uint32_t eax, ebx, ecx, edx;
    cpuid(0, eax, ebx, ecx, edx);
    char vendor[13];
    memcpy(vendor, &ebx, 4); memcpy(vendor + 4, &edx, 4); memcpy(vendor + 8, &ecx, 4);
    vendor[12] = '\0';
    return std::string(vendor);
}

std::string getCPUBrandString() {
    char brand[49] = {0};
    uint32_t eax, ebx, ecx, edx;
    for (uint32_t i = 0x80000002; i <= 0x80000004; i++) {
        cpuid(i, eax, ebx, ecx, edx);
        memcpy(brand + ((i - 0x80000002) * 16), &eax, 4);
        memcpy(brand + ((i - 0x80000002) * 16) + 4, &ebx, 4);
        memcpy(brand + ((i - 0x80000002) * 16) + 8, &ecx, 4);
        memcpy(brand + ((i - 0x80000002) * 16) + 12, &edx, 4);
    }
    return std::string(brand);
}

std::string getCurrentTimeString() {
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    char buffer[80];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
    return std::string(buffer);
}

std::string getCompilerInfo() {
    std::ostringstream oss;
#if defined(_MSC_VER)
    oss << "MSVC " << _MSC_VER;
#elif defined(__clang__)
    oss << "Clang " << __clang_major__ << "." << __clang_minor__;
#elif defined(__GNUC__)
    oss << "GCC " << __GNUC__ << "." << __GNUC_MINOR__;
#else
    oss << "Unknown Compiler";
#endif
    return oss.str();
}

// ============================================================================
// Windows 平台实现
// ============================================================================
#if defined(_WIN32) || defined(_WIN64)

std::string getWindowsVersion() {
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        char productName[256] = {0};
        DWORD size = sizeof(productName);
        if (RegQueryValueExA(hKey, "ProductName", NULL, NULL, (LPBYTE)productName, &size) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return std::string(productName);
        }
        RegCloseKey(hKey);
    }
    return "Windows (Unknown Version)";
}

std::string getLocalIP_Windows() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return "Unknown";
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) != 0) { WSACleanup(); return "Unknown"; }
    struct addrinfo hints, *result, *rp;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    if (getaddrinfo(hostname, NULL, &hints, &result) != 0) { WSACleanup(); return "Unknown"; }
    std::string ip = "Unknown";
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        struct sockaddr_in* addr = (struct sockaddr_in*)rp->ai_addr;
        ip = std::string(inet_ntoa(addr->sin_addr));
        if (ip != "127.0.0.1") break;
    }
    freeaddrinfo(result);
    WSACleanup();
    return ip;
}

double getCPUUsage_Windows() {
    static FILETIME prevIdleTime, prevKernelTime, prevUserTime;
    static bool firstRun = true;
    FILETIME idleTime, kernelTime, userTime;
    if (!GetSystemTimes(&idleTime, &kernelTime, &userTime)) return -1.0;
    if (firstRun) {
        prevIdleTime = idleTime; prevKernelTime = kernelTime; prevUserTime = userTime;
        firstRun = false; Sleep(100); return getCPUUsage_Windows();
    }
    ULARGE_INTEGER prevIdle, prevKernel, prevUser, idle, kernel, user;
    prevIdle.LowPart = prevIdleTime.dwLowDateTime; prevIdle.HighPart = prevIdleTime.dwHighDateTime;
    prevKernel.LowPart = prevKernelTime.dwLowDateTime; prevKernel.HighPart = prevKernelTime.dwHighDateTime;
    prevUser.LowPart = prevUserTime.dwLowDateTime; prevUser.HighPart = prevUserTime.dwHighDateTime;
    idle.LowPart = idleTime.dwLowDateTime; idle.HighPart = idleTime.dwHighDateTime;
    kernel.LowPart = kernelTime.dwLowDateTime; kernel.HighPart = kernelTime.dwHighDateTime;
    user.LowPart = userTime.dwLowDateTime; user.HighPart = userTime.dwHighDateTime;
    ULONGLONG idleDiff = idle.QuadPart - prevIdle.QuadPart;
    ULONGLONG kernelDiff = (kernel.QuadPart - prevKernel.QuadPart) - idleDiff;
    ULONGLONG userDiff = user.QuadPart - prevUser.QuadPart;
    ULONGLONG total = idleDiff + kernelDiff + userDiff;
    prevIdleTime = idleTime; prevKernelTime = kernelTime; prevUserTime = userTime;
    if (total == 0) return 0.0;
    return ((double)(kernelDiff + userDiff) / total) * 100.0;
}

double getMemoryUsage_Windows() {
    MEMORYSTATUSEX memStatus;
    memStatus.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memStatus)) {
        return ((double)(memStatus.ullTotalPhys - memStatus.ullAvailPhys) / memStatus.ullTotalPhys) * 100.0;
    }
    return -1.0;
}

struct SystemInfo {
    std::string osName, cpuModel, cpuVendor, localIP, chargingStatus;
    uint64_t totalMemoryGB, usedMemoryGB;
    int coreCount, batteryPercent;
    double cpuUsage, memoryUsage;
};

SystemInfo getWindowsSystemInfo() {
    SystemInfo info;
    info.totalMemoryGB = 0; info.usedMemoryGB = 0; info.coreCount = 0;
    info.cpuUsage = 0.0; info.memoryUsage = 0.0; info.batteryPercent = -1;
    
    info.osName = getWindowsVersion();
    info.cpuModel = getCPUBrandString();
    info.cpuVendor = getCPUVendor();
    if (info.cpuModel.empty()) {
        HKEY hKey;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            char processorName[256] = {0}; DWORD size = sizeof(processorName);
            if (RegQueryValueExA(hKey, "ProcessorNameString", NULL, NULL, (LPBYTE)processorName, &size) == ERROR_SUCCESS)
                info.cpuModel = std::string(processorName);
            RegCloseKey(hKey);
        }
    }
    
    MEMORYSTATUSEX memStatus; memStatus.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memStatus)) {
        info.totalMemoryGB = memStatus.ullTotalPhys / (1024 * 1024 * 1024);
        info.usedMemoryGB = (memStatus.ullTotalPhys - memStatus.ullAvailPhys) / (1024 * 1024 * 1024);
    }
    SYSTEM_INFO sysInfo; GetSystemInfo(&sysInfo);
    info.coreCount = sysInfo.dwNumberOfProcessors;
    info.localIP = getLocalIP_Windows();
    info.cpuUsage = getCPUUsage_Windows();
    info.memoryUsage = getMemoryUsage_Windows();

    SYSTEM_POWER_STATUS powerStatus;
    if (GetSystemPowerStatus(&powerStatus)) {
        if (powerStatus.BatteryFlag & 128) {
            info.batteryPercent = 100;
            info.chargingStatus = "是";
        } else {
            if (powerStatus.BatteryLifePercent != 255) {
                info.batteryPercent = powerStatus.BatteryLifePercent;
            } else {
                info.batteryPercent = -1;
            }
            if (powerStatus.ACLineStatus == 1) {
                info.chargingStatus = "是";
            } else {
                info.chargingStatus = "否";
            }
        }
    } else {
        info.batteryPercent = -1;
        info.chargingStatus = "未知";
    }

    return info;
}

// ============================================================================
// Linux 平台实现（完整版）
// ============================================================================
#elif defined(__linux__)

std::string getLinuxVersion() {
    std::ifstream file("/etc/os-release");
    std::string line;
    while (std::getline(file, line)) {
        if (line.find("PRETTY_NAME=") == 0) {
            std::string version = line.substr(12);
            if (!version.empty() && version.front() == '"') version.erase(0, 1);
            if (!version.empty() && version.back() == '"') version.pop_back();
            return version;
        }
    }
    return "Linux (Unknown Distribution)";
}

std::string getLocalIP_Linux() {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) return "Unknown";
    
    struct ifconf ifc;
    char buffer[1024];
    ifc.ifc_len = sizeof(buffer);
    ifc.ifc_buf = buffer;
    
    if (ioctl(sock, SIOCGIFCONF, &ifc) < 0) {
        close(sock);
        return "Unknown";
    }
    
    struct ifreq* ifr = ifc.ifc_req;
    int count = ifc.ifc_len / sizeof(struct ifreq);
    
    std::string ip = "Unknown";
    for (int i = 0; i < count; i++) {
        if (ifr[i].ifr_addr.sa_family == AF_INET) {
            struct sockaddr_in* addr = (struct sockaddr_in*)&ifr[i].ifr_addr;
            char ipstr[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(addr->sin_addr), ipstr, INET_ADDRSTRLEN);
            std::string temp(ipstr);
            if (temp != "127.0.0.1") {
                ip = temp;
                break;
            }
        }
    }
    close(sock);
    return ip;
}

double getCPUUsage_Linux() {
    static unsigned long long prevTotalUser = 0, prevTotalNice = 0, prevTotalSystem = 0, prevIdle = 0;
    static bool firstRun = true;
    
    std::ifstream file("/proc/stat");
    std::string line;
    if (!std::getline(file, line) || line.substr(0, 3) != "cpu") {
        return -1.0;
    }
    
    unsigned long long user, nice, system, idle, iowait, irq, softirq;
    std::istringstream iss(line);
    std::string cpu;
    iss >> cpu >> user >> nice >> system >> idle >> iowait >> irq >> softirq;
    
    if (firstRun) {
        prevTotalUser = user;
        prevTotalNice = nice;
        prevTotalSystem = system;
        prevIdle = idle;
        firstRun = false;
        usleep(100000);
        return getCPUUsage_Linux();
    }
    
    unsigned long long totalUser = user - prevTotalUser;
    unsigned long long totalNice = nice - prevTotalNice;
    unsigned long long totalSystem = system - prevTotalSystem;
    unsigned long long totalIdle = idle - prevIdle;
    
    unsigned long long total = totalUser + totalNice + totalSystem + totalIdle;
    
    prevTotalUser = user;
    prevTotalNice = nice;
    prevTotalSystem = system;
    prevIdle = idle;
    
    if (total == 0) return 0.0;
    
    return (100.0 * (total - totalIdle)) / total;
}

double getMemoryUsage_Linux() {
    std::ifstream file("/proc/meminfo");
    std::string line;
    unsigned long long memTotal = 0, memFree = 0, buffers = 0, cached = 0;
    
    while (std::getline(file, line)) {
        if (line.substr(0, 8) == "MemTotal:") {
            std::istringstream iss(line.substr(8));
            iss >> memTotal;
        } else if (line.substr(0, 7) == "MemFree:") {
            std::istringstream iss(line.substr(7));
            iss >> memFree;
        } else if (line.substr(0, 8) == "Buffers:") {
            std::istringstream iss(line.substr(8));
            iss >> buffers;
        } else if (line.substr(0, 6) == "Cached:") {
            std::istringstream iss(line.substr(6));
            iss >> cached;
        }
    }
    
    if (memTotal == 0) return -1.0;
    
    unsigned long long used = memTotal - memFree - buffers - cached;
    return (100.0 * used) / memTotal;
}

int getBatteryInfo_Linux(int& batteryPercent, std::string& chargingStatus) {
    // 检查是否存在电池目录
    struct stat dirInfo;
    if (stat("/sys/class/power_supply", &dirInfo) != 0) {
        // 目录不存在，可能是台式机
        batteryPercent = 100;
        chargingStatus = "是";
        return 0;
    }
    
    bool hasBattery = false;
    DIR* dir = opendir("/sys/class/power_supply");
    if (dir == NULL) {
        batteryPercent = -1;
        chargingStatus = "未知";
        return -1;
    }
    
    std::string batteryPath;
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        std::string name = entry->d_name;
        if (name == "." || name == "..") continue;
        
        std::string typePath = "/sys/class/power_supply/" + name + "/type";
        std::ifstream typeFile(typePath);
        if (typeFile.is_open()) {
            std::string type;
            std::getline(typeFile, type);
            if (type.find("Battery") != std::string::npos) {
                batteryPath = "/sys/class/power_supply/" + name;
                hasBattery = true;
                break;
            }
        }
    }
    closedir(dir);
    
    if (!hasBattery) {
        // 没有电池，台式机
        batteryPercent = 100;
        chargingStatus = "是";
        return 0;
    }
    
    // 读取电池容量
    std::string capacityPath = batteryPath + "/capacity";
    std::ifstream capacityFile(capacityPath);
    if (capacityFile.is_open()) {
        capacityFile >> batteryPercent;
    } else {
        batteryPercent = -1;
    }
    
    // 读取充电状态
    std::string statusPath = batteryPath + "/status";
    std::ifstream statusFile(statusPath);
    if (statusFile.is_open()) {
        std::string status;
        std::getline(statusFile, status);
        if (status == "Charging" || status == "Full") {
            chargingStatus = "是";
        } else if (status == "Discharging") {
            chargingStatus = "否";
        } else {
            chargingStatus = "未知";
        }
    } else {
        chargingStatus = "未知";
    }
    
    return 0;
}

struct SystemInfo {
    std::string osName, cpuModel, cpuVendor, localIP, chargingStatus;
    uint64_t totalMemoryGB, usedMemoryGB;
    int coreCount, batteryPercent;
    double cpuUsage, memoryUsage;
};

SystemInfo getLinuxSystemInfo() {
    SystemInfo info;
    info.totalMemoryGB = 0;
    info.usedMemoryGB = 0;
    info.coreCount = 0;
    info.cpuUsage = 0.0;
    info.memoryUsage = 0.0;
    info.batteryPercent = -1;
    
    info.osName = getLinuxVersion();
    
    std::ifstream cpuFile("/proc/cpuinfo");
    std::string line;
    
    while (std::getline(cpuFile, line)) {
        if (line.find("model name") == 0) {
            size_t pos = line.find(":");
            if (pos != std::string::npos) {
                info.cpuModel = line.substr(pos + 1);
                size_t start = info.cpuModel.find_first_not_of(" \t");
                size_t end = info.cpuModel.find_last_not_of(" \t");
                if (start != std::string::npos && end != std::string::npos) {
                    info.cpuModel = info.cpuModel.substr(start, end - start + 1);
                }
            }
        }
        else if (line.find("vendor_id") == 0) {
            size_t pos = line.find(":");
            if (pos != std::string::npos) {
                info.cpuVendor = line.substr(pos + 1);
                size_t start = info.cpuVendor.find_first_not_of(" \t");
                size_t end = info.cpuVendor.find_last_not_of(" \t");
                if (start != std::string::npos && end != std::string::npos) {
                    info.cpuVendor = info.cpuVendor.substr(start, end - start + 1);
                }
            }
        }
        else if (line.find("processor") == 0) {
            info.coreCount++;
        }
    }
    cpuFile.close();
    
    std::string brandString = getCPUBrandString();
    if (!brandString.empty()) {
        info.cpuModel = brandString;
    }
    
    std::string vendor = getCPUVendor();
    if (!vendor.empty()) {
        info.cpuVendor = vendor;
    }
    
    std::ifstream memFile("/proc/meminfo");
    unsigned long long memTotal = 0, memFree = 0, buffers = 0, cached = 0;
    while (std::getline(memFile, line)) {
        if (line.find("MemTotal") == 0) {
            size_t start = line.find_first_of("0123456789");
            if (start != std::string::npos) {
                memTotal = std::stoull(line.substr(start));
            }
        } else if (line.find("MemFree") == 0) {
            size_t start = line.find_first_of("0123456789");
            if (start != std::string::npos) {
                memFree = std::stoull(line.substr(start));
            }
        } else if (line.find("Buffers") == 0) {
            size_t start = line.find_first_of("0123456789");
            if (start != std::string::npos) {
                buffers = std::stoull(line.substr(start));
            }
        } else if (line.find("Cached") == 0) {
            size_t start = line.find_first_of("0123456789");
            if (start != std::string::npos) {
                cached = std::stoull(line.substr(start));
            }
        }
    }
    memFile.close();
    
    info.totalMemoryGB = memTotal / (1024 * 1024);
    unsigned long long used = memTotal - memFree - buffers - cached;
    info.usedMemoryGB = used / (1024 * 1024);
    
    info.localIP = getLocalIP_Linux();
    info.cpuUsage = getCPUUsage_Linux();
    info.memoryUsage = getMemoryUsage_Linux();
    
    getBatteryInfo_Linux(info.batteryPercent, info.chargingStatus);
    
    return info;
}

// ============================================================================
// macOS 平台实现（完整版）
// ============================================================================
#elif defined(__APPLE__)

std::string getMacOSVersion() {
    char buffer[256];
    size_t size = sizeof(buffer);
    
    if (sysctlbyname("kern.osproductversion", buffer, &size, NULL, 0) == 0) {
        return std::string("macOS ") + std::string(buffer);
    }
    
    size = sizeof(buffer);
    if (sysctlbyname("kern.osrelease", buffer, &size, NULL, 0) == 0) {
        return std::string("macOS (Darwin Kernel ") + std::string(buffer) + ")";
    }
    
    return "macOS (Unknown Version)";
}

std::string getLocalIP_macOS() {
    struct ifaddrs* ifaddr;
    if (getifaddrs(&ifaddr) == -1) {
        return "Unknown";
    }
    
    std::string ip = "Unknown";
    for (struct ifaddrs* ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) continue;
        if (ifa->ifa_addr->sa_family != AF_INET) continue;
        
        struct sockaddr_in* addr = (struct sockaddr_in*)ifa->ifa_addr;
        char ipstr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(addr->sin_addr), ipstr, INET_ADDRSTRLEN);
        std::string temp(ipstr);
        
        if (temp != "127.0.0.1") {
            ip = temp;
            break;
        }
    }
    
    freeifaddrs(ifaddr);
    return ip;
}

double getCPUUsage_macOS() {
    static unsigned long long prevUser = 0, prevSystem = 0, prevIdle = 0;
    static bool firstRun = true;
    
    unsigned long long cpuTimes[CPU_STATE_MAX];
    size_t len = sizeof(cpuTimes);
    if (sysctlbyname("kern.cp_time", &cpuTimes, &len, NULL, 0) != 0) {
        return -1.0;
    }
    
    unsigned long long user = cpuTimes[CP_USER];
    unsigned long long system = cpuTimes[CP_SYS];
    unsigned long long idle = cpuTimes[CP_IDLE];
    
    if (firstRun) {
        prevUser = user;
        prevSystem = system;
        prevIdle = idle;
        firstRun = false;
        usleep(100000);
        return getCPUUsage_macOS();
    }
    
    unsigned long long userDiff = user - prevUser;
    unsigned long long systemDiff = system - prevSystem;
    unsigned long long idleDiff = idle - prevIdle;
    unsigned long long total = userDiff + systemDiff + idleDiff;
    
    prevUser = user;
    prevSystem = system;
    prevIdle = idle;
    
    if (total == 0) return 0.0;
    
    return (100.0 * (userDiff + systemDiff)) / total;
}

double getMemoryUsage_macOS() {
    uint64_t totalPhys = 0;
    size_t len = sizeof(totalPhys);
    
    if (sysctlbyname("hw.memsize", &totalPhys, &len, NULL, 0) != 0) {
        return -1.0;
    }
    
    struct vm_statistics64 vmStats;
    mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
    host_port_t hostPort = mach_host_self();
    
    if (host_statistics64(hostPort, HOST_VM_INFO64, (host_info64_t)&vmStats, &count) != KERN_SUCCESS) {
        return -1.0;
    }
    
    uint64_t pageSize = vm_kernel_page_size;
    uint64_t freeMem = vmStats.free_count * pageSize;
    uint64_t inactiveMem = vmStats.inactive_count * pageSize;
    uint64_t usedMem = totalPhys - freeMem - inactiveMem;
    
    return (100.0 * usedMem) / totalPhys;
}

void getBatteryInfo_macOS(int& batteryPercent, std::string& chargingStatus) {
    CFTypeRef powerSources = IOPSCopyPowerSourcesInfo();
    if (powerSources == NULL) {
        // 无法获取电源信息，假设是台式机
        batteryPercent = 100;
        chargingStatus = "是";
        return;
    }
    
    CFArrayRef sources = IOPSCopyPowerSourcesList(powerSources);
    if (sources == NULL || CFArrayGetCount(sources) == 0) {
        // 没有电池，台式机
        batteryPercent = 100;
        chargingStatus = "是";
        if (sources) CFRelease(sources);
        CFRelease(powerSources);
        return;
    }
    
    bool hasBattery = false;
    for (CFIndex i = 0; i < CFArrayGetCount(sources); i++) {
        CFTypeRef ps = CFArrayGetValueAtIndex(sources, i);
        CFDictionaryRef description = IOPSGetPowerSourceDescription(powerSources, ps);
        
        if (description) {
            CFTypeRef type = (CFTypeRef)CFDictionaryGetValue(description, CFSTR(kIOPSTypeKey));
            if (type && CFStringCompare((CFStringRef)type, CFSTR(kIOPSInternalBatteryType), 0) == kCFCompareEqualTo) {
                hasBattery = true;
                
                // 获取电池容量
                CFNumberRef capacity = (CFNumberRef)CFDictionaryGetValue(description, CFSTR(kIOPSCurrentCapacityKey));
                CFNumberRef maxCapacity = (CFNumberRef)CFDictionaryGetValue(description, CFSTR(kIOPSMaxCapacityKey));
                
                if (capacity && maxCapacity) {
                    int currentCap, maxCap;
                    CFNumberGetValue(capacity, kCFNumberIntType, &currentCap);
                    CFNumberGetValue(maxCapacity, kCFNumberIntType, &maxCap);
                    
                    if (maxCap > 0) {
                        batteryPercent = (currentCap * 100) / maxCap;
                    }
                }
                
                // 获取充电状态
                CFTypeRef isCharging = (CFTypeRef)CFDictionaryGetValue(description, CFSTR(kIOPSIsChargingKey));
                if (isCharging && CFBooleanGetValue((CFBooleanRef)isCharging)) {
                    chargingStatus = "是";
                } else {
                    chargingStatus = "否";
                }
                break;
            }
        }
    }
    
    if (!hasBattery) {
        batteryPercent = 100;
        chargingStatus = "是";
    }
    
    if (sources) CFRelease(sources);
    CFRelease(powerSources);
}

struct SystemInfo {
    std::string osName, cpuModel, cpuVendor, localIP, chargingStatus;
    uint64_t totalMemoryGB, usedMemoryGB;
    int coreCount, batteryPercent;
    double cpuUsage, memoryUsage;
};

SystemInfo getMacOSSystemInfo() {
    SystemInfo info;
    info.totalMemoryGB = 0;
    info.usedMemoryGB = 0;
    info.coreCount = 0;
    info.cpuUsage = 0.0;
    info.memoryUsage = 0.0;
    info.batteryPercent = -1;
    
    info.osName = getMacOSVersion();
    
    char buffer[256];
    size_t size = sizeof(buffer);
    
    if (sysctlbyname("machdep.cpu.brand_string", buffer, &size, NULL, 0) == 0) {
        info.cpuModel = std::string(buffer);
    } else {
        info.cpuModel = getCPUBrandString();
    }
    
    std::string vendor = getCPUVendor();
    if (!vendor.empty()) {
        info.cpuVendor = vendor;
    } else {
        if (sysctlbyname("hw.optional.arm64", buffer, &size, NULL, 0) == 0) {
            info.cpuVendor = "Apple";
        } else {
            info.cpuVendor = "Intel";
        }
    }
    
    uint64_t totalMem = 0;
    size = sizeof(totalMem);
    if (sysctlbyname("hw.memsize", &totalMem, &size, NULL, 0) != 0) {
        totalMem = 0;
    }
    info.totalMemoryGB = totalMem / (1024 * 1024 * 1024);
    
    struct vm_statistics64 vmStats;
    mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
    host_port_t hostPort = mach_host_self();
    
    if (host_statistics64(hostPort, HOST_VM_INFO64, (host_info64_t)&vmStats, &count) == KERN_SUCCESS) {
        uint64_t pageSize = vm_kernel_page_size;
        uint64_t freeMem = vmStats.free_count * pageSize;
        uint64_t inactiveMem = vmStats.inactive_count * pageSize;
        uint64_t usedMem = totalMem - freeMem - inactiveMem;
        info.usedMemoryGB = usedMem / (1024 * 1024 * 1024);
    }
    
    size = sizeof(info.coreCount);
    sysctlbyname("hw.ncpu", &info.coreCount, &size, NULL, 0);
    
    info.localIP = getLocalIP_macOS();
    info.cpuUsage = getCPUUsage_macOS();
    info.memoryUsage = getMemoryUsage_macOS();
    
    getBatteryInfo_macOS(info.batteryPercent, info.chargingStatus);
    
    return info;
}

#endif

// ============================================================================
// 主函数
// ============================================================================
int main() {
    std::cout << "╔══════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║           完整系统信息检测 (System Information)          ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════════════════════╝" << std::endl;
    std::cout << std::endl;
    
    std::cout << "【检测时间】" << std::endl;
    std::cout << "  当前时间: " << getCurrentTimeString() << std::endl;
    std::cout << std::endl;
    
    std::cout << "【编译环境】" << std::endl;
    std::cout << "  编译器: " << getCompilerInfo() << std::endl;
    std::cout << std::endl;
    
#if defined(_WIN32) || defined(_WIN64)
    SystemInfo info = getWindowsSystemInfo();
#elif defined(__linux__)
    SystemInfo info = getLinuxSystemInfo();
#elif defined(__APPLE__)
    SystemInfo info = getMacOSSystemInfo();
#else
    std::cout << "不支持的操作系统平台" << std::endl;
    return 1;
#endif
    
    std::cout << "【操作系统】" << std::endl;
    std::cout << "  系统环境: " << info.osName << std::endl;
    std::cout << "  本地IP地址: " << info.localIP << std::endl;
    std::cout << std::endl;
    
    std::cout << "【CPU 信息】" << std::endl;
    std::cout << "  CPU 厂商: " << info.cpuVendor << std::endl;
    std::cout << "  CPU 型号: " << info.cpuModel << std::endl;
    std::cout << "  CPU 核心数: " << info.coreCount << " 核心" << std::endl;
    std::cout << "  CPU 使用率: " << std::fixed << std::setprecision(1) << info.cpuUsage << "%" << std::endl;
    std::cout << std::endl;
    
    std::cout << "【内存信息】" << std::endl;
    std::cout << "  物理内存: " << info.totalMemoryGB << " GB" << std::endl;
    std::cout << "  已用内存: " << info.usedMemoryGB << " GB" << std::endl;
    std::cout << "  内存使用率: " << std::fixed << std::setprecision(1) << info.memoryUsage << "%" << std::endl;
    std::cout << std::endl;

    std::cout << "【电源/电池信息】" << std::endl;
    if (info.batteryPercent != -1) {
        std::cout << "  剩余电量: " << info.batteryPercent << "%" << std::endl;
    } else {
        std::cout << "  剩余电量: 未知" << std::endl;
    }
    std::cout << "  充电状态: " << info.chargingStatus << std::endl;
    std::cout << std::endl;
    
    std::cout << "═══════════════════════════════════════════════════════════" << std::endl;
    
    return 0;
}
