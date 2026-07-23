// injector.cpp – 改进版 Android ptrace 注入器 (arm64)
// 编译: arm64-linux-android-clang++ -static -std=c++17 injector.cpp -o injector
// 用法:
//   ./injector [so_path] [package_name]       // 自动查找进程
//   ./injector [so_path] [pid]                // 直接指定 PID（第三个参数为数字）
// 示例:
//   ./injector /data/local/tmp/libcheat.so com.tencent.jkchess
//   ./injector /data/local/tmp/libcheat.so 12345

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

// ---------- 通过 cmdline 精确查找进程 ----------
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
                // cmdline 的第一个字符串是命令（包含路径）
                char* pkg = cmdline;
                // 去除路径前缀
                char* last_slash = strrchr(pkg, '/');
                if (last_slash) pkg = last_slash + 1;
                // 比较完整包名
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

// ---------- 远程内存读写辅助 ----------
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

// ---------- 注入主逻辑 ----------
int ptrace_inject(int pid, const char* so_path) {
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

    // 2. 保存原始寄存器
    struct user_pt_regs regs, orig_regs;
    struct iovec iov = { &regs, sizeof(regs) };
    if (ptrace(PTRACE_GETREGSET, pid, NT_PRSTATUS, &iov) == -1) {
        perror("ptrace getregset");
        ptrace(PTRACE_DETACH, pid, 0, 0);
        return -1;
    }
    orig_regs = regs;

    // 3. 在远程进程中分配内存存放 SO 路径
    size_t path_len = strlen(so_path) + 1;
    regs.regs[0] = 0;
    regs.regs[1] = path_len;
    regs.regs[2] = PROT_READ | PROT_WRITE;
    regs.regs[3] = MAP_PRIVATE | MAP_ANONYMOUS;
    regs.regs[4] = -1;
    regs.regs[5] = 0;
    regs.regs[8] = 222; // mmap syscall
    iov.iov_base = &regs; iov.iov_len = sizeof(regs);
    if (ptrace(PTRACE_SETREGSET, pid, NT_PRSTATUS, &iov) == -1) {
        perror("ptrace setregset for mmap");
        ptrace(PTRACE_DETACH, pid, 0, 0);
        return -1;
    }
    if (ptrace(PTRACE_SYSCALL, pid, 0, 0) == -1) {
        perror("ptrace syscall");
        ptrace(PTRACE_DETACH, pid, 0, 0);
        return -1;
    }
    waitpid(pid, &status, 0);
    if (!WIFSTOPPED(status)) {
        fprintf(stderr, "process not stopped after syscall\n");
        ptrace(PTRACE_DETACH, pid, 0, 0);
        return -1;
    }
    struct user_pt_regs ret_regs;
    iov.iov_base = &ret_regs; iov.iov_len = sizeof(ret_regs);
    if (ptrace(PTRACE_GETREGSET, pid, NT_PRSTATUS, &iov) == -1) {
        perror("ptrace getregset after mmap");
        ptrace(PTRACE_DETACH, pid, 0, 0);
        return -1;
    }
    long remote_addr = ret_regs.regs[0];
    if (remote_addr <= 0) {
        fprintf(stderr, "mmap failed, remote_addr=0x%lx\n", remote_addr);
        ptrace(PTRACE_DETACH, pid, 0, 0);
        return -1;
    }
    printf("Allocated remote memory at 0x%lx\n", remote_addr);

    // 4. 写入 SO 路径
    if (ptrace_writedata(pid, remote_addr, so_path, path_len) == -1) {
        perror("ptrace writedata");
        ptrace(PTRACE_DETACH, pid, 0, 0);
        return -1;
    }

    // 5. 获取远程 dlopen 地址
    void* local_dlopen = (void*)dlopen;
    FILE* maps = fopen("/proc/self/maps", "r");
    if (!maps) {
        perror("fopen self maps");
        ptrace(PTRACE_DETACH, pid, 0, 0);
        return -1;
    }
    unsigned long local_libdl_base = 0;
    char line[512];
    while (fgets(line, sizeof(line), maps)) {
        if (strstr(line, "libdl.so")) {
            sscanf(line, "%lx", &local_libdl_base);
            break;
        }
    }
    fclose(maps);
    if (local_libdl_base == 0) {
        fprintf(stderr, "cannot find local libdl.so base\n");
        ptrace(PTRACE_DETACH, pid, 0, 0);
        return -1;
    }
    unsigned long local_dlopen_offset = (unsigned long)local_dlopen - local_libdl_base;

    char remote_maps_path[64];
    snprintf(remote_maps_path, sizeof(remote_maps_path), "/proc/%d/maps", pid);
    maps = fopen(remote_maps_path, "r");
    if (!maps) {
        perror("fopen remote maps");
        ptrace(PTRACE_DETACH, pid, 0, 0);
        return -1;
    }
    unsigned long remote_libdl_base = 0;
    while (fgets(line, sizeof(line), maps)) {
        if (strstr(line, "libdl.so")) {
            sscanf(line, "%lx", &remote_libdl_base);
            break;
        }
    }
    fclose(maps);
    if (remote_libdl_base == 0) {
        fprintf(stderr, "cannot find remote libdl.so base\n");
        ptrace(PTRACE_DETACH, pid, 0, 0);
        return -1;
    }
    unsigned long remote_dlopen_addr = remote_libdl_base + local_dlopen_offset;
    printf("Remote dlopen address: 0x%lx\n", remote_dlopen_addr);

    // 6. 调用远程 dlopen
    regs = orig_regs;
    regs.regs[0] = remote_addr;
    regs.regs[1] = RTLD_LAZY;
    regs.pc = remote_dlopen_addr;
    regs.regs[30] = 0x0; // LR = 0，执行后崩溃以便我们捕获
    iov.iov_base = &regs; iov.iov_len = sizeof(regs);
    if (ptrace(PTRACE_SETREGSET, pid, NT_PRSTATUS, &iov) == -1) {
        perror("ptrace setregset for dlopen");
        ptrace(PTRACE_DETACH, pid, 0, 0);
        return -1;
    }
    if (ptrace(PTRACE_CONT, pid, 0, 0) == -1) {
        perror("ptrace cont");
        ptrace(PTRACE_DETACH, pid, 0, 0);
        return -1;
    }
    waitpid(pid, &status, 0);
    if (WIFSIGNALED(status) && WTERMSIG(status) == SIGSEGV) {
        printf("dlopen executed (segfault expected)\n");
    } else {
        fprintf(stderr, "unexpected stop: status=%d\n", status);
        ptrace(PTRACE_DETACH, pid, 0, 0);
        return -1;
    }

    // 7. 恢复寄存器并 detach
    iov.iov_base = &orig_regs; iov.iov_len = sizeof(orig_regs);
    if (ptrace(PTRACE_SETREGSET, pid, NT_PRSTATUS, &iov) == -1) {
        perror("restore regs");
    }
    if (ptrace(PTRACE_DETACH, pid, 0, 0) == -1) {
        perror("detach");
        return -1;
    }

    printf("Injection successful!\n");
    return 0;
}

// ---------- 主函数 ----------
int main(int argc, char** argv) {
    const char* so_path = "/data/local/tmp/libcheat.so";   // 默认路径
    const char* target = "com.tencent.jkchess";            // 默认包名
    int pid = -1;

    if (argc >= 2) so_path = argv[1];
    if (argc >= 3) {
        // 检查第三个参数是否为纯数字（PID）
        bool is_number = true;
        for (int i = 0; argv[2][i]; i++) {
            if (!isdigit(argv[2][i])) { is_number = false; break; }
        }
        if (is_number) {
            pid = atoi(argv[2]);
            printf("Using specified PID: %d\n", pid);
        } else {
            target = argv[2];
        }
    }
    if (argc >= 4) {
        // 如果第四个参数存在，则作为 PID（覆盖之前的推断）
        pid = atoi(argv[3]);
        printf("Using specified PID (from arg4): %d\n", pid);
    }

    if (pid <= 0) {
        printf("Searching for process '%s' ...\n", target);
        pid = find_pid_by_name(target);
        if (pid <= 0) {
            fprintf(stderr, "Process '%s' not found. Please ensure the game is running.\n", target);
            fprintf(stderr, "You can also specify PID manually: %s <so_path> <pid>\n", argv[0]);
            return 1;
        }
        printf("Found PID: %d\n", pid);
    }

    // 检查 SO 文件是否存在
    if (access(so_path, F_OK) == -1) {
        fprintf(stderr, "SO file '%s' does not exist\n", so_path);
        return 1;
    }

    return ptrace_inject(pid, so_path);
}
