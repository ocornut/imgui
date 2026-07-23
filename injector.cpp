/**
 * injector.cpp – 全自动注入器 (arm64)
 * 编译: aarch64-linux-android-clang++ -static -std=c++17 injector.cpp -o injector
 * 用法: 直接执行 /data/1/injector （无需参数）
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

// ---------- 默认配置 ----------
#define DEFAULT_SO_PATH   "/data/1/libMyMenu.so"
#define DEFAULT_PKG_NAME  "com.tencent.jkchess"

// ---------- 关闭/恢复 SELinux ----------
void set_selinux(int enforce) {
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "setenforce %d", enforce);
    system(cmd);
}

// ---------- 查找进程 (通过 cmdline) ----------
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

// ---------- ptrace 读写内存 ----------
long ptrace_readdata(int pid, unsigned long addr, void* buffer, size_t size) {
    for (size_t i = 0; i < size; i += sizeof(long)) {
        long data = ptrace(PTRACE_PEEKDATA, pid, addr + i, 0);
        if (data == -1 && errno) return -1;
        memcpy((char*)buffer + i, &data, std::min(sizeof(long), size - i));
    }
    return 0;
}

long ptrace_writedata(int pid, unsigned long addr, const void* buffer, size_t size) {
    for (size_t i = 0; i < size; i += sizeof(long)) {
        long data = 0;
        memcpy(&data, (char*)buffer + i, std::min(sizeof(long), size - i));
        if (ptrace(PTRACE_POKEDATA, pid, addr + i, data) == -1)
            return -1;
    }
    return 0;
}

// ---------- 执行系统调用 ----------
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
    if (ptrace(PTRACE_SYSCALL, pid, 0, 0) == -1) {
        perror("ptrace syscall exit");
        return -1;
    }
    waitpid(pid, &status, 0);
    if (!WIFSTOPPED(status)) {
        fprintf(stderr, "process not stopped after syscall exit\n");
        return -1;
    }
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
    // 附加进程
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

    // 获取原始寄存器
    struct user_pt_regs orig_regs, regs;
    struct iovec iov = { &orig_regs, sizeof(orig_regs) };
    if (ptrace(PTRACE_GETREGSET, pid, NT_PRSTATUS, &iov) == -1) {
        perror("getregset original");
        ptrace(PTRACE_DETACH, pid, 0, 0);
        return -1;
    }
    regs = orig_regs;

    // 分配内存
    size_t path_len = strlen(so_path) + 1;
    regs.regs[0] = 0;
    regs.regs[1] = path_len;
    regs.regs[2] = PROT_READ | PROT_WRITE;
    regs.regs[3] = MAP_PRIVATE | MAP_ANONYMOUS;
    regs.regs[4] = -1;
    regs.regs[5] = 0;
    regs.regs[8] = 222; // mmap
    if (remote_syscall(pid, regs) != 0) {
        fprintf(stderr, "mmap syscall failed\n");
        ptrace(PTRACE_DETACH, pid, 0, 0);
        return -1;
    }
    long remote_addr = regs.regs[0];
    if (remote_addr <= 0 || remote_addr == -1) {
        fprintf(stderr, "mmap returned invalid: 0x%lx\n", remote_addr);
        ptrace(PTRACE_DETACH, pid, 0, 0);
        return -1;
    }
    printf("[+] Allocated remote memory at 0x%lx\n", remote_addr);

    // 写入 SO 路径
    if (ptrace_writedata(pid, remote_addr, so_path, path_len) == -1) {
        perror("ptrace_writedata");
        ptrace(PTRACE_DETACH, pid, 0, 0);
        return -1;
    }
    printf("[+] Wrote SO path\n");

    // 计算远程 dlopen
    void* local_dlopen = (void*)dlopen;
    unsigned long local_libdl_base = 0;
    FILE* fp = fopen("/proc/self/maps", "r");
    if (!fp) { perror("self maps"); ptrace(PTRACE_DETACH, pid,0,0); return -1; }
    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, "libdl.so")) {
            sscanf(line, "%lx", &local_libdl_base);
            break;
        }
    }
    fclose(fp);
    if (local_libdl_base == 0) {
        fprintf(stderr, "can't find local libdl.so\n");
        ptrace(PTRACE_DETACH, pid,0,0); return -1;
    }
    unsigned long dlopen_offset = (unsigned long)local_dlopen - local_libdl_base;

    char remote_maps_path[64];
    snprintf(remote_maps_path, sizeof(remote_maps_path), "/proc/%d/maps", pid);
    fp = fopen(remote_maps_path, "r");
    if (!fp) { perror("remote maps"); ptrace(PTRACE_DETACH, pid,0,0); return -1; }
    unsigned long remote_libdl_base = 0;
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, "libdl.so")) {
            sscanf(line, "%lx", &remote_libdl_base);
            break;
        }
    }
    fclose(fp);
    if (remote_libdl_base == 0) {
        fprintf(stderr, "can't find remote libdl.so\n");
        ptrace(PTRACE_DETACH, pid,0,0); return -1;
    }
    unsigned long remote_dlopen = remote_libdl_base + dlopen_offset;
    printf("[+] Remote dlopen at 0x%lx\n", remote_dlopen);

    // 执行 dlopen
    regs = orig_regs;
    regs.regs[0] = remote_addr;
    regs.regs[1] = RTLD_LAZY;
    regs.pc = remote_dlopen;
    regs.regs[30] = 0x0;  // LR=0 -> segfault
    iov.iov_base = &regs; iov.iov_len = sizeof(regs);
    if (ptrace(PTRACE_SETREGSET, pid, NT_PRSTATUS, &iov) == -1) {
        perror("setregset for dlopen");
        ptrace(PTRACE_DETACH, pid,0,0); return -1;
    }
    if (ptrace(PTRACE_CONT, pid, 0, 0) == -1) {
        perror("ptrace cont");
        ptrace(PTRACE_DETACH, pid,0,0); return -1;
    }
    waitpid(pid, &status, 0);
    if (WIFSIGNALED(status) && WTERMSIG(status) == SIGSEGV) {
        printf("[+] dlopen executed (expected segfault)\n");
    } else {
        fprintf(stderr, "Unexpected stop: %d\n", status);
        ptrace(PTRACE_DETACH, pid,0,0); return -1;
    }

    // 恢复寄存器并 detach
    iov.iov_base = &orig_regs; iov.iov_len = sizeof(orig_regs);
    ptrace(PTRACE_SETREGSET, pid, NT_PRSTATUS, &iov);
    ptrace(PTRACE_DETACH, pid, 0, 0);
    printf("[+] Injection successful!\n");
    return 0;
}

// ---------- 主函数 ----------
int main(int argc, char** argv) {
    // 默认参数
    const char* so_path = DEFAULT_SO_PATH;
    const char* pkg_name = DEFAULT_PKG_NAME;
    int pid = -1;

    // 如果用户传了参数则覆盖（但全自动模式推荐无参）
    if (argc >= 2) so_path = argv[1];
    if (argc >= 3) {
        // 如果第三个参数是数字则作为 PID，否则作为包名
        bool is_digit = true;
        for (int i = 0; argv[2][i]; i++) {
            if (!isdigit(argv[2][i])) { is_digit = false; break; }
        }
        if (is_digit) pid = atoi(argv[2]);
        else pkg_name = argv[2];
    }

    printf("[Auto] SO: %s\n", so_path);
    printf("[Auto] Target: %s\n", pkg_name);

    // 检查 SO 是否存在
    if (access(so_path, F_OK) != 0) {
        fprintf(stderr, "SO file '%s' not found!\n", so_path);
        return 1;
    }

    // 关闭 SELinux（允许 ptrace）
    printf("[Auto] Disabling SELinux...\n");
    set_selinux(0);

    // 查找进程
    if (pid <= 0) {
        printf("[Auto] Searching for process '%s'...\n", pkg_name);
        pid = find_pid_by_name(pkg_name);
        if (pid <= 0) {
            fprintf(stderr, "Process '%s' not found. Is the game running?\n", pkg_name);
            set_selinux(1);  // 恢复
            return 1;
        }
        printf("[Auto] Found PID: %d\n", pid);
    }

    // 注入
    int ret = inject_so(pid, so_path);

    // 恢复 SELinux
    printf("[Auto] Restoring SELinux...\n");
    set_selinux(1);

    return ret;
}
