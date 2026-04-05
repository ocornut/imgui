#include <jni.h>
#include <pthread.h>
#include <unistd.h>
#include <android/log.h>
#include <string.h>
#include <sys/mman.h>
#include <stdio.h>  // 新增：修复 FILE, fopen 报错
#include <stdlib.h> // 新增：修复 strtoull 报错
#include "dobby.h"

#define TAG "JKMenu_SGame"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

// 获取模块基址
uintptr_t get_module_base(const char* name) {
    FILE* fp = fopen("/proc/self/maps", "r");
    char line[1024];
    uintptr_t base = 0;
    while (fp && fgets(line, sizeof(line), fp)) {
        if (strstr(line, name)) {
            base = (uintptr_t)strtoull(line, NULL, 16);
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

    // 王者荣耀核心库通常是 libGameCore.so 或 Unity 环境下的 libunity.so
    // 我们先循环检测，直到库加载完成
    while (game_base == 0 && retry < 60) {
        game_base = get_module_base("libGameCore.so"); 
        if (game_base == 0) {
            game_base = get_module_base("libunity.so"); // 兼容性检测
        }
        
        if (game_base == 0) {
            if (retry % 5 == 0) LOGI("Waiting for game libraries... (retry %d)", retry);
            sleep(1);
            retry++;
        }
    }

    if (game_base == 0) {
        LOGE("Error: Game core library not found after 60s.");
        return nullptr;
    }

    LOGI("Found game base at: %p", (void*)game_base);

    // TODO: 在此处添加你的 DobbyHook 逻辑
    // 例如: DobbyHook((void*)(game_base + 0x123456), (void*)new_func, (void**)&orig_func);

    LOGI("All initialization steps completed.");
    return nullptr;
}

__attribute__((constructor))
void _init() {
    pthread_t tid;
    // 使用分离模式创建线程，确保不会阻塞游戏进程
    pthread_create(&tid, nullptr, init_thread, nullptr);
    pthread_detach(tid);
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    return JNI_VERSION_1_6;
}
