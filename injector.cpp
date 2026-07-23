/**
 * injector.cpp – 让 dlopen 正常返回（不再依赖 segfault）
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/uio.h>
#include <sys/wait.h>
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

// ---------- 扫描远程进程的可写内存 ----------
unsigned long find_writable_region(int pid, size_t needed_size) {
    char maps_path[64];
    snprintf(maps_path, sizeof(maps_path), "/proc/%d/maps", pid);
    FILE* fp = fopen(maps_path, "r");
    if (!fp) {
        perror("open remote maps");
        return 0;
    }
    char line[512];
    unsigned long best_addr = 0;
    while (fgets(line, sizeof(line), fp)) {
        unsigned long start, end;
        char perms[8];
        if (sscanf(line, "%lx-%lx %s", &start, &end, perms) != 3) continue;
        if (strchr(perms, 'w') && start >= 0x1000000 && (end - start) >= needed_size) {
            if (strstr(line, "libc_malloc") || strstr(line, "[heap]") || strstr(line, "[anon]")) {
                best_addr = start;
                break;
            }
            if (best_addr == 0) best_addr = start;
        }
    }
    fclose(fp);
    return best_addr;
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
    unsigned long remote_addr = find_writable_region(pid, path_len);
    if (remote_addr == 0) {
        fprintf(stderr, "No suitable writable region\n");
        ptrace(PTRACE_DETACH, pid, 0, 0);
        return -1;
    }
    printf("[+] Using writable region at 0x%lx\n", remote_addr);

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

    // 设置 PC 为 dlopen，LR 为原 PC+4（让 dlopen 返回后继续执行）
    regs = orig_regs;
    regs.regs[0] = remote_addr;
    regs.regs[1] = RTLD_LAZY;
    regs.pc = remote_dlopen;
    regs.regs[30] = orig_regs.pc + 4;  // 返回地址设为原 PC 后一条指令

    iov.iov_base = &regs; iov.iov_len = sizeof(regs);
    if (ptrace(PTRACE_SETREGSET, pid, NT_PRSTATUS, &iov) == -1) {
        perror("setregset for dlopen");
        ptrace(PTRACE_DETACH, pid, 0, 0);
        return -1;
    }

    // 继续执行，不再等待 segfault
    if (ptrace(PTRACE_CONT, pid, 0, 0) == -1) {
        perror("ptrace cont");
        ptrace(PTRACE_DETACH, pid, 0, 0);
        return -1;
    }

    // 等待进程暂停（可能由于信号）或退出
    waitpid(pid, &status, 0);
    // 如果进程意外停止，尝试恢复
    if (WIFSTOPPED(status)) {
        printf("[!] Process stopped with signal %d, continuing...\n", WSTOPSIG(status));
        ptrace(PTRACE_CONT, pid, 0, 0);
        waitpid(pid, &status, 0);
    }

    // 恢复原始寄存器并 detach
    iov.iov_base = &orig_regs; iov.iov_len = sizeof(orig_regs);
    ptrace(PTRACE_SETREGSET, pid, NT_PRSTATUS, &iov);
    ptrace(PTRACE_DETACH, pid, 0, 0);
    printf("[+] Injection completed (dlopen returned).\n");
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
