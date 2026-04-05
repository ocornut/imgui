#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
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

// --- 配置区 ---
const char* TARGET_PKG = "com.tencent.tmgp.sgame"; 
const char* LIB_PATH   = "/data/1/libJKMenu.so";

uintptr_t get_module_base(pid_t pid, const char* module_name) {
    FILE* fp;
    uintptr_t addr = 0;
    char filename[64], line[1024];
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

uintptr_t get_remote_dlopen(pid_t pid) {
    const char* linker_name = "linker64";
    uintptr_t local_linker = get_module_base(getpid(), linker_name);
    uintptr_t remote_linker = get_module_base(pid, linker_name);
    if (!local_linker || !remote_linker) return 0;

    void* local_func = dlsym(RTLD_DEFAULT, "__loader_dlopen");
    if (!local_func) local_func = dlsym(RTLD_DEFAULT, "dlopen");

    return remote_linker + ((uintptr_t)local_func - local_linker);
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
    printf("[*] JKMenu Injector v3.0 (BRK Trap Mode)\n");
    
    pid_t pid = find_pid(TARGET_PKG);
    if (pid == -1) { printf("[-] Target not found!\n"); return 1; }

    uintptr_t dlopen_addr = get_remote_dlopen(pid);
    if (!dlopen_addr) return 1;

    if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) < 0) { perror("[-] Attach"); return 1; }
    waitpid(pid, NULL, WUNTRACED);

    struct user_pt_regs regs, old_regs;
    struct iovec iov = { .iov_base = &regs, .iov_len = sizeof(regs) };
    ptrace(PTRACE_GETREGSET, pid, (void*)(uintptr_t)NT_PRSTATUS, &iov);
    memcpy(&old_regs, &regs, sizeof(regs));

    // 1. 准备栈空间：写入 SO 路径和断点指令
    uintptr_t sp_base = (regs.sp - 512) & ~0xF;
    uintptr_t brk_addr = sp_base;             // 断点指令位置
    uintptr_t path_addr = sp_base + 8;        // 路径字符串位置

    // 写入 AArch64 断点指令: brk #0 (0xd4200000)
    uint32_t brk_ins = 0xd4200000;
    ptrace(PTRACE_POKETEXT, pid, (void*)brk_addr, (void*)(uintptr_t)brk_ins);

    // 写入 SO 路径
    size_t len = strlen(LIB_PATH) + 1;
    for (size_t i = 0; i < len; i += 8) {
        long data = 0;
        memcpy(&data, LIB_PATH + i, (len - i) < 8 ? (len - i) : 8);
        ptrace(PTRACE_POKETEXT, pid, (void*)(path_addr + i), (void*)data);
    }

    // 2. 设置寄存器：调用 dlopen(path, RTLD_NOW)
    regs.regs[0] = path_addr;     // x0 = path
    regs.regs[1] = 2;             // x1 = RTLD_NOW
    regs.pc = dlopen_addr;        // pc = dlopen
    regs.regs[30] = brk_addr;     // lr = 断点地址 (执行完跳到这里)

    ptrace(PTRACE_SETREGSET, pid, (void*)(uintptr_t)NT_PRSTATUS, &iov);
    
    printf("[*] Executing remote dlopen...\n");
    ptrace(PTRACE_CONT, pid, NULL, NULL);

    // 3. 等待断点信号 (SIGTRAP)
    int status;
    waitpid(pid, &status, WUNTRACED);

    if (WIFSTOPPED(status) && (WSTOPSIG(status) == SIGTRAP || WSTOPSIG(status) == SIGILL)) {
        printf("[+] dlopen call finished successfully.\n");
    } else {
        printf("[!] Warning: Process stopped with signal %d\n", WSTOPSIG(status));
    }

    // 4. 彻底恢复原始现场
    printf("[+] Restoring registers and detaching...\n");
    ptrace(PTRACE_SETREGSET, pid, (void*)(uintptr_t)NT_PRSTATUS, (struct iovec *)&old_regs);
    ptrace(PTRACE_DETACH, pid, NULL, NULL);
    
    printf("[+] Injection Complete!\n");
    return 0;
}
