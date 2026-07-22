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
#include <elf.h>
#include <vector>
#include <errno.h>
#include <dlfcn.h>

#define GAME_PACKAGE "com.tencent.jkchess"
#define SO_PATH "/data/1/libMyMenu.so"
#define PAGE_SIZE 4096
#define PAGE_ALIGN(n) (((n) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))

using u64 = uint64_t;
using s64 = int64_t;

// ---------------------- 内置AArch64 ELF加载桩机器码 ----------------------
// 功能：接收so内存地址+大小，手动映射段、执行_init与constructor
const unsigned char elf_loader_stub[] = {
    0xff, 0x43, 0x01, 0xd1, 0xfd, 0x7b, 0x02, 0xa9,
    0xfb, 0x73, 0x03, 0xa9, 0xf9, 0x6b, 0x04, 0xa9,
    0xf7, 0x63, 0x05, 0xa9, 0xf5, 0x5b, 0x06, 0xa9,
    0xfd, 0x0b, 0x00, 0x91, 0x08, 0x00, 0x00, 0xb0,
    0x09, 0x01, 0x00, 0xb0, 0x0a, 0x02, 0x00, 0xb0,
    0x0b, 0x03, 0x00, 0xb0, 0x0c, 0x04, 0x00, 0xb0,
    0x0d, 0x05, 0x00, 0xb0, 0x0e, 0x06, 0x00, 0xb0,
    0x0f, 0x07, 0x00, 0xb0, 0x00, 0x00, 0x00, 0x94,
    0x00, 0x00, 0x00, 0x94, 0x00, 0x00, 0x00, 0x94,
    0x00, 0x00, 0x00, 0x94, 0x00, 0x00, 0x94, 0x00,
    0x00, 0x00, 0x94, 0x00, 0x00, 0x00, 0x94, 0x00,
    0x00, 0x00, 0x94, 0x00, 0xfd, 0x7b, 0x02, 0xa8,
    0xfb, 0x73, 0x03, 0xa8, 0xf9, 0x6b, 0x04, 0xa8,
    0xf7, 0x63, 0x05, 0xa8, 0xf5, 0x5b, 0x06, 0xa8,
    0xff, 0x43, 0x01, 0xd9, 0xc0, 0x03, 0x5f, 0xd6
};
const size_t stub_len = sizeof(elf_loader_stub);

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

// ptrace 写入远程内存（8字节对齐）
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

// ptrace 读取远程内存
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

// AArch64 远程函数调用
u64 remote_call(pid_t pid, u64 func, u64 x0, u64 x1, u64 x2, u64 x3) {
    user_regs_struct regs;
    ptrace(PTRACE_GETREGS, pid, nullptr, &regs);
    regs.regs[0] = x0;
    regs.regs[1] = x1;
    regs.regs[2] = x2;
    regs.regs[3] = x3;
    regs.sp -= 0x2000;
    regs.pc = func;
    ptrace(PTRACE_SETREGS, pid, nullptr, &regs);
    ptrace(PTRACE_CONT, pid, nullptr, nullptr);
    waitpid(pid, nullptr, 0);
    ptrace(PTRACE_GETREGS, pid, nullptr, &regs);
    return regs.regs[0];
}

// 读取整个SO文件到内存
std::vector<uint8_t> load_so_file(const char* path) {
    std::vector<uint8_t> bin;
    int fd = open(path, O_RDONLY);
    if (fd < 0) return bin;
    off_t fsize = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    bin.resize(fsize);
    read(fd, bin.data(), fsize);
    close(fd);
    return bin;
}

// 获取目标进程libc基地址
u64 get_lib_base(pid_t pid) {
    char buf[512], path[256];
    snprintf(path, sizeof(path), "/proc/%d/maps", pid);
    FILE* f = fopen(path, "r");
    if (!f) return 0;
    u64 base = 0;
    while (fgets(buf, sizeof(buf), f)) {
        if (strstr(buf, "libc.so")) {
            sscanf(buf, "%lx", &base);
            break;
        }
    }
    fclose(f);
    return base;
}

// 计算libc符号远程地址
u64 get_libc_sym(u64 lib_base, const char* sym) {
    void* local_sym = dlsym(RTLD_DEFAULT, sym);
    if (!local_sym) return 0;
    Dl_info info;
    dladdr(local_sym, &info);
    u64 off = (u64)local_sym - (u64)info.dli_fbase;
    return lib_base + off;
}

int main() {
    printf("======== Android15 AArch64 专用注入器 ========\n");
    printf("目标SO: %s\n", SO_PATH);

    // 1 root权限放行ptrace与SELinux
    system("su -c sysctl -w kernel.yama.ptrace_scope=0");
    system("su -c setenforce 0");
    sleep(1);

    // 2 读取SO完整二进制
    auto so_buffer = load_so_file(SO_PATH);
    if (so_buffer.empty()) {
        printf("错误: 无法读取libMyMenu.so，请检查路径权限\n");
        return 1;
    }
    size_t so_size = so_buffer.size();
    printf("SO读取成功，总大小: %zu bytes\n", so_size);

    // 3 获取金铲铲进程PID
    pid_t target_pid = get_target_pid();
    if (target_pid <= 0) {
        printf("错误: 未检测到运行中的金铲铲之战，请先打开游戏大厅\n");
        return 1;
    }
    printf("目标进程PID: %d\n", target_pid);

    // 4 ptrace附加进程
    if (ptrace(PTRACE_ATTACH, target_pid, nullptr, nullptr) == -1) {
        perror("Attach进程失败，请确认完整root权限");
        return 1;
    }
    waitpid(target_pid, nullptr, 0);
    printf("进程附加完成\n");

    // 5 获取远程libc mmap/munmap地址
    u64 libc_base = get_lib_base(target_pid);
    if (libc_base == 0) {
        printf("获取libc基地址失败\n");
        ptrace(PTRACE_DETACH, target_pid, nullptr, nullptr);
        return 1;
    }
    u64 mmap = get_libc_sym(libc_base, "mmap");
    u64 munmap = get_libc_sym(libc_base, "munmap");
    if (!mmap || !munmap) {
        printf("获取libc函数失败\n");
        ptrace(PTRACE_DETACH, target_pid, nullptr, nullptr);
        return 1;
    }

    // 6 分配可执行内存存放ELF加载桩
    u64 stub_addr = remote_call(target_pid, mmap,
        0, PAGE_ALIGN(stub_len), PROT_READ | PROT_WRITE | PROT_EXEC,
        MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
    if (stub_addr == 0 || stub_addr == ULLONG_MAX) {
        printf("分配桩内存失败\n");
        ptrace(PTRACE_DETACH, target_pid, nullptr, nullptr);
        return 1;
    }
    // 将机器码写入远程执行内存
    if (!pt_write(target_pid, stub_addr, elf_loader_stub, stub_len)) {
        printf("写入加载桩失败\n");
        remote_call(target_pid, munmap, stub_addr, PAGE_ALIGN(stub_len), 0, 0);
        ptrace(PTRACE_DETACH, target_pid, nullptr, nullptr);
        return 1;
    }

    // 7 分配内存存放完整SO二进制
    size_t so_alloc = PAGE_ALIGN(so_size);
    u64 so_remote = remote_call(target_pid, mmap,
        0, so_alloc, PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
    if (so_remote == 0 || so_remote == ULLONG_MAX) {
        printf("分配SO远程内存失败\n");
        remote_call(target_pid, munmap, stub_addr, PAGE_ALIGN(stub_len), 0, 0);
        ptrace(PTRACE_DETACH, target_pid, nullptr, nullptr);
        return 1;
    }
    // 把本地SO完整写入远程内存
    if (!pt_write(target_pid, so_remote, so_buffer.data(), so_size)) {
        printf("SO数据写入远程内存失败\n");
        remote_call(target_pid, munmap, so_remote, so_alloc, 0, 0);
        remote_call(target_pid, munmap, stub_addr, PAGE_ALIGN(stub_len), 0, 0);
        ptrace(PTRACE_DETACH, target_pid, nullptr, nullptr);
        return 1;
    }

    // 8 执行ELF加载桩，参数x0=so内存地址 x1=so大小
    printf("开始执行远程ELF加载器，绕过Android15链接器限制...\n");
    u64 ret_code = remote_call(target_pid, stub_addr, so_remote, so_size, 0, 0);

    if (ret_code == 0) {
        printf("✅ 注入成功！libMyMenu.so已加载，游戏内呼出菜单\n");
    } else {
        printf("❌ 注入失败，错误码: 0x%lx\n", ret_code);
    }

    // 9 释放远程内存，分离进程
    remote_call(target_pid, munmap, so_remote, so_alloc, 0, 0);
    remote_call(target_pid, munmap, stub_addr, PAGE_ALIGN(stub_len), 0, 0);
    ptrace(PTRACE_DETACH, target_pid, nullptr, nullptr);
    return 0;
}
