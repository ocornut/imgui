// dear imgui - i18n (internationalization) module
// Provides runtime language switching for imgui_demo.cpp.
//
// Usage:
//   Define IMGUI_DEMO_ENABLE_I18N in your build system to opt in.
//   Without it, Tr(s) is a no-op and other examples are unaffected.
//
// See i18n/README.md for details and how to add a new language.
#pragma once

#ifdef IMGUI_DEMO_ENABLE_I18N

#include <string>
#include <initializer_list>
#include <utility>

// Wrap every user-visible string literal in imgui_demo.cpp with Tr().
#define Tr(s)       imgui_i18n::translate(s)
// Variant with $1/$2/... placeholder substitution.
#define TrF(s, ...) imgui_i18n::fmt(s, __VA_ARGS__)

namespace imgui_i18n {

// Set active locale: "" or "en" = English (default, zero-overhead fast path).
void        setLocale(const char* locale);
const char* getLocale();

// Look up key in the active locale table; falls back to key itself if not found.
const char* translate(const char* key);

// Replace $1, $2, ... placeholders with args (in order).
std::string fmt(const char* key, std::initializer_list<std::string> args);

// Register a translation table for a locale (called by locale/*.cpp static initializers).
void registerLocale(
    const char* locale,
    std::initializer_list<std::pair<const char*, const char*>> entries);

} // namespace imgui_i18n

#else // IMGUI_DEMO_ENABLE_I18N not defined

// Stub: Tr() is a compile-time identity — zero overhead, no dependencies.
#define Tr(s)  (s)
#define TrF(s, ...) (s)

#endif // IMGUI_DEMO_ENABLE_I18N
