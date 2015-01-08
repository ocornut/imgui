
---------------------------------
 EXTRA FONTS FOR IMGUI
---------------------------------

ImGui embeds a copy of 'ProggyClean.ttf' that you can use without any external files.

Load .TTF file with:

  ImGuiIO& io = ImGui::GetIO();
  io.Font = new ImFont();
  io.Font->LoadFromFileTTF("myfontfile.ttf", size_pixels);
  
Add a third parameter to bake specific font ranges:

  io.Font->LoadFromFileTTF("myfontfile.ttf", size_pixels, ImFont::GetGlyphRangesDefault());		// Basic Latin, Extended Latin 
  io.Font->LoadFromFileTTF("myfontfile.ttf", size_pixels, ImFont::GetGlyphRangesJapanese());		// Default + Hiragana, Katakana, Half-Width, Selection of 1946 Ideographs
  io.Font->LoadFromFileTTF("myfontfile.ttf", size_pixels, ImFont::GetGlyphRangesChinese());		// Japanese + full set of about 21000 CJK Unified Ideographs

Offset font by altering the io.Font->DisplayOffset value:

  io.Font->DisplayOffset.y += 1;   // Render 1 pixel down

-----------------------------------
 RECOMMENDED SIZES
-----------------------------------

	ProggyTiny.ttf			Size: 10.0f			Offset: Y: +1
	ProggyClean.ttf			Size: 13.0f			Offset: Y: +1
	
