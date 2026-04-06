#include <pthread.h>
#include <unistd.h>
#include <android/log.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <dlfcn.h>
#include <android/input.h>
#include <stdio.h>
#include <algorithm>
#include <atomic>
#include <vector>
#include <mutex>
#include <thread>
#include <chrono>
#include <fcntl.h>
#include <dirent.h>
#include "dobby.h"
#include "imgui.h"
#include "imgui_impl_android.h"
#include "imgui_impl_opengl3.h"

#define TAG "ZeroConfig_Drive"
#define GAME_MODULE "libil2cpp.so" 

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

static bool g_Initialized = false;
static bool g_FontLoaded = false; 

struct {
    float x = 0.0f;
    float y = 0.0f;
    std::atomic<bool> down{false};
    float renderW = 0.0f; 
    float renderH = 0.0f;
    float physW = 3392.0f;
    float physH = 2400.0f;
    float offsetX = 0.0f;
    float offsetY = 0.0f;
} g_Touch;

static float g_DynamicScale = 1.0f;
static float g_UserUIScale = 1.2f; 

// Hook 状态实时监控
static std::atomic<int> g_InitActorTriggerCount{0};
static std::atomic<int> g_DobbyStatus{-99}; 
static uintptr_t g_GameBase = 0;
static uintptr_t g_HookAddr_InitActor = 0;

// =================================================================
// 核心读取：Pipe 安全管道模式
// =================================================================
static int g_SafePipe[2] = {-1, -1};
void InitSafePipe() {
    if (g_SafePipe[0] == -1 && pipe(g_SafePipe) == -1) {
        LOGE("[-] Pipeline Create Failed!");
    }
}

template <typename T>
T SafeRead(uintptr_t address) {
    T value{};
    if (address < 0x10000000 || address > 0x00007FFFFFFFFFFF) return value;
    if (g_SafePipe[0] == -1) InitSafePipe();
    if (write(g_SafePipe[1], (void*)address, sizeof(T)) == -1) return value;
    read(g_SafePipe[0], &value, sizeof(T));
    return value;
}

struct DynamicOffsets {
    uint32_t x0_to_addr1    = 32;   
    uint32_t addr1_to_addr2 = 16;   
    uint32_t item_to_addr3  = 24;   
    uint32_t addr3_to_addr4 = 48;   
    uint32_t prop_hp        = 48;
};

DynamicOffsets g_Offsets;
std::mutex g_OffsetMutex;

struct DebugChain {
    uintptr_t baseX0 = 0;
    uintptr_t addr1 = 0;
    uintptr_t addr2 = 0;
    struct Item {
        uintptr_t base = 0, addr3 = 0, addr4 = 0, hp_final_ptr = 0;
    } items[10];
};

struct GameSnapshot {
    bool inMatch = false;
    DynamicOffsets offsets; 
    DebugChain debug;
};

std::mutex g_SnapshotMutex;
GameSnapshot g_CurrentSnapshot; 
std::atomic<uintptr_t> g_ActorManager{0};

void DataWorkerThread() {
    while (true) {
        GameSnapshot newSnap;
        DynamicOffsets currOff;
        { std::lock_guard<std::mutex> lock(g_OffsetMutex); currOff = g_Offsets; }
        newSnap.offsets = currOff;

        uintptr_t baseX0 = g_ActorManager.load();
        if (baseX0 != 0) {
            newSnap.inMatch = true;
            newSnap.debug.baseX0 = baseX0;
            uintptr_t addr1 = SafeRead<uintptr_t>(baseX0 + currOff.x0_to_addr1);
            newSnap.debug.addr1 = addr1;
            if (addr1) {
                uintptr_t addr2 = SafeRead<uintptr_t>(addr1 + currOff.addr1_to_addr2);
                newSnap.debug.addr2 = addr2;
                if (addr2) {
                    for (int i = 0; i < 10; ++i) { 
                        uintptr_t item = SafeRead<uintptr_t>(addr2 + (i * 0x8)); 
                        newSnap.debug.items[i].base = item;
                        if (!item) continue;
                        uintptr_t addr3 = SafeRead<uintptr_t>(item + currOff.item_to_addr3);
                        newSnap.debug.items[i].addr3 = addr3;
                        if (addr3) {
                            uintptr_t addr4 = SafeRead<uintptr_t>(addr3 + currOff.addr3_to_addr4);
                            newSnap.debug.items[i].addr4 = addr4;
                            if (addr4) newSnap.debug.items[i].hp_final_ptr = addr4 + currOff.prop_hp;
                        }
                    }
                }
            }
        }
        { std::lock_guard<std::mutex> lock(g_SnapshotMutex); g_CurrentSnapshot = newSnap; }
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}

uintptr_t GetModuleBase(const char* module_name) {
    uintptr_t base = 0;
    FILE* f = fopen("/proc/self/maps", "r");
    if (!f) return 0;
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, module_name)) {
            sscanf(line, "%lx", &base);
            break;
        }
    }
    fclose(f);
    return base;
}

void (*old_InitActorParams)(void* x0, void* x1, void* x2, void* x3);
void hook_InitActorParams(void* x0, void* x1, void* x2, void* x3) {
    g_InitActorTriggerCount++;
    LOGI("[TRIGGER] InitActorParams Hooked! x0 = %p", x0);
    g_ActorManager.store((uintptr_t)x0);
    old_InitActorParams(x0, x1, x2, x3);
}

// =================================================================
// UI 绘制与状态输出
// =================================================================
void DrawPointerDebugger(const GameSnapshot& snap, float w, float h) {
    ImGui::SetNextWindowPos(ImVec2(w * 0.45f, 50), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(w * 0.52f, h * 0.85f), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin(u8"地址链路全监控 (Debug Mode)")) {
        ImGui::TextColored(ImVec4(1, 1, 0, 1), u8"---- 核心 Hook 状态诊断 ----");
        ImGui::Text(u8"Dobby 状态: %d (0成功)", (int)g_DobbyStatus);
        ImGui::Text(u8"触发次数: %d", (int)g_InitActorTriggerCount);
        if (g_InitActorTriggerCount == 0) {
            ImGui::TextColored(ImVec4(1, 0.2f, 0.2f, 1), u8"提示: 进局内加载时才触发捕获!");
        }
        ImGui::Separator();

        if (ImGui::CollapsingHeader(u8"十进制偏移调节", ImGuiTreeNodeFlags_DefaultOpen)) {
            std::lock_guard<std::mutex> lock(g_OffsetMutex);
            auto drawBtn = [](const char* label, uint32_t& val) {
                ImGui::PushID(&val);
                ImGui::Text("%s: %u", label, val);
                if (ImGui::Button("-1")) val -= 1; ImGui::SameLine();
                if (ImGui::Button("+1")) val += 1; ImGui::SameLine();
                if (ImGui::Button("+8")) val += 8;
                ImGui::PopID();
                ImGui::Separator();
            };
            drawBtn(u8"Addr1 (X0+?)", g_Offsets.x0_to_addr1);
            drawBtn(u8"Array (A1+?)", g_Offsets.addr1_to_addr2);
        }

        const auto& d = snap.debug;
        ImGui::TextColored(ImVec4(0, 1, 1, 1), u8"基址: 0x%lx", g_GameBase);
        ImGui::TextColored(d.baseX0 ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1), u8"X0 (起点): 0x%lx", d.baseX0);
        if (d.baseX0) {
            ImGui::TextColored(d.addr1 ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1), u8" └─ Addr1: 0x%lx", d.addr1);
            ImGui::TextColored(d.addr2 ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1), u8"     └─ Array: 0x%lx", d.addr2);
        }
    }
    ImGui::End();
}

typedef EGLBoolean (*p_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
static p_eglSwapBuffers old_eglSwapBuffers = nullptr;

// =================================================================
// 核心修复：极致 OpenGL 状态保护，防止画面花屏
// =================================================================
EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    EGLint w, h; eglQuerySurface(dpy, surface, EGL_WIDTH, &w); eglQuerySurface(dpy, surface, EGL_HEIGHT, &h);
    if (w <= 0 || h <= 0) return old_eglSwapBuffers(dpy, surface);
    g_Touch.renderW = (float)w; g_Touch.renderH = (float)h;

    // --- [1] 深度备份所有可能影响画面的 OpenGL 状态 ---
    GLint last_program, last_texture, last_active_texture, last_array_buffer, last_element_array_buffer, last_vertex_array;
    GLint last_viewport[4], last_scissor_box[4];
    GLint last_blend_src_rgb, last_blend_dst_rgb, last_blend_src_alpha, last_blend_dst_alpha, last_blend_equation_rgb, last_blend_equation_alpha;
    GLboolean last_enable_blend = glIsEnabled(GL_BLEND);
    GLboolean last_enable_cull_face = glIsEnabled(GL_CULL_FACE);
    GLboolean last_enable_depth_test = glIsEnabled(GL_DEPTH_TEST);
    GLboolean last_enable_scissor_test = glIsEnabled(GL_SCISSOR_TEST);
    GLboolean last_depth_mask; glGetBooleanv(GL_DEPTH_WRITEMASK, &last_depth_mask);

    glGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &last_active_texture);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &last_element_array_buffer);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array);
    glGetIntegerv(GL_VIEWPORT, last_viewport);
    glGetIntegerv(GL_SCISSOR_BOX, last_scissor_box);
    glGetIntegerv(GL_BLEND_SRC_RGB, &last_blend_src_rgb);
    glGetIntegerv(GL_BLEND_DST_RGB, &last_blend_dst_rgb);
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &last_blend_src_alpha);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &last_blend_dst_alpha);
    glGetIntegerv(GL_BLEND_EQUATION_RGB, &last_blend_equation_rgb);
    glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &last_blend_equation_alpha);

    // --- [2] 绘制 ImGui ---
    GameSnapshot snapshot;
    { std::lock_guard<std::mutex> lock(g_SnapshotMutex); snapshot = g_CurrentSnapshot; }

    if (!g_Initialized) {
        ImGui::CreateContext();
        ImGui_ImplOpenGL3_Init("#version 300 es");
        ImGuiIO& io = ImGui::GetIO();
        const char* myFonts[] = { "/system/fonts/SysSans-Hans-Regular.ttf", "/system/fonts/NotoSansCJKjp-Regular.otc", "/system/fonts/NotoSansSC-Regular.otf" };
        for (int i = 0; i < 3; i++) {
            if (access(myFonts[i], R_OK) == 0) {
                io.Fonts->AddFontFromFileTTF(myFonts[i], 32.0f, NULL, io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
                g_FontLoaded = true; break;
            }
        }
        g_Initialized = true;
    }

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(g_Touch.renderW, g_Touch.renderH);
    io.MousePos = ImVec2(g_Touch.x, g_Touch.y);
    io.MouseDown[0] = g_Touch.down.load(); 

    float finalScale = (h / 1000.0f) * g_UserUIScale; 
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(1.2f * finalScale);
    style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
    io.FontGlobalScale = 1.2f * finalScale;

    ImGui_ImplOpenGL3_NewFrame(); ImGui::NewFrame();
    
    ImGui::SetNextWindowPos(ImVec2(50, 50), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(w * 0.4f, h * 0.3f), ImGuiCond_FirstUseEver);
    if (ImGui::Begin(u8"自动对局控制台 (画面保护版)")) {
        ImGui::SliderFloat(u8"UI 缩放", &g_UserUIScale, 0.5f, 2.0f);
        if (ImGui::Button(u8"重置触控")) { g_Touch.offsetX = 0; g_Touch.offsetY = 0; }
        ImGui::Text(u8"状态: %s", snapshot.inMatch ? u8"读取中" : u8"待命");
    }
    ImGui::End();

    DrawPointerDebugger(snapshot, w, h);
    
    ImGui::Render();
    glViewport(0, 0, w, h); 
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // --- [3] 极致还原所有 OpenGL 状态 ---
    glUseProgram(last_program);
    glBindTexture(GL_TEXTURE_2D, last_texture);
    glActiveTexture(last_active_texture);
    glBindVertexArray(last_vertex_array);
    glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, last_element_array_buffer);
    glBlendEquationSeparate(last_blend_equation_rgb, last_blend_equation_alpha);
    glBlendFuncSeparate(last_blend_src_rgb, last_blend_dst_rgb, last_blend_src_alpha, last_blend_dst_alpha);
    glViewport(last_viewport[0], last_viewport[1], last_viewport[2], last_viewport[3]);
    glScissor(last_scissor_box[0], last_scissor_box[1], last_scissor_box[2], last_scissor_box[3]);
    glDepthMask(last_depth_mask);
    
    if (last_enable_blend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
    if (last_enable_cull_face) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
    if (last_enable_depth_test) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if (last_enable_scissor_test) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);

    return old_eglSwapBuffers(dpy, surface);
}

// 触摸 Hook (略)
typedef int (*p_consume)(void* instance, void* factory, bool consumeBatches, int64_t frameTime, uint32_t* outSeq, void** outEvent);
static p_consume old_consume = nullptr;
int hook_consume(void* instance, void* factory, bool consumeBatches, int64_t frameTime, uint32_t* outSeq, void** outEvent) {
    int res = old_consume(instance, factory, consumeBatches, frameTime, outSeq, outEvent);
    if (res == 0 && outEvent && *outEvent) {
        AInputEvent* ev = (AInputEvent*)(*outEvent);
        if (AInputEvent_getType(ev) == AINPUT_EVENT_TYPE_MOTION) {
            int32_t action = AMotionEvent_getAction(ev) & AMOTION_EVENT_ACTION_MASK;
            float scX = g_Touch.renderW / g_Touch.physW; float scY = g_Touch.renderH / g_Touch.physH;
            g_Touch.x = (AMotionEvent_getX(ev, 0) * scX) + g_Touch.offsetX;
            g_Touch.y = (AMotionEvent_getY(ev, 0) * scY) + g_Touch.offsetY;
            if (action == AMOTION_EVENT_ACTION_DOWN) g_Touch.down.store(true);
            else if (action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_CANCEL) g_Touch.down.store(false);
        }
    }
    return res;
}

void* DelayedHookThread(void*) {
    InitSafePipe();
    void* egl = DobbySymbolResolver("libEGL.so", "eglSwapBuffers");
    if (egl) DobbyHook(egl, (void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);
    void* ins = DobbySymbolResolver("libinput.so", "_ZN7android13InputConsumer7consumeEPNS_26InputEventFactoryInterfaceEblPjPPNS_10InputEventE");
    if (ins) DobbyHook(ins, (void*)hook_consume, (void**)&old_consume);
    std::thread(DataWorkerThread).detach();
    while (true) {
        uintptr_t base = GetModuleBase(GAME_MODULE);
        if (base) {
            g_GameBase = base;
            g_HookAddr_InitActor = base + 0x73507bc;
            int ret = DobbyHook((void*)g_HookAddr_InitActor, (void*)hook_InitActorParams, (void**)&old_InitActorParams);
            g_DobbyStatus.store(ret);
            break;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    return nullptr;
}

__attribute__((constructor)) void init_plugin() {
    pthread_t t; pthread_create(&t, nullptr, DelayedHookThread, nullptr);
}
