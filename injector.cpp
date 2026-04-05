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

// 目标包名
const char* TARGET_PKG = "com.tencent.jkchess"; 
const char* LIB_PATH = "/data/local/tmp/libJKMenu.so";

pid_t find_pid_by_name(const char* package_name) {
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
            if (fgets(cmd, sizeof(cmd), f) && strstr(cmd, package_name)) {
                fclose(f); closedir(dir); return pid;
            }
            fclose(f);
        }
    }
    closedir(dir); return -1;
}

int main() {
    pid_t pid = find_pid_by_name(TARGET_PKG);
    if (pid == -1) { printf("[-] Target process %s not found!\n", TARGET_PKG); return 1; }

    if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) < 0) { perror("[-] Attach failed"); return 1; }
    waitpid(pid, NULL, WUNTRACED);

    struct user_pt_regs regs;
    struct iovec iov = { .iov_base = &regs, .iov_len = sizeof(regs) };
    ptrace(PTRACE_GETREGSET, pid, NT_PRSTATUS, &iov);

    // 简单获取远程 dlopen (在 linker64 中)
    uintptr_t remote_dlopen = (uintptr_t)dlsym(RTLD_DEFAULT, "__loader_dlopen"); 

    // 远程写入 SO 路径并调用
    uintptr_t sp = (regs.sp - 256) & ~0xF;
    size_t len = strlen(LIB_PATH) + 1;
    for (size_t i = 0; i < len; i += 8) {
        long data = 0;
        memcpy(&data, LIB_PATH + i, (len - i) < 8 ? (len - i) : 8);
        ptrace(PTRACE_POKETEXT, pid, sp + i, (void*)data);
    }

    regs.regs[0] = sp;  // x0: path
    regs.regs[1] = 2;   // x1: RTLD_NOW
    regs.pc = remote_dlopen;
    ptrace(PTRACE_SETREGSET, pid, NT_PRSTATUS, &iov);
    ptrace(PTRACE_DETACH, pid, NULL, NULL);

    printf("[+] Injection request sent to PID %d\n", pid);
    return 0;
}
