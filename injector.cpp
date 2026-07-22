#define _GNU_SOURCE
#define __USE_GNU

#ifndef PTRACE_GETREGS
#define PTRACE_GETREGS  12
#endif
#ifndef PTRACE_SETREGS
#define PTRACE_SETREGS  13
#endif

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <sys/reg.h>
#include <sys/ucontext.h>
#include <sys/mman.h>
#include <dirent.h>
#include <fcntl.h>
#include <vector>
#include <errno.h>
#include <dlfcn.h>

#define GAME_PACKAGE "com.tencent.jkchess"
#define SO_PATH "/data/1/libMyMenu.so"
#define PAGE_SIZE 4096
#define PAGE_ALIGN(n) (((n) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))

using u64 = uint64_t;
using s64 = int64_t;

pid_t get_target_pid() {
    DIR* dir = opendir("/proc");
    if (!dir) return -1;
    struct dirent* ent;
    while ((ent = readdir(dir))) {
        if (!(ent->d_name[0] >= '0' && ent->d_name[0] <= '9')) continue;
        pid_t pid = atoi(ent->d_name);
        char path[256], buf[1024] = {0};
        snprintf(path, sizeof(path), "/proc/%d/cmdline", pid);
        FILE* f = fopen(path, "rb");
        if (!f) continue;
        fread(buf, sizeof(buf)-1, 1, f);
        fclose(f);
        if (strstr(buf, GAME_PACKAGE)) {
            closedir(dir);
            return pid;
        }
    }
    closedir(dir);
    return -1;
}

bool pt_write(pid_t pid, u64 addr, const void* data, size_t len) {
    const u64* src = (const u64*)data;
    size_t full = len / 8;
    size_t rem = len % 8;
    for (size_t i = 0; i < full; i++) {
        if (ptrace(PTRACE_POKETEXT, pid, addr + i * 8, src[i]) == -1)
            return false;
    }
    if (rem > 0) {
        u64 last = 0;
        ptrace(PTRACE_PEEKTEXT, pid, addr + full * 8, &last);
        memcpy(&last, (char*)src + full * 8, rem);
        if (ptrace(PTRACE_POKETEXT, pid, addr + full * 8, last) == -1)
            return false;
    }
    return true;
}

bool pt_read(pid_t pid, u64 addr, void* out, size_t len) {
    u64* dst = (u64*)out;
    size_t full = len / 8;
    size_t rem = len % 8;
    for (size_t i = 0; i < full; i++) {
        if (ptrace(PTRACE_PEEKTEXT, pid, addr + i * 8, &dst[i]) == -1)
            return false;
    }
    if (rem > 0) {
        u64 last = 0;
        if (ptrace(PTRACE_PEEKTEXT, pid, addr + full * 8, &last) == -1)
            return false;
        memcpy((char*)dst + full * 8, &last, rem);
    }
    return true;
}

u64 remote_call(pid_t pid, u64 func, u64 x0=0, u64 x1=0, u64 x2=0, u64 x3=0, u64 x4=0, u64 x5=0) {
    user_regs_struct regs;
    ptrace(PTRACE_GETREGS, pid, nullptr, &regs);
    regs.regs[0] = x0;
    regs.regs[1] = x1;
    regs.regs[2] = x2;
    regs.regs[3] = x3;
    regs.regs[4] = x4;
    regs.regs[5] = x5;
    regs.sp -= 0x2000;
    regs.pc = func;
    ptrace(PTRACE_SETREGS, pid, nullptr, &regs);
    ptrace(PTRACE_CONT, pid, nullptr, nullptr);
    waitpid(pid, nullptr, 0);
    ptrace(PTRACE_GETREGS, pid, nullptr, &regs);
    return regs.regs[0];
}

// 【修复点】正确扫描shturl.基地址
u64 get_lib_base(pid_t pid) {
    char buf[512], path[256];
    snprintf(path, sizeof(path), "/proc/%d/maps", pid);
    FILE* f = fopen(path, "r");
    if (!f) return 0;
    u64 base = 0;
    while (fgets(buf, sizeof(buf), f)) {
        if (strstr(buf, "shturl.")) {
            sscanf(buf, "%lx", &base);
            break;
        }
    }
    fclose(f);
    return base;
}

u64 get_libc_sym(u64 lib_base, const char* sym) {
    void* local_sym = dlsym(RTLD_DEFAULT, sym);
    if (!local_sym) return 0;
    Dl_info info;
    dladdr(local_sym, &info);
    u64 off = (u64)local_sym - (u64)info.dli_fbase;
    return lib_base + off;
}

int main() {
    printf("======== AArch64 Android Ptrace Injector ========\n");
    printf("Target SO: %s\n", SO_PATH);

    system("su -c setenforce 0");
    sleep(1);

    pid_t target_pid = get_target_pid();
    if (target_pid <= 0) {
        printf("Error: Game process not found, launch game first!\n");
        return 1;
    }
    printf("Target PID: %d\n", target_pid);

    if (ptrace(PTRACE_ATTACH, target_pid, nullptr, nullptr) == -1) {
        perror("PTRACE_ATTACH failed, check root");
        return 1;
    }
    waitpid(target_pid, nullptr, 0);
    printf("Attached target process\n");

    u64 libc_base = get_lib_base(target_pid);
    if (libc_base == 0) {
        printf("Failed get libc base address\n");
        ptrace(PTRACE_DETACH, target_pid, nullptr, nullptr);
        return 1;
    }

    u64 mmap_sym = get_libc_sym(libc_base, "mmap");
    u64 munmap_sym = get_libc_sym(libc_base, "munmap");
    u64 dlopen_sym = get_libc_sym(libc_base, "dlopen");
    if (!mmap_sym || !munmap_sym || !dlopen_sym) {
        printf("Resolve libc function failed\n");
        ptrace(PTRACE_DETACH, target_pid, nullptr, nullptr);
        return 1;
    }

    const char* path_str = SO_PATH;
    size_t str_len = strlen(path_str) + 1;
    size_t str_alloc = PAGE_ALIGN(str_len);

    u64 str_ptr = remote_call(target_pid, mmap_sym,
        0, str_alloc, PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, 0, 0);

    if (str_ptr == 0 || str_ptr == ULLONG_MAX) {
        printf("Remote mmap allocate failed\n");
        ptrace(PTRACE_DETACH, target_pid, nullptr, nullptr);
        return 1;
    }

    pt_write(target_pid, str_ptr, path_str, str_len);
    u64 handle = remote_call(target_pid, dlopen_sym, str_ptr, RTLD_NOW | RTLD_GLOBAL);

    if (handle != 0) {
        printf("✅ Inject Success! Handle: 0x%lx\n", handle);
    } else {
        printf("❌ Inject Failed, dlopen returned NULL\n");
    }

    remote_call(target_pid, munmap_sym, str_ptr, str_alloc);
    ptrace(PTRACE_DETACH, target_pid, nullptr, nullptr);
    return 0;
}
