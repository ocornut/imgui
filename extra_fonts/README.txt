
 The code in imgui.cpp embeds a copy of 'ProggyClean.ttf' (by Tristan Grimmer) that is used by default.
 We embed the font in source code so you can use Dear ImGui without any file system access.
 You may also load external .TTF/.OTF files. 
 The files in this folder are suggested fonts, provided as a convenience.
 (Note: .OTF support in stb_truetype.h currently doesn't appear to load every font)

 Fonts are rasterized in a single texture at the time of calling either of io.Fonts.GetTexDataAsAlpha8()/GetTexDataAsRGBA32()/Build().

---------------------------------
 USING ICONS
---------------------------------

 Using an icon font (such as FontAwesome: http://fontawesome.io) is an easy and practical way to use icons in your ImGui application.
 A common pattern is to merge the icon font within your main font, so you can refer to the icons directly from your strings without having to change fonts back and forth.
 To refer to the icon from your C++ code, you can use headers files created by Juliette Foucaut, at https://github.com/juliettef/IconFontCppHeaders
 See Links below for other icons fonts and related tools.

    // Merge icons into default tool font
   #include "IconsFontAwesome.h"
   ImGuiIO& io = ImGui::GetIO();
   io.Fonts->AddFontDefault();
   ImFontConfig config;
   config.MergeMode = true;
   static const ImWchar icon_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
   io.Fonts->AddFontFromFileTTF("fonts/fontawesome-webfont.ttf", 13.0f, &config, icon_ranges);

   // Usage, e.g.
   ImGui::Text("%s Search", ICON_FA_SEARCH);


---------------------------------
 FONTS LOADING INSTRUCTIONS
---------------------------------

 Load default font with:

   ImGuiIO& io = ImGui::GetIO();
   io.Fonts->AddFontDefault();

 Load .TTF/.OTF file with:

   ImGuiIO& io = ImGui::GetIO();
   io.Fonts->AddFontFromFileTTF("font.ttf", size_pixels);
  
 Advanced options:

   ImFontConfig config;
   config.OversampleH = 3;
   config.OversampleV = 1;
   config.GlyphExtraSpacing.x = 1.0f;
   io.Fonts->AddFontFromFileTTF("font.ttf", size_pixels, &config);

 If you have very large number of glyphs or multiple fonts:

  - Mind the fact that some graphics drivers have texture size limitation.
  - Set io.Fonts.TexDesiredWidth to specify a texture width to minimize texture height (see comment in ImFontAtlas::Build function).
  - You may reduce oversampling, e.g. config.OversampleH = 2 or 1.
  - Reduce glyphs ranges, consider calculating them based on your source data if this is possible.

 Combine two fonts into one:

   // Load a first font
   io.Fonts->AddFontDefault();

   // Add character ranges and merge into the previous font
   // The ranges array is not copied by the AddFont* functions and is used lazily
   // so ensure it is available for duration of font usage
   static const ImWchar icons_ranges[] = { 0xf000, 0xf3ff, 0 }; // will not be copied by AddFont* so keep in scope.
   ImFontConfig config;
   config.MergeMode = true;
   io.Fonts->AddFontFromFileTTF("DroidSans.ttf", 18.0f, &config, io.Fonts->GetGlyphRangesJapanese());
   io.Fonts->AddFontFromFileTTF("fontawesome-webfont.ttf", 18.0f, &config, icons_ranges);

 Add a fourth parameter to bake specific font ranges only:

   // Basic Latin, Extended Latin
   io.Fonts->AddFontFromFileTTF("font.ttf", size_pixels, NULL, io.Fonts->GetGlyphRangesDefault());
   
   // Include full set of about 21000 CJK Unified Ideographs
   io.Fonts->AddFontFromFileTTF("font.ttf", size_pixels, NULL, io.Fonts->GetGlyphRangesJapanese());
   
   // Default + Hiragana, Katakana, Half-Width, Selection of 1946 Ideographs
   io.Fonts->AddFontFromFileTTF("font.ttf", size_pixels, NULL, io.Fonts->GetGlyphRangesChinese());

 Offset font vertically by altering the io.Font->DisplayOffset value:

   ImFont* font = io.Fonts->AddFontFromFileTTF("font.ttf", size_pixels);
   font->DisplayOffset.y += 1;   // Render 1 pixel down


---------------------------------
 BUILDING CUSTOM GLYPH RANGES
---------------------------------

 You can use the ImFontAtlas::GlyphRangesBuilder helper to create glyph ranges based on text input.
 For exemple: for a game where your script is known, if you can feed your entire script to it and only build the characters the game needs. 

   ImVector<ImWchar> ranges;
   ImFontAtlas::GlyphRangesBuilder builder;
   builder.AddText("Hello world");                        // Add a string (here "Hello world" contains 7 unique characters)
   builder.AddChar(0x7262);                               // Add a specific character
   builder.AddRanges(io.Fonts->GetGlyphRangesJapanese()); // Add one of the default ranges
   builder.BuildRanges(&ranges);                          // Build the final result (ordered ranges with all the unique characters submitted)
   io.Fonts->AddFontFromFileTTF("myfontfile.ttf", size_in_pixels, NULL, ranges.Data);


---------------------------------
 REMAPPING CODEPOINTS
---------------------------------

 All your strings needs to use UTF-8 encoding. Specifying literal in your source code using a local code page (such as CP-923 for Japanese, or CP-1251 for Cyrillic) will NOT work!
 In C++11 you can encode a string literal in UTF-8 by using the u8"hello" syntax. Otherwise you can convert yourself to UTF-8 or load text data from file already saved as UTF-8.
 e.g.
      u8"hello"
      u8"こんにちは"
 You may also try to remap your local codepage characters to their Unicode codepoint using font->AddRemapChar(), but international users may have problems reading/editing your source code.


---------------------------------
 EMBEDDING FONT IN SOURCE CODE
---------------------------------

 Compile and use 'binary_to_compressed_c.cpp' to create a compressed C style array that you can embed in source code.
 See the documentation in binary_to_compressed_c.cpp for instruction on how to use the tool.
 You may find a precompiled version binary_to_compressed_c.exe for Windows instead of demo binaries package (see README).
 The tool optionally used Base85 encoding to reduce the size of _source code_ but the read-only arrays will be about 20% bigger. 

 Then load the font with:
 
   ImFont* font = io.Fonts->AddFontFromMemoryCompressedTTF(compressed_data, compressed_data_size, size_pixels, ...);
   
 Or 
 
   ImFont* font = io.Fonts->AddFontFromMemoryCompressedBase85TTF(compressed_data_base85, size_pixels, ...);


---------------------------------
 FONT FILES INCLUDED IN THIS FOLDER
---------------------------------

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
   recommended loading setting in ImGui: Size = 13.0, DisplayOffset.Y = +1
   http://www.proggyfonts.net/

 ProggyTiny.ttf
   Copyright (c) 2004, 2005 Tristan Grimmer
   MIT License
   recommended loading setting in ImGui: Size = 10.0, DisplayOffset.Y = +1
   http://www.proggyfonts.net/

 Karla-Regular.ttf
   Copyright (c) 2012, Jonathan Pinhorn
   SIL OPEN FONT LICENSE Version 1.1


---------------------------------
 LINKS & OTHER FONTS
---------------------------------

 (Icons) Icon fonts
   https://fortawesome.github.io/Font-Awesome/
   https://github.com/SamBrishes/kenney-icon-font
   https://design.google.com/icons/
   You can use https://github.com/juliettef/IconFontCppHeaders for C/C++ header files with name #define to access icon codepoint in source code.

 (Icons) IcoMoon - Custom Icon font builder
   https://icomoon.io/app

 (Regular) Open Sans Fonts
   https://fonts.google.com/specimen/Open+Sans

 (Regular) Google Noto Fonts (worldwide languages)
   https://www.google.com/get/noto/
 
 (Monospace) Typefaces for source code beautification
   https://github.com/chrissimpkins/codeface

 (Monospace) Programmation fonts
   http://s9w.github.io/font_compare/

 (Monospace) Proggy Programming Fonts
   http://upperbounds.net
   
 (Monospace) Inconsolata
   http://www.levien.com/type/myfonts/inconsolata.html

 (Monospace) Adobe Source Code Pro: Monospaced font family for user interface and coding environments
   https://github.com/adobe-fonts/source-code-pro

 (Monospace) Monospace/Fixed Width Programmer's Fonts
   http://www.lowing.org/fonts/

 (Japanese) M+ fonts by Coji Morishita are free and include most useful Kanjis you would need.
   http://mplus-fonts.sourceforge.jp/mplus-outline-fonts/index-en.html

 Or use Arial Unicode or other Unicode fonts provided with Windows for full characters coverage (not sure of their licensing).
