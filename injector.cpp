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
// 注入库路径
const char* LIB_PATH = "/data/1/libJKMenu.so";

// 获取进程中模块的基地址
uintptr_t get_module_base(pid_t pid, const char* module_name) {
    FILE* fp;
    uintptr_t addr = 0;
    char filename[32];
    char line[1024];
    snprintf(filename, sizeof(filename), "/proc/%d/maps", pid);
    fp = fopen(filename, "r");
    if (fp) {
        while (fgets(line, sizeof(line), fp)) {
            if (strstr(line, module_name)) {
                addr = strtoull(line, NULL, 16);
                break;
            }
        }
        fclose(fp);
    }
    return addr;
}

// 获取远程进程中 linker 函数的绝对地址
uintptr_t get_remote_dlopen(pid_t pid) {
    uintptr_t local_linker = 0;
    uintptr_t remote_linker = 0;
    const char* linker_path = NULL;

    // 尝试定位 64位 或 32位 linker
    if ((local_linker = get_module_base(getpid(), "linker64"))) {
        remote_linker = get_module_base(pid, "linker64");
        linker_path = "/system/bin/linker64";
    } else if ((local_linker = get_module_base(getpid(), "linker"))) {
        remote_linker = get_module_base(pid, "linker");
        linker_path = "/system/bin/linker";
    }

    if (!local_linker || !remote_linker) {
        printf("[-] Could not locate linker in maps.\n");
        return 0;
    }

    // 优先寻找 __loader_dlopen (Android 8+)
    void* handle = dlopen(linker_path, RTLD_LAZY);
    if (!handle) return 0;
    
    void* local_func = dlsym(handle, "__loader_dlopen");
    if (!local_func) local_func = dlsym(handle, "dlopen");
    
    // 终极回退方案：如果 dlsym 被拦截，直接取当前进程 dlopen 的内存地址
    if (!local_func) {
        local_func = (void*)dlopen;
    }
    
    dlclose(handle);

    // 计算偏移并应用到远程基址
    uintptr_t offset = (uintptr_t)local_func - local_linker;
    return remote_linker + offset;
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

    uintptr_t dlopen_addr = get_remote_dlopen(pid);
    if (!dlopen_addr) {
        printf("[-] Failed to calculate dlopen address!\n");
        return 1;
    }
    printf("[+] Remote dlopen address: %lx\n", (long)dlopen_addr);

    // 1. Attach 目标进程
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

    // 2. 在栈上写入 .so 路径 (SP - 512 字节安全区)
    uintptr_t sp = (regs.sp - 512) & ~0xF;
    size_t len = strlen(LIB_PATH) + 1;
    for (size_t i = 0; i < len; i += 8) {
        long data = 0;
        memcpy(&data, LIB_PATH + i, (len - i) < 8 ? (len - i) : 8);
        ptrace(PTRACE_POKETEXT, pid, (void*)(sp + i), (void*)data);
    }

    // 3. 修改寄存器模拟函数调用
    // AArch64 调用约定: X0 = 第一个参数, X1 = 第二个参数, PC = 函数地址
    regs.regs[0] = sp;       // 参数 1: SO 路径
    regs.regs[1] = 2;        // 参数 2: RTLD_NOW
    regs.pc = dlopen_addr;   // 跳转执行
    regs.regs[30] = 0;       // LR = 0, 函数返回时会崩溃并被 ptrace 捕获

    if (ptrace(PTRACE_SETREGSET, pid, (void*)(uintptr_t)NT_PRSTATUS, &iov) < 0) {
        perror("[-] Set regs failed");
        ptrace(PTRACE_DETACH, pid, NULL, NULL);
        return 1;
    }

    // 4. 继续运行并脱离
    printf("[*] Triggering dlopen in target...\n");
    ptrace(PTRACE_CONT, pid, NULL, NULL);
    usleep(500000); 

    ptrace(PTRACE_DETACH, pid, NULL, NULL);
    printf("[+] Injection cycle finished.\n");
    return 0;
}
