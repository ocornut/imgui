
---------------------------------
 EXTRA FONTS FOR IMGUI
---------------------------------

 ProggyClean.ttf
 Copyright (c) 2004, 2005 Tristan Grimmer
 MIT License

 ProggyTiny.ttf
 Copyright (c) 2004, 2005 Tristan Grimmer
 MIT License

 Karla-Regular
 Copyright (c) 2012, Jonathan Pinhorn
 SIL OPEN FONT LICENSE Version 1.1

imgui.cpp embeds a copy of 'ProggyClean.ttf' that you can use without any external files.

Load .TTF file with:

  ImGuiIO& io = ImGui::GetIO();
  io.Fonts->AddFontFromFileTTF("myfontfile.ttf", size_pixels);
  
Add a third parameter to bake specific font ranges:

  io.Fonts->LoadFromFileTTF("myfontfile.ttf", size_pixels, io.Fonts->GetGlyphRangesDefault());      // Basic Latin, Extended Latin 
  io.Fonts->LoadFromFileTTF("myfontfile.ttf", size_pixels, io.Fonts->GetGlyphRangesJapanese());     // Default + Hiragana, Katakana, Half-Width, Selection of 1946 Ideographs
  io.Fonts->LoadFromFileTTF("myfontfile.ttf", size_pixels, io.Fonts->GetGlyphRangesChinese());      // Include full set of about 21000 CJK Unified Ideographs

Offset font by altering the io.Font->DisplayOffset value:

  ImFont* font = io.Fonts->AddFontFromFileTTF("myfontfile.ttf", size_pixels);
  font->DisplayOffset.y += 1;   // Render 1 pixel down

-----------------------------------
 RECOMMENDED SIZES
-----------------------------------

	ProggyTiny.ttf			Size: 10.0f			Offset: Y: +1
	ProggyClean.ttf			Size: 13.0f			Offset: Y: +1
	
