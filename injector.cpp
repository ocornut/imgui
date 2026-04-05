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
// 注意：根据你之前的记录，包名可能是 com.tencent.tmgp.sgame 或 com.tencent.jkchess
const char* TARGET_PKG = "com.tencent.tmgp.sgame"; 
const char* LIB_PATH   = "/data/1/libJKMenu.so";

// 获取模块基址 (支持 APEX 路径)
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

// 计算远程 dlopen 地址
uintptr_t get_remote_dlopen(pid_t pid) {
    const char* linker_name = "linker64";
    uintptr_t local_linker = get_module_base(getpid(), linker_name);
    uintptr_t remote_linker = get_module_base(pid, linker_name);

    if (!local_linker || !remote_linker) {
        printf("[-] Error: Linker not found. Local: %lx, Remote: %lx\n", local_linker, remote_linker);
        return 0;
    }

    void* local_func = dlsym(RTLD_DEFAULT, "__loader_dlopen");
    if (!local_func) local_func = dlsym(RTLD_DEFAULT, "dlopen");

    uintptr_t offset = (uintptr_t)local_func - local_linker;
    return remote_linker + offset;
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
    printf("[*] JKMenu Injector Starting...\n");
    
    pid_t pid = find_pid(TARGET_PKG);
    if (pid == -1) { 
        printf("[-] Target %s not found!\n", TARGET_PKG); 
        return 1; 
    }

    uintptr_t dlopen_addr = get_remote_dlopen(pid);
    if (!dlopen_addr) return 1;
    printf("[+] Remote dlopen: %lx\n", dlopen_addr);

    if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) < 0) {
        perror("[-] Attach failed");
        return 1;
    }
    waitpid(pid, NULL, WUNTRACED);
    printf("[+] Attached to %d\n", pid);

    struct user_pt_regs regs, old_regs;
    struct iovec iov = { .iov_base = &regs, .iov_len = sizeof(regs) };
    ptrace(PTRACE_GETREGSET, pid, (void*)(uintptr_t)NT_PRSTATUS, &iov);
    memcpy(&old_regs, &regs, sizeof(regs));

    uintptr_t sp_path = (regs.sp - 512) & ~0xF;
    size_t len = strlen(LIB_PATH) + 1;
    for (size_t i = 0; i < len; i += 8) {
        long data = 0;
        memcpy(&data, LIB_PATH + i, (len - i) < 8 ? (len - i) : 8);
        ptrace(PTRACE_POKETEXT, pid, (void*)(sp_path + i), (void*)data);
    }

    regs.regs[0] = sp_path;       
    regs.regs[1] = 2;             
    regs.pc = dlopen_addr;        
    regs.regs[30] = 0;            

    ptrace(PTRACE_SETREGSET, pid, (void*)(uintptr_t)NT_PRSTATUS, &iov);
    
    printf("[*] Calling remote dlopen...\n");
    ptrace(PTRACE_CONT, pid, NULL, NULL);

    int status;
    waitpid(pid, &status, WUNTRACED); 

    printf("[+] Restoring original state...\n");
    ptrace(PTRACE_SETREGSET, pid, (void*)(uintptr_t)NT_PRSTATUS, (struct iovec *)&old_regs);
    
    ptrace(PTRACE_DETACH, pid, NULL, NULL);
    printf("[+] Done!\n");

    return 0;
}
