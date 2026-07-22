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
#include <sys/syscall.h> // Added for mmap syscall

#define TAG "AndKitty_Injector"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__); printf("[INFO] " __VA_ARGS__); printf("\n")
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__); printf("[ERROR] " __VA_ARGS__); printf("\n")
// Ensure NT_PRSTATUS is defined for certain NDK versions
#ifndef NT_PRSTATUS
#define NT_PRSTATUS 1
#endif

using namespace std;

// Iterates through /proc to find the PID of the target package name
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
                if (strncmp(cmdline, package_name, strlen(package_name)) == 0) {
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

// Reads /proc/[pid]/maps to find the base address of a loaded module
uintptr_t get_module_base(pid_t pid, const char* module_name) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/maps", pid);
    FILE* f = fopen(path, "r");
    if (!f) return 0;
    
    char line[512];
    uintptr_t base = 0;
    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, module_name)) {
            // Parse the first hex value on the line (start address)
            base = (uintptr_t)strtoull(line, NULL, 16);
            break;
        }
    }
    fclose(f);
    return base;
}

// Calculates the address of a function in the remote process's memory space
uintptr_t get_remote_addr(pid_t pid, const char* module_name, void* local_addr) {
    uintptr_t local_base = get_module_base(getpid(), module_name);
    uintptr_t remote_base = get_module_base(pid, module_name);
    if (!local_base || !remote_base) return 0;
    return remote_base + ((uintptr_t)local_addr - local_base);
}

// Read memory from the remote process (useful for verifying writes)
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

// Ptrace 写内存 (improved byte-by-byte alignment handling is omitted for brevity but standard long-aligned writing is kept)
bool ptrace_write(pid_t pid, uintptr_t addr, void* buf, size_t len) {
    size_t i;
    long* lbuf = (long*)buf;
    // We write in word-sized chunks
    for (i = 0; i < len; i += sizeof(long)) {
        if (ptrace(PTRACE_POKETEXT, pid, addr + i, lbuf[i / sizeof(long)]) < 0) {
            LOGE("Ptrace write failed at addr %p, errno: %d", (void*)(addr + i), errno);
            return false;
        }
    }
    return true;
}

// Ptrace 调用远程函数 (ARM64 specific)
uint64_t ptrace_call(pid_t pid, uintptr_t func_addr, uint64_t* args, int nargs) {
    struct user_pt_regs regs, old_regs;
    struct iovec iov;
    
    // 1. Get current register state to restore later
    iov.iov_base = &regs;
    iov.iov_len = sizeof(struct user_pt_regs);
    if (ptrace(PTRACE_GETREGSET, pid, (void*)NT_PRSTATUS, &iov) < 0) {
        LOGE("ptrace_call: GETREGSET failed");
        return 0;
    }
    
    memcpy(&old_regs, &regs, sizeof(regs));

    for (int i = 0; i < nargs && i < 8; i++) regs.regs[i] = args[i];
    regs.regs[30] = 0; // LR set to 0 to trigger SIGSEGV upon return
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
    
    // Check if process stopped due to our intentional SIGSEGV
    if (WIFSTOPPED(status) && WSTOPSIG(status) != SIGSEGV) {
        LOGI("ptrace_call: Process stopped with unexpected signal: %d", WSTOPSIG(status));
        // You might need to handle other signals here in a real scenario
    }

    if (ptrace(PTRACE_GETREGSET, pid, (void*)NT_PRSTATUS, &iov) < 0) {
        LOGE("ptrace_call: Final GETREGSET failed");
        return 0;
    }
    uint64_t res = regs.regs[0];
    
    iov.iov_base = &old_regs;
    if (ptrace(PTRACE_SETREGSET, pid, (void*)NT_PRSTATUS, &iov) < 0) {
        LOGE("ptrace_call: Restore SETREGSET failed");
    }
    return res;
}

int main(int argc, char** argv) {
    if (argc < 3) {
        printf("Usage: ./injector <package> <so_path>\n");
        return -1;
    }

    const char* pkg = argv[1];
    const char* so_path = argv[2];

    LOGI("Starting injection: Target=%s, Payload=%s", pkg, so_path);
    if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) < 0) {
        LOGE("Ptrace Attach failed: %s", strerror(errno));
        return -1;
    }
    waitpid(pid, NULL, WUNTRACED);
    LOGI("Attached to PID: %d successfully.", pid);

    // Find remote mmap to allocate memory safely
    uintptr_t remote_mmap = get_remote_addr(pid, "libc.so", (void*)mmap);
    if (!remote_mmap) {
        LOGE("Could not locate remote mmap in libc.so");
        ptrace(PTRACE_DETACH, pid, NULL, NULL);
        return -1;
    }
    LOGI("Remote mmap address: %p", (void*)remote_mmap);

    // Find remote dlopen
    uintptr_t remote_dlopen = get_remote_addr(pid, "libdl.so", (void*)dlopen);
    if (!remote_dlopen) remote_dlopen = get_remote_addr(pid, "linker64", (void*)dlopen);

    if (!remote_dlopen) {
        LOGE("Could not locate remote dlopen");
        ptrace(PTRACE_DETACH, pid, NULL, NULL);
        return -1;
    }
    LOGI("Remote dlopen address: %p", (void*)remote_dlopen);

    // Call mmap in the remote process to get a safe buffer
    // void* mmap(void* addr, size_t length, int prot, int flags, int fd, off_t offset);
    size_t string_len = strlen(so_path) + 1;
    // Align length to page size (usually 4096)
    size_t alloc_len = (string_len + 4095) & ~4095; 

    uint64_t mmap_args[6] = {
        0,                                      // addr: NULL (let OS choose)
        alloc_len,                              // length
        PROT_READ | PROT_WRITE | PROT_EXEC,     // prot: RWX (Exec might be needed depending on SELinux)
        MAP_PRIVATE | MAP_ANONYMOUS,            // flags: anonymous memory
        (uint64_t)-1,                           // fd: -1 for anonymous
        0                                       // offset: 0
    };

    LOGI("Calling remote mmap to allocate %zu bytes...", alloc_len);
    uint64_t remote_buf_addr = ptrace_call(pid, remote_mmap, mmap_args, 6);
    
    // mmap returns MAP_FAILED (-1) on error. We cast to uint64_t for comparison.
    if (remote_buf_addr == 0 || remote_buf_addr == (uint64_t)-1) {
        LOGE("Remote mmap failed. Cannot allocate safe memory.");
        ptrace(PTRACE_DETACH, pid, NULL, NULL);
        return -1;
    }
    LOGI("Allocated remote memory at: %p", (void*)remote_buf_addr);

    // Write the .so path into the safely allocated remote memory
    // Note: We write string_len, but padded to sizeof(long) for the ptrace_write logic
    size_t write_len = (string_len + sizeof(long) - 1) & ~(sizeof(long) - 1);
    
    // Create a temporary buffer padded with nulls to avoid reading junk past the string end
    char* padded_path = (char*)calloc(1, write_len);
    strcpy(padded_path, so_path);

    if (!ptrace_write(pid, remote_buf_addr, (void*)padded_path, write_len)) {
        LOGE("Write remote memory failed at safely allocated block");
        free(padded_path);
        ptrace(PTRACE_DETACH, pid, NULL, NULL);
        return -1;
    }
    free(padded_path);
    LOGI("Successfully wrote .so path to target process.");

    // Prepare arguments for dlopen(remote_buf_addr, RTLD_NOW)
    uint64_t dlopen_args[2] = { remote_buf_addr, RTLD_NOW };
    LOGI("Calling remote dlopen...");
    
    // Execute the remote call
    uint64_t handle = ptrace_call(pid, remote_dlopen, dlopen_args, 2);

    if (handle == 0) {
        LOGE("==========================================================");
        LOGE("Injection failed: dlopen returned NULL.");
        LOGE("Reason 1: Linker Namespace restrictions (Android 7+).");
        LOGE("Reason 2: Target app cannot access %s due to SELinux/Permissions.", so_path);
        LOGE("Reason 3: Architecture mismatch (32/64 bit).");
        LOGE("==========================================================");
    } else {
        LOGI("Injection success! Handle: %p", (void*)handle);
    }

    // Cleanup: Detach and let the process continue normally
    // We intentionally leak the mmaped buffer here, it's tiny and usually fine for injection.
    if (ptrace(PTRACE_DETACH, pid, NULL, NULL) < 0) {
        LOGE("Failed to detach gracefully.");
    } else {
        LOGI("Detached successfully.");
    }
    
    return 0;
}
