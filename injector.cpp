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

const char* TARGET_PKG = "com.tencent.jkchess"; 
const char* LIB_PATH = "/data/local/tmp/libJKMenu.so";

pid_t find_pid() {
    DIR* dir = opendir("/proc");
    struct dirent* entry;
    while ((entry = readdir(dir))) {
        pid_t pid = atoi(entry->d_name);
        if (pid <= 0) continue;
        char path[256], cmd[256];
        snprintf(path, sizeof(path), "/proc/%d/cmdline", pid);
        FILE* f = fopen(path, "r");
        if (f) {
            if (fgets(cmd, sizeof(cmd), f) && strstr(cmd, TARGET_PKG)) {
                fclose(f); closedir(dir); return pid;
            }
            fclose(f);
        }
    }
    closedir(dir); return -1;
}

int main() {
    pid_t pid = find_pid();
    if (pid == -1) { printf("Process not found!\n"); return 1; }

    ptrace(PTRACE_ATTACH, pid, NULL, NULL);
    waitpid(pid, NULL, WUNTRACED);

    struct user_pt_regs regs;
    struct iovec iov = { .iov_base = &regs, .iov_len = sizeof(regs) };
    ptrace(PTRACE_GETREGSET, pid, NT_PRSTATUS, &iov);

    // 远程定位 dlopen (简单示例逻辑)
    uintptr_t remote_dlopen = (uintptr_t)dlsym(RTLD_DEFAULT, "__loader_dlopen"); 

    // 写入路径并调用 (x0=path, x1=RTLD_NOW)
    uintptr_t sp = (regs.sp - 256) & ~0xF;
    for (int i = 0; i < strlen(LIB_PATH) + 1; i += 8) {
        ptrace(PTRACE_POKETEXT, pid, sp + i, *(long*)(LIB_PATH + i));
    }

    regs.regs[0] = sp;
    regs.regs[1] = 2; // RTLD_NOW
    regs.pc = remote_dlopen;
    ptrace(PTRACE_SETREGSET, pid, NT_PRSTATUS, &iov);
    ptrace(PTRACE_DETACH, pid, NULL, NULL);

    printf("Injection command sent to PID %d\n", pid);
    return 0;
}
