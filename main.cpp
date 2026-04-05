#include <jni.h>
#include <pthread.h>
#include <unistd.h>
#include <android/log.h>
#include <string.h>
#include <sys/mman.h>
#include "dobby.h" // 确保项目中有 dobby.h

#define TAG "JKMenu_SGame"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

// 模拟你的 Hook 目标（请根据实际偏移修改）
// 假设你要 Hook libUnity.so 或 libGameCore.so
uintptr_t get_module_base(const char* name) {
    FILE* fp = fopen("/proc/self/maps", "r");
    char line[1024];
    uintptr_t base = 0;
    while (fp && fgets(line, sizeof(line), fp)) {
        if (strstr(line, name)) {
            base = strtoull(line, NULL, 16);
            break;
        }
    }
    if (fp) fclose(fp);
    return base;
}

void* init_thread(void*) {
    LOGI("Thread started, entering monitoring loop...");

    uintptr_t game_base = 0;
    int retry = 0;

    // 1. 等待游戏主核心库加载（王者荣耀通常是 libGameCore.so 或 libunity.so）
    while (game_base == 0 && retry < 60) {
        game_base = get_module_base("libGameCore.so"); // 尝试获取核心库
        if (game_base == 0) {
            if (retry % 5 == 0) LOGI("Waiting for libGameCore.so... (retry %d)", retry);
            sleep(1);
            retry++;
        }
    }

    if (game_base == 0) {
        LOGE("Error: libGameCore.so not found after 60s, exiting thread.");
        return nullptr;
    }

    LOGI("Found libGameCore.so at: %p", (void*)game_base);

    // 2. 执行 Hook 逻辑
    // 建议先只 Hook 一个最简单的函数测试稳定性
    /*
    uintptr_t target_offset = 0x123456; // 你的偏移
    void* target_addr = (void*)(game_base + target_offset);
    
    LOGI("Attempting to Hook at %p", target_addr);
    
    // DobbyHook(target_addr, (void*)my_fake_func, (void**)&orig_func);
    */

    LOGI("All initialization steps completed.");
    return nullptr;
}

__attribute__((constructor))
void _init() {
    pthread_t tid;
    // 创建分离线程，防止阻塞导致游戏启动黑屏
    pthread_create(&tid, nullptr, init_thread, nullptr);
    pthread_detach(tid);
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    return JNI_VERSION_1_6;
}
