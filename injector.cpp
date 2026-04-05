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
const char* LIB_PATH = "/data/local/tmp/libJKMenu.so";

// 这里的获取方式更适合 Android 高版本
uintptr_t get_remote_linker_func(pid_t pid, const char* func_name) {
    // 尝试直接获取本地地址（通常 linker 加载地址在所有进程中是一致的）
    void* handle = dlopen("linker64", RTLD_LAZY);
    if (!handle) handle = dlopen("linker", RTLD_LAZY);
    uintptr_t addr = (uintptr_t)dlsym(handle, func_name);
    if (handle) dlclose(handle);
    return addr;
}

pid_t find_pid(const char* pkg) {
    DIR* dir = opendir("/proc");
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
    if (pid == -1) { printf("[-] SGame not found!\n"); return 1; }

    // 获取 dlopen 地址 (尝试 __loader_dlopen)
    uintptr_t dlopen_addr = get_remote_linker_func(pid, "__loader_dlopen");
    if (!dlopen_addr) dlopen_addr = (uintptr_t)dlopen; // 最后的保底

    if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) < 0) {
        perror("[-] Attach failed");
        return 1;
    }
    waitpid(pid, NULL, WUNTRACED);

    struct user_pt_regs regs;
    struct iovec iov = { .iov_base = &regs, .iov_len = sizeof(regs) };
    ptrace(PTRACE_GETREGSET, pid, (void*)(uintptr_t)NT_PRSTATUS, &iov);

    // 写入路径
    uintptr_t sp = (regs.sp - 512) & ~0xF;
    size_t len = strlen(LIB_PATH) + 1;
    for (size_t i = 0; i < len; i += 8) {
        long data = 0;
        memcpy(&data, LIB_PATH + i, (len - i) < 8 ? (len - i) : 8);
        ptrace(PTRACE_POKETEXT, pid, (void*)(sp + i), (void*)data);
    }

    // 远程调用 dlopen(path, RTLD_NOW)
    regs.regs[0] = sp; 
    regs.regs[1] = 2; 
    regs.pc = dlopen_addr;
    
    ptrace(PTRACE_SETREGSET, pid, (void*)(uintptr_t)NT_PRSTATUS, &iov);
    ptrace(PTRACE_DETACH, pid, NULL, NULL);

    printf("[+] Injecting into SGame (PID: %d)...\n", pid);
    printf("[+] Remote dlopen address: %lx\n", (long)dlopen_addr);
    return 0;
}
