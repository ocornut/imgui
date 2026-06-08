// i18n/imgui_i18n.cpp
#include "imgui_i18n.h"
#include <unordered_map>
#include <string>

namespace imgui_i18n {

// Meyers Singleton — 保证跨编译单元 static initializer 调用时已初始化（避免 SIOF）
static std::string& locale() {
    static std::string s;
    return s;
}

static std::unordered_map<std::string, std::unordered_map<std::string, std::string>>& tables() {
    static std::unordered_map<std::string, std::unordered_map<std::string, std::string>> s;
    return s;
}

void setLocale(const char* loc) {
    locale() = loc ? loc : "";
}

const char* getLocale() {
    return locale().c_str();
}

const char* translate(const char* key) {
    if (!key) return "";
    if (locale().empty()) return key; // English fast path
    auto it = tables().find(locale());
    if (it == tables().end()) return key;
    auto it2 = it->second.find(key);
    if (it2 == it->second.end()) return key;
    return it2->second.c_str();
}

std::string fmt(const char* key, std::initializer_list<std::string> args) {
    std::string result = translate(key);
    int i = 1;
    for (const auto& arg : args) {
        std::string ph = "$" + std::to_string(i++);
        size_t pos;
        while ((pos = result.find(ph)) != std::string::npos)
            result.replace(pos, ph.size(), arg);
    }
    return result;
}

void registerLocale(
    const char* loc,
    std::initializer_list<std::pair<const char*, const char*>> entries)
{
    // 注意：registerLocale 必须在第一次调用 translate() 之前全部完成（通常由 static initializer 保证）
    // 注册完成后不可再追加，否则 unordered_map rehash 会使已返回的 c_str() 指针失效
    auto& table = tables()[loc];
    for (const auto& e : entries)
        table[e.first] = e.second;
}

} // namespace imgui_i18n
