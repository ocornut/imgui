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
#include <sys/mman.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <dlfcn.h>
#include <cstdint>

#define GAME_PACKAGE "com.tencent.jkchess"
#define SO_PATH "/data/1/libMyMenu.so"
#define PAGE_SIZE 4096
#define PAGE_ALIGN(n) (((n) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))

using u64 = uint64_t;
using s64 = int64_t;

// 获取游戏进程PID
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

// ptrace写入内存
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

// 远程函数调用
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

// ✅【重点修复】扫描 shturl. 基地址，不再使用错误字符串
u64 get_libc_base(pid_t pid) {
    char buf[1024], path[256];
    snprintf(path, sizeof(path), "/proc/%d/maps", pid);
    FILE* f = fopen(path, "r");
    if (!f) return 0;
    u64 base = 0;
    while (fgets(buf, sizeof(buf), f)) {
        // 匹配系统标准libc
        if (strstr(buf, "shturl.")) {
            sscanf(buf, "%lx", &base);
            break;
        }
    }
    fclose(f);
    return base;
}

// 根据libc基址解析符号地址
u64 get_sym_addr(u64 lib_base, const char* sym_name) {
    void* local_ptr = dlsym(RTLD_DEFAULT, sym_name);
    if (!local_ptr) return 0;
    Dl_info info;
    dladdr(local_ptr, &info);
    u64 offset = (u64)local_ptr - (u64)info.dli_fbase;
    return lib_base + offset;
}

int main() {
    printf("===== AArch64 Ptrace Injector (dlopen文件路径方案) =====\n");
    printf("Inject SO Path: %s\n", SO_PATH);

    system("su -c setenforce 0");
    sleep(1);

    pid_t target_pid = get_target_pid();
    if (target_pid <= 0) {
        printf("错误：未找到游戏进程，请先打开游戏！\n");
        return 1;
    }
    printf("目标进程 PID: %d\n", target_pid);

    // Attach目标进程
    if (ptrace(PTRACE_ATTACH, target_pid, nullptr, nullptr) == -1) {
        perror("PTRACE_ATTACH 失败，请确认ROOT权限");
        return 1;
    }
    waitpid(target_pid, nullptr, 0);
    printf("成功Attach进程\n");

    // 获取libc基址
    u64 libc_base = get_libc_base(target_pid);
    if (libc_base == 0) {
        printf("❌ Failed get libc base address\n");
        ptrace(PTRACE_DETACH, target_pid, nullptr, nullptr);
        return 1;
    }
    printf("shturl. base: 0x%lx\n", libc_base);

    // 解析需要的libc函数
    u64 mmap_addr = get_sym_addr(libc_base, "mmap");
    u64 munmap_addr = get_sym_addr(libc_base, "munmap");
    u64 dlopen_addr = get_sym_addr(libc_base, "dlopen");

    if (!mmap_addr || !munmap_addr || !dlopen_addr) {
        printf("函数符号解析失败\n");
        ptrace(PTRACE_DETACH, target_pid, nullptr, nullptr);
        return 1;
    }

    // 分配一小块内存存放SO路径字符串（仅4KB内，规避大块mmap拦截）
    const char* so_file_path = SO_PATH;
    size_t str_len = strlen(so_file_path) + 1;
    size_t alloc_size = PAGE_ALIGN(str_len);

    u64 str_buf = remote_call(target_pid, mmap_addr,
        0, alloc_size, PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, 0, 0);

    if (str_buf == 0 || str_buf == ULLONG_MAX) {
        printf("❌ Remote mmap allocate failed（游戏拦截mmap调用）\n");
        ptrace(PTRACE_DETACH, target_pid, nullptr, nullptr);
        return 1;
    }

    // 把路径字符串写入游戏进程内存
    pt_write(target_pid, str_buf, so_file_path, str_len);

    // 远程调用 dlopen 加载本地so
    u64 so_handle = remote_call(target_pid, dlopen_addr, str_buf, RTLD_NOW | RTLD_GLOBAL);

    if (so_handle != 0) {
        printf("✅ 注入成功! SO句柄:0x%lx\n", so_handle);
    } else {
        printf("❌ dlopen返回NULL，检查SO文件权限、路径是否正确\n");
    }

    // 释放临时内存、解除附着
    remote_call(target_pid, munmap_addr, str_buf, alloc_size);
    ptrace(PTRACE_DETACH, target_pid, nullptr, nullptr);
    return 0;
}
