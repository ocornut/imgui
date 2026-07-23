// injector.cpp – Android ptrace 注入器 (arm64)
// 使用 PTRACE_GETREGSET / PTRACE_SETREGSET 兼容 Android NDK
// 编译: arm64-linux-android-clang++ -static -std=c++17 injector.cpp -o injector
// 运行: adb push injector /data/local/tmp/ && adb shell su -c /data/local/tmp/injector [so_path] [package_name]

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
#include <linux/elf.h>     // NT_PRSTATUS
#include <asm/ptrace.h>    // user_pt_regs
#include <string>
#include <vector>
#include <algorithm>

// ---------- 工具函数 ----------
// 根据进程名获取 PID
int find_pid_by_name(const char* proc_name) {
    DIR* dir = opendir("/proc");
    if (!dir) return -1;
    struct dirent* entry;
    int pid = -1;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type != DT_DIR) continue;
        int tmp_pid = atoi(entry->d_name);
        if (tmp_pid <= 0) continue;
        char path[256];
        snprintf(path, sizeof(path), "/proc/%d/comm", tmp_pid);
        FILE* fp = fopen(path, "r");
        if (fp) {
            char comm[64] = {0};
            if (fgets(comm, sizeof(comm), fp)) {
                comm[strcspn(comm, "\n")] = 0;
                if (strcmp(comm, proc_name) == 0) {
                    pid = tmp_pid;
                    fclose(fp);
                    break;
                }
            }
            fclose(fp);
        }
    }
    closedir(dir);
    return pid;
}

// 读取远程进程内存
long ptrace_readdata(int pid, unsigned long addr, void* buffer, size_t size) {
    for (size_t i = 0; i < size; i += sizeof(long)) {
        long data = ptrace(PTRACE_PEEKDATA, pid, addr + i, 0);
        if (data == -1 && errno) return -1;
        memcpy((char*)buffer + i, &data, std::min(sizeof(long), size - i));
    }
    return 0;
}

// 写入远程进程内存
long ptrace_writedata(int pid, unsigned long addr, const void* buffer, size_t size) {
    for (size_t i = 0; i < size; i += sizeof(long)) {
        long data = 0;
        memcpy(&data, (char*)buffer + i, std::min(sizeof(long), size - i));
        if (ptrace(PTRACE_POKEDATA, pid, addr + i, data) == -1)
            return -1;
    }
    return 0;
}

// 在远程进程中调用 dlopen 并加载库
// 成功返回 0，失败返回 -1
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

    // 2. 保存原始寄存器（用于恢复）
    struct user_pt_regs regs, orig_regs;
    struct iovec iov = { &regs, sizeof(regs) };
    if (ptrace(PTRACE_GETREGSET, pid, NT_PRSTATUS, &iov) == -1) {
        perror("ptrace getregset");
        ptrace(PTRACE_DETACH, pid, 0, 0);
        return -1;
    }
    orig_regs = regs;

    // 3. 在远程进程中分配内存，用于存放 so 路径
    size_t path_len = strlen(so_path) + 1;
    // 使用 mmap 分配匿名内存 (ARM64 系统调用号 222)
    regs.regs[0] = 0;                     // addr
    regs.regs[1] = path_len;              // length
    regs.regs[2] = PROT_READ | PROT_WRITE;// prot
    regs.regs[3] = MAP_PRIVATE | MAP_ANONYMOUS; // flags
    regs.regs[4] = -1;                    // fd
    regs.regs[5] = 0;                     // offset
    regs.regs[8] = 222;                   // syscall number (mmap)
    iov.iov_base = &regs; iov.iov_len = sizeof(regs);
    if (ptrace(PTRACE_SETREGSET, pid, NT_PRSTATUS, &iov) == -1) {
        perror("ptrace setregset for mmap");
        ptrace(PTRACE_DETACH, pid, 0, 0);
        return -1;
    }
    // 执行系统调用
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
    // 获取返回值（分配的地址）
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

    // 4. 将 so 路径写入远程内存
    if (ptrace_writedata(pid, remote_addr, so_path, path_len) == -1) {
        perror("ptrace writedata");
        ptrace(PTRACE_DETACH, pid, 0, 0);
        return -1;
    }

    // 5. 获取远程进程中 dlopen 的地址
    //    方法：从本地 dlopen 地址减去本地 libdl.so 基址，再加上远程 libdl.so 基址
    void* local_dlopen = (void*)dlopen;
    // 获取本地 libdl.so 基址
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

    // 获取远程 libdl.so 基址
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

    // 6. 调用远程 dlopen(so_path, RTLD_LAZY)
    regs = orig_regs; // 恢复成刚 attach 时的状态
    regs.regs[0] = remote_addr;                  // 路径指针
    regs.regs[1] = RTLD_LAZY;                    // 第二个参数
    regs.pc = remote_dlopen_addr;                // 跳转到 dlopen
    regs.regs[30] = 0x0;                         // LR = 0，执行完会 crash
    iov.iov_base = &regs; iov.iov_len = sizeof(regs);
    if (ptrace(PTRACE_SETREGSET, pid, NT_PRSTATUS, &iov) == -1) {
        perror("ptrace setregset for dlopen");
        ptrace(PTRACE_DETACH, pid, 0, 0);
        return -1;
    }

    // 让远程进程继续执行 dlopen，然后会因 PC=0 而 crash
    if (ptrace(PTRACE_CONT, pid, 0, 0) == -1) {
        perror("ptrace cont");
        ptrace(PTRACE_DETACH, pid, 0, 0);
        return -1;
    }
    waitpid(pid, &status, 0);
    if (WIFSIGNALED(status) && WTERMSIG(status) == SIGSEGV) {
        // 预期的段错误，因为 LR=0
        printf("dlopen executed (segfault expected)\n");
    } else {
        fprintf(stderr, "unexpected stop: status=%d\n", status);
        ptrace(PTRACE_DETACH, pid, 0, 0);
        return -1;
    }

    // 7. 清理：远程内存可以不释放，进程会继续运行，so 已加载

    // 8. 恢复原始寄存器并 detach
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

int main(int argc, char** argv) {
    const char* target_name = "com.tencent.jkchess";
    const char* so_path = "/data/data/com.tencent.jkchess/libcheat.so"; // 默认路径

    if (argc >= 2) so_path = argv[1];
    if (argc >= 3) target_name = argv[2];

    printf("Injecting %s into %s ...\n", so_path, target_name);

    int pid = find_pid_by_name(target_name);
    if (pid <= 0) {
        fprintf(stderr, "Process %s not found\n", target_name);
        return 1;
    }
    printf("Found PID: %d\n", pid);

    // 检查 so 文件是否存在
    if (access(so_path, F_OK) == -1) {
        fprintf(stderr, "SO file %s does not exist\n", so_path);
        return 1;
    }

    return ptrace_inject(pid, so_path);
}
