#ifndef MEMORY_UTILS_H
#define MEMORY_UTILS_H

#include <unistd.h>
#include <sys/uio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

// --- 安全读取内存逻辑 (学习自“那份代码”) ---
// 使用 process_vm_readv 或者 pipe 探测法来防止非法地址读取导致的闪退

template <typename T>
T Read(uintptr_t address) {
    T data{};
    if (address < 0x1000000 || address > 0x7FFFFFFFFFFF) {
        return data; // 过滤明显的非法指针
    }

    struct iovec local[1];
    struct iovec remote[1];

    local[0].iov_base = &data;
    local[0].iov_len = sizeof(T);
    remote[0].iov_base = (void*)address;
    remote[0].iov_len = sizeof(T);

    // 在内部注入时，如果 process_vm_readv 不可用，可以尝试使用安全拷贝
    // 这里的实现保证了如果地址无效，程序不会直接崩溃
    ssize_t nread = process_vm_readv(getpid(), local, 1, remote, 1, 0);
    
    if (nread < 0) {
        // 如果系统不支持 vm_readv，回退到基本内存校验（可选实现）
        return data; 
    }

    return data;
}

// 专门用于读取 Il2cpp 字符串或游戏名称的工具
inline char* ReadString(uintptr_t address, size_t size) {
    static char buffer[256];
    memset(buffer, 0, sizeof(buffer));
    
    struct iovec local[1];
    struct iovec remote[1];

    local[0].iov_base = buffer;
    local[0].iov_len = (size > 255) ? 255 : size;
    remote[0].iov_base = (void*)address;
    remote[0].iov_len = local[0].iov_len;

    process_vm_readv(getpid(), local, 1, remote, 1, 0);
    return buffer;
}

#endif // MEMORY_UTILS_H
