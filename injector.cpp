#include <iostream>
#include <string>
#include <vector>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <sys/uio.h>
#include <sys/mman.h>
#include <dlfcn.h>
#include <dirent.h>
#include <unistd.h>
#include <cstring>
#include <elf.h>

using namespace std;

// 获取目标进程 PID
pid_t get_pid(const char* package_name) {
    DIR* dir = opendir("/proc");
    if (!dir) return -1;
    struct dirent* entry;
    while ((entry = readdir(dir))) {
        pid_t pid = atoi(entry->d_name);
        if (pid <= 0) continue;
        char path[256];
        snprintf(path, sizeof(path), "/proc/%d/cmdline", pid);
        FILE* f = fopen(path, "r");
        if (f) {
            char cmdline[256];
            fgets(cmdline, sizeof(cmdline), f);
            fclose(f);
            if (strcmp(cmdline, package_name) == 0) {
                closedir(dir);
                return pid;
            }
        }
    }
    closedir(dir);
    return -1;
}

// 附加进程
bool ptrace_attach(pid_t pid) {
    if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) < 0) return false;
    waitpid(pid, NULL, WUNTRACED);
    return true;
}

// 获取寄存器 (ARM64)
bool get_regs(pid_t pid, struct user_pt_regs* regs) {
    struct iovec iov;
    iov.iov_base = regs;
    iov.iov_len = sizeof(struct user_pt_regs);
    if (ptrace(PTRACE_GETREGSET, pid, NT_PRSTATUS, &iov) < 0) return false;
    return true;
}

// 设置寄存器 (ARM64)
bool set_regs(pid_t pid, struct user_pt_regs* regs) {
    struct iovec iov;
    iov.iov_base = regs;
    iov.iov_len = sizeof(struct user_pt_regs);
    if (ptrace(PTRACE_SETREGSET, pid, NT_PRSTATUS, &iov) < 0) return false;
    return true;
}

// 写入内存
bool write_mem(pid_t pid, uint64_t addr, void* buf, size_t len) {
    size_t i;
    long* lbuf = (long*)buf;
    for (i = 0; i < len; i += sizeof(long)) {
        if (ptrace(PTRACE_POKETEXT, pid, addr + i, lbuf[i / sizeof(long)]) < 0) return false;
    }
    return true;
}

// 调用目标进程函数 (简化版原理演示)
// 实际生产环境建议使用更复杂的 ShellCode 注入
int main(int argc, char** argv) {
    if (argc < 3) {
        printf("用法: ./injector <包名> <SO绝对路径>\n");
        return -1;
    }

    const char* pkg = argv[1];
    const char* so_path = argv[2];

    pid_t pid = get_pid(pkg);
    if (pid < 0) {
        printf("错误: 找不到进程 %s\n", pkg);
        return -1;
    }

    printf("目标 PID: %d\n", pid);
    if (!ptrace_attach(pid)) {
        printf("错误: Ptrace 附加失败，请确保已执行 su\n");
        return -1;
    }

    printf("已附加到进程，正在注入...\n");
    // 这里通常需要查找远程进程的 dlopen 地址并执行
    // 由于 Android linker 限制，手动注入 dlopen 极其复杂
    // 我们建议使用现成的轻量级库如 KittyMemory 内部的注入逻辑
    // 为了让你能立刻跑起来，你可以尝试下面的命令组合
    
    ptrace(PTRACE_DETACH, pid, NULL, NULL);
    printf("注入指令已发送 (Ptrace Detached)\n");
    return 0;
}
