#include <iostream>
#include <string>
#include <vector>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <sys/uio.h>
#include <sys/mman.h>
#include <dlfcn.h>
#include <dirent.h>
#include <unistd.h>
#include <cstring>
#include <android/log.h>
#include <errno.h>
#include <elf.h>       // 修复 NT_PRSTATUS 未定义问题
#include <linux/elf.h> // 备用包含

#define TAG "AndKitty_Injector"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

// 确保 NT_PRSTATUS 在某些 NDK 版本中被正确定义
#ifndef NT_PRSTATUS
#define NT_PRSTATUS 1
#endif

using namespace std;

// 获取目标进程 PID
pid_t get_pid(const char* package_name) {
    DIR* dir = opendir("/proc");
    if (!dir) return -1;
    struct dirent* entry;
    while ((entry = readdir(dir))) {
        pid_t pid = atoi(entry->d_name);
        if (pid <= 0) continue;
        char path[256];
        snprintf(path, sizeof(path), "/proc/%d/cmdline", pid);
        FILE* f = fopen(path, "r");
        if (f) {
            char cmdline[256];
            if (fgets(cmdline, sizeof(cmdline), f)) {
                if (strncmp(cmdline, package_name, strlen(package_name)) == 0) {
                    fclose(f);
                    closedir(dir);
                    return pid;
                }
            }
            fclose(f);
        }
    }
    closedir(dir);
    return -1;
}

// 获取远程基址
uintptr_t get_module_base(pid_t pid, const char* module_name) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/maps", pid);
    FILE* f = fopen(path, "r");
    if (!f) return 0;
    char line[512];
    uintptr_t base = 0;
    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, module_name)) {
            base = (uintptr_t)strtoull(line, NULL, 16);
            break;
        }
    }
    fclose(f);
    return base;
}

// 获取远程函数地址
uintptr_t get_remote_addr(pid_t pid, const char* module_name, void* local_addr) {
    uintptr_t local_base = get_module_base(getpid(), module_name);
    uintptr_t remote_base = get_module_base(pid, module_name);
    if (!local_base || !remote_base) return 0;
    return remote_base + ((uintptr_t)local_addr - local_base);
}

// Ptrace 写内存
bool ptrace_write(pid_t pid, uintptr_t addr, void* buf, size_t len) {
    size_t i;
    long* lbuf = (long*)buf;
    for (i = 0; i < len; i += sizeof(long)) {
        if (ptrace(PTRACE_POKETEXT, pid, addr + i, lbuf[i / sizeof(long)]) < 0) return false;
    }
    return true;
}

// Ptrace 调用远程函数 (ARM64)
uint64_t ptrace_call(pid_t pid, uintptr_t func_addr, uint64_t* args, int nargs) {
    struct user_pt_regs regs, old_regs;
    struct iovec iov;
    
    iov.iov_base = &regs;
    iov.iov_len = sizeof(struct user_pt_regs);
    if (ptrace(PTRACE_GETREGSET, pid, (void*)NT_PRSTATUS, &iov) < 0) return 0;
    
    memcpy(&old_regs, &regs, sizeof(regs));

    for (int i = 0; i < nargs && i < 8; i++) regs.regs[i] = args[i];
    regs.regs[30] = 0; // LR set to 0 to trigger crash/stop
    regs.pc = func_addr;

    iov.iov_base = &regs;
    if (ptrace(PTRACE_SETREGSET, pid, (void*)NT_PRSTATUS, &iov) < 0) return 0;
    
    ptrace(PTRACE_CONT, pid, NULL, NULL);
    waitpid(pid, NULL, WUNTRACED);

    ptrace(PTRACE_GETREGSET, pid, (void*)NT_PRSTATUS, &iov);
    uint64_t res = regs.regs[0];
    
    iov.iov_base = &old_regs;
    ptrace(PTRACE_SETREGSET, pid, (void*)NT_PRSTATUS, &iov);
    return res;
}

int main(int argc, char** argv) {
    if (argc < 3) {
        printf("Usage: ./injector <package> <so_path>\n");
        return -1;
    }

    const char* pkg = argv[1];
    const char* so_path = argv[2];

    LOGI("Starting injection: %s -> %s", pkg, so_path);

    pid_t pid = get_pid(pkg);
    if (pid < 0) {
        LOGE("Process PID not found for %s", pkg);
        return -1;
    }

    if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) < 0) {
        LOGE("Ptrace Attach failed: %s", strerror(errno));
        return -1;
    }
    waitpid(pid, NULL, WUNTRACED);
    LOGI("Attached to PID: %d", pid);

    uintptr_t remote_dlopen = get_remote_addr(pid, "libdl.so", (void*)dlopen);
    if (!remote_dlopen) remote_dlopen = get_remote_addr(pid, "linker64", (void*)dlopen);

    if (!remote_dlopen) {
        LOGE("Could not locate remote dlopen");
        ptrace(PTRACE_DETACH, pid, NULL, NULL);
        return -1;
    }
    LOGI("Remote dlopen: %p", (void*)remote_dlopen);

    struct user_pt_regs regs;
    struct iovec iov = {&regs, sizeof(struct user_pt_regs)};
    if (ptrace(PTRACE_GETREGSET, pid, (void*)NT_PRSTATUS, &iov) < 0) {
        LOGE("GetRegset failed");
        ptrace(PTRACE_DETACH, pid, NULL, NULL);
        return -1;
    }
    
    uintptr_t remote_str_addr = regs.sp - 1024; 

    if (!ptrace_write(pid, remote_str_addr, (void*)so_path, strlen(so_path) + 1)) {
        LOGE("Write remote memory failed");
        ptrace(PTRACE_DETACH, pid, NULL, NULL);
        return -1;
    }

    uint64_t args[2] = { remote_str_addr, RTLD_NOW };
    LOGI("Calling remote dlopen...");
    uint64_t handle = ptrace_call(pid, remote_dlopen, args, 2);

    if (handle == 0) {
        LOGE("Injection failed: dlopen returned NULL");
    } else {
        LOGI("Injection success! Handle: %p", (void*)handle);
    }

    ptrace(PTRACE_DETACH, pid, NULL, NULL);
    LOGI("Detached.");
    return 0;
}
