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
#include <android/log.h>

#define LOG_TAG "JKInjector"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

/**
 * Find process ID by package name
 */
pid_t find_pid(const char* process_name) {
    DIR* dir = opendir("/proc");
    if (!dir) return -1;
    struct dirent* entry;
    while ((entry = readdir(dir))) {
        size_t pid = atoi(entry->d_name);
        if (pid <= 0) continue;
        char path[256], cmdline[256];
        snprintf(path, sizeof(path), "/proc/%zu/cmdline", pid);
        FILE* f = fopen(path, "r");
        if (f) {
            if (fgets(cmdline, sizeof(cmdline), f)) {
                if (strcmp(cmdline, process_name) == 0) {
                    fclose(f);
                    closedir(dir);
                    return (pid_t)pid;
                }
            }
            fclose(f);
        }
    }
    closedir(dir);
    return -1;
}

/**
 * Get module base address in remote process
 */
uintptr_t get_module_base(pid_t pid, const char* module_name) {
    char path[256];
    char line[512];
    uintptr_t base = 0;
    snprintf(path, sizeof(path), "/proc/%d/maps", pid);
    FILE* f = fopen(path, "r");
    if (!f) return 0;
    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, module_name)) {
            base = strtoull(line, NULL, 16);
            break;
        }
    }
    fclose(f);
    return base;
}

/**
 * Core Injection Logic using PTRACE
 */
int inject_so(pid_t pid, const char* so_path) {
    printf("[+] Target PID: %d\n", pid);
    
    // 1. Attach to process
    if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) < 0) {
        perror("[-] PTRACE_ATTACH failed");
        return -1;
    }
    waitpid(pid, NULL, WUNTRACED);
    printf("[+] Attached successfully\n");

    // 2. Backup registers
    struct user_pt_regs regs, old_regs;
    struct iovec iov = { .iov_base = &regs, .iov_len = sizeof(regs) };
    ptrace(PTRACE_GETREGSET, pid, NT_PRSTATUS, &iov);
    memcpy(&old_regs, &regs, sizeof(regs));

    // 3. Locate remote dlopen (via linker64)
    uintptr_t local_linker = get_module_base(getpid(), "linker64");
    uintptr_t remote_linker = get_module_base(pid, "linker64");
    
    // Modern Android uses __loader_dlopen inside linker64
    uintptr_t local_dlopen = (uintptr_t)dlsym(RTLD_DEFAULT, "__loader_dlopen");
    if (!local_dlopen) {
        local_dlopen = (uintptr_t)dlsym(RTLD_DEFAULT, "dlopen");
    }
    
    uintptr_t remote_dlopen = local_dlopen - local_linker + remote_linker;
    printf("[+] Calculated remote dlopen: %p\n", (void*)remote_dlopen);

    // 4. Write SO path to remote stack
    uintptr_t sp = regs.sp - 512; // Buffer area
    size_t path_len = strlen(so_path) + 1;
    for (size_t i = 0; i < path_len; i += 8) {
        long data = 0;
        memcpy(&data, so_path + i, (path_len - i) < 8 ? (path_len - i) : 8);
        ptrace(PTRACE_POKETEXT, pid, sp + i, (void*)data);
    }

    // 5. Hijack registers to call dlopen(path, RTLD_NOW)
    // ARM64 calling convention: x0 = arg0, x1 = arg1, pc = func, x30 = LR
    regs.regs[0] = sp;                // x0: const char *filename
    regs.regs[1] = RTLD_NOW;          // x1: int flag
    regs.regs[30] = 0;                // LR: Return to null to trigger stop
    regs.pc = remote_dlopen;          // PC: Jump to dlopen

    ptrace(PTRACE_SETREGSET, pid, NT_PRSTATUS, &iov);

    // 6. Resume and wait for finish
    ptrace(PTRACE_CONT, pid, NULL, NULL);
    waitpid(pid, NULL, WUNTRACED);

    // 7. Restore registers and detach
    iov.iov_base = &old_regs;
    ptrace(PTRACE_SETREGSET, pid, NT_PRSTATUS, &iov);
    ptrace(PTRACE_DETACH, pid, NULL, NULL);
    
    printf("[+] Injection cycle finished.\n");
    return 0;
}

int main(int argc, char* argv[]) {
    const char* target_pkg = "com.tencent.jkchess";
    const char* so_to_inject = "/data/local/tmp/libJKMenu.so";

    if (getuid() != 0) {
        printf("[!] Please run as ROOT (su)\n");
        return 1;
    }

    pid_t target_pid = find_pid(target_pkg);
    if (target_pid == -1) {
        printf("[-] Target %s not found. Open the game first.\n", target_pkg);
        return 1;
    }

    if (inject_so(target_pid, so_to_inject) == 0) {
        printf("[*] Done. Check the game screen for ImGui menu.\n");
    } else {
        printf("[!] Failed to inject.\n");
    }
    
    return 0;
}
