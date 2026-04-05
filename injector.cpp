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

// 目标包名 (根据你的配置)
const char* TARGET_PKG = "com.tencent.tmgp.sgame"; 
// 注入库路径
const char* LIB_PATH = "/data/1/libJKMenu.so";

// 获取进程中模块的基地址，增加对 APEX 路径的兼容
uintptr_t get_module_base(pid_t pid, const char* module_name) {
    FILE* fp;
    uintptr_t addr = 0;
    char filename[64];
    char line[1024];
    snprintf(filename, sizeof(filename), "/proc/%d/maps", pid);
    fp = fopen(filename, "r");
    if (fp) {
        while (fgets(line, sizeof(line), fp)) {
            // 搜索包含 linker 的路径并确保是可执行段 (r-xp)
            if (strstr(line, module_name) && strstr(line, "r-xp")) {
                addr = strtoull(line, NULL, 16);
                break;
            }
        }
        fclose(fp);
    }
    return addr;
}

// 寻找 dlopen 地址的核心逻辑
uintptr_t get_remote_dlopen(pid_t pid) {
    uintptr_t local_linker = 0;
    uintptr_t remote_linker = 0;

    // 适配 Android 10+ 的 APEX 路径和常规路径
    const char* linker_paths[] = {
        "bin/linker64",
        "bin/linker",
        "/apex/com.android.runtime/bin/linker64",
        "/apex/com.android.runtime/bin/linker"
    };

    for (int i = 0; i < 4; i++) {
        local_linker = get_module_base(getpid(), linker_paths[i]);
        remote_linker = get_module_base(pid, linker_paths[i]);
        if (local_linker && remote_linker) {
            printf("[+] Found Linker at: %s\n", linker_paths[i]);
            break;
        }
    }

    if (!local_linker || !remote_linker) {
        printf("[-] Error: Local Linker: %lx, Remote Linker: %lx\n", (long)local_linker, (long)remote_linker);
        return 0;
    }

    // 获取本地 __loader_dlopen 地址 (Android 8.0+)
    void* local_func = dlsym(RTLD_DEFAULT, "__loader_dlopen");
    if (!local_func) {
        local_func = dlsym(RTLD_DEFAULT, "dlopen");
    }
    
    // 终极保底：直接使用本地 dlopen 函数指针
    if (!local_func) {
        local_func = (void*)dlopen;
    }

    uintptr_t offset = (uintptr_t)local_func - local_linker;
    uintptr_t remote_addr = remote_linker + offset;

    printf("[+] Local Linker Base: %lx\n", (long)local_linker);
    printf("[+] Remote Linker Base: %lx\n", (long)remote_linker);
    printf("[+] Function Offset: %lx\n", (long)offset);

    return remote_addr;
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
    printf("[*] JKMenu Injector Starting...\n");
    pid_t pid = find_pid(TARGET_PKG);
    if (pid == -1) { 
        printf("[-] Target process %s not found!\n", TARGET_PKG); 
        return 1; 
    }
    printf("[+] Found Target PID: %d\n", pid);

    uintptr_t dlopen_addr = get_remote_dlopen(pid);
    if (!dlopen_addr) {
        printf("[-] Critical: Failed to calculate dlopen address!\n");
        return 1;
    }
    printf("[+] Calculated Remote dlopen: %lx\n", (long)dlopen_addr);

    // --- Ptrace 注入开始 ---
    if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) < 0) {
        perror("[-] Attach failed");
        return 1;
    }
    printf("[*] Waiting for process to stop...\n");
    waitpid(pid, NULL, WUNTRACED);

    struct user_pt_regs regs;
    struct iovec iov = { .iov_base = &regs, .iov_len = sizeof(regs) };
    if (ptrace(PTRACE_GETREGSET, pid, (void*)(uintptr_t)NT_PRSTATUS, &iov) < 0) {
        perror("[-] Get regs failed");
        ptrace(PTRACE_DETACH, pid, NULL, NULL);
        return 1;
    }

    // 准备栈空间存放路径
    uintptr_t sp = (regs.sp - 512) & ~0xF;
    size_t len = strlen(LIB_PATH) + 1;
    for (size_t i = 0; i < len; i += 8) {
        long data = 0;
        memcpy(&data, LIB_PATH + i, (len - i) < 8 ? (len - i) : 8);
        ptrace(PTRACE_POKETEXT, pid, (void*)(sp + i), (void*)data);
    }

    // 设置寄存器 (AArch64)
    regs.regs[0] = sp;       // x0: .so path
    regs.regs[1] = 2;        // x1: RTLD_NOW
    regs.pc = dlopen_addr;   // pc: call dlopen
    regs.regs[30] = 0;       // lr: exit on return

    if (ptrace(PTRACE_SETREGSET, pid, (void*)(uintptr_t)NT_PRSTATUS, &iov) < 0) {
        perror("[-] Set regs failed");
        ptrace(PTRACE_DETACH, pid, NULL, NULL);
        return 1;
    }

    printf("[*] Triggering remote load...\n");
    ptrace(PTRACE_CONT, pid, NULL, NULL);
    
    // 给系统加载 SO 留一点缓冲时间
    usleep(500000); 

    ptrace(PTRACE_DETACH, pid, NULL, NULL);
    printf("[+] Injection Done. Check logcat for 'libJKMenu' results.\n");

    return 0;
}
