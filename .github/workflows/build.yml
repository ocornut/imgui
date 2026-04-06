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
#include <android/log.h>

#define TAG "AndKitty_Injector"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

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
            if (strncmp(cmdline, package_name, strlen(package_name)) == 0) {
                closedir(dir);
                return pid;
            }
        }
    }
    closedir(dir);
    return -1;
}

// 获取远程基址
uintptr_t get_module_base(pid_t pid, const char* module_name) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/maps", pid);
    FILE* f = fopen(path, "r");
    if (!f) return 0;
    char line[512];
    uintptr_t base = 0;
    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, module_name)) {
            base = strtoull(line, NULL, 16);
            break;
        }
    }
    fclose(f);
    return base;
}

// 获取远程函数地址
uintptr_t get_remote_addr(pid_t pid, const char* module_name, void* local_addr) {
    uintptr_t local_base = get_module_base(getpid(), module_name);
    uintptr_t remote_base = get_module_base(pid, module_name);
    if (!local_base || !remote_base) return 0;
    return remote_base + ((uintptr_t)local_addr - local_base);
}

// Ptrace 写内存
bool ptrace_write(pid_t pid, uintptr_t addr, void* buf, size_t len) {
    size_t i;
    long* lbuf = (long*)buf;
    for (i = 0; i < len; i += sizeof(long)) {
        if (ptrace(PTRACE_POKETEXT, pid, addr + i, lbuf[i / sizeof(long)]) < 0) return false;
    }
    return true;
}

// Ptrace 调用远程函数 (ARM64)
uint64_t ptrace_call(pid_t pid, uintptr_t func_addr, uint64_t* args, int nargs) {
    struct user_pt_regs regs, old_regs;
    struct iovec iov = {&regs, sizeof(struct user_pt_regs)};
    ptrace(PTRACE_GETREGSET, pid, NT_PRSTATUS, &iov);
    memcpy(&old_regs, &regs, sizeof(regs));

    for (int i = 0; i < nargs && i < 8; i++) regs.regs[i] = args[i];
    regs.regs[30] = 0; // LR 置零以便触发停止
    regs.pc = func_addr;

    iov.iov_base = &regs;
    ptrace(PTRACE_SETREGSET, pid, NT_PRSTATUS, &iov);
    ptrace(PTRACE_CONT, pid, NULL, NULL);
    waitpid(pid, NULL, WUNTRACED);

    ptrace(PTRACE_GETREGSET, pid, NT_PRSTATUS, &iov);
    uint64_t res = regs.regs[0];
    ptrace(PTRACE_SETREGSET, pid, NT_PRSTATUS, &old_regs);
    return res;
}

int main(int argc, char** argv) {
    if (argc < 3) {
        printf("Usage: ./injector <package> <so_path>\n");
        return -1;
    }

    const char* pkg = argv[1];
    const char* so_path = argv[2];

    LOGI("开始注入: %s -> %s", pkg, so_path);

    pid_t pid = get_pid(pkg);
    if (pid < 0) {
        LOGE("未找到进程 PID");
        return -1;
    }

    if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) < 0) {
        LOGE("Ptrace Attach 失败: %s", strerror(errno));
        return -1;
    }
    waitpid(pid, NULL, WUNTRACED);
    LOGI("成功附加到 PID: %d", pid);

    // 尝试从不同的库寻找 dlopen
    uintptr_t remote_dlopen = get_remote_addr(pid, "libdl.so", (void*)dlopen);
    if (!remote_dlopen) remote_dlopen = get_remote_addr(pid, "linker64", (void*)dlopen);

    if (!remote_dlopen) {
        LOGE("无法定位远程 dlopen 地址");
        ptrace(PTRACE_DETACH, pid, NULL, NULL);
        return -1;
    }
    LOGI("远程 dlopen 地址: %p", (void*)remote_dlopen);

    struct user_pt_regs regs;
    struct iovec iov = {&regs, sizeof(struct user_pt_regs)};
    ptrace(PTRACE_GETREGSET, pid, NT_PRSTATUS, &iov);
    uintptr_t remote_str_addr = regs.sp - 1024; // 在栈下方写入路径

    if (!ptrace_write(pid, remote_str_addr, (void*)so_path, strlen(so_path) + 1)) {
        LOGE("写入路径到远程内存失败");
        ptrace(PTRACE_DETACH, pid, NULL, NULL);
        return -1;
    }

    uint64_t args[2] = { remote_str_addr, RTLD_NOW };
    LOGI("正在执行远程 dlopen...");
    uint64_t handle = ptrace_call(pid, remote_dlopen, args, 2);

    if (handle == 0) {
        LOGE("注入失败: dlopen 返回 NULL (可能权限不足或路径错误)");
    } else {
        LOGI("注入成功! 句柄: %p", (void*)handle);
    }

    ptrace(PTRACE_DETACH, pid, NULL, NULL);
    LOGI("已分离进程.");
    return 0;
}
