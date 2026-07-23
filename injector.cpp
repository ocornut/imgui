/**
 * injector.cpp – Android arm64 ptrace 注入器（支持 Android 8~14）
 * 编译: aarch64-linux-android-clang++ -static -std=c++17 injector.cpp -o injector
 * 用法:
 *   ./injector <so_path> <package_name|pid>
 * 示例:
 *   ./injector /data/1/libMyMenu.so com.tencent.jkchess
 *   ./injector /data/1/libMyMenu.so 12345
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
#include <string>
#include <vector>
#include <algorithm>

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

// ---------- 远程内存读写（使用 process_vm 系列，更高效） ----------
ssize_t remote_read(int pid, unsigned long addr, void* buffer, size_t size) {
    struct iovec local = { buffer, size };
    struct iovec remote = { (void*)addr, size };
    return process_vm_readv(pid, &local, 1, &remote, 1, 0);
}

ssize_t remote_write(int pid, unsigned long addr, const void* buffer, size_t size) {
    struct iovec local = { (void*)buffer, size };
    struct iovec remote = { (void*)addr, size };
    return process_vm_writev(pid, &local, 1, &remote, 1, 0);
}

// ---------- 执行系统调用（ptrace） ----------
int remote_syscall(int pid, struct user_pt_regs& regs) {
    struct iovec iov = { &regs, sizeof(regs) };
    if (ptrace(PTRACE_SETREGSET, pid, NT_PRSTATUS, &iov) == -1) {
        perror("setregset before syscall");
        return -1;
    }
    if (ptrace(PTRACE_SYSCALL, pid, 0, 0) == -1) {
        perror("ptrace syscall");
        return -1;
    }
    int status;
    waitpid(pid, &status, 0);
    if (!WIFSTOPPED(status)) {
        fprintf(stderr, "process not stopped after syscall\n");
        return -1;
    }
    // 再执行一次 PTRACE_SYSCALL 以获取返回值（Linux 在 syscall 进入和退出都会停住）
    if (ptrace(PTRACE_SYSCALL, pid, 0, 0) == -1) {
        perror("ptrace syscall exit");
        return -1;
    }
    waitpid(pid, &status, 0);
    if (!WIFSTOPPED(status)) {
        fprintf(stderr, "process not stopped after syscall exit\n");
        return -1;
    }
    // 读取返回值
    struct user_pt_regs ret_regs;
    iov.iov_base = &ret_regs; iov.iov_len = sizeof(ret_regs);
    if (ptrace(PTRACE_GETREGSET, pid, NT_PRSTATUS, &iov) == -1) {
        perror("getregset for return");
        return -1;
    }
    regs = ret_regs;
    return 0;
}

// ---------- 注入主函数 ----------
int inject_so(int pid, const char* so_path) {
    // 1. 附加进程
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

    // 2. 获取原始寄存器
    struct user_pt_regs orig_regs, regs;
    struct iovec iov = { &orig_regs, sizeof(orig_regs) };
    if (ptrace(PTRACE_GETREGSET, pid, NT_PRSTATUS, &iov) == -1) {
        perror("getregset original");
        ptrace(PTRACE_DETACH, pid, 0, 0);
        return -1;
    }
    regs = orig_regs;

    // 3. 在远程进程分配内存（mmap）
    size_t path_len = strlen(so_path) + 1;
    regs.regs[0] = 0;                   // addr
    regs.regs[1] = path_len;            // length
    regs.regs[2] = PROT_READ | PROT_WRITE;
    regs.regs[3] = MAP_PRIVATE | MAP_ANONYMOUS;
    regs.regs[4] = -1;                  // fd
    regs.regs[5] = 0;                   // offset
    regs.regs[8] = 222;                 // __NR_mmap (arm64)
    if (remote_syscall(pid, regs) != 0) {
        fprintf(stderr, "mmap syscall failed\n");
        ptrace(PTRACE_DETACH, pid, 0, 0);
        return -1;
    }
    long remote_addr = regs.regs[0];
    if (remote_addr <= 0 || remote_addr == -1) {
        fprintf(stderr, "mmap returned invalid address: 0x%lx\n", remote_addr);
        ptrace(PTRACE_DETACH, pid, 0, 0);
        return -1;
    }
    printf("[+] Allocated memory at 0x%lx (size=%zu)\n", remote_addr, path_len);

    // 4. 写入 so 路径
    if (remote_write(pid, remote_addr, so_path, path_len) != (ssize_t)path_len) {
        perror("remote_write");
        ptrace(PTRACE_DETACH, pid, 0, 0);
        return -1;
    }
    printf("[+] Wrote SO path into remote memory\n");

    // 5. 计算远程 dlopen 地址
    // 获取本机 dlopen 相对于 libdl.so 的偏移
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

    // 获取远程 libdl.so 基址
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
    printf("[+] Remote dlopen address: 0x%lx\n", remote_dlopen);

    // 6. 设置远程寄存器执行 dlopen
    regs = orig_regs;
    regs.regs[0] = remote_addr;        // path
    regs.regs[1] = RTLD_LAZY;          // flag
    regs.pc = remote_dlopen;
    regs.regs[30] = 0x0;               // LR = 0 (导致返回后 segfault)
    iov.iov_base = &regs; iov.iov_len = sizeof(regs);
    if (ptrace(PTRACE_SETREGSET, pid, NT_PRSTATUS, &iov) == -1) {
        perror("setregset for dlopen");
        ptrace(PTRACE_DETACH, pid, 0, 0);
        return -1;
    }

    // 7. 继续执行
    if (ptrace(PTRACE_CONT, pid, 0, 0) == -1) {
        perror("ptrace cont");
        ptrace(PTRACE_DETACH, pid, 0, 0);
        return -1;
    }
    waitpid(pid, &status, 0);
    if (WIFSIGNALED(status) && WTERMSIG(status) == SIGSEGV) {
        printf("[+] dlopen executed (expected segfault due to LR=0)\n");
    } else {
        fprintf(stderr, "Unexpected stop: status=%d\n", status);
        ptrace(PTRACE_DETACH, pid, 0, 0);
        return -1;
    }

    // 8. 恢复寄存器并 detach
    iov.iov_base = &orig_regs; iov.iov_len = sizeof(orig_regs);
    if (ptrace(PTRACE_SETREGSET, pid, NT_PRSTATUS, &iov) == -1) {
        perror("restore registers");
    }
    if (ptrace(PTRACE_DETACH, pid, 0, 0) == -1) {
        perror("ptrace detach");
        return -1;
    }

    printf("[+] Injection successful!\n");
    return 0;
}

// ---------- 主程序 ----------
int main(int argc, char** argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <so_path> <package_name|pid>\n", argv[0]);
        return 1;
    }

    const char* so_path = argv[1];
    const char* target = argv[2];

    int pid = 0;
    // 检查 target 是否为纯数字（PID）
    bool is_digit = true;
    for (int i = 0; target[i]; i++) {
        if (!isdigit(target[i])) { is_digit = false; break; }
    }
    if (is_digit) {
        pid = atoi(target);
        printf("[+] Using specified PID: %d\n", pid);
    } else {
        printf("[+] Searching for process: %s\n", target);
        pid = find_pid_by_name(target);
        if (pid <= 0) {
            fprintf(stderr, "Process '%s' not found. Make sure it's running.\n", target);
            return 1;
        }
        printf("[+] Found PID: %d\n", pid);
    }

    // 检查 SO 文件是否存在
    if (access(so_path, F_OK) != 0) {
        fprintf(stderr, "SO file '%s' does not exist\n", so_path);
        return 1;
    }
    printf("[+] SO file: %s\n", so_path);

    return inject_so(pid, so_path);
}
