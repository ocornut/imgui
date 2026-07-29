# Dear ImGui — i18n (Internationalization)

Runtime language switching for `imgui_demo.cpp`.  
Currently ships with **Simplified Chinese** (`zh_CN`).

---

## How It Works

| Component | Role |
|-----------|------|
| `imgui_i18n.h` | `Tr()` macro + namespace declaration |
| `imgui_i18n.cpp` | Lookup table, fallback, `$1`/`$2` placeholder substitution |
| `locale/zh_CN.cpp` | Chinese translation table (static initializer, auto-registered) |
| `tools/extract_strings.py` | Helper to extract translatable strings from a source range |

Every user-visible string literal in `imgui_demo.cpp` is wrapped with `Tr("...")`.  
`Tr()` looks up the active locale table and returns the translated string, or falls back to the original English if no translation exists.

### Opt-in build flag

Define **`IMGUI_DEMO_ENABLE_I18N`** in your build to activate i18n.  
Without it, `Tr(s)` expands to `(s)` at compile time — zero overhead, no extra files needed.

---

## Quick Start (macOS example_apple_metal)

```bash
# Makefile already includes IMGUI_DEMO_ENABLE_I18N and the i18n sources.
cd examples/example_apple_metal
make && ./example_apple_metal
# Open the Language menu in the demo window menu bar → switch to 中文
```

For other platforms/examples, see **Integrating into Your Build** below.

---

## Adding a New Language

### 1. Create a locale file

Copy `locale/zh_CN.cpp` to `locale/<locale_id>.cpp` (e.g. `locale/ja_JP.cpp`).

```cpp
// locale/ja_JP.cpp
#include "../imgui_i18n.h"

namespace {
struct JaJPRegistrar {
    JaJPRegistrar() {
        imgui_i18n::registerLocale("ja_JP", {
            {"Configuration",  "設定"},
            {"Dear ImGui Demo","Dear ImGui デモ"},
            // ... add all entries
        });
    }
} s_ja_jp;
} // namespace
```

**Key rules:**
- The **key** is the exact English string as it appears in `imgui_demo.cpp` (including punctuation and newlines).
- Missing keys automatically fall back to English — you can ship a partial translation.
- For strings containing `##` (ImGui internal IDs like `"Configuration##2"`), the key in the table is only the visible part (`"Configuration"`); the `##2` suffix is appended in code.

### 2. Add the locale file to your build

**Makefile (Linux/macOS):**
```makefile
SOURCES += $(IMGUI_DIR)/misc/i18n/imgui_i18n.cpp
SOURCES += $(IMGUI_DIR)/misc/i18n/locale/ja_JP.cpp   # add your new locale
CXXFLAGS += -I$(IMGUI_DIR)/i18n -DIMGUI_DEMO_ENABLE_I18N
```

**CMake:**
```cmake
target_sources(my_app PRIVATE
    ${IMGUI_DIR}/misc/i18n/imgui_i18n.cpp
    ${IMGUI_DIR}/misc/i18n/locale/ja_JP.cpp
)
target_include_directories(my_app PRIVATE ${IMGUI_DIR}/i18n)
target_compile_definitions(my_app PRIVATE IMGUI_DEMO_ENABLE_I18N)
```

**Xcode:** Add both `.cpp` files to the target's *Compile Sources* phase.

### 3. Add a menu item for the new language

In `imgui_demo.cpp`, find the Language menu (search for `BeginMenu(Tr("Language"))`):

```cpp
if (ImGui::MenuItem("日本語", nullptr, is_ja) && !is_ja)
{
    imgui_i18n::setLocale("ja_JP");
#if defined(__APPLE__)
    g_need_font_rebuild = true;   // triggers CJK font reload on Apple Metal
#endif
}
```

### 4. Font loading (CJK / non-Latin scripts)

For languages requiring non-Latin glyphs, add font loading logic in your platform's
`main` file (see `examples/example_apple_metal/main.mm` → `RebuildFonts()` for reference).

The pattern:
1. Call `io.Fonts->AddFontDefault(&cfg)` with explicit `SizePixels` first (required by ImGui v1.92+).
2. Merge the target-language font with `cfg.MergeMode = true`.
3. Choose glyph ranges: use `io.Fonts->GetGlyphRangesChineseSimplifiedCommon()` for Chinese,
   `GetGlyphRangesJapanese()` for Japanese, etc.
4. Call `io.Fonts->Build()`.

---

## Translation Guidelines

Follow these rules when translating strings to ensure quality and consistency.

### What to translate

Translate user-visible UI labels: menu items, button labels, window titles, section
headers, tooltip descriptions, and status messages.

### What NOT to translate

| Category | Example | Reason |
|----------|---------|--------|
| Universally recognized programming examples | `"Hello, world!"` | Part of global programming culture; keeping English avoids confusion for learners following tutorials |
| Standard boilerplate example text | `"This is some useful text."` | Canonical ImGui getting-started template text |
| C++ type/variable names used as labels | `"float"`, `"counter = %d"` | Labels the underlying type or variable name, not a description |
| Intentional filler/nonsense | `"blah blah blah"` | The meaninglessness is the message; translating it to real words defeats the purpose |
| API / function names | `"IsItemHovered()"` | Must remain identical to the actual API; adding an entry that maps to itself is redundant |
| Brand / product names | `"Dear ImGui"`, `"dear imgui"` | Proper nouns; do not translate |
| `##`-suffixed ImGui IDs | `"Config##2"` | The `##` part is a disambiguation suffix handled in code, not user-visible |

### Translation quality rules

1. **Accuracy over literalness.** Translate meaning, not word-by-word.  
   Bad: `Metrics` → `性能` (performance)  
   Good: `Metrics` → `指标` (indicators/measurements)

2. **Preserve format specifiers.** `%d`, `%s`, `%.3f`, `$1`, `$2` must appear unchanged in the translated string.

3. **Preserve newlines.** `\n` positions in the translation may differ from the English, but must be present where the UI needs line breaks.

4. **Preserve punctuation intent.** If the English ends with `:`, the translation should too (adjusted for the target language's punctuation conventions).

5. **Redundant identity entries are noise.** If a string needs no translation (e.g. an API function name), do **not** add `{"Foo()", "Foo()"}` to the table. The fallback mechanism returns the key automatically.

---

## Extracting Strings for Translation

Use the helper script to generate a translation skeleton for a range of lines:

```bash
cd imgui-master    # or wherever imgui_demo.cpp lives
python3 misc/i18n/tools/extract_strings.py imgui_demo.cpp <start_line> <end_line>
```

Output is ready-to-paste `{"English key", ""},` entries.  
Paste them into your locale file and fill in the translated values.

---

## Integrating into Your Build (Other Examples)

Only `example_apple_metal` ships with i18n enabled by default.  
To enable it for any other example:

1. Add `imgui_i18n.cpp` and your locale `.cpp` files to the build.
2. Add `-I<imgui_root>/i18n` to include paths.
3. Add `-DIMGUI_DEMO_ENABLE_I18N` to compiler flags.
4. Add font loading + language-switch UI to your platform's `main` file  
   (use `examples/example_apple_metal/main.mm` as a reference).

---

## API Reference

```cpp
// imgui_i18n.h (available when IMGUI_DEMO_ENABLE_I18N is defined)

Tr("key")                         // translate key, return const char*
TrF("Draw $1 of $2", {"3","10"}) // translate + substitute placeholders

imgui_i18n::setLocale("zh_CN");   // switch language (call before NewFrame)
imgui_i18n::getLocale();          // returns current locale string ("" = English)
imgui_i18n::registerLocale(       // called by locale/*.cpp static initializers
    "locale_id",
    { {"English key", "Translation"}, ... }
);
```

---

## Current Translations

| Locale | Language | Coverage |
|--------|----------|----------|
| *(default)* | English | 100% (source) |
| `zh_CN` | Simplified Chinese | ~100% of `imgui_demo.cpp` |

Contributions welcome — see **Adding a New Language** above.
