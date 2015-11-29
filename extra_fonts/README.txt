
 The code in imgui.cpp embeds a copy of 'ProggyClean.ttf' that you can use without any external files.
 Those are only provided as a convenience, you can load your own .TTF files.

---------------------------------
 LOADING INSTRUCTIONS
---------------------------------

 Load default font with:

   ImGuiIO& io = ImGui::GetIO();
   io.Fonts->AddFontDefault();

 Load .TTF file with:

   ImGuiIO& io = ImGui::GetIO();
   io.Fonts->AddFontFromFileTTF("font.ttf", size_pixels);
  
 Detailed options:

   ImFontConfig config;
   config.OversampleH = 3;
   config.OversampleV = 3;
   config.GlyphExtraSpacing.x = 1.0f;
   io.Fonts->AddFontFromFileTTF("font.ttf", size_pixels, &config);

 Combine two fonts into one:

   // Load main font
   io.Fonts->AddFontDefault();

   // Add character ranges and merge into main font
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
 EMBED A FONT IN SOURCE CODE
---------------------------------

 Compile and use 'binary_to_compressed_c.cpp' to create a compressed C style array. Then load the font with:
 
   ImFont* font = io.Fonts->AddFontFromMemoryCompressedTTF(compressed_data, compressed_data_size, size_pixels, ...);
   
 Or 
 
   ImFont* font = io.Fonts->AddFontFromMemoryCompressedBase85TTF(compressed_data_base85, size_pixels, ...);

---------------------------------
 INCLUDED FONT FILES
---------------------------------

 Cousine-Regular.ttf
   Digitized data copyright (c) 2010 Google Corporation.
   Licensed under the SIL Open Font License, Version 1.1   

 DroidSans.ttf
   Copyright (c) Steve Matteson
   Apache License, version 2.0
   http://www.google.com/fonts/specimen/Droid+Sans

 ProggyClean.ttf
   Copyright (c) 2004, 2005 Tristan Grimmer
   MIT License
   recommended loading setting in ImGui: Size = 13.0, DisplayOffset.Y = +1

 ProggyTiny.ttf
   Copyright (c) 2004, 2005 Tristan Grimmer
   MIT License
   recommended loading setting in ImGui: Size = 10.0, DisplayOffset.Y = +1

 Karla-Regular
   Copyright (c) 2012, Jonathan Pinhorn
   SIL OPEN FONT LICENSE Version 1.1

---------------------------------
 LINKS
---------------------------------

 Icon fonts
   https://fortawesome.github.io/Font-Awesome/
   https://github.com/SamBrishes/kenney-icon-font

 Typefaces for source code beautification
   https://github.com/chrissimpkins/codeface
 
 Programmation fonts
   http://s9w.github.io/font_compare/

 Proggy Programming Fonts
   http://upperbounds.net
   
 Inconsolata
   http://www.levien.com/type/myfonts/inconsolata.html

 Adobe Source Code Pro: Monospaced font family for user interface and coding environments
   https://github.com/adobe-fonts/source-code-pro

 Monospace/Fixed Width Programmer's Fonts
   http://www.lowing.org/fonts/

 (Japanese) M+ fonts by Coji Morishita are free and include most useful Kanjis you would need.
   http://mplus-fonts.sourceforge.jp/mplus-outline-fonts/index-en.html

 Or use Arial Unicode or other Unicode fonts provided with Windows for full characters coverage (not sure of their licensing).
