#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <dirent.h>
#include <dlfcn.h>
#include <sys/uio.h>
#include <elf.h>

#ifndef NT_PRSTATUS
#define NT_PRSTATUS 1
#endif

// 目标包名
const char* TARGET_PKG = "com.tencent.tmgp.sgame"; 
// 根据你的要求修改路径为 /data/1/libJKMenu.so
const char* LIB_PATH = "/data/1/libJKMenu.so";

// 获取远程进程中 linker 函数的地址
uintptr_t get_remote_linker_func(pid_t pid, const char* func_name) {
    void* handle = dlopen("linker64", RTLD_LAZY);
    if (!handle) handle = dlopen("linker", RTLD_LAZY);
    uintptr_t addr = (uintptr_t)dlsym(handle, func_name);
    if (handle) dlclose(handle);
    return addr;
}

// 查找进程 PID
pid_t find_pid(const char* pkg) {
    DIR* dir = opendir("/proc");
    if (!dir) return -1;
    struct dirent* entry;
    while ((entry = readdir(dir))) {
        pid_t pid = atoi(entry->d_name);
        if (pid <= 0) continue;
        char path[256], cmd[256];
        snprintf(path, sizeof(path), "/proc/%d/cmdline", pid);
        FILE* f = fopen(path, "r");
        if (f) {
            if (fgets(cmd, sizeof(cmd), f) && strstr(cmd, pkg)) {
                fclose(f); closedir(dir); return pid;
            }
            fclose(f);
        }
    }
    closedir(dir); return -1;
}

int main() {
    pid_t pid = find_pid(TARGET_PKG);
    if (pid == -1) { 
        printf("[-] Target process %s not found!\n", TARGET_PKG); 
        return 1; 
    }

    // 在高版本 Android (10+) 中通常需要 __loader_dlopen
    uintptr_t dlopen_addr = get_remote_linker_func(pid, "__loader_dlopen");
    if (!dlopen_addr) {
        printf("[!] Falling back to standard dlopen...\n");
        dlopen_addr = (uintptr_t)dlopen; 
    }

    if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) < 0) {
        perror("[-] Ptrace attach failed");
        return 1;
    }
    waitpid(pid, NULL, WUNTRACED);

    struct user_pt_regs regs;
    struct iovec iov = { .iov_base = &regs, .iov_len = sizeof(regs) };
    if (ptrace(PTRACE_GETREGSET, pid, (void*)(uintptr_t)NT_PRSTATUS, &iov) < 0) {
        perror("[-] Failed to get registers");
        ptrace(PTRACE_DETACH, pid, NULL, NULL);
        return 1;
    }

    // 在栈上分配空间存放 .so 路径 (从当前 SP 往下偏移)
    uintptr_t sp = (regs.sp - 512) & ~0xF;
    size_t len = strlen(LIB_PATH) + 1;
    for (size_t i = 0; i < len; i += 8) {
        long data = 0;
        memcpy(&data, LIB_PATH + i, (len - i) < 8 ? (len - i) : 8);
        ptrace(PTRACE_POKETEXT, pid, (void*)(sp + i), (void*)data);
    }

    // 设置寄存器，准备调用 dlopen(path, RTLD_NOW)
    // AArch64 调用约定: x0=arg1, x1=arg2
    regs.regs[0] = sp;       // 第一个参数: .so 路径
    regs.regs[1] = 2;        // 第二个参数: RTLD_NOW (2)
    regs.pc = dlopen_addr;   // 跳转执行 dlopen
    
    if (ptrace(PTRACE_SETREGSET, pid, (void*)(uintptr_t)NT_PRSTATUS, &iov) < 0) {
        perror("[-] Failed to set registers");
        ptrace(PTRACE_DETACH, pid, NULL, NULL);
        return 1;
    }

    ptrace(PTRACE_DETACH, pid, NULL, NULL);

    printf("[+] Target: %s (PID: %d)\n", TARGET_PKG, pid);
    printf("[+] Path: %s\n", LIB_PATH);
    printf("[+] Using dlopen address: %lx\n", (long)dlopen_addr);
    printf("[+] Done.\n");

    return 0;
}
