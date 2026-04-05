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

// 修改点1: 匹配你的正确包名
const char* TARGET_PROCESS = "com.tencent.jkchess"; 
const char* SO_PATH = "/data/local/tmp/libJKMenu.so";

pid_t find_target_pid() {
    DIR* dir = opendir("/proc");
    if (!dir) return -1;
    struct dirent* entry;
    while ((entry = readdir(dir))) {
        pid_t pid = atoi(entry->d_name);
        if (pid <= 0) continue;
        char path[256], cmdline[256];
        snprintf(path, sizeof(path), "/proc/%d/cmdline", pid);
        FILE* f = fopen(path, "r");
        if (f) {
            if (fgets(cmdline, sizeof(cmdline), f)) {
                if (strcmp(cmdline, TARGET_PROCESS) == 0 || strstr(cmdline, TARGET_PROCESS)) {
                    fclose(f);
                    closedir(dir);
                    return pid;
                }
            }
            fclose(f);
        }
    }
    closedir(dir);
    return -1;
}

uintptr_t get_remote_module_base(pid_t pid, const char* module_name) {
    char path[256], line[512];
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

int run_injection(pid_t pid, const char* so_path) {
    printf("[+] Injecting into PID: %d\n", pid);
    
    if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) < 0) {
        perror("[-] Attach failed");
        return -1;
    }
    waitpid(pid, NULL, WUNTRACED);

    struct user_pt_regs regs, old_regs;
    struct iovec iov = { .iov_base = &regs, .iov_len = sizeof(regs) };
    ptrace(PTRACE_GETREGSET, pid, NT_PRSTATUS, &iov);
    memcpy(&old_regs, &regs, sizeof(regs));

    // 修改点2: 更可靠的 dlopen 定位逻辑
    uintptr_t local_linker = get_remote_module_base(getpid(), "linker64");
    uintptr_t remote_linker = get_remote_module_base(pid, "linker64");
    
    // 优先找 __loader_dlopen
    uintptr_t local_dlopen = (uintptr_t)dlsym(RTLD_DEFAULT, "__loader_dlopen");
    if (!local_dlopen) {
        void* handle = dlopen("libdl.so", RTLD_NOW);
        local_dlopen = (uintptr_t)dlsym(handle, "dlopen");
    }
    
    uintptr_t remote_dlopen = local_dlopen - local_linker + remote_linker;
    printf("[+] Remote dlopen: %p\n", (void*)remote_dlopen);

    // 修改点3: SP 必须 16 字节对齐
    uintptr_t sp = (regs.sp - 512) & ~0xF; 
    size_t path_len = strlen(so_path) + 1;
    for (size_t i = 0; i < path_len; i += 8) {
        long data = 0;
        memcpy(&data, so_path + i, (path_len - i) < 8 ? (path_len - i) : 8);
        ptrace(PTRACE_POKETEXT, pid, sp + i, (void*)data);
    }

    // 修改点4: 使用 RTLD_GLOBAL | RTLD_NOW (通常是 2 | 256)
    regs.regs[0] = sp;                  // x0: so_path
    regs.regs[1] = 2 | 256;             // x1: mode (RTLD_NOW | RTLD_GLOBAL)
    regs.regs[30] = 0;                  // LR: set to 0 to trigger crash after return
    regs.pc = remote_dlopen;

    ptrace(PTRACE_SETREGSET, pid, NT_PRSTATUS, &iov);
    ptrace(PTRACE_CONT, pid, NULL, NULL);
    
    int status;
    waitpid(pid, &status, WUNTRACED);

    struct user_pt_regs post_regs;
    struct iovec post_iov = { .iov_base = &post_regs, .iov_len = sizeof(post_regs) };
    ptrace(PTRACE_GETREGSET, pid, NT_PRSTATUS, &post_iov);
    
    if (post_regs.regs[0] == 0) {
        printf("[-] dlopen returned NULL! Check Logcat.\n");
    } else {
        printf("[+] SUCCESS! Handle: %p\n", (void*)post_regs.regs[0]);
    }

    iov.iov_base = &old_regs;
    ptrace(PTRACE_SETREGSET, pid, NT_PRSTATUS, &iov);
    ptrace(PTRACE_DETACH, pid, NULL, NULL);
    
    return (post_regs.regs[0] == 0) ? -1 : 0;
}

int main() {
    if (getuid() != 0) {
        printf("[-] Need Root!\n");
        return 1;
    }

    pid_t pid = find_target_pid();
    if (pid == -1) {
        printf("[-] %s not found.\n", TARGET_PROCESS);
        return 1;
    }

    // 提示：注入前最好执行 su -c "setenforce 0"
    return run_injection(pid, SO_PATH);
}
