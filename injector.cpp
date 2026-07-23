/**
 * injector.cpp – 稳健注入器（使用 mmap 独立内存，不破坏进程）
 * 编译: aarch64-linux-android-clang++ -static -std=c++17 -O2 injector.cpp -o injector
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/user.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include <linux/elf.h>
#include <asm/ptrace.h>
#include <ctype.h>

#define DEFAULT_SO_PATH   "/data/1/libMyMenu.so"
#define DEFAULT_PKG_NAME  "com.tencent.jkchess"

// ---------- 查找进程 ----------
int find_pid_by_name(const char* proc_name) {
    DIR* dir = opendir("/proc");
    if (!dir) return -1;
    struct dirent* entry;
    int pid = -1;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type != DT_DIR) continue;
        int tmp_pid = atoi(entry->d_name);
        if (tmp_pid <= 0) continue;
        char cmdline_path[256];
        snprintf(cmdline_path, sizeof(cmdline_path), "/proc/%d/cmdline", tmp_pid);
        FILE* fp = fopen(cmdline_path, "r");
        if (fp) {
            char cmdline[256] = {0};
            size_t n = fread(cmdline, 1, sizeof(cmdline) - 1, fp);
            fclose(fp);
            if (n > 0) {
                char* pkg = cmdline;
                char* last_slash = strrchr(pkg, '/');
                if (last_slash) pkg = last_slash + 1;
                if (strcmp(pkg, proc_name) == 0) {
                    pid = tmp_pid;
                    break;
                }
            }
        }
    }
    closedir(dir);
    return pid;
}

// ---------- ptrace 读写 ----------
long ptrace_writedata(int pid, unsigned long addr, const void* buffer, size_t size) {
    for (size_t i = 0; i < size; i += sizeof(long)) {
        long data = 0;
        memcpy(&data, (char*)buffer + i, (size - i) < sizeof(long) ? (size - i) : sizeof(long));
        if (ptrace(PTRACE_POKEDATA, pid, addr + i, data) == -1) {
            perror("ptrace_pokedata");
            return -1;
        }
    }
    return 0;
}

// ---------- 执行系统调用 ----------
int remote_syscall(int pid, struct user_pt_regs& regs, unsigned long* result = nullptr) {
    struct iovec iov = { &regs, sizeof(regs) };
    if (ptrace(PTRACE_SETREGSET, pid, NT_PRSTATUS, &iov) == -1) {
        perror("setregset before syscall");
        return -1;
    }
    if (ptrace(PTRACE_SYSCALL, pid, 0, 0) == -1) {
        perror("ptrace syscall enter");
        return -1;
    }
    int status;
    waitpid(pid, &status, 0);
    if (!WIFSTOPPED(status)) {
        fprintf(stderr, "not stopped after syscall enter\n");
        return -1;
    }
    if (ptrace(PTRACE_SYSCALL, pid, 0, 0) == -1) {
        perror("ptrace syscall exit");
        return -1;
    }
    waitpid(pid, &status, 0);
    if (!WIFSTOPPED(status)) {
        fprintf(stderr, "not stopped after syscall exit\n");
        return -1;
    }
    struct user_pt_regs ret_regs;
    iov.iov_base = &ret_regs; iov.iov_len = sizeof(ret_regs);
    if (ptrace(PTRACE_GETREGSET, pid, NT_PRSTATUS, &iov) == -1) {
        perror("getregset for return");
        return -1;
    }
    if (result) *result = ret_regs.regs[0];
    regs = ret_regs;
    return 0;
}

// ---------- 注入主函数 ----------
int inject_so(int pid, const char* so_path) {
    if (ptrace(PTRACE_ATTACH, pid, 0, 0) == -1) {
        perror("ptrace attach");
        return -1;
    }
    int status;
    waitpid(pid, &status, 0);
    if (!WIFSTOPPED(status)) {
        fprintf(stderr, "process not stopped\n");
        ptrace(PTRACE_DETACH, pid, 0, 0);
        return -1;
    }

    struct user_pt_regs orig_regs, regs;
    struct iovec iov = { &orig_regs, sizeof(orig_regs) };
    if (ptrace(PTRACE_GETREGSET, pid, NT_PRSTATUS, &iov) == -1) {
        perror("getregset original");
        ptrace(PTRACE_DETACH, pid, 0, 0);
        return -1;
    }
    regs = orig_regs;

    size_t path_len = strlen(so_path) + 1;

    // 尝试 mmap 多个候选地址
    unsigned long candidate_addrs[] = { 0x10000000, 0x20000000, 0x30000000, 0x40000000, 0 };
    unsigned long remote_addr = 0;
    int retry = 0;
    while (candidate_addrs[retry] != 0) {
        regs = orig_regs;
        regs.regs[0] = candidate_addrs[retry];
        regs.regs[1] = path_len;
        regs.regs[2] = PROT_READ | PROT_WRITE;
        regs.regs[3] = MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE; // 不覆盖已有映射
        regs.regs[4] = -1;
        regs.regs[5] = 0;
        regs.regs[8] = 222; // mmap

        if (remote_syscall(pid, regs, &remote_addr) != 0) {
            fprintf(stderr, "mmap syscall failed at 0x%lx\n", candidate_addrs[retry]);
            retry++;
            continue;
        }
        if (remote_addr != (unsigned long)-1 && remote_addr != 0) {
            printf("[+] mmap succeeded at 0x%lx\n", remote_addr);
            break;
        }
        retry++;
    }
    if (remote_addr == 0 || remote_addr == (unsigned long)-1) {
        fprintf(stderr, "All mmap attempts failed\n");
        ptrace(PTRACE_DETACH, pid, 0, 0);
        return -1;
    }

    // 写入 SO 路径
    if (ptrace_writedata(pid, remote_addr, so_path, path_len) == -1) {
        fprintf(stderr, "ptrace_writedata failed\n");
        ptrace(PTRACE_DETACH, pid, 0, 0);
        return -1;
    }
    printf("[+] Wrote SO path\n");

    // 计算远程 dlopen
    void* local_dlopen = (void*)dlopen;
    unsigned long local_libdl_base = 0;
    FILE* fp = fopen("/proc/self/maps", "r");
    if (!fp) {
        perror("open self maps");
        ptrace(PTRACE_DETACH, pid, 0, 0);
        return -1;
    }
    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, "libdl.so")) {
            sscanf(line, "%lx", &local_libdl_base);
            break;
        }
    }
    fclose(fp);
    if (local_libdl_base == 0) {
        fprintf(stderr, "can't find local libdl.so base\n");
        ptrace(PTRACE_DETACH, pid, 0, 0);
        return -1;
    }
    unsigned long dlopen_offset = (unsigned long)local_dlopen - local_libdl_base;

    char remote_maps_path[64];
    snprintf(remote_maps_path, sizeof(remote_maps_path), "/proc/%d/maps", pid);
    fp = fopen(remote_maps_path, "r");
    if (!fp) {
        perror("open remote maps");
        ptrace(PTRACE_DETACH, pid, 0, 0);
        return -1;
    }
    unsigned long remote_libdl_base = 0;
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, "libdl.so")) {
            sscanf(line, "%lx", &remote_libdl_base);
            break;
        }
    }
    fclose(fp);
    if (remote_libdl_base == 0) {
        fprintf(stderr, "can't find remote libdl.so base\n");
        ptrace(PTRACE_DETACH, pid, 0, 0);
        return -1;
    }
    unsigned long remote_dlopen = remote_libdl_base + dlopen_offset;
    printf("[+] Remote dlopen: 0x%lx\n", remote_dlopen);

    // 执行 dlopen，LR 设回原 PC+4
    regs = orig_regs;
    regs.regs[0] = remote_addr;
    regs.regs[1] = RTLD_LAZY;
    regs.pc = remote_dlopen;
    regs.regs[30] = orig_regs.pc + 4;   // 正常返回

    iov.iov_base = &regs; iov.iov_len = sizeof(regs);
    if (ptrace(PTRACE_SETREGSET, pid, NT_PRSTATUS, &iov) == -1) {
        perror("setregset for dlopen");
        ptrace(PTRACE_DETACH, pid, 0, 0);
        return -1;
    }

    // 继续执行，不等待信号
    if (ptrace(PTRACE_CONT, pid, 0, 0) == -1) {
        perror("ptrace cont");
        ptrace(PTRACE_DETACH, pid, 0, 0);
        return -1;
    }

    // 等待一下，让 dlopen 执行
    usleep(100000); // 100ms

    // 检查进程状态，如果停止则恢复并继续
    pid_t wpid = waitpid(pid, &status, WNOHANG);
    if (wpid > 0 && WIFSTOPPED(status)) {
        printf("[!] Process stopped with signal %d, continuing...\n", WSTOPSIG(status));
        ptrace(PTRACE_CONT, pid, 0, 0);
    }

    // 恢复寄存器并 detach
    iov.iov_base = &orig_regs; iov.iov_len = sizeof(orig_regs);
    ptrace(PTRACE_SETREGSET, pid, NT_PRSTATUS, &iov);
    ptrace(PTRACE_DETACH, pid, 0, 0);
    printf("[+] Injection completed successfully.\n");
    return 0;
}

// ---------- 主函数 ----------
int main(int argc, char** argv) {
    const char* so_path = DEFAULT_SO_PATH;
    const char* target = DEFAULT_PKG_NAME;
    int pid = -1;

    if (argc >= 2) so_path = argv[1];
    if (argc >= 3) {
        bool is_digit = true;
        for (int i = 0; argv[2][i]; i++) {
            if (!isdigit(argv[2][i])) { is_digit = false; break; }
        }
        if (is_digit) pid = atoi(argv[2]);
        else target = argv[2];
    }

    printf("[Injector] SO: %s\n", so_path);
    printf("[Injector] Target: %s\n", target);

    if (access(so_path, F_OK) != 0) {
        fprintf(stderr, "SO file not found: %s\n", so_path);
        return 1;
    }

    if (pid <= 0) {
        printf("[Injector] Searching for process '%s'...\n", target);
        pid = find_pid_by_name(target);
        if (pid <= 0) {
            fprintf(stderr, "Process '%s' not found\n", target);
            return 1;
        }
        printf("[Injector] Found PID: %d\n", pid);
    } else {
        printf("[Injector] Using specified PID: %d\n", pid);
    }

    return inject_so(pid, so_path);
}
