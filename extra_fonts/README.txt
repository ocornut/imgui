
Extra fonts for ImGui.
THOSE FONTS ARE OPTIONAL.

ImGui embeds a copy of 'proggy_clean' that you can use without any external files.
Export your own font with bmfont (www.angelcode.com/products/bmfont).

bmfont reads fonts (.ttf, .fon, etc.) and output a .fnt file and a texture file, e.g:

  proggy_clean.fon --> [bmfont] ---> proggy_clean_13.fnt
                                     proggy_clean_13.png

If you need a free font that supports chinese/japanese characters, you can use the M+ fonts.
TTF and sources are availables at http://mplus-fonts.sourceforge.jp/mplus-outline-fonts.
This directory include some of the M+ fonts converted by bmfont.

//-----------------------------------------------------------------------------

Configure bmfont:

  - Export .fnt as Binary
  - Output .png, 32-bits (or whatever is suitable for your loader/renderer)
  - Tip: uncheck "Render from TrueType outline" and "Font Smoothing" for best result with non-anti-aliased type fonts. 
    But you can experiment with other settings if you want anti-aliased fonts.
  - Tip: use pngout.exe (http://advsys.net/ken/utils.htm) to further reduce the file size of .png files
    All files in this folder have been optimised with pngout.exe

-----------------------------------------------------------------------------

(A) Use font data embedded in ImGui

    // Access embedded font data
    const void* fnt_data;   // pointer to FNT data
    unsigned fnt_size;      // size of FNT data
    const void* png_data;   // pointer to PNG data
    unsigned int png_size;  // size of PNG data
    ImGui::GetDefaultFontData(&fnt_data, &fnt_size, &png_data, &png_size);

  1. Load the .FNT data from 'fnt_data' (NB: this is done for you by default if you don't do anything)

    ImGuiIO& io = ImGui::GetIO();
    io.Font = new ImFont();
    io.Font->LoadFromMemory(fnt_data, fnt_size);
    
  2. Load the .PNG data from 'png_data' into a texture

//-----------------------------------------------------------------------------

(B) Use fonts from external files

  You need to set io.Font->TexUvForWhite to UV coordinates pointing to a white pixel in the texture.
  You can either locate a white pixel manually or use code at runtime to find or write one.
  The OpenGL example include sample code to find a white pixel given an uncompressed 32-bits texture:

  	  // Automatically find white pixel from the texture we just loaded
	  // (io.Font->TexUvForWhite needs to contains UV coordinates pointing to a white pixel in order to render solid objects)
	  for (int tex_data_off = 0; tex_data_off < tex_x*tex_y; tex_data_off++)
	      if (((unsigned int*)tex_data)[tex_data_off] == 0xffffffff)
	      {
	          io.Font->TexUvForWhite = ImVec2((float)(tex_data_off % tex_x)/(tex_x), (float)(tex_data_off / tex_x)/(tex_y));
	          break;
	      }

  1. Load the .FNT data, e.g.

    ImGuiIO& io = ImGui::GetIO();
    
    // proggy_clean_13 [default]
    io.Font = new ImFont();
    io.Font->LoadFromFile("proggy_clean_13.fnt");
    IM_ASSERT(io.Font->IsLoaded());
    io.Font->TexUvForWhite = ImVec2(0.0f/256.0f,0.0f/128);
    io.Font->DisplayOffset = ImVec2(0.0f, +1.0f);

    // proggy_small_12
    io.Font = new ImFont();
    io.Font->LoadFromFile("proggy_small_12.fnt");
    IM_ASSERT(io.Font->IsLoaded());
    io.Font->TexUvForWhite = ImVec2(84.0f/256.0f,20.0f/64);
    io.Font->DisplayOffset = ImVec2(0.0f, +2.0f);
    
    // proggy_small_14
    io.Font = new ImFont();
    io.Font->LoadFromFile("proggy_small_14.fnt");
    IM_ASSERT(io.Font->IsLoaded());
    io.Font->TexUvForWhite = ImVec2(84.0f/256.0f,20.0f/64);
    io.Font->DisplayOffset = ImVec2(0.0f, +3.0f);
    
    // courier_new_16
    io.Font->LoadFromFile("courier_new_16.fnt");
    io.Font->TexUvForWhite = ImVec2(1.0f/256.0f,4.0f/128);
    
    // courier_new_18
    io.Font->LoadFromFile("courier_new_18.fnt");
    io.Font->TexUvForWhite = ImVec2(4.0f/256.0f,5.0f/256);


  2. Load the matching .PNG data into a texture

//-----------------------------------------------------------------------------
