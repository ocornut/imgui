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

// Find process ID for SGame (Honor of Kings)
pid_t find_sgame_pid() {
    const char* process_name = "com.tencent.tmgp.sgame";
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
                // Use strstr for fuzzy matching to handle trailing spaces or arguments
                if (strstr(cmdline, process_name)) {
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

// Get module base address in remote process memory
uintptr_t get_remote_module_base(pid_t pid, const char* module_name) {
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

// Core PTRACE injection logic
int run_injection(pid_t pid, const char* so_path) {
    printf("[+] Target PID: %d\n", pid);
    
    // 1. Attach to the process
    if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) < 0) {
        perror("[-] PTRACE_ATTACH failed. Ensure SELinux is permissive (setenforce 0)");
        return -1;
    }
    waitpid(pid, NULL, WUNTRACED);

    // 2. Backup current registers
    struct user_pt_regs regs, old_regs;
    struct iovec iov = { .iov_base = &regs, .iov_len = sizeof(regs) };
    ptrace(PTRACE_GETREGSET, pid, NT_PRSTATUS, &iov);
    memcpy(&old_regs, &regs, sizeof(regs));

    // 3. Find remote dlopen address in linker64
    uintptr_t local_linker = get_remote_module_base(getpid(), "linker64");
    uintptr_t remote_linker = get_remote_module_base(pid, "linker64");
    uintptr_t local_dlopen = (uintptr_t)dlsym(RTLD_DEFAULT, "__loader_dlopen");
    if (!local_dlopen) local_dlopen = (uintptr_t)dlsym(RTLD_DEFAULT, "dlopen");
    
    uintptr_t remote_dlopen = local_dlopen - local_linker + remote_linker;
    printf("[+] Remote dlopen located at: %p\n", (void*)remote_dlopen);

    // 4. Write SO path into remote stack space
    uintptr_t sp = regs.sp - 512; 
    size_t path_len = strlen(so_path) + 1;
    for (size_t i = 0; i < path_len; i += 8) {
        long data = 0;
        memcpy(&data, so_path + i, (path_len - i) < 8 ? (path_len - i) : 8);
        ptrace(PTRACE_POKETEXT, pid, sp + i, (void*)data);
    }

    // 5. Setup registers for dlopen(path, RTLD_NOW) call
    // x0 = path, x1 = flag, x30 = LR (Return Address), pc = function address
    regs.regs[0] = sp;
    regs.regs[1] = RTLD_NOW;
    regs.regs[30] = 0; // Return to NULL will trigger a stop
    regs.pc = remote_dlopen;

    ptrace(PTRACE_SETREGSET, pid, NT_PRSTATUS, &iov);

    // 6. Resume execution to trigger dlopen
    ptrace(PTRACE_CONT, pid, NULL, NULL);
    waitpid(pid, NULL, WUNTRACED);

    // 7. Restore original registers and detach
    iov.iov_base = &old_regs;
    ptrace(PTRACE_SETREGSET, pid, NT_PRSTATUS, &iov);
    ptrace(PTRACE_DETACH, pid, NULL, NULL);
    
    return 0;
}

int main(int argc, char* argv[]) {
    // Default SO path (ensure this matches your deployment)
    const char* so_path = "/data/local/tmp/libJKMenu.so";

    if (getuid() != 0) {
        printf("[!] Error: Root access required.\n");
        return 1;
    }

    pid_t pid = find_sgame_pid();
    if (pid == -1) {
        printf("[-] Error: SGame (com.tencent.tmgp.sgame) not running.\n");
        return 1;
    }

    if (run_injection(pid, so_path) == 0) {
        printf("[*] Success: Injection command sent to SGame.\n");
    } else {
        printf("[!] Error: Injection process failed.\n");
    }
    
    return 0;
}
