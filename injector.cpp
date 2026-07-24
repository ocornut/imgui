#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/uio.h>
#include <elf.h>
#include <dlfcn.h>
#include <errno.h>

#if defined(__aarch64__)
#define pt_regs user_pt_regs
#define uregs regs
#define ARM_pc pc
#define ARM_sp sp
#define ARM_cpsr pstate
#define ARM_lr regs[30]
#define ARM_r0 regs[0]
#else
#error "This injector currently only supports ARM64 (aarch64) architecture."
#endif

const char* TARGET_PROCESS = "com.tencent.jkchess"; // 目标进程名
const char* PAYLOAD_PATH = "/data/local/tmp/libjk_internal.so"; // 您的动态库路径

// =================================================================
// 辅助函数：通过进程名获取 PID
// =================================================================
pid_t GetPID(const char *process_name) {
    pid_t pid = -1;
    DIR *dir = opendir("/proc");
    if (!dir) return -1;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        int current_pid = atoi(entry->d_name);
        if (current_pid == 0) continue;

        char cmdline_path[256];
        snprintf(cmdline_path, sizeof(cmdline_path), "/proc/%d/cmdline", current_pid);
        
        FILE *fp = fopen(cmdline_path, "r");
        if (fp) {
            char cmdline[256];
            if (fgets(cmdline, sizeof(cmdline), fp) != NULL) {
                if (strcmp(cmdline, process_name) == 0) {
                    pid = current_pid;
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

// =================================================================
// 辅助函数：获取模块在指定进程中的基址
// =================================================================
uintptr_t GetModuleBase(pid_t pid, const char *module_name) {
    uintptr_t base_addr = 0;
    char maps_path[256];
    snprintf(maps_path, sizeof(maps_path), "/proc/%d/maps", pid);

    FILE *fp = fopen(maps_path, "r");
    if (!fp) return 0;

    char line[1024];
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, module_name) && strstr(line, "r-xp")) {
            sscanf(line, "%lx", &base_addr);
            break;
        }
    }
    fclose(fp);
    return base_addr;
}

// =================================================================
// 辅助函数：计算远程函数的绝对地址
// =================================================================
uintptr_t GetRemoteFuncAddr(pid_t target_pid, const char *module_name, void *local_func_addr) {
    uintptr_t local_base = GetModuleBase(getpid(), module_name);
    uintptr_t remote_base = GetModuleBase(target_pid, module_name);
    
    if (local_base == 0 || remote_base == 0) return 0;
    return (uintptr_t)local_func_addr - local_base + remote_base;
}

// =================================================================
// Ptrace 核心操作
// =================================================================
bool PtraceRead(pid_t pid, uintptr_t addr, void *buf, size_t size) {
    uintptr_t i = 0;
    long data;
    while (i < size) {
        data = ptrace(PTRACE_PEEKTEXT, pid, addr + i, NULL);
        if (data == -1 && errno != 0) return false;
        size_t copy_size = (size - i) > sizeof(long) ? sizeof(long) : (size - i);
        memcpy((uint8_t*)buf + i, &data, copy_size);
        i += copy_size;
    }
    return true;
}

bool PtraceWrite(pid_t pid, uintptr_t addr, const void *buf, size_t size) {
    uintptr_t i = 0;
    long data;
    while (i < size) {
        size_t copy_size = (size - i) > sizeof(long) ? sizeof(long) : (size - i);
        if (copy_size < sizeof(long)) {
            data = ptrace(PTRACE_PEEKTEXT, pid, addr + i, NULL);
            memcpy(&data, (uint8_t*)buf + i, copy_size);
        } else {
            memcpy(&data, (uint8_t*)buf + i, sizeof(long));
        }
        if (ptrace(PTRACE_POKETEXT, pid, addr + i, data) == -1) return false;
        i += copy_size;
    }
    return true;
}

bool PtraceCall(pid_t pid, uintptr_t func_addr, long *params, int num_params, struct pt_regs *regs) {
    for (int i = 0; i < num_params && i < 8; i++) {
        regs->uregs[i] = params[i];
    }
    
    // 设置返回地址为 0，当执行到 0 时会触发 SIGSEGV 被 ptrace 捕获
    regs->ARM_lr = 0;
    regs->ARM_pc = func_addr;
    
    if (ptrace(PTRACE_SETREGSET, pid, NT_PRSTATUS, regs) == -1) return false;
    if (ptrace(PTRACE_CONT, pid, NULL, 0) == -1) return false;
    
    int status = 0;
    waitpid(pid, &status, WUNTRACED);
    
    while (WIFSTOPPED(status)) {
        if (WSTOPSIG(status) == SIGSEGV) {
            // 函数执行完毕，读取寄存器获取返回值
            ptrace(PTRACE_GETREGSET, pid, NT_PRSTATUS, regs);
            return true;
        }
        ptrace(PTRACE_CONT, pid, NULL, 0);
        waitpid(pid, &status, WUNTRACED);
    }
    return false;
}

// =================================================================
// 注入主逻辑
// =================================================================
bool InjectLibrary(pid_t pid, const char *library_path) {
    printf("[+] 开始注入目标进程 PID: %d\n", pid);

    if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) == -1) {
        printf("[-] Ptrace 附加失败 (需要 root 权限)\n");
        return false;
    }
    waitpid(pid, NULL, WUNTRACED);
    printf("[+] 成功附加到进程\n");

    struct pt_regs original_regs, regs;
    struct iovec io;
    io.iov_base = &original_regs;
    io.iov_len = sizeof(original_regs);
    ptrace(PTRACE_GETREGSET, pid, NT_PRSTATUS, &io);
    memcpy(&regs, &original_regs, sizeof(regs));

    // 1. 获取目标进程 mmap 和 dlopen 的地址
    // 注意：Android 8.0+ dlopen 受到命名空间限制。若注入失败，建议使用 __loader_dlopen
    uintptr_t remote_mmap = GetRemoteFuncAddr(pid, "libc.so", (void*)mmap);
    uintptr_t remote_dlopen = GetRemoteFuncAddr(pid, "libdl.so", (void*)dlopen); // 或 "linker64"
    
    if (!remote_mmap || !remote_dlopen) {
        printf("[-] 无法获取远程函数地址 mmap: %lx, dlopen: %lx\n", remote_mmap, remote_dlopen);
        ptrace(PTRACE_DETACH, pid, NULL, NULL);
        return false;
    }
    printf("[+] 远程 mmap 地址: 0x%lx\n", remote_mmap);
    printf("[+] 远程 dlopen 地址: 0x%lx\n", remote_dlopen);

    // 2. 在目标进程调用 mmap 分配空间存放 SO 路径
    long parameters[6];
    parameters[0] = 0;                                 // addr
    parameters[1] = 0x1000;                            // size
    parameters[2] = PROT_READ | PROT_WRITE | PROT_EXEC;// prot
    parameters[3] = MAP_ANONYMOUS | MAP_PRIVATE;       // flags
    parameters[4] = 0;                                 // fd
    parameters[5] = 0;                                 // offset

    printf("[+] 调用远程 mmap 分配内存...\n");
    if (!PtraceCall(pid, remote_mmap, parameters, 6, &regs)) {
        printf("[-] 远程 mmap 调用失败\n");
        ptrace(PTRACE_SETREGSET, pid, NT_PRSTATUS, &io);
        ptrace(PTRACE_DETACH, pid, NULL, NULL);
        return false;
    }

    uintptr_t remote_string_addr = regs.ARM_r0;
    printf("[+] 目标进程内存分配成功，地址: 0x%lx\n", remote_string_addr);

    // 3. 将 SO 路径写入目标进程分配的内存中
    if (!PtraceWrite(pid, remote_string_addr, library_path, strlen(library_path) + 1)) {
        printf("[-] 写入 SO 路径失败\n");
        ptrace(PTRACE_SETREGSET, pid, NT_PRSTATUS, &io);
        ptrace(PTRACE_DETACH, pid, NULL, NULL);
        return false;
    }
    printf("[+] SO 路径已写入远程内存\n");

    // 4. 恢复寄存器，准备调用 dlopen
    memcpy(&regs, &original_regs, sizeof(regs));
    parameters[0] = remote_string_addr; // filename
    parameters[1] = RTLD_NOW | RTLD_LOCAL; // flags

    printf("[+] 调用远程 dlopen 加载 SO...\n");
    if (!PtraceCall(pid, remote_dlopen, parameters, 2, &regs)) {
        printf("[-] 远程 dlopen 调用失败\n");
    } else {
        uintptr_t so_handle = regs.ARM_r0;
        if (so_handle == 0) {
            printf("[-] dlopen 返回 NULL，注入可能被 linker namespace 阻止！\n");
        } else {
            printf("[+] 注入成功! SO Handle: 0x%lx\n", so_handle);
        }
    }

    // 5. 恢复目标进程原本的寄存器状态并脱离
    ptrace(PTRACE_SETREGSET, pid, NT_PRSTATUS, &io);
    ptrace(PTRACE_DETACH, pid, NULL, NULL);
    printf("[+] 已脱离目标进程\n");

    return true;
}

int main(int argc, char *argv[]) {
    printf("======================================\n");
    printf("     Android ARM64 Ptrace Injector    \n");
    printf("======================================\n");

    pid_t target_pid = GetPID(TARGET_PROCESS);
    if (target_pid == -1) {
        printf("[-] 未找到目标进程: %s，游戏是否已启动？\n", TARGET_PROCESS);
        return -1;
    }

    if (access(PAYLOAD_PATH, F_OK) == -1) {
        printf("[-] 找不到需要注入的文件: %s\n", PAYLOAD_PATH);
        printf("[-] 请先将您的 SO 文件 adb push 到该路径。\n");
        return -1;
    }

    InjectLibrary(target_pid, PAYLOAD_PATH);

    return 0;
}
