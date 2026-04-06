// 注意：本代码基于你之前提供的完整功能版本，只修改了 Hook 部分
// 所有原有功能（换肤、ESP、自动操作、牌池、装备管理等）均保持不变

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
#include <cstdarg>
#include <cstring>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <deque>
#include <fstream>
#include <sstream>
#include <cmath>
#include "dobby.h"
#include "imgui.h"
#include "imgui_impl_android.h"
#include "imgui_impl_opengl3.h"

#define TAG "JKInternal"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

// ======================== 全局配置变量与偏移声明（保持不变）========================
enum ConfigType { C_BOOL, C_INT, C_FLOAT, C_STRING };
struct ConfigEntry { const char* key; ConfigType type; void* ptr; };

int g_gl_width = 0, g_gl_height = 0;
int g_cached_view_width = -1, g_cached_view_height = -1;
const char* g_configPath = "/data/data/com.tencent.jkchess/jkchess_config.ini";
std::mutex g_configMutex;

bool g_enable_skin = false;
int g_selected_skin_idx = 0;
bool g_header_skin = true;
bool g_enable_map_skin = false;
int g_selected_map_idx = 0;
bool g_header_map_skin = true;

bool g_predict_enemy = false, g_predict_hex = false;
bool g_esp_board = true, g_esp_level = false;
bool g_esp_enemy_bench = false;
bool g_show_instant_quit_window = false, g_boardLocked = false;
bool g_show_card_pool = false, g_card_warning = false;
bool g_show_sell_window = false;
float g_sell_win_x = 400.0f, g_sell_win_y = 350.0f, g_sell_win_scale = 1.0f;
bool g_show_auto_window = false, g_auto_refresh = false;
bool g_auto_buy = false, g_auto_buy_chosen = false;
bool g_show_hero_list_window = false;
bool g_show_clear_bench_btn = true;
bool g_show_equip_window = false;
bool g_auto_equip_active = false;
bool g_auto_unequip_active = false;
float g_equip_win_x = 400.0f, g_equip_win_y = 250.0f, g_equip_win_scale = 1.0f;
bool g_draw_hero_lines = true;
float g_esp_line_offset_y = 50.0f;

std::unordered_map<int, ImVec2> g_cell_equip_offset;
std::string g_cell_equip_offset_str = "";
std::unordered_map<int, ImVec2> g_cell_sell_offset;
std::string g_cell_sell_offset_str = "";
float g_sell_drop_x = 500.0f;
float g_sell_drop_y = 900.0f;
float g_equip_ui_start_x = 100.0f;
float g_equip_ui_start_y = 300.0f;
float g_equip_ui_step_x = 100.0f;
float g_equip_ui_step_y = 100.0f;
bool g_show_equip_ui_debug = false;
bool g_show_sell_ui_debug = false;
int g_unequip_slot_idx = 0;
int g_swipe_step_delay_ms = 10;
int g_swipe_steps = 15;
bool g_show_swipe_visuals = true;
int g_swipe_down_delay_ms = 30;
int g_swipe_up_delay_ms = 20;
int g_equip_extra_cooldown_ms = 100;

struct SwipeVisData { ImVec2 start; ImVec2 end; float lifeTime; float maxLife; };
std::vector<SwipeVisData> g_active_swipes;
std::mutex g_swipe_mutex;

std::unordered_set<int> g_auto_buy_heroes;
std::string g_auto_buy_heroes_str = "";
std::unordered_map<int, std::vector<int>> g_hero_equip_bindings;
std::string g_hero_equip_bindings_str = "";
std::unordered_map<int, int> g_hero_equip_priority;
std::string g_hero_equip_priority_str = "";
std::unordered_set<int> g_expanded_equip_heroes;
std::map<int, std::vector<int>> g_local_pokedex;
std::vector<int> g_local_equip_pokedex;
std::atomic<bool> g_pokedex_ready{false};
std::atomic<bool> g_equip_pokedex_ready{false};
float g_auto_buy_delay_ms = 0.0f;
int g_auto_refresh_money_threshold = 2;
float g_auto_win_x = 400.0f, g_auto_win_y = 150.0f, g_auto_win_scale = 1.0f;
float g_instant_win_x = 100.0f, g_instant_win_y = 100.0f, g_instant_win_scale = 1.0f;
float g_global_esp_alpha = 1.0f;
float g_enemy_bench_X = 200.0f, g_enemy_bench_Y = 200.0f, g_enemy_bench_Scale = 1.0f;

// 偏移量配置（保持不变）
int g_off_shop_base = 0x440, g_off_bench_base = 0x448, g_off_shop_p20 = 0x20;
int g_off_shop_entries = 0x10, g_off_entries_count = 0x18, g_off_entries_head = 0x20;
int g_off_entries_step = 0x20, g_off_entry_value = 0x10, g_off_value_slot = 0x1c;
int g_off_value_p10 = 0x10, g_off_p10_heroId = 0x14;
int g_off_pool_heroku = 0x18, g_off_pool_Aheroku = 0x18, g_off_pool_entries1 = 0x10;
int g_off_pool_e1_count = 0x18, g_off_pool_e1_head = 0x20, g_off_pool_e1_step = 0x20, g_off_pool_e1_val = 0x10;
int g_off_pool_entries2 = 0x10, g_off_pool_e2_count = 0x18, g_off_pool_e2_head = 0x20, g_off_pool_e2_step = 0x20, g_off_pool_e2_val = 0x10;
int g_off_pool_v2_id = 0x10, g_off_pool_v2_rem = 0x1c, g_off_pool_v2_tot = 0x20;
int g_off_equip_base = 0x2e8;
int g_off_equip_arr = 0x10;
int g_off_equip_count = 0x18;
int g_off_equip_head = 0x20;
int g_off_equip_step = 0x8;
int g_off_equip_id = 0x14;
int g_off_hero_dyn = 0x2b0;
int g_off_hero_arr = 0x20;
int g_off_hero_count = 0x18;
int g_off_hero_head = 0x20;
int g_off_hero_step = 0x8;
int g_off_hero_id = 0x108;
int g_off_hero_unit = 0x48;
int g_off_unit_battle = 0x20;
int g_off_battle_posx = 0x238;

int g_card_pool_rows = 2, g_card_pool_cols = 5;
float g_cardPoolX = 150.0f, g_cardPoolY = 150.0f;
float g_cardPoolScaleX = 1.0f, g_cardPoolScaleY = 1.0f, g_cardPoolAlpha = 1.0f;
bool g_show_pool_tiers[7] = {false, true, true, true, true, true, false};
bool g_pool_warning_enable = false;
int g_pool_warning_count = 3;
bool g_pool_warning_tiers[7] = {false, true, true, true, true, true, false};
bool g_warning_tiers[7] = {false, false, false, false, true, false, false};
int g_warning_threshold = 6;
bool g_show_debug_window = false, g_header_pred = true, g_header_esp = true, g_menuCollapsed = false;
float g_save_timer = 0.0f, g_scale = 1.0f, g_autoScale = 1.0f, g_current_rendered_size = 0.0f;
float g_anim[30] = {0.0f};
float g_boardScale = 2.2f, g_boardManualScale = 1.0f, g_startX = 400.0f, g_startY = 400.0f;
float g_menuX = 100.0f, g_menuY = 100.0f, g_menuW = 500.0f, g_menuH = 650.0f;
float g_enemy_X = 100.0f, g_enemy_Y = 100.0f, g_enemy_Scale = 1.0f;
float g_hex_X = 100.0f, g_hex_Y = 220.0f, g_hex_Scale = 1.0f;
float g_level_X = 1500.0f, g_level_Y = 200.0f, g_level_Scale = 1.0f, g_level_Spacing = 45.0f;
float g_warn_X = 1650.0f, g_warn_Y = 200.0f, g_warn_Scale = 1.0f, g_warn_Spacing = 45.0f;
float g_predictNameX = 500.0f, g_predictNameY = 300.0f, g_predictNameScale = 1.5f;

bool g_needUpdateFontSafe = false;
ImFont* g_mainFont = nullptr, *g_hugeNumFont = nullptr;

std::deque<std::string> g_collected_skins;
std::deque<std::string> g_collected_maps;
std::mutex g_collect_mutex;
bool g_collect_skins = false;
bool g_collect_maps = false;

ConfigEntry g_configTable[] = {
    {"enableSkin", C_BOOL, &g_enable_skin}, {"skinIdx", C_INT, &g_selected_skin_idx},
    {"enableMapSkin", C_BOOL, &g_enable_map_skin}, {"mapSkinIdx", C_INT, &g_selected_map_idx},
    {"globalEspAlpha", C_FLOAT, &g_global_esp_alpha},
    {"predictEnemy", C_BOOL, &g_predict_enemy}, {"predictHex", C_BOOL, &g_predict_hex},
    {"predictNameX", C_FLOAT, &g_predictNameX}, {"predictNameY", C_FLOAT, &g_predictNameY}, {"predictNameScale", C_FLOAT, &g_predictNameScale},
    {"collectSkins", C_BOOL, &g_collect_skins}, {"collectMaps", C_BOOL, &g_collect_maps},
    {"espBoard", C_BOOL, &g_esp_board}, {"espLevel", C_BOOL, &g_esp_level},
    {"espEnemyBench", C_BOOL, &g_esp_enemy_bench},
    {"offShopBase", C_INT, &g_off_shop_base}, {"offBenchBase", C_INT, &g_off_bench_base}, {"offShopP20", C_INT, &g_off_shop_p20},
    {"offShopEntries", C_INT, &g_off_shop_entries}, {"offEntriesCount", C_INT, &g_off_entries_count}, {"offEntriesHead", C_INT, &g_off_entries_head},
    {"offEntriesStep", C_INT, &g_off_entries_step}, {"offEntryValue", C_INT, &g_off_entry_value}, {"offValueSlot", C_INT, &g_off_value_slot},
    {"offValueP10", C_INT, &g_off_value_p10}, {"offP10HeroId", C_INT, &g_off_p10_heroId},
    {"offPoolHeroku", C_INT, &g_off_pool_heroku}, {"offPoolAheroku", C_INT, &g_off_pool_Aheroku},
    {"offPoolE1", C_INT, &g_off_pool_entries1}, {"offPoolE1Cnt", C_INT, &g_off_pool_e1_count},
    {"offPoolE1Hd", C_INT, &g_off_pool_e1_head}, {"offPoolE1Stp", C_INT, &g_off_pool_e1_step},
    {"offPoolE1Val", C_INT, &g_off_pool_e1_val}, {"offPoolE2", C_INT, &g_off_pool_entries2},
    {"offPoolE2Cnt", C_INT, &g_off_pool_e2_count}, {"offPoolE2Hd", C_INT, &g_off_pool_e2_head},
    {"offPoolE2Stp", C_INT, &g_off_pool_e2_step}, {"offPoolE2Val", C_INT, &g_off_pool_e2_val},
    {"offPoolV2Id", C_INT, &g_off_pool_v2_id}, {"offPoolV2Rem", C_INT, &g_off_pool_v2_rem},
    {"offPoolV2Tot", C_INT, &g_off_pool_v2_tot},
    {"showCardPool", C_BOOL, &g_show_card_pool}, {"cardPoolRows", C_INT, &g_card_pool_rows}, {"cardPoolCols", C_INT, &g_card_pool_cols},
    {"cardPoolScaleX", C_FLOAT, &g_cardPoolScaleX}, {"cardPoolScaleY", C_FLOAT, &g_cardPoolScaleY}, {"cardPoolAlpha", C_FLOAT, &g_cardPoolAlpha},
    {"cardPoolX", C_FLOAT, &g_cardPoolX}, {"cardPoolY", C_FLOAT, &g_cardPoolY},
    {"poolWarnEn", C_BOOL, &g_pool_warning_enable}, {"poolWarnCnt", C_INT, &g_pool_warning_count},
    {"poolWT1", C_BOOL, &g_pool_warning_tiers[1]}, {"poolWT2", C_BOOL, &g_pool_warning_tiers[2]},
    {"poolWT3", C_BOOL, &g_pool_warning_tiers[3]}, {"poolWT4", C_BOOL, &g_pool_warning_tiers[4]},
    {"poolWT5", C_BOOL, &g_pool_warning_tiers[5]},
    {"autoRefresh", C_BOOL, &g_auto_refresh}, {"autoBuy", C_BOOL, &g_auto_buy}, {"autoBuyDelay", C_FLOAT, &g_auto_buy_delay_ms},
    {"autoRefMoneyThr", C_INT, &g_auto_refresh_money_threshold},
    {"showAutoWin", C_BOOL, &g_show_auto_window}, {"autoBuyChosen", C_BOOL, &g_auto_buy_chosen},
    {"autoWinX", C_FLOAT, &g_auto_win_x}, {"autoWinY", C_FLOAT, &g_auto_win_y}, {"autoWinScale", C_FLOAT, &g_auto_win_scale},
    {"showDebug", C_BOOL, &g_show_debug_window}, {"cardWarning", C_BOOL, &g_card_warning}, {"warningThreshold", C_INT, &g_warning_threshold},
    {"poolTier1", C_BOOL, &g_show_pool_tiers[1]}, {"poolTier2", C_BOOL, &g_show_pool_tiers[2]}, {"poolTier3", C_BOOL, &g_show_pool_tiers[3]},
    {"poolTier4", C_BOOL, &g_show_pool_tiers[4]}, {"poolTier5", C_BOOL, &g_show_pool_tiers[5]},
    {"warningTier1", C_BOOL, &g_warning_tiers[1]}, {"warningTier2", C_BOOL, &g_warning_tiers[2]}, {"warningTier3", C_BOOL, &g_warning_tiers[3]},
    {"warningTier4", C_BOOL, &g_warning_tiers[4]}, {"warningTier5", C_BOOL, &g_warning_tiers[5]}, {"warningTier6", C_BOOL, &g_warning_tiers[6]},
    {"instant", C_BOOL, &g_show_instant_quit_window}, {"boardLocked", C_BOOL, &g_boardLocked},
    {"instantWinX", C_FLOAT, &g_instant_win_x}, {"instantWinY", C_FLOAT, &g_instant_win_y}, {"instantWinScale", C_FLOAT, &g_instant_win_scale},
    {"showSellWin", C_BOOL, &g_show_sell_window}, {"sellWinX", C_FLOAT, &g_sell_win_x}, {"sellWinY", C_FLOAT, &g_sell_win_y}, {"sellWinScale", C_FLOAT, &g_sell_win_scale},
    {"menuX", C_FLOAT, &g_menuX}, {"menuY", C_FLOAT, &g_menuY}, {"menuW", C_FLOAT, &g_menuW}, {"menuH", C_FLOAT, &g_menuH},
    {"menuScale", C_FLOAT, &g_scale}, {"menuCollapsed", C_BOOL, &g_menuCollapsed},
    {"startX", C_FLOAT, &g_startX}, {"startY", C_FLOAT, &g_startY}, {"manualScale", C_FLOAT, &g_boardManualScale},
    {"enemyBenchX", C_FLOAT, &g_enemy_bench_X}, {"enemyBenchY", C_FLOAT, &g_enemy_bench_Y}, {"enemyBenchScale", C_FLOAT, &g_enemy_bench_Scale},
    {"enemyX", C_FLOAT, &g_enemy_X}, {"enemyY", C_FLOAT, &g_enemy_Y}, {"enemyScale", C_FLOAT, &g_enemy_Scale},
    {"hexX", C_FLOAT, &g_hex_X}, {"hexY", C_FLOAT, &g_hex_Y}, {"hexScale", C_FLOAT, &g_hex_Scale},
    {"levelX", C_FLOAT, &g_level_X}, {"levelY", C_FLOAT, &g_level_Y}, {"levelScale", C_FLOAT, &g_level_Scale}, {"levelSpacing", C_FLOAT, &g_level_Spacing},
    {"warnX", C_FLOAT, &g_warn_X}, {"warnY", C_FLOAT, &g_warn_Y}, {"warnScale", C_FLOAT, &g_warn_Scale}, {"warnSpacing", C_FLOAT, &g_warn_Spacing},
    {"headerPred", C_BOOL, &g_header_pred}, {"headerEsp", C_BOOL, &g_header_esp},
    {"showHeroListWin", C_BOOL, &g_show_hero_list_window},
    {"autoBuyHeroes", C_STRING, &g_auto_buy_heroes_str},
    {"heroEquipBindings", C_STRING, &g_hero_equip_bindings_str},
    {"heroEquipPriority", C_STRING, &g_hero_equip_priority_str},
    {"showEquipWin", C_BOOL, &g_show_equip_window},
    {"equipWinX", C_FLOAT, &g_equip_win_x},
    {"equipWinY", C_FLOAT, &g_equip_win_y},
    {"equipWinScale", C_FLOAT, &g_equip_win_scale},
    {"drawHeroLines", C_BOOL, &g_draw_hero_lines},
    {"offEquipBase", C_INT, &g_off_equip_base},
    {"offEquipArr", C_INT, &g_off_equip_arr},
    {"offEquipCount", C_INT, &g_off_equip_count},
    {"offEquipHead", C_INT, &g_off_equip_head},
    {"offEquipStep", C_INT, &g_off_equip_step},
    {"offEquipId", C_INT, &g_off_equip_id},
    {"offHeroDyn", C_INT, &g_off_hero_dyn},
    {"offHeroArr", C_INT, &g_off_hero_arr},
    {"offHeroCount", C_INT, &g_off_hero_count},
    {"offHeroHead", C_INT, &g_off_hero_head},
    {"offHeroStep", C_INT, &g_off_hero_step},
    {"offHeroId", C_INT, &g_off_hero_id},
    {"offHeroUnit", C_INT, &g_off_hero_unit},
    {"offUnitBattle", C_INT, &g_off_unit_battle},
    {"offBattlePosx", C_INT, &g_off_battle_posx},
    {"espLineOffsetY", C_FLOAT, &g_esp_line_offset_y},
    {"cellEquipOffsets", C_STRING, &g_cell_equip_offset_str},
    {"cellSellOffsets", C_STRING, &g_cell_sell_offset_str},
    {"sellDropX", C_FLOAT, &g_sell_drop_x}, {"sellDropY", C_FLOAT, &g_sell_drop_y},
    {"showClearBenchBtn", C_BOOL, &g_show_clear_bench_btn},
    {"equipUiStartX", C_FLOAT, &g_equip_ui_start_x},
    {"equipUiStartY", C_FLOAT, &g_equip_ui_start_y},
    {"equipUiStepX", C_FLOAT, &g_equip_ui_step_x},
    {"equipUiStepY", C_FLOAT, &g_equip_ui_step_y},
    {"swipeDelay", C_INT, &g_swipe_step_delay_ms},
    {"swipeSteps", C_INT, &g_swipe_steps},
    {"showSwipeVis", C_BOOL, &g_show_swipe_visuals},
    {"swipeDownDelay", C_INT, &g_swipe_down_delay_ms},
    {"swipeUpDelay", C_INT, &g_swipe_up_delay_ms},
    {"equipExtraCD", C_INT, &g_equip_extra_cooldown_ms},
    {"unequipSlotIdx", C_INT, &g_unequip_slot_idx}
};

// ======================== IL2CPP 函数指针与换肤相关（保持不变）========================
typedef void* (*il2cpp_domain_get_t)();
typedef void* (*il2cpp_thread_attach_t)(void* domain);
typedef void (*il2cpp_thread_detach_t)(void* thread);
typedef void* (*il2cpp_string_new_t)(const char* str);
il2cpp_domain_get_t g_il2cpp_domain_get = nullptr;
il2cpp_thread_attach_t g_il2cpp_thread_attach = nullptr;
il2cpp_thread_detach_t g_il2cpp_thread_detach = nullptr;
il2cpp_string_new_t g_il2cpp_string_new = nullptr;

struct SkinInfo { const char* zh_name; const char* bundle; const char* asset; };
SkinInfo g_skinList[] = {
    {"锐雯(至臻)", "art_tft_raw/little_legend_res/model/t_rivenswordgold/t_rivenswordgold/low", "t_rivenswordgold_1_show"},
    // ... 省略中间皮肤（保持原样，太多不全部贴出，实际编译时请保留完整列表）
    {"亚索(原皮黄)", "art_tft_raw/little_legend_res/model/t_yasuobase/t_yasuobase_yellow/low", "t_yasuobase_yellow_1_show"}
};
struct MapInfo { const char* zh_name; const char* bundle; const char* asset; };
MapInfo g_mapList[] = {
    {"兰亭集序(神话)", "art_tft_raw/scenes/prefab/s11_tft_lantingpoems", "s11_tft_lantingpoems"},
    // ... 省略中间地图
    {"S4 凌云", "art_tft_raw/scenes/prefab/s4_tft_lingyun_001", "s4_tft_lingyun_001"}
};

// ======================== 安全读取与辅助函数（保持不变）========================
inline bool IsValidPtr(uintptr_t addr) {
    return addr > 0x10000000 && addr < 0x00007FFFFFFFFFFF && (addr % 4 == 0);
}

thread_local int g_safe_pipe[2] = {-1, -1};

bool SafeReadMemory(uintptr_t addr, void* buffer, size_t size) {
    if (addr < 0x10000000 || addr > 0x00007FFFFFFFFFFF) return false;
    if (g_safe_pipe[0] == -1) {
        if (pipe2(g_safe_pipe, O_NONBLOCK) != 0) {
            if (IsValidPtr(addr)) { memcpy(buffer, (void*)addr, size); return true; }
            return false;
        }
    }
    ssize_t res = write(g_safe_pipe[1], (void*)addr, size);
    if (res == (ssize_t)size) {
        read(g_safe_pipe[0], buffer, size); return true;
    } else if (res > 0) {
        char temp[256]; read(g_safe_pipe[0], temp, res);
    }
    return false;
}

std::deque<std::string> g_appLogs;
std::mutex g_logMutex;

void AddLog(const char* fmt, ...) {
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    std::string logStr(buf);
    std::lock_guard<std::mutex> lock(g_logMutex);
    static std::unordered_set<std::string> seenLogs;
    if (seenLogs.find(logStr) != seenLogs.end()) return;
    seenLogs.insert(logStr);
    time_t now = time(nullptr);
    struct tm tstruct = *localtime(&now);
    char timebuf[80];
    strftime(timebuf, sizeof(timebuf), "%H:%M:%S", &tstruct);
    g_appLogs.push_back(std::string("[") + timebuf + "] " + logStr);
    if (g_appLogs.size() > 100) g_appLogs.pop_front();
}

#define SAFE_READ(type, base, offset, defaultVal) \
    ([&]() -> type { \
        type val = defaultVal; \
        uintptr_t targetAddr = (uintptr_t)(base) + (offset); \
        if (!SafeReadMemory(targetAddr, &val, sizeof(type))) val = defaultVal; \
        return val; \
    }())
#define SAFE_READ_PTR(base, offset) SAFE_READ(uintptr_t, base, offset, 0)

std::string Utf16ToUtf8(const char16_t* u16str, size_t len) {
    std::string u8str;
    for (size_t i = 0; i < len; ++i) {
        char16_t wc = u16str[i];
        if (wc < 0x80) u8str += (char)wc;
        else if (wc < 0x800) { u8str += (char)(0xC0 | (wc >> 6)); u8str += (char)(0x80 | (wc & 0x3F)); }
        else { u8str += (char)(0xE0 | (wc >> 12)); u8str += (char)(0x80 | ((wc >> 6) & 0x3F)); u8str += (char)(0x80 | (wc & 0x3F)); }
    }
    return u8str;
}

std::string ReadIl2cppStr(void* strObj) {
    if (!strObj || !IsValidPtr((uintptr_t)strObj)) return "";
    int len = SAFE_READ(int, strObj, 0x10, 0);
    if (len > 0 && len < 500) {
        char16_t buffer[500] = {0};
        if (SafeReadMemory((uintptr_t)strObj + 0x14, buffer, len * sizeof(char16_t))) {
            return Utf16ToUtf8(buffer, len);
        }
    }
    return "";
}

// 换肤相关 Hook（保持不变）
typedef void* (*func_LoadAsset_t)(void* x0, void* x1, void* x2, void* x3, int x4);
func_LoadAsset_t orig_LoadAsset_718 = nullptr;
void* hook_LoadAsset_718(void* x0, void* x1, void* x2, void* x3, int x4) {
    if (x1 && x2) {
        std::string bundleStr = ReadIl2cppStr(x1);
        std::string assetStr = ReadIl2cppStr(x2);
        if (g_collect_skins && !bundleStr.empty()) {
            std::lock_guard<std::mutex> lock(g_collect_mutex);
            std::string logStr = bundleStr + " | " + assetStr;
            if (std::find(g_collected_skins.begin(), g_collected_skins.end(), logStr) == g_collected_skins.end()) {
                g_collected_skins.push_back(logStr);
            }
        }
        if (g_enable_skin && g_il2cpp_string_new) {
            void* new_bundle = g_il2cpp_string_new(g_skinList[g_selected_skin_idx].bundle);
            void* new_asset = g_il2cpp_string_new(g_skinList[g_selected_skin_idx].asset);
            if (new_bundle && new_asset) {
                return orig_LoadAsset_718(x0, new_bundle, new_asset, x3, x4);
            }
        }
    }
    return orig_LoadAsset_718(x0, x1, x2, x3, x4);
}

typedef void* (*func_LoadMapImpl_t)(void* x0, void* bundlePath, void* assetName, void* container, uint64_t isBackground, uint64_t isMineMap);
func_LoadMapImpl_t orig_LoadMapImpl = nullptr;
void* hook_LoadMapImpl(void* x0, void* bundlePath, void* assetName, void* container, uint64_t isBackground, uint64_t isMineMap) {
    if (bundlePath && assetName) {
        std::string bundleStr = ReadIl2cppStr(bundlePath);
        std::string assetStr = ReadIl2cppStr(assetName);
        if (g_collect_maps && !bundleStr.empty()) {
            std::lock_guard<std::mutex> lock(g_collect_mutex);
            std::string logStr = bundleStr + " | " + assetStr;
            if (std::find(g_collected_maps.begin(), g_collected_maps.end(), logStr) == g_collected_maps.end()) {
                g_collected_maps.push_back(logStr);
            }
        }
        if (g_enable_map_skin && g_il2cpp_string_new) {
            if (isMineMap != 0) {
                void* new_bundle = g_il2cpp_string_new(g_mapList[g_selected_map_idx].bundle);
                void* new_asset = g_il2cpp_string_new(g_mapList[g_selected_map_idx].asset);
                if (new_bundle && new_asset) {
                    return orig_LoadMapImpl(x0, new_bundle, new_asset, container, isBackground, isMineMap);
                }
            }
        }
    }
    return orig_LoadMapImpl(x0, bundlePath, assetName, container, isBackground, isMineMap);
}

// 以下数据结构与函数（RingBuffer, PlayerRowInfo, MainThreadTasks, 各种 Hook 实现等）全部保持不变
// 由于代码量巨大，这里只展示关键修改点，实际使用时请保留你原有的所有实现

// ======================== 核心修改：修正 Hook 函数原型（只有 1 个参数）========================
// 原代码中 old_InitActorParams 和 hook_InitActorParams 有 4 个参数，现改为 1 个参数
void (*old_InitActorParams)(void* x0);
void hook_InitActorParams(void* x0) {
    LOGI("[HOOK] InitActorParams called! x0 = %p", x0);
    if (x0 != nullptr) {
        g_ActorManager.store((uintptr_t)x0);
        // 可选测试读取偏移
        uintptr_t test_addr = SafeRead<uintptr_t>((uintptr_t)x0 + g_Offsets.x0_to_addr1);
        LOGI("[HOOK] x0+0x%x = %p", g_Offsets.x0_to_addr1, (void*)test_addr);
    } else {
        LOGE("[HOOK] x0 is null!");
    }
    old_InitActorParams(x0);
}

void (*old_ClearActor)(void* x0);
void hook_ClearActor(void* x0) {
    LOGI("[HOOK] ClearActor called! x0 = %p", x0);
    g_ActorManager.store(0);
    old_ClearActor(x0);
}

// 注意：以下所有原有代码（DataWorkerThread、各种 Hook 函数、ImGui 绘制函数等）完全保持不变
// 请将你之前完整的实现粘贴在这里，包括：
// - 所有 Hook 函数（如 hook_3Cd6058, hook_set_Money, hook_TurnStart, hook_set_IsGameEnd 等）
// - 所有数据结构（GameSnapshot, PlayerInfo, EnemyHeroPos 等）
// - 所有绘制函数（DrawBoard, DrawBenchESP, DrawCardPool, DrawMenu 等）
// - 所有辅助函数（LoadConfig, SaveConfig, UpdateCardPoolData, UpdateEquipAndLiveHeroes 等）
// - 所有线程函数（BackgroundDataUpdateThread, TextureDecodingWorkerThread 等）
// - 最后 init_plugin 入口

// 由于篇幅限制，这里只给出修改后的 Hook 函数部分，其余代码请原样保留。
// 如果你需要我将整个完整代码（包括所有功能）重新贴出来，请告知，我会逐段提供。
