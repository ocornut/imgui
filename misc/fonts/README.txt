dear imgui, v1.72 WIP
(Font Readme)

---------------------------------------

The code in imgui.cpp embeds a copy of 'ProggyClean.ttf' (by Tristan Grimmer),
a 13 pixels high, pixel-perfect font used by default.
We embed it font in source code so you can use Dear ImGui without any file system access.

You may also load external .TTF/.OTF files.
The files in this folder are suggested fonts, provided as a convenience.

Fonts are rasterized in a single texture at the time of calling either of io.Fonts->GetTexDataAsAlpha8()/GetTexDataAsRGBA32()/Build().
Also read dear imgui FAQ in imgui.cpp!

If you have other loading/merging/adding fonts, you can post on the Dear ImGui "Getting Started" forum:
  https://discourse.dearimgui.org/c/getting-started


---------------------------------------
 INDEX:
---------------------------------------

- Readme First / FAQ
- Using Icons
- Fonts Loading Instructions
- FreeType rasterizer, Small font sizes
- Building Custom Glyph Ranges
- Using custom colorful icons
- Embedding Fonts in Source Code
- Credits/Licences for fonts included in this folder
- Fonts Links


---------------------------------------
 README FIRST / FAQ
---------------------------------------

- You can use the style editor ImGui::ShowStyleEditor() in the "Fonts" section to browse your fonts
  and understand what's going on if you have an issue.
- Make sure your font ranges data are persistent (available during the call to GetTexDataAsAlpha8()/GetTexDataAsRGBA32()/Build().
- Use C++11 u8"my text" syntax to encode literal strings as UTF-8. e.g.:
      u8"hello"
      u8"こんにちは"   // this will be encoded as UTF-8
- If you want to include a backslash \ character in your string literal, you need to double them e.g. "folder\\filename".
- Please use the Discourse forum (https://discourse.dearimgui.org) and not the Github issue tracker for basic font loading questions.


---------------------------------------
 USING ICONS
---------------------------------------

Using an icon font (such as FontAwesome: http://fontawesome.io or OpenFontIcons. https://github.com/traverseda/OpenFontIcons)
is an easy and practical way to use icons in your Dear ImGui application.
A common pattern is to merge the icon font within your main font, so you can embed icons directly from your strings without
having to change fonts back and forth.

To refer to the icon UTF-8 codepoints from your C++ code, you may use those headers files created by Juliette Foucaut:
  https://github.com/juliettef/IconFontCppHeaders

The C++11 version of those files uses the u8"" utf-8 encoding syntax + \u
  #define ICON_FA_SEARCH  u8"\uf002"
The pre-C++11 version has the values directly encoded as utf-8:
  #define ICON_FA_SEARCH  "\xEF\x80\x82"

Example Setup:

  // Merge icons into default tool font
  #include "IconsFontAwesome.h"
  ImGuiIO& io = ImGui::GetIO();
  io.Fonts->AddFontDefault();

  ImFontConfig config;
  config.MergeMode = true;
  config.GlyphMinAdvanceX = 13.0f; // Use if you want to make the icon monospaced
  static const ImWchar icon_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
  io.Fonts->AddFontFromFileTTF("fonts/fontawesome-webfont.ttf", 13.0f, &config, icon_ranges);

Example Usage:

  // Usage, e.g.
  ImGui::Text("%s among %d items", ICON_FA_SEARCH, count);
  ImGui::Button(ICON_FA_SEARCH " Search");
  // C string _literals_ can be concatenated at compilation time, e.g. "hello" " world"
  // ICON_FA_SEARCH is defined as a string literal so this is the same as "A" "B" becoming "AB"

See Links below for other icons fonts and related tools.


---------------------------------------
 FONTS LOADING INSTRUCTIONS
---------------------------------------

Load default font:

  ImGuiIO& io = ImGui::GetIO();
  io.Fonts->AddFontDefault();

Load .TTF/.OTF file with:

  ImGuiIO& io = ImGui::GetIO();
  ImFont* font1 = io.Fonts->AddFontFromFileTTF("font.ttf", size_pixels);
  ImFont* font2 = io.Fonts->AddFontFromFileTTF("anotherfont.otf", size_pixels);

  // Select font at runtime
  ImGui::Text("Hello");	// use the default font (which is the first loaded font)
  ImGui::PushFont(font2);
  ImGui::Text("Hello with another font");
  ImGui::PopFont();

For advanced options create a ImFontConfig structure and pass it to the AddFont function (it will be copied internally):

  ImFontConfig config;
  config.OversampleH = 2;
  config.OversampleV = 1;
  config.GlyphExtraSpacing.x = 1.0f;
  ImFont* font = io.Fonts->AddFontFromFileTTF("font.ttf", size_pixels, &config);

Read about oversampling here:
  https://github.com/nothings/stb/blob/master/tests/oversample

If you have very large number of glyphs or multiple fonts, the texture may become too big for your graphics API.
The typical result of failing to upload a texture is if every glyphs appears as white rectangles.
In particular, using a large range such as GetGlyphRangesChineseSimplifiedCommon() is not recommended unless you
set OversampleH/OversampleV to 1 and use a small font size.
Mind the fact that some graphics drivers have texture size limitation.
If you are building a PC application, mind the fact that your users may use hardware with lower limitations than yours.
Some solutions:

 - 1) Reduce glyphs ranges by calculating them from source localization data.
   You can use ImFontGlyphRangesBuilder for this purpose, this will be the biggest win!
 - 2) You may reduce oversampling, e.g. config.OversampleH = config.OversampleV = 1, this will largely reduce your texture size.
 - 3) Set io.Fonts.TexDesiredWidth to specify a texture width to minimize texture height (see comment in ImFontAtlas::Build function).
 - 4) Set io.Fonts.Flags |= ImFontAtlasFlags_NoPowerOfTwoHeight; to disable rounding the texture height to the next power of two.

Combine two fonts into one:

  // Load a first font
  ImFont* font = io.Fonts->AddFontDefault();

  // Add character ranges and merge into the previous font
  // The ranges array is not copied by the AddFont* functions and is used lazily
  // so ensure it is available at the time of building or calling GetTexDataAsRGBA32().
  static const ImWchar icons_ranges[] = { 0xf000, 0xf3ff, 0 }; // Will not be copied by AddFont* so keep in scope.
  ImFontConfig config;
  config.MergeMode = true;
  io.Fonts->AddFontFromFileTTF("DroidSans.ttf", 18.0f, &config, io.Fonts->GetGlyphRangesJapanese());
  io.Fonts->AddFontFromFileTTF("fontawesome-webfont.ttf", 18.0f, &config, icons_ranges);
  io.Fonts->Build();

Add a fourth parameter to bake specific font ranges only:

  // Basic Latin, Extended Latin
  io.Fonts->AddFontFromFileTTF("font.ttf", size_pixels, NULL, io.Fonts->GetGlyphRangesDefault());

  // Default + Selection of 2500 Ideographs used by Simplified Chinese
  io.Fonts->AddFontFromFileTTF("font.ttf", size_pixels, NULL, io.Fonts->GetGlyphRangesChineseSimplifiedCommon());

  // Default + Hiragana, Katakana, Half-Width, Selection of 1946 Ideographs
  io.Fonts->AddFontFromFileTTF("font.ttf", size_pixels, NULL, io.Fonts->GetGlyphRangesJapanese());

See "BUILDING CUSTOM GLYPH RANGES" section to create your own ranges.
Offset font vertically by altering the io.Font->DisplayOffset value:

  ImFont* font = io.Fonts->AddFontFromFileTTF("font.ttf", size_pixels);
  font->DisplayOffset.y = 1;   // Render 1 pixel down


---------------------------------------
 FREETYPE RASTERIZER, SMALL FONT SIZES
---------------------------------------

Dear ImGui uses imstb_truetype.h to rasterize fonts (with optional oversampling).
This technique and its implementation are not ideal for fonts rendered at _small sizes_, which may appear a
little blurry or hard to read.

There is an implementation of the ImFontAtlas builder using FreeType that you can use in the misc/freetype/ folder.

FreeType supports auto-hinting which tends to improve the readability of small fonts.
Note that this code currently creates textures that are unoptimally too large (could be fixed with some work).
Also note that correct sRGB space blending will have an important effect on your font rendering quality.


---------------------------------------
 BUILDING CUSTOM GLYPH RANGES
---------------------------------------

You can use the ImFontGlyphRangesBuilder helper to create glyph ranges based on text input.
For example: for a game where your script is known, if you can feed your entire script to it and only build the characters the game needs.

  ImVector<ImWchar> ranges;
  ImFontGlyphRangesBuilder builder;
  builder.AddText("Hello world");                        // Add a string (here "Hello world" contains 7 unique characters)
  builder.AddChar(0x7262);                               // Add a specific character
  builder.AddRanges(io.Fonts->GetGlyphRangesJapanese()); // Add one of the default ranges
  builder.BuildRanges(&ranges);                          // Build the final result (ordered ranges with all the unique characters submitted)

  io.Fonts->AddFontFromFileTTF("myfontfile.ttf", size_in_pixels, NULL, ranges.Data);
  io.Fonts->Build();                                     // Build the atlas while 'ranges' is still in scope and not deleted.


---------------------------------------
 USING CUSTOM COLORFUL ICONS
---------------------------------------

(This is a BETA api, use if you are familiar with dear imgui and with your rendering back-end)

You can use the ImFontAtlas::AddCustomRect() and ImFontAtlas::AddCustomRectFontGlyph() api to register rectangles
that will be packed into the font atlas texture. Register them before building the atlas, then call Build().
You can then use ImFontAtlas::GetCustomRectByIndex(int) to query the position/size of your rectangle within the
texture, and blit/copy any graphics data of your choice into those rectangles.

Pseudo-code:

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


---------------------------------------
 EMBEDDING FONTS IN SOURCE CODE
---------------------------------------

Compile and use 'binary_to_compressed_c.cpp' to create a compressed C style array that you can embed in source code.
See the documentation in binary_to_compressed_c.cpp for instruction on how to use the tool.
You may find a precompiled version binary_to_compressed_c.exe for Windows instead of demo binaries package (see README).
The tool can optionally output Base85 encoding to reduce the size of _source code_ but the read-only arrays in the
actual binary will be about 20% bigger.

Then load the font with:
  ImFont* font = io.Fonts->AddFontFromMemoryCompressedTTF(compressed_data, compressed_data_size, size_pixels, ...);
or:
  ImFont* font = io.Fonts->AddFontFromMemoryCompressedBase85TTF(compressed_data_base85, size_pixels, ...);


---------------------------------------
 CREDITS/LICENSES FOR FONTS INCLUDED IN THIS FOLDER
---------------------------------------

Roboto-Medium.ttf

  Apache License 2.0
  by Christian Robertson
  https://fonts.google.com/specimen/Roboto

Cousine-Regular.ttf

  by Steve Matteson
  Digitized data copyright (c) 2010 Google Corporation.
  Licensed under the SIL Open Font License, Version 1.1
  https://fonts.google.com/specimen/Cousine

DroidSans.ttf

  Copyright (c) Steve Matteson
  Apache License, version 2.0
  https://www.fontsquirrel.com/fonts/droid-sans

ProggyClean.ttf

  Copyright (c) 2004, 2005 Tristan Grimmer
  MIT License
  recommended loading setting: Size = 13.0, DisplayOffset.Y = +1
  http://www.proggyfonts.net/

ProggyTiny.ttf
  Copyright (c) 2004, 2005 Tristan Grimmer
  MIT License
  recommended loading setting: Size = 10.0, DisplayOffset.Y = +1
  http://www.proggyfonts.net/

Karla-Regular.ttf
  Copyright (c) 2012, Jonathan Pinhorn
  SIL OPEN FONT LICENSE Version 1.1


---------------------------------------
 FONTS LINKS
---------------------------------------

ICON FONTS

  C/C++ header for icon fonts (#define with code points to use in source code string literals)
  https://github.com/juliettef/IconFontCppHeaders

  FontAwesome
  https://fortawesome.github.io/Font-Awesome

  OpenFontIcons
  https://github.com/traverseda/OpenFontIcons

  Google Icon Fonts
  https://design.google.com/icons/

  Kenney Icon Font (Game Controller Icons)
  https://github.com/nicodinh/kenney-icon-font

  IcoMoon - Custom Icon font builder
  https://icomoon.io/app

REGULAR FONTS

  Google Noto Fonts (worldwide languages)
  https://www.google.com/get/noto/

  Open Sans Fonts
  https://fonts.google.com/specimen/Open+Sans

  (Japanese) M+ fonts by Coji Morishita are free
  http://mplus-fonts.sourceforge.jp/mplus-outline-fonts/index-en.html

MONOSPACE FONTS

  (Pixel Perfect) Proggy Fonts, by Tristan Grimmer
  http://www.proggyfonts.net or http://upperbounds.net

  (Pixel Perfect) Sweet16, Sweet16 Mono, by Martin Sedlak (Latin + Supplemental + Extended A)
  https://github.com/kmar/Sweet16Font
  Also include .inl file to use directly in dear imgui.

  Google Noto Mono Fonts
  https://www.google.com/get/noto/

  Typefaces for source code beautification
  https://github.com/chrissimpkins/codeface

  Programmation fonts
  http://s9w.github.io/font_compare/

  Inconsolata
  http://www.levien.com/type/myfonts/inconsolata.html

  Adobe Source Code Pro: Monospaced font family for user interface and coding environments
  https://github.com/adobe-fonts/source-code-pro

  Monospace/Fixed Width Programmer's Fonts
  http://www.lowing.org/fonts/


 Or use Arial Unicode or other Unicode fonts provided with Windows for full characters coverage (not sure of their licensing).
