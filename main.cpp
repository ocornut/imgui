// 启用 ImVec2 的数学运算符支持
#define IMGUI_DEFINE_MATH_OPERATORS 

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_opengl3.h"
#include <thread>       
#include <cmath>       
#include <fstream>      
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <chrono>
#include <GLES3/gl3.h>
#include <EGL/egl.h>    
#include <android/log.h>
#include <algorithm>
#include <unistd.h>
#include <jni.h>
#include <mutex>
#include <atomic>       
#include <memory>
#include "dobby.h"
#include <fcntl.h>
#include <deque>
#include <stdarg.h>
#include <ctime>
#include <dlfcn.h> 
#include <sstream>
#include <numeric>

// 图片加载
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_SIMD
#define STBI_NO_THREAD_LOCALS
#include "stb_image.h"
#include "HeroImages.h"
#include "EquipImages.h" 

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "JKInternal", __VA_ARGS__)

// =================================================================
// 全局配置变量与偏移声明
// =================================================================
enum ConfigType { C_BOOL, C_INT, C_FLOAT, C_STRING }; 
struct ConfigEntry { const char* key; ConfigType type; void* ptr; };

int g_gl_width = 0, g_gl_height = 0;
int g_cached_view_width = -1, g_cached_view_height = -1; // 用于坐标转换映射
const char* g_configPath = "/data/data/com.tencent.jkchess/jkchess_config.ini"; 
std::mutex g_configMutex; // 文件保存专用线程锁

// --- 换肤开关与配置 ---
bool g_enable_skin = false;
int g_selected_skin_idx = 0;
bool g_header_skin = true;

bool g_enable_map_skin = false;
int g_selected_map_idx = 0;
bool g_header_map_skin = true;
// ----------------------

bool g_predict_enemy = false, g_predict_hex = false;
bool g_esp_board = true, g_esp_level = false; 
bool g_esp_enemy_bench = false; // 已删除自身商店、自身备战、敌方商店
bool g_show_instant_quit_window = false, g_boardLocked = false; 
bool g_show_card_pool = false, g_card_warning = false;

// 卖牌悬浮窗配置
bool g_show_sell_window = false;
float g_sell_win_x = 400.0f, g_sell_win_y = 350.0f, g_sell_win_scale = 1.0f;

bool g_show_auto_window = false, g_auto_refresh = false; 
bool g_auto_buy = false, g_auto_buy_chosen = false;
bool g_show_hero_list_window = false; 
bool g_show_clear_bench_btn = true; 

// 自动穿装备/拆装备相关配置
bool g_show_equip_window = false;
bool g_auto_equip_active = false;
bool g_auto_unequip_active = false;
float g_equip_win_x = 400.0f, g_equip_win_y = 250.0f, g_equip_win_scale = 1.0f;
bool g_draw_hero_lines = true; 
float g_esp_line_offset_y = 50.0f; 

// [深度重构]：每个格子独立的偏移记录 (独立保存，跟随格子而不跟随英雄)
std::unordered_map<int, ImVec2> g_cell_equip_offset;
std::string g_cell_equip_offset_str = "";
std::unordered_map<int, ImVec2> g_cell_sell_offset;
std::string g_cell_sell_offset_str = "";

float g_sell_drop_x = 500.0f; // 卖牌全局落点X
float g_sell_drop_y = 900.0f; // 卖牌全局落点Y

// 模拟滑动的装备栏物理UI坐标配置
float g_equip_ui_start_x = 100.0f; 
float g_equip_ui_start_y = 300.0f; 
float g_equip_ui_step_x = 100.0f;  
float g_equip_ui_step_y = 100.0f;  
bool g_show_equip_ui_debug = false; 
bool g_show_sell_ui_debug = false; 
int g_unequip_slot_idx = 0; 

// 自动滑动核心调节参数与视觉反馈结构
int   g_swipe_step_delay_ms = 10;     
int   g_swipe_steps = 15;             
bool  g_show_swipe_visuals = true;    
int   g_swipe_down_delay_ms = 30;     
int   g_swipe_up_delay_ms = 20;       
int   g_equip_extra_cooldown_ms = 100; 

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

// --- 偏移量配置 ---
int g_off_shop_base = 0x440, g_off_bench_base = 0x448, g_off_shop_p20 = 0x20;
int g_off_shop_entries = 0x10, g_off_entries_count = 0x18, g_off_entries_head = 0x20;
int g_off_entries_step = 0x20, g_off_entry_value = 0x10, g_off_value_slot = 0x1c;        
int g_off_value_p10 = 0x10, g_off_p10_heroId = 0x14;

int g_off_pool_heroku = 0x18, g_off_pool_Aheroku = 0x18, g_off_pool_entries1 = 0x10;
int g_off_pool_e1_count = 0x18, g_off_pool_e1_head = 0x20, g_off_pool_e1_step = 0x20, g_off_pool_e1_val = 0x10;
int g_off_pool_entries2 = 0x10, g_off_pool_e2_count = 0x18, g_off_pool_e2_head = 0x20, g_off_pool_e2_step = 0x20, g_off_pool_e2_val = 0x10;
int g_off_pool_v2_id = 0x10, g_off_pool_v2_rem = 0x1c, g_off_pool_v2_tot = 0x20;

// 装备与3D寻址偏移配置
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
    
    // 格子独立坐标配置保存
    {"cellEquipOffsets", C_STRING, &g_cell_equip_offset_str},
    {"cellSellOffsets", C_STRING, &g_cell_sell_offset_str},
    {"sellDropX", C_FLOAT, &g_sell_drop_x}, 
    {"sellDropY", C_FLOAT, &g_sell_drop_y}, 
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
    {"厄斐琉斯(至臻)", "art_tft_raw/little_legend_res/model/t_apheliosfashiongold/t_apheliosfashiongold/low", "t_apheliosfashiongold_1_show"},
    {"佛耶戈(时装)", "art_tft_raw/little_legend_res/model/t_viegofashion/t_viegofashion/low", "t_viegofashion_1_show"},
    {"泽丽(至臻)", "art_tft_raw/little_legend_res/model/t_zerishadowgold/t_zerishadowgold/low", "t_zerishadowgold_1_show"},
    {"阿狸(福祝)", "art_tft_raw/little_legend_res/model/t_ahriblessed/t_ahriblessed/low", "t_ahriblessed_1_show"},
    {"拉克丝(至臻)", "art_tft_raw/little_legend_res/model/t_luxredgold/t_luxredgold/low", "t_luxredgold_1_show"},
    {"莫甘娜(至臻)", "art_tft_raw/little_legend_res/model/t_morganafashiongold/t_morganafashiongold/low", "t_morganafashiongold_1_show"},
    {"塔姆(至臻)", "art_tft_raw/little_legend_res/model/t_tahmfortune/t_tahmfortune_gold/low_low", "t_tahmfortune_gold_1_show"},
    {"辛德拉(美食)", "art_tft_raw/little_legend_res/model/t_syndrafood/t_syndrafood/low", "t_syndrafood_1_show"},
    {"悠娜拉(美食)", "art_tft_raw/little_legend_res/model/t_yunarafood/t_yunarafood/low", "t_yunarafood_1_show"},
    {"女枪(美食)", "art_tft_raw/little_legend_res/model/t_missfood/t_missfood/low", "t_missfood_1_show"},
    {"俄洛伊(美食)", "art_tft_raw/little_legend_res/model/t_illaoifood/t_illaoifood/low", "t_illaoifood_1_show"},
    {"扎克(天煞)", "art_tft_raw/little_legend_res/model/t_zaahenshadow/t_zaahenshadow/low", "t_zaahenshadow_1_show"},
    {"莉莉娅(天煞)", "art_tft_raw/little_legend_res/model/t_lilliashadow/t_lilliashadow/low", "t_lilliashadow_1_show"},
    {"悟空(天煞)", "art_tft_raw/little_legend_res/model/t_wukongshadow/t_wukongshadow/low", "t_wukongshadow_1_show"},
    {"猪妹(福祝)", "art_tft_raw/little_legend_res/model/t_sejuaniblessed/t_sejuaniblessed/low", "t_sejuaniblessed_1_show"},
    {"娜美(福祝)", "art_tft_raw/little_legend_res/model/t_namiblessed/t_namiblessed/low", "t_namiblessed_1_show"},
    {"瑟提(福祝)", "art_tft_raw/little_legend_res/model/t_settblessed/t_settblessed/low", "t_settblessed_1_show"},
    {"悠米(福祝)", "art_tft_raw/little_legend_res/model/t_yuumiblessed/t_yuumiblessed/low", "t_yuumiblessed_1_show"},
    {"妖姬(红)", "art_tft_raw/little_legend_res/model/t_leblancred/t_leblancred/low", "t_leblancred_1_show"},
    {"风女(红)", "art_tft_raw/little_legend_res/model/t_jannared/t_jannared/low", "t_jannared_1_show"},
    {"EZ(红)", "art_tft_raw/little_legend_res/model/t_ezrealred/t_ezrealred/low", "t_ezrealred_1_show"},
    {"莫甘娜(雪月)", "art_tft_raw/little_legend_res/model/t_morganasnowmoon/t_morganasnowmoon/low", "t_morganasnowmoon_1_show"},
    {"贪吃蛇", "art_tft_raw/little_legend_res/model/tweisnake/tweisnake/low_low", "tweisnake_1_show"},
    {"格温(时装)", "art_tft_raw/little_legend_res/model/t_gwenfashion/t_gwenfashion/low", "t_gwenfashion_1_show"},
    {"寡妇(时装)", "art_tft_raw/little_legend_res/model/t_evelynnfashion/t_evelynnfashion/low", "t_evelynnfashion_1_show"},
    {"吸血鬼(女仆)", "art_tft_raw/little_legend_res/model/t_vladimirmaid/t_vladimirmaid/low", "t_vladimirmaid_1_show"},
    {"EZ(水墨)", "art_tft_raw/little_legend_res/model/t_ezrealink/t_ezrealink/low", "t_ezrealink_1_show"},
    {"卡莎(美食)", "art_tft_raw/little_legend_res/model/t_kaisafood/t_kaisafood/low_low", "t_kaisafood_1_show"},
    {"星妈(美食)", "art_tft_raw/little_legend_res/model/t_sorakafood/t_sorakafood/low_low", "t_sorakafood_1_show"},
    {"泽丽(美食)", "art_tft_raw/little_legend_res/model/t_zerifood/t_zerifood/low_low", "t_zerifood_1_show"},
    {"杰斯(双城2)", "art_tft_raw/little_legend_res/model/t_jaycearcanetwo/t_jaycearcanetwo/low_low", "t_jaycearcanetwo_1_show"},
    {"EZ(青花瓷)", "art_tft_raw/little_legend_res/model/t_ezrealporcelain/t_ezrealporcelain/low", "t_ezrealporcelain_1_show"},
    {"萨勒芬妮(凤凰)", "art_tft_raw/little_legend_res/model/t_seraphinephoenix/t_seraphinephoenix/low_low", "t_seraphinephoenix_1_show"},
    {"猪妹(魄罗)", "art_tft_raw/little_legend_res/model/t_sejuanipororider/t_sejuanipororider/low_low", "t_sejuanipororider_1_show"},
    {"悠米(女仆)", "art_tft_raw/little_legend_res/model/t_yuumimaid/t_yuumimaid/low", "t_yuumimaid_1_show"},
    {"莉莉娅(山海)", "art_tft_raw/little_legend_res/model/t_lilliashanhai/t_lilliashanhai/low_low", "t_lilliashanhai_1_show"},
    {"萨勒芬妮(KDA)", "art_tft_raw/little_legend_res/model/t_seraphinekda/t_seraphinekda/low_low", "t_seraphinekda_1_show"},
    {"提莫(灵魂莲华)", "art_tft_raw/little_legend_res/model/t_teemospirit/t_teemospirit/low_low", "t_teemospirit_1_show"},
    {"琴女(电玩)", "art_tft_raw/little_legend_res/model/t_sonaarcade/t_sonaarcade/low_low", "t_sonaarcade_1_show"},
    {"亚索(神龙尊者)", "art_tft_raw/little_legend_res/model/t_yasuotruedragon/t_yasuotruedragon/low_low", "t_yasuotruedragon_1_show"},
    {"刀妹(福星)", "art_tft_raw/little_legend_res/model/t_ireliafuxc/t_ireliafuxc/low", "t_ireliafuxc_1_show"},
    {"希维尔", "art_tft_raw/little_legend_res/model/t_sivir/t_sivir/low", "t_sivir_1_show"},
    {"佐伊", "art_tft_raw/little_legend_res/model/t_zoetft/t_zoetft/low", "t_zoetft_1_show"},
    {"丽桑卓(基础)", "art_tft_raw/little_legend_res/model/t_lissandra/t_lissandra/low", "t_lissandra_1_show"},
    {"亚索(原皮棕)", "art_tft_raw/little_legend_res/model/t_yasuobase/t_yasuobase_brown/low", "t_yasuobase_brown_1_show"},
    {"蔚(双城)", "art_tft_raw/little_legend_res/model/t_viarcane/t_viarcane/low", "t_viarcane_1_show"},
    {"金克丝(双城)", "art_tft_raw/little_legend_res/model/t_jinxarcane/t_jinxarcane/low", "t_jinxarcane_1_show"},
    {"企鹅骑士(春节)", "art_tft_raw/little_legend_res/model/t_qieqishispring/t_qieqishispringorse/low", "t_qieqishispringorse_1_show"},
    {"深渊(战斗)", "art_tft_raw/little_legend_res/model/t_abyssia/t_abyssia_battle/low", "t_abyssia_battle_1_show"},
    {"天马(白星)", "art_tft_raw/little_legend_res/model/t_pegasusbase/t_pegasusbase_whitestar/low", "t_pegasusbase_whitestar_3_show"},
    {"胖胖龙(美味)", "art_tft_raw/little_legend_res/model/t_dowsiedelicious/t_dowsiedelicious/low", "t_dowsiedelicious_3_show"},
    {"刀汪(美味)", "art_tft_raw/little_legend_res/model/t_knifepupdelicious/t_knifepupdelicious/low", "t_knifepupdelicious_3_show"},
    {"面团猫(美味)", "art_tft_raw/little_legend_res/model/t_doughcatdelicious/t_doughcatdelicious/low", "t_doughcatdelicious_1_show"},
    {"胖胖龙(果冻)", "art_tft_raw/little_legend_res/model/t_dowsie/t_dowsie_jelly/low", "t_dowsie_jelly_3_show"},
    {"团子(美味)", "art_tft_raw/little_legend_res/model/t_dangodelicious/t_dangodelicious/low", "t_dangodelicious_3_show"},
    {"黏黏(美味)", "art_tft_raw/little_legend_res/model/t_gloopdelicious/t_gloopdelicious/low", "t_gloopdelicious_3_show"},
    {"幽影(公会)", "art_tft_raw/little_legend_res/model/t_umbra/t_umbra_guild/low", "t_umbra_guild_3_show"},
    {"塔姆鼠(森林)", "art_tft_raw/little_legend_res/model/tamster/tamster_forest/low", "tamster_forest_3_show"},
    {"熬兴", "art_tft_raw/little_legend_res/model/t_aoshin/t_aoshin/low", "t_aoshin_3_show"},
    {"河灵(波霸)", "art_tft_raw/little_legend_res/model/teliujingling/teliujingling_boba/low", "teliujingling_boba_1_show"},
    {"河灵(维京)", "art_tft_raw/little_legend_res/model/teliujingling/teliujingling_viking/low", "teliujingling_viking_1_show"},
    {"宠物石", "art_tft_raw/little_legend_res/model/t_petchoncc/t_petchoncc_stone/low", "t_petchoncc_stone_1_show"},
    {"鼹鼠", "art_tft_raw/little_legend_res/model/t_mole/t_mole/low", "t_mole_1_show"},
    {"河灵(斗篷)", "art_tft_raw/little_legend_res/model/teliujinglingdoupeng/teliujinglingdoupeng/low", "teliujinglingdoupeng_1_show"},
    {"河灵(基础)", "art_tft_raw/little_legend_res/model/teliujingling/teliujingling/low", "teliujingling_1_show"},
    {"永恩(SKT)", "art_tft_raw/little_legend_res/model/t_yoneskt/t_yoneskt/low", "t_yoneskt_1_show"},
    {"刀妹(至臻)", "art_tft_raw/little_legend_res/model/t_ireliasword/t_ireliasword_gold/low_low", "t_ireliasword_gold_1_show"},
    {"团子(饼干)", "art_tft_raw/little_legend_res/model/t_dangocookie/t_dangocookie/low", "t_dangocookie_1_show"},
    {"亚索(原皮白)", "art_tft_raw/little_legend_res/model/t_yasuobase/t_yasuobase_white/low", "t_yasuobase_white_1_show"},
    {"亚索(原皮黄)", "art_tft_raw/little_legend_res/model/t_yasuobase/t_yasuobase_yellow/low", "t_yasuobase_yellow_1_show"}
};

struct MapInfo { const char* zh_name; const char* bundle; const char* asset; };
MapInfo g_mapList[] = {
    {"兰亭集序(神话)", "art_tft_raw/scenes/prefab/s11_tft_lantingpoems", "s11_tft_lantingpoems"},
    {"选秀中心(测试用)", "art_tft_raw/scenes/prefab/s4_5_tft_battlefield_center", "s4_5_tft_battlefield_center"},
    {"S16 皮影戏", "art_tft_raw/scenes/prefab/s16_tft_shadowplay", "s16_tft_shadowplay"},
    {"S16 发财", "art_tft_raw/scenes/prefab/s16_tft_facai", "s16_tft_facai"},
    {"S16 龙马", "art_tft_raw/scenes/prefab/s16_tft_longma", "s16_tft_longma"},
    {"S16 大观园", "art_tft_raw/scenes/prefab/s16_tft_dgy", "s16_tft_dgy"},
    {"S6 虎年新春", "art_tft_raw/scenes/prefab/s6_tft_newyear", "s6_tft_newyear"},
    {"S16 时装", "art_tft_raw/scenes/prefab/s16_tft_fashion", "s16_tft_fashion"},
    {"S13 饺子馆", "art_tft_raw/scenes/prefab/s13_tft_dumpling", "s13_tft_dumpling"},
    {"S6 艾克实验室", "art_tft_raw/scenes/prefab/s6_tft_ekkolab", "s6_tft_ekkolab"},
    {"S8 街头", "art_tft_raw/scenes/prefab/s8_tft_street", "s8_tft_street"},
    {"S8 赛博朋克", "art_tft_raw/scenes/prefab/s8_tft_cyberpunk", "s8_tft_cyberpunk"},
    {"S7 星之守护者", "art_tft_raw/scenes/prefab/s7_tft_xingshou", "s7_tft_xingshou"},
    {"S17 富贵马", "art_tft_raw/scenes/prefab/s17_tft_richhorse", "s17_tft_richhorse"},
    {"S16 2026迎新", "art_tft_raw/scenes/prefab/s16_tft_2026yx", "s16_tft_2026yx"},
    {"S16 掌剑", "art_tft_raw/scenes/prefab/s16_tft_zhangjian", "s16_tft_zhangjian"},
    {"S13 广州", "art_tft_raw/scenes/prefab/s13_tft_guangzhou", "s13_tft_guangzhou"},
    {"S9 德玛西亚", "art_tft_raw/scenes/prefab/s9_tft_demacia", "s9_tft_demacia"},
    {"S9 艾欧尼亚亚索", "art_tft_raw/scenes/prefab/s9_tft_ioniayasuo", "s9_tft_ioniayasuo"},
    {"S4 凌云", "art_tft_raw/scenes/prefab/s4_tft_lingyun_001", "s4_tft_lingyun_001"}
};

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

template <typename T, size_t Size>
class RingBuffer {
    T buffer[Size];
    std::atomic<size_t> head{0};
    std::atomic<size_t> tail{0};
public:
    bool push(const T& item) {
        size_t current_tail = tail.load(std::memory_order_relaxed);
        size_t next_tail = (current_tail + 1) % Size;
        if (next_tail != head.load(std::memory_order_acquire)) {
            buffer[current_tail] = item;
            tail.store(next_tail, std::memory_order_release);
            return true;
        }
        return false;
    }
    bool pop(T& item) {
        size_t current_head = head.load(std::memory_order_relaxed);
        if (current_head == tail.load(std::memory_order_acquire)) return false; 
        item = buffer[current_head];
        head.store((current_head + 1) % Size, std::memory_order_release);
        return true;
    }
    void clear() {
        head.store(0, std::memory_order_relaxed);
        tail.store(0, std::memory_order_relaxed);
    }
};

struct PlayerRowInfo { uintptr_t obj; int rowIdx; };
struct PlayerStatInfo { uintptr_t obj; int val; };

struct MainThreadTasks {
    std::atomic<bool> trigger_surrender{false};
    std::atomic<bool> trigger_game_end{false};
    std::atomic<uintptr_t> pending_hex_cfg_obj{0};
    uintptr_t processed_hex_cfg_obj = 0;
    std::atomic<uintptr_t> pending_turn_start_obj{0};

    RingBuffer<uintptr_t, 128> update_turn_start_queue;
    RingBuffer<PlayerRowInfo, 128> player_row_queue;
    RingBuffer<PlayerStatInfo, 256> money_queue;
    RingBuffer<PlayerStatInfo, 256> level_queue;
} g_Tasks;

uintptr_t g_shop_slot_instances[5] = {0};
int g_shop_slot_index = 0;
typedef void (*func_reqbuyhero_t)(void* x0, void* x1, void* x2, void* x3);
func_reqbuyhero_t g_reqbuyhero = nullptr;

uintptr_t g_refresh_instance = 0;
typedef void (*func_req_refresh_t)(void* x0);
func_req_refresh_t g_req_refresh = nullptr;

struct Vector3 { float x, y, z; };

// 增加 cellIndex 来记录是哪个格子(0-27场上, 28-36备战席)
struct LiveHeroData { int heroId; Vector3 worldPos; Vector3 screenPos; int equipCount; int cellIndex; };

struct DebugHeroTrace {
    int index;
    uintptr_t heroObj;
    int heroId;
    uintptr_t unitData;
    uintptr_t battleUnit;
    Vector3 worldPos;
    Vector3 screenPos;
};

struct PlayerInfo {
    uintptr_t objAddr = 0, midAddr = 0, strPtr = 0, dynamicAddr = 0, logicObjAddr = 0, heroArrayAddr = 0, csoGameAddr = 0;
    int playerId = 0, rowIdx = 0, nameLen = 0, money = 0, level = 0;
    int winStreak = 0, loseStreak = 0; 
    std::string name = "";
    bool isAi = false;
};
struct EnemyHeroPos { int heroId; int x; int y; int equipCount; };
struct HeroSlotInfo { int heroId; int baseHeroId; int slotIndex; int cost; int starLevel; };
struct CardInfo { int heroId; int remaining; int total; int cost; };
struct PlayerWarningInfo { int rowIdx; std::vector<std::pair<int, int>> heroCounts; };

struct GameSnapshot {
    std::unordered_map<int, PlayerInfo> players;
    std::unordered_map<uintptr_t, int> objToPlayerId;
    std::vector<EnemyHeroPos> currentEnemyHeroes;
    std::vector<HeroSlotInfo> myShopHeroes, myBenchHeroes, enemyShopHeroes, enemyBenchHeroes;
    std::unordered_map<int, CardInfo> cardPoolData;
    std::vector<PlayerWarningInfo> playerWarnings;

    uintptr_t chessBattleModel = 0;
    uintptr_t csoGame = 0;
    int season = 0;
    int myPlayerId = -1;
    int nextEnemyPlayerId = -1;
    
    int cachedHexes[4] = {0, 0, 0, 0};
    int validHexCount = 0;
    bool hex_fetched = false;
    
    uintptr_t mainCamera = 0;
    std::vector<int> myItems; 
    std::vector<LiveHeroData> myLiveHeroes;
    std::vector<LiveHeroData> myBenchLiveHeroes; 
    
    std::vector<DebugHeroTrace> debugHeroTraces;
};

GameSnapshot g_BackendData;
GameSnapshot g_Snapshots[2];
std::atomic<int> g_WriteIdx{0};
std::atomic<int> g_ReadIdx{0};

std::atomic<bool> g_is_in_match{false}; 

void PushSnapshot() {
    int write_idx = g_WriteIdx.load(std::memory_order_relaxed);
    int next_write = 1 - write_idx;
    g_Snapshots[write_idx] = g_BackendData;
    g_ReadIdx.store(write_idx, std::memory_order_release);
    g_WriteIdx.store(next_write, std::memory_order_relaxed);
}

const GameSnapshot* GetSnapshot() {
    int read_idx = g_ReadIdx.load(std::memory_order_acquire);
    return &g_Snapshots[read_idx];
}

uintptr_t g_il2cppBase = 0, g_debug_mgrObj = 0;
void* g_turnStartThisObj = nullptr; 
void* g_hexCfgObj = nullptr;

int g_debug_TurnStartCalled = 0, g_debug_hook_3Cd6058_count = 0, g_debug_hook_set_Money_count = 0, g_debug_hook_set_Level_count = 0;
int g_debug_UpdateTurnStart_count = 0, g_debug_set_IsGameEnd_count = 0, g_debug_SendWillRenderCanvases_count = 0;
int g_debug_collected_players_count = 0, g_debug_hook_InitTurnDropCfg_count = 0, g_debug_bg_thread_count = 0; 
int g_debug_hook_6A67e48_count = 0;

std::atomic<bool> g_collected_all_players{false};
std::atomic<bool> g_matchFetchRequested{false}, g_matchFetchReady{false};
std::chrono::steady_clock::time_point g_matchFetchTime;
std::atomic<uintptr_t> g_pool_entries1{0}; 

// =================================================================
// 极速数学解析引擎
// =================================================================
inline void FastParseHeroId(int rawHeroId, int& baseHeroId, int& cost, int& starLevel) {
    if (rawHeroId < 10) { baseHeroId = rawHeroId; cost = 1; starLevel = 1; return; }
    if (rawHeroId >= 10000) { starLevel = rawHeroId / 10000; cost = (rawHeroId / 1000) % 10; baseHeroId = rawHeroId - (starLevel * 10000) + 10000; } 
    else if (rawHeroId >= 1000) { starLevel = rawHeroId / 1000; cost = (rawHeroId / 100) % 10; baseHeroId = rawHeroId - (starLevel * 1000) + 1000; } 
    else if (rawHeroId >= 100) { starLevel = rawHeroId / 100; cost = (rawHeroId / 10) % 10; baseHeroId = rawHeroId - (starLevel * 100) + 100; } 
    else { starLevel = rawHeroId / 10; cost = rawHeroId % 10; baseHeroId = rawHeroId - (starLevel * 10) + 10; }
}

inline int GetCostFromHeroId(int heroId) {
    if (heroId < 10) return 1;
    int cost = 1;
    if (heroId >= 10000) cost = (heroId / 1000) % 10;
    else if (heroId >= 1000) cost = (heroId / 100) % 10;
    else if (heroId >= 100) cost = (heroId / 10) % 10;
    else cost = heroId % 10;
    return (cost >= 1 && cost <= 5) ? cost : 1; 
}

void BuildLocalPokedex() {
    if (g_pokedex_ready.load()) return;
    
    std::thread([]() {
        std::map<int, std::vector<int>> temp_pokedex;
        for (int i = 1; i <= 60000; i++) { 
            int len = 0;
            const unsigned char* ptr = GetHeroImageBytes(i, &len);
            if (ptr != nullptr && len > 0) {
                int cost = GetCostFromHeroId(i);
                if (cost >= 1 && cost <= 5) { temp_pokedex[cost].push_back(i); }
            }
        }
        g_local_pokedex = std::move(temp_pokedex);
        
        std::vector<int> temp_equip_pokedex;
        for (int i = 1; i <= 20000; i++) { 
            int len = 0;
            const unsigned char* ptr = GetEquipImageBytes(i, &len);
            if (ptr != nullptr && len > 0) {
                temp_equip_pokedex.push_back(i);
            }
        }
        g_local_equip_pokedex = std::move(temp_equip_pokedex);
        g_equip_pokedex_ready.store(true);

        g_pokedex_ready.store(true);
    }).detach();
}

// =================================================================
// 异步纹理加载队列
// =================================================================
struct TexDecodedData { int w, h; unsigned char* pixels; };
std::mutex g_TexMutex;

std::unordered_map<int, GLuint> g_heroTextureCache; 
std::vector<std::pair<int, TexDecodedData>> g_HeroTexDecodedQueue;

std::unordered_map<int, GLuint> g_equipTextureCache; 
std::vector<std::pair<int, TexDecodedData>> g_EquipTexDecodedQueue;

struct DecodeRequest { int id; bool isEquip; };
std::deque<DecodeRequest> g_DecodeRequestQueue;
std::mutex g_DecodeRequestMutex;

void ClearAllTextureCaches() {
    std::lock_guard<std::mutex> lock(g_TexMutex);
    for (auto& pair : g_heroTextureCache) { if (pair.second > 0) glDeleteTextures(1, &pair.second); }
    g_heroTextureCache.clear();
    for (auto& item : g_HeroTexDecodedQueue) { if (item.second.pixels) stbi_image_free(item.second.pixels); }
    g_HeroTexDecodedQueue.clear();

    for (auto& pair : g_equipTextureCache) { if (pair.second > 0) glDeleteTextures(1, &pair.second); }
    g_equipTextureCache.clear();
    for (auto& item : g_EquipTexDecodedQueue) { if (item.second.pixels) stbi_image_free(item.second.pixels); }
    g_EquipTexDecodedQueue.clear();
}

void TextureDecodingWorkerThread() {
    while (true) {
        DecodeRequest req;
        bool hasReq = false;

        {
            std::lock_guard<std::mutex> lock(g_DecodeRequestMutex);
            if (!g_DecodeRequestQueue.empty()) {
                req = g_DecodeRequestQueue.front();
                g_DecodeRequestQueue.pop_front();
                hasReq = true;
            }
        }

        if (hasReq) {
            int imgLen = 0;
            const unsigned char* imgData = req.isEquip ? GetEquipImageBytes(req.id, &imgLen) : GetHeroImageBytes(req.id, &imgLen);
            
            if (imgData != nullptr && imgLen > 0) {
                int w, h, channels;
                unsigned char* data = stbi_load_from_memory(imgData, imgLen, &w, &h, &channels, 4);
                if (data && w > 0 && h > 0 && w <= 2048 && h <= 2048) {
                    std::lock_guard<std::mutex> lock(g_TexMutex);
                    if (req.isEquip) g_EquipTexDecodedQueue.push_back({req.id, {w, h, data}});
                    else g_HeroTexDecodedQueue.push_back({req.id, {w, h, data}});
                } else if (data) {
                    stbi_image_free(data);
                }
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    }
}

GLuint GetHeroTexture(int heroId) {
    auto it = g_heroTextureCache.find(heroId);
    if (it != g_heroTextureCache.end()) return it->second; 
    
    g_heroTextureCache[heroId] = 0; 
    std::lock_guard<std::mutex> lock(g_DecodeRequestMutex);
    g_DecodeRequestQueue.push_back({heroId, false});
    
    return 0;
}

GLuint GetEquipTexture(int equipId) {
    auto it = g_equipTextureCache.find(equipId);
    if (it != g_equipTextureCache.end()) return it->second;

    g_equipTextureCache[equipId] = 0; 
    std::lock_guard<std::mutex> lock(g_DecodeRequestMutex);
    g_DecodeRequestQueue.push_back({equipId, true});

    return 0;
}

void ProcessTextureQueue() {
    std::lock_guard<std::mutex> lock(g_TexMutex);
    if (g_HeroTexDecodedQueue.empty() && g_EquipTexDecodedQueue.empty()) return;

    GLint last_unpack; glGetIntegerv(GL_UNPACK_ALIGNMENT, &last_unpack);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); 

    for (auto& item : g_HeroTexDecodedQueue) {
        GLuint tex = 0;
        glGenTextures(1, &tex);
        if (tex != 0) {
            glBindTexture(GL_TEXTURE_2D, tex);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, item.second.w, item.second.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, item.second.pixels);
        }
        stbi_image_free(item.second.pixels);
        g_heroTextureCache[item.first] = tex;
    }
    g_HeroTexDecodedQueue.clear();

    for (auto& item : g_EquipTexDecodedQueue) {
        GLuint tex = 0;
        glGenTextures(1, &tex);
        if (tex != 0) {
            glBindTexture(GL_TEXTURE_2D, tex);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, item.second.w, item.second.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, item.second.pixels);
        }
        stbi_image_free(item.second.pixels);
        g_equipTextureCache[item.first] = tex;
    }
    g_EquipTexDecodedQueue.clear();

    glPixelStorei(GL_UNPACK_ALIGNMENT, last_unpack);
}

inline void DrawTextWithStroke(ImDrawList* d, ImFont* font, float fontSize, ImVec2 pos, ImU32 textColor, ImU32 strokeColor, const char* text) {
    d->AddText(font, fontSize, pos + ImVec2(1.5f, 1.5f), strokeColor, text);
    d->AddText(font, fontSize, pos, textColor, text);
}

// =================================================================
// 牌池全自动极速采集引擎
// =================================================================
typedef void* (*func_6A67e48_t)(void* x0, void* x1, void* x2, void* x3);
func_6A67e48_t orig_6A67e48 = nullptr;

void* hook_6A67e48(void* x0, void* x1, void* x2, void* x3) {
    if (!g_is_in_match.load(std::memory_order_relaxed)) return orig_6A67e48(x0, x1, x2, x3);
    if (g_pool_entries1.load(std::memory_order_relaxed) != 0) {
        return orig_6A67e48(x0, x1, x2, x3);
    }
    g_debug_hook_6A67e48_count++; 
    
    if (x0 != nullptr) {
        uintptr_t base = (uintptr_t)x0;
        uintptr_t heroku = SAFE_READ_PTR(base, g_off_pool_heroku);
        if (IsValidPtr(heroku)) {
            uintptr_t aheroku = SAFE_READ_PTR(heroku, g_off_pool_Aheroku);
            if (IsValidPtr(aheroku)) {
                uintptr_t entries1 = SAFE_READ_PTR(aheroku, g_off_pool_entries1);
                if (IsValidPtr(entries1)) g_pool_entries1.store(entries1, std::memory_order_release); 
            }
        }
    }
    return orig_6A67e48(x0, x1, x2, x3);
}

void UpdateCardPoolData() {
    uintptr_t pEntries1 = g_pool_entries1.load(std::memory_order_acquire);
    if (pEntries1 == 0 || !IsValidPtr(pEntries1)) return;

    static auto last_pool_update = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_pool_update).count() < 100) return;
    last_pool_update = now;

    int count1 = SAFE_READ(int, pEntries1, g_off_pool_e1_count, 0);
    int maxIter1 = std::clamp(count1, 0, 150);

    for (int i = 0; i < maxIter1; i++) {
        uintptr_t entry1_ptr = pEntries1 + g_off_pool_e1_head + i * g_off_pool_e1_step;
        uintptr_t value1 = SAFE_READ_PTR(entry1_ptr, g_off_pool_e1_val);
        if (!IsValidPtr(value1)) continue;

        uintptr_t entries2 = SAFE_READ_PTR(value1, g_off_pool_entries2);
        if (!IsValidPtr(entries2)) continue;

        int count2 = SAFE_READ(int, entries2, g_off_pool_e2_count, 0);
        int maxIter2 = std::clamp(count2, 0, 100);

        for (int j = 0; j < maxIter2; j++) {
            uintptr_t entry2_ptr = entries2 + g_off_pool_e2_head + j * g_off_pool_e2_step;
            uintptr_t value2 = SAFE_READ_PTR(entry2_ptr, g_off_pool_e2_val);
            if (!IsValidPtr(value2)) continue;

            int heroId = SAFE_READ(int, value2, g_off_pool_v2_id, 0);
            if (heroId > 0) {
                int rem = SAFE_READ(int, value2, g_off_pool_v2_rem, 0);
                int tot = SAFE_READ(int, value2, g_off_pool_v2_tot, 0);
                int cost = GetCostFromHeroId(heroId);
                g_BackendData.cardPoolData[heroId] = { heroId, rem, tot, cost };
            }
        }
    }
}

// =================================================================
// 解析引擎核心 (后台线程无锁调用)
// =================================================================
auto getEntriesDynamically = [](uintptr_t objAddr, int baseOff) -> uintptr_t {
    if (!objAddr || !IsValidPtr(objAddr)) return 0;
    uintptr_t pBase = SAFE_READ_PTR(objAddr, baseOff);
    if (!pBase) return 0;
    uintptr_t p20 = SAFE_READ_PTR(pBase, g_off_shop_p20);
    if (!p20) return 0;
    return SAFE_READ_PTR(p20, g_off_shop_entries);
};

void EnsureChessBattleModel() {
    if (g_il2cppBase == 0) return;
    typedef void* (*func_58543a8_t)();
    func_58543a8_t getMgr = (func_58543a8_t)(g_il2cppBase + 0x58543a8);
    void* mgrObj = getMgr();
    
    if (mgrObj && IsValidPtr((uintptr_t)mgrObj)) {
        g_debug_mgrObj = (uintptr_t)mgrObj; 
        uintptr_t current_model = SAFE_READ_PTR(mgrObj, 0x10);
        if (IsValidPtr(current_model)) {
            typedef int (*func_3fac734_t)(void* battleModel);
            func_3fac734_t getMyId = (func_3fac734_t)(g_il2cppBase + 0x3fac734);
            int myId = getMyId ? getMyId((void*)current_model) : -1;

            bool isNewModel = (g_BackendData.chessBattleModel != current_model) || (g_BackendData.myPlayerId != myId);

            if (isNewModel && myId != -1) {
                int season = SAFE_READ(int, current_model, 0x140, 0);

                g_BackendData = GameSnapshot(); 
                g_BackendData.chessBattleModel = current_model;
                g_BackendData.myPlayerId = myId;
                g_BackendData.season = season;
                g_BackendData.nextEnemyPlayerId = -1;

                g_collected_all_players.store(false, std::memory_order_relaxed);
                g_debug_collected_players_count = 0;
                
                g_hexCfgObj = nullptr;
                g_Tasks.processed_hex_cfg_obj = 0;
                g_Tasks.pending_hex_cfg_obj.store(0);
                g_pool_entries1.store(0, std::memory_order_release);
                
                g_Tasks.player_row_queue.clear();
                g_Tasks.money_queue.clear();
                g_Tasks.level_queue.clear();
                g_Tasks.update_turn_start_queue.clear();
                g_shop_slot_index = 0;
                for (int i = 0; i < 5; i++) g_shop_slot_instances[i] = 0;

                {
                    std::lock_guard<std::mutex> lock(g_swipe_mutex);
                    g_active_swipes.clear(); 
                }

                g_is_in_match.store(true, std::memory_order_release);
                
                PushSnapshot();
                PushSnapshot();
            }
        }
    }
}

void CheckReadyMatchFetch() {
    uintptr_t modelToFetch = g_BackendData.chessBattleModel; 
    int myIdToFetch = g_BackendData.myPlayerId;

    if (modelToFetch != 0 && myIdToFetch != -1 && IsValidPtr(modelToFetch)) {
        typedef int (*func_GetMatchPlayerId_t)(void* battleModel, int myPlayerId);
        func_GetMatchPlayerId_t getMatch = (func_GetMatchPlayerId_t)(g_il2cppBase + 0x3fd7a80);
        if (getMatch) {
            try {
                int nextEnemy = getMatch((void*)modelToFetch, myIdToFetch);
                if (nextEnemy != -1) {
                    g_BackendData.nextEnemyPlayerId = nextEnemy;
                }
            } catch (...) {}
        }
    }
}

void UpdateShopAndBenchData() {
    uintptr_t myShop = 0, myBench = 0, enemyShop = 0, enemyBench = 0;
    uintptr_t myLogicObj = 0, myHeroArray = 0; 
    
    auto myIt = g_BackendData.players.find(g_BackendData.myPlayerId);
    if (g_BackendData.myPlayerId != -1 && myIt != g_BackendData.players.end()) {
        uintptr_t myObj = myIt->second.objAddr;
        myShop = getEntriesDynamically(myObj, g_off_shop_base);
        myBench = getEntriesDynamically(myObj, g_off_bench_base);
        
        myLogicObj = myIt->second.logicObjAddr;
        myHeroArray = myIt->second.heroArrayAddr;
    }
    
    int enemyId = g_BackendData.nextEnemyPlayerId;
    auto enemyIt = g_BackendData.players.find(enemyId);
    if (enemyId != -1 && enemyIt != g_BackendData.players.end()) {
        uintptr_t enemyObj = enemyIt->second.objAddr;
        enemyShop = getEntriesDynamically(enemyObj, g_off_shop_base);
        enemyBench = getEntriesDynamically(enemyObj, g_off_bench_base);
    }

    if (!IsValidPtr(myHeroArray) && IsValidPtr(myLogicObj)) {
        uintptr_t dyn2b0 = SAFE_READ_PTR(myLogicObj, 0x2b0);
        if (IsValidPtr(dyn2b0)) {
            myHeroArray = SAFE_READ_PTR(dyn2b0, 0x20);
            if (IsValidPtr(myHeroArray)) g_BackendData.players[g_BackendData.myPlayerId].heroArrayAddr = myHeroArray;
        }
    }

    auto parseEntries = [](uintptr_t entriesAddr, int maxSlots, std::vector<HeroSlotInfo>& outVec) {
        outVec.clear(); 
        if (!IsValidPtr(entriesAddr)) return;
        
        int count = SAFE_READ(int, entriesAddr, g_off_entries_count, 0);
        int maxIter = std::clamp(count, 0, 50); 
        outVec.reserve(maxIter); 
        
        for (int i = 0; i < maxIter; i++) {
            uintptr_t entryStart = entriesAddr + g_off_entries_head + i * g_off_entries_step; 
            uintptr_t value = SAFE_READ_PTR(entryStart, g_off_entry_value); 
            if (!value) continue;

            int slotIndex = SAFE_READ(int, value, g_off_value_slot, -1); 
            if (slotIndex >= 0 && slotIndex < maxSlots) { 
                uintptr_t p10 = SAFE_READ_PTR(value, g_off_value_p10); 
                if (p10) {
                    int heroId = SAFE_READ(int, p10, g_off_p10_heroId, 0); 
                    if (heroId > 0) {
                        int starLevel, cost, baseHeroId;
                        FastParseHeroId(heroId, baseHeroId, cost, starLevel);
                        outVec.push_back({heroId, baseHeroId, slotIndex, cost, starLevel});
                    }
                }
            }
        }
    };

    parseEntries(myShop, 5, g_BackendData.myShopHeroes);
    parseEntries(myBench, 9, g_BackendData.myBenchHeroes);
    parseEntries(enemyShop, 5, g_BackendData.enemyShopHeroes);
    parseEntries(enemyBench, 9, g_BackendData.enemyBenchHeroes);
}

void UpdateEnemyBoardState() {
    uintptr_t heroArray = 0;
    uintptr_t logicObj = 0;
    
    auto it = g_BackendData.players.find(g_BackendData.nextEnemyPlayerId);
    if (g_BackendData.nextEnemyPlayerId != -1 && it != g_BackendData.players.end()) {
        logicObj = it->second.logicObjAddr;
    }

    if (IsValidPtr(logicObj)) {
        uintptr_t dynAddr2b0 = SAFE_READ_PTR(logicObj, 0x2b0);
        if (IsValidPtr(dynAddr2b0)) {
            uintptr_t newArray = SAFE_READ_PTR(dynAddr2b0, 0x20);
            if (IsValidPtr(newArray)) {
                heroArray = newArray;
                if (it != g_BackendData.players.end()) {
                    it->second.heroArrayAddr = heroArray; 
                }
            }
        }
    }
    if (!IsValidPtr(heroArray) && it != g_BackendData.players.end()) {
        heroArray = it->second.heroArrayAddr;
    }

    if (!IsValidPtr(heroArray)) return;

    int heroCount = SAFE_READ(int, heroArray, g_off_hero_count, 0);
    if (heroCount <= 0 || heroCount > 200) {
        auto it = g_BackendData.players.find(g_BackendData.nextEnemyPlayerId);
        if (it != g_BackendData.players.end()) it->second.heroArrayAddr = 0;
        return;
    }

    g_BackendData.currentEnemyHeroes.clear();

    for (int x = 0; x < 7; ++x) {
        for (int y = 0; y < 4; ++y) {
            uintptr_t heroObj = SAFE_READ_PTR(heroArray, 0x20 + (x * 8 + y) * 8);
            if (IsValidPtr(heroObj)) {
                int heroId = SAFE_READ(int, heroObj, 0x108, 0);
                if (heroId > 0) {
                    int equipCount = 0;
                    uintptr_t equipAddr = SAFE_READ_PTR(heroObj, 0x218);
                    if (IsValidPtr(equipAddr)) equipCount = SAFE_READ(int, equipAddr, 0x18, 0);
                    g_BackendData.currentEnemyHeroes.push_back({heroId, x, y, equipCount});
                }
            }
        }
    }
}

typedef Vector3 (*func_WorldToScreenPoint_t)(void* camera, Vector3 position);

void UpdateEquipAndLiveHeroes() {
    auto it = g_BackendData.players.find(g_BackendData.myPlayerId);
    if (it == g_BackendData.players.end()) return;
    uintptr_t logicObj = it->second.logicObjAddr;
    if (!IsValidPtr(logicObj)) return;

    if (g_il2cppBase != 0) {
        typedef void* (*func_get_main_t)();
        func_get_main_t get_main = (func_get_main_t)(g_il2cppBase + 0x9898204);
        if (get_main) {
            try {
                void* cam = get_main();
                if (cam && IsValidPtr((uintptr_t)cam)) {
                    g_BackendData.mainCamera = (uintptr_t)cam;
                } else {
                    g_BackendData.mainCamera = 0;
                }
            } catch(...) {}
        }
    }

    g_BackendData.myItems.clear();
    uintptr_t pEquipBase = SAFE_READ_PTR(logicObj, g_off_equip_base);
    if (IsValidPtr(pEquipBase)) {
        uintptr_t itemArr = SAFE_READ_PTR(pEquipBase, g_off_equip_arr);
        if (IsValidPtr(itemArr)) {
            int equipCount = SAFE_READ(int, itemArr, g_off_equip_count, 0);
            int maxEquipIter = std::clamp(equipCount, 0, 20); 
            for (int i = 0; i < maxEquipIter; i++) {
                uintptr_t itemObj = SAFE_READ_PTR(itemArr, g_off_equip_head + i * g_off_equip_step);
                int equipId = 0;
                if (IsValidPtr(itemObj)) {
                    equipId = SAFE_READ(int, itemObj, g_off_equip_id, 0);
                }
                g_BackendData.myItems.push_back(equipId);
            }
        }
    }

    g_BackendData.myLiveHeroes.clear();
    g_BackendData.myBenchLiveHeroes.clear(); 
    g_BackendData.debugHeroTraces.clear(); 
    
    uintptr_t pDyn = SAFE_READ_PTR(logicObj, g_off_hero_dyn);
    uintptr_t cachedHeroArr = 0;
    if (IsValidPtr(pDyn)) {
        cachedHeroArr = SAFE_READ_PTR(pDyn, g_off_hero_arr);
        if (IsValidPtr(cachedHeroArr)) {
            it->second.heroArrayAddr = cachedHeroArr;
        }
    }
    if (!IsValidPtr(cachedHeroArr)) {
        cachedHeroArr = it->second.heroArrayAddr;
    }

    if (IsValidPtr(cachedHeroArr)) {
        int heroCount = SAFE_READ(int, cachedHeroArr, g_off_hero_count, 0);
        
        if (heroCount <= 0 || heroCount > 200) {
            it->second.heroArrayAddr = 0;
            return;
        }

        int maxHeroIter = std::min(heroCount, 128); 
        
        for (int i = 0; i < maxHeroIter; i++) {
            uintptr_t heroObj = SAFE_READ_PTR(cachedHeroArr, g_off_hero_head + i * g_off_hero_step);
            if (IsValidPtr(heroObj)) {
                int hId = SAFE_READ(int, heroObj, g_off_hero_id, 0);
                if (hId > 0 && hId < 60000) {
                    uintptr_t unitData = SAFE_READ_PTR(heroObj, g_off_hero_unit);
                    uintptr_t battleUnit = 0;
                    Vector3 pos = {0,0,0};
                    Vector3 screenPos = {0,0,0};
                    
                    if (IsValidPtr(unitData)) {
                        battleUnit = SAFE_READ_PTR(unitData, g_off_unit_battle);
                        if (IsValidPtr(battleUnit)) {
                            pos.x = SAFE_READ(float, battleUnit, g_off_battle_posx, 0.0f);
                            pos.y = SAFE_READ(float, battleUnit, g_off_battle_posx + 4, 0.0f);
                            pos.z = SAFE_READ(float, battleUnit, g_off_battle_posx + 8, 0.0f);

                            // 【彻底修复追踪线乱跳】：加上严格的野指针检测与边界过滤！
                            if (std::isnan(pos.x) || std::isnan(pos.y) || std::isnan(pos.z) || 
                                std::isinf(pos.x) || std::isinf(pos.y) || std::isinf(pos.z) ||
                                pos.y < -50.0f || pos.y > 200.0f || 
                                pos.x < -300.0f || pos.x > 300.0f || 
                                pos.z < -300.0f || pos.z > 300.0f) {
                                continue;
                            }

                            if (g_BackendData.mainCamera != 0 && g_il2cppBase != 0) {
                                func_WorldToScreenPoint_t w2s = (func_WorldToScreenPoint_t)(g_il2cppBase + 0x9897df8);
                                try { screenPos = w2s((void*)g_BackendData.mainCamera, pos); } catch(...) {}
                            }
                            
                            // 屏幕坐标过滤：摄像机背面(z<=0.01)或者转换失败的，直接丢弃
                            if (screenPos.z <= 0.01f || std::isnan(screenPos.x) || std::isnan(screenPos.y) || std::isinf(screenPos.x) || std::isinf(screenPos.y)) {
                                continue;
                            }
                            
                            int equipCount = 0;
                            uintptr_t equipAddr = SAFE_READ_PTR(heroObj, 0x218);
                            if (IsValidPtr(equipAddr)) equipCount = SAFE_READ(int, equipAddr, 0x18, 0);

                            if (i < 28) {
                                g_BackendData.myLiveHeroes.push_back({hId, pos, screenPos, equipCount, i});
                            } else {
                                g_BackendData.myBenchLiveHeroes.push_back({hId, pos, screenPos, equipCount, i});
                            }
                        }
                    }
                    
                    DebugHeroTrace trace;
                    trace.index = i;
                    trace.heroObj = heroObj;
                    trace.heroId = hId;
                    trace.unitData = unitData;
                    trace.battleUnit = battleUnit;
                    trace.worldPos = pos;
                    trace.screenPos = screenPos;
                    g_BackendData.debugHeroTraces.push_back(trace);
                }
            }
        }
    }
}

void UpdateWarningData() {
    if (!g_card_warning) return; 
    
    g_BackendData.playerWarnings.clear();
    for (const auto& pair : g_BackendData.players) {
        const PlayerInfo& p = pair.second;
        if (p.rowIdx < 1 || p.rowIdx > 8) continue;
        
        std::unordered_map<int, int> counts; 
        auto addHeroToCount = [&](int heroId) {
            if (heroId <= 0) return;
            int star = 1, baseId = heroId, dummyCost;
            FastParseHeroId(heroId, baseId, dummyCost, star); 
            counts[baseId] += (star == 3 ? 9 : (star == 2 ? 3 : 1));
        };

        uintptr_t shopEntries = getEntriesDynamically(p.objAddr, g_off_shop_base);
        if (IsValidPtr(shopEntries)) {
            int count = SAFE_READ(int, shopEntries, g_off_entries_count, 0);
            for (int i = 0; i < std::clamp(count, 0, 5); i++) {
                uintptr_t entryStart = shopEntries + g_off_entries_head + i * g_off_entries_step;
                uintptr_t value = SAFE_READ_PTR(entryStart, g_off_entry_value);
                if (value) {
                    uintptr_t p10 = SAFE_READ_PTR(value, g_off_value_p10);
                    if (p10) addHeroToCount(SAFE_READ(int, p10, g_off_p10_heroId, 0));
                }
            }
        }

        uintptr_t benchEntries = getEntriesDynamically(p.objAddr, g_off_bench_base);
        if (IsValidPtr(benchEntries)) {
            int count = SAFE_READ(int, benchEntries, g_off_entries_count, 0);
            for (int i = 0; i < std::clamp(count, 0, 9); i++) {
                uintptr_t entryStart = benchEntries + g_off_entries_head + i * g_off_entries_step;
                uintptr_t value = SAFE_READ_PTR(entryStart, g_off_entry_value);
                if (value) {
                    uintptr_t p10 = SAFE_READ_PTR(value, g_off_value_p10);
                    if (p10) addHeroToCount(SAFE_READ(int, p10, g_off_p10_heroId, 0));
                }
            }
        }

        uintptr_t heroArr = 0;
        if (IsValidPtr(p.logicObjAddr)) {
            uintptr_t dyn2b0 = SAFE_READ_PTR(p.logicObjAddr, 0x2b0);
            if (IsValidPtr(dyn2b0)) heroArr = SAFE_READ_PTR(dyn2b0, 0x20);
        }
        if (!IsValidPtr(heroArr)) {
            heroArr = p.heroArrayAddr;
        }
        if (IsValidPtr(heroArr)) {
            for (int i = 0; i < 28; i++) { 
                uintptr_t heroObj = SAFE_READ_PTR(heroArr, 0x20 + i * 8);
                if (IsValidPtr(heroObj)) addHeroToCount(SAFE_READ(int, heroObj, 0x108, 0));
            }
        }

        PlayerWarningInfo info;
        info.rowIdx = p.rowIdx;
        info.heroCounts.reserve(counts.size());
        for (const auto& kv : counts) info.heroCounts.push_back({kv.first, kv.second});
        g_BackendData.playerWarnings.push_back(std::move(info));
    }
}

void UpdatePlayerStreaks() {
    for (auto& pair : g_BackendData.players) {
        uintptr_t logicObj = pair.second.logicObjAddr;
        if (IsValidPtr(logicObj)) {
            pair.second.winStreak = SAFE_READ(int, logicObj, 0xdc, 0);
            pair.second.loseStreak = SAFE_READ(int, logicObj, 0xe0, 0);
        }
    }
}

// =================================================================
// JNI Hook 与 剪贴板支持
// =================================================================
JavaVM* g_jvm = nullptr;
jobject g_context = nullptr;
jobject g_view_obj = nullptr; 

void CopyToAndroidClipboard(const char* text) {
    if (!g_jvm || !g_context) return;
    JNIEnv* env = nullptr;
    int status = g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    bool attached = false;
    if (status == JNI_EDETACHED) {
        if (g_jvm->AttachCurrentThread(&env, nullptr) != JNI_OK) return;
        attached = true;
    }
    if (!env) return;

    jstring jtext = env->NewStringUTF(text);
    jclass contextClass = env->GetObjectClass(g_context);
    jmethodID getSystemServiceMid = env->GetMethodID(contextClass, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
    jstring serviceName = env->NewStringUTF("clipboard");
    jobject clipboard = env->CallObjectMethod(g_context, getSystemServiceMid, serviceName);

    if (clipboard) {
        jclass clipboardClass = env->GetObjectClass(clipboard);
        jclass clipDataClass = env->FindClass("android/content/ClipData");
        if (clipDataClass) {
            jmethodID newPlainTextMid = env->GetStaticMethodID(clipDataClass, "newPlainText", "(Ljava/lang/CharSequence;Ljava/lang/CharSequence;)Landroid/content/ClipData;");
            jstring label = env->NewStringUTF("CopiedPath");
            jobject clipData = env->CallStaticObjectMethod(clipDataClass, newPlainTextMid, label, jtext);
            
            jmethodID setPrimaryClipMid = env->GetMethodID(clipboardClass, "setPrimaryClip", "(Landroid/content/ClipData;)V");
            if (setPrimaryClipMid) env->CallVoidMethod(clipboard, setPrimaryClipMid, clipData);
            
            env->DeleteLocalRef(clipData);
            env->DeleteLocalRef(label);
        } else {
            env->ExceptionClear();
            jmethodID setTextMid = env->GetMethodID(clipboardClass, "setText", "(Ljava/lang/CharSequence;)V");
            if (setTextMid) env->CallVoidMethod(clipboard, setTextMid, jtext);
        }
        env->DeleteLocalRef(clipboardClass);
        env->DeleteLocalRef(clipboard);
    }

    env->DeleteLocalRef(serviceName);
    env->DeleteLocalRef(contextClass);
    env->DeleteLocalRef(jtext);

    if (attached) g_jvm->DetachCurrentThread();
}

typedef void (*nativeInjectEvent_t)(JNIEnv*, jobject, jobject);
nativeInjectEvent_t old_nativeInjectEvent = nullptr;

void SimulateSwipe(float startX, float startY, float endX, float endY, bool instant = false) {
    if (g_show_swipe_visuals) {
        std::lock_guard<std::mutex> lock(g_swipe_mutex);
        g_active_swipes.push_back({{startX, startY}, {endX, endY}, instant ? 0.5f : 2.0f, instant ? 0.5f : 2.0f}); 
    }

    if (!g_jvm || !g_view_obj || !old_nativeInjectEvent) return;

    std::thread([startX, startY, endX, endY, instant]() {
        JNIEnv* env = nullptr;
        bool attached = false;
        if (g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6) == JNI_EDETACHED) {
            if (g_jvm->AttachCurrentThread(&env, nullptr) != JNI_OK) return;
            attached = true;
        }

        jclass sysClockClass = env->FindClass("android/os/SystemClock");
        jmethodID uptimeMid = env->GetStaticMethodID(sysClockClass, "uptimeMillis", "()J");
        jclass meClass = env->FindClass("android/view/MotionEvent");
        jmethodID obtainMid = env->GetStaticMethodID(meClass, "obtain", "(JJIFFI)Landroid/view/MotionEvent;");
        jmethodID recycleMid = env->GetMethodID(meClass, "recycle", "()V");
        jmethodID setSourceMid = env->GetMethodID(meClass, "setSource", "(I)V"); 

        if (sysClockClass && uptimeMid && meClass && obtainMid) {
            long downTime = env->CallStaticLongMethod(sysClockClass, uptimeMid);
            
            float v_startX = startX, v_startY = startY, v_endX = endX, v_endY = endY;
            if (g_cached_view_width > 0 && g_gl_width > 0) {
                v_startX = startX * ((float)g_cached_view_width / g_gl_width);
                v_endX   = endX   * ((float)g_cached_view_width / g_gl_width);
            }
            if (g_cached_view_height > 0 && g_gl_height > 0) {
                v_startY = startY * ((float)g_cached_view_height / g_gl_height);
                v_endY   = endY   * ((float)g_cached_view_height / g_gl_height);
            }

            AddLog("[底层滑动] 渲染坐标(%.1f, %.1f)->(%.1f, %.1f)", startX, startY, endX, endY);
            AddLog("[底层滑动] 物理映射(%.1f, %.1f)->(%.1f, %.1f)", v_startX, v_startY, v_endX, v_endY);

            int steps = instant ? 0 : g_swipe_steps;
            int step_delay = instant ? 0 : g_swipe_step_delay_ms;
            int down_delay = instant ? 20 : g_swipe_down_delay_ms;
            int up_delay = instant ? 20 : g_swipe_up_delay_ms;

            auto injectTouchEvent = [&](int action, float x, float y) {
                long eventTime = env->CallStaticLongMethod(sysClockClass, uptimeMid);
                jobject meObj = env->CallStaticObjectMethod(meClass, obtainMid, downTime, eventTime, action, x, y, 0);
                if (meObj) {
                    if (setSourceMid) env->CallVoidMethod(meObj, setSourceMid, 4098); 
                    old_nativeInjectEvent(env, g_view_obj, meObj);
                    if (recycleMid) env->CallVoidMethod(meObj, recycleMid); 
                    env->DeleteLocalRef(meObj);
                }
            };

            injectTouchEvent(0, v_startX, v_startY);
            if (down_delay > 0) std::this_thread::sleep_for(std::chrono::milliseconds(down_delay));

            if (steps > 0) {
                for (int i = 1; i <= steps; i++) {
                    float t = (float)i / steps;
                    float curX = v_startX + (v_endX - v_startX) * t;
                    float curY = v_startY + (v_endY - v_startY) * t;
                    injectTouchEvent(2, curX, curY);
                    if (step_delay > 0) std::this_thread::sleep_for(std::chrono::milliseconds(step_delay));
                }
            } else {
                injectTouchEvent(2, v_endX, v_endY);
                if (step_delay > 0) std::this_thread::sleep_for(std::chrono::milliseconds(step_delay));
            }
            
            if (up_delay > 0) std::this_thread::sleep_for(std::chrono::milliseconds(up_delay));
            injectTouchEvent(1, v_endX, v_endY);
        }

        if (sysClockClass) env->DeleteLocalRef(sysClockClass);
        if (meClass) env->DeleteLocalRef(meClass);

        if (attached) g_jvm->DetachCurrentThread();
    }).detach(); 
}

// =================================================================
// 序列化
// =================================================================
void SaveConfig() {
    std::lock_guard<std::mutex> lock(g_configMutex); 

    g_auto_buy_heroes_str = "";
    for (int id : g_auto_buy_heroes) {
        g_auto_buy_heroes_str += std::to_string(id) + ",";
    }

    g_hero_equip_bindings_str = "";
    for(const auto& pair : g_hero_equip_bindings) {
        if(pair.second.empty()) continue;
        g_hero_equip_bindings_str += std::to_string(pair.first) + ":";
        for(size_t i=0; i<pair.second.size(); i++) {
            g_hero_equip_bindings_str += std::to_string(pair.second[i]);
            if(i < pair.second.size()-1) g_hero_equip_bindings_str += ",";
        }
        g_hero_equip_bindings_str += ";";
    }

    g_hero_equip_priority_str = "";
    for(const auto& pair : g_hero_equip_priority) {
        if (pair.second > 0) {
            g_hero_equip_priority_str += std::to_string(pair.first) + ":" + std::to_string(pair.second) + ";";
        }
    }

    g_cell_equip_offset_str = "";
    for (const auto& kv : g_cell_equip_offset) {
        g_cell_equip_offset_str += std::to_string(kv.first) + ":" + std::to_string(kv.second.x) + "," + std::to_string(kv.second.y) + ";";
    }
    g_cell_sell_offset_str = "";
    for (const auto& kv : g_cell_sell_offset) {
        g_cell_sell_offset_str += std::to_string(kv.first) + ":" + std::to_string(kv.second.x) + "," + std::to_string(kv.second.y) + ";";
    }

    std::ofstream out(g_configPath);
    if (!out.is_open()) return;
    for (const auto& entry : g_configTable) {
        out << entry.key << "=";
        if (entry.type == C_BOOL) out << (*(bool*)entry.ptr);
        else if (entry.type == C_INT) out << (*(int*)entry.ptr);
        else if (entry.type == C_FLOAT) out << (*(float*)entry.ptr);
        else if (entry.type == C_STRING) out << (*(std::string*)entry.ptr);
        out << "\n";
    }
    out.close();
}

// =================================================================
// 自适应分辨率：自动穿/拆装备主干逻辑 (多维排序)
// =================================================================
void ExecuteMainThreadAutoEquip() {
    auto snap = GetSnapshot();
    if (!snap) return;

    int cooldown_ms = g_swipe_down_delay_ms + (g_swipe_steps * g_swipe_step_delay_ms) + g_swipe_up_delay_ms + g_equip_extra_cooldown_ms;
    
    static auto last_swipe_time = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_swipe_time).count() < cooldown_ms) {
        return; 
    }

    static auto equip_idle_start = std::chrono::steady_clock::now(); 
    
    static bool was_equip_active = false;
    if (g_auto_equip_active && !was_equip_active) {
        equip_idle_start = std::chrono::steady_clock::now(); 
    }
    was_equip_active = g_auto_equip_active;

    // --- 自动穿装备 ---
    if (g_auto_equip_active) {
        bool equipped_this_frame = false;
        int num_heroes = snap->myLiveHeroes.size();
        
        struct HeroEquipTarget {
            const LiveHeroData* hero;
            int priority;
            int cost;
            int star;
        };
        std::vector<HeroEquipTarget> sorted_targets;
        
        for (int i = 0; i < num_heroes; i++) {
            const auto& liveHero = snap->myLiveHeroes[i];
            
            if (liveHero.equipCount >= 9) continue; 

            int baseId, cost, star;
            FastParseHeroId(liveHero.heroId, baseId, cost, star);

            int prio = std::max(g_hero_equip_priority[liveHero.heroId], g_hero_equip_priority[baseId]);
            sorted_targets.push_back({&liveHero, prio, cost, star});
        }

        std::stable_sort(sorted_targets.begin(), sorted_targets.end(), [](const HeroEquipTarget& a, const HeroEquipTarget& b) {
            if (a.priority != b.priority) return a.priority > b.priority;
            if (a.cost != b.cost) return a.cost > b.cost;
            return a.star > b.star;
        });

        std::unordered_set<int> used_slots_this_frame;

        for (const auto& target : sorted_targets) {
            const auto& liveHero = *(target.hero);
            int heroId = liveHero.heroId;
            int baseId, dummyCost, dummyStar;
            FastParseHeroId(heroId, baseId, dummyCost, dummyStar);

            auto binds_it = g_hero_equip_bindings.find(heroId);
            if (binds_it == g_hero_equip_bindings.end()) {
                binds_it = g_hero_equip_bindings.find(baseId); 
            }
            
            if (binds_it != g_hero_equip_bindings.end()) {
                for (int targetEquip : binds_it->second) {
                    
                    int found_slot_index = -1;
                    for (size_t item_idx = 0; item_idx < snap->myItems.size(); ++item_idx) {
                        if (snap->myItems[item_idx] == targetEquip && used_slots_this_frame.find(item_idx) == used_slots_this_frame.end()) {
                            found_slot_index = item_idx;
                            break;
                        }
                    }
                    
                    if (found_slot_index != -1 && liveHero.screenPos.z > 0.01f) {
                        used_slots_this_frame.insert(found_slot_index); 
                        
                        int col = found_slot_index / 10;
                        int row = found_slot_index % 10;
                        
                        float startX = (g_equip_ui_start_x + col * g_equip_ui_step_x) * g_autoScale;
                        float startY = (g_equip_ui_start_y + row * g_equip_ui_step_y) * g_autoScale;
                        
                        if (g_cell_equip_offset.find(liveHero.cellIndex) == g_cell_equip_offset.end()) {
                            g_cell_equip_offset[liveHero.cellIndex] = ImVec2(0.0f, -80.0f);
                        }
                        ImVec2 off = g_cell_equip_offset[liveHero.cellIndex];
                        float endX = liveHero.screenPos.x + off.x * g_autoScale;
                        float endY = g_gl_height - liveHero.screenPos.y + off.y * g_autoScale;

                        if (endX > 0 && endY > 0 && endX < g_gl_width && endY < g_gl_height) {
                            AddLog("[智能穿装] 目标:%d Prio:%d 装备:%d 槽位:%d", baseId, target.priority, targetEquip, found_slot_index);
                            SimulateSwipe(startX, startY, endX, endY);
                            last_swipe_time = std::chrono::steady_clock::now();
                            equipped_this_frame = true;
                            break; 
                        }
                    }
                }
            }
            if (equipped_this_frame) break; 
        }
        
        if (equipped_this_frame) {
            equip_idle_start = std::chrono::steady_clock::now();
        } else {
            auto idle_duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - equip_idle_start).count();
            if (idle_duration > 1500) { 
                g_auto_equip_active = false;
                equip_idle_start = std::chrono::steady_clock::now();
                AddLog("[穿装结束] 所有优先级目标装备已处理完毕");
            }
        }
    }
    
    // --- 自动拆装备 ---
    if (g_auto_unequip_active) {
        bool swiped_this_frame = false;
        bool all_cleared = true;
        
        int col = g_unequip_slot_idx / 10;
        int row = g_unequip_slot_idx % 10;
        float startX = (g_equip_ui_start_x + col * g_equip_ui_step_x) * g_autoScale;
        float startY = (g_equip_ui_start_y + row * g_equip_ui_step_y) * g_autoScale;

        auto tryUnequipList = [&](const std::vector<LiveHeroData>& heroes) -> bool {
            for (const auto& hero : heroes) {
                if (hero.equipCount > 0 && hero.screenPos.z > 0.01f) {
                    all_cleared = false; 
                    
                    if (g_cell_equip_offset.find(hero.cellIndex) == g_cell_equip_offset.end()) {
                        g_cell_equip_offset[hero.cellIndex] = ImVec2(0.0f, -80.0f);
                    }
                    ImVec2 off = g_cell_equip_offset[hero.cellIndex];
                    float endX = hero.screenPos.x + off.x * g_autoScale;
                    float endY = g_gl_height - hero.screenPos.y + off.y * g_autoScale;

                    if (endX > 0 && endY > 0 && endX < g_gl_width && endY < g_gl_height) {
                        AddLog("[智能拆装] 目标英雄ID:%d 剩余装备:%d", hero.heroId, hero.equipCount);
                        SimulateSwipe(startX, startY, endX, endY);
                        last_swipe_time = std::chrono::steady_clock::now();
                        swiped_this_frame = true;
                        return true; 
                    }
                }
            }
            return false;
        };

        if (!tryUnequipList(snap->myLiveHeroes)) {
            tryUnequipList(snap->myBenchLiveHeroes);
        }
        
        if (!swiped_this_frame && all_cleared) {
            g_auto_unequip_active = false;
            AddLog("[拆装完成] 全场英雄装备已清空");
        }
    }
}

// =================================================================
// 胶囊窗口一键卖英雄核心逻辑
// =================================================================
void TriggerSellAllHeroes() {
    static std::atomic<bool> is_selling{false};
    if (is_selling.load()) return; 
    is_selling.store(true);

    std::thread([]() {
        int retries = 0;
        const int MAX_RETRIES = 8; 
        
        while (retries < MAX_RETRIES) {
            auto snap = GetSnapshot();
            if (!snap) break;

            std::vector<ImVec2> sell_positions;
            
            auto add_sell_pos = [&](const std::vector<LiveHeroData>& heroes) {
                for (const auto& hero : heroes) {
                    if (hero.screenPos.z > 0.01f) {
                        ImVec2 off(0.0f, -80.0f);
                        auto it = g_cell_sell_offset.find(hero.cellIndex);
                        if (it != g_cell_sell_offset.end()) off = it->second;
                        
                        float sx = hero.screenPos.x + off.x * g_autoScale;
                        float sy = g_gl_height - hero.screenPos.y + off.y * g_autoScale; 
                        sell_positions.push_back(ImVec2(sx, sy));
                    }
                }
            };

            add_sell_pos(snap->myBenchLiveHeroes);
            add_sell_pos(snap->myLiveHeroes);

            if (sell_positions.empty()) {
                if (retries > 0) AddLog("[一键卖牌] 场上已空，所有英雄成功售出");
                break;
            }

            if (retries == 0) AddLog("[一键卖牌] 开始极速出售所有 %d 个英雄", (int)sell_positions.size());
            else AddLog("[一键卖牌] 检测到漏卖，第 %d 次重试，剩余 %d 个", retries, (int)sell_positions.size());

            for (const auto& pos : sell_positions) {
                float endX = g_sell_drop_x * g_autoScale;
                float endY = g_sell_drop_y * g_autoScale;
                SimulateSwipe(pos.x, pos.y, endX, endY, true); 
                std::this_thread::sleep_for(std::chrono::milliseconds(70)); 
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(600)); 
            retries++;
        }
        
        if (retries >= MAX_RETRIES) {
            AddLog("[一键卖牌] 已达到最大重试次数，已停止。可能存在无法出售的单位(如假人)");
        }
        
        is_selling.store(false);
    }).detach();
}

extern "C" void hook_nativeInjectEvent(JNIEnv* env, jobject obj, jobject event) {
    if (!g_jvm) env->GetJavaVM(&g_jvm);
    
    if (obj) {
        if (!g_view_obj || !env->IsSameObject(g_view_obj, obj)) {
            if (g_view_obj) env->DeleteGlobalRef(g_view_obj);
            g_view_obj = env->NewGlobalRef(obj);
        }
        if (!g_context) {
            jclass viewClass = env->GetObjectClass(obj);
            jmethodID getContextMid = env->GetMethodID(viewClass, "getContext", "()Landroid/content/Context;");
            if (getContextMid) {
                jobject ctx = env->CallObjectMethod(obj, getContextMid);
                if (ctx) g_context = env->NewGlobalRef(ctx);
            }
            env->DeleteLocalRef(viewClass);
        }
    }

    if (event) {
        static jclass motionEventClassGlobal = nullptr;
        if (!motionEventClassGlobal) {
            jclass meClass = env->FindClass("android/view/MotionEvent");
            if (meClass) { motionEventClassGlobal = (jclass)env->NewGlobalRef(meClass); env->DeleteLocalRef(meClass); }
        }

        if (motionEventClassGlobal && env->IsInstanceOf(event, motionEventClassGlobal)) {
            static jmethodID getWidthMid = nullptr, getHeightMid = nullptr;
            static jmethodID getActionMid = nullptr, getXMid = nullptr, getYMid = nullptr;
            
            if (getWidthMid == nullptr) {
                jclass viewClass = env->GetObjectClass(obj);
                getWidthMid = env->GetMethodID(viewClass, "getWidth", "()I"); getHeightMid = env->GetMethodID(viewClass, "getHeight", "()I"); env->DeleteLocalRef(viewClass);
                getActionMid = env->GetMethodID(motionEventClassGlobal, "getAction", "()I"); getXMid = env->GetMethodID(motionEventClassGlobal, "getX", "()F"); getYMid = env->GetMethodID(motionEventClassGlobal, "getY", "()F"); 
            }
            
            if (getActionMid && getXMid && getYMid) {
                int action = env->CallIntMethod(event, getActionMid) & 255;
                if ((action == 0 && g_cached_view_width <= 0) || g_cached_view_width <= 0) { 
                    g_cached_view_width = env->CallIntMethod(obj, getWidthMid); 
                    g_cached_view_height = env->CallIntMethod(obj, getHeightMid); 
                }
                float raw_x = env->CallFloatMethod(event, getXMid), raw_y = env->CallFloatMethod(event, getYMid);
                float scale_x = 1.0f, scale_y = 1.0f;
                if (g_cached_view_width > 0 && g_gl_width > 0) scale_x = (float)g_gl_width / g_cached_view_width;
                if (g_cached_view_height > 0 && g_gl_height > 0) scale_y = (float)g_gl_height / g_cached_view_height;
                
                ImGuiIO& io = ImGui::GetIO(); io.AddMousePosEvent(raw_x * scale_x, raw_y * scale_y);
                if (action == 0) io.AddMouseButtonEvent(0, true); else if (action == 1) io.AddMouseButtonEvent(0, false);
                if (io.WantCaptureMouse) return;
            }
        }
    }
    if (old_nativeInjectEvent) old_nativeInjectEvent(env, obj, event);
}

void FindAndHookHiddenJNI() {
    FILE* fp = fopen("/proc/self/maps", "r"); if (!fp) return;
    char line[1024]; struct MemRegion { uintptr_t start, end; bool is_rw; }; std::vector<MemRegion> regions;
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, "libunity.so")) {
            bool is_r = strstr(line, "r-") != nullptr; bool is_rw = strstr(line, "rw") != nullptr;
            if (is_r || is_rw) { uintptr_t start, end; sscanf(line, "%lx-%lx", &start, &end); regions.push_back({start, end, is_rw}); }
        }
    }
    fclose(fp);

    const char* target_string = "nativeInjectEvent"; std::vector<uintptr_t> string_addrs;
    for (const auto& reg : regions) {
        if (!reg.is_rw) {
            for (uintptr_t p = reg.start; p < reg.end - strlen(target_string); p++) {
                if (memcmp((void*)p, target_string, strlen(target_string)) == 0) string_addrs.push_back(p);
            }
        }
    }
    if (string_addrs.empty()) return;

    bool found_func = false;
    for (const auto& reg : regions) {
        uintptr_t align_start = (reg.start + 7) & ~7;
        for (uintptr_t p = align_start; p < reg.end - sizeof(void*)*3; p += sizeof(void*)) {
            uintptr_t ptr_val = *(uintptr_t*)p;
            for (uintptr_t str_addr : string_addrs) {
                if (ptr_val == str_addr) {
                    void** fnPtr_addr = (void**)(p + 16); void* real_function_addr = *fnPtr_addr;
                    if (real_function_addr != nullptr && (uintptr_t)real_function_addr > 0x100000) {
                        DobbyHook(real_function_addr, (void*)hook_nativeInjectEvent, (void**)&old_nativeInjectEvent);
                        found_func = true; break;
                    }
                }
            }
            if (found_func) break;
        }
        if (found_func) break;
    }
}

typedef int (*func_3Cd6058_t)(void* playerObj); func_3Cd6058_t orig_3Cd6058 = nullptr;
typedef void (*func_set_Money_t)(void* playerObj, int money); func_set_Money_t orig_set_Money = nullptr;
typedef void (*func_set_Level_t)(void* playerObj, int level); func_set_Level_t orig_set_Level = nullptr;
typedef void (*func_TurnStart_t)(void* thisObj); func_TurnStart_t orig_TurnStart = nullptr;
typedef void (*func_set_IsGameEnd_t)(void* thisObj, uint8_t isEnd); func_set_IsGameEnd_t orig_set_IsGameEnd = nullptr;
typedef void (*func_UpdateTurnStart_t)(void* arg0); func_UpdateTurnStart_t orig_UpdateTurnStart = nullptr;
typedef int (*func_InitTurnDropCfg_t)(void* obj, int index); func_InitTurnDropCfg_t orig_InitTurnDropCfg = nullptr;

// === 大量使用 g_is_in_match 保护机制，防止大厅/加载界面卡死 ===
int hook_3Cd6058(void* playerObj) {
    if (!g_is_in_match.load(std::memory_order_relaxed)) return orig_3Cd6058(playerObj);
    g_debug_hook_3Cd6058_count++; 
    int rowIdx = orig_3Cd6058(playerObj);
    if (playerObj && IsValidPtr((uintptr_t)playerObj)) {
        g_Tasks.player_row_queue.push({(uintptr_t)playerObj, rowIdx});
    }
    return rowIdx;
}

void hook_set_Money(void* playerObj, int money) {
    orig_set_Money(playerObj, money);
    if (!g_is_in_match.load(std::memory_order_relaxed)) return;
    g_debug_hook_set_Money_count++; 
    if (playerObj && IsValidPtr((uintptr_t)playerObj)) {
        g_Tasks.money_queue.push({(uintptr_t)playerObj, money});
    }
}

void hook_set_Level(void* playerObj, int level) {
    orig_set_Level(playerObj, level);
    if (!g_is_in_match.load(std::memory_order_relaxed)) return;
    g_debug_hook_set_Level_count++; 
    if (playerObj && IsValidPtr((uintptr_t)playerObj)) {
        g_Tasks.level_queue.push({(uintptr_t)playerObj, level});
    }
}

void hook_TurnStart(void* thisObj) {
    orig_TurnStart(thisObj);
    if (!g_is_in_match.load(std::memory_order_relaxed)) return;
    g_debug_TurnStartCalled++; 
    if (thisObj) g_Tasks.pending_turn_start_obj.store((uintptr_t)thisObj, std::memory_order_release);
}

// 【修复 1】：权威状态接管
void hook_set_IsGameEnd(void* thisObj, uint8_t isEnd) {
    orig_set_IsGameEnd(thisObj, isEnd);
    g_debug_set_IsGameEnd_count++;

    if (isEnd == 0) { 
        if (!g_is_in_match.load(std::memory_order_acquire)) {
            AddLog("[状态监控] 引擎触发 isGameEnd = 0, 正式进入对局！");
            g_is_in_match.store(true, std::memory_order_release);
        }
    } 
    else { 
        if (g_is_in_match.load(std::memory_order_acquire)) {
            AddLog("[状态监控] 引擎触发 isGameEnd = %d, 退出对局，清理残余数据！", isEnd);
            g_is_in_match.store(false, std::memory_order_release);
            g_Tasks.trigger_game_end.store(true, std::memory_order_release);
        }
    }
}

void hook_UpdateTurnStart(void* arg0) {
    orig_UpdateTurnStart(arg0);
    if (!g_is_in_match.load(std::memory_order_relaxed)) return;
    if (g_collected_all_players.load(std::memory_order_relaxed)) return;
    g_debug_UpdateTurnStart_count++; 
    if (arg0 && IsValidPtr((uintptr_t)arg0)) {
        g_Tasks.update_turn_start_queue.push((uintptr_t)arg0);
    }
}

int hook_InitTurnDropCfg(void* obj, int index) {
    if (!g_is_in_match.load(std::memory_order_relaxed)) return orig_InitTurnDropCfg(obj, index);
    g_debug_hook_InitTurnDropCfg_count++;
    int ret = orig_InitTurnDropCfg(obj, index);
    if (obj && IsValidPtr((uintptr_t)obj)) g_Tasks.pending_hex_cfg_obj.store((uintptr_t)obj, std::memory_order_release);
    return ret; 
}

typedef void* (*func_46cce08_t)(void* x0, void* x1, void* x2, void* x3);
func_46cce08_t orig_46cce08 = nullptr;

void* hook_46cce08(void* x0, void* arg1, void* arg2, void* arg3) {
    if (!g_is_in_match.load(std::memory_order_relaxed)) return orig_46cce08(x0, arg1, arg2, arg3);
    if (g_shop_slot_index < 5 && x0 != nullptr) {
        uintptr_t ptr_x0 = (uintptr_t)x0;
        bool already_exists = false;
        for (int i = 0; i < g_shop_slot_index; i++) {
            if (g_shop_slot_instances[i] == ptr_x0) { already_exists = true; break; }
        }
        if (!already_exists) { g_shop_slot_instances[g_shop_slot_index++] = ptr_x0; }
    }
    return orig_46cce08(x0, arg1, arg2, arg3);
}

static auto g_last_buy_request_time = std::chrono::steady_clock::now() - std::chrono::hours(1);

void ExecuteMainThreadAutoRefresh() {
    if (!g_auto_refresh || g_refresh_instance == 0 || g_req_refresh == nullptr) return;
    static auto last_refresh_time = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    
    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - g_last_buy_request_time).count() < 300) return;
    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_refresh_time).count() < g_auto_buy_delay_ms) return; 

    auto snap = GetSnapshot();
    int myMoney = 0;
    bool has_target_in_shop = false;
    
    auto it = snap->players.find(snap->myPlayerId);
    if (it != snap->players.end()) myMoney = it->second.money;
    
    for (const auto& shopHero : snap->myShopHeroes) {
        if (g_auto_buy_heroes.count(shopHero.baseHeroId) || g_auto_buy_heroes.count(shopHero.heroId)) {
            if (shopHero.starLevel == 2 && !g_auto_buy_chosen) continue;
            has_target_in_shop = true;
            break;
        }
    }

    if (has_target_in_shop || myMoney < g_auto_refresh_money_threshold) {
        if (myMoney < g_auto_refresh_money_threshold) g_auto_refresh = false;
        return;
    }

    try {
        g_req_refresh((void*)g_refresh_instance);
        last_refresh_time = now;
    } catch (...) {}
}

void ExecuteMainThreadAutoBuy() {
    if (!g_auto_buy || g_reqbuyhero == nullptr) return;

    auto snap = GetSnapshot();
    if (g_auto_buy_heroes.empty()) return;

    uintptr_t myObj = 0;
    auto it = snap->players.find(snap->myPlayerId);
    if (it != snap->players.end()) myObj = it->second.objAddr;
    if (!myObj || !IsValidPtr(myObj)) return;

    uintptr_t shopEntries = getEntriesDynamically(myObj, g_off_shop_base);
    if (!IsValidPtr(shopEntries)) return;

    int count = SAFE_READ(int, shopEntries, g_off_entries_count, 0);
    int maxIter = std::clamp(count, 0, 5);

    for (int i = 0; i < maxIter; i++) {
        uintptr_t entryStart = shopEntries + g_off_entries_head + i * g_off_entries_step;
        uintptr_t value = SAFE_READ_PTR(entryStart, g_off_entry_value);
        if (!value) continue;

        int slot = SAFE_READ(int, value, g_off_value_slot, -1);
        if (slot >= 0 && slot < 5) {
            uintptr_t p10 = SAFE_READ_PTR(value, g_off_value_p10);
            if (p10) {
                int heroId = SAFE_READ(int, p10, g_off_p10_heroId, 0);
                if (heroId > 0) {
                    int baseId, cost, star;
                    FastParseHeroId(heroId, baseId, cost, star);

                    if (star == 2 && !g_auto_buy_chosen) continue;

                    bool is_recommended = g_auto_buy_heroes.count(baseId) || g_auto_buy_heroes.count(heroId);

                    if (is_recommended) {
                        uintptr_t target_x0 = g_shop_slot_instances[slot];
                        if (target_x0 != 0) {
                            try {
                                g_reqbuyhero((void*)target_x0, nullptr, nullptr, nullptr);
                                g_reqbuyhero((void*)target_x0, nullptr, nullptr, nullptr);
                                g_reqbuyhero((void*)target_x0, nullptr, nullptr, nullptr);
                                g_reqbuyhero((void*)target_x0, nullptr, nullptr, nullptr);
                                g_reqbuyhero((void*)target_x0, nullptr, nullptr, nullptr);
                                g_last_buy_request_time = std::chrono::steady_clock::now(); 
                            } catch (...) {}
                        }
                    }
                }
            }
        }
    }
}

void ProcessMainThreadTasks() {
    if (g_Tasks.trigger_game_end.exchange(false, std::memory_order_acquire)) {
        g_BackendData = GameSnapshot(); 
        PushSnapshot(); 
        PushSnapshot(); 

        g_refresh_instance = 0; g_turnStartThisObj = nullptr; g_hexCfgObj = nullptr;                                
        g_pool_entries1.store(0, std::memory_order_release); 
        g_debug_hook_6A67e48_count = 0; 
        g_matchFetchRequested.store(false, std::memory_order_relaxed);
        g_matchFetchReady.store(false, std::memory_order_relaxed);
        g_collected_all_players.store(false, std::memory_order_relaxed);
        g_debug_collected_players_count = 0; g_debug_UpdateTurnStart_count = 0;
        
        g_shop_slot_index = 0;
        for (int i = 0; i < 5; i++) g_shop_slot_instances[i] = 0;
        g_Tasks.pending_hex_cfg_obj.store(0); g_Tasks.processed_hex_cfg_obj = 0;
        g_Tasks.pending_turn_start_obj.store(0); 

        g_Tasks.player_row_queue.clear();
        g_Tasks.money_queue.clear();
        g_Tasks.level_queue.clear();
        g_Tasks.update_turn_start_queue.clear();
        
        std::lock_guard<std::mutex> lock(g_swipe_mutex);
        g_active_swipes.clear();
    }

    uintptr_t tObj = g_Tasks.pending_turn_start_obj.exchange(0, std::memory_order_acquire);
    if (tObj != 0) {
        g_turnStartThisObj = (void*)tObj;
        if (!g_matchFetchRequested.load(std::memory_order_acquire)) {
            g_matchFetchTime = std::chrono::steady_clock::now() + std::chrono::seconds(1);
            g_matchFetchRequested.store(true, std::memory_order_release);
        }
    }

    uintptr_t hexObj = g_Tasks.pending_hex_cfg_obj.load(std::memory_order_acquire);
    if (hexObj != 0 && hexObj != g_Tasks.processed_hex_cfg_obj) {
        g_Tasks.processed_hex_cfg_obj = hexObj;
        g_hexCfgObj = (void*)hexObj; 
        
        g_BackendData.hex_fetched = false; 
        for(int i=0; i<4; i++) g_BackendData.cachedHexes[i] = 0;
        g_BackendData.validHexCount = 0;
    }

    uintptr_t pObj;
    while (g_Tasks.update_turn_start_queue.pop(pObj)) {
        bool isAi = SAFE_READ(bool, pObj, 0x58, false);
        uintptr_t dynAddrs = SAFE_READ_PTR(pObj, 0x50);
        if (IsValidPtr(dynAddrs)) {
            int pid = SAFE_READ(int, dynAddrs, 0x20, -1);
            if (pid != -1) {
                if (g_BackendData.players[pid].logicObjAddr != pObj) {
                    g_BackendData.players[pid].logicObjAddr = pObj;
                    g_BackendData.players[pid].dynamicAddr = dynAddrs;
                    g_BackendData.players[pid].isAi = isAi;
                    g_BackendData.players[pid].heroArrayAddr = 0; 
                }
            } 
        }
        int valid_count = 0;
        for (const auto& pair : g_BackendData.players) if (pair.second.logicObjAddr != 0) valid_count++;
        g_debug_collected_players_count = valid_count;
        if (valid_count >= 8) g_collected_all_players.store(true, std::memory_order_relaxed);
    }

    PlayerRowInfo rowItem;
    while (g_Tasks.player_row_queue.pop(rowItem)) {
        uintptr_t objAddr = rowItem.obj;
        int rowIdx = rowItem.rowIdx;
        uintptr_t tempCsoGame = SAFE_READ_PTR(objAddr, 0x30);
        int playerId = SAFE_READ(int, objAddr, 0x258, -1);
        if (playerId == -1) continue;

        // 【修复 3：跨局野指针终极清空保护】
        if (g_BackendData.csoGame != 0 && tempCsoGame != 0 && IsValidPtr(tempCsoGame) && g_BackendData.csoGame != tempCsoGame) {
            AddLog("[系统] 检测到新的一局游戏，强行清空上一局残留数据！");
            g_Tasks.trigger_game_end.store(true, std::memory_order_release);
            return; 
        }

        bool needFetchName = false;
        if (g_BackendData.csoGame == 0 && IsValidPtr(tempCsoGame)) g_BackendData.csoGame = tempCsoGame;
        auto it = g_BackendData.players.find(playerId);
        
        if (it == g_BackendData.players.end() || it->second.objAddr != objAddr) {
            PlayerInfo& info = g_BackendData.players[playerId];
            info.objAddr = objAddr; info.csoGameAddr = tempCsoGame; info.playerId = playerId; info.rowIdx = rowIdx;
            if(it == g_BackendData.players.end()) info.name = "Unknown";
            g_BackendData.objToPlayerId[objAddr] = playerId; needFetchName = true;
        } else {
            it->second.rowIdx = rowIdx; it->second.csoGameAddr = tempCsoGame; 
            if (it->second.name == "Unknown" || it->second.name.empty()) needFetchName = true;
        }
        
        if (needFetchName) {
            std::string name = "未知";
            uintptr_t midAddr = SAFE_READ_PTR(objAddr, 0x268);
            uintptr_t strPtr = 0; int nameLen = 0;
            if (IsValidPtr(midAddr)) {
                strPtr = SAFE_READ_PTR(midAddr, 0x28);
                if (IsValidPtr(strPtr)) {
                    nameLen = SAFE_READ(int, strPtr, 0x10, 0);
                    if (nameLen > 0 && nameLen < 64) { 
                        char16_t chars[64] = {0}; 
                        if (SafeReadMemory(strPtr + 0x14, chars, nameLen * sizeof(char16_t))) name = Utf16ToUtf8(chars, nameLen);
                    }
                }
            }
            auto it2 = g_BackendData.players.find(playerId);
            if (it2 != g_BackendData.players.end()) {
                it2->second.name = name; it2->second.midAddr = midAddr;   
                it2->second.strPtr = strPtr; it2->second.nameLen = nameLen;   
            }
        }
    }

    PlayerStatInfo statItem;
    while (g_Tasks.money_queue.pop(statItem)) {
        auto it = g_BackendData.objToPlayerId.find(statItem.obj);
        if (it != g_BackendData.objToPlayerId.end()) g_BackendData.players[it->second].money = statItem.val;
    }
    while (g_Tasks.level_queue.pop(statItem)) {
        auto it = g_BackendData.objToPlayerId.find(statItem.obj);
        if (it != g_BackendData.objToPlayerId.end()) g_BackendData.players[it->second].level = statItem.val;
    }
}

void BackgroundDataUpdateThread() {
    while (!g_il2cpp_domain_get || !g_il2cpp_thread_attach || !g_il2cpp_thread_detach) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    void* domain = g_il2cpp_domain_get();

    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(30)); 
        g_debug_bg_thread_count++; 

        if (g_il2cppBase == 0) continue;

        // 【修复 2】：完全信任 hook_set_IsGameEnd 的判断，不再主动试探引发闪退！
        if (!g_is_in_match.load(std::memory_order_acquire)) {
            ProcessMainThreadTasks();
            std::this_thread::sleep_for(std::chrono::milliseconds(200)); 
            continue; 
        }

        void* current_thread_handle = g_il2cpp_thread_attach(domain);

        EnsureChessBattleModel();
        ProcessMainThreadTasks();

        if (g_matchFetchRequested.load(std::memory_order_acquire)) {
            if (std::chrono::steady_clock::now() >= g_matchFetchTime) {
                g_matchFetchRequested.store(false, std::memory_order_relaxed);
                g_matchFetchReady.store(true, std::memory_order_release);
            }
        }
        if (g_matchFetchReady.load(std::memory_order_acquire)) {
            g_matchFetchReady.store(false, std::memory_order_relaxed); 
            CheckReadyMatchFetch();
        }

        if (g_predict_hex && g_hexCfgObj != nullptr && !g_BackendData.hex_fetched) {
            typedef int (*func_GetNextHexLevel_t)(void* obj, int index);
            func_GetNextHexLevel_t GetNextHexLevel = (func_GetNextHexLevel_t)(g_il2cppBase + 0x4446EDc);
            
            if (GetNextHexLevel) {
                int tempHexes[5] = {0}, verifyHexes[5] = {0};
                int checkCount = 5; 
                
                for (int i = 0; i < checkCount; i++) tempHexes[i] = GetNextHexLevel(g_hexCfgObj, i);
                for (int i = 0; i < checkCount; i++) verifyHexes[i] = GetNextHexLevel(g_hexCfgObj, i);

                bool match = true;
                int validCount = 0;

                for (int i = 0; i < checkCount; i++) {
                    if (tempHexes[i] != verifyHexes[i]) {
                        match = false; 
                        break;
                    }
                    if (tempHexes[i] >= 1 && tempHexes[i] <= 3) {
                        validCount++;
                    }
                }

                if (!match) {
                    g_BackendData.validHexCount = 0;
                }
                else if (validCount > 0) {
                    int saveCount = (validCount > 4) ? 4 : validCount;
                    for (int i = 0; i < saveCount; i++) {
                        g_BackendData.cachedHexes[i] = tempHexes[i];
                    }
                    g_BackendData.validHexCount = saveCount; 
                    g_BackendData.hex_fetched = true;
                }
            }
        }

        UpdateShopAndBenchData();
        UpdateCardPoolData(); 
        if (g_predict_enemy) UpdateEnemyBoardState(); 
        UpdateWarningData();
        UpdatePlayerStreaks(); 
        
        UpdateEquipAndLiveHeroes();

        PushSnapshot();

        ExecuteMainThreadAutoEquip();

        if (g_Tasks.trigger_surrender.load(std::memory_order_acquire)) {
            void* turnObj = g_turnStartThisObj; 
            int myId = g_BackendData.myPlayerId;
            if (g_il2cppBase != 0 && turnObj != nullptr && myId != -1) {
                if (IsValidPtr((uintptr_t)turnObj)) {
                    typedef void (*func_UserExit_t)(void* thisObj, int playerId, int surrenderFlag);
                    func_UserExit_t doUserExit = (func_UserExit_t)(g_il2cppBase + 0x3da60c0);
                    try { doUserExit(turnObj, myId, 1); } catch (...) {}
                }
            }
            g_Tasks.trigger_surrender.store(false, std::memory_order_release);
            g_Tasks.trigger_game_end.store(true, std::memory_order_release);
        }

        g_il2cpp_thread_detach(current_thread_handle);
    }
}

typedef void (*f_SendWillRenderCanvases)();
f_SendWillRenderCanvases orig_SendWillRenderCanvases = nullptr;

void hook_SendWillRenderCanvases() {
    if (orig_SendWillRenderCanvases) orig_SendWillRenderCanvases();
    if (!g_is_in_match.load(std::memory_order_relaxed)) return;
    
    g_debug_SendWillRenderCanvases_count++;
    ExecuteMainThreadAutoBuy();
    ExecuteMainThreadAutoRefresh();
}

typedef void* (*func_4848cec_t)(void* x0, void* x1, void* x2, void* x3);
func_4848cec_t orig_4848cec = nullptr;

void* hook_4848cec(void* x0, void* arg1, void* arg2, void* arg3) {
    if (!g_is_in_match.load(std::memory_order_relaxed)) return orig_4848cec(x0, arg1, arg2, arg3);
    if (x0 != nullptr) g_refresh_instance = (uintptr_t)x0;
    return orig_4848cec(x0, arg1, arg2, arg3);
}

void InitBusinessHooks() {
    while (g_il2cppBase == 0) {
        FILE *fp = fopen("/proc/self/maps", "r");
        if (fp) {
            char line[512];
            while (fgets(line, sizeof(line), fp)) {
                if (strstr(line, "libil2cpp.so") && strstr(line, "r-xp")) {
                    sscanf(line, "%lx", &g_il2cppBase); break;
                }
            }
            fclose(fp);
        }
        if (g_il2cppBase == 0) sleep(1);
    }

    void* handle = dlopen("libil2cpp.so", RTLD_LAZY);
    if (handle) {
        g_il2cpp_domain_get = (il2cpp_domain_get_t)dlsym(handle, "il2cpp_domain_get");
        g_il2cpp_thread_attach = (il2cpp_thread_attach_t)dlsym(handle, "il2cpp_thread_attach");
        g_il2cpp_thread_detach = (il2cpp_thread_detach_t)dlsym(handle, "il2cpp_thread_detach");
        g_il2cpp_string_new = (il2cpp_string_new_t)dlsym(handle, "il2cpp_string_new"); 
    }

    DobbyHook((void*)(g_il2cppBase + 0x5277718), (void*)hook_LoadAsset_718, (void**)&orig_LoadAsset_718);
    DobbyHook((void*)(g_il2cppBase + 0x470f4b0), (void*)hook_LoadMapImpl, (void**)&orig_LoadMapImpl);

    DobbyHook((void*)(g_il2cppBase + 0x46cce08), (void*)hook_46cce08, (void**)&orig_46cce08);
    g_reqbuyhero = (func_reqbuyhero_t)(g_il2cppBase + 0x46d6e94);
    DobbyHook((void*)(g_il2cppBase + 0x4848cec), (void*)hook_4848cec, (void**)&orig_4848cec);
    g_req_refresh = (func_req_refresh_t)(g_il2cppBase + 0x483fce4);
    DobbyHook((void*)(g_il2cppBase + 0xAE25E80), (void*)hook_SendWillRenderCanvases, (void**)&orig_SendWillRenderCanvases);
    DobbyHook((void*)(g_il2cppBase + 0x3Cd6058), (void*)hook_3Cd6058, (void**)&orig_3Cd6058);
    DobbyHook((void*)(g_il2cppBase + 0x3cd3f80), (void*)hook_set_Money, (void**)&orig_set_Money);
    DobbyHook((void*)(g_il2cppBase + 0x3cd2d18), (void*)hook_set_Level, (void**)&orig_set_Level);
    DobbyHook((void*)(g_il2cppBase + 0x3d7c338), (void*)hook_TurnStart, (void**)&orig_TurnStart); 
    DobbyHook((void*)(g_il2cppBase + 0x3fc3c70), (void*)hook_set_IsGameEnd, (void**)&orig_set_IsGameEnd); 
    DobbyHook((void*)(g_il2cppBase + 0x3f7f704), (void*)hook_UpdateTurnStart, (void**)&orig_UpdateTurnStart); 
    DobbyHook((void*)(g_il2cppBase + 0x4440520), (void*)hook_InitTurnDropCfg, (void**)&orig_InitTurnDropCfg);
    DobbyHook((void*)(g_il2cppBase + 0x6A67e48), (void*)hook_6A67e48, (void**)&orig_6A67e48);
    
    std::thread(BackgroundDataUpdateThread).detach();
}

void LoadConfig() {
    std::ifstream in(g_configPath);
    if (!in.is_open()) return;
    std::string line;
    while (std::getline(in, line)) {
        size_t pos = line.find('=');
        if (pos == std::string::npos) continue; 
        std::string k = line.substr(0, pos), v = line.substr(pos + 1);
        if (k == "playersX") { g_level_X = std::stof(v); g_warn_X = std::stof(v) + 150.0f * g_autoScale; continue; }
        if (k == "playersY") { g_level_Y = std::stof(v); g_warn_Y = std::stof(v); continue; }
        for (const auto& entry : g_configTable) {
            if (k == entry.key) {
                try {
                    if (entry.type == C_BOOL) *(bool*)entry.ptr = (v == "1");
                    else if (entry.type == C_INT) *(int*)entry.ptr = std::stoi(v);
                    else if (entry.type == C_FLOAT) *(float*)entry.ptr = std::stof(v);
                    else if (entry.type == C_STRING) *(std::string*)entry.ptr = v;
                } catch (...) {} 
                break;
            }
        }
    }
    in.close();

    g_auto_buy_heroes.clear();
    std::stringstream ss(g_auto_buy_heroes_str);
    std::string item;
    while (std::getline(ss, item, ',')) {
        if (!item.empty()) { try { g_auto_buy_heroes.insert(std::stoi(item)); } catch (...) {} }
    }

    g_hero_equip_bindings.clear();
    std::stringstream ss_binds(g_hero_equip_bindings_str);
    std::string item_bind;
    while(std::getline(ss_binds, item_bind, ';')) {
        if(item_bind.empty()) continue;
        size_t colon_pos = item_bind.find(':');
        if(colon_pos != std::string::npos) {
            try {
                int hId = std::stoi(item_bind.substr(0, colon_pos));
                std::string eqs = item_bind.substr(colon_pos + 1);
                std::stringstream ss_eqs(eqs);
                std::string eq;
                while(std::getline(ss_eqs, eq, ',')) {
                    if(!eq.empty()) g_hero_equip_bindings[hId].push_back(std::stoi(eq));
                }
            } catch (...) {}
        }
    }

    g_hero_equip_priority.clear();
    std::stringstream ss_prio(g_hero_equip_priority_str);
    std::string item_prio;
    while(std::getline(ss_prio, item_prio, ';')) {
        if(item_prio.empty()) continue;
        size_t colon_pos = item_prio.find(':');
        if(colon_pos != std::string::npos) {
            try {
                int hId = std::stoi(item_prio.substr(0, colon_pos));
                int prio = std::stoi(item_prio.substr(colon_pos + 1));
                g_hero_equip_priority[hId] = prio;
            } catch (...) {}
        }
    }

    // [格子独立记录] 坐标读取
    g_cell_equip_offset.clear();
    std::stringstream ss_eqOff(g_cell_equip_offset_str);
    std::string item_eqOff;
    while(std::getline(ss_eqOff, item_eqOff, ';')) {
        if(item_eqOff.empty()) continue;
        size_t colon = item_eqOff.find(':');
        size_t comma = item_eqOff.find(',');
        if(colon != std::string::npos && comma != std::string::npos) {
            try {
                int cIdx = std::stoi(item_eqOff.substr(0, colon));
                float x = std::stof(item_eqOff.substr(colon + 1, comma - colon - 1));
                float y = std::stof(item_eqOff.substr(comma + 1));
                g_cell_equip_offset[cIdx] = ImVec2(x, y);
            } catch(...) {}
        }
    }
    
    g_cell_sell_offset.clear();
    std::stringstream ss_sellOff(g_cell_sell_offset_str);
    std::string item_sellOff;
    while(std::getline(ss_sellOff, item_sellOff, ';')) {
        if(item_sellOff.empty()) continue;
        size_t colon = item_sellOff.find(':');
        size_t comma = item_sellOff.find(',');
        if(colon != std::string::npos && comma != std::string::npos) {
            try {
                int cIdx = std::stoi(item_sellOff.substr(0, colon));
                float x = std::stof(item_sellOff.substr(colon + 1, comma - colon - 1));
                float y = std::stof(item_sellOff.substr(comma + 1));
                g_cell_sell_offset[cIdx] = ImVec2(x, y);
            } catch(...) {}
        }
    }

    g_anim[1] = g_predict_enemy ? 1.0f : 0.0f; g_anim[2] = g_predict_hex ? 1.0f : 0.0f;
    g_anim[3] = g_esp_board ? 1.0f : 0.0f; 
    g_anim[5] = g_esp_enemy_bench ? 1.0f : 0.0f; 
    g_anim[7] = g_show_instant_quit_window ? 1.0f : 0.0f;
    g_anim[8] = g_boardLocked ? 1.0f : 0.0f; g_anim[9] = g_esp_level ? 1.0f : 0.0f;
    g_anim[10] = g_show_card_pool ? 1.0f : 0.0f; g_anim[11] = g_card_warning ? 1.0f : 0.0f;
    g_anim[12] = g_show_debug_window ? 1.0f : 0.0f; g_anim[17] = g_pool_warning_enable ? 1.0f : 0.0f;
    g_anim[18] = g_enable_skin ? 1.0f : 0.0f; 
    g_anim[19] = g_enable_map_skin ? 1.0f : 0.0f; 
    g_anim[20] = g_show_hero_list_window ? 1.0f : 0.0f;
    g_anim[23] = g_show_sell_window ? 1.0f : 0.0f;
    g_anim[24] = g_show_clear_bench_btn ? 1.0f : 0.0f; 
    g_anim[25] = g_show_equip_ui_debug ? 1.0f : 0.0f;
    g_anim[26] = g_show_sell_ui_debug ? 1.0f : 0.0f;
    g_needUpdateFontSafe = true; 
}

// =================================================================
// 绘制基础 UI 模块
// =================================================================
void UpdateFontHD(bool force = false) {
    ImGuiIO& io = ImGui::GetIO();
    float screenH = (io.DisplaySize.y > 100.0f) ? io.DisplaySize.y : 2400.0f; 
    g_autoScale = screenH / 1080.0f;
    float targetSize = std::clamp(20.0f * g_autoScale, 16.0f, 45.0f); 
    if (!force && std::abs(targetSize - g_current_rendered_size) < 2.0f) return;
    
    ImGui_ImplOpenGL3_DestroyDeviceObjects(); 
    io.Fonts->Clear(); 
    g_mainFont = nullptr; 
    g_hugeNumFont = nullptr;
    
    ImFontConfig configMain; configMain.OversampleH = 2; configMain.OversampleV = 2; configMain.PixelSnapH = false; 
    ImFontConfig configNum; configNum.OversampleH = 2; configNum.OversampleV = 2; configNum.PixelSnapH = false;
    
    float mainResFactor = 1.5f; 
    float numResFactor = (targetSize * 4.0f > 200.0f) ? (200.0f / targetSize) : 4.0f; 
    static const ImWchar numRanges[] = { 0x0020, 0x00FF, 0 }; 
    const char* fonts[] = { "/system/fonts/Miui-Regular.ttf", "/system/fonts/SysSans-Hans-Regular.ttf", "/system/fonts/NotoSansCJK-Regular.ttc", "/system/fonts/DroidSansFallback.ttf" };
    
    bool loaded = false;
    for(const char* path : fonts) {
        if (access(path, R_OK) == 0) { 
            g_mainFont = io.Fonts->AddFontFromFileTTF(path, targetSize * mainResFactor, &configMain, io.Fonts->GetGlyphRangesChineseSimplifiedCommon()); 
            if (g_mainFont) g_mainFont->Scale = 1.0f / mainResFactor;
            g_hugeNumFont = io.Fonts->AddFontFromFileTTF(path, targetSize * numResFactor, &configNum, numRanges);
            if (g_hugeNumFont) g_hugeNumFont->Scale = 1.0f / numResFactor;
            loaded = true; 
            break; 
        }
    }
    if(!loaded || !g_mainFont) { 
        g_mainFont = io.Fonts->AddFontDefault(); 
        if (g_mainFont) g_mainFont->Scale = 1.0f / mainResFactor; 
    }
    io.Fonts->Build(); 
    ImGui_ImplOpenGL3_CreateDeviceObjects(); 
    g_current_rendered_size = targetSize;
}

void HandleGridInteractionCore(
    float& out_x, float& out_y, float& out_scaleX, float& out_scaleY, float& t_x, float& t_y, float& t_scaleX, float& t_scaleY,
    bool& isDragging, bool& isScaling, ImVec2& dragOffset, ImVec2& scaleDragOffset,
    float h_dx, float h_dy, float c_dx, float c_dy, float hitMinX, float hitMinY, float hitMaxX, float hitMaxY, 
    bool locked, bool* isOpen, bool enableScale, bool uniformScale) 
{
    ImGuiIO& io = ImGui::GetIO();
    if (!locked) {
        ImVec2 p_scale(out_x + h_dx * out_scaleX, out_y + h_dy * out_scaleY);
        ImVec2 p_close(out_x + c_dx * out_scaleX, out_y + c_dy * out_scaleY);
        if (ImGui::IsMouseClicked(0)) {
            if (isOpen && ImLengthSqr(io.MousePos - p_close) < (1600.0f * g_autoScale * g_autoScale)) { *isOpen = false; return; }
            else if (enableScale && ImLengthSqr(io.MousePos - p_scale) < (400.0f * g_autoScale * g_autoScale)) { 
                isScaling = true; scaleDragOffset = io.MousePos - ImVec2(t_x + h_dx * t_scaleX, t_y + h_dy * t_scaleY);
            } 
            else if (ImRect(ImVec2(out_x + hitMinX * out_scaleX, out_y + hitMinY * out_scaleY), ImVec2(out_x + hitMaxX * out_scaleX, out_y + hitMaxY * out_scaleY)).Contains(io.MousePos)) {
                isDragging = true; dragOffset = ImVec2(t_x - io.MousePos.x, t_y - io.MousePos.y);
            }
        }
        if (isScaling && enableScale) {
            if (ImGui::IsMouseDown(0)) {
                ImVec2 tCenter = io.MousePos - scaleDragOffset;
                if (uniformScale) {
                    float dx = tCenter.x - t_x;
                    float dy = tCenter.y - t_y;
                    float dist = sqrtf(dx * dx + dy * dy);
                    float baseDist = sqrtf(h_dx * h_dx + h_dy * h_dy);
                    t_scaleX = t_scaleY = std::clamp(dist / (baseDist > 0.01f ? baseDist : 0.01f), 0.05f, 10.0f);
                } else {
                    t_scaleX = std::clamp((tCenter.x - t_x) / (h_dx > 0.01f ? h_dx : 0.01f), 0.05f, 10.0f);
                    t_scaleY = std::clamp((tCenter.y - t_y) / (h_dy > 0.01f ? h_dy : 0.01f), 0.05f, 10.0f);
                }
            } else isScaling = false; 
        }
        if (isDragging && !isScaling) {
            if (ImGui::IsMouseDown(0)) { t_x = io.MousePos.x + dragOffset.x; t_y = io.MousePos.y + dragOffset.y; } 
            else isDragging = false; 
        }
    }
    float smoothness = 1.0f - expf(-20.0f * io.DeltaTime);
    out_x = ImLerp(out_x, t_x, smoothness); out_y = ImLerp(out_y, t_y, smoothness); 
    if (enableScale) { out_scaleX = ImLerp(out_scaleX, t_scaleX, smoothness); out_scaleY = uniformScale ? out_scaleX : ImLerp(out_scaleY, t_scaleY, smoothness); }
}

void HandleGridInteraction(float& ox, float& oy, float& os, float& tx, float& ty, float& ts, bool& d, bool& s, ImVec2& doff, ImVec2& soff, float hdx, float hdy, float cdx, float cdy, float mx, float my, float mX, float mY, bool l, bool* o, bool e) {
    HandleGridInteractionCore(ox, oy, os, os, tx, ty, ts, ts, d, s, doff, soff, hdx, hdy, cdx, cdy, mx, my, mX, mY, l, o, e, true);
}

void HandleGridInteractionXY(float& ox, float& oy, float& osX, float& osY, float& tx, float& ty, float& tsX, float& tsY, bool& d, bool& s, ImVec2& doff, ImVec2& soff, float hdx, float hdy, float cdx, float cdy, float mx, float my, float mX, float mY, bool l, bool* o) {
    HandleGridInteractionCore(ox, oy, osX, osY, tx, ty, tsX, tsY, d, s, doff, soff, hdx, hdy, cdx, cdy, mx, my, mX, mY, l, o, true, false);
}

void DrawScaleHandle(ImDrawList* d, ImVec2 p, bool isScaling) {
    d->AddCircleFilled(p, 10.0f * g_autoScale, IM_COL32(255, 215, 0, 240));
    d->AddCircleFilled(p, 4.0f * g_autoScale, isScaling ? IM_COL32(0, 255, 180, 255) : IM_COL32(255, 255, 255, 255));
    d->AddCircle(p, 14.0f * g_autoScale, IM_COL32(255, 215, 0, 150), 32, 2.0f * g_autoScale);
}

void DrawCloseHandle(ImDrawList* d, ImVec2 p, bool* isOpen) {
    if (!isOpen) return; 
    ImGuiIO& io = ImGui::GetIO(); float cr = 13.0f * g_autoScale;
    static std::unordered_map<void*, float> hover_map; 
    float cha = (hover_map[isOpen] = ImLerp(hover_map[isOpen], ImLengthSqr(io.MousePos - p) < (cr*cr * 2.5f) ? 1.0f : 0.0f, 1.0f - expf(-15.0f * io.DeltaTime)));
    d->AddCircleFilled(p, cr, IM_COL32(200 + 55*cha, 50, 50, 200 + 55*cha));
    d->AddLine(p - ImVec2(cr*0.35f, cr*0.35f), p + ImVec2(cr*0.35f, cr*0.35f), IM_COL32_WHITE, 2.5f * g_autoScale);
    d->AddLine(p + ImVec2(cr*0.35f, -cr*0.35f), p - ImVec2(cr*0.35f, -cr*0.35f), IM_COL32_WHITE, 2.5f * g_autoScale);
}

void ColoredSeparator() { 
    ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.0f, 0.85f, 0.55f, 0.6f)); 
    ImGui::Separator(); 
    ImGui::PopStyleColor(); 
}

template <typename T> void ModernAdjusterInternal(const char* label, T* v, T v_min, T v_max, T step, const char* format) {
    ImGuiWindow* win = ImGui::GetCurrentWindow(); const ImGuiStyle& style = ImGui::GetStyle(); ImGui::PushID(label);
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetFrameHeight() * 2.2f + style.ItemInnerSpacing.x); ImGui::Text("%s", label); ImGui::SameLine();
    
    float btn_sz = ImGui::GetFrameHeight(); 
    float val_w = 40.0f * g_autoScale * g_scale; 
    
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 15.0f * g_autoScale * g_scale);

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.25f, 0.3f, 1.0f)); ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f * g_autoScale);
    ImGui::PushButtonRepeat(true);
    if (ImGui::Button("-", ImVec2(btn_sz, btn_sz))) if (*v > v_min) *v -= step; ImGui::SameLine();
    char buf[32]; std::is_same<T, int>::value ? snprintf(buf, sizeof(buf), format, *v) : snprintf(buf, sizeof(buf), format, (float)(*v)); 
    ImVec2 t_sz = ImGui::CalcTextSize(buf); ImVec2 val_pos = win->DC.CursorPos;
    win->DrawList->AddRectFilled(val_pos, val_pos + ImVec2(val_w, btn_sz), IM_COL32(20, 25, 30, 255), 4.0f * g_autoScale * g_scale);
    win->DrawList->AddText(ImGui::GetFont(), ImGui::GetFontSize(), val_pos + ImVec2((val_w - t_sz.x)*0.5f, (btn_sz - t_sz.y)*0.5f), IM_COL32(0, 255, 180, 255), buf); ImGui::Dummy(ImVec2(val_w, btn_sz)); ImGui::SameLine();
    if (ImGui::Button("+", ImVec2(btn_sz, btn_sz))) if (*v < v_max) *v += step; 
    ImGui::PopButtonRepeat();
    ImGui::PopStyleVar(); ImGui::PopStyleColor(); ImGui::PopID();
}

void ModernNumberAdjuster(const char* label, int* v, int v_min, int v_max) { ModernAdjusterInternal<int>(label, v, v_min, v_max, 1, "%d"); }
void ModernFloatAdjuster(const char* label, float* v, float v_min, float v_max, float step) { ModernAdjusterInternal<float>(label, v, v_min, v_max, step, "%.1f"); }

void ModernSlider(const char* label, float* v, float v_min, float v_max) {
    ImGuiWindow* win = ImGui::GetCurrentWindow(); const ImGuiStyle& style = ImGui::GetStyle(); ImGui::PushID(label);
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetFrameHeight() * 2.2f + style.ItemInnerSpacing.x); ImGui::Text("%s", label); ImGui::SameLine();
    
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 15.0f * g_autoScale * g_scale);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.18f, 0.22f, 1.0f)); ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.20f, 0.25f, 0.30f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.0f, 0.85f, 0.55f, 1.0f)); ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.0f, 0.95f, 0.65f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f * g_autoScale); 
    
    ImGui::PushItemWidth(180.0f * g_autoScale * g_scale); 
    ImGui::SliderFloat("##sl", v, v_min, v_max, "%.2f"); 
    ImGui::PopItemWidth(); ImGui::PopStyleVar(); ImGui::PopStyleColor(4); ImGui::PopID();
}

bool ModernToggle(const char* label, bool* v, int idx) {
    ImGuiWindow* win = ImGui::GetCurrentWindow(); const ImGuiStyle& style = ImGui::GetStyle(); ImGuiID id = win->GetID(label);
    float h = ImGui::GetFrameHeight(); float w = h * 2.2f; const ImRect bb(win->DC.CursorPos, win->DC.CursorPos + ImVec2(w + style.ItemInnerSpacing.x + ImGui::CalcTextSize(label).x, h));
    ImGui::ItemSize(bb, style.FramePadding.y); bool pressed = false;
    if (ImGui::ItemAdd(bb, id)) { bool hovered, held; if ((pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held))) *v = !(*v); }
    g_anim[idx] = ImLerp(g_anim[idx], *v ? 1.0f : 0.0f, 1.0f - expf(-20.0f * ImGui::GetIO().DeltaTime)); 
    win->DrawList->AddRectFilled(bb.Min, bb.Min + ImVec2(w, h), ImGui::GetColorU32(ImLerp(ImVec4(0.2f, 0.22f, 0.27f, 1.0f), ImVec4(0.0f, 0.85f, 0.55f, 1.0f), g_anim[idx])), h*0.5f);
    win->DrawList->AddRect(bb.Min, bb.Min + ImVec2(w, h), IM_COL32(0, 0, 0, 80), h*0.5f, 0, 1.0f);
    ImVec2 hc = bb.Min + ImVec2(h*0.5f + g_anim[idx]*(w-h), h*0.5f);
    win->DrawList->AddCircleFilled(hc + ImVec2(0, 1.5f), h*0.5f - 2.5f, IM_COL32(0, 0, 0, 90)); win->DrawList->AddCircleFilled(hc, h*0.5f - 2.5f, IM_COL32_WHITE);
    ImGui::RenderText(ImVec2(bb.Min.x + w + style.ItemInnerSpacing.x, bb.Min.y + (h - ImGui::GetFontSize()) * 0.5f), label);
    return pressed;
}

bool ModernAnimatedFolder(const char* label, bool* state) {
    ImGuiWindow* win = ImGui::GetCurrentWindow(); ImGuiID id = win->GetID(label); ImVec2 pos = win->DC.CursorPos; ImVec2 size(ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeight() * 1.3f);
    const ImRect bb(pos, pos + size); ImGui::ItemSize(bb); bool hovered = false, held = false; 
    if (ImGui::ItemAdd(bb, id)) if (ImGui::ButtonBehavior(bb, id, &hovered, &held)) *state = !(*state); 
    float* p_anim = win->StateStorage.GetFloatRef(id, *state ? 1.0f : 0.0f); float anim = (*p_anim = ImLerp(*p_anim, *state ? 1.0f : 0.0f, 1.0f - expf(-18.0f * ImGui::GetIO().DeltaTime)));
    win->DrawList->AddRectFilled(bb.Min, bb.Max, hovered ? IM_COL32(50, 60, 75, 200) : IM_COL32(40, 48, 60, 150), 8.0f * g_autoScale * g_scale);
    float cx = bb.Min.x + 15.0f * g_autoScale * g_scale; float cy = bb.Min.y + size.y * 0.5f; float ang = anim * 1.5708f; float arr = 5.0f * g_autoScale * g_scale; 
    win->DrawList->AddTriangleFilled(ImVec2(cx+cosf(ang)*arr, cy+sinf(ang)*arr), ImVec2(cx+cosf(ang+2.094f)*arr, cy+sinf(ang+2.094f)*arr), ImVec2(cx+cosf(ang-2.094f)*arr, cy+sinf(ang-2.094f)*arr), IM_COL32(200, 200, 200, 255));
    ImFont* font = ImGui::GetFont(); float fSz = ImGui::GetFontSize();
    win->DrawList->AddText(font, fSz, ImVec2(cx + 25.0f * g_autoScale * g_scale, bb.Min.y + (size.y - fSz)*0.5f), IM_COL32_WHITE, label);
    if (*state) { ImGui::Indent(15.0f * g_autoScale * g_scale); return true; } return false;
}

void EndModernAnimatedFolder() { ImGui::Unindent(15.0f * g_autoScale * g_scale); }

void ModernTierSelector(const char* label, bool* tierArray) {
    ImGuiWindow* win = ImGui::GetCurrentWindow(); const ImGuiStyle& style = ImGui::GetStyle();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetFrameHeight() * 2.2f + style.ItemInnerSpacing.x); ImGui::Text("%s", label); ImGui::SameLine();
    for (int i = 1; i <= 5; i++) {
        if (i > 1) ImGui::SameLine(0, 5.0f * g_autoScale);
        ImVec2 pos = win->DC.CursorPos; ImRect bb(pos, pos + ImVec2(ImGui::GetFrameHeight()*1.2f, ImGui::GetFrameHeight())); ImGui::ItemSize(bb);
        if (ImGui::ItemAdd(bb, win->GetID(tierArray + i))) { bool h, he; if (ImGui::ButtonBehavior(bb, win->GetID(tierArray+i), &h, &he)) tierArray[i] = !tierArray[i]; }
        static std::unordered_map<void*, float> anims_map; float& a = (anims_map[tierArray + i] = ImLerp(anims_map[tierArray + i], tierArray[i] ? 1.0f : 0.0f, 1.0f - expf(-20.0f * ImGui::GetIO().DeltaTime)));
        win->DrawList->AddRectFilled(bb.Min, bb.Max, IM_COL32((int)(30+70*a), (int)(35+100*a), (int)(45+50*a), 255), 4.0f * g_autoScale);
        if (a > 0.01f) win->DrawList->AddRect(bb.Min, bb.Max, IM_COL32(255, 200, 50, (int)(200*a)), 4.0f*g_autoScale, 0, 1.5f*g_autoScale);
        char buf[4]; snprintf(buf, sizeof(buf), "%d", i); ImVec2 t_sz = ImGui::CalcTextSize(buf);
        win->DrawList->AddText(ImGui::GetFont(), ImGui::GetFontSize(), pos + ImVec2((bb.GetWidth() - t_sz.x)*0.5f, (bb.GetHeight() - t_sz.y)*0.5f), tierArray[i] ? IM_COL32(0,0,0,255) : IM_COL32_WHITE, buf);
    }
}

// =================================================================
// 绘制 ESP / 功能模块
// =================================================================
void DrawSwipePaths() {
    if (!g_show_swipe_visuals) return;
    std::lock_guard<std::mutex> lock(g_swipe_mutex);
    if (g_active_swipes.empty()) return;

    ImDrawList* d = ImGui::GetForegroundDrawList();
    float dt = ImGui::GetIO().DeltaTime;
    ImFont* font = g_mainFont ? g_mainFont : ImGui::GetFont();

    for (auto it = g_active_swipes.begin(); it != g_active_swipes.end(); ) {
        it->lifeTime -= dt;
        if (it->lifeTime <= 0.0f) {
            it = g_active_swipes.erase(it);
            continue;
        }

        float alpha = it->lifeTime / it->maxLife;
        ImU32 lineColor = IM_COL32(255, 200, 0, (int)(255 * alpha));
        
        d->AddLine(it->start, it->end, lineColor, 3.0f * g_autoScale);
        
        d->AddCircleFilled(it->start, 12.0f * g_autoScale, IM_COL32(0, 255, 100, (int)(200 * alpha)));
        d->AddCircle(it->start, 16.0f * g_autoScale, IM_COL32(255, 255, 255, (int)(255 * alpha)), 0, 2.0f * g_autoScale);
        
        d->AddCircleFilled(it->end, 12.0f * g_autoScale, IM_COL32(255, 50, 50, (int)(200 * alpha)));
        d->AddCircle(it->end, 16.0f * g_autoScale, IM_COL32(255, 255, 255, (int)(255 * alpha)), 0, 2.0f * g_autoScale);

        float progress = 1.0f - (it->lifeTime / it->maxLife);
        ImVec2 currentPos = ImVec2(it->start.x + (it->end.x - it->start.x) * progress, 
                                   it->start.y + (it->end.y - it->start.y) * progress);
        d->AddCircleFilled(currentPos, 8.0f * g_autoScale, IM_COL32(255, 255, 255, (int)(255 * alpha)));

        DrawTextWithStroke(d, font, 18.0f * g_autoScale, it->end + ImVec2(15, 15), IM_COL32(255, 255, 255, (int)(255*alpha)), IM_COL32(0,0,0,(int)(255*alpha)), "滑动释放");

        ++it;
    }
}

void DrawEquipUIDebug() {
    bool show_equip = g_show_equip_ui_debug;
    bool show_sell = g_show_sell_ui_debug;

    if (!show_equip && !show_sell) return;

    ImDrawList* d = ImGui::GetForegroundDrawList();
    ImFont* font = g_mainFont ? g_mainFont : ImGui::GetFont();
    ImVec2 mouse = ImGui::GetIO().MousePos;
    bool mouseDown = ImGui::IsMouseDown(0);
    bool mouseClicked = ImGui::IsMouseClicked(0);

    static int dragging_idx = -1; // 0: 穿装绿圈, 1: 卖牌粉圈(起点), 2: 卖牌红圈(落点)
    static int dragging_cell_idx = -1; // 正在拖拽的独立格子索引
    static ImVec2 drag_start_mouse;
    static float drag_start_off_x = 0;
    static float drag_start_off_y = 0;

    // 松手时自动保存配置
    if (!mouseDown && dragging_idx != -1) {
        dragging_idx = -1;
        dragging_cell_idx = -1;
        g_save_timer = 1.5f; 
        std::thread([]() { SaveConfig(); }).detach(); 
    }

    auto snap = GetSnapshot();
    float hitRadius = 45.0f * g_autoScale;  // 触摸判定范围(大，防断触)
    float drawRadius = 10.0f * g_autoScale; // 实际画的圆圈尺寸
    float dummyX = g_gl_width * 0.5f;
    float dummyY = g_gl_height * 0.5f;

    std::vector<LiveHeroData> allHeroData;
    if (snap) {
        for (const auto& hero : snap->myLiveHeroes) {
            if (hero.screenPos.z > 0.01f) allHeroData.push_back(hero);
        }
        for (const auto& hero : snap->myBenchLiveHeroes) {
            if (hero.screenPos.z > 0.01f) allHeroData.push_back(hero);
        }
    }

    // ==========================================
    // 【核心修复 3：新增装备席物理坐标透视】
    // ==========================================
    if (show_equip) {
        for (int i = 0; i < 20; i++) { 
            int col = i / 10;
            int row = i % 10;
            float startX = (g_equip_ui_start_x + col * g_equip_ui_step_x) * g_autoScale;
            float startY = (g_equip_ui_start_y + row * g_equip_ui_step_y) * g_autoScale;
            
            ImVec2 pos(startX, startY);
            d->AddCircleFilled(pos, 15.0f * g_autoScale, IM_COL32(0, 200, 255, 80));
            d->AddCircle(pos, 15.0f * g_autoScale, IM_COL32(0, 220, 255, 255), 0, 2.0f * g_autoScale);
            
            char buf[16]; snprintf(buf, sizeof(buf), "槽 %d", i);
            d->AddText(font, 12.0f * g_autoScale, pos - ImVec2(10 * g_autoScale, 8 * g_autoScale), IM_COL32(255, 255, 255, 255), buf);
        }
    }

    // ==========================================
    // 1. 穿拆装备可视化 (各个格子独立偏移，深绿色)
    // ==========================================
    if (show_equip) {
        if (allHeroData.empty()) {
            if (g_cell_equip_offset.find(0) == g_cell_equip_offset.end()) g_cell_equip_offset[0] = ImVec2(0.0f, -80.0f);
            ImVec2 targetPos(dummyX + g_cell_equip_offset[0].x * g_autoScale, dummyY + g_cell_equip_offset[0].y * g_autoScale);
            
            if (!g_boardLocked && mouseClicked && dragging_idx == -1 && ImLengthSqr(mouse - targetPos) < hitRadius * hitRadius) {
                dragging_idx = 0; dragging_cell_idx = 0; drag_start_mouse = mouse; 
                drag_start_off_x = g_cell_equip_offset[0].x; drag_start_off_y = g_cell_equip_offset[0].y;
            }
            if (mouseDown && dragging_idx == 0 && dragging_cell_idx == 0) {
                g_cell_equip_offset[0].x = drag_start_off_x + (mouse.x - drag_start_mouse.x) / g_autoScale;
                g_cell_equip_offset[0].y = drag_start_off_y + (mouse.y - drag_start_mouse.y) / g_autoScale;
                targetPos = ImVec2(dummyX + g_cell_equip_offset[0].x * g_autoScale, dummyY + g_cell_equip_offset[0].y * g_autoScale);
            }
            d->AddCircleFilled(ImVec2(dummyX, dummyY), 4.0f * g_autoScale, IM_COL32(255,255,255,150));
            d->AddText(font, 14.0f*g_autoScale, ImVec2(dummyX-30, dummyY-20), IM_COL32(255,255,255,150), "假人");
            d->AddLine(ImVec2(dummyX, dummyY), targetPos, IM_COL32(0, 160, 0, 100), 2.0f * g_autoScale);
            d->AddCircleFilled(targetPos, drawRadius, IM_COL32(0, 160, 0, 230));
            d->AddCircle(targetPos, drawRadius + 1.5f, IM_COL32(0, 220, 0, 255), 0, 1.5f * g_autoScale);
            d->AddText(font, 14.0f*g_autoScale, targetPos + ImVec2(drawRadius+2, -drawRadius-2), IM_COL32(0, 220, 0, 255), "格子0 穿装落点");
        } else {
            // 点击判定
            if (!g_boardLocked && mouseClicked && dragging_idx == -1) {
                for (const auto& hero : allHeroData) {
                    if (g_cell_equip_offset.find(hero.cellIndex) == g_cell_equip_offset.end()) g_cell_equip_offset[hero.cellIndex] = ImVec2(0.0f, -80.0f);
                    ImVec2 targetPos(hero.screenPos.x + g_cell_equip_offset[hero.cellIndex].x * g_autoScale, g_gl_height - hero.screenPos.y + g_cell_equip_offset[hero.cellIndex].y * g_autoScale);
                    if (ImLengthSqr(mouse - targetPos) < hitRadius * hitRadius) {
                        dragging_idx = 0; dragging_cell_idx = hero.cellIndex; drag_start_mouse = mouse; 
                        drag_start_off_x = g_cell_equip_offset[hero.cellIndex].x; drag_start_off_y = g_cell_equip_offset[hero.cellIndex].y;
                        break;
                    }
                }
            }
            // 拖拽应用
            if (mouseDown && dragging_idx == 0 && dragging_cell_idx != -1) {
                g_cell_equip_offset[dragging_cell_idx].x = drag_start_off_x + (mouse.x - drag_start_mouse.x) / g_autoScale;
                g_cell_equip_offset[dragging_cell_idx].y = drag_start_off_y + (mouse.y - drag_start_mouse.y) / g_autoScale;
            }
            // 渲染
            std::unordered_set<int> drawn_cells;
            for (const auto& hero : allHeroData) {
                if (drawn_cells.count(hero.cellIndex)) continue;
                drawn_cells.insert(hero.cellIndex);
                if (g_cell_equip_offset.find(hero.cellIndex) == g_cell_equip_offset.end()) g_cell_equip_offset[hero.cellIndex] = ImVec2(0.0f, -80.0f);

                ImVec2 basePos(hero.screenPos.x, g_gl_height - hero.screenPos.y);
                ImVec2 targetPos(basePos.x + g_cell_equip_offset[hero.cellIndex].x * g_autoScale, basePos.y + g_cell_equip_offset[hero.cellIndex].y * g_autoScale);
                
                d->AddLine(basePos, targetPos, IM_COL32(0, 160, 0, 100), 2.0f * g_autoScale);
                d->AddCircleFilled(targetPos, drawRadius, IM_COL32(0, 160, 0, 230)); 
                d->AddCircle(targetPos, drawRadius + 1.5f, IM_COL32(0, 220, 0, 255), 0, 1.5f * g_autoScale);
                char buf[32]; snprintf(buf, sizeof(buf), "格子%d穿装落点", hero.cellIndex);
                d->AddText(font, 14.0f*g_autoScale, targetPos + ImVec2(drawRadius+2, -drawRadius-2), IM_COL32(0, 220, 0, 255), buf);
            }
        }
    }

    // ==========================================
    // 2. 卖出英雄可视化 (各个格子独立起点 -> 统一全局落点)
    // ==========================================
    if (show_sell) {
        ImVec2 dropPos(g_sell_drop_x * g_autoScale, g_sell_drop_y * g_autoScale); 

        // 点击判定
        if (!g_boardLocked && mouseClicked && dragging_idx == -1) {
            if (ImLengthSqr(mouse - dropPos) < hitRadius * hitRadius) { // 优先点红圈
                dragging_idx = 2; drag_start_mouse = mouse; 
                drag_start_off_x = g_sell_drop_x; drag_start_off_y = g_sell_drop_y;
            } else {
                for (const auto& hero : allHeroData) { 
                    if (g_cell_sell_offset.find(hero.cellIndex) == g_cell_sell_offset.end()) g_cell_sell_offset[hero.cellIndex] = ImVec2(0.0f, -80.0f);
                    ImVec2 grabPos(hero.screenPos.x + g_cell_sell_offset[hero.cellIndex].x * g_autoScale, g_gl_height - hero.screenPos.y + g_cell_sell_offset[hero.cellIndex].y * g_autoScale);
                    if (ImLengthSqr(mouse - grabPos) < hitRadius * hitRadius) {
                        dragging_idx = 1; dragging_cell_idx = hero.cellIndex; drag_start_mouse = mouse; 
                        drag_start_off_x = g_cell_sell_offset[hero.cellIndex].x; drag_start_off_y = g_cell_sell_offset[hero.cellIndex].y;
                        break;
                    }
                }
            }
            // 假人补充逻辑
            if (dragging_idx == -1 && allHeroData.empty()) {
                if (g_cell_sell_offset.find(0) == g_cell_sell_offset.end()) g_cell_sell_offset[0] = ImVec2(0.0f, -80.0f);
                ImVec2 grabPos(dummyX + g_cell_sell_offset[0].x * g_autoScale, dummyY + g_cell_sell_offset[0].y * g_autoScale);
                if (ImLengthSqr(mouse - grabPos) < hitRadius * hitRadius) {
                    dragging_idx = 1; dragging_cell_idx = 0; drag_start_mouse = mouse; drag_start_off_x = g_cell_sell_offset[0].x; drag_start_off_y = g_cell_sell_offset[0].y;
                }
            }
        }

        // 拖动计算
        if (mouseDown) {
            if (dragging_idx == 1 && dragging_cell_idx != -1) { // 拖动独立格子起滑点
                g_cell_sell_offset[dragging_cell_idx].x = drag_start_off_x + (mouse.x - drag_start_mouse.x) / g_autoScale;
                g_cell_sell_offset[dragging_cell_idx].y = drag_start_off_y + (mouse.y - drag_start_mouse.y) / g_autoScale;
            } else if (dragging_idx == 2) { // 拖动全局落点
                g_sell_drop_x = drag_start_off_x + (mouse.x - drag_start_mouse.x) / g_autoScale;
                g_sell_drop_y = drag_start_off_y + (mouse.y - drag_start_mouse.y) / g_autoScale;
                dropPos = ImVec2(g_sell_drop_x * g_autoScale, g_sell_drop_y * g_autoScale); 
            }
        }

        // 渲染假人
        if (allHeroData.empty()) {
            ImVec2 grabPos(dummyX + g_cell_sell_offset[0].x * g_autoScale, dummyY + g_cell_sell_offset[0].y * g_autoScale);
            d->AddCircleFilled(ImVec2(dummyX, dummyY), 4.0f * g_autoScale, IM_COL32(255,255,255,150));
            d->AddLine(grabPos, dropPos, IM_COL32(160, 0, 160, 150), 1.5f * g_autoScale); 
            d->AddCircleFilled(grabPos, drawRadius, IM_COL32(160, 0, 160, 230));
            d->AddCircle(grabPos, drawRadius + 1.5f, IM_COL32(220, 50, 220, 255), 0, 1.5f * g_autoScale);
            d->AddText(font, 14.0f*g_autoScale, grabPos + ImVec2(drawRadius+2, -drawRadius-2), IM_COL32(220, 50, 220, 255), "格子0卖出起滑");
        }

        // 画单一落点 (深红圈)
        d->AddCircleFilled(dropPos, drawRadius * 1.5f, IM_COL32(180, 0, 0, 230));
        d->AddCircle(dropPos, drawRadius * 1.5f + 1.5f, IM_COL32(255, 50, 50, 255), 0, 2.0f * g_autoScale);
        d->AddText(font, 14.0f*g_autoScale, dropPos + ImVec2(drawRadius*1.5f+2, -drawRadius*1.5f-2), IM_COL32(255, 50, 50, 255), "全局卖出落点");

        // 渲染所有真实英雄独立格子起滑点 (深紫圈) 及连线
        std::unordered_set<int> drawn_cells;
        for (const auto& hero : allHeroData) {
            if (drawn_cells.count(hero.cellIndex)) continue;
            drawn_cells.insert(hero.cellIndex);
            if (g_cell_sell_offset.find(hero.cellIndex) == g_cell_sell_offset.end()) g_cell_sell_offset[hero.cellIndex] = ImVec2(0.0f, -80.0f);

            ImVec2 basePos(hero.screenPos.x, g_gl_height - hero.screenPos.y);
            ImVec2 grabPos(basePos.x + g_cell_sell_offset[hero.cellIndex].x * g_autoScale, basePos.y + g_cell_sell_offset[hero.cellIndex].y * g_autoScale);
            
            d->AddLine(grabPos, dropPos, IM_COL32(160, 0, 160, 100), 1.5f * g_autoScale); 
            d->AddLine(basePos, grabPos, IM_COL32(160, 0, 160, 60), 1.0f * g_autoScale);  
            
            d->AddCircleFilled(grabPos, drawRadius, IM_COL32(160, 0, 160, 230));
            d->AddCircle(grabPos, drawRadius + 1.5f, IM_COL32(220, 50, 220, 255), 0, 1.5f * g_autoScale);
            char buf[32]; snprintf(buf, sizeof(buf), "格子%d卖出起滑", hero.cellIndex);
            d->AddText(font, 14.0f*g_autoScale, grabPos + ImVec2(drawRadius+2, -drawRadius-2), IM_COL32(220, 50, 220, 255), buf);
        }
    }
}

void DrawSingleSlotOverlay(const char* name, const char* rowLabel, bool& isOpen, float& posX, float& posY, float& scale, int totalSlots, 
                     const std::vector<HeroSlotInfo>& data, ImU32 baseLabelColor) {
    static std::unordered_map<const char*, float> alphas_raw;
    float deltaTime = ImGui::GetIO().DeltaTime;
    alphas_raw[name] = ImLerp(alphas_raw[name], isOpen ? 1.0f : 0.0f, 1.0f - expf(-20.0f * deltaTime));
    float alpha = alphas_raw[name] * g_global_esp_alpha;
    if (alpha < 0.01f) return;

    ImDrawList* d = ImGui::GetForegroundDrawList();
    static std::unordered_map<const char*, float> t_xs, t_ys, t_scales;
    if (t_scales.find(name) == t_scales.end()) { t_xs[name] = posX; t_ys[name] = posY; t_scales[name] = scale; }
    static std::unordered_map<const char*, bool> draggings, scalings;
    static std::unordered_map<const char*, ImVec2> dragOffsets, scaleDragOffsets;

    float baseSz = 60.0f * g_autoScale, labelW = 40.0f * g_autoScale; 
    float totalW = labelW + totalSlots * baseSz, totalH = baseSz;

    HandleGridInteraction(posX, posY, scale, t_xs[name], t_ys[name], t_scales[name], draggings[name], scalings[name], dragOffsets[name], scaleDragOffsets[name],
                          totalW + 20.0f * g_autoScale, totalH * 0.5f, -10.0f * g_autoScale, totalH * 0.5f,
                          0, 0, totalW, totalH, g_boardLocked, &isOpen, true);

    if (!g_boardLocked && alpha > 0.9f * g_global_esp_alpha) {
        DrawScaleHandle(d, ImVec2(posX + (totalW + 20.0f * g_autoScale) * scale, posY + totalH * 0.5f * scale), scalings[name]);
        DrawCloseHandle(d, ImVec2(posX - 10.0f * g_autoScale * scale, posY + totalH * 0.5f * scale), &isOpen);
    }

    ImFont* font = g_mainFont ? g_mainFont : ImGui::GetFont();
    ImU32 labelColor = (baseLabelColor & 0x00FFFFFF) | ((int)(255 * alpha) << 24);

    float lFSz = 26.0f * g_autoScale * scale;
    ImVec2 lSz = font->CalcTextSizeA(lFSz, FLT_MAX, 0.0f, rowLabel);
    ImVec2 lPos = ImVec2(posX + (labelW * scale - lSz.x) * 0.5f, posY + (baseSz * scale - lSz.y) * 0.5f);
    
    DrawTextWithStroke(d, font, lFSz, lPos, labelColor, IM_COL32(0,0,0,(int)(255*alpha)), rowLabel);

    for (int i = 0; i < totalSlots; i++) {
        float cx = posX + labelW * scale + i * baseSz * scale + (baseSz * 0.5f) * scale;
        float cy = posY + (baseSz * 0.5f) * scale;
        float drawRadius = baseSz * 0.5f * scale;

        ImU32 borderColor = IM_COL32(100, 100, 100, (int)(255 * alpha));
        int matchedBaseId = -1, matchedCost = -1, matchedStarLevel = -1;

        for (const auto& hero : data) {
            if (hero.slotIndex == i) {
                matchedBaseId = hero.baseHeroId; matchedCost = hero.cost; matchedStarLevel = hero.starLevel; 
                break; 
            }
        }

        if (matchedCost != -1) {
            switch (matchedCost) {
                case 1: borderColor = IM_COL32(200, 200, 200, (int)(255 * alpha)); break; 
                case 2: borderColor = IM_COL32(0, 255, 0, (int)(255 * alpha)); break;   
                case 3: borderColor = IM_COL32(0, 200, 255, (int)(255 * alpha)); break; 
                case 4: borderColor = IM_COL32(210, 50, 255, (int)(255 * alpha)); break; 
                case 5: borderColor = IM_COL32(255, 215, 0, (int)(255 * alpha)); break; 
            }
        }

        if (matchedBaseId != -1) {
            GLuint tex = GetHeroTexture(matchedBaseId);
            ImVec2 pMin = ImVec2(cx - drawRadius, cy - drawRadius);
            ImVec2 pMax = ImVec2(cx + drawRadius, cy + drawRadius);
            
            if (tex != 0) {
                d->AddImageRounded((ImTextureID)(intptr_t)tex, pMin, pMax, ImVec2(0,0), ImVec2(1,1), IM_COL32(255, 255, 255, (int)(255 * alpha)), 8.0f * g_autoScale * scale);
            } else {
                char idBuf[16]; snprintf(idBuf, sizeof(idBuf), "%d", matchedBaseId);
                float idFSz = 20.0f * g_autoScale * scale;
                ImVec2 idSz = font->CalcTextSizeA(idFSz, FLT_MAX, 0.0f, idBuf);
                d->AddText(font, idFSz, ImVec2(cx - idSz.x * 0.5f, cy - idSz.y * 0.5f), borderColor, idBuf);
            }

            char starBuf[16]; snprintf(starBuf, sizeof(starBuf), "%d星", matchedStarLevel);
            float fSzStar = 20.0f * g_autoScale * scale;
            ImVec2 tPos = pMin + ImVec2(4.0f * scale, 4.0f * scale);
            DrawTextWithStroke(d, font, fSzStar, tPos, borderColor, IM_COL32(0,0,0, (int)(255 * alpha)), starBuf);
        }
        d->AddRect(ImVec2(cx - drawRadius, cy - drawRadius), ImVec2(cx + drawRadius, cy + drawRadius), borderColor, 8.0f * g_autoScale * scale, 0, 2.5f * g_autoScale * scale);
    }
}

void DrawAutoEquipWindow() {
    if (!g_show_equip_window) return;
    static float alpha = 0.0f; 
    alpha = ImLerp(alpha, g_show_equip_window ? 1.0f : 0.0f, 1.0f - expf(-20.0f * ImGui::GetIO().DeltaTime));
    if (alpha < 0.01f) return;

    ImDrawList* d = ImGui::GetForegroundDrawList();
    ImGuiIO& io = ImGui::GetIO();

    static float t_x = g_equip_win_x, t_y = g_equip_win_y, t_scale = g_equip_win_scale;
    static bool first = true; if (first) { t_x = g_equip_win_x; t_y = g_equip_win_y; t_scale = g_equip_win_scale; first = false; }
    static bool isDragging = false, isScaling = false; static ImVec2 dragOffset, scaleDragOffset;

    float baseW = 260.0f * g_autoScale, baseH = 60.0f * g_autoScale;
    HandleGridInteraction(g_equip_win_x, g_equip_win_y, g_equip_win_scale, t_x, t_y, t_scale,
                          isDragging, isScaling, dragOffset, scaleDragOffset,
                          baseW + 20.0f * g_autoScale, baseH * 0.5f,
                          -20.0f * g_autoScale, baseH * 0.5f,
                          0, 0, baseW, baseH, g_boardLocked, &g_show_equip_window, true);

    float curW = baseW * g_equip_win_scale, curH = baseH * g_equip_win_scale;
    ImRect bgRect(ImVec2(g_equip_win_x, g_equip_win_y), ImVec2(g_equip_win_x + curW, g_equip_win_y + curH));

    if (!g_boardLocked && alpha > 0.9f) {
        DrawScaleHandle(d, ImVec2(g_equip_win_x + (baseW + 20.0f * g_autoScale) * g_equip_win_scale, g_equip_win_y + baseH * 0.5f * g_equip_win_scale), isScaling);
        DrawCloseHandle(d, ImVec2(g_equip_win_x - 20.0f * g_autoScale * g_equip_win_scale, g_equip_win_y + baseH * 0.5f * g_equip_win_scale), &g_show_equip_window);
    }
    
    float rounding = curH * 0.5f;
    d->AddRectFilled(bgRect.Min, bgRect.Max, IM_COL32(20, 25, 30, (int)(220 * alpha)), rounding);
    d->AddRect(bgRect.Min, bgRect.Max, IM_COL32(80, 85, 95, (int)(200 * alpha)), rounding, 0, 2.0f * g_autoScale * g_equip_win_scale);

    ImFont* font = g_mainFont ? g_mainFont : ImGui::GetFont();

    float curBtnW = 110.0f * g_autoScale * g_equip_win_scale, curBtnH = 44.0f * g_autoScale * g_equip_win_scale;
    float gap = (curW - curBtnW * 2.0f) / 3.0f;
    ImVec2 btn1Pos = ImVec2(g_equip_win_x + gap, g_equip_win_y + (curH - curBtnH) * 0.5f);
    ImVec2 btn2Pos = ImVec2(g_equip_win_x + gap * 2.0f + curBtnW, g_equip_win_y + (curH - curBtnH) * 0.5f);

    auto drawBtn = [&](ImVec2 pos, const char* text, bool& active, int id) {
        ImRect bb(pos, pos + ImVec2(curBtnW, curBtnH));
        bool hovered = bb.Contains(io.MousePos);
        if (hovered && ImGui::IsMouseClicked(0) && !isDragging && !isScaling) active = !active;
        
        static std::unordered_map<int, float> anims;
        float target = active ? 1.0f : (hovered ? 0.6f : 0.0f);
        float a = (anims[id] = ImLerp(anims[id], target, 1.0f - expf(-20.0f * io.DeltaTime)));
        float rR = curBtnH * 0.5f;

        float hue = fmodf((float)ImGui::GetTime() * 0.5f + id * 0.3f, 1.0f);
        float r, g, b; ImGui::ColorConvertHSVtoRGB(hue, 0.7f, 1.0f, r, g, b);
        ImU32 neonColor = IM_COL32((int)(r * 255), (int)(g * 255), (int)(b * 255), (int)(200 * alpha));

        if (active) {
            d->AddRectFilled(bb.Min, bb.Max, IM_COL32(0, 200, 150, (int)(255 * alpha)), rR);
            d->AddRect(bb.Min, bb.Max, IM_COL32(0, 255, 180, (int)(255 * alpha)), rR, 0, 2.5f * g_autoScale * g_equip_win_scale);
        } else {
            d->AddRectFilled(bb.Min, bb.Max, IM_COL32(40, 45, 55, (int)(255 * alpha)), rR);
            d->AddRect(bb.Min, bb.Max, neonColor, rR, 0, 2.0f * g_autoScale * g_equip_win_scale);
        }

        float fSz = 20.0f * g_autoScale * g_equip_win_scale;
        ImVec2 tSz = font->CalcTextSizeA(fSz, FLT_MAX, 0.0f, text);
        d->AddText(font, fSz, pos + ImVec2((curBtnW - tSz.x)*0.5f, (curBtnH - tSz.y)*0.5f), IM_COL32(255, 255, 255, (int)(255 * alpha)), text);
    };

    drawBtn(btn1Pos, (const char*)u8"穿装备", g_auto_equip_active, 201);
    drawBtn(btn2Pos, (const char*)u8"拆装备", g_auto_unequip_active, 202);
}

void DrawHeroLinesESP(const GameSnapshot* snap) {
    if (!snap || !g_draw_hero_lines || snap->mainCamera == 0) return;

    ImDrawList* d = ImGui::GetBackgroundDrawList();
    ImGuiIO& io = ImGui::GetIO();
    
    ImVec2 screenCenter(io.DisplaySize.x * 0.5f, 0.0f);
    
    auto drawLine = [&](const LiveHeroData& hero) {
        if (hero.screenPos.z > 0.01f) {
            float imguiX = hero.screenPos.x;
            float imguiY = io.DisplaySize.y - hero.screenPos.y + g_esp_line_offset_y * g_autoScale;
            ImVec2 targetPos(imguiX, imguiY);
            
            d->AddLine(screenCenter, targetPos, IM_COL32(0, 255, 200, 150), 2.0f);
            d->AddCircleFilled(targetPos, 5.0f, IM_COL32(0, 255, 200, 255));
            
            char buf[32]; snprintf(buf, sizeof(buf), "Hero: %d", hero.heroId);
            ImFont* font = g_mainFont ? g_mainFont : ImGui::GetFont();
            DrawTextWithStroke(d, font, 18.0f * g_autoScale, targetPos + ImVec2(10.0f, -10.0f), IM_COL32(255,255,255,255), IM_COL32(0,0,0,255), buf);
        }
    };

    for (const auto& hero : snap->myLiveHeroes) drawLine(hero);
    for (const auto& hero : snap->myBenchLiveHeroes) drawLine(hero);
}

void DrawAutoActionWindow() {
    if (!g_show_auto_window) return;
    static float alpha = 0.0f; 
    alpha = ImLerp(alpha, g_show_auto_window ? 1.0f : 0.0f, 1.0f - expf(-20.0f * ImGui::GetIO().DeltaTime));
    if (alpha < 0.01f) return;

    ImDrawList* d = ImGui::GetForegroundDrawList();
    ImGuiIO& io = ImGui::GetIO();

    static float t_x = g_auto_win_x, t_y = g_auto_win_y, t_scale = g_auto_win_scale;
    static bool first = true; if (first) { t_x = g_auto_win_x; t_y = g_auto_win_y; t_scale = g_auto_win_scale; first = false; }
    static bool isDragging = false, isScaling = false; static ImVec2 dragOffset, scaleDragOffset;

    float baseW = 260.0f * g_autoScale;
    float baseH = g_show_clear_bench_btn ? (130.0f * g_autoScale) : (60.0f * g_autoScale); 
    
    HandleGridInteraction(g_auto_win_x, g_auto_win_y, g_auto_win_scale, t_x, t_y, t_scale,
                          isDragging, isScaling, dragOffset, scaleDragOffset,
                          baseW + 20.0f * g_autoScale, baseH * 0.5f,
                          -20.0f * g_autoScale, baseH * 0.5f,
                          0, 0, baseW, baseH, g_boardLocked, &g_show_auto_window, true);

    float curW = baseW * g_auto_win_scale, curH = baseH * g_auto_win_scale;
    ImRect bgRect(ImVec2(g_auto_win_x, g_auto_win_y), ImVec2(g_auto_win_x + curW, g_auto_win_y + curH));

    if (!g_boardLocked && alpha > 0.9f) {
        DrawScaleHandle(d, ImVec2(g_auto_win_x + (baseW + 20.0f * g_autoScale) * g_auto_win_scale, g_auto_win_y + baseH * 0.5f * g_auto_win_scale), isScaling);
        DrawCloseHandle(d, ImVec2(g_auto_win_x - 20.0f * g_autoScale * g_auto_win_scale, g_auto_win_y + baseH * 0.5f * g_auto_win_scale), &g_show_auto_window);
    }

    float rounding = 60.0f * g_autoScale * g_auto_win_scale * 0.3f;
    d->AddRectFilled(bgRect.Min, bgRect.Max, IM_COL32(20, 25, 30, (int)(220 * alpha)), rounding);
    d->AddRect(bgRect.Min, bgRect.Max, IM_COL32(80, 85, 95, (int)(200 * alpha)), rounding, 0, 2.0f * g_autoScale * g_auto_win_scale);

    ImFont* font = g_mainFont ? g_mainFont : ImGui::GetFont();

    float curBtnW = 110.0f * g_autoScale * g_auto_win_scale, curBtnH = 44.0f * g_autoScale * g_auto_win_scale;
    float gap = (curW - curBtnW * 2.0f) / 3.0f;
    
    ImVec2 btn1Pos = ImVec2(g_auto_win_x + gap, g_auto_win_y + gap);
    ImVec2 btn2Pos = ImVec2(g_auto_win_x + gap * 2.0f + curBtnW, g_auto_win_y + gap);

    auto drawBtn = [&](ImVec2 pos, const char* text, bool& active, int id) {
        ImRect bb(pos, pos + ImVec2(curBtnW, curBtnH));
        bool hovered = bb.Contains(io.MousePos);
        if (hovered && ImGui::IsMouseClicked(0) && !isDragging && !isScaling) active = !active;
        
        static std::unordered_map<int, float> anims;
        float target = active ? 1.0f : (hovered ? 0.6f : 0.0f);
        float a = (anims[id] = ImLerp(anims[id], target, 1.0f - expf(-20.0f * io.DeltaTime)));
        float rR = curBtnH * 0.5f;

        float hue = fmodf((float)ImGui::GetTime() * 0.5f + id * 0.3f, 1.0f);
        float r, g, b; ImGui::ColorConvertHSVtoRGB(hue, 0.7f, 1.0f, r, g, b);
        ImU32 neonColor = IM_COL32((int)(r * 255), (int)(g * 255), (int)(b * 255), (int)(200 * alpha));

        if (active) {
            d->AddRectFilled(bb.Min, bb.Max, IM_COL32(0, 200, 150, (int)(255 * alpha)), rR);
            d->AddRect(bb.Min, bb.Max, IM_COL32(0, 255, 180, (int)(255 * alpha)), rR, 0, 2.5f * g_autoScale * g_auto_win_scale);
        } else {
            d->AddRectFilled(bb.Min, bb.Max, IM_COL32(40, 45, 55, (int)(255 * alpha)), rR);
            d->AddRect(bb.Min, bb.Max, neonColor, rR, 0, 2.0f * g_autoScale * g_auto_win_scale);
        }

        float fSz = 20.0f * g_autoScale * g_auto_win_scale;
        ImVec2 tSz = font->CalcTextSizeA(fSz, FLT_MAX, 0.0f, text);
        d->AddText(font, fSz, pos + ImVec2((curBtnW - tSz.x)*0.5f, (curBtnH - tSz.y)*0.5f), IM_COL32(255, 255, 255, (int)(255 * alpha)), text);
    };

    auto drawClickBtn = [&](ImVec2 pos, float w, float h, const char* text) -> bool {
        ImRect bb(pos, pos + ImVec2(w, h));
        bool hovered = bb.Contains(io.MousePos);
        bool clicked = hovered && ImGui::IsMouseClicked(0) && !isDragging && !isScaling;
        bool held = hovered && ImGui::IsMouseDown(0);
        
        float rR = h * 0.5f;
        float alpha_factor = held ? 0.8f : (hovered ? 1.0f : 0.7f);
        ImU32 bgColor = IM_COL32(220, 60, 60, (int)(255 * alpha * alpha_factor)); 
        
        d->AddRectFilled(bb.Min, bb.Max, bgColor, rR);
        d->AddRect(bb.Min, bb.Max, IM_COL32(255, 100, 100, (int)(255 * alpha)), rR, 0, 2.0f * g_autoScale * g_auto_win_scale);
        
        float fSz = 20.0f * g_autoScale * g_auto_win_scale;
        ImVec2 tSz = font->CalcTextSizeA(fSz, FLT_MAX, 0.0f, text);
        d->AddText(font, fSz, pos + ImVec2((w - tSz.x)*0.5f, (h - tSz.y)*0.5f), IM_COL32(255, 255, 255, (int)(255 * alpha)), text);
        
        return clicked;
    };

    drawBtn(btn1Pos, (const char*)u8"自动刷新", g_auto_refresh, 101);
    drawBtn(btn2Pos, (const char*)u8"自动拿牌", g_auto_buy, 102);

    if (g_show_clear_bench_btn) {
        ImVec2 btn3Pos = ImVec2(g_auto_win_x + gap, g_auto_win_y + gap * 2.0f + curBtnH); 
        if (drawClickBtn(btn3Pos, curW - gap * 2.0f, curBtnH, (const char*)u8"一键清空备战席")) {
            TriggerSellAllHeroes();
        }
    }
}

void DrawInstantQuitWindow() {
    if (!g_show_instant_quit_window) return;
    static float alpha = 0.0f; 
    alpha = ImLerp(alpha, g_show_instant_quit_window ? 1.0f : 0.0f, 1.0f - expf(-20.0f * ImGui::GetIO().DeltaTime));
    if (alpha < 0.01f) return;

    ImDrawList* d = ImGui::GetForegroundDrawList();
    ImGuiIO& io = ImGui::GetIO();

    static float t_x = g_instant_win_x, t_y = g_instant_win_y, t_scale = g_instant_win_scale;
    static bool first = true; if (first) { t_x = g_instant_win_x; t_y = g_instant_win_y; t_scale = g_instant_win_scale; first = false; }
    static bool isDragging = false, isScaling = false; static ImVec2 dragOffset, scaleDragOffset;

    float baseW = 160.0f * g_autoScale, baseH = 60.0f * g_autoScale;
    HandleGridInteraction(g_instant_win_x, g_instant_win_y, g_instant_win_scale, t_x, t_y, t_scale,
                          isDragging, isScaling, dragOffset, scaleDragOffset,
                          baseW + 20.0f * g_autoScale, baseH * 0.5f,
                          -20.0f * g_autoScale, baseH * 0.5f,
                          0, 0, baseW, baseH, g_boardLocked, &g_show_instant_quit_window, true);

    float curW = baseW * g_instant_win_scale, curH = baseH * g_instant_win_scale;
    ImRect bgRect(ImVec2(g_instant_win_x, g_instant_win_y), ImVec2(g_instant_win_x + curW, g_instant_win_y + curH));

    if (!g_boardLocked && alpha > 0.9f) {
        DrawScaleHandle(d, ImVec2(g_instant_win_x + (baseW + 20.0f * g_autoScale) * g_instant_win_scale, g_instant_win_y + baseH * 0.5f * g_instant_win_scale), isScaling);
        DrawCloseHandle(d, ImVec2(g_instant_win_x - 20.0f * g_autoScale * g_instant_win_scale, g_instant_win_y + baseH * 0.5f * g_instant_win_scale), &g_show_instant_quit_window);
    }

    float rounding = curH * 0.5f;
    d->AddRectFilled(bgRect.Min, bgRect.Max, IM_COL32(20, 25, 30, (int)(220 * alpha)), rounding);
    d->AddRect(bgRect.Min, bgRect.Max, IM_COL32(80, 85, 95, (int)(200 * alpha)), rounding, 0, 2.0f * g_autoScale * g_instant_win_scale);

    ImFont* font = g_mainFont ? g_mainFont : ImGui::GetFont();

    float curBtnW = 130.0f * g_autoScale * g_instant_win_scale, curBtnH = 44.0f * g_autoScale * g_instant_win_scale;
    ImVec2 btnPos = ImVec2(g_instant_win_x + (curW - curBtnW) * 0.5f, g_instant_win_y + (curH - curBtnH) * 0.5f);
    ImRect bb(btnPos, btnPos + ImVec2(curBtnW, curBtnH));

    static int confirmState = 0; 
    static float confirmTimer = 0.0f;

    if (confirmState == 1) {
        confirmTimer -= io.DeltaTime;
        if (confirmTimer <= 0.0f) confirmState = 0;
    }

    bool hovered = bb.Contains(io.MousePos);
    if (hovered && ImGui::IsMouseClicked(0) && !isDragging && !isScaling) {
        if (confirmState == 0) {
            confirmState = 1;
            confirmTimer = 3.0f; 
        } else {
            g_Tasks.trigger_surrender.store(true);
            confirmState = 0;
        }
    }

    static float animA = 0.0f;
    animA = ImLerp(animA, confirmState == 1 ? 1.0f : 0.0f, 1.0f - expf(-15.0f * io.DeltaTime));
    float rR = curBtnH * 0.5f;

    ImU32 normalBg = IM_COL32(40, 45, 55, (int)(255 * alpha));
    ImU32 confirmBg = IM_COL32(200, 50, 50, (int)(255 * alpha));
    
    float pulse = (sinf((float)ImGui::GetTime() * 8.0f) + 1.0f) * 0.5f;
    ImU32 normalBorder = hovered ? IM_COL32(150, 150, 150, (int)(255 * alpha)) : IM_COL32(100, 100, 100, (int)(200 * alpha));
    ImU32 confirmBorder = IM_COL32(255, 100 + (int)(100*pulse), 100 + (int)(100*pulse), (int)(255 * alpha));

    auto lerpColor = [](ImU32 c1, ImU32 c2, float t) -> ImU32 {
        int r = (int)( ((c1 >> 0) & 0xFF) * (1-t) + ((c2 >> 0) & 0xFF) * t );
        int g = (int)( ((c1 >> 8) & 0xFF) * (1-t) + ((c2 >> 8) & 0xFF) * t );
        int b = (int)( ((c1 >> 16) & 0xFF) * (1-t) + ((c2 >> 16) & 0xFF) * t );
        int a = (int)( ((c1 >> 24) & 0xFF) * (1-t) + ((c2 >> 24) & 0xFF) * t );
        return IM_COL32(r, g, b, a);
    };

    d->AddRectFilled(bb.Min, bb.Max, lerpColor(normalBg, confirmBg, animA), rR);
    d->AddRect(bb.Min, bb.Max, lerpColor(normalBorder, confirmBorder, animA), rR, 0, 2.0f * g_autoScale * g_instant_win_scale);

    float fSz = 20.0f * g_autoScale * g_instant_win_scale;
    const char* textNormal = (const char*)u8"极速退游";
    const char* textConfirm = (const char*)u8"确认退游?";

    if (animA < 0.99f) {
        ImVec2 tSzNormal = font->CalcTextSizeA(fSz, FLT_MAX, 0.0f, textNormal);
        d->AddText(font, fSz, btnPos + ImVec2((curBtnW - tSzNormal.x)*0.5f, (curBtnH - tSzNormal.y)*0.5f), IM_COL32(255, 255, 255, (int)(255 * alpha * (1.0f - animA))), textNormal);
    }
    if (animA > 0.01f) {
        ImVec2 tSzConfirm = font->CalcTextSizeA(fSz, FLT_MAX, 0.0f, textConfirm);
        d->AddText(font, fSz, btnPos + ImVec2((curBtnW - tSzConfirm.x)*0.5f, (curBtnH - tSzConfirm.y)*0.5f), IM_COL32(255, 255, 255, (int)(255 * alpha * animA)), textConfirm);
    }
}

void DrawSellHeroWindow() {
    if (!g_show_sell_window) return;
    static float alpha = 0.0f; 
    alpha = ImLerp(alpha, g_show_sell_window ? 1.0f : 0.0f, 1.0f - expf(-20.0f * ImGui::GetIO().DeltaTime));
    if (alpha < 0.01f) return;

    ImDrawList* d = ImGui::GetForegroundDrawList();
    ImGuiIO& io = ImGui::GetIO();

    static float t_x = g_sell_win_x, t_y = g_sell_win_y, t_scale = g_sell_win_scale;
    static bool first = true; if (first) { t_x = g_sell_win_x; t_y = g_sell_win_y; t_scale = g_sell_win_scale; first = false; }
    static bool isDragging = false, isScaling = false; static ImVec2 dragOffset, scaleDragOffset;

    float baseW = 120.0f * g_autoScale, baseH = 44.0f * g_autoScale;
    HandleGridInteraction(g_sell_win_x, g_sell_win_y, g_sell_win_scale, t_x, t_y, t_scale,
                          isDragging, isScaling, dragOffset, scaleDragOffset,
                          baseW + 20.0f * g_autoScale, baseH * 0.5f,
                          -20.0f * g_autoScale, baseH * 0.5f,
                          0, 0, baseW, baseH, g_boardLocked, &g_show_sell_window, true);

    float curW = baseW * g_sell_win_scale, curH = baseH * g_sell_win_scale;
    ImRect bgRect(ImVec2(g_sell_win_x, g_sell_win_y), ImVec2(g_sell_win_x + curW, g_sell_win_y + curH));

    if (!g_boardLocked && alpha > 0.9f) {
        DrawScaleHandle(d, ImVec2(g_sell_win_x + (baseW + 20.0f * g_autoScale) * g_sell_win_scale, g_sell_win_y + baseH * 0.5f * g_sell_win_scale), isScaling);
        DrawCloseHandle(d, ImVec2(g_sell_win_x - 20.0f * g_autoScale * g_sell_win_scale, g_sell_win_y + baseH * 0.5f * g_sell_win_scale), &g_show_sell_window);
    }

    float rounding = curH * 0.5f; 
    
    bool hovered = bgRect.Contains(io.MousePos);
    bool clicked = hovered && ImGui::IsMouseClicked(0) && !isDragging && !isScaling;
    bool held = hovered && ImGui::IsMouseDown(0);

    if (clicked) {
        TriggerSellAllHeroes();
    }

    float alpha_factor = held ? 0.8f : (hovered ? 1.0f : 0.7f);
    ImU32 bgColor = IM_COL32(220, 50, 50, (int)(255 * alpha * alpha_factor));

    d->AddRectFilled(bgRect.Min, bgRect.Max, bgColor, rounding);
    d->AddRect(bgRect.Min, bgRect.Max, IM_COL32(255, 100, 100, (int)(200 * alpha)), rounding, 0, 2.0f * g_autoScale * g_sell_win_scale);

    ImFont* font = g_mainFont ? g_mainFont : ImGui::GetFont();
    float fSz = 20.0f * g_autoScale * g_sell_win_scale;
    const char* text = (const char*)u8"卖英雄";
    ImVec2 tSz = font->CalcTextSizeA(fSz, FLT_MAX, 0.0f, text);
    d->AddText(font, fSz, ImVec2(g_sell_win_x + (curW - tSz.x)*0.5f, g_sell_win_y + (curH - tSz.y)*0.5f), IM_COL32(255, 255, 255, (int)(255 * alpha)), text);
}

void DrawHeroSelectionWindow() {
    if (!g_show_hero_list_window) return;

    ImGui::SetNextWindowPos(ImVec2(g_menuX + g_menuW + 10.0f * g_autoScale, g_menuY), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(480.0f * g_autoScale, 650.0f * g_autoScale), ImGuiCond_FirstUseEver);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoSavedSettings;
    if (ImGui::Begin((const char*)u8"自动拿牌图鉴 (随时配置，无需开局)", &g_show_hero_list_window, flags)) {
        
        static float h_scale = 1.0f;
        if (!ImGui::IsWindowCollapsed()) {
            float curW = ImGui::GetWindowSize().x;
            h_scale = std::clamp(curW / (480.0f * g_autoScale), 0.5f, 3.0f);
        }
        ImGui::SetWindowFontScale(h_scale);

        if (!g_pokedex_ready.load()) {
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), (const char*)u8"正在极速扫描本地图片数据，请稍候...");
        } else if (g_local_pokedex.empty()) {
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), (const char*)u8"未扫描到有效的图片数据，请确认 HeroImages.h 已正确包含！");
        } else {
            ImGuiStyle& style = ImGui::GetStyle();
            float window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
            ImDrawList* draw_list = ImGui::GetWindowDrawList();

            static int active_hero_id = -1;
            static double press_time = 0.0; 
            static bool just_opened_popup = false;

            auto renderHeroBlock = [&](int heroId, ImU32 costColor, bool bottomList) {
                ImGui::PushID(heroId + (bottomList ? 50000 : 0));
                
                bool is_selected = g_auto_buy_heroes.count(heroId) > 0;
                auto& binds = g_hero_equip_bindings[heroId];

                float smallEqBoxSz = 24.0f * g_autoScale * h_scale; 
                float smEqSpaceX = 4.0f * g_autoScale * h_scale;
                float smEqSpaceY = 4.0f * g_autoScale * h_scale;
                float smallIconSz = 3.0f * smallEqBoxSz + 2.0f * smEqSpaceX; 
                
                float prioH = 20.0f * g_autoScale * h_scale;
                float triHeight = 28.0f * g_autoScale * h_scale; 
                
                bool is_expanded = g_expanded_equip_heroes.count(heroId) > 0;
                int cols_in_fold = 3;
                int eq_rows = is_expanded ? std::ceil((float)binds.size() / cols_in_fold) : 0;
                if (eq_rows == 0 && is_expanded) eq_rows = 1; 
                
                ImVec2 startPos = ImGui::GetCursorScreenPos();
                ImGui::BeginGroup();
                
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
                
                int prio = g_hero_equip_priority[heroId];
                ImGui::PushID("prio_minus");
                if (ImGui::InvisibleButton("-", ImVec2(smallIconSz * 0.3f, prioH))) {
                    prio = std::max(0, prio - 1);
                    g_hero_equip_priority[heroId] = prio;
                    g_save_timer = 1.0f; SaveConfig();
                }
                ImGui::PopID();
                
                ImGui::SameLine(0, 0);
                ImGui::Dummy(ImVec2(smallIconSz * 0.4f, prioH)); 
                
                ImGui::SameLine(0, 0);
                ImGui::PushID("prio_plus");
                if (ImGui::InvisibleButton("+", ImVec2(smallIconSz * 0.3f, prioH))) {
                    prio = std::min(5, prio + 1);
                    g_hero_equip_priority[heroId] = prio;
                    g_save_timer = 1.0f; SaveConfig();
                }
                ImGui::PopID();
                
                ImGui::PushID("hero");
                ImGui::InvisibleButton("img", ImVec2(smallIconSz, smallIconSz));
                bool isHovered = ImGui::IsItemHovered();
                bool isActive = ImGui::IsItemActive();
                bool isDeactivated = ImGui::IsItemDeactivated();
                ImGui::PopID(); 
                
                if (isActive) {
                    if (active_hero_id != heroId) {
                        active_hero_id = heroId;
                        press_time = ImGui::GetTime();
                        just_opened_popup = false;
                    } else if (!just_opened_popup && (ImGui::GetTime() - press_time) > 0.45) {
                        ImGui::OpenPopup("EquipPopup");
                        just_opened_popup = true;
                    }
                } else if (isDeactivated) {
                    if (active_hero_id== heroId) {
                        if (!just_opened_popup && (ImGui::GetTime() - press_time) <= 0.25) {
                            if (bottomList) {
                                g_auto_buy_heroes.erase(heroId); 
                            } else {
                                if (is_selected) g_auto_buy_heroes.erase(heroId);
                                else g_auto_buy_heroes.insert(heroId);
                            }
                            g_save_timer = 1.0f; SaveConfig();
                        }
                        active_hero_id = -1; 
                    }
                }
                
                ImGui::PushID("tri");
                if (ImGui::InvisibleButton("fold", ImVec2(smallIconSz, triHeight))) {
                    if (is_expanded) g_expanded_equip_heroes.erase(heroId);
                    else g_expanded_equip_heroes.insert(heroId);
                }
                ImGui::PopID();

                ImGui::PopStyleVar(); 

                if (is_expanded) {
                    float eqH = 4.0f * g_autoScale * h_scale + eq_rows * (smallEqBoxSz + smEqSpaceY);
                    ImGui::Dummy(ImVec2(smallIconSz, eqH));
                }
                
                ImGui::EndGroup();

                ImFont* font = g_mainFont ? g_mainFont : ImGui::GetFont();
                
                ImU32 pColor;
                switch(prio) {
                    case 5: pColor = IM_COL32(255, 30, 30, 255); break;  
                    case 4: pColor = IM_COL32(255, 120, 0, 255); break;  
                    case 3: pColor = IM_COL32(255, 200, 0, 255); break;  
                    case 2: pColor = IM_COL32(50, 200, 50, 255); break;  
                    case 1: pColor = IM_COL32(50, 150, 255, 255); break; 
                    default: pColor = IM_COL32(100, 100, 100, 255); break;
                }
                ImVec2 prioBarMin = startPos;
                ImVec2 prioBarMax = ImVec2(startPos.x + smallIconSz, startPos.y + prioH);
                
                draw_list->AddRectFilled(prioBarMin, prioBarMax, IM_COL32(40, 40, 40, 255), 4.0f * g_autoScale * h_scale, ImDrawFlags_RoundCornersTop);
                
                ImVec2 mBtnMax = ImVec2(prioBarMin.x + smallIconSz * 0.3f, prioBarMax.y);
                draw_list->AddRectFilled(prioBarMin, mBtnMax, IM_COL32(70, 70, 70, 255), 4.0f * g_autoScale * h_scale, ImDrawFlags_RoundCornersTopLeft);
                ImVec2 mTxtSz = font->CalcTextSizeA(16.0f*g_autoScale*h_scale, FLT_MAX, 0.0f, "-");
                draw_list->AddText(font, 16.0f*g_autoScale*h_scale, ImVec2(prioBarMin.x + (smallIconSz*0.3f - mTxtSz.x)*0.5f, prioBarMin.y + (prioH - mTxtSz.y)*0.5f), IM_COL32_WHITE, "-");

                ImVec2 pBtnMin = ImVec2(prioBarMin.x + smallIconSz * 0.7f, prioBarMin.y);
                draw_list->AddRectFilled(pBtnMin, prioBarMax, IM_COL32(70, 70, 70, 255), 4.0f * g_autoScale * h_scale, ImDrawFlags_RoundCornersTopRight);
                ImVec2 pTxtSz = font->CalcTextSizeA(16.0f*g_autoScale*h_scale, FLT_MAX, 0.0f, "+");
                draw_list->AddText(font, 16.0f*g_autoScale*h_scale, ImVec2(pBtnMin.x + (smallIconSz*0.3f - pTxtSz.x)*0.5f, pBtnMin.y + (prioH - pTxtSz.y)*0.5f), IM_COL32_WHITE, "+");

                char pText[8];
                if (prio == 0) snprintf(pText, sizeof(pText), "无");
                else snprintf(pText, sizeof(pText), "P%d", prio);
                ImVec2 tSz = font->CalcTextSizeA(14.0f*g_autoScale*h_scale, FLT_MAX, 0.0f, pText);
                draw_list->AddText(font, 14.0f*g_autoScale*h_scale, ImVec2(prioBarMin.x + smallIconSz*0.5f - tSz.x*0.5f, prioBarMin.y + (prioH - tSz.y)*0.5f), pColor, pText);

                ImVec2 heroMin = ImVec2(startPos.x, startPos.y + prioH);
                ImVec2 heroMax = ImVec2(heroMin.x + smallIconSz, heroMin.y + smallIconSz);
                GLuint tex = GetHeroTexture(heroId);
                
                if (tex) draw_list->AddImage((ImTextureID)(intptr_t)tex, heroMin, heroMax);
                else draw_list->AddRectFilled(heroMin, heroMax, IM_COL32(50,50,50,255));

                if (is_selected) {
                    draw_list->AddRect(heroMin, heroMax, IM_COL32(0, 255, 180, 255), 0, 0, 3.0f * g_autoScale * h_scale);
                    draw_list->AddCircleFilled(ImVec2(heroMax.x - 10.0f*g_autoScale*h_scale, heroMax.y - 10.0f*g_autoScale*h_scale), 8.0f*g_autoScale*h_scale, IM_COL32(0, 200, 100, 255));
                } else {
                    draw_list->AddRect(heroMin, heroMax, costColor, 0, 0, 1.5f * g_autoScale * h_scale);
                }

                ImVec2 triMin = ImVec2(startPos.x, heroMax.y);
                ImVec2 triMax = ImVec2(startPos.x + smallIconSz, triMin.y + triHeight);
                draw_list->AddRectFilled(triMin, triMax, IM_COL32(35, 35, 45, 255), 4.0f * g_autoScale * h_scale, ImDrawFlags_RoundCornersBottom);
                
                float triSz = 6.0f * g_autoScale * h_scale; 
                ImVec2 triC = ImVec2(triMin.x + smallIconSz * 0.5f, triMin.y + triHeight * 0.5f);
                if (is_expanded) {
                    draw_list->AddTriangleFilled(triC - ImVec2(triSz, -triSz), triC + ImVec2(triSz, triSz), triC - ImVec2(0, triSz), IM_COL32_WHITE);
                } else {
                    draw_list->AddTriangleFilled(triC - ImVec2(triSz, triSz), triC + ImVec2(triSz, -triSz), triC + ImVec2(0, triSz), IM_COL32(180, 180, 180, 255));
                }

                if (is_expanded) {
                    ImVec2 equipStartPos = ImVec2(startPos.x, triMax.y + 4.0f * g_autoScale * h_scale);
                    int slots_to_draw = std::max((int)binds.size(), cols_in_fold); 
                    for(int k = 0; k < slots_to_draw; k++) {
                        int row = k / cols_in_fold;
                        int col = k % cols_in_fold;
                        ImVec2 eqMin = ImVec2(equipStartPos.x + col * (smallEqBoxSz + smEqSpaceX), 
                                              equipStartPos.y + row * (smallEqBoxSz + smEqSpaceY));
                        ImVec2 eqMax = ImVec2(eqMin.x + smallEqBoxSz, eqMin.y + smallEqBoxSz);
                        
                        if (k < binds.size()) {
                            GLuint eqTex = GetEquipTexture(binds[k]);
                            if (eqTex) draw_list->AddImageRounded((ImTextureID)(intptr_t)eqTex, eqMin, eqMax, ImVec2(0,0), ImVec2(1,1), IM_COL32_WHITE, 2.0f*h_scale);
                            else draw_list->AddRectFilled(eqMin, eqMax, IM_COL32(100,100,100,255), 2.0f*h_scale);
                            draw_list->AddRect(eqMin, eqMax, IM_COL32(150,150,150,200), 2.0f*h_scale, 0, 1.0f);
                            
                            char orderBuf[4]; snprintf(orderBuf, sizeof(orderBuf), "%d", k + 1);
                            draw_list->AddText(font, 13.0f * g_autoScale * h_scale, eqMin + ImVec2(1.0f, 1.0f), IM_COL32(0,0,0,255), orderBuf);
                            draw_list->AddText(font, 13.0f * g_autoScale * h_scale, eqMin, IM_COL32(0,255,150,255), orderBuf);
                        } else {
                            draw_list->AddRectFilled(eqMin, eqMax, IM_COL32(30,30,30,100), 2.0f*h_scale);
                            draw_list->AddRect(eqMin, eqMax, IM_COL32(80,80,80,150), 2.0f*h_scale, 0, 1.0f);
                        }
                    }
                }

                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10 * g_autoScale * h_scale, 10 * g_autoScale * h_scale));
                if (ImGui::BeginPopup("EquipPopup")) {
                    ImGui::SetWindowFontScale(h_scale); 
                    ImGui::TextColored(ImColor(costColor).Value, "为英雄 %d 选择推荐装备 (最多18件，按点击顺序定优先级)", heroId);
                    
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 0.8f));
                    if (ImGui::Button((const char*)u8"一键清空已选装备", ImVec2(-1, 30 * g_autoScale * h_scale))) {
                        binds.clear();
                        g_save_timer = 1.0f; SaveConfig();
                    }
                    ImGui::PopStyleColor();
                    ImGui::Separator();
                    
                    if (!g_equip_pokedex_ready.load()) ImGui::TextColored(ImVec4(1,1,0,1), "扫描中...");
                    else {
                        int cols = 8;
                        for (size_t e = 0; e < g_local_equip_pokedex.size(); e++) {
                            int equipId = g_local_equip_pokedex[e];
                            ImGui::PushID(equipId + 20000);
                            GLuint equipTex = GetEquipTexture(equipId);
                            ImVec2 btnSize = ImVec2(45 * g_autoScale * h_scale, 45 * g_autoScale * h_scale);
                            
                            auto it_bind = std::find(binds.begin(), binds.end(), equipId);
                            bool is_equipped = (it_bind != binds.end());
                            int order_idx = is_equipped ? std::distance(binds.begin(), it_bind) + 1 : 0;
                            
                            if (is_equipped) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0.8f, 0.4f, 0.6f));
                            else ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));

                            ImVec2 btnMin = ImGui::GetCursorScreenPos();
                            if (equipTex != 0) {
                                if (ImGui::ImageButton("##eq2", (ImTextureID)(intptr_t)equipTex, btnSize)) {
                                    if (is_equipped) binds.erase(std::remove(binds.begin(), binds.end(), equipId), binds.end());
                                    else if (binds.size() < 18) binds.push_back(equipId);
                                    g_save_timer = 1.0f; SaveConfig();
                                }
                            } else {
                                if (ImGui::Button(std::to_string(equipId).c_str(), btnSize)) {
                                    if (is_equipped) binds.erase(std::remove(binds.begin(), binds.end(), equipId), binds.end());
                                    else if (binds.size() < 18) binds.push_back(equipId);
                                    g_save_timer = 1.0f; SaveConfig();
                                }
                            }
                            
                            if (is_equipped) {
                                char numBuf[4]; snprintf(numBuf, sizeof(numBuf), "%d", order_idx);
                                draw_list->AddText(font, 16.0f * g_autoScale * h_scale, btnMin + ImVec2(4.0f, 4.0f), IM_COL32(0,0,0,255), numBuf);
                                draw_list->AddText(font, 16.0f * g_autoScale * h_scale, btnMin + ImVec2(2.0f, 2.0f), IM_COL32(0,255,150,255), numBuf);
                            }

                            ImGui::PopStyleColor();
                            if ((e + 1) % cols != 0 && (e + 1) < g_local_equip_pokedex.size()) ImGui::SameLine();
                            ImGui::PopID();
                        }
                    }
                    ImGui::EndPopup();
                }
                ImGui::PopStyleVar();

                if (isHovered) {
                    ImGui::SetTooltip((const char*)u8"单击选定/取消 | 长按改装备");
                }

                float last_button_x2 = ImGui::GetItemRectMax().x;
                float next_button_x2 = last_button_x2 + style.ItemSpacing.x + smallIconSz;
                if (next_button_x2 < window_visible_x2) {
                    ImGui::SameLine();
                }
            };

            for (int cost = 1; cost <= 5; cost++) {
                if (g_local_pokedex[cost].empty()) continue;

                ImU32 costColor;
                switch(cost) {
                    case 1: costColor = IM_COL32(200, 200, 200, 255); break;
                    case 2: costColor = IM_COL32(0, 255, 0, 255); break;
                    case 3: costColor = IM_COL32(0, 200, 255, 255); break;
                    case 4: costColor = IM_COL32(210, 50, 255, 255); break;
                    case 5: costColor = IM_COL32(255, 215, 0, 255); break;
                    default:costColor = IM_COL32(255, 255, 255, 255); break;
                }

                ImGui::PushStyleColor(ImGuiCol_Text, costColor);
                ImGui::Text("%d费英雄", cost);
                ImGui::PopStyleColor();
                ImGui::Separator();

                for (size_t i = 0; i < g_local_pokedex[cost].size(); i++) {
                    renderHeroBlock(g_local_pokedex[cost][i], costColor, false);
                }
                ImGui::NewLine();
                ImGui::Spacing();
            }

            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.0f, 0.85f, 0.55f, 1.0f), (const char*)u8"★ 已选自动拿牌列表 (长按头像弹装 | 点击三角折叠)");
            
            for (int cost = 1; cost <= 5; cost++) {
                std::vector<int> selected_heroes;
                for (int heroId : g_local_pokedex[cost]) {
                    if (g_auto_buy_heroes.count(heroId)) selected_heroes.push_back(heroId);
                }
                if (selected_heroes.empty()) continue;

                ImU32 costColor;
                switch(cost) {
                    case 1: costColor = IM_COL32(200, 200, 200, 255); break;
                    case 2: costColor = IM_COL32(0, 255, 0, 255); break;
                    case 3: costColor = IM_COL32(0, 200, 255, 255); break;
                    case 4: costColor = IM_COL32(210, 50, 255, 255); break;
                    case 5: costColor = IM_COL32(255, 215, 0, 255); break;
                    default:costColor = IM_COL32(255, 255, 255, 255); break;
                }

                ImGui::TextColored(ImColor(costColor), "[%d费]", cost); ImGui::SameLine();

                for (size_t i = 0; i < selected_heroes.size(); i++) {
                    renderHeroBlock(selected_heroes[i], costColor, true);
                }
                ImGui::NewLine();
            }
        }
    }
    ImGui::End();
}

void DrawShopESP(const GameSnapshot* snap) {
    if (!snap) return;
    // 已经移除了自己和敌方商店的单槽位透视调用，这里可以安全留空。
}

void DrawBenchESP(const GameSnapshot* snap) {
    if (!snap) return;
    // 只保留敌方备战席的透视调用
    DrawSingleSlotOverlay("EnemyBench", "敌方备战席", g_esp_enemy_bench, g_enemy_bench_X, g_enemy_bench_Y, g_enemy_bench_Scale, 9, snap->enemyBenchHeroes, IM_COL32(255, 100, 100, 255));
}

void DrawPurePredictEnemy(const GameSnapshot* snap) {
    if (!snap) return;
    static float alpha_raw = 0.0f; alpha_raw = ImLerp(alpha_raw, g_predict_enemy ? 1.0f : 0.0f, 1.0f - expf(-20.0f * ImGui::GetIO().DeltaTime));
    float alpha = alpha_raw * g_global_esp_alpha;
    if (alpha < 0.01f) return;
    ImDrawList* d = ImGui::GetForegroundDrawList();
    static float t_x = g_enemy_X, t_y = g_enemy_Y, t_scale = g_enemy_Scale; static bool first = true; if (first) { t_x = g_enemy_X; t_y = g_enemy_Y; t_scale = g_enemy_Scale; first = false; }
    static bool isDragging = false, isScaling = false; static ImVec2 dragOffset, scaleDragOffset;

    int display_row = 1; bool isAi = false;
    if (snap->nextEnemyPlayerId != -1) {
        auto it = snap->players.find(snap->nextEnemyPlayerId);
        if (it != snap->players.end()) { display_row = it->second.rowIdx; isAi = it->second.isAi; }
    }
    
    HandleGridInteraction(g_enemy_X, g_enemy_Y, g_enemy_Scale, t_x, t_y, t_scale, isDragging, isScaling, dragOffset, scaleDragOffset,
                          25.0f * g_autoScale * 2.5f, 25.0f * g_autoScale, -15.0f * g_autoScale, 25.0f * g_autoScale, 0, 0, 50.0f * g_autoScale, 50.0f * g_autoScale, g_boardLocked, &g_predict_enemy, true);
    
    float drawRadius = 25.0f * g_autoScale * g_enemy_Scale; ImVec2 center(g_enemy_X + drawRadius, g_enemy_Y + drawRadius);
    float r, g, b; ImGui::ColorConvertHSVtoRGB(fmodf((float)ImGui::GetTime() * 0.5f, 1.0f), 0.8f, 1.0f, r, g, b);
    d->AddCircleFilled(center, drawRadius, IM_COL32((int)(r*255), (int)(g*255), (int)(b*255), (int)(255 * alpha)), 32);

    if (isAi) {
        ImFont* font = g_mainFont ? g_mainFont : ImGui::GetFont();
        float fSz = 20.0f * g_autoScale * g_enemy_Scale; 
        ImVec2 tSz = font->CalcTextSizeA(fSz, FLT_MAX, 0.0f, "Ai");
        DrawTextWithStroke(d, font, fSz, center - ImVec2(tSz.x*0.5f, tSz.y*0.5f), IM_COL32(255, 255, 255, (int)(255 * alpha)), IM_COL32(0, 0, 0, (int)(255 * alpha)), "Ai");
    }
    
    if (!g_boardLocked && alpha > 0.9f * g_global_esp_alpha) {
        DrawScaleHandle(d, ImVec2(g_enemy_X + 62.5f * g_autoScale * g_enemy_Scale, g_enemy_Y + 25.0f * g_autoScale * g_enemy_Scale), isScaling);
        DrawCloseHandle(d, ImVec2(g_enemy_X - 15.0f * g_autoScale * g_enemy_Scale, g_enemy_Y + 25.0f * g_autoScale * g_enemy_Scale), &g_predict_enemy);
    }
}

void DrawPurePredictHex(const GameSnapshot* snap) {
    if (!snap) return;
    static float alpha_raw = 0.0f; alpha_raw = ImLerp(alpha_raw, g_predict_hex ? 1.0f : 0.0f, 1.0f - expf(-20.0f * ImGui::GetIO().DeltaTime));
    float alpha = alpha_raw * g_global_esp_alpha;
    if (alpha < 0.01f) return;
    
    ImDrawList* d = ImGui::GetForegroundDrawList();
    static float t_x = g_hex_X, t_y = g_hex_Y, t_scale = g_hex_Scale; static bool first = true; if (first) { t_x = g_hex_X; t_y = g_hex_Y; t_scale = g_hex_Scale; first = false; }
    static bool isDragging = false, isScaling = false; static ImVec2 dragOffset, scaleDragOffset;

    int drawCount = snap->validHexCount > 0 ? snap->validHexCount : 3;
    if (drawCount > 4) drawCount = 4; // 安全限制

    float bW = (drawCount * 60.0f - 15.0f) * g_autoScale; 
    float bH = 45.0f * g_autoScale;

    HandleGridInteraction(g_hex_X, g_hex_Y, g_hex_Scale, t_x, t_y, t_scale, isDragging, isScaling, dragOffset, scaleDragOffset,
                          bW + 15.0f * g_autoScale, bH * 0.5f, -15.0f * g_autoScale, bH * 0.5f, 0, 0, bW, bH, g_boardLocked, &g_predict_hex, true);
    
    ImFont* font = g_mainFont ? g_mainFont : ImGui::GetFont();
    float fSz = 24.0f * g_autoScale * g_hex_Scale;

    for (int i = 0; i < drawCount; i++) {
        int rarity = (snap->validHexCount > 0) ? snap->cachedHexes[i] : 0; 
        ImU32 boxColor = IM_COL32(100, 100, 100, (int)(150 * alpha)); 
        const char* text = "?";
        
        if (rarity == 1) { boxColor = IM_COL32(200, 200, 200, (int)(255 * alpha)); text = "银"; }
        else if (rarity == 2) { boxColor = IM_COL32(255, 215, 0, (int)(255 * alpha)); text = "金"; }
        else if (rarity == 3) { 
            float hue = fmodf((float)ImGui::GetTime() * 0.5f + i * 0.2f, 1.0f);
            float r, g, b; ImGui::ColorConvertHSVtoRGB(hue, 0.8f, 1.0f, r, g, b);
            boxColor = IM_COL32((int)(r*255), (int)(g*255), (int)(b*255), (int)(255 * alpha));
            text = "彩"; 
        }

        ImVec2 pMin(g_hex_X + i * 60.0f * g_autoScale * g_hex_Scale, g_hex_Y);
        ImVec2 pMax(g_hex_X + i * 60.0f * g_autoScale * g_hex_Scale + 45.0f * g_autoScale * g_hex_Scale, g_hex_Y + 45.0f * g_autoScale * g_hex_Scale);
        
        d->AddRectFilled(pMin, pMax, IM_COL32(20, 20, 20, (int)(200 * alpha)), 6.0f * g_autoScale * g_hex_Scale);
        d->AddRect(pMin, pMax, boxColor, 6.0f * g_autoScale * g_hex_Scale, 0, 2.0f * g_autoScale * g_hex_Scale);
        
        ImVec2 tSz = font->CalcTextSizeA(fSz, FLT_MAX, 0.0f, text);
        ImVec2 tPos = ImVec2(pMin.x + (45.0f * g_autoScale * g_hex_Scale - tSz.x) * 0.5f, pMin.y + (45.0f * g_autoScale * g_hex_Scale - tSz.y) * 0.5f);
        DrawTextWithStroke(d, font, fSz, tPos, boxColor, IM_COL32(0,0,0,(int)(255*alpha)), text);
    }
    
    if (!g_boardLocked && alpha > 0.9f * g_global_esp_alpha) {
        DrawScaleHandle(d, ImVec2(g_hex_X + (bW + 15.0f * g_autoScale) * g_hex_Scale, g_hex_Y + bH * 0.5f * g_hex_Scale), isScaling);
        DrawCloseHandle(d, ImVec2(g_hex_X - 15.0f * g_autoScale * g_hex_Scale, g_hex_Y + bH * 0.5f * g_hex_Scale), &g_predict_hex);
    }
}

void DrawLevelOverlay(const GameSnapshot* snap) {
    if (!snap || !g_esp_level) return;
    static float alpha_raw = 0.0f; alpha_raw = ImLerp(alpha_raw, g_esp_level ? 1.0f : 0.0f, 1.0f - expf(-20.0f * ImGui::GetIO().DeltaTime));
    float alpha = alpha_raw * g_global_esp_alpha;
    if (alpha < 0.01f) return;
    ImDrawList* d = ImGui::GetForegroundDrawList();
    static float t_x = g_level_X, t_y = g_level_Y, t_scale = g_level_Scale; static bool first = true; if (first) { t_x = g_level_X; t_y = g_level_Y; t_scale = g_level_Scale; first = false; }
    static bool isDragging = false, isScaling = false; static ImVec2 dragOffset, scaleDragOffset;
    
    float bW = 160.0f * g_autoScale;
    float bH = g_level_Spacing * g_autoScale * 7.0f + 32.0f * g_autoScale;
    
    HandleGridInteraction(g_level_X, g_level_Y, g_level_Scale, t_x, t_y, t_scale, isDragging, isScaling, dragOffset, scaleDragOffset,
                          bW + 20.0f * g_autoScale, bH * 0.5f, -20.0f * g_autoScale, bH * 0.5f, 0, 0, bW, bH, g_boardLocked, &g_esp_level, true);
    
    if (!g_boardLocked && alpha > 0.9f * g_global_esp_alpha) {
        DrawScaleHandle(d, ImVec2(g_level_X + (bW + 20.0f * g_autoScale) * g_level_Scale, g_level_Y + bH * 0.5f * g_level_Scale), isScaling);
        DrawCloseHandle(d, ImVec2(g_level_X - 20.0f * g_autoScale * g_level_Scale, g_level_Y + bH * 0.5f * g_level_Scale), &g_esp_level);
    }
    
    ImFont* font = ImGui::GetFont(); float fSz = ImGui::GetFontSize() * g_level_Scale;

    for (int i = 0; i < 8; i++) {
        float cy = g_level_Y + i * g_level_Spacing * g_autoScale * g_level_Scale; 
        int cm = 0, cl = 0, cw = 0, clost = 0;
        
        for (const auto& pair : snap->players) {
            if (pair.second.rowIdx == i + 1) { 
                cm = pair.second.money; 
                cl = pair.second.level; 
                cw = pair.second.winStreak; 
                clost = pair.second.loseStreak; 
                break; 
            }
        }
        char buf[16]; 
        
        snprintf(buf, sizeof(buf), "%d", cm); 
        ImVec2 tSz = font->CalcTextSizeA(fSz, FLT_MAX, 0.0f, buf);
        ImVec2 posM = ImVec2(g_level_X + (40.0f * g_autoScale * g_level_Scale - tSz.x)*0.5f, cy + (32.0f * g_autoScale * g_level_Scale - tSz.y)*0.5f);
        DrawTextWithStroke(d, font, fSz, posM, IM_COL32(255, 215, 0, (int)(255 * alpha)), IM_COL32(0, 0, 0, (int)(255 * alpha)), buf);
        
        snprintf(buf, sizeof(buf), "%d", cl); 
        tSz = font->CalcTextSizeA(fSz, FLT_MAX, 0.0f, buf);
        ImVec2 posL = ImVec2(g_level_X + 50.0f * g_autoScale * g_level_Scale + (40.0f * g_autoScale * g_level_Scale - tSz.x)*0.5f, cy + (32.0f * g_autoScale * g_level_Scale - tSz.y)*0.5f);
        DrawTextWithStroke(d, font, fSz, posL, IM_COL32(0, 255, 180, (int)(255 * alpha)), IM_COL32(0, 0, 0, (int)(255 * alpha)), buf);

        char streakBuf[16] = "-";
        ImU32 streakColor = IM_COL32(100, 100, 100, (int)(200 * alpha));
        if (cw > 0) {
            snprintf(streakBuf, sizeof(streakBuf), "%d", cw);
            streakColor = IM_COL32(255, 80, 50, (int)(255 * alpha)); 
        } else if (clost > 0) {
            snprintf(streakBuf, sizeof(streakBuf), "%d", clost);
            streakColor = IM_COL32(50, 180, 255, (int)(255 * alpha)); 
        }
        
        tSz = font->CalcTextSizeA(fSz, FLT_MAX, 0.0f, streakBuf);
        ImVec2 posS = ImVec2(g_level_X + 100.0f * g_autoScale * g_level_Scale + (60.0f * g_autoScale * g_level_Scale - tSz.x)*0.5f, cy + (32.0f * g_autoScale * g_level_Scale - tSz.y)*0.5f);
        DrawTextWithStroke(d, font, fSz, posS, streakColor, IM_COL32(0, 0, 0, (int)(255 * alpha)), streakBuf);
    }
}

void DrawWarningOverlay(const GameSnapshot* snap) {
    if (!snap || !g_card_warning) return;
    static float alpha_raw = 0.0f; alpha_raw = ImLerp(alpha_raw, g_card_warning ? 1.0f : 0.0f, 1.0f - expf(-20.0f * ImGui::GetIO().DeltaTime));
    float alpha = alpha_raw * g_global_esp_alpha;
    if (alpha < 0.01f) return;

    ImDrawList* d = ImGui::GetForegroundDrawList();
    static float t_x = g_warn_X, t_y = g_warn_Y, t_scale = g_warn_Scale; 
    static bool first = true; if (first) { t_x = g_warn_X; t_y = g_warn_Y; t_scale = g_warn_Scale; first = false; }
    static bool isDragging = false, isScaling = false; static ImVec2 dragOffset, scaleDragOffset;

    float baseSz = 40.0f * g_autoScale, gap = 2.0f * g_autoScale; 
    float bH = g_warn_Spacing * g_autoScale * 7.0f + 32.0f * g_autoScale;
    
    HandleGridInteraction(g_warn_X, g_warn_Y, g_warn_Scale, t_x, t_y, t_scale, isDragging, isScaling, dragOffset, scaleDragOffset,
                          60.0f * g_autoScale, bH * 0.5f, -220.0f * g_autoScale, bH * 0.5f, -200.0f * g_autoScale, 0, 40.0f * g_autoScale, bH, g_boardLocked, &g_card_warning, true);

    if (!g_boardLocked && alpha > 0.9f * g_global_esp_alpha) {
        DrawScaleHandle(d, ImVec2(g_warn_X + 60.0f * g_autoScale * g_warn_Scale, g_warn_Y + bH * 0.5f * g_warn_Scale), isScaling);
        DrawCloseHandle(d, ImVec2(g_warn_X - 220.0f * g_autoScale * g_warn_Scale, g_warn_Y + bH * 0.5f * g_warn_Scale), &g_card_warning);
    }

    ImFont* font = g_mainFont ? g_mainFont : ImGui::GetFont();
    float fSz = 20.0f * g_autoScale * g_warn_Scale;

    for (const auto& w : snap->playerWarnings) {
        int displayRow = w.rowIdx - 1;
        float cy = g_warn_Y + displayRow * g_warn_Spacing * g_autoScale * g_warn_Scale;

        int drawIdx = 0;
        for (const auto& kv : w.heroCounts) {
            int baseId = kv.first, count = kv.second;
            if (count < g_warning_threshold) continue;
            
            int cost = 1;
            auto pit = snap->cardPoolData.find(baseId);
            if (pit != snap->cardPoolData.end()) {
                cost = pit->second.cost;
            } else {
                if (baseId >= 100) cost = (baseId / 10) % 10;
                else if (baseId >= 10) cost = baseId % 10;
            }
            
            if (cost < 1 || cost > 5 || !g_warning_tiers[cost]) continue;

            ImU32 borderColor = IM_COL32(255, 255, 255, (int)(255 * alpha));
            switch(cost) {
                case 1: borderColor = IM_COL32(200, 200, 200, (int)(255 * alpha)); break; 
                case 2: borderColor = IM_COL32(0, 255, 0, (int)(255 * alpha)); break;   
                case 3: borderColor = IM_COL32(0, 200, 255, (int)(255 * alpha)); break; 
                case 4: borderColor = IM_COL32(210, 50, 255, (int)(255 * alpha)); break; 
                case 5: borderColor = IM_COL32(255, 215, 0, (int)(255 * alpha)); break; 
            }

            float cx = g_warn_X - drawIdx * (baseSz + gap) * g_warn_Scale;
            ImVec2 pMin = ImVec2(cx, cy), pMax = ImVec2(cx + baseSz * g_warn_Scale, cy + baseSz * g_warn_Scale);

            GLuint tex = GetHeroTexture(baseId);
            if (tex) {
                d->AddImageRounded((ImTextureID)(intptr_t)tex, pMin, pMax, ImVec2(0,0), ImVec2(1,1), IM_COL32(255,255,255, (int)(255*alpha)), 6.0f * g_autoScale * g_warn_Scale);
            } else {
                d->AddRectFilled(pMin, pMax, IM_COL32(30,30,30, (int)(200*alpha)), 6.0f * g_autoScale * g_warn_Scale);
                char idBuf[16]; snprintf(idBuf, sizeof(idBuf), "%d", baseId);
                float idFSz = 14.0f * g_autoScale * g_warn_Scale;
                ImVec2 idSz = font->CalcTextSizeA(idFSz, FLT_MAX, 0.0f, idBuf);
                d->AddText(font, idFSz, ImVec2(cx + (baseSz*g_warn_Scale - idSz.x)*0.5f, cy + (baseSz*g_warn_Scale - idSz.y)*0.5f), borderColor, idBuf);
            }
            d->AddRect(pMin, pMax, borderColor, 6.0f * g_autoScale * g_warn_Scale, 0, 2.0f * g_autoScale * g_warn_Scale);

            char cntBuf[16]; snprintf(cntBuf, sizeof(cntBuf), "%d张", count);
            ImVec2 tSz = font->CalcTextSizeA(fSz, FLT_MAX, 0.0f, cntBuf);
            ImVec2 tPos = ImVec2(cx + (baseSz*g_warn_Scale - tSz.x)*0.5f, cy + baseSz*g_warn_Scale - tSz.y * 0.2f);
            
            DrawTextWithStroke(d, font, fSz, tPos, borderColor, IM_COL32(0,0,0, (int)(255*alpha)), cntBuf);
            drawIdx++;
        }
    }
}

void DrawBoard(const GameSnapshot* snap) {
    if (!snap || !g_esp_board) return;

    ImDrawList* d = ImGui::GetForegroundDrawList();
    static float t_x = g_startX, t_y = g_startY, t_scale = g_boardManualScale;
    static bool firstFrame = true;
    if (firstFrame) { t_x = g_startX; t_y = g_startY; t_scale = g_boardManualScale; firstFrame = false; }
    static bool isDragging = false, isScaling = false;
    static ImVec2 dragOffset, scaleDragOffset;   
    
    float baseSz = 38.0f * g_boardScale * g_autoScale;
    float baseXStep = baseSz * 1.73205f, baseYStep = baseSz * 1.5f;
    float hitMinY = -baseSz * 2.0f; 

    HandleGridInteraction(g_startX, g_startY, g_boardManualScale, t_x, t_y, t_scale,
                          isDragging, isScaling, dragOffset, scaleDragOffset,
                          -6.5f * baseXStep, 3.5f * baseYStep, 0.5f * baseXStep, 3.5f * baseYStep,  
                          -7.5f * baseXStep, hitMinY, 1.0f * baseXStep, 4.0f * baseYStep,  
                          g_boardLocked, &g_esp_board, true);
    
    static float alpha_raw = 0.0f; 
    alpha_raw = ImLerp(alpha_raw, g_esp_board ? 1.0f : 0.0f, 1.0f - expf(-20.0f * ImGui::GetIO().DeltaTime));
    float final_alpha = alpha_raw * g_global_esp_alpha; 
    if (final_alpha < 0.01f) return;
    
    float curSz = baseSz * g_boardManualScale, curXStep = baseXStep * g_boardManualScale, curYStep = baseYStep * g_boardManualScale;
    float time = (float)ImGui::GetTime(); 

    if (!g_boardLocked) {
        DrawScaleHandle(d, ImVec2(g_startX - 6.5f * baseXStep * g_boardManualScale, g_startY + 3.5f * baseYStep * g_boardManualScale), isScaling);
        DrawCloseHandle(d, ImVec2(g_startX + 0.5f * baseXStep * g_boardManualScale, g_startY + 3.5f * baseYStep * g_boardManualScale), &g_esp_board);
    }

    std::string display_name = "未知";

    if (g_predict_enemy && snap->nextEnemyPlayerId != -1) {
        auto it = snap->players.find(snap->nextEnemyPlayerId);
        if (it != snap->players.end() && !it->second.name.empty() && it->second.name != "Unknown") {
            display_name = it->second.name;
        }
    }

    if (g_predict_enemy) {
        std::string full_text = std::string("下场对手预测: ") + display_name;
        ImFont* font = g_mainFont ? g_mainFont : ImGui::GetFont();
        float fontSize = 24.0f * g_autoScale * g_predictNameScale; 
        ImVec2 tSz = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, full_text.c_str());
        
        static float t_name_x = g_predictNameX, t_name_y = g_predictNameY, t_name_scale = g_predictNameScale;
        static bool first_name = true; 
        if (first_name) { t_name_x = g_predictNameX; t_name_y = g_predictNameY; t_name_scale = g_predictNameScale; first_name = false; }
        static bool isNameDragging = false, isNameScaling = false; 
        static ImVec2 nameDragOffset, nameScaleDragOffset;
        
        HandleGridInteraction(g_predictNameX, g_predictNameY, g_predictNameScale, t_name_x, t_name_y, t_name_scale,
                              isNameDragging, isNameScaling, nameDragOffset, nameScaleDragOffset,
                              tSz.x / g_predictNameScale + 20.0f * g_autoScale, tSz.y / g_predictNameScale * 0.5f,
                              0, 0, 0, 0, tSz.x / g_predictNameScale, tSz.y / g_predictNameScale, 
                              g_boardLocked, nullptr, true);
                              
        if (!g_boardLocked && final_alpha > 0.9f * g_global_esp_alpha) {
            DrawScaleHandle(d, ImVec2(g_predictNameX + (tSz.x / g_predictNameScale + 20.0f * g_autoScale) * g_predictNameScale, g_predictNameY + (tSz.y / g_predictNameScale * 0.5f) * g_predictNameScale), isNameScaling);
        }

        ImVec2 textPos(g_predictNameX, g_predictNameY);
        float r, g, b; ImGui::ColorConvertHSVtoRGB(fmodf(time * 0.4f, 1.0f), 0.5f, 1.0f, r, g, b);
        DrawTextWithStroke(d, font, fontSize, textPos, IM_COL32((int)(r*255), (int)(g*255), (int)(b*255), (int)(255 * final_alpha)), IM_COL32(0,0,0, (int)(255 * final_alpha)), full_text.c_str());
    }

    for(int y = 0; y < 4; y++) {
        for(int x = 0; x < 7; x++) {
            float cx = g_startX - x * curXStep - (y % 2 == 1 ? curXStep * 0.5f : 0);
            float cy = g_startY + y * curYStep;
            
            bool hasHero = false;
            int cellCost = -1, cellBaseHeroId = -1, cellStarLevel = 1, cellHeroId = -1, cellEquipCount = 0;

            if (g_predict_enemy) {
                for (const auto& hero : snap->currentEnemyHeroes) {
                    if (hero.x == x && hero.y == y) { 
                        hasHero = true; cellHeroId = hero.heroId; cellEquipCount = hero.equipCount; 
                        FastParseHeroId(hero.heroId, cellBaseHeroId, cellCost, cellStarLevel);
                        break; 
                    }
                }
            }

            ImU32 hexColor = IM_COL32(255, 255, 255, (int)(150 * final_alpha));
            if (hasHero && cellCost != -1) {
                switch(cellCost) {
                    case 1: hexColor = IM_COL32(200, 200, 200, (int)(220 * final_alpha)); break; 
                    case 2: hexColor = IM_COL32(0, 255, 0, (int)(220 * final_alpha)); break;     
                    case 3: hexColor = IM_COL32(0, 200, 255, (int)(220 * final_alpha)); break;   
                    case 4: hexColor = IM_COL32(210, 50, 255, (int)(220 * final_alpha)); break;  
                    case 5: hexColor = IM_COL32(255, 215, 0, (int)(220 * final_alpha)); break;   
                    default:hexColor = IM_COL32(255, 255, 255, (int)(220 * final_alpha)); break;
                }
            }

            if (hasHero) {
                GLuint tex = GetHeroTexture(cellBaseHeroId);
                float imgSz = curSz * 1.5f; 
                ImVec2 pMin = ImVec2(cx - imgSz*0.5f, cy - imgSz*0.5f), pMax = ImVec2(cx + imgSz*0.5f, cy + imgSz*0.5f);
                
                if (tex != 0) d->AddImageRounded((ImTextureID)(intptr_t)tex, pMin, pMax, ImVec2(0,0), ImVec2(1,1), IM_COL32(255, 255, 255, (int)(255 * final_alpha)), imgSz*0.5f);
                else d->AddCircleFilled(ImVec2(cx, cy), curSz * 0.4f, IM_COL32(0,0,0, (int)(255 * final_alpha)));
            }

            ImVec2 pts[6];
            for(int i = 0; i < 6; i++) {
                float a = (60.0f * i - 30.0f) * (M_PI / 180.0f);
                pts[i] = ImVec2(cx + curSz * cosf(a), cy + curSz * sinf(a));
            }
            d->AddPolyline(pts, 6, hexColor, ImDrawFlags_Closed, 12.0f * g_autoScale * g_boardManualScale);

            if (hasHero) {
                char infoBuf[16]; snprintf(infoBuf, sizeof(infoBuf), "%d/%d", cellEquipCount, cellStarLevel);
                ImFont* font = g_mainFont ? g_mainFont : ImGui::GetFont();
                float fSzInfo = 70.0f * g_autoScale * g_boardManualScale; 
                ImVec2 tSzInfo = font->CalcTextSizeA(fSzInfo, FLT_MAX, 0.0f, infoBuf);
                ImVec2 tPosInfo = ImVec2(cx - tSzInfo.x * 0.5f, cy + curSz * 1.5f * 0.10f);
                DrawTextWithStroke(d, font, fSzInfo, tPosInfo, hexColor | ((int)(255 * final_alpha) << 24), IM_COL32(0,0,0, (int)(255 * final_alpha)), infoBuf);
            }
        }
    }
}

void DrawCardPool(const GameSnapshot* snap) {
    if (!snap) return;
    static float alpha_raw = 0.0f; alpha_raw = ImLerp(alpha_raw, g_show_card_pool ? 1.0f : 0.0f, 1.0f - expf(-20.0f * ImGui::GetIO().DeltaTime));
    float final_alpha = alpha_raw * g_global_esp_alpha;
    if (final_alpha < 0.01f) return;

    ImDrawList* d = ImGui::GetForegroundDrawList(); ImGuiIO& io = ImGui::GetIO();
    
    std::vector<CardInfo> normalGroups[6];
    std::vector<CardInfo> warningGroups[6];
    for (const auto& pair : snap->cardPoolData) {
        int cost = pair.second.cost;
        if (cost >= 1 && cost <= 5 && g_show_pool_tiers[cost]) {
            if (g_pool_warning_enable && g_pool_warning_tiers[cost] && pair.second.remaining <= g_pool_warning_count) {
                warningGroups[cost].push_back(pair.second);
            } else {
                normalGroups[cost].push_back(pair.second);
            }
        }
    }
    
    for(int c = 1; c <= 5; c++) {
        std::sort(normalGroups[c].begin(), normalGroups[c].end(), [](const CardInfo& a, const CardInfo& b) { return a.heroId < b.heroId; });
        std::sort(warningGroups[c].begin(), warningGroups[c].end(), [](const CardInfo& a, const CardInfo& b) { return a.heroId < b.heroId; });
    }
    
    static float t_x = g_cardPoolX, t_y = g_cardPoolY, t_scaleX = g_cardPoolScaleX, t_scaleY = g_cardPoolScaleY; 
    static bool first = true; if (first) { t_x = g_cardPoolX; t_y = g_cardPoolY; t_scaleX = g_cardPoolScaleX; t_scaleY = g_cardPoolScaleY; first = false; }
    
    static float current_cols = g_card_pool_cols;
    current_cols = ImLerp(current_cols, (float)g_card_pool_cols, 1.0f - expf(-15.0f * io.DeltaTime));
    int draw_cols = std::max(1, (int)std::ceil(current_cols));

    float baseImgSz = 45.0f * g_autoScale, gap = 5.0f * g_autoScale;
    
    int total_rows = 0;
    int max_items_in_row = 0;
    for(int cost = 1; cost <= 5; cost++) {
        int n_size = normalGroups[cost].size();
        int w_size = warningGroups[cost].size();
        if(n_size == 0 && w_size == 0) continue;
        
        if (n_size > 0) {
            total_rows += std::ceil((float)n_size / draw_cols);
            max_items_in_row = std::max(max_items_in_row, std::min(n_size, draw_cols));
        }
        if (w_size > 0) {
            total_rows += std::ceil((float)w_size / draw_cols);
            max_items_in_row = std::max(max_items_in_row, std::min(w_size, draw_cols));
        }
    }
    if(total_rows == 0) return; 

    float totalW = max_items_in_row * baseImgSz + std::max(0, max_items_in_row - 1) * gap; 
    float totalH = total_rows * baseImgSz + std::max(0, total_rows - 1) * gap;
    
    static bool isDragging = false, isScaling = false; static ImVec2 dragOffset, scaleDragOffset;
    if (g_show_card_pool) {
        HandleGridInteractionXY(g_cardPoolX, g_cardPoolY, g_cardPoolScaleX, g_cardPoolScaleY, t_x, t_y, t_scaleX, t_scaleY,
                              isDragging, isScaling, dragOffset, scaleDragOffset, totalW + 10.0f * g_autoScale, totalH + 10.0f * g_autoScale, 
                              -15.0f * g_autoScale, -15.0f * g_autoScale, -15.0f * g_autoScale, -15.0f * g_autoScale, totalW + 15.0f * g_autoScale, totalH + 15.0f * g_autoScale, g_boardLocked, &g_show_card_pool);
    }
    
    if (!g_boardLocked && final_alpha > 0.9f * g_global_esp_alpha) {
        DrawScaleHandle(d, ImVec2(g_cardPoolX + (totalW + 10.0f * g_autoScale) * g_cardPoolScaleX, g_cardPoolY + (totalH + 10.0f * g_autoScale) * g_cardPoolScaleY), isScaling);
        DrawCloseHandle(d, ImVec2(g_cardPoolX - 15.0f * g_autoScale * g_cardPoolScaleX, g_cardPoolY - 15.0f * g_autoScale * g_cardPoolScaleY), &g_show_card_pool);
    }

    ImFont* font = g_mainFont ? g_mainFont : ImGui::GetFont(); 
    float avgScale = (g_cardPoolScaleX + g_cardPoolScaleY) * 0.5f, fSz = 18.0f * g_autoScale * avgScale;
    float fa = final_alpha * g_cardPoolAlpha; 
    int aInt = (int)(255 * fa); if (aInt <= 0) return;

    int current_row_offset = 0;
    for(int cost = 1; cost <= 5; cost++) {
        if(normalGroups[cost].empty() && warningGroups[cost].empty()) continue;
        
        ImU32 bcNormal;
        switch(cost) { 
            case 1: bcNormal = IM_COL32(169,169,169,aInt); break; case 2: bcNormal = IM_COL32(0,255,0,aInt); break; 
            case 3: bcNormal = IM_COL32(0,150,255,aInt); break; case 4: bcNormal = IM_COL32(200,0,255,aInt); break; 
            case 5: bcNormal = IM_COL32(255,215,0,aInt); break; default:bcNormal = IM_COL32(255,255,255,aInt); 
        }
        ImU32 bcWarning = IM_COL32(255, 50, 50, aInt); 
        
        auto drawGroup = [&](const std::vector<CardInfo>& group, ImU32 bc) {
            if(group.empty()) return;
            for (size_t i = 0; i < group.size(); i++) {
                int c = i % draw_cols; 
                int r = current_row_offset + (i / draw_cols);
                const auto& card = group[i]; 

                float x = g_cardPoolX + c * (baseImgSz * g_cardPoolScaleX + gap * g_cardPoolScaleX); 
                float y = g_cardPoolY + r * (baseImgSz * g_cardPoolScaleY + gap * g_cardPoolScaleY);
                
                GLuint tex = GetHeroTexture(card.heroId); float round = 6.0f * g_autoScale * avgScale;
                if (tex != 0) d->AddImageRounded((ImTextureID)(intptr_t)tex, ImVec2(x, y), ImVec2(x + baseImgSz*g_cardPoolScaleX, y + baseImgSz*g_cardPoolScaleY), ImVec2(0,0), ImVec2(1,1), IM_COL32(255, 255, 255, aInt), round);
                else {
                    d->AddRectFilled(ImVec2(x, y), ImVec2(x + baseImgSz*g_cardPoolScaleX, y + baseImgSz*g_cardPoolScaleY), IM_COL32(20, 20, 20, aInt), round);
                    char idBuf[16]; snprintf(idBuf, sizeof(idBuf), "%d", card.heroId); float idFSz = 14.0f * g_autoScale * avgScale; ImVec2 idSz = font->CalcTextSizeA(idFSz, FLT_MAX, 0.0f, idBuf);
                    d->AddText(font, idFSz, ImVec2(x + (baseImgSz*g_cardPoolScaleX - idSz.x)*0.5f, y + (baseImgSz*g_cardPoolScaleY - idSz.y)*0.5f), bc, idBuf);
                }
                
                float borderThick = (bc == bcWarning) ? (2.5f * g_autoScale * avgScale) : (1.0f * g_autoScale * avgScale);
                if (bc == bcWarning) {
                    float pulse = (sinf((float)ImGui::GetTime() * 8.0f) + 1.0f) * 0.5f; 
                    bc = IM_COL32(255, 50 + (int)(100*pulse), 50 + (int)(100*pulse), aInt); 
                    d->AddRect(ImVec2(x, y), ImVec2(x + baseImgSz*g_cardPoolScaleX, y + baseImgSz*g_cardPoolScaleY), IM_COL32(255,0,0, (int)(aInt*0.4f)), round, 0, borderThick + 2.0f);
                }
                d->AddRect(ImVec2(x, y), ImVec2(x + baseImgSz*g_cardPoolScaleX, y + baseImgSz*g_cardPoolScaleY), bc, round, ImDrawFlags_RoundCornersAll, borderThick);

                char buf[32]; snprintf(buf, sizeof(buf), "%d/%d", card.remaining, card.total); 
                ImVec2 tSz = font->CalcTextSizeA(fSz, FLT_MAX, 0.0f, buf);
                ImVec2 tPos = ImVec2(x + (baseImgSz*g_cardPoolScaleX - tSz.x)*0.5f, y + baseImgSz*g_cardPoolScaleY - tSz.y - 2.0f);
                ImU32 textColor = (bc == bcWarning) ? IM_COL32(255,100,100, aInt) : (bc | (aInt << 24));
                DrawTextWithStroke(d, font, fSz, tPos, textColor, IM_COL32(0,0,0, aInt), buf);
            }
            current_row_offset += std::ceil((float)group.size() / draw_cols);
        };

        drawGroup(normalGroups[cost], bcNormal);
        drawGroup(warningGroups[cost], bcWarning);
    }
}

void DrawDebugWindow(const GameSnapshot* snap) {
    if (!snap || !g_show_debug_window) return;
    ImGui::SetNextWindowSize(ImVec2(550 * g_autoScale, 600 * g_autoScale), ImGuiCond_FirstUseEver);
    if (ImGui::Begin((const char*)u8"调试日志 & 全功能偏移链监控", &g_show_debug_window)) {
        
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), (const char*)u8"当前渲染帧率 (FPS): %.1f", ImGui::GetIO().Framerate);
        ImGui::Separator();

        if (ImGui::BeginTabBar("DebugTabs")) {

            if (ImGui::BeginTabItem((const char*)u8"3D追踪与装备偏移调试")) {
                auto InputHex = [](const char* label, int* v) {
                    int step = 1, step_fast = 16;
                    ImGui::InputScalar(label, ImGuiDataType_S32, v, &step, &step_fast, "%X", ImGuiInputTextFlags_CharsHexadecimal);
                };

                ImGui::TextColored(ImVec4(1, 0.8f, 0.2f, 1), (const char*)u8"【装备栏偏移调节】");
                InputHex("Equip Base (0x2e8)", &g_off_equip_base);
                InputHex("Equip Array (0x10)", &g_off_equip_arr);
                InputHex("Equip Count (0x18)", &g_off_equip_count); 
                InputHex("Equip Head (0x20)", &g_off_equip_head); 
                InputHex("Equip Step (0x8)", &g_off_equip_step);  
                InputHex("Equip ID (0x14)", &g_off_equip_id);
                
                ImGui::Separator();
                ImGui::TextColored(ImVec4(1, 0.8f, 0.2f, 1), (const char*)u8"【场上英雄实体与3D坐标偏移调节】");
                InputHex("Hero Dyn Base (0x2b0)", &g_off_hero_dyn);
                InputHex("Hero Array (0x20)", &g_off_hero_arr);
                InputHex("Hero Count (0x18)", &g_off_hero_count);   
                InputHex("Hero Head (0x20)", &g_off_hero_head);   
                InputHex("Hero Step (0x8)", &g_off_hero_step);    
                InputHex("Hero ID (0x108)", &g_off_hero_id);
                InputHex("UnitData (0x48)", &g_off_hero_unit);
                InputHex("BattleUnit (0x20)", &g_off_unit_battle);
                InputHex("Pos X (0x238)", &g_off_battle_posx);
                
                ImGui::Separator();
                ImGui::TextColored(ImVec4(1, 0.5f, 0.5f, 1), (const char*)u8"【穿拆装备可视化校准】");
                if (ImGui::Checkbox((const char*)u8"显示[穿拆装备]辅助圈 (直接在屏幕拖拽英雄上的绿圈)", &g_show_equip_ui_debug)) {
                    if (g_show_equip_ui_debug) g_show_sell_ui_debug = false;
                }
                
                ImGui::Separator();
                ImGui::TextColored(ImVec4(1, 0.5f, 0.8f, 1), (const char*)u8"【卖出英雄可视化校准】");
                if (ImGui::Checkbox((const char*)u8"显示[卖出英雄]连线与圈 (拖拽英雄身上的粉圈或底部的红圈)", &g_show_sell_ui_debug)) {
                    if (g_show_sell_ui_debug) g_show_equip_ui_debug = false;
                }
                
                ImGui::Separator();
                ImGui::TextColored(ImVec4(0.8f, 1.0f, 0.8f, 1), (const char*)u8"【滑动动作控制】");
                ImGui::Checkbox((const char*)u8"开启滑动动画视觉反馈", &g_show_swipe_visuals);
                ImGui::SliderInt((const char*)u8"按下后停留时间 (ms)", &g_swipe_down_delay_ms, 0, 100);
                ImGui::SliderInt((const char*)u8"滑动拖拽步数 (0为瞬间移动)", &g_swipe_steps, 0, 50);
                ImGui::SliderInt((const char*)u8"单步拖拽延迟 (ms)", &g_swipe_step_delay_ms, 0, 50);
                ImGui::SliderInt((const char*)u8"释放前停留时间 (ms)", &g_swipe_up_delay_ms, 0, 100);
                ImGui::SliderInt((const char*)u8"动作执行后额外缓冲 (ms)", &g_equip_extra_cooldown_ms, 0, 500);

                ImGui::Separator();
                if (ImGui::Button((const char*)u8"保存上述所有配置", ImVec2(180 * g_autoScale, 30 * g_autoScale))) { SaveConfig(); g_save_timer = 1.5f; }

                ImGui::Separator();
                ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), (const char*)u8"【寻址状态实时输出监控】");
                ImGui::Text("Main Camera: 0x%lX", snap->mainCamera);

                ImGui::TextColored(ImVec4(0.8f, 0.6f, 1.0f, 1.0f), (const char*)u8"★ 左侧装备数组 (0-19):");
                if (snap->myItems.empty()) {
                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), (const char*)u8"暂未读取到装备...");
                } else {
                    std::string eqRow1 = "";
                    std::string eqRow2 = "";
                    for (int i = 0; i < snap->myItems.size(); i++) {
                        if (i < 10) eqRow1 += std::to_string(snap->myItems[i]) + " ";
                        else eqRow2 += std::to_string(snap->myItems[i]) + " ";
                    }
                    ImGui::TextWrapped("列1(0-9): %s", eqRow1.c_str());
                    ImGui::TextWrapped("列2(10-19): %s", eqRow2.c_str());
                }
                ImGui::Separator();

                ImGui::BeginChild("TracesScroll", ImVec2(0, 0), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);
                if (snap->debugHeroTraces.empty()) {
                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), (const char*)u8"暂未读取到英雄实体...");
                } else {
                    for (const auto& t : snap->debugHeroTraces) {
                        ImGui::TextColored(ImVec4(1,1,0,1), "[槽位 %d] HeroID: %d", t.index, t.heroId);
                        ImGui::Text(" HeroObj: 0x%lX", t.heroObj);
                        ImGui::Text(" UnitData: 0x%lX", t.unitData);
                        ImGui::Text(" BattleUnit: 0x%lX", t.battleUnit);
                        ImGui::TextColored(ImVec4(0,1,1,1), " 3D World: (%.1f, %.1f, %.1f)", t.worldPos.x, t.worldPos.y, t.worldPos.z);
                        ImGui::TextColored(ImVec4(0,1,0,1), " 2D Screen: (%.1f, %.1f)", t.screenPos.x, t.screenPos.y);
                        ImGui::Separator();
                    }
                }
                ImGui::EndChild();

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem((const char*)u8"自动拿牌/刷新(UI组件)状态")) {
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), (const char*)u8"【HeroRoot 0x46cce08 拦截列表 - 自动拿牌】");
                ImGui::Separator();
                for (int i = 0; i < 5; i++) {
                    if (g_shop_slot_instances[i] != 0) ImGui::TextColored(ImVec4(0.6f, 0.9f, 1.0f, 1.0f), "[买牌槽位 %d] UI 实例 x0: 0x%lX", i, g_shop_slot_instances[i]);
                    else ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "[买牌槽位 %d] 等待商店刷新捕获...", i);
                }
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem((const char*)u8"商店/备战席偏移调试")) {
                ImGui::TextColored(ImVec4(1, 0.8f, 0.2f, 1), (const char*)u8"【动态偏移调节】");
                auto InputHex = [](const char* label, int* v) {
                    int step = 1, step_fast = 16;
                    ImGui::InputScalar(label, ImGuiDataType_S32, v, &step, &step_fast, "%X", ImGuiInputTextFlags_CharsHexadecimal);
                };

                InputHex("Shop Base", &g_off_shop_base); InputHex("Bench Base", &g_off_bench_base);
                InputHex("Base -> p20", &g_off_shop_p20); InputHex("p20 -> Entries", &g_off_shop_entries);
                if (ImGui::Button((const char*)u8"保存上述偏移配置", ImVec2(180 * g_autoScale, 30 * g_autoScale))) { SaveConfig(); g_save_timer = 1.5f; }
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem((const char*)u8"运行日志(防崩溃)")) {
                ImGui::BeginChild("LogScrollRegion", ImVec2(0, 0), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);
                {
                    std::lock_guard<std::mutex> logLock(g_logMutex);
                    if (g_appLogs.empty()) ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), (const char*)u8"暂无错误日志...");
                    else for (const auto& log : g_appLogs) ImGui::TextWrapped("%s", log.c_str());
                    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) ImGui::SetScrollHereY(1.0f);
                }
                ImGui::EndChild();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem((const char*)u8"资源路径收集器")) {
                ImGui::Checkbox((const char*)u8"开启小小英雄路径收集", &g_collect_skins);
                ImGui::SameLine();
                ImGui::Checkbox((const char*)u8"开启棋盘路径收集", &g_collect_maps);
                
                ImGui::Spacing();
                if (ImGui::Button((const char*)u8"清空所有记录", ImVec2(-1, 30 * g_autoScale))) {
                    std::lock_guard<std::mutex> lock(g_collect_mutex);
                    g_collected_skins.clear();
                    g_collected_maps.clear();
                }

                ImGui::Separator();
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), (const char*)u8"【截获的小小英雄模型路径】");
                ImGui::BeginChild("SkinsScroll", ImVec2(0, 180 * g_autoScale), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);
                {
                    std::lock_guard<std::mutex> lock(g_collect_mutex);
                    if (g_collected_skins.empty()) ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), (const char*)u8"暂无记录，请勾选开关并在游戏中切换界面或等待...");
                    
                    static int copied_skin_idx = -1;
                    static float skin_copy_timer = 0.0f;
                    if (skin_copy_timer > 0.0f) skin_copy_timer -= ImGui::GetIO().DeltaTime;

                    int s_idx = 0;
                    for (const auto& s : g_collected_skins) {
                        ImGui::TextWrapped("%s", s.c_str());
                        ImGui::PushID(s_idx);
                        if (ImGui::Button((const char*)u8"复制路径", ImVec2(100 * g_autoScale, 26 * g_autoScale))) {
                            ImGui::SetClipboardText(s.c_str());
                            CopyToAndroidClipboard(s.c_str());
                            copied_skin_idx = s_idx;
                            skin_copy_timer = 2.0f;
                        }
                        if (copied_skin_idx == s_idx && skin_copy_timer > 0.0f) {
                            ImGui::SameLine();
                            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), (const char*)u8"✓ 已复制");
                        }
                        ImGui::PopID();
                        ImGui::Separator();
                        s_idx++;
                    }
                }
                ImGui::EndChild();

                ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), (const char*)u8"【截获的竞技场/棋盘路径】");
                ImGui::BeginChild("MapsScroll", ImVec2(0, 180 * g_autoScale), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);
                {
                    std::lock_guard<std::mutex> lock(g_collect_mutex);
                    if (g_collected_maps.empty()) ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), (const char*)u8"暂无记录，请勾选开关并在游戏中切换界面或等待...");
                    
                    static int copied_map_idx = -1;
                    static float map_copy_timer = 0.0f;
                    if (map_copy_timer > 0.0f) map_copy_timer -= ImGui::GetIO().DeltaTime;

                    int m_idx = 0;
                    for (const auto& m : g_collected_maps) {
                        ImGui::TextWrapped("%s", m.c_str());
                        ImGui::PushID(m_idx + 10000); // 避免ID冲突
                        if (ImGui::Button((const char*)u8"复制路径", ImVec2(100 * g_autoScale, 26 * g_autoScale))) {
                            ImGui::SetClipboardText(m.c_str());
                            CopyToAndroidClipboard(m.c_str());
                            copied_map_idx = m_idx;
                            map_copy_timer = 2.0f;
                        }
                        if (copied_map_idx == m_idx && map_copy_timer > 0.0f) {
                            ImGui::SameLine();
                            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), (const char*)u8"✓ 已复制");
                        }
                        ImGui::PopID();
                        ImGui::Separator();
                        m_idx++;
                    }
                }
                ImGui::EndChild();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
    }
    ImGui::End();
}

void DrawMenu() {
    ImGuiIO& io = ImGui::GetIO(); ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 16.0f * g_autoScale; style.FrameRounding = 8.0f * g_autoScale; style.PopupRounding = 8.0f * g_autoScale;
    style.ItemSpacing = ImVec2(12 * g_autoScale, 12 * g_autoScale); style.WindowPadding = ImVec2(16 * g_autoScale, 16 * g_autoScale); 
    style.WindowBorderSize = 1.0f; style.ScrollbarSize = 35.0f * g_autoScale; style.GrabMinSize = 25.0f * g_autoScale;
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.09f, 0.11f, 0.92f); style.Colors[ImGuiCol_Border] = ImVec4(1.0f, 1.0f, 1.0f, 0.08f);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.09f, 0.11f, 0.95f); style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.12f, 0.13f, 0.15f, 0.95f);

    static bool firstMenuOpen = true; 
    if (firstMenuOpen) { 
        ImGui::SetNextWindowPos(ImVec2(g_menuX, g_menuY), ImGuiCond_Always); 
        ImGui::SetNextWindowSize(ImVec2(g_menuW, g_menuH), ImGuiCond_Always);
        ImGui::SetNextWindowCollapsed(g_menuCollapsed, ImGuiCond_Always); 
        firstMenuOpen = false; 
    }

    if (ImGui::Begin((const char*)u8"金铲铲助手", NULL, ImGuiWindowFlags_NoSavedSettings)) {
        if (!ImGui::IsWindowCollapsed()) {
            float curW = ImGui::GetWindowSize().x, curH = ImGui::GetWindowSize().y;
            if (std::abs(curW - g_menuW) > 5.0f || std::abs(curH - g_menuH) > 5.0f) { g_menuW = curW; g_menuH = curH; g_scale = std::clamp(curW / (500.0f * g_autoScale), 0.5f, 2.5f); }
        }
        g_menuX = ImGui::GetWindowPos().x; g_menuY = ImGui::GetWindowPos().y; g_menuCollapsed = ImGui::IsWindowCollapsed();
        if (!g_menuCollapsed) {
            ImGui::SetWindowFontScale(g_scale);
            ImGui::TextColored(ImVec4(0.0f, 0.85f, 0.55f, 1.0f), (const char*)u8"[+] 稳定原版运行中 | FPS: %.1f", io.Framerate);
            ColoredSeparator();
            
            // ===============================================
            // 神话换肤UI 
            // ===============================================
            if (ModernAnimatedFolder((const char*)u8"🏆 神话皮肤换肤", &g_header_skin)) {
                ModernToggle((const char*)u8"开启小小英雄换肤 (对局内生效)", &g_enable_skin, 18);
                if (g_enable_skin) {
                    ImGui::Indent(15.0f * g_autoScale * g_scale);
                    
                    const int numSkins = sizeof(g_skinList)/sizeof(g_skinList[0]);
                    std::vector<const char*> skinNames(numSkins);
                    for (int i = 0; i < numSkins; i++) {
                        skinNames[i] = g_skinList[i].zh_name;
                    }

                    ImGui::PushItemWidth(260.0f * g_autoScale * g_scale);
                    if (ImGui::Combo("##SkinSelect", &g_selected_skin_idx, skinNames.data(), numSkins, 10)) {
                        g_save_timer = 1.5f; SaveConfig();
                    }
                    ImGui::PopItemWidth();
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), (const char*)u8"选择英雄");

                    ImGui::Unindent(15.0f * g_autoScale * g_scale);
                }
                
                ImGui::Separator();
                
                ModernToggle((const char*)u8"开启竞技场换肤 (大厅或进图前开启)", &g_enable_map_skin, 19);
                if (g_enable_map_skin) {
                    ImGui::Indent(15.0f * g_autoScale * g_scale);
                    
                    const int numMaps = sizeof(g_mapList)/sizeof(g_mapList[0]);
                    std::vector<const char*> mapNames(numMaps);
                    for (int i = 0; i < numMaps; i++) {
                        mapNames[i] = g_mapList[i].zh_name;
                    }

                    ImGui::PushItemWidth(260.0f * g_autoScale * g_scale);
                    if (ImGui::Combo("##MapSelect", &g_selected_map_idx, mapNames.data(), numMaps, 10)) {
                        g_save_timer = 1.5f; SaveConfig();
                    }
                    ImGui::PopItemWidth();
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), (const char*)u8"选择棋盘");

                    ImGui::Unindent(15.0f * g_autoScale * g_scale);
                }
                
                EndModernAnimatedFolder();
                ColoredSeparator();
            }

            ModernToggle((const char*)u8"打开极速退游悬浮窗", &g_show_instant_quit_window, 7);
            ModernToggle((const char*)u8"打开卖空英雄悬浮窗", &g_show_sell_window, 23); 
            ColoredSeparator();

            // 穿拆装备与连线选项
            ModernToggle((const char*)u8"打开穿卸装备悬浮窗", &g_show_equip_window, 21);
            ModernToggle((const char*)u8"开启英雄3D追踪连线", &g_draw_hero_lines, 22);
            ColoredSeparator();
            
            ModernToggle((const char*)u8"主线程并发极速秒牌", &g_auto_buy, 14);
            if (g_auto_buy) {
                ImGui::Indent(15.0f * g_autoScale * g_scale);
                ModernToggle((const char*)u8"英雄列表及装备图鉴", &g_show_hero_list_window, 20);
                ImGui::Unindent(15.0f * g_autoScale * g_scale);
            }
            ModernSlider((const char*)u8"自动刷新(D牌)间隔 (毫秒)", &g_auto_buy_delay_ms, 0.0f, 1000.0f);
            ModernNumberAdjuster((const char*)u8"低于该金币停止刷新", &g_auto_refresh_money_threshold, 0, 200);
            ColoredSeparator();

            ModernToggle((const char*)u8"打开自动操作悬浮窗", &g_show_auto_window, 15);
            if (g_show_auto_window) {
                ImGui::Indent(15.0f * g_autoScale * g_scale);
                ModernToggle((const char*)u8"自动拿天选 (二星英雄)", &g_auto_buy_chosen, 16);
                ModernToggle((const char*)u8"显示[一键清空备战席]按钮", &g_show_clear_bench_btn, 24); 
                ImGui::Unindent(15.0f * g_autoScale * g_scale);
            }
            ColoredSeparator();
            
            if (ModernAnimatedFolder((const char*)u8"预测功能", &g_header_pred)) {
                ModernToggle((const char*)u8"预测对手", &g_predict_enemy, 1); 
                if (g_predict_enemy) {
                    ImGui::Indent(15.0f * g_autoScale * g_scale); ModernSlider((const char*)u8"预测名字缩放", &g_predictNameScale, 0.5f, 5.0f); ImGui::Unindent(15.0f * g_autoScale * g_scale);
                }
                ModernToggle((const char*)u8"预测海克斯", &g_predict_hex, 2); 
                EndModernAnimatedFolder();
            }
            if (ModernAnimatedFolder((const char*)u8"透视功能", &g_header_esp)) {
                ModernSlider((const char*)u8"整体透视透明度", &g_global_esp_alpha, 0.1f, 1.0f); 
                ModernToggle((const char*)u8"对手棋盘透视", &g_esp_board, 3); 
                ModernToggle((const char*)u8"敌方备战席透视", &g_esp_enemy_bench, 5); 
                ModernToggle((const char*)u8"金币等级透视", &g_esp_level, 9); 
                if (g_esp_level) {
                    ImGui::Indent(15.0f * g_autoScale * g_scale); ModernSlider((const char*)u8"行与行间距", &g_level_Spacing, 20.0f, 150.0f); ModernSlider((const char*)u8"字体缩放", &g_level_Scale, 0.5f, 3.0f); ImGui::Unindent(15.0f * g_autoScale * g_scale);
                }
                EndModernAnimatedFolder();
            }
            ColoredSeparator();
            
            ModernToggle((const char*)u8"锁定所有窗口", &g_boardLocked, 8); 
            
            if (ModernToggle((const char*)u8"显示/调节 [穿装] 落点 (需解锁窗口移动)", &g_show_equip_ui_debug, 25)) {
                if (g_show_equip_ui_debug) g_show_sell_ui_debug = false;
            }
            if (ModernToggle((const char*)u8"显示/调节 [卖牌] 落点 (需解锁窗口移动)", &g_show_sell_ui_debug, 26)) {
                if (g_show_sell_ui_debug) g_show_equip_ui_debug = false;
            }

            if (g_boardLocked) {
                ImGui::Indent(15.0f * g_autoScale * g_scale);
                ImGui::TextColored(ImVec4(1, 0.8f, 0.2f, 1), (const char*)u8"【装备栏点击点微调】");
                ModernFloatAdjuster((const char*)u8"起点 X", &g_equip_ui_start_x, 0.0f, 3000.0f, 1.0f);
                ModernFloatAdjuster((const char*)u8"起点 Y", &g_equip_ui_start_y, 0.0f, 3000.0f, 1.0f);
                ModernFloatAdjuster((const char*)u8"列间距 X", &g_equip_ui_step_x, 10.0f, 500.0f, 1.0f);
                ModernFloatAdjuster((const char*)u8"行间距 Y", &g_equip_ui_step_y, 10.0f, 500.0f, 1.0f);
                ImGui::Unindent(15.0f * g_autoScale * g_scale);
            }
            
            ModernToggle((const char*)u8"牌库显示", &g_show_card_pool, 10);
            if (g_show_card_pool) {
                ImGui::Indent(15.0f * g_autoScale * g_scale); ModernTierSelector((const char*)u8"显示等级:", g_show_pool_tiers); 
                ModernNumberAdjuster((const char*)u8"单行最大列数", &g_card_pool_cols, 1, 30); ModernFloatAdjuster((const char*)u8"牌库透明度", &g_cardPoolAlpha, 0.1f, 1.0f, 0.1f);
                
                ColoredSeparator();
                ModernToggle((const char*)u8"开启牌库剩余量红框预警", &g_pool_warning_enable, 17);
                if (g_pool_warning_enable) {
                    ImGui::Indent(15.0f * g_autoScale * g_scale);
                    ModernTierSelector((const char*)u8"预警卡费:", g_pool_warning_tiers);
                    ModernNumberAdjuster((const char*)u8"低于几张红框并沉底", &g_pool_warning_count, 1, 30);
                    ImGui::Unindent(15.0f * g_autoScale * g_scale);
                }
                
                ImGui::Unindent(15.0f * g_autoScale * g_scale);
            }
            ModernToggle((const char*)u8"卡牌预警数量", &g_card_warning, 11);
            if (g_card_warning) {
                ImGui::Indent(15.0f * g_autoScale * g_scale); ModernTierSelector((const char*)u8"预警等级:", g_warning_tiers); 
                ModernNumberAdjuster((const char*)u8"预警张数", &g_warning_threshold, 1, 30); ModernSlider((const char*)u8"预警行间距", &g_warn_Spacing, 20.0f, 150.0f); ModernSlider((const char*)u8"预警整体缩放", &g_warn_Scale, 0.5f, 3.0f);
                ImGui::Unindent(15.0f * g_autoScale * g_scale);
            }
            ModernToggle((const char*)u8"全功能偏移追踪与调试", &g_show_debug_window, 12);
            
            ImGui::Spacing();
            
            ColoredSeparator();
            
            if (g_save_timer > 0.0f) {
                g_save_timer -= io.DeltaTime;
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.7f, 0.4f, 1.0f)); ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 0.7f, 0.4f, 1.0f)); ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.7f, 0.4f, 1.0f));
                ImGui::Button((const char*)u8"✓ 配置文件已保存", ImVec2(-1, 55 * g_autoScale)); ImGui::PopStyleColor(3);
            } else {
                if (ImGui::Button((const char*)u8"保存当前配置", ImVec2(-1, 55 * g_autoScale))) { g_save_timer = 1.5f; SaveConfig(); }
            }
        }
    }
    ImGui::End();
}

// =================================================================
// 核心渲染循环 EGLSwapBuffers
// =================================================================
EGLBoolean (*old_eglSwap)(EGLDisplay, EGLSurface) = nullptr;
std::atomic<bool> g_engine_rendering{false};

EGLBoolean hook_eglSwap(EGLDisplay dpy, EGLSurface surf) {
    if (!g_engine_rendering.load()) {
        g_engine_rendering.store(true);
    }

    eglQuerySurface(dpy, surf, EGL_WIDTH, &g_gl_width);
    eglQuerySurface(dpy, surf, EGL_HEIGHT, &g_gl_height);
    
    if (g_gl_width <= 0 || g_gl_height <= 0) {
        return old_eglSwap(dpy, surf);
    }

    static bool init = false;
    static auto last_time = std::chrono::high_resolution_clock::now();
    
    static EGLContext last_ctx = EGL_NO_CONTEXT;
    EGLContext current_ctx = eglGetCurrentContext();
    if (current_ctx != last_ctx) {
        if (init) {
            ImGui_ImplOpenGL3_DestroyDeviceObjects();
            ClearAllTextureCaches();     
            g_needUpdateFontSafe = true; 
            g_cached_view_width = -1;    
            g_cached_view_height = -1;
        }
        last_ctx = current_ctx;
    }

    if (!init) {
        ImGui::CreateContext(); ImGuiIO& io = ImGui::GetIO(); io.IniFilename = nullptr; LoadConfig(); 
        ImGui_ImplOpenGL3_Init("#version 300 es"); io.DisplaySize = ImVec2((float)g_gl_width, (float)g_gl_height); UpdateFontHD(true); init = true;
    }
    if (g_needUpdateFontSafe) { UpdateFontHD(true); g_needUpdateFontSafe = false; }
    
    auto current_time = std::chrono::high_resolution_clock::now();
    float delta = std::chrono::duration<float>(current_time - last_time).count();
    last_time = current_time;
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)g_gl_width, (float)g_gl_height);
    io.DeltaTime = delta > 0.0001f ? delta : (1.0f / 120.0f); 
    
    ProcessTextureQueue();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();
    glDisable(GL_SCISSOR_TEST); 
    
    auto snapshot = GetSnapshot();
    
    DrawBoard(snapshot); 
    DrawBenchESP(snapshot);   
    DrawPurePredictEnemy(snapshot); 
    DrawPurePredictHex(snapshot); 
    DrawLevelOverlay(snapshot); 
    DrawWarningOverlay(snapshot);   
    DrawCardPool(snapshot); 
    DrawShopESP(snapshot);
    DrawAutoActionWindow(); 
    DrawAutoEquipWindow();     
    DrawSellHeroWindow(); 
    DrawHeroLinesESP(snapshot);
    DrawEquipUIDebug();         
    DrawSwipePaths();           
    DrawInstantQuitWindow();
    DrawHeroSelectionWindow(); 
    DrawMenu();
    DrawDebugWindow(snapshot); 
    
    ImGui::Render();
    glViewport(0, 0, g_gl_width, g_gl_height);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    return old_eglSwap(dpy, surf);
}

// =================================================================
// 初始化入口
// =================================================================
void* DelayedHookThread(void*) {
    while (!g_engine_rendering.load()) {
        sleep(1);
    }
    sleep(4);

    FindAndHookHiddenJNI();
    InitBusinessHooks();
    return nullptr;
}

void* SetupThread(void*) {
    // 初始化瞬间在后台建立本地全图鉴，不会卡住游戏
    BuildLocalPokedex();

    // 启动图片解压专属工人线程（终结并发发热）
    std::thread(TextureDecodingWorkerThread).detach();

    void* egl_ptr = nullptr;
    // [关键修复] 循环等待直到 Unity 引擎真正加载了 libEGL.so
    while ((egl_ptr = DobbySymbolResolver("libEGL.so", "eglSwapBuffers")) == nullptr) {
        sleep(1); // 每秒检测一次，直到图形库加载完成
    }
    
    // 成功获取到指针后再进行 Hook
    DobbyHook(egl_ptr, (void*)hook_eglSwap, (void**)&old_eglSwap);
    
    pthread_t t;
    pthread_create(&t, 0, DelayedHookThread, 0);
    pthread_detach(t);
    
    return nullptr;
}

__attribute__((constructor)) void Init() { 
    pthread_t t; 
    pthread_create(&t, 0, SetupThread, 0); 
    pthread_detach(t); 
}
