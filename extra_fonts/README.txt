
Extra fonts for ImGui.
THOSE FONTS ARE OPTIONAL.

ImGui embeds a copy of 'proggy_clean' that you can use without any external files.
Export your own font with bmfont (www.angelcode.com/products/bmfont).

bmfont reads fonts (.ttf, .fon, etc.) and output a .fnt file and a texture file, e.g:

  proggy_clean.fon --> [bmfont] ---> proggy_clean_13.fnt
                                     proggy_clean_13.png

Configure bmfont:

  - Export .fnt as Binary
  - Tip: uncheck "Render from TrueType outline" and "Font Smoothing" for best result with non-anti-aliased type fonts. 
    But you can experiment with other settings if you want anti-aliased fonts.


(A) Use font data embedded in ImGui

    // Access embedded font data
    const void* fnt_data;   // pointer to FNT data
    unsigned fnt_size;      // size of FNT data
    const void* png_data;   // pointer to PNG data
    unsigned int png_size;  // size of PNG data
    ImGui::GetDefaultFontData(&fnt_data, &fnt_size, &png_data, &png_size);

  1. Load the .FNT data from 'fnt_data' (NB: this is done for you by default if you don't do anything)

    ImGuiIO& io = ImGui::GetIO();
    io.Font = new ImBitmapFont();
    io.Font->LoadFromMemory(fnt_data, fnt_size);
    
  2. Load the .PNG data from 'png_data' into a texture



(B) Use fonts from external files

    ImGuiIO& io = ImGui::GetIO();

  1. Load the .FNT data, e.g.
    
    // proggy_clean_13 [default]
    io.Font->LoadFromFile("proggy_clean_13.fnt");
    io.FontTexUvForWhite = ImVec2(0.0f/256.0f,0.0f/128);
    io.FontYOffset = +1;

    // proggy_small_12
    io.Font = new ImBitmapFont();
    io.Font->LoadFromFile("proggy_small_12.fnt");
    io.FontTexUvForWhite = ImVec2(84.0f/256.0f,20.0f/64);
    io.FontYOffset = +2;
    
    // proggy_small_14
    io.Font = new ImBitmapFont();
    io.Font->LoadFromFile("proggy_small_14.fnt");
    io.FontTexUvForWhite = ImVec2(84.0f/256.0f,20.0f/64);
    io.FontYOffset = +3;
    
    // courier_new_16
    io.Font->LoadFromFile("courier_new_16.fnt");
    io.FontTexUvForWhite = ImVec2(1.0f/256.0f,4.0f/128);
    
    // courier_new_18
    io.Font->LoadFromFile("courier_new_18.fnt");
    io.FontTexUvForWhite = ImVec2(4.0f/256.0f,5.0f/256);

  2. Load the matching .PNG data into a texture

