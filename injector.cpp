#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/system_properties.h>
#include <android/log.h>
#include <thread>
#include <chrono>
#include <sstream>
#include <iomanip>

#define LOGI(...) printf("[+] " __VA_ARGS__); printf("\n")
#define LOGE(...) printf("[-] " __VA_ARGS__); printf("\n")
#define LOGD(...) printf("[*] " __VA_ARGS__); printf("\n")

// 目标包名
#define TARGET_PACKAGE "com.tencent.tmgp.sgame"

// 查找目标进程的PID
pid_t find_pid(const std::string& package_name) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "pidof %s", package_name.c_str());
    FILE* fp = popen(cmd, "r");
    if (!fp) return -1;
    
    pid_t pid = -1;
    fscanf(fp, "%d", &pid);
    pclose(fp);
    return pid;
}

// 等待目标进程启动，并返回其PID
pid_t wait_for_process(const std::string& package_name, int timeout_seconds = 30) {
    LOGI("Waiting for process '%s' to start...", package_name.c_str());
    auto start_time = std::chrono::steady_clock::now();
    
    while (true) {
        pid_t pid = find_pid(package_name);
        if (pid > 0) {
            LOGI("Found process '%s' with PID: %d", package_name.c_str(), pid);
            return pid;
        }
        
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - start_time).count() > timeout_seconds) {
            LOGE("Timeout waiting for process '%s'", package_name.c_str());
            return -1;
        }
        
        sleep(1);
    }
}

// 获取远程进程中的函数地址（例如 dlopen）
unsigned long get_remote_function_address(pid_t pid, const char* library_name, const char* function_name) {
    char maps_path[256];
    snprintf(maps_path, sizeof(maps_path), "/proc/%d/maps", pid);
    FILE* fp = fopen(maps_path, "r");
    if (!fp) {
        LOGE("Cannot open maps file for PID %d", pid);
        return 0;
    }
    
    unsigned long base_addr = 0;
    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, library_name)) {
            sscanf(line, "%lx-", &base_addr);
            break;
        }
    }
    fclose(fp);
    
    if (base_addr == 0) {
        LOGE("Library '%s' not found in target process", library_name);
        return 0;
    }
    
    // 从本地进程中获取函数偏移量
    void* local_handle = dlopen(library_name, RTLD_LAZY);
    if (!local_handle) {
        LOGE("Cannot load local library '%s': %s", library_name, dlerror());
        return 0;
    }
    
    void* local_func = dlsym(local_handle, function_name);
    if (!local_func) {
        LOGE("Cannot find function '%s' locally: %s", function_name, dlerror());
        dlclose(local_handle);
        return 0;
    }
    
    // 获取本地进程中库的基址
    FILE* local_maps = fopen("/proc/self/maps", "r");
    unsigned long local_base = 0;
    while (fgets(line, sizeof(line), local_maps)) {
        if (strstr(line, library_name)) {
            sscanf(line, "%lx-", &local_base);
            break;
        }
    }
    fclose(local_maps);
    
    if (local_base == 0) {
        LOGE("Cannot find local base address for '%s'", library_name);
        dlclose(local_handle);
        return 0;
    }
    
    unsigned long offset = (unsigned long)local_func - local_base;
    unsigned long remote_addr = base_addr + offset;
    
    LOGI("Remote %s!%s address: 0x%lx", library_name, function_name, remote_addr);
    dlclose(local_handle);
    return remote_addr;
}

// 写入远程内存
bool write_remote_memory(pid_t pid, unsigned long addr, const void* data, size_t size) {
    size_t i = 0;
    for (i = 0; i < size; i += sizeof(long)) {
        long word = 0;
        memcpy(&word, (const char*)data + i, sizeof(long));
        if (ptrace(PTRACE_POKETEXT, pid, (void*)(addr + i), (void*)word) == -1) {
            LOGE("ptrace POKETEXT failed at 0x%lx: %s", addr + i, strerror(errno));
            return false;
        }
    }
    return true;
}

// 在远程进程中分配内存
unsigned long remote_mmap(pid_t pid, unsigned long addr, size_t size, int prot, int flags, int fd, off_t offset) {
    unsigned long mmap_addr = get_remote_function_address(pid, "libc.so", "mmap");
    if (!mmap_addr) return 0;
    
    struct pt_regs regs, original_regs;
    // 保存原始寄存器状态
    if (ptrace(PTRACE_GETREGS, pid, nullptr, &original_regs) == -1) {
        LOGE("Failed to get original registers: %s", strerror(errno));
        return 0;
    }
    
    // 设置参数
    regs = original_regs;
#if defined(__arm__)
    regs.ARM_r0 = addr;          // 地址（通常为0，让系统选择）
    regs.ARM_r1 = size;          // 大小
    regs.ARM_r2 = prot;          // 保护标志
    regs.ARM_r3 = flags;         // 标志
    regs.ARM_r4 = fd;            // 文件描述符
    regs.ARM_r5 = offset;        // 偏移量
    regs.ARM_pc = mmap_addr;     // 设置PC为mmap函数地址
    regs.ARM_cpsr = original_regs.ARM_cpsr;
#elif defined(__aarch64__)
    regs.regs[0] = addr;
    regs.regs[1] = size;
    regs.regs[2] = prot;
    regs.regs[3] = flags;
    regs.regs[4] = fd;
    regs.regs[5] = offset;
    regs.pc = mmap_addr;
    regs.pstate = original_regs.pstate;
#endif
    
    if (ptrace(PTRACE_SETREGS, pid, nullptr, &regs) == -1) {
        LOGE("Failed to set registers for mmap: %s", strerror(errno));
        return 0;
    }
    
    // 单步执行mmap
    if (ptrace(PTRACE_SINGLESTEP, pid, nullptr, nullptr) == -1) {
        LOGE("Failed to single step for mmap: %s", strerror(errno));
        ptrace(PTRACE_SETREGS, pid, nullptr, &original_regs);
        return 0;
    }
    waitpid(pid, nullptr, 0);
    
    // 获取返回值
    if (ptrace(PTRACE_GETREGS, pid, nullptr, &regs) == -1) {
        LOGE("Failed to get return registers: %s", strerror(errno));
        ptrace(PTRACE_SETREGS, pid, nullptr, &original_regs);
        return 0;
    }
    
    unsigned long result;
#if defined(__arm__)
    result = regs.ARM_r0;
#elif defined(__aarch64__)
    result = regs.regs[0];
#endif
    
    // 恢复原始寄存器
    ptrace(PTRACE_SETREGS, pid, nullptr, &original_regs);
    
    if ((long)result <= 0) {
        LOGE("mmap failed with result 0x%lx", result);
        return 0;
    }
    
    LOGI("mmap allocated memory at 0x%lx", result);
    return result;
}

// 远程调用 dlopen
bool remote_dlopen(pid_t pid, const char* library_path) {
    unsigned long dlopen_addr = get_remote_function_address(pid, "libdl.so", "dlopen");
    if (!dlopen_addr) return false;
    
    // 在远程进程中分配内存，存储库路径
    size_t path_len = strlen(library_path) + 1;
    unsigned long remote_path = remote_mmap(pid, 0, path_len, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (!remote_path) {
        LOGE("Failed to allocate memory for library path");
        return false;
    }
    
    if (!write_remote_memory(pid, remote_path, library_path, path_len)) {
        LOGE("Failed to write library path to remote memory");
        return false;
    }
    
    LOGI("Library path written to remote memory at 0x%lx", remote_path);
    
    // 保存寄存器状态
    struct pt_regs regs, original_regs;
    if (ptrace(PTRACE_GETREGS, pid, nullptr, &original_regs) == -1) {
        LOGE("Failed to get original registers: %s", strerror(errno));
        return false;
    }
    
    // 设置参数并调用 dlopen
    regs = original_regs;
#if defined(__arm__)
    regs.ARM_r0 = remote_path;   // 第一个参数：库路径
    regs.ARM_r1 = RTLD_LAZY;     // 第二个参数：标志
    regs.ARM_pc = dlopen_addr;   // 设置PC为dlopen函数地址
#elif defined(__aarch64__)
    regs.regs[0] = remote_path;
    regs.regs[1] = RTLD_LAZY;
    regs.pc = dlopen_addr;
#endif
    
    if (ptrace(PTRACE_SETREGS, pid, nullptr, &regs) == -1) {
        LOGE("Failed to set registers for dlopen: %s", strerror(errno));
        return false;
    }
    
    // 单步执行 dlopen
    if (ptrace(PTRACE_SINGLESTEP, pid, nullptr, nullptr) == -1) {
        LOGE("Failed to single step for dlopen: %s", strerror(errno));
        ptrace(PTRACE_SETREGS, pid, nullptr, &original_regs);
        return false;
    }
    waitpid(pid, nullptr, 0);
    
    // 恢复寄存器
    ptrace(PTRACE_SETREGS, pid, nullptr, &original_regs);
    
    LOGI("dlopen called successfully");
    return true;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <library_path> [package_name]\n", argv[0]);
        printf("  library_path: Path to the .so file to inject\n");
        printf("  package_name: Target package name (default: %s)\n", TARGET_PACKAGE);
        return 1;
    }
    
    const char* library_path = argv[1];
    const char* package_name = (argc >= 3) ? argv[2] : TARGET_PACKAGE;
    
    LOGI("Starting injector for package: %s", package_name);
    LOGI("Library to inject: %s", library_path);
    
    // 等待目标进程启动
    pid_t target_pid = wait_for_process(package_name, 30);
    if (target_pid <= 0) {
        LOGE("Failed to find target process");
        return 1;
    }
    
    // 附加到目标进程
    if (ptrace(PTRACE_ATTACH, target_pid, nullptr, nullptr) == -1) {
        LOGE("Failed to attach to process %d: %s", target_pid, strerror(errno));
        return 1;
    }
    
    LOGI("Attached to process %d", target_pid);
    
    // 等待目标进程停止
    int status;
    waitpid(target_pid, &status, 0);
    
    // 注入并调用 dlopen
    if (remote_dlopen(target_pid, library_path)) {
        LOGI("Library injected successfully!");
    } else {
        LOGE("Failed to inject library");
    }
    
    // 分离并恢复进程
    if (ptrace(PTRACE_DETACH, target_pid, nullptr, nullptr) == -1) {
        LOGE("Failed to detach from process: %s", strerror(errno));
        return 1;
    }
    
    LOGI("Detached from process %d", target_pid);
    return 0;
}
