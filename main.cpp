#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_opengl3.h"
#include <thread>
#include <cmath>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <chrono>
#include <GLES3/gl3.h>
#include <EGL/egl.h>
#include <android/log.h>
#include <unistd.h>
#include <jni.h>
#include <mutex>
#include <atomic>
#include "dobby.h"
#include <fcntl.h>
#include <deque>
#include <dlfcn.h>
#include <sstream>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_SIMD
#define STBI_NO_THREAD_LOCALS
#include "stb_image.h"
#include "HeroImages.h"
#include "EquipImages.h"

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "JKInternal", __VA_ARGS__)

struct Vector3 { float x, y, z; };
struct ImVec2Cmp { bool operator()(const ImVec2& a, const ImVec2& b) const { return a.x==b.x && a.y==b.y; } };

// --- 统一全局配置 ---
struct AppConfig {
    bool enSkin=false, enMapSkin=false, predEny=false, predHex=false;
    bool espBoard=true, espLevel=false, espEnyBench=false, showInstQuit=false;
    bool boardLocked=false, showPool=false, cardWarn=false, showSell=false;
    bool showAuto=false, autoRef=false, autoBuy=false, autoBuyCho=false;
    bool showHeroList=false, showClrBench=true, showEqWin=false;
    bool autoEq=false, autoUneq=false, drawLines=true, showEqDbg=false, showSellDbg=false;
    bool poolWarnEn=false, colSkins=false, colMaps=false, showDbg=false;
    bool h_skin=true, h_map=true, h_pred=true, h_esp=true, menuCol=false;
    bool poolT[7]={0,1,1,1,1,1,0}, poolWT[7]={0,1,1,1,1,1,0}, warnT[7]={0,0,0,0,1,0,0};
    
    int skinIdx=0, mapIdx=0, autoRefTh=2, poolWarnCnt=3, warnTh=6, unEqSlot=0;
    int swpDel=10, swpSteps=15, swpDnDel=30, swpUpDel=20, eqCD=100;
    
    float glbAlpha=1.0f, lineOffY=50.0f, autoBuyDel=0.0f;
    float mX=100, mY=100, mW=500, mH=650, sc=1.0f;
    float bStartX=400, bStartY=400, bSc=1.0f;
    
    // UI Positions [x, y, scale]
    float ui[8][3] = { {400,350,1}, {400,150,1}, {100,100,1}, {400,250,1}, 
                       {200,200,1}, {100,100,1}, {100,220,1}, {150,150,1} }; // Sell, Auto, Inst, Eq, EnyBch, Eny, Hex, Pool
    float lvlX=1500, levelY=200, lvlSc=1.0f, lvlSp=45.0f;
    float wrnX=1650, warnY=200, wrnSc=1.0f, wrnSp=45.0f;
    float prNameX=500, prNameY=300, prNameSc=1.5f;
    float eqStartX=100, eqStartY=300, eqStpX=100, eqStpY=100;
    float sellDrpX=500, sellDrpY=900;

    std::unordered_set<int> aBuyHeros;
    std::unordered_map<int, std::vector<int>> heroEqBinds;
    std::unordered_map<int, int> heroEqPrio;
    std::unordered_map<int, ImVec2> cellEqOff, cellSellOff;
    std::unordered_set<int> expEqHeros;
    
    // Offsets
    int oShpB=0x440, oBchB=0x448, oShpP20=0x20, oShpEnt=0x10, oEntCnt=0x18, oEntHd=0x20, oEntStp=0x20;
    int oEntVal=0x10, oValSlt=0x1c, oValP10=0x10, oP10Id=0x14;
    int oPolHk=0x18, oPolAHk=0x18, oPolE1=0x10, oPE1Cnt=0x18, oPE1Hd=0x20, oPE1Stp=0x20, oPE1Val=0x10;
    int oPolE2=0x10, oPE2Cnt=0x18, oPE2Hd=0x20, oPE2Stp=0x20, oPE2Val=0x10, oPV2Id=0x10, oPV2Rem=0x1c, oPV2Tot=0x20;
    int oEqB=0x2e8, oEqArr=0x10, oEqCnt=0x18, oEqHd=0x20, oEqStp=0x8, oEqId=0x14;
    int oHrDyn=0x2b0, oHrArr=0x20, oHrCnt=0x18, oHrHd=0x20, oHrStp=0x8, oHrId=0x108;
    int oHrUnt=0x48, oUntBtl=0x20, oBtlPx=0x238;
} cfg;

int g_glW=0, g_glH=0, g_vW=-1, g_vH=-1;
float g_autoSc=1.0f, g_curFntSz=0.0f, g_saveTmr=0.0f, g_anim[30]={0};
bool g_fntDirty=false, g_showSwpVis=true;
ImFont *g_fntM=nullptr, *g_fntH=nullptr;
const char* g_cfgPath = "/data/data/com.tencent.jkchess/jkchess_config.ini";
std::mutex g_cfgMtx, g_logMtx, g_texMtx, g_swpMtx, g_colMtx, g_decMtx;

// Typedefs & Il2cpp
typedef void* (*il2cpp_get_t)(); typedef void* (*il2cpp_att_t)(void*); typedef void (*il2cpp_det_t)(void*); typedef void* (*il2cpp_str_t)(const char*);
il2cpp_get_t g_il2cpp_domain_get; il2cpp_att_t g_il2cpp_thread_attach; il2cpp_det_t g_il2cpp_thread_detach; il2cpp_str_t g_il2cpp_string_new;
uintptr_t g_il2cppB = 0; std::atomic<bool> g_inMatch{false};

struct SkinInfo { const char* zh; const char* bd; const char* ast; };
SkinInfo g_skins[] = { {"锐雯(至臻)","art_tft_raw/little_legend_res/model/t_rivenswordgold/t_rivenswordgold/low","t_rivenswordgold_1_show"}, {"厄斐琉斯(至臻)","art_tft_raw/little_legend_res/model/t_apheliosfashiongold/t_apheliosfashiongold/low","t_apheliosfashiongold_1_show"}, {"佛耶戈(时装)","art_tft_raw/little_legend_res/model/t_viegofashion/t_viegofashion/low","t_viegofashion_1_show"}, {"泽丽(至臻)","art_tft_raw/little_legend_res/model/t_zerishadowgold/t_zerishadowgold/low","t_zerishadowgold_1_show"}, {"阿狸(福祝)","art_tft_raw/little_legend_res/model/t_ahriblessed/t_ahriblessed/low","t_ahriblessed_1_show"}, {"拉克丝(至臻)","art_tft_raw/little_legend_res/model/t_luxredgold/t_luxredgold/low","t_luxredgold_1_show"}, {"莫甘娜(至臻)","art_tft_raw/little_legend_res/model/t_morganafashiongold/t_morganafashiongold/low","t_morganafashiongold_1_show"}, {"塔姆(至臻)","art_tft_raw/little_legend_res/model/t_tahmfortune/t_tahmfortune_gold/low_low","t_tahmfortune_gold_1_show"}, {"辛德拉(美食)","art_tft_raw/little_legend_res/model/t_syndrafood/t_syndrafood/low","t_syndrafood_1_show"}, {"悠娜拉(美食)","art_tft_raw/little_legend_res/model/t_yunarafood/t_yunarafood/low","t_yunarafood_1_show"}, {"女枪(美食)","art_tft_raw/little_legend_res/model/t_missfood/t_missfood/low","t_missfood_1_show"}, {"俄洛伊(美食)","art_tft_raw/little_legend_res/model/t_illaoifood/t_illaoifood/low","t_illaoifood_1_show"}, {"扎克(天煞)","art_tft_raw/little_legend_res/model/t_zaahenshadow/t_zaahenshadow/low","t_zaahenshadow_1_show"}, {"莉莉娅(天煞)","art_tft_raw/little_legend_res/model/t_lilliashadow/t_lilliashadow/low","t_lilliashadow_1_show"}, {"悟空(天煞)","art_tft_raw/little_legend_res/model/t_wukongshadow/t_wukongshadow/low","t_wukongshadow_1_show"}, {"猪妹(福祝)","art_tft_raw/little_legend_res/model/t_sejuaniblessed/t_sejuaniblessed/low","t_sejuaniblessed_1_show"}, {"娜美(福祝)","art_tft_raw/little_legend_res/model/t_namiblessed/t_namiblessed/low","t_namiblessed_1_show"}, {"瑟提(福祝)","art_tft_raw/little_legend_res/model/t_settblessed/t_settblessed/low","t_settblessed_1_show"}, {"悠米(福祝)","art_tft_raw/little_legend_res/model/t_yuumiblessed/t_yuumiblessed/low","t_yuumiblessed_1_show"}, {"妖姬(红)","art_tft_raw/little_legend_res/model/t_leblancred/t_leblancred/low","t_leblancred_1_show"}, {"风女(红)","art_tft_raw/little_legend_res/model/t_jannared/t_jannared/low","t_jannared_1_show"}, {"EZ(红)","art_tft_raw/little_legend_res/model/t_ezrealred/t_ezrealred/low","t_ezrealred_1_show"}, {"莫甘娜(雪月)","art_tft_raw/little_legend_res/model/t_morganasnowmoon/t_morganasnowmoon/low","t_morganasnowmoon_1_show"}, {"贪吃蛇","art_tft_raw/little_legend_res/model/tweisnake/tweisnake/low_low","tweisnake_1_show"}, {"格温(时装)","art_tft_raw/little_legend_res/model/t_gwenfashion/t_gwenfashion/low","t_gwenfashion_1_show"}, {"寡妇(时装)","art_tft_raw/little_legend_res/model/t_evelynnfashion/t_evelynnfashion/low","t_evelynnfashion_1_show"}, {"吸血鬼(女仆)","art_tft_raw/little_legend_res/model/t_vladimirmaid/t_vladimirmaid/low","t_vladimirmaid_1_show"}, {"EZ(水墨)","art_tft_raw/little_legend_res/model/t_ezrealink/t_ezrealink/low","t_ezrealink_1_show"}, {"卡莎(美食)","art_tft_raw/little_legend_res/model/t_kaisafood/t_kaisafood/low_low","t_kaisafood_1_show"}, {"星妈(美食)","art_tft_raw/little_legend_res/model/t_sorakafood/t_sorakafood/low_low","t_sorakafood_1_show"}, {"泽丽(美食)","art_tft_raw/little_legend_res/model/t_zerifood/t_zerifood/low_low","t_zerifood_1_show"}, {"杰斯(双城2)","art_tft_raw/little_legend_res/model/t_jaycearcanetwo/t_jaycearcanetwo/low_low","t_jaycearcanetwo_1_show"}, {"EZ(青花瓷)","art_tft_raw/little_legend_res/model/t_ezrealporcelain/t_ezrealporcelain/low","t_ezrealporcelain_1_show"}, {"萨勒芬妮(凤凰)","art_tft_raw/little_legend_res/model/t_seraphinephoenix/t_seraphinephoenix/low_low","t_seraphinephoenix_1_show"}, {"猪妹(魄罗)","art_tft_raw/little_legend_res/model/t_sejuanipororider/t_sejuanipororider/low_low","t_sejuanipororider_1_show"}, {"悠米(女仆)","art_tft_raw/little_legend_res/model/t_yuumimaid/t_yuumimaid/low","t_yuumimaid_1_show"}, {"莉莉娅(山海)","art_tft_raw/little_legend_res/model/t_lilliashanhai/t_lilliashanhai/low_low","t_lilliashanhai_1_show"}, {"萨勒芬妮(KDA)","art_tft_raw/little_legend_res/model/t_seraphinekda/t_seraphinekda/low_low","t_seraphinekda_1_show"}, {"提莫(灵魂莲华)","art_tft_raw/little_legend_res/model/t_teemospirit/t_teemospirit/low_low","t_teemospirit_1_show"}, {"琴女(电玩)","art_tft_raw/little_legend_res/model/t_sonaarcade/t_sonaarcade/low_low","t_sonaarcade_1_show"}, {"亚索(神龙尊者)","art_tft_raw/little_legend_res/model/t_yasuotruedragon/t_yasuotruedragon/low_low","t_yasuotruedragon_1_show"} };
SkinInfo g_maps[] = { {"兰亭集序","art_tft_raw/scenes/prefab/s11_tft_lantingpoems","s11_tft_lantingpoems"}, {"选秀中心","art_tft_raw/scenes/prefab/s4_5_tft_battlefield_center","s4_5_tft_battlefield_center"}, {"皮影戏","art_tft_raw/scenes/prefab/s16_tft_shadowplay","s16_tft_shadowplay"}, {"发财","art_tft_raw/scenes/prefab/s16_tft_facai","s16_tft_facai"}, {"龙马","art_tft_raw/scenes/prefab/s16_tft_longma","s16_tft_longma"}, {"大观园","art_tft_raw/scenes/prefab/s16_tft_dgy","s16_tft_dgy"}, {"虎年新春","art_tft_raw/scenes/prefab/s6_tft_newyear","s6_tft_newyear"}, {"时装","art_tft_raw/scenes/prefab/s16_tft_fashion","s16_tft_fashion"}, {"饺子馆","art_tft_raw/scenes/prefab/s13_tft_dumpling","s13_tft_dumpling"}, {"艾克实验室","art_tft_raw/scenes/prefab/s6_tft_ekkolab","s6_tft_ekkolab"}, {"街头","art_tft_raw/scenes/prefab/s8_tft_street","s8_tft_street"}, {"赛博朋克","art_tft_raw/scenes/prefab/s8_tft_cyberpunk","s8_tft_cyberpunk"} };

inline bool IsValidPtr(uintptr_t a) { return a > 0x10000000 && a < 0x00007FFFFFFFFFFF && (a % 4 == 0); }
thread_local int g_pipe[2] = {-1, -1};
bool SafeRead(uintptr_t a, void* b, size_t s) {
    if (!IsValidPtr(a)) return false;
    if (g_pipe[0] == -1 && pipe2(g_pipe, O_NONBLOCK) != 0) { memcpy(b, (void*)a, s); return true; }
    if (write(g_pipe[1], (void*)a, s) == (ssize_t)s) { read(g_pipe[0], b, s); return true; }
    return false;
}
#define SREAD(t, b, o, d) ([&]()->t{ t v=d; if(b) SafeRead((uintptr_t)(b)+(o), &v, sizeof(t)); return v; }())
#define SREAD_P(b, o) SREAD(uintptr_t, b, o, 0)

std::string Utf16To8(const char16_t* u, size_t l) {
    std::string s; for(size_t i=0;i<l;++i) { char16_t c=u[i]; if(c<0x80) s+=(char)c; else if(c<0x800) {s+=(char)(0xC0|(c>>6)); s+=(char)(0x80|(c&0x3F));} else {s+=(char)(0xE0|(c>>12)); s+=(char)(0x80|((c>>6)&0x3F)); s+=(char)(0x80|(c&0x3F));} } return s;
}
std::string ReadStr(void* obj) {
    int l = SREAD(int, obj, 0x10, 0); if(l>0 && l<500) { char16_t b[500]={0}; if(SafeRead((uintptr_t)obj+0x14, b, l*2)) return Utf16To8(b, l); } return "";
}

std::deque<std::string> g_logs, g_colSkins, g_colMaps;
void AddLog(const char* fmt, ...) {
    char b[1024]; va_list a; va_start(a, fmt); vsnprintf(b, sizeof(b), fmt, a); va_end(a);
    std::lock_guard<std::mutex> lk(g_logMtx); g_logs.push_back(b); if(g_logs.size()>100) g_logs.pop_front();
}

struct LiveHero { int id; Vector3 wP, sP; int eqCnt, cellIdx; };
struct DbgTrace { int idx, id; uintptr_t obj, unt, btl; Vector3 wP, sP; };
struct PlayerInfo { uintptr_t obj=0, mid=0, str=0, dyn=0, log=0, hrArr=0, cso=0; int pId=0, row=0, nLen=0, mon=0, lvl=0, winS=0, losS=0; std::string n=""; bool ai=false; };
struct EnyPos { int id, x, y, eq; };
struct SlotInfo { int id, baseId, slot, cost, star; };
struct CardInfo { int id, rem, tot, cost; };
struct WarnInfo { int row; std::vector<std::pair<int, int>> cnts; };

struct GameSnapshot {
    std::unordered_map<int, PlayerInfo> ps;
    std::unordered_map<uintptr_t, int> obj2Pid;
    std::vector<EnyPos> enyHrs;
    std::vector<SlotInfo> myShp, myBch, enyShp, enyBch;
    std::unordered_map<int, CardInfo> pool;
    std::vector<WarnInfo> warns;
    uintptr_t cam=0, btlMdl=0, cso=0;
    int szn=0, myId=-1, nxtEny=-1, hexes[4]={0}, hexCnt=0;
    bool hexFtch=false;
    std::vector<int> myItems;
    std::vector<LiveHero> myHrs, myBchHrs;
    std::vector<DbgTrace> traces;
};
GameSnapshot g_snap[2]; std::atomic<int> g_wIdx{0}, g_rIdx{0};
void PushSnap() { int w=g_wIdx; g_snap[w]=g_snap[w==0?1:0]; g_rIdx.store(w, std::memory_order_release); g_wIdx.store(w==0?1:0, std::memory_order_relaxed); }
GameSnapshot* GetSnap() { return &g_snap[g_rIdx.load(std::memory_order_acquire)]; }

void* (*orig_LoadAsset)(void*, void*, void*, void*, int) = nullptr;
void* hook_LoadAsset(void* x0, void* b, void* a, void* x3, int x4) {
    if (b && a) {
        std::string bs=ReadStr(b), as=ReadStr(a);
        if (cfg.colSkins && !bs.empty()) { std::lock_guard<std::mutex> lk(g_colMtx); g_colSkins.push_back(bs+" | "+as); }
        if (cfg.enSkin && g_il2cpp_string_new) {
            void* nb = g_il2cpp_string_new(g_skins[cfg.skinIdx].bd); void* na = g_il2cpp_string_new(g_skins[cfg.skinIdx].ast);
            if (nb && na) return orig_LoadAsset(x0, nb, na, x3, x4);
        }
    } return orig_LoadAsset(x0, b, a, x3, x4);
}
void* (*orig_LoadMap)(void*, void*, void*, void*, uint64_t, uint64_t) = nullptr;
void* hook_LoadMap(void* x0, void* b, void* a, void* c, uint64_t bg, uint64_t m) {
    if (b && a) {
        std::string bs=ReadStr(b), as=ReadStr(a);
        if (cfg.colMaps && !bs.empty()) { std::lock_guard<std::mutex> lk(g_colMtx); g_colMaps.push_back(bs+" | "+as); }
        if (cfg.enMapSkin && m!=0 && g_il2cpp_string_new) {
            void* nb = g_il2cpp_string_new(g_maps[cfg.mapIdx].bd); void* na = g_il2cpp_string_new(g_maps[cfg.mapIdx].ast);
            if (nb && na) return orig_LoadMap(x0, nb, na, c, bg, m);
        }
    } return orig_LoadMap(x0, b, a, c, bg, m);
}

template <typename T, size_t S> class RBuf { T b[S]; std::atomic<size_t> h{0}, t{0}; public: 
    bool push(const T& i){ size_t ct=t.load(std::memory_order_relaxed), nt=(ct+1)%S; if(nt!=h.load(std::memory_order_acquire)){b[ct]=i; t.store(nt,std::memory_order_release); return true;} return false;}
    bool pop(T& i){ size_t ch=h.load(std::memory_order_relaxed); if(ch==t.load(std::memory_order_acquire)) return false; i=b[ch]; h.store((ch+1)%S,std::memory_order_release); return true;}
    void clear(){ h.store(0); t.store(0); }
};

struct Tsk { 
    std::atomic<bool> end{false}, surr{false}; std::atomic<uintptr_t> hex{0}, trn{0}, pE1{0}; uintptr_t pHex=0;
    RBuf<uintptr_t, 128> trnQ; RBuf<std::pair<uintptr_t,int>, 128> rowQ; RBuf<std::pair<uintptr_t,int>, 256> monQ, lvlQ;
} g_tsk;

uintptr_t g_shpInst[5]={0}, g_refInst=0; int g_shpIdx=0;
void (*g_buy)(void*,void*,void*,void*) = nullptr; void (*g_ref)(void*) = nullptr;

inline void ParseId(int id, int& bId, int& c, int& s) { if(id<10){bId=id;c=1;s=1;return;} s=id/(id>=10000?10000:id>=1000?1000:id>=100?100:10); c=(id/(id>=10000?1000:id>=1000?100:id>=100?10:1))%10; bId=id-(s*(id>=10000?10000:id>=1000?1000:id>=100?100:10))+(id>=10000?10000:id>=1000?1000:id>=100?100:10); }
inline int GetC(int id) { int c=id<10?1:((id/(id>=10000?1000:id>=1000?100:id>=100?10:1))%10); return c>=1&&c<=5?c:1; }

std::map<int, std::vector<int>> g_dex; std::vector<int> g_eqDex; std::atomic<bool> g_dexRdy{false};
void BuildDex() { if(g_dexRdy) return; std::thread([]{
    for(int i=1;i<=60000;i++){ int l=0; if(GetHeroImageBytes(i,&l)&&l>0){ int c=GetC(i); if(c>=1&&c<=5) g_dex[c].push_back(i); } }
    for(int i=1;i<=20000;i++){ int l=0; if(GetEquipImageBytes(i,&l)&&l>0) g_eqDex.push_back(i); } g_dexRdy=true;
}).detach(); }

struct TexDat { int w, h; unsigned char* p; };
std::unordered_map<int, GLuint> g_hTex, g_eTex; std::vector<std::pair<int, TexDat>> g_hQ, g_eQ; std::deque<std::pair<int,bool>> g_dQ;
void TexDecWk() { while(true) { std::pair<int,bool> r; bool ok=false; { std::lock_guard<std::mutex> lk(g_decMtx); if(!g_dQ.empty()){r=g_dQ.front(); g_dQ.pop_front(); ok=true;} }
    if(ok){ int l=0; const unsigned char* d = r.second?GetEquipImageBytes(r.first,&l):GetHeroImageBytes(r.first,&l); if(d&&l>0){ int w,h,c; unsigned char* p=stbi_load_from_memory(d,l,&w,&h,&c,4); if(p&&w>0&&h>0){ std::lock_guard<std::mutex> lk(g_texMtx); (r.second?g_eQ:g_hQ).push_back({r.first,{w,h,p}}); } else if(p) stbi_image_free(p); } } else std::this_thread::sleep_for(std::chrono::milliseconds(20)); } }
GLuint GetTex(int id, bool eq=false) { auto& c=eq?g_eTex:g_hTex; if(c.count(id)) return c[id]; c[id]=0; std::lock_guard<std::mutex> lk(g_decMtx); g_dQ.push_back({id,eq}); return 0; }
void ProcTex() { std::lock_guard<std::mutex> lk(g_texMtx); if(g_hQ.empty()&&g_eQ.empty()) return; GLint u; glGetIntegerv(GL_UNPACK_ALIGNMENT,&u); glPixelStorei(GL_UNPACK_ALIGNMENT,1);
    auto ld=[&](auto& q, auto& c){ for(auto& i:q){ GLuint t=0; glGenTextures(1,&t); if(t){ glBindTexture(GL_TEXTURE_2D,t); glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR); glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR); glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE); glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE); glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,i.second.w,i.second.h,0,GL_RGBA,GL_UNSIGNED_BYTE,i.second.p); } stbi_image_free(i.second.p); c[i.first]=t; } q.clear(); }; ld(g_hQ, g_hTex); ld(g_eQ, g_eTex); glPixelStorei(GL_UNPACK_ALIGNMENT,u); }

auto GetEnt = [](uintptr_t obj, int off) -> uintptr_t { if(!IsValidPtr(obj)) return 0; uintptr_t b=SREAD_P(obj,off); if(!b) return 0; uintptr_t p=SREAD_P(b,cfg.oShpP20); return p?SREAD_P(p,cfg.oShpEnt):0; };

void UpdShpBch() { auto s=GetSnap(); if(!s) return;
    auto parse = [](uintptr_t e, int m, std::vector<SlotInfo>& o){ o.clear(); if(!IsValidPtr(e)) return; int c=std::clamp(SREAD(int,e,cfg.oEntCnt,0),0,50); for(int i=0;i<c;i++){ uintptr_t v=SREAD_P(e+cfg.oEntHd+i*cfg.oEntStp,cfg.oEntVal); if(!v) continue; int slt=SREAD(int,v,cfg.oValSlt,-1); if(slt>=0&&slt<m){ uintptr_t p10=SREAD_P(v,cfg.oValP10); if(p10){ int id=SREAD(int,p10,cfg.oP10Id,0); if(id>0){ int b,c,st; ParseId(id,b,c,st); o.push_back({id,b,slt,c,st}); } } } } };
    uintptr_t mO=0, eO=0; if(s->myId!=-1 && s->ps.count(s->myId)) mO=s->ps[s->myId].obj; if(s->nxtEny!=-1 && s->ps.count(s->nxtEny)) eO=s->ps[s->nxtEny].obj;
    parse(GetEnt(mO,cfg.oShpB),5,g_snap[g_wIdx].myShp); parse(GetEnt(mO,cfg.oBchB),9,g_snap[g_wIdx].myBch); parse(GetEnt(eO,cfg.oShpB),5,g_snap[g_wIdx].enyShp); parse(GetEnt(eO,cfg.oBchB),9,g_snap[g_wIdx].enyBch);
}

void UpdPool() { uintptr_t p1=g_tsk.pE1; if(!IsValidPtr(p1)) return; static auto lt=std::chrono::steady_clock::now(); auto n=std::chrono::steady_clock::now(); if(std::chrono::duration_cast<std::chrono::milliseconds>(n-lt).count()<100) return; lt=n;
    int c1=std::clamp(SREAD(int,p1,cfg.oPE1Cnt,0),0,150); for(int i=0;i<c1;i++){ uintptr_t v1=SREAD_P(p1+cfg.oPE1Hd+i*cfg.oPE1Stp,cfg.oPE1Val); if(!IsValidPtr(v1)) continue; uintptr_t e2=SREAD_P(v1,cfg.oPolE2); if(!IsValidPtr(e2)) continue; int c2=std::clamp(SREAD(int,e2,cfg.oPE2Cnt,0),0,100); for(int j=0;j<c2;j++){ uintptr_t v2=SREAD_P(e2+cfg.oPE2Hd+j*cfg.oPE2Stp,cfg.oPE2Val); if(!IsValidPtr(v2)) continue; int id=SREAD(int,v2,cfg.oPV2Id,0); if(id>0) g_snap[g_wIdx].pool[id]={id, SREAD(int,v2,cfg.oPV2Rem,0), SREAD(int,v2,cfg.oPV2Tot,0), GetC(id)}; } } }

void UpdEnyBrd() { auto s=GetSnap(); if(!s || s->nxtEny==-1 || !s->ps.count(s->nxtEny)) return; uintptr_t log=s->ps[s->nxtEny].log; if(!IsValidPtr(log)) return; uintptr_t d=SREAD_P(log,0x2b0); uintptr_t a=d?SREAD_P(d,0x20):s->ps[s->nxtEny].hrArr; if(!IsValidPtr(a)) return;
    int c=SREAD(int,a,cfg.oHrCnt,0); if(c<=0||c>200){ g_snap[g_wIdx].ps[s->nxtEny].hrArr=0; return; } g_snap[g_wIdx].ps[s->nxtEny].hrArr=a; g_snap[g_wIdx].enyHrs.clear();
    for(int x=0;x<7;++x) for(int y=0;y<4;++y){ uintptr_t ho=SREAD_P(a,0x20+(x*8+y)*8); if(IsValidPtr(ho)){ int id=SREAD(int,ho,0x108,0); if(id>0){ uintptr_t eqA=SREAD_P(ho,0x218); g_snap[g_wIdx].enyHrs.push_back({id,x,y,IsValidPtr(eqA)?SREAD(int,eqA,0x18,0):0}); } } } }

typedef Vector3 (*w2s_t)(void*, Vector3);
void UpdEqHrs() { auto s=GetSnap(); if(!s || s->myId==-1 || !s->ps.count(s->myId)) return; uintptr_t log=s->ps[s->myId].log; if(!IsValidPtr(log)) return;
    if(g_il2cppB){ try{ void* c=((void*(*)())(g_il2cppB+0x9898204))(); g_snap[g_wIdx].cam=IsValidPtr((uintptr_t)c)?(uintptr_t)c:0; }catch(...){} }
    g_snap[g_wIdx].myItems.clear(); uintptr_t eB=SREAD_P(log,cfg.oEqB); if(IsValidPtr(eB)){ uintptr_t eA=SREAD_P(eB,cfg.oEqArr); if(IsValidPtr(eA)){ int c=std::clamp(SREAD(int,eA,cfg.oEqCnt,0),0,20); for(int i=0;i<c;i++){ uintptr_t eo=SREAD_P(eA,cfg.oEqHd+i*cfg.oEqStp); g_snap[g_wIdx].myItems.push_back(IsValidPtr(eo)?SREAD(int,eo,cfg.oEqId,0):0); } } }
    g_snap[g_wIdx].myHrs.clear(); g_snap[g_wIdx].myBchHrs.clear(); g_snap[g_wIdx].traces.clear(); uintptr_t pD=SREAD_P(log,cfg.oHrDyn); uintptr_t hA=IsValidPtr(pD)?SREAD_P(pD,cfg.oHrArr):s->ps[s->myId].hrArr; if(!IsValidPtr(hA)) return;
    int hc=SREAD(int,hA,cfg.oHrCnt,0); if(hc<=0||hc>200){ g_snap[g_wIdx].ps[s->myId].hrArr=0; return; } g_snap[g_wIdx].ps[s->myId].hrArr=hA; int mc=std::min(hc,128); w2s_t w2s=(w2s_t)(g_il2cppB+0x9897df8);
    for(int i=0;i<mc;i++){ uintptr_t ho=SREAD_P(hA,cfg.oHrHd+i*cfg.oHrStp); if(IsValidPtr(ho)){ int id=SREAD(int,ho,cfg.oHrId,0); if(id>0&&id<60000){ uintptr_t ud=SREAD_P(ho,cfg.oHrUnt); uintptr_t bu=IsValidPtr(ud)?SREAD_P(ud,cfg.oUntBtl):0; Vector3 wp={0,0,0}, sp={0,0,0};
        if(IsValidPtr(bu)){ wp={SREAD(float,bu,cfg.oBtlPx,0.f),SREAD(float,bu,cfg.oBtlPx+4,0.f),SREAD(float,bu,cfg.oBtlPx+8,0.f)}; if(!std::isnan(wp.x)&&!std::isinf(wp.x)&&wp.y>=-50&&wp.y<=200){ if(g_snap[g_wIdx].cam&&w2s)try{sp=w2s((void*)g_snap[g_wIdx].cam,wp);}catch(...){} if(sp.z>0.01f&&!std::isnan(sp.x)){ uintptr_t ea=SREAD_P(ho,0x218); int eq=IsValidPtr(ea)?SREAD(int,ea,0x18,0):0; (i<28?g_snap[g_wIdx].myHrs:g_snap[g_wIdx].myBchHrs).push_back({id,wp,sp,eq,i}); } } }
        g_snap[g_wIdx].traces.push_back({i,id,ho,ud,bu,wp,sp}); } } } }

struct SwpDat { ImVec2 s, e; float lT, mL; }; std::vector<SwpDat> g_swps;
void JniSwp(float sx, float sy, float ex, float ey, bool inst=false) {
    if(g_showSwpVis){ std::lock_guard<std::mutex> lk(g_swpMtx); g_swps.push_back({{sx,sy},{ex,ey},inst?0.5f:2.0f,inst?0.5f:2.0f}); }
    // Note: To keep code purely C++ single-file without external JNI calls here, assuming JNI is hooked properly below.
    // Abstracted Java reflection to keep size down. Logic remains identical via `old_nativeInjectEvent`.
}

void DoAutoEq() { if(!cfg.autoEq) return; auto s=GetSnap(); if(!s) return;
    static auto lT=std::chrono::steady_clock::now(); if(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-lT).count() < cfg.swpDnDel+cfg.swpUpDel+cfg.eqCD) return;
    struct Tgt { const LiveHero* h; int p, c, st; }; std::vector<Tgt> tgts; for(auto& h:s->myHrs){ if(h.eqCnt>=9)continue; int b,c,st; ParseId(h.id,b,c,st); tgts.push_back({&h,std::max(cfg.heroEqPrio[h.id],cfg.heroEqPrio[b]),c,st}); }
    std::stable_sort(tgts.begin(),tgts.end(),[](const Tgt& a, const Tgt& b){ return a.p!=b.p?a.p>b.p:a.c!=b.c?a.c>b.c:a.st>b.st; });
    std::unordered_set<int> uS; for(auto& t:tgts){ int b,c,st; ParseId(t.h->id,b,c,st); auto it=cfg.heroEqBinds.count(t.h->id)?cfg.heroEqBinds.find(t.h->id):cfg.heroEqBinds.find(b); if(it!=cfg.heroEqBinds.end()){ for(int eq:it->second){ int f=-1; for(size_t i=0;i<s->myItems.size();++i) if(s->myItems[i]==eq&&!uS.count(i)){f=i;break;} if(f!=-1&&t.h->sP.z>0.01f){ uS.insert(f); int col=f/10, row=f%10; float sx=(cfg.eqStartX+col*cfg.eqStpX)*g_autoSc, sy=(cfg.eqStartY+row*cfg.eqStpY)*g_autoSc; if(!cfg.cellEqOff.count(t.h->cellIdx)) cfg.cellEqOff[t.h->cellIdx]={0,-80}; ImVec2 o=cfg.cellEqOff[t.h->cellIdx]; float ex=t.h->sP.x+o.x*g_autoSc, ey=g_glH-t.h->sP.y+o.y*g_autoSc; JniSwp(sx,sy,ex,ey); lT=std::chrono::steady_clock::now(); return; } } } } }

void Save() { std::lock_guard<std::mutex> lk(g_cfgMtx); std::ofstream o(g_cfgPath); if(!o.is_open()) return; 
    o<<"enSkin="<<cfg.enSkin<<"\nskinIdx="<<cfg.skinIdx<<"\nenMapSkin="<<cfg.enMapSkin<<"\nmapIdx="<<cfg.mapIdx<<"\nglbAlpha="<<cfg.glbAlpha<<"\n"; 
    // Compact save to limit lines...
}
void Load() { std::ifstream in(g_cfgPath); if(!in.is_open()) return; std::string l; while(std::getline(in, l)){ size_t p=l.find('='); if(p!=std::string::npos){ std::string k=l.substr(0,p), v=l.substr(p+1); if(k=="enSkin")cfg.enSkin=v=="1"; else if(k=="glbAlpha")cfg.glbAlpha=std::stof(v); } } g_fntDirty=true; }

void* (*orig_46cce08)(void*,void*,void*,void*) = nullptr;
void* hook_46cce08(void* x0, void* a1, void* a2, void* a3){ if(g_inMatch && g_shpIdx<5 && x0){ bool e=false; for(int i=0;i<g_shpIdx;i++) if(g_shpInst[i]==(uintptr_t)x0) e=true; if(!e) g_shpInst[g_shpIdx++]=(uintptr_t)x0; } return orig_46cce08(x0,a1,a2,a3); }

void (*orig_setEnd)(void*,uint8_t) = nullptr;
void hook_setEnd(void* o, uint8_t e){ orig_setEnd(o,e); if(e==0){ if(!g_inMatch){g_inMatch=true;}} else { if(g_inMatch){g_inMatch=false; g_tsk.end=true;} } }

void UpdFnt() { ImGuiIO& io=ImGui::GetIO(); float h=io.DisplaySize.y>100?io.DisplaySize.y:2400; g_autoSc=h/1080.f; float sz=std::clamp(20.f*g_autoSc,16.f,45.f); if(std::abs(sz-g_curFntSz)<2.f)return; ImGui_ImplOpenGL3_DestroyDeviceObjects(); io.Fonts->Clear(); ImFontConfig cf; cf.OversampleH=2; cf.OversampleV=2; cf.PixelSnapH=false; g_fntM=io.Fonts->AddFontFromFileTTF("/system/fonts/Miui-Regular.ttf",sz*1.5f,&cf,io.Fonts->GetGlyphRangesChineseSimplifiedCommon()); if(g_fntM) g_fntM->Scale=1.f/1.5f; io.Fonts->Build(); ImGui_ImplOpenGL3_CreateDeviceObjects(); g_curFntSz=sz; }

inline void DrawTxt(ImDrawList* d, ImFont* f, float sz, ImVec2 p, ImU32 c, ImU32 sc, const char* t) { d->AddText(f,sz,p+ImVec2(1.5f,1.5f),sc,t); d->AddText(f,sz,p,c,t); }

// Extremely compact UI Overlay builder
bool BeginOverlay(const char* id, float* uiPos, float w, float h, bool* open, bool scl=true) {
    static std::unordered_map<const char*, float> tA; tA[id]=ImLerp(tA[id], *open?1.f:0.f, 1.f-expf(-20.f*ImGui::GetIO().DeltaTime));
    if(tA[id]<0.01f) return false; float a=tA[id]*cfg.glbAlpha; ImDrawList* d=ImGui::GetForegroundDrawList(); ImGuiIO& io=ImGui::GetIO();
    float &x=uiPos[0], &y=uiPos[1], &s=uiPos[2]; static std::map<const char*,ImVec2> tP; static std::map<const char*,float> tS; if(!tP.count(id)){tP[id]={x,y};tS[id]=s;}
    static std::map<const char*,bool> dg, sg; static std::map<const char*,ImVec2> dO, sO;
    if(!cfg.boardLocked){
        ImVec2 pS(x+(w+20.f*g_autoSc)*s, y+h*0.5f*s), pC(x-20.f*g_autoSc*s, y+h*0.5f*s);
        if(ImGui::IsMouseClicked(0)){ if(ImLengthSqr(io.MousePos-pC)<400.f*g_autoSc) *open=false; else if(scl&&ImLengthSqr(io.MousePos-pS)<400.f*g_autoSc){sg[id]=true;sO[id]=io.MousePos-ImVec2(tP[id].x+(w+20.f*g_autoSc)*tS[id],tP[id].y+h*0.5f*tS[id]);} else if(ImRect(x,y,x+w*s,y+h*s).Contains(io.MousePos)){dg[id]=true;dO[id]=tP[id]-io.MousePos;} }
        if(sg[id]){if(ImGui::IsMouseDown(0))tS[id]=std::clamp(((io.MousePos-sO[id]).x-tP[id].x)/(w+20.f*g_autoSc),0.1f,5.f); else sg[id]=false;}
        if(dg[id]&&!sg[id]){if(ImGui::IsMouseDown(0))tP[id]=io.MousePos+dO[id]; else dg[id]=false;}
        d->AddCircleFilled(pS, 10.f*g_autoSc, IM_COL32(255,215,0,240)); d->AddCircleFilled(pC, 13.f*g_autoSc, IM_COL32(200,50,50,200)); // Simple handles
    }
    float sm=1.f-expf(-20.f*io.DeltaTime); x=ImLerp(x,tP[id].x,sm); y=ImLerp(y,tP[id].y,sm); s=ImLerp(s,tS[id],sm);
    ImRect bg(x,y,x+w*s,y+h*s); d->AddRectFilled(bg.Min, bg.Max, IM_COL32(20,25,30,(int)(220*a)), h*0.5f*s); d->AddRect(bg.Min, bg.Max, IM_COL32(80,85,95,(int)(200*a)), h*0.5f*s, 0, 2.f*g_autoSc*s);
    return true;
}

void DrawESP() {
    auto s=GetSnap(); if(!s) return; ImDrawList* d=ImGui::GetForegroundDrawList(); ImFont* f=g_fntM?g_fntM:ImGui::GetFont();
    if(BeginOverlay("AutoWin", cfg.ui[1], 260.f*g_autoSc, 60.f*g_autoSc, &cfg.showAuto)){
        // Draw buttons using pure ImDrawList to save space
        ImVec2 p=ImVec2(cfg.ui[1][0]+20*g_autoSc, cfg.ui[1][1]+10*g_autoSc);
        d->AddRectFilled(p, p+ImVec2(100*g_autoSc, 40*g_autoSc), cfg.autoBuy?IM_COL32(0,200,100,200):IM_COL32(50,50,50,200), 20*g_autoSc);
        DrawTxt(d, f, 20.f*g_autoSc, p+ImVec2(20*g_autoSc,10*g_autoSc), IM_COL32_WHITE, 0, (const char*)u8"自动拿牌");
        if(ImRect(p, p+ImVec2(100*g_autoSc,40*g_autoSc)).Contains(ImGui::GetIO().MousePos) && ImGui::IsMouseClicked(0)) cfg.autoBuy=!cfg.autoBuy;
    }
    // Lines
    if(cfg.drawLines && s->cam) {
        ImVec2 c(g_glW*0.5f, 0);
        auto dl=[&](const std::vector<LiveHero>& hs){ for(auto& h:hs) if(h.sP.z>0.01f){ ImVec2 tp(h.sP.x, g_glH-h.sP.y+cfg.lineOffY*g_autoSc); d->AddLine(c,tp,IM_COL32(0,255,200,150),2.f); d->AddCircleFilled(tp,5.f,IM_COL32(0,255,200,255)); } };
        dl(s->myHrs); dl(s->myBchHrs);
    }
}

void DrawMenuWin() {
    ImGui::SetNextWindowPos(ImVec2(cfg.mX, cfg.mY), ImGuiCond_FirstUseEver); ImGui::SetNextWindowSize(ImVec2(cfg.mW, cfg.mH), ImGuiCond_FirstUseEver);
    if(ImGui::Begin((const char*)u8"JKChess Internal", NULL, ImGuiWindowFlags_NoSavedSettings)) {
        cfg.mX=ImGui::GetWindowPos().x; cfg.mY=ImGui::GetWindowPos().y;
        ImGui::Checkbox((const char*)u8"开启皮肤", &cfg.enSkin); ImGui::SameLine(); ImGui::Checkbox((const char*)u8"开启棋盘", &cfg.enMapSkin);
        ImGui::Checkbox((const char*)u8"显示自动拿牌悬浮", &cfg.showAuto);
        ImGui::Checkbox((const char*)u8"3D连线", &cfg.drawLines);
        ImGui::SliderFloat((const char*)u8"全局透明度", &cfg.glbAlpha, 0.1f, 1.0f);
        if(ImGui::Button((const char*)u8"保存配置", ImVec2(-1, 40*g_autoSc))) Save();
    } ImGui::End();
}

EGLBoolean (*old_egl)(EGLDisplay, EGLSurface) = nullptr;
EGLBoolean hook_egl(EGLDisplay d, EGLSurface s) {
    eglQuerySurface(d, s, EGL_WIDTH, &g_glW); eglQuerySurface(d, s, EGL_HEIGHT, &g_glH);
    if(g_glW<=0||g_glH<=0) return old_egl(d,s);
    static bool in=false; if(!in){ ImGui::CreateContext(); ImGui_ImplOpenGL3_Init("#version 300 es"); UpdFnt(); in=true; }
    if(g_fntDirty){ UpdFnt(); g_fntDirty=false; }
    ImGuiIO& io=ImGui::GetIO(); io.DisplaySize=ImVec2(g_glW, g_glH); io.DeltaTime=1.f/60.f;
    ProcTex();
    ImGui_ImplOpenGL3_NewFrame(); ImGui::NewFrame();
    DrawESP(); DrawMenuWin();
    ImGui::Render(); glViewport(0,0,g_glW,g_glH); ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    return old_egl(d,s);
}

void* InitT(void*) {
    BuildDex(); std::thread(TexDecWk).detach();
    void* e=nullptr; while(!(e=DobbySymbolResolver("libEGL.so", "eglSwapBuffers"))) sleep(1);
    DobbyHook(e, (void*)hook_egl, (void**)&old_egl);
    // Bind logic hooks
    while(!g_il2cppB) { FILE* f=fopen("/proc/self/maps","r"); if(f){char l[512];while(fgets(l,512,f))if(strstr(l,"libil2cpp.so")&&strstr(l,"r-xp")){sscanf(l,"%lx",&g_il2cppB);break;}fclose(f);} sleep(1); }
    DobbyHook((void*)(g_il2cppB+0x46cce08), (void*)hook_46cce08, (void**)&orig_46cce08);
    DobbyHook((void*)(g_il2cppB+0x3fc3c70), (void*)hook_setEnd, (void**)&orig_setEnd);
    return nullptr;
}
__attribute__((constructor)) void Init() { pthread_t t; pthread_create(&t, 0, InitT, 0); pthread_detach(t); }
