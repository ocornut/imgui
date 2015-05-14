
 The code in imgui.cpp embeds a copy of 'ProggyClean.ttf' that you can use without any external files.
 Those are only provided as a convenience, you can load your own .TTF files.

---------------------------------
 INCLUDED FONTS
---------------------------------

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
 OTHER FONTS
---------------------------------

 For Japanese:
 
   M+ fonts by Coji Morishita are free and include most useful Kanjis you would need.
   mplus-fonts.sourceforge.jp/mplus-outline-fonts/index-en.html
   
 For Japanese, Chinese, Korean:
 
   You can use Arial Unicode or other Unicode fonts provided with Windows (not sure of their license).
   Other suggestions?

---------------------------------
 LOADING INSTRUCTIONS
---------------------------------

 Load .TTF file with:

   ImGuiIO& io = ImGui::GetIO();
   io.Fonts->AddFontFromFileTTF("myfontfile.ttf", size_pixels);
  
 Add a third parameter to bake specific font ranges:

   io.Fonts->LoadFromFileTTF("myfontfile.ttf", size_pixels, io.Fonts->GetGlyphRangesDefault());   // Basic Latin, Extended Latin 
   io.Fonts->LoadFromFileTTF("myfontfile.ttf", size_pixels, io.Fonts->GetGlyphRangesJapanese());  // Default + Hiragana, Katakana, Half-Width, Selection of 1946 Ideographs
   io.Fonts->LoadFromFileTTF("myfontfile.ttf", size_pixels, io.Fonts->GetGlyphRangesChinese());   // Include full set of about 21000 CJK Unified Ideographs

Offset font by altering the io.Font->DisplayOffset value:

   ImFont* font = io.Fonts->AddFontFromFileTTF("myfontfile.ttf", size_pixels);
   font->DisplayOffset.y += 1;   // Render 1 pixel down

