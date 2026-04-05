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
#include <sys/uio.h>
#include <elf.h>
#include <asm/ptrace.h>
#include <thread>
#include <chrono>

#define LOGI(...) printf("[+] " __VA_ARGS__), printf("\n")
#define LOGE(...) printf("[-] " __VA_ARGS__), printf("\n")

#define TARGET_PACKAGE "com.tencent.tmgp.sgame"

// 查找进程 PID
pid_t find_pid(const char* package_name) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "pidof %s", package_name);
    FILE* fp = popen(cmd, "r");
    if (!fp) return -1;
    pid_t pid = -1;
    fscanf(fp, "%d", &pid);
    pclose(fp);
    return pid;
}

// 等待进程启动
pid_t wait_for_process(const char* package_name, int timeout_sec = 30) {
    LOGI("Waiting for process '%s'...", package_name);
    auto start = std::chrono::steady_clock::now();
    while (true) {
        pid_t pid = find_pid(package_name);
        if (pid > 0) return pid;
        if (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start).count() > timeout_sec) {
            LOGE("Timeout waiting for %s", package_name);
            return -1;
        }
        sleep(1);
    }
}

// 获取远程进程中的函数地址（通过本地偏移计算）
unsigned long get_remote_func_addr(pid_t pid, const char* lib, const char* func) {
    // 1. 读取远程进程的 /proc/pid/maps 找到 lib 的基址
    char maps_path[64];
    snprintf(maps_path, sizeof(maps_path), "/proc/%d/maps", pid);
    FILE* fp = fopen(maps_path, "r");
    if (!fp) return 0;
    unsigned long base = 0;
    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, lib)) {
            sscanf(line, "%lx-", &base);
            break;
        }
    }
    fclose(fp);
    if (!base) return 0;

    // 2. 本地获取函数偏移
    void* handle = dlopen(lib, RTLD_LAZY);
    if (!handle) return 0;
    void* local_func = dlsym(handle, func);
    dlclose(handle);
    if (!local_func) return 0;

    // 3. 获取本地 lib 基址
    FILE* self_maps = fopen("/proc/self/maps", "r");
    unsigned long local_base = 0;
    while (fgets(line, sizeof(line), self_maps)) {
        if (strstr(line, lib)) {
            sscanf(line, "%lx-", &local_base);
            break;
        }
    }
    fclose(self_maps);
    if (!local_base) return 0;

    unsigned long offset = (unsigned long)local_func - local_base;
    unsigned long remote_addr = base + offset;
    LOGI("Found %s!%s at 0x%lx", lib, func, remote_addr);
    return remote_addr;
}

// 写入远程内存
bool write_remote(pid_t pid, unsigned long addr, const void* data, size_t len) {
    for (size_t i = 0; i < len; i += sizeof(long)) {
        long word = 0;
        memcpy(&word, (char*)data + i, sizeof(long));
        if (ptrace(PTRACE_POKETEXT, pid, (void*)(addr + i), (void*)word) == -1) {
            LOGE("ptrace POKETEXT failed at 0x%lx", addr + i);
            return false;
        }
    }
    return true;
}

// 远程调用 mmap 分配内存
unsigned long remote_mmap(pid_t pid, size_t size) {
    unsigned long mmap_addr = get_remote_func_addr(pid, "libc.so", "mmap");
    if (!mmap_addr) return 0;

    struct iovec iov;
    struct user_pt_regs regs, old_regs;

    // 保存原寄存器
    iov.iov_base = &old_regs;
    iov.iov_len = sizeof(old_regs);
    if (ptrace(PTRACE_GETREGSET, pid, NT_PRSTATUS, &iov) == -1) {
        LOGE("PTRACE_GETREGSET failed: %s", strerror(errno));
        return 0;
    }

    // 设置 mmap 参数 (arm64)
    regs = old_regs;
    regs.regs[0] = 0;               // addr (NULL)
    regs.regs[1] = size;            // length
    regs.regs[2] = PROT_READ | PROT_WRITE; // prot
    regs.regs[3] = MAP_ANONYMOUS | MAP_PRIVATE; // flags
    regs.regs[4] = -1;              // fd
    regs.regs[5] = 0;               // offset
    regs.pc = mmap_addr;

    iov.iov_base = &regs;
    if (ptrace(PTRACE_SETREGSET, pid, NT_PRSTATUS, &iov) == -1) {
        LOGE("PTRACE_SETREGSET failed: %s", strerror(errno));
        return 0;
    }

    // 单步执行
    if (ptrace(PTRACE_SINGLESTEP, pid, NULL, NULL) == -1) {
        LOGE("PTRACE_SINGLESTEP failed: %s", strerror(errno));
        iov.iov_base = &old_regs;
        ptrace(PTRACE_SETREGSET, pid, NT_PRSTATUS, &iov);
        return 0;
    }
    waitpid(pid, NULL, 0);

    // 获取返回值
    iov.iov_base = &regs;
    if (ptrace(PTRACE_GETREGSET, pid, NT_PRSTATUS, &iov) == -1) {
        LOGE("Failed to get return registers");
        iov.iov_base = &old_regs;
        ptrace(PTRACE_SETREGSET, pid, NT_PRSTATUS, &iov);
        return 0;
    }
    unsigned long result = regs.regs[0];

    // 恢复寄存器
    iov.iov_base = &old_regs;
    ptrace(PTRACE_SETREGSET, pid, NT_PRSTATUS, &iov);

    if ((long)result <= 0) {
        LOGE("mmap returned 0x%lx", result);
        return 0;
    }
    LOGI("mmap allocated at 0x%lx", result);
    return result;
}

// 远程调用 dlopen
bool remote_dlopen(pid_t pid, const char* path) {
    unsigned long dlopen_addr = get_remote_func_addr(pid, "libdl.so", "dlopen");
    if (!dlopen_addr) return false;

    // 分配内存存放路径
    size_t len = strlen(path) + 1;
    unsigned long remote_path = remote_mmap(pid, len);
    if (!remote_path) return false;
    if (!write_remote(pid, remote_path, path, len)) return false;

    struct iovec iov;
    struct user_pt_regs regs, old_regs;

    iov.iov_base = &old_regs;
    iov.iov_len = sizeof(old_regs);
    if (ptrace(PTRACE_GETREGSET, pid, NT_PRSTATUS, &iov) == -1) {
        LOGE("Failed to save registers");
        return false;
    }

    regs = old_regs;
    regs.regs[0] = remote_path;
    regs.regs[1] = RTLD_LAZY;
    regs.pc = dlopen_addr;

    iov.iov_base = &regs;
    if (ptrace(PTRACE_SETREGSET, pid, NT_PRSTATUS, &iov) == -1) {
        LOGE("Failed to set registers for dlopen");
        return false;
    }

    if (ptrace(PTRACE_SINGLESTEP, pid, NULL, NULL) == -1) {
        LOGE("PTRACE_SINGLESTEP failed");
        iov.iov_base = &old_regs;
        ptrace(PTRACE_SETREGSET, pid, NT_PRSTATUS, &iov);
        return false;
    }
    waitpid(pid, NULL, 0);

    // 恢复寄存器
    iov.iov_base = &old_regs;
    ptrace(PTRACE_SETREGSET, pid, NT_PRSTATUS, &iov);

    LOGI("dlopen executed");
    return true;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <library_path> [package_name]\n", argv[0]);
        return 1;
    }
    const char* libpath = argv[1];
    const char* pkg = (argc >= 3) ? argv[2] : TARGET_PACKAGE;

    LOGI("Target package: %s", pkg);
    LOGI("Library: %s", libpath);

    pid_t pid = wait_for_process(pkg, 30);
    if (pid <= 0) return 1;

    if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) == -1) {
        LOGE("PTRACE_ATTACH failed: %s", strerror(errno));
        return 1;
    }
    LOGI("Attached to PID %d", pid);
    waitpid(pid, NULL, 0);

    if (remote_dlopen(pid, libpath)) {
        LOGI("Injection successful!");
    } else {
        LOGE("Injection failed");
    }

    ptrace(PTRACE_DETACH, pid, NULL, NULL);
    return 0;
}
