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

// 查找王者荣耀进程 PID
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

// 获取远程模块基址
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

// 核心 PTRACE 注入逻辑
int run_injection(pid_t pid, const char* so_path) {
    printf("[+] Target PID: %d\n", pid);
    
    // 1. 附加进程
    if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) < 0) {
        perror("[-] PTRACE_ATTACH failed");
        return -1;
    }
    waitpid(pid, NULL, WUNTRACED);

    // 2. 备份寄存器
    struct user_pt_regs regs, old_regs;
    struct iovec iov = { .iov_base = &regs, .iov_len = sizeof(regs) };
    ptrace(PTRACE_GETREGSET, pid, NT_PRSTATUS, &iov);
    memcpy(&old_regs, &regs, sizeof(regs));

    // 3. 定位远程 dlopen
    uintptr_t local_linker = get_remote_module_base(getpid(), "linker64");
    uintptr_t remote_linker = get_remote_module_base(pid, "linker64");
    uintptr_t local_dlopen = (uintptr_t)dlsym(RTLD_DEFAULT, "__loader_dlopen");
    if (!local_dlopen) local_dlopen = (uintptr_t)dlsym(RTLD_DEFAULT, "dlopen");
    
    uintptr_t remote_dlopen = local_dlopen - local_linker + remote_linker;
    printf("[+] Remote dlopen located at: %p\n", (void*)remote_dlopen);

    // 4. 在远程栈写入 SO 路径
    uintptr_t sp = regs.sp - 512; 
    size_t path_len = strlen(so_path) + 1;
    for (size_t i = 0; i < path_len; i += 8) {
        long data = 0;
        memcpy(&data, so_path + i, (path_len - i) < 8 ? (path_len - i) : 8);
        ptrace(PTRACE_POKETEXT, pid, sp + i, (void*)data);
    }

    // 5. 设置寄存器以调用 dlopen
    regs.regs[0] = sp;                // x0: path
    regs.regs[1] = RTLD_NOW;          // x1: mode
    regs.regs[30] = 0;                // LR: 0 会触发 SIGSEGV 从而停止
    regs.pc = remote_dlopen;          // PC: 跳转到 dlopen

    ptrace(PTRACE_SETREGSET, pid, NT_PRSTATUS, &iov);

    // 6. 执行并等待
    ptrace(PTRACE_CONT, pid, NULL, NULL);
    int status;
    waitpid(pid, &status, WUNTRACED);

    // 检查执行后的结果寄存器 x0 (dlopen 的返回值)
    struct user_pt_regs post_regs;
    struct iovec post_iov = { .iov_base = &post_regs, .iov_len = sizeof(post_regs) };
    ptrace(PTRACE_GETREGSET, pid, NT_PRSTATUS, &post_iov);
    
    if (post_regs.regs[0] == 0) {
        printf("[-] Error: dlopen returned NULL. Check Logcat for details.\n");
    } else {
        printf("[+] Success: SO handle returned: %p\n", (void*)post_regs.regs[0]);
    }

    // 7. 恢复寄存器并分离
    iov.iov_base = &old_regs;
    ptrace(PTRACE_SETREGSET, pid, NT_PRSTATUS, &iov);
    ptrace(PTRACE_DETACH, pid, NULL, NULL);
    
    return (post_regs.regs[0] == 0) ? -1 : 0;
}

int main(int argc, char* argv[]) {
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
        printf("[*] Injection confirmed. Use 'logcat -s JKMenu' to see menu logs.\n");
    } else {
        printf("[!] Injection failed. Possible SELinux or Path issue.\n");
    }
    
    return 0;
}
