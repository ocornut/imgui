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
#include <cstdint>
#include <android/log.h>
#include <errno.h>
#include <elf.h>
#include <linux/elf.h>
#include <sys/syscall.h>

#define TAG "AndKitty_Injector"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__); printf("[INFO] " __VA_ARGS__); printf("\n")
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__); printf("[ERROR] " __VA_ARGS__); printf("\n")
#ifndef NT_PRSTATUS
#define NT_PRSTATUS 1
#endif

using namespace std;

// ========== 固定配置 无需再修改 ==========
constexpr const char* TARGET_PACKAGE = "com.tencent.jkchess";
constexpr const char* TARGET_SO_PATH  = "/data/data/com.tencent.jkchess/libMyMenu.so";

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
            if (fgets(cmdline, sizeof(cmdline), f)) {
                if (strstr(cmdline, package_name)) {
                    fclose(f);
                    closedir(dir);
                    return pid;
                }
            }
            fclose(f);
        }
    }
    closedir(dir);
    return -1;
}

uintptr_t get_module_base(pid_t pid, const char* module_name) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/maps", pid);
    FILE* f = fopen(path, "r");
    if (!f) return 0;
    char line[512];
    uintptr_t base = 0;
    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, module_name)) {
            base = (uintptr_t)strtoull(line, NULL, 16);
            break;
        }
    }
    fclose(f);
    return base;
}

uintptr_t get_remote_addr(pid_t pid, const char* module_name, void* local_addr) {
    uintptr_t local_base = get_module_base(getpid(), module_name);
    uintptr_t remote_base = get_module_base(pid, module_name);
    if (!local_base || !remote_base) return 0;
    return remote_base + ((uintptr_t)local_addr - local_base);
}

bool ptrace_read(pid_t pid, uintptr_t addr, void* buf, size_t len) {
    size_t i;
    long* lbuf = (long*)buf;
    for (i = 0; i < len; i += sizeof(long)) {
        long val = ptrace(PTRACE_PEEKTEXT, pid, addr + i, NULL);
        if (val == -1 && errno != 0) {
            return false;
        }
        lbuf[i / sizeof(long)] = val;
    }
    return true;
}

bool ptrace_write(pid_t pid, uintptr_t addr, void* buf, size_t len) {
    size_t i;
    long* lbuf = (long*)buf;
    for (i = 0; i < len; i += sizeof(long)) {
        if (ptrace(PTRACE_POKETEXT, pid, addr + i, lbuf[i / sizeof(long)]) < 0) {
            LOGE("Ptrace write failed at addr %p, errno: %d", (void*)(addr + i), errno);
            return false;
        }
    }
    return true;
}

uint64_t ptrace_call(pid_t pid, uintptr_t func_addr, uint64_t* args, int nargs) {
    struct user_pt_regs regs, old_regs;
    struct iovec iov;
    iov.iov_base = &regs;
    iov.iov_len = sizeof(struct user_pt_regs);
    if (ptrace(PTRACE_GETREGSET, pid, (void*)NT_PRSTATUS, &iov) < 0) {
        LOGE("ptrace_call: GETREGSET failed");
        return 0;
    }
    memcpy(&old_regs, &regs, sizeof(regs));
    for (int i = 0; i < nargs && i < 8; i++) regs.regs[i] = args[i];
    regs.regs[30] = 0;
    regs.pc = func_addr;
    iov.iov_base = &regs;
    if (ptrace(PTRACE_SETREGSET, pid, (void*)NT_PRSTATUS, &iov) < 0) {
        LOGE("ptrace_call: SETREGSET failed");
        return 0;
    }
    if (ptrace(PTRACE_CONT, pid, NULL, NULL) < 0) {
        LOGE("ptrace_call: CONT failed");
        return 0;
    }
    int status;
    waitpid(pid, &status, WUNTRACED);
    if (WIFSTOPPED(status) && WSTOPSIG(status) != SIGSEGV) {
        LOGI("ptrace_call: Process stopped with unexpected signal: %d", WSTOPSIG(status));
    }
    if (ptrace(PTRACE_GETREGSET, pid, (void*)NT_PRSTATUS, &iov) < 0) {
        LOGE("ptrace_call: Final GETREGSET failed");
        return 0;
    }
    uint64_t res = regs.regs[0];
    iov.iov_base = &old_regs;
    ptrace(PTRACE_SETREGSET, pid, (void*)NT_PRSTATUS, &iov);
    return res;
}

int main() {
    LOGI("===== AndKitty Auto Injector =====");
    LOGI("Target Package: %s", TARGET_PACKAGE);
    LOGI("Inject SO: %s", TARGET_SO_PATH);

    pid_t pid = get_pid(TARGET_PACKAGE);
    if (pid <= 0) {
        LOGE("Cannot find target game process! Start game first.");
        return -1;
    }
    LOGI("Found target PID = %d", pid);

    if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) < 0) {
        LOGE("Ptrace Attach failed: %s", strerror(errno));
        return -1;
    }
    waitpid(pid, NULL, WUNTRACED);
    LOGI("Attached to PID: %d successfully.", pid);

    uintptr_t remote_mmap = get_remote_addr(pid, "libc.so", (void*)mmap);
    if (!remote_mmap) {
        LOGE("Could not locate remote mmap in libc.so");
        ptrace(PTRACE_DETACH, pid, NULL, NULL);
        return -1;
    }
    LOGI("Remote mmap address: %p", (void*)remote_mmap);

    uintptr_t remote_dlopen = get_remote_addr(pid, "libdl.so", (void*)dlopen);
    if (!remote_dlopen) remote_dlopen = get_remote_addr(pid, "linker64", (void*)dlopen);
    if (!remote_dlopen) {
        LOGE("Could not locate remote dlopen");
        ptrace(PTRACE_DETACH, pid, NULL, NULL);
        return -1;
    }
    LOGI("Remote dlopen address: %p", (void*)remote_dlopen);

    size_t string_len = strlen(TARGET_SO_PATH) + 1;
    size_t alloc_len = (string_len + 4095) & ~4095;

    uint64_t mmap_args[6] = {
        0,
        alloc_len,
        PROT_READ | PROT_WRITE | PROT_EXEC,
        MAP_PRIVATE | MAP_ANONYMOUS,
        (uint64_t)-1,
        0
    };

    LOGI("Calling remote mmap to allocate %zu bytes...", alloc_len);
    uint64_t remote_buf_addr = ptrace_call(pid, remote_mmap, mmap_args, 6);
    if (remote_buf_addr == 0 || remote_buf_addr == (uint64_t)-1) {
        LOGE("Remote mmap failed. Cannot allocate safe memory.");
        ptrace(PTRACE_DETACH, pid, NULL, NULL);
        return -1;
    }
    LOGI("Allocated remote memory at: %p", (void*)remote_buf_addr);

    size_t write_len = (string_len + sizeof(long) - 1) & ~(sizeof(long) - 1);
    char* padded_path = (char*)calloc(1, write_len);
    strcpy(padded_path, TARGET_SO_PATH);

    if (!ptrace_write(pid, remote_buf_addr, (void*)padded_path, write_len)) {
        LOGE("Write remote memory failed");
        free(padded_path);
        ptrace(PTRACE_DETACH, pid, NULL, NULL);
        return -1;
    }
    free(padded_path);
    LOGI("Successfully wrote .so path to target process.");

    uint64_t dlopen_args[2] = { remote_buf_addr, RTLD_NOW };
    LOGI("Calling remote dlopen...");
    uint64_t handle = ptrace_call(pid, remote_dlopen, dlopen_args, 2);

    if (handle == 0) {
        LOGE("==========================================================");
        LOGE("Injection failed: dlopen returned NULL.");
        LOGE("1. So权限不足 / 文件不存在");
        LOGE("2. 架构不匹配(必须arm64-v8a)");
        LOGE("3. SELinux强制模式拦截");
        LOGE("==========================================================");
    } else {
        LOGI("✅ Injection success! SO Handle: %p", (void*)handle);
    }

    if (ptrace(PTRACE_DETACH, pid, NULL, NULL) < 0) {
        LOGE("Failed to detach gracefully.");
    } else {
        LOGI("Detached successfully.");
    }
    return 0;
}
