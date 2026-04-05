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

const char* TARGET_PKG = "com.tencent.tmgp.sgame"; 
const char* LIB_PATH = "/data/local/tmp/libJKMenu.so";

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

    ptrace(PTRACE_ATTACH, pid, NULL, NULL);
    waitpid(pid, NULL, WUNTRACED);

    struct user_pt_regs regs;
    struct iovec iov = { .iov_base = &regs, .iov_len = sizeof(regs) };
    ptrace(PTRACE_GETREGSET, pid, NT_PRSTATUS, &iov);

    // 获取远程 dlopen
    uintptr_t remote_dlopen = (uintptr_t)dlsym(RTL_DEFAULT, "__loader_dlopen"); 

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

    printf("[+] Injecting into SGame (PID: %d)...\n", pid);
    return 0;
}
