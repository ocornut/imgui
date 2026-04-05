#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <dirent.h>
#include <dlfcn.h>
#include <sys/uio.h>
#include <elf.h>

#ifndef NT_PRSTATUS
#define NT_PRSTATUS 1
#endif

// --- 配置区 ---
const char* TARGET_PKG = "com.tencent.tmgp.sgame"; 
const char* LIB_PATH   = "/data/1/libJKMenu.so";

// 获取模块基址 (支持 APEX 路径)
uintptr_t get_module_base(pid_t pid, const char* module_name) {
    FILE* fp;
    uintptr_t addr = 0;
    char filename[64], line[1024];
    snprintf(filename, sizeof(filename), "/proc/%d/maps", pid);
    fp = fopen(filename, "r");
    if (fp) {
        while (fgets(line, sizeof(line), fp)) {
            // 只要包含关键字且是该段的起始地址
            if (strstr(line, module_name)) {
                addr = strtoull(line, NULL, 16);
                break;
            }
        }
        fclose(fp);
    }
    return addr;
}

// 计算远程 dlopen 地址
uintptr_t get_remote_dlopen(pid_t pid) {
    // 你的设备 linker 路径在 APEX 中，使用 linker64 关键字最稳
    const char* linker_name = "linker64";
    uintptr_t local_linker = get_module_base(getpid(), linker_name);
    uintptr_t remote_linker = get_module_base(pid, linker_name);

    if (!local_linker || !remote_linker) {
        printf("[-] Error: Linker not found. Local: %lx, Remote: %lx\n", local_linker, remote_linker);
        return 0;
    }

    // 优先尝试 __loader_dlopen (Android 8.0+)
    void* local_func = dlsym(RTLD_DEFAULT, "__loader_dlopen");
    if (!local_func) local_func = dlsym(RTLD_DEFAULT, "dlopen");

    uintptr_t offset = (uintptr_t)local_func - local_linker;
    return remote_linker + offset;
}

// 查找进程 PID
pid_t find_pid(const char* pkg) {
    DIR* dir = opendir("/proc");
    if (!dir) return -1;
    struct dirent* entry;
    while ((entry = readdir(dir))) {
        pid_t pid = atoi(entry->d_name);
        if (pid <= 0) continue;
        char path[256], cmd[256];
        snprintf(path, sizeof(path), "/proc/%d/cmdline", pid);
        FILE* f = fopen(path, "r");
        if (f) {
            if (fgets(cmd, sizeof(cmd), f) && strstr(cmd, pkg)) {
                fclose(f); closedir(dir); return pid;
            }
            fclose(f);
        }
    }
    closedir(dir); return -1;
}

int main() {
    printf("[*] JKMenu Injector v2.0 (Stable)\n");
    
    pid_t pid = find_pid(TARGET_PKG);
    if (pid == -1) { 
        printf("[-] Target %s not found!\n", TARGET_PKG); 
        return 1; 
    }

    uintptr_t dlopen_addr = get_remote_dlopen(pid);
    if (!dlopen_addr) return 1;
    printf("[+] Remote dlopen: %lx\n", dlopen_addr);

    // 1. Attach 目标进程
    if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) < 0) {
        perror("[-] Attach failed");
        return 1;
    }
    waitpid(pid, NULL, WUNTRACED);
    printf("[+] Attached to %d\n", pid);

    // 2. 备份寄存器现场
    struct user_pt_regs regs, old_regs;
    struct iovec iov = { .iov_base = &regs, .iov_len = sizeof(regs) };
    ptrace(PTRACE_GETREGSET, pid, (void*)(uintptr_t)NT_PRSTATUS, &iov);
    memcpy(&old_regs, &regs, sizeof(regs));

    // 3. 写入 SO 路径字符串到栈空间
    // 留出 512 字节安全区，并 16 字节对齐
    uintptr_t sp_path = (regs.sp - 512) & ~0xF;
    size_t len = strlen(LIB_PATH) + 1;
    for (size_t i = 0; i < len; i += 8) {
        long data = 0;
        memcpy(&data, LIB_PATH + i, (len - i) < 8 ? (len - i) : 8);
        ptrace(PTRACE_POKETEXT, pid, (void*)(sp_path + i), (void*)data);
    }

    // 4. 修改寄存器劫持执行流
    // AArch64 调用约定: x0=arg1, x1=arg2, lr=return_addr, pc=target_func
    regs.regs[0] = sp_path;       // 参数1: 路径
    regs.regs[1] = 2;             // 参数2: RTLD_NOW
    regs.pc = dlopen_addr;        // 跳往 dlopen
    regs.regs[30] = 0;            // LR 设为 0，让它运行完产生信号被我们捕获

    ptrace(PTRACE_SETREGSET, pid, (void*)(uintptr_t)NT_PRSTATUS, &iov);
    
    // 5. 触发并等待执行完成
    printf("[*] Calling remote dlopen...\n");
    ptrace(PTRACE_CONT, pid, NULL, NULL);

    int status;
    waitpid(pid, &status, WUNTRACED); 

    // 6. 恢复原始现场 (非常重要，防止崩溃)
    printf("[+] Restoring original state...\n");
    ptrace(PTRACE_SETREGSET, pid, (void*)(uintptr_t)NT_PRSTATUS, (struct iovec *)&old_regs);
    
    // 7. 释放控制权
    ptrace(PTRACE_DETACH, pid, NULL, NULL);
    printf("[+] Done! Check logcat for 'libJKMenu' messages.\n");

    return 0;
}
```

### 🚀 编译与使用指南

1.  **编译**：
    使用 NDK 中的 `aarch64-linux-android-clang` 编译，或者直接在手机端用带有开发环境的终端编译。确保目标架构是 **arm64-v8a**。

2.  **准备环境**：
    ```bash
    su
    setenforce 0
    chmod 777 /data/1/libJKMenu.so
    chmod 777 /data/1/jk_injector
    ```

3.  **运行监控**：
    开启一个窗口执行你之前的日志过滤命令：
    ```bash
    logcat -c && logcat -v color | grep -iE "JKMenu|Dobby|libJKMenu|dlopen" | grep -v "MediaProvider"
    ```

4.  **执行注入**：
    在游戏进入主界面（大厅）后运行：
    ```bash
    /data/1/jk_injector
