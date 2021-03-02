_(You may browse this at https://github.com/ocornut/imgui/blob/master/docs/FONTS.md or view this file with any Markdown viewer)_

## Dear ImGui: Using Fonts

The code in imgui.cpp embeds a copy of 'ProggyClean.ttf' (by Tristan Grimmer),
a 13 pixels high, pixel-perfect font used by default. We embed it in the source code so you can use Dear ImGui without any file system access. ProggyClean does not scale smoothly, therefore it is recommended that you load your own file when using Dear ImGui in an application aiming to look nice and wanting to support multiple resolutions.

You may also load external .TTF/.OTF files. 
In the [misc/fonts/](https://github.com/ocornut/imgui/tree/master/misc/fonts) folder you can find a few suggested fonts, provided as a convenience.

**Also read the FAQ:** https://www.dearimgui.org/faq (there is a Fonts section!)

## Index
- [Readme First](#readme-first)
- [How should I handle DPI in my application?](#how-should-i-handle-dpi-in-my-application)
- [Fonts Loading Instructions](#font-loading-instructions)
- [Using Icons](#using-icons)
- [Using FreeType Rasterizer](#using-freetype-rasterizer)
- [Using Custom Glyph Ranges](#using-custom-glyph-ranges)
- [Using Custom Colorful Icons](#using-custom-colorful-icons)
- [Using Font Data Embedded In Source Code](#using-font-data-embedded-in-source-code)
- [About filenames](#about-filenames)
- [Credits/Licenses For Fonts Included In Repository](#creditslicenses-for-fonts-included-in-repository)
- [Font Links](#font-links)

---------------------------------------
 ## Readme First

- All loaded fonts glyphs are rendered into a single texture atlas ahead of time. Calling either of `io.Fonts->GetTexDataAsAlpha8()`, `io.Fonts->GetTexDataAsRGBA32()` or `io.Fonts->Build()` will build the atlas. 

- You can use the style editor `ImGui::ShowStyleEditor()` in the "Fonts" section to browse your fonts and understand what's going on if you have an issue. You can also reach it in `Demo->Tools->Style Editor->Fonts`:

![imgui_capture_0008](https://user-images.githubusercontent.com/8225057/84162822-1a731f00-aa71-11ea-9b6b-89c2332aa161.png)

- Make sure your font ranges data are persistent (available during the calls to `GetTexDataAsAlpha8()`/`GetTexDataAsRGBA32()/`Build()`.

- Use C++11 u8"my text" syntax to encode literal strings as UTF-8. e.g.:
```cpp
u8"hello"
u8"こんにちは"   // this will be encoded as UTF-8
```

##### [Return to Index](#index)

## How should I handle DPI in my application?

See [FAQ entry](https://github.com/ocornut/imgui/blob/master/docs/FAQ.md#q-how-should-i-handle-dpi-in-my-application).

##### [Return to Index](#index)


## Font Loading Instructions

**Load default font:**
```cpp
ImGuiIO& io = ImGui::GetIO();
io.Fonts->AddFontDefault();
```


**Load .TTF/.OTF file with:**
```cpp
ImGuiIO& io = ImGui::GetIO();
io.Fonts->AddFontFromFileTTF("font.ttf", size_pixels);
```
If you get an assert stating "Could not load font file!", your font filename is likely incorrect. Read "[About filenames](#about-filenames)" carefully.


**Load multiple fonts:**
```cpp
// Init
ImGuiIO& io = ImGui::GetIO();
ImFont* font1 = io.Fonts->AddFontFromFileTTF("font.ttf", size_pixels);
ImFont* font2 = io.Fonts->AddFontFromFileTTF("anotherfont.otf", size_pixels);
```
```cpp
// In application loop: select font at runtime
ImGui::Text("Hello"); // use the default font (which is the first loaded font)
ImGui::PushFont(font2);
ImGui::Text("Hello with another font");
ImGui::PopFont();
```


**For advanced options create a ImFontConfig structure and pass it to the AddFont() function (it will be copied internally):**
```cpp
ImFontConfig config;
config.OversampleH = 2;
config.OversampleV = 1;
config.GlyphExtraSpacing.x = 1.0f;
ImFont* font = io.Fonts->AddFontFromFileTTF("font.ttf", size_pixels, &config);
```


**Combine multiple fonts into one:**
```cpp
// Load a first font
ImFont* font = io.Fonts->AddFontDefault();

// Add character ranges and merge into the previous font
// The ranges array is not copied by the AddFont* functions and is used lazily
// so ensure it is available at the time of building or calling GetTexDataAsRGBA32().
static const ImWchar icons_ranges[] = { 0xf000, 0xf3ff, 0 }; // Will not be copied by AddFont* so keep in scope.
ImFontConfig config;
config.MergeMode = true;
io.Fonts->AddFontFromFileTTF("DroidSans.ttf", 18.0f, &config, io.Fonts->GetGlyphRangesJapanese()); // Merge into first font
io.Fonts->AddFontFromFileTTF("fontawesome-webfont.ttf", 18.0f, &config, icons_ranges);             // Merge into first font
io.Fonts->Build();
```

**Add a fourth parameter to bake specific font ranges only:**

```cpp
// Basic Latin, Extended Latin
io.Fonts->AddFontFromFileTTF("font.ttf", size_pixels, NULL, io.Fonts->GetGlyphRangesDefault());

// Default + Selection of 2500 Ideographs used by Simplified Chinese
io.Fonts->AddFontFromFileTTF("font.ttf", size_pixels, NULL, io.Fonts->GetGlyphRangesChineseSimplifiedCommon());

// Default + Hiragana, Katakana, Half-Width, Selection of 1946 Ideographs
io.Fonts->AddFontFromFileTTF("font.ttf", size_pixels, NULL, io.Fonts->GetGlyphRangesJapanese());
```
See [Using Custom Glyph Ranges](#using-custom-glyph-ranges) section to create your own ranges.


**Example loading and using a Japanese font:**

```cpp
ImGuiIO& io = ImGui::GetIO();
io.Fonts->AddFontFromFileTTF("NotoSansCJKjp-Medium.otf", 20.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
```
```cpp
ImGui::Text(u8"こんにちは！テスト %d", 123);
if (ImGui::Button(u8"ロード"))
{
    // do stuff
}
ImGui::InputText("string", buf, IM_ARRAYSIZE(buf));
ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
```

![sample code output](https://raw.githubusercontent.com/wiki/ocornut/imgui/web/v160/code_sample_02_jp.png)
<br>_(settings: Dark style (left), Light style (right) / Font: NotoSansCJKjp-Medium, 20px / Rounding: 5)_

**Font Atlas too large?**

- If you have very large number of glyphs or multiple fonts, the texture may become too big for your graphics API. The typical result of failing to upload a texture is if every glyphs appears as white rectangles.
- Mind the fact that some graphics drivers have texture size limitation. If you are building a PC application, mind the fact that your users may use hardware with lower limitations than yours.

Some solutions:

1. Reduce glyphs ranges by calculating them from source localization data.
   You can use the `ImFontGlyphRangesBuilder` for this purpose and rebuilding your atlas between frames when new characters are needed. This will be the biggest win!
2. You may reduce oversampling, e.g. `font_config.OversampleH = 2`, this will largely reduce your texture size.
   Note that while OversampleH = 2 looks visibly very close to 3 in most situations, with OversampleH = 1 the quality drop will be noticeable.
3. Set `io.Fonts.TexDesiredWidth` to specify a texture width to minimize texture height (see comment in `ImFontAtlas::Build()` function).
4. Set `io.Fonts.Flags |= ImFontAtlasFlags_NoPowerOfTwoHeight;` to disable rounding the texture height to the next power of two.
5. Read about oversampling [here](https://github.com/nothings/stb/blob/master/tests/oversample).
6. To support the extended range of unicode beyond 0xFFFF (e.g. emoticons, dingbats, symbols, shapes, ancient languages, etc...) add `#define IMGUI_USE_WCHAR32`in your `imconfig.h`.

##### [Return to Index](#index)

## Using Icons

Using an icon font (such as [FontAwesome](http://fontawesome.io) or [OpenFontIcons](https://github.com/traverseda/OpenFontIcons)) is an easy and practical way to use icons in your Dear ImGui application.
A common pattern is to merge the icon font within your main font, so you can embed icons directly from your strings without having to change fonts back and forth.

To refer to the icon UTF-8 codepoints from your C++ code, you may use those headers files created by Juliette Foucaut: https://github.com/juliettef/IconFontCppHeaders.
  
So you can use `ICON_FA_SEARCH` as a string that will render as a "Search" icon.

Example Setup:
```cpp
// Merge icons into default tool font
#include "IconsFontAwesome.h"
ImGuiIO& io = ImGui::GetIO();
io.Fonts->AddFontDefault();

ImFontConfig config;
config.MergeMode = true;
config.GlyphMinAdvanceX = 13.0f; // Use if you want to make the icon monospaced
static const ImWchar icon_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
io.Fonts->AddFontFromFileTTF("fonts/fontawesome-webfont.ttf", 13.0f, &config, icon_ranges);
```
Example Usage:
```cpp
// Usage, e.g.
ImGui::Text("%s among %d items", ICON_FA_SEARCH, count);
ImGui::Button(ICON_FA_SEARCH " Search");
// C string _literals_ can be concatenated at compilation time, e.g. "hello" " world"
// ICON_FA_SEARCH is defined as a string literal so this is the same as "A" "B" becoming "AB"
```
See Links below for other icons fonts and related tools.

Here's an application using icons ("Avoyd", https://www.avoyd.com):
![avoyd](https://user-images.githubusercontent.com/8225057/81696852-c15d9e80-9464-11ea-9cab-2a4d4fc84396.jpg)

##### [Return to Index](#index)

## Using FreeType Rasterizer

- Dear ImGui uses imstb\_truetype.h to rasterize fonts (with optional oversampling). This technique and its implementation are not ideal for fonts rendered at small sizes, which may appear a little blurry or hard to read.
- There is an implementation of the ImFontAtlas builder using FreeType that you can use in the [misc/freetype/](https://github.com/ocornut/imgui/tree/master/misc/freetype) folder.
- FreeType supports auto-hinting which tends to improve the readability of small fonts.
- Read documentation in the [misc/freetype/](https://github.com/ocornut/imgui/tree/master/misc/freetype) folder.
- Correct sRGB space blending will have an important effect on your font rendering quality.

##### [Return to Index](#index)

## Using Custom Glyph Ranges

You can use the `ImFontGlyphRangesBuilder` helper to create glyph ranges based on text input. For example: for a game where your script is known, if you can feed your entire script to it and only build the characters the game needs.
```cpp
ImVector<ImWchar> ranges;
ImFontGlyphRangesBuilder builder;
builder.AddText("Hello world");                        // Add a string (here "Hello world" contains 7 unique characters)
builder.AddChar(0x7262);                               // Add a specific character
builder.AddRanges(io.Fonts->GetGlyphRangesJapanese()); // Add one of the default ranges
builder.BuildRanges(&ranges);                          // Build the final result (ordered ranges with all the unique characters submitted)

io.Fonts->AddFontFromFileTTF("myfontfile.ttf", size_in_pixels, NULL, ranges.Data);
io.Fonts->Build();                                     // Build the atlas while 'ranges' is still in scope and not deleted.
```

##### [Return to Index](#index)

## Using Custom Colorful Icons

**(This is a BETA api, use if you are familiar with dear imgui and with your rendering backend)**

- You can use the `ImFontAtlas::AddCustomRect()` and `ImFontAtlas::AddCustomRectFontGlyph()` api to register rectangles that will be packed into the font atlas texture. Register them before building the atlas, then call Build()`.
- You can then use `ImFontAtlas::GetCustomRectByIndex(int)` to query the position/size of your rectangle within the texture, and blit/copy any graphics data of your choice into those rectangles.
- This API is beta because it is likely to change in order to support multi-dpi (multiple viewports on multiple monitors with varying DPI scale).

#### Pseudo-code:
```cpp
// Add font, then register two custom 13x13 rectangles mapped to glyph 'a' and 'b' of this font
ImFont* font = io.Fonts->AddFontDefault();
int rect_ids[2];
rect_ids[0] = io.Fonts->AddCustomRectFontGlyph(font, 'a', 13, 13, 13+1);
rect_ids[1] = io.Fonts->AddCustomRectFontGlyph(font, 'b', 13, 13, 13+1);

// Build atlas
io.Fonts->Build();

// Retrieve texture in RGBA format
unsigned char* tex_pixels = NULL;
int tex_width, tex_height;
io.Fonts->GetTexDataAsRGBA32(&tex_pixels, &tex_width, &tex_height);

for (int rect_n = 0; rect_n < IM_ARRAYSIZE(rect_ids); rect_n++)
{
    int rect_id = rects_ids[rect_n];
    if (const ImFontAtlas::CustomRect* rect = io.Fonts->GetCustomRectByIndex(rect_id))
    {
        // Fill the custom rectangle with red pixels (in reality you would draw/copy your bitmap data here!)
        for (int y = 0; y < rect->Height; y++)
        {
            ImU32* p = (ImU32*)tex_pixels + (rect->Y + y) * tex_width + (rect->X);
            for (int x = rect->Width; x > 0; x--)
                *p++ = IM_COL32(255, 0, 0, 255);
        }
    }
}
```

##### [Return to Index](#index)

## Using Font Data Embedded In Source Code

- Compile and use [binary_to_compressed_c.cpp](https://github.com/ocornut/imgui/blob/master/misc/fonts/binary_to_compressed_c.cpp) to create a compressed C style array that you can embed in source code.
- See the documentation in [binary_to_compressed_c.cpp](https://github.com/ocornut/imgui/blob/master/misc/fonts/binary_to_compressed_c.cpp) for instruction on how to use the tool.
- You may find a precompiled version binary_to_compressed_c.exe for Windows instead of demo binaries package (see [README](https://github.com/ocornut/imgui/blob/master/docs/README.md)).
- The tool can optionally output Base85 encoding to reduce the size of _source code_ but the read-only arrays in the actual binary will be about 20% bigger.

Then load the font with:
```cpp
ImFont* font = io.Fonts->AddFontFromMemoryCompressedTTF(compressed_data, compressed_data_size, size_pixels, ...);
```
or
```cpp
ImFont* font = io.Fonts->AddFontFromMemoryCompressedBase85TTF(compressed_data_base85, size_pixels, ...);
```

##### [Return to Index](#index)

## About filenames

**Please note that many new C/C++ users have issues their files _because the filename they provide is wrong_.**

Two things to watch for:
- Make sure your IDE/debugger settings starts your executable from the right working directory. In Visual Studio you can change your working directory in project `Properties > General > Debugging > Working Directory`. People assume that their execution will start from the root folder of the project, where by default it oftens start from the folder where object or executable files are stored.
```cpp
// Relative filename depends on your Working Directory when running your program!
io.Fonts->AddFontFromFileTTF("MyImage01.jpg", ...);

// Load from the parent folder of your Working Directory
io.Fonts->AddFontFromFileTTF("../MyImage01.jpg", ...);
```
- In C/C++ and most programming languages if you want to use a backslash `\` within a string literal, you need to write it double backslash `\\`. At it happens, Windows uses backslashes as a path separator, so be mindful.
```cpp
io.Fonts->AddFontFromFileTTF("MyFiles\MyImage01.jpg", ...);   // This is INCORRECT!!
io.Fonts->AddFontFromFileTTF("MyFiles\\MyImage01.jpg", ...);  // This is CORRECT
```
In some situations, you may also use `/` path separator under Windows.

##### [Return to Index](#index)

## Credits/Licenses For Fonts Included In Repository

Some fonts files are available in the `misc/fonts/` folder:

**Roboto-Medium.ttf**, by Christian Robetson
<br>Apache License 2.0
<br>https://fonts.google.com/specimen/Roboto

**Cousine-Regular.ttf**, by Steve Matteson
<br>Digitized data copyright (c) 2010 Google Corporation. 
<br>Licensed under the SIL Open Font License, Version 1.1
<br>https://fonts.google.com/specimen/Cousine

**DroidSans.ttf**, by Steve Matteson
<br>Apache License 2.0
<br>https://www.fontsquirrel.com/fonts/droid-sans

**ProggyClean.ttf**, by Tristan Grimmer
<br>MIT License
<br>(recommended loading setting: Size = 13.0, GlyphOffset.y = +1)
<br>http://www.proggyfonts.net/

**ProggyTiny.ttf**, by Tristan Grimmer
<br>MIT License
<br>(recommended loading setting: Size = 10.0, GlyphOffset.y = +1)
<br>http://www.proggyfonts.net/

**Karla-Regular.ttf**, by Jonathan Pinhorn
<br>SIL OPEN FONT LICENSE Version 1.1

##### [Return to Index](#index)

## Font Links

#### ICON FONTS

- C/C++ header for icon fonts (#define with code points to use in source code string literals) https://github.com/juliettef/IconFontCppHeaders
- FontAwesome https://fortawesome.github.io/Font-Awesome
- OpenFontIcons https://github.com/traverseda/OpenFontIcons
- Google Icon Fonts https://design.google.com/icons/
- Kenney Icon Font (Game Controller Icons) https://github.com/nicodinh/kenney-icon-font
- IcoMoon - Custom Icon font builder https://icomoon.io/app

#### REGULAR FONTS

- Google Noto Fonts (worldwide languages) https://www.google.com/get/noto/
- Open Sans Fonts https://fonts.google.com/specimen/Open+Sans
- (Japanese) M+ fonts by Coji Morishita http://mplus-fonts.sourceforge.jp/mplus-outline-fonts/index-en.html

#### MONOSPACE FONTS

Pixel Perfect:
- Proggy Fonts, by Tristan Grimmer http://www.proggyfonts.net or http://upperbounds.net
- Sweet16, Sweet16 Mono, by Martin Sedlak (Latin + Supplemental + Extended A) https://github.com/kmar/Sweet16Font (also include an .inl file to use directly in dear imgui.)

Regular:
- Google Noto Mono Fonts https://www.google.com/get/noto/
- Typefaces for source code beautification https://github.com/chrissimpkins/codeface
- Programmation fonts http://s9w.github.io/font_compare/
- Inconsolata http://www.levien.com/type/myfonts/inconsolata.html
- Adobe Source Code Pro: Monospaced font family for ui & coding environments https://github.com/adobe-fonts/source-code-pro
- Monospace/Fixed Width Programmer's Fonts http://www.lowing.org/fonts/

Or use Arial Unicode or other Unicode fonts provided with Windows for full characters coverage (not sure of their licensing).

##### [Return to Index](#index)
