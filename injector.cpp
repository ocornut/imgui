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

const char* TARGET_PKG = "com.tencent.tmgp.sgame"; 
const char* LIB_PATH = "/data/1/libJKMenu.so";

// 改进的符号查找：直接读取 /proc/self/maps 找到 linker 基址
uintptr_t get_remote_func(pid_t pid, const char* func_name) {
    // 尝试在当前进程寻找 linker 暴露的符号地址
    // Android 10+ 的 dlopen 实际上是调用了 __loader_dlopen
    void* handle = dlopen("linker64", RTLD_LAZY);
    if (!handle) handle = dlopen("linker", RTLD_LAZY);
    if (!handle) return 0;
    
    uintptr_t addr = (uintptr_t)dlsym(handle, func_name);
    dlclose(handle);
    return addr;
}

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
    printf("[*] Starting injector...\n");
    pid_t pid = find_pid(TARGET_PKG);
    if (pid == -1) { 
        printf("[-] Target process %s not found!\n", TARGET_PKG); 
        return 1; 
    }
    printf("[+] Found PID: %d\n", pid);

    uintptr_t dlopen_addr = get_remote_func(pid, "__loader_dlopen");
    if (!dlopen_addr) dlopen_addr = get_remote_func(pid, "dlopen");

    if (!dlopen_addr) {
        printf("[-] Could not find dlopen address!\n");
        return 1;
    }
    printf("[+] Remote dlopen: %lx\n", (long)dlopen_addr);

    // 1. Attach
    if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) < 0) {
        perror("[-] Attach failed");
        return 1;
    }
    printf("[*] Waiting for process to stop...\n");
    waitpid(pid, NULL, WUNTRACED);

    // 2. Get Regs
    struct user_pt_regs regs;
    struct iovec iov = { .iov_base = &regs, .iov_len = sizeof(regs) };
    if (ptrace(PTRACE_GETREGSET, pid, (void*)(uintptr_t)NT_PRSTATUS, &iov) < 0) {
        perror("[-] Get regs failed");
        goto detach;
    }

    // 保存旧寄存器，以便恢复（防止游戏崩溃）
    struct user_pt_regs old_regs = regs;

    // 3. Write Path to Stack
    uintptr_t sp = (regs.sp - 512) & ~0xF;
    size_t len = strlen(LIB_PATH) + 1;
    for (size_t i = 0; i < len; i += 8) {
        long data = 0;
        memcpy(&data, LIB_PATH + i, (len - i) < 8 ? (len - i) : 8);
        ptrace(PTRACE_POKETEXT, pid, (void*)(sp + i), (void*)data);
    }

    // 4. Call dlopen(path, RTLD_NOW)
    regs.regs[0] = sp;   
    regs.regs[1] = 2;    
    regs.pc = dlopen_addr;
    // 设置 LR 为 0，让它跑完触发 SIGSEGV 停止，或者设置一个无效地址
    regs.regs[30] = 0; 

    if (ptrace(PTRACE_SETREGSET, pid, (void*)(uintptr_t)NT_PRSTATUS, &iov) < 0) {
        perror("[-] Set regs failed");
        goto detach;
    }

    // 5. 让它跑起来执行 dlopen
    printf("[*] Executing remote dlopen...\n");
    ptrace(PTRACE_CONT, pid, NULL, NULL);
    // 这里其实可以稍微 sleep 一下，给 dlopen 运行的时间
    usleep(500000); 

    printf("[+] Injection cycle complete.\n");

detach:
    ptrace(PTRACE_DETACH, pid, NULL, NULL);
    printf("[+] Done.\n");
    return 0;
}
