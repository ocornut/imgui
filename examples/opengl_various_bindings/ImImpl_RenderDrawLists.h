#ifndef IMIMPL_RENDERDRAWLISTS_H
#define IMIMPL_RENDERDRAWLISTS_H

#include <string.h>
#include <stdio.h>

extern void InitGL();
extern void ResizeGL(int w,int h);
extern void DrawGL();
extern void DestroyGL();

// These variables can be declared extern and set at runtime-----------------------------------------------------
bool gImGuiPaused = false;
float gImGuiInverseFPSClamp = -1.0f;    // CAN'T BE 0. < 0 = No clamping.
// --------------------------------------------------------------------------------------------------------------

struct ImImpl_InitParams	{
	ImVec2 gWindowSize;
	char gWindowTitle[1024];
	char gOptionalFntFilePath[2048];
	char gOptionalImageFilePath[2048];
	ImVec2 gOptionalTexCoordUVForWhite;
    float gFpsClamp;	// <0 -> no clamp
    const unsigned char* pOptionalReferenceToFntFileInMemory;
    size_t      pOptionalSizeOfFntFileInMemory;
    const unsigned char* pOptionalReferenceToImageFileInMemory;
    size_t      pOptionalSizeOfImageFileInMemory;
    ImImpl_InitParams(
            int windowWidth=1270,
            int windowHeight=720,
            const char* windowTitle=NULL,
            const char* optionalFntFilePath=NULL,
            const char* optionalImageFilePath=NULL,
            float optionalTexCoordUForWhite=-1.f,
            float optionalTexCoordVForWhite=-1.f
            ,const unsigned char*    _pOptionalReferenceToFntFileInMemory=NULL,
            size_t                  _pOptionalSizeOfFntFileInMemory=0,
            const unsigned char*    _pOptionalReferenceToImageFileInMemory=NULL,
            size_t                  _pOptionalSizeOfImageFileInMemory=0
    ) : gFpsClamp(-1.0f)
    ,pOptionalReferenceToFntFileInMemory(_pOptionalReferenceToFntFileInMemory),
    pOptionalSizeOfFntFileInMemory(_pOptionalSizeOfFntFileInMemory),
    pOptionalReferenceToImageFileInMemory(_pOptionalReferenceToImageFileInMemory),
    pOptionalSizeOfImageFileInMemory(_pOptionalSizeOfImageFileInMemory)
	{
        gWindowSize.x = windowWidth<=0?1270:windowWidth;gWindowSize.y = windowHeight<=0?720:windowHeight;
        gOptionalTexCoordUVForWhite.x = optionalTexCoordUForWhite;
        gOptionalTexCoordUVForWhite.y = optionalTexCoordVForWhite;

		gWindowTitle[0]='\0';
		if (windowTitle)	{
			const size_t len = strlen(windowTitle);
			if (len<1023) strcat(gWindowTitle,windowTitle);
			else		  {
				memcpy(gWindowTitle,windowTitle,1023);
				gWindowTitle[1023]='\0';
			}
		}
		else strcat(gWindowTitle,"ImGui OpenGL Example");

		gOptionalFntFilePath[0]='\0';
		if (optionalFntFilePath)	{
			const size_t len = strlen(optionalFntFilePath);
			if (len<2047) strcat(gOptionalFntFilePath,optionalFntFilePath);
        }

		gOptionalImageFilePath[0]='\0';
		if (optionalImageFilePath)	{
			const size_t len = strlen(optionalImageFilePath);
			if (len<2047) strcat(gOptionalImageFilePath,optionalImageFilePath);
        }

	}

};

struct ImImpl_PrivateParams  {
#ifndef IMIMPL_SHADER_NONE

#ifndef IMIMPL_NUM_ROUND_ROBIN_VERTEX_BUFFERS
#define IMIMPL_NUM_ROUND_ROBIN_VERTEX_BUFFERS 1
#elif (IMIMPL_NUM_ROUND_ROBIN_VERTEX_BUFFERS<=0)
#undef IMIMPL_NUM_ROUND_ROBIN_VERTEX_BUFFERS
#define IMIMPL_NUM_ROUND_ROBIN_VERTEX_BUFFERS 1
#endif //IMIMPL_NUM_ROUND_ROBIN_VERTEX_BUFFERS

    GLuint vertexBuffers[IMIMPL_NUM_ROUND_ROBIN_VERTEX_BUFFERS];
    GLuint program;
    // gProgram uniform locations:
    GLint uniLocOrthoMatrix;
    GLint uniLocTexture;
    // gProgram attribute locations:
    GLint attrLocPosition;
    GLint attrLocUV;
    GLint attrLocColour;
    // font texture
    GLuint fontTex;
    ImImpl_PrivateParams() :program(0),uniLocOrthoMatrix(-1),uniLocTexture(-1),
        attrLocPosition(-1),attrLocUV(-1),attrLocColour(-1),fontTex(0)
    {for (int i=0;i<IMIMPL_NUM_ROUND_ROBIN_VERTEX_BUFFERS;i++) vertexBuffers[i]=0;}
#else //IMIMPL_SHADER_NONE
    // font texture
    GLuint fontTex;
    ImImpl_PrivateParams() :fontTex(0) {}
#endif //IMIMPL_SHADER_NONE
};
static ImImpl_PrivateParams gImImplPrivateParams;


#ifndef IMIMPL_SHADER_NONE
static GLuint CompileShaders(const GLchar** vertexShaderSource, const GLchar** fragmentShaderSource )
{
    //Compile vertex shader
    GLuint vertexShader( glCreateShader( GL_VERTEX_SHADER ) );
    glShaderSource( vertexShader, 1, vertexShaderSource, NULL );
    glCompileShader( vertexShader );


    // check
    GLint bShaderCompiled;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &bShaderCompiled);

    if (!bShaderCompiled)        {
        int i32InfoLogLength, i32CharsWritten;
        glGetShaderiv(vertexShader, GL_INFO_LOG_LENGTH, &i32InfoLogLength);

        char* pszInfoLog = new char[i32InfoLogLength];
        glGetShaderInfoLog(vertexShader, i32InfoLogLength, &i32CharsWritten, pszInfoLog);
        printf("********VertexShader %s\n", pszInfoLog);

        delete[] pszInfoLog;
    }

    //Compile fragment shader
    GLuint fragmentShader( glCreateShader( GL_FRAGMENT_SHADER ) );
    glShaderSource( fragmentShader, 1, fragmentShaderSource, NULL );
    glCompileShader( fragmentShader );

    //check
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &bShaderCompiled);

    if (!bShaderCompiled)        {
        int i32InfoLogLength, i32CharsWritten;
        glGetShaderiv(fragmentShader, GL_INFO_LOG_LENGTH, &i32InfoLogLength);

        char* pszInfoLog = new char[i32InfoLogLength];
        glGetShaderInfoLog(fragmentShader, i32InfoLogLength, &i32CharsWritten, pszInfoLog);
        printf("********FragmentShader %s\n", pszInfoLog);

        delete[] pszInfoLog;
    }

    //Link vertex and fragment shader together
    GLuint program( glCreateProgram() );
    glAttachShader( program, vertexShader );
    glAttachShader( program, fragmentShader );
    glLinkProgram( program );

    //check
    GLint bLinked;
    glGetProgramiv(program, GL_LINK_STATUS, &bLinked);
    if (!bLinked)        {
        int i32InfoLogLength, i32CharsWritten;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &i32InfoLogLength);

        char* pszInfoLog = new char[i32InfoLogLength];
        glGetProgramInfoLog(program, i32InfoLogLength, &i32CharsWritten, pszInfoLog);
        printf("%s",pszInfoLog);

        delete[] pszInfoLog;
    }

    //Delete shaders objects
    glDeleteShader( vertexShader );
    glDeleteShader( fragmentShader );

    return program;
}
#endif //IMIMPL_SHADER_NONE

static void DestroyImGuiFontTexture()	{
    if (gImImplPrivateParams.fontTex)	{
        glDeleteTextures( 1, &gImImplPrivateParams.fontTex );
        gImImplPrivateParams.fontTex = 0;
	}
}
#ifndef STBI_INCLUDE_STB_IMAGE_H
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif //STBI_INCLUDE_STB_IMAGE_H
static void InitImGuiFontTexture(const ImImpl_InitParams* pOptionalInitParams=NULL) {
    const char* optionalFntFilePath=NULL;
    const char* optionalImageFilePath=NULL;
    float optionalTexCoordUForWhite=-1;
    float optionalTexCoordVForWhite=-1;
    if (pOptionalInitParams)    {
        if (pOptionalInitParams->gOptionalFntFilePath[0]!='\0') optionalFntFilePath = (const char*) &pOptionalInitParams->gOptionalFntFilePath[0];
        if (pOptionalInitParams->gOptionalImageFilePath[0]!='\0') optionalImageFilePath = (const char*) &pOptionalInitParams->gOptionalImageFilePath[0];
        optionalTexCoordUForWhite = pOptionalInitParams->gOptionalTexCoordUVForWhite.x;
        optionalTexCoordVForWhite = pOptionalInitParams->gOptionalTexCoordUVForWhite.y;
    }

    ImGuiIO& io = ImGui::GetIO();

    DestroyImGuiFontTexture();	// reentrant
    // Load font texture
    glGenTextures(1, &gImImplPrivateParams.fontTex);
    glBindTexture(GL_TEXTURE_2D, gImImplPrivateParams.fontTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	void* tex_data = NULL;
    int tex_x=0, tex_y=0;
	bool fontOk = false;
	if (optionalFntFilePath && optionalImageFilePath)	{
	   // Custom font from filesystem
    	io.Font = new ImBitmapFont();
	    io.Font->LoadFromFile(optionalFntFilePath);
    	if (io.Font->IsLoaded())	{
            int tex_comp;
            tex_data = stbi_load(optionalImageFilePath, &tex_x, &tex_y, &tex_comp, 4);
		    if(tex_data != NULL)	{
		    	fontOk = true;
                if (optionalTexCoordUForWhite<0 || optionalTexCoordVForWhite<0)	{
				    // Automatically find white pixel from the texture we just loaded
				    // (io.FontTexUvForWhite needs to contains UV coordinates pointing to a white pixel in order to render solid objects)
				    bool found = false;
				    for (int tex_data_off = 0; tex_data_off < tex_x*tex_y; tex_data_off++)
			        if (((unsigned int*)tex_data)[tex_data_off] == 0xffffffff)	{
		            	io.FontTexUvForWhite = ImVec2((float)(tex_data_off % tex_x)/(tex_x), (float)(tex_data_off / tex_x)/(tex_y));
    		        	found = true;
    		        	break;
    		    	}
    				if (!found) {
    					fprintf(stderr,"Error: white pixel not found in font image: \"%s\"\n",optionalImageFilePath);
    					fontOk = false;
    				}
                    else printf("InitImGuiFontTexture(...) Tip: for faster loading \"%s\" please use: optionalTexCoordUForWhite = %1.10f, optionalTexCoordVForWhite = %1.10f\n",optionalImageFilePath,io.FontTexUvForWhite.x,io.FontTexUvForWhite.y);
				}
				else {io.FontTexUvForWhite.x = optionalTexCoordUForWhite;io.FontTexUvForWhite.y = optionalTexCoordVForWhite;}
			}
		}
	}
    // IMIMPL_ALLOW_CLIENT_EMBEDDED_FONT
    if (!fontOk && pOptionalInitParams)  {
        const unsigned char* pFnt = pOptionalInitParams->pOptionalReferenceToFntFileInMemory;
        const size_t fntSize = pOptionalInitParams->pOptionalSizeOfFntFileInMemory;
        const unsigned char* pImg = pOptionalInitParams->pOptionalReferenceToImageFileInMemory;
        const size_t imgSize = pOptionalInitParams->pOptionalSizeOfImageFileInMemory;
        if (pFnt && fntSize>0 && pImg && imgSize>0) {
            if (io.Font)    {delete io.Font; io.Font = NULL;}
            if (tex_data) {stbi_image_free(tex_data);tex_data=0;}
            // Load custom font (embedded in code)
            io.Font = new ImBitmapFont();
            io.Font->LoadFromMemory(pFnt, fntSize);
            io.FontYOffset = +1;
            if (io.Font->IsLoaded())	{
                int tex_comp;
                tex_data =  stbi_load_from_memory(pImg, (int)imgSize, &tex_x, &tex_y, &tex_comp, 4);
                if(tex_data != NULL)	{
                    fontOk = true;
                    if (optionalTexCoordUForWhite<0 || optionalTexCoordVForWhite<0)	{
                        // Automatically find white pixel from the texture we just loaded
                        // (io.FontTexUvForWhite needs to contains UV coordinates pointing to a white pixel in order to render solid objects)
                        bool found = false;
                        for (int tex_data_off = 0; tex_data_off < tex_x*tex_y; tex_data_off++)
                            if (((unsigned int*)tex_data)[tex_data_off] == 0xffffffff)	{
                                io.FontTexUvForWhite = ImVec2((float)(tex_data_off % tex_x)/(tex_x), (float)(tex_data_off / tex_x)/(tex_y));
                                found = true;
                                break;
                            }
                        if (!found) {
                            fprintf(stderr,"Error: white pixel not found in embedded font image.\n");
                            fontOk = false;
                        }
                        else printf("InitImGuiFontTexture(...) Tip: for faster loading the embedded font image please use: optionalTexCoordUForWhite = %1.10f, optionalTexCoordVForWhite = %1.10f\n",io.FontTexUvForWhite.x,io.FontTexUvForWhite.y);
                    }
                    else {io.FontTexUvForWhite.x = optionalTexCoordUForWhite;io.FontTexUvForWhite.y = optionalTexCoordVForWhite;}
                }
            }
        }
    }
    // end IMIMPL_ALLOW_CLIENT_EMBEDDED_FONT
	if (!fontOk)	{
        if (io.Font)    {delete io.Font; io.Font = NULL;}
		if (tex_data) {stbi_image_free(tex_data);tex_data=0;}

	    // Default font (embedded in code)
    	const void* png_data;
    	unsigned int png_size;
    	ImGui::GetDefaultFontData(NULL, NULL, &png_data, &png_size);
        int tex_comp;
        tex_data = stbi_load_from_memory((const unsigned char*)png_data, (int)png_size, &tex_x, &tex_y, &tex_comp, 4);
    	fontOk = (tex_data != NULL);
    }

	if (fontOk)	{
	    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex_x, tex_y, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex_data);
    	stbi_image_free(tex_data);tex_data=0;
    }
    else {
        if (io.Font)    {delete io.Font; io.Font = NULL;}
        if (tex_data) {stbi_image_free(tex_data);tex_data=0;}
    }
}


static void InitImGuiProgram()  {
#ifndef IMIMPL_SHADER_NONE
// -----------------------------------------------------------------------
// START SHADER CODE
//------------------------------------------------------------------------
// shaders
#ifdef IMIMPL_SHADER_GL3
static const GLchar* gVertexShaderSource[] = {
#ifdef IMIMPL_SHADER_GLES
      "#version 300 es\n"
#else  //IMIMPL_SHADER_GLES
      "#version 330\n"
#endif //IMIMPL_SHADER_GLES
      "precision highp float;\n"
      "uniform mat4 ortho;\n"
      "layout (location = 0 ) in vec2 Position;\n"
      "layout (location = 1 ) in vec2 UV;\n"
      "layout (location = 2 ) in vec4 Colour;\n"
      "out vec2 Frag_UV;\n"
      "out vec4 Frag_Colour;\n"
      "void main()\n"
      "{\n"
      " Frag_UV = UV;\n"
      " Frag_Colour = Colour;\n"
      "\n"
      " gl_Position = ortho*vec4(Position.xy,0,1);\n"
      "}\n"
    };

static const GLchar* gFragmentShaderSource[] = {
#ifdef IMIMPL_SHADER_GLES
      "#version 300 es\n"
#else //IMIMPL_SHADER_GLES
      "#version 330\n"
#endif //IMIMPL_SHADER_GLES
      "precision mediump float;\n"
      "uniform lowp sampler2D Texture;\n"
      "in vec2 Frag_UV;\n"
      "in vec4 Frag_Colour;\n"
      "out vec4 FragColor;\n"
      "void main()\n"
      "{\n"
      " FragColor = Frag_Colour * texture( Texture, Frag_UV.st);\n"
      "}\n"
    };
#else //NO IMIMPL_SHADER_GL3
static const GLchar* gVertexShaderSource[] = {
#ifdef IMIMPL_SHADER_GLES
      "#version 100\n"
      "precision highp float;\n"
#endif //IMIMPL_SHADER_GLES
      "uniform mat4 ortho;\n"
      "attribute vec2 Position;\n"
      "attribute vec2 UV;\n"
      "attribute vec4 Colour;\n"
      "varying vec2 Frag_UV;\n"
      "varying vec4 Frag_Colour;\n"
      "void main()\n"
      "{\n"
      " Frag_UV = UV;\n"
      " Frag_Colour = Colour;\n"
      "\n"
      " gl_Position = ortho*vec4(Position.xy,0,1);\n"
      "}\n"
    };

static const GLchar* gFragmentShaderSource[] = {
#ifdef IMIMPL_SHADER_GLES
      "#version 100\n"
      "precision mediump float;\n"
      "uniform lowp sampler2D Texture;\n"
#else //IMIMPL_SHADER_GLES
      "uniform sampler2D Texture;\n"
#endif //IMIMPL_SHADER_GLES
      "varying vec2 Frag_UV;\n"
      "varying vec4 Frag_Colour;\n"
      "void main()\n"
      "{\n"
      " gl_FragColor = Frag_Colour * texture2D( Texture, Frag_UV.st);\n"
      "}\n"
    };
#endif //IMIMPL_SHADER_GL3
//------------------------------------------------------------------------
// END SHADER CODE
//------------------------------------------------------------------------

    if (gImImplPrivateParams.program==0)    {
        gImImplPrivateParams.program = CompileShaders(gVertexShaderSource, gFragmentShaderSource );
        if (gImImplPrivateParams.program==0) {
            fprintf(stderr,"Error compiling shaders.\n");
            return;
        }
        //Get Uniform locations
        gImImplPrivateParams.uniLocTexture = glGetUniformLocation(gImImplPrivateParams.program,"Texture");
        gImImplPrivateParams.uniLocOrthoMatrix = glGetUniformLocation(gImImplPrivateParams.program,"ortho");

        //Get Attribute locations
        gImImplPrivateParams.attrLocPosition  = glGetAttribLocation(gImImplPrivateParams.program,"Position");
        gImImplPrivateParams.attrLocUV  = glGetAttribLocation(gImImplPrivateParams.program,"UV");
        gImImplPrivateParams.attrLocColour  = glGetAttribLocation(gImImplPrivateParams.program,"Colour");

        // Debug
        /*
        printf("gUniLocTexture = %d\n",gImImplPrivateParams.gUniLocTexture);
        printf("gUniLocLayers = %d\n",gImImplPrivateParams.gUniLocOrthoMatrix);
        printf("gAttrLocPosition = %d\n",gImImplPrivateParams.gAttrLocPosition);
        printf("gAttrLocUV = %d\n",gImImplPrivateParams.gAttrLocUV);
        printf("gAttrLocColour = %d\n",gImImplPrivateParams.gAttrLocColour);
        */
	}
#endif //IMIMPL_SHADER_NONE
}

static void DestroyImGuiProgram()	{
#ifndef IMIMPL_SHADER_NONE
    if (gImImplPrivateParams.program)	{
        glDeleteProgram(gImImplPrivateParams.program);
        gImImplPrivateParams.program = 0;
	}
#endif //IMIMPL_SHADER_NONE
}
static void InitImGuiBuffer()	{
#ifndef IMIMPL_SHADER_NONE
    if (gImImplPrivateParams.vertexBuffers[0]==0) glGenBuffers(IMIMPL_NUM_ROUND_ROBIN_VERTEX_BUFFERS, &gImImplPrivateParams.vertexBuffers[0]);
#endif //IMIMPL_SHADER_NONE
}
static void DestroyImGuiBuffer()	{
#ifndef IMIMPL_SHADER_NONE
    if (gImImplPrivateParams.vertexBuffers[0]) {
        glDeleteBuffers( IMIMPL_NUM_ROUND_ROBIN_VERTEX_BUFFERS, &gImImplPrivateParams.vertexBuffers[0] );
        gImImplPrivateParams.vertexBuffers[0] = 0;
	}
#endif //IMIMPL_SHADER_NONE
}



static void ImImpl_RenderDrawLists(ImDrawList** const cmd_lists, int cmd_lists_count)
{
    // ----------------------------------------------------------------------------------------------------------------
    // Warning: we SHOULD store the gl state here and restore it later; we don't do it: we just return a gl state with:
    // GL_BLEND,GL_SCISSOR_TEST disabled
    // GL_DEPTH_TEST            enabled
    // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // glCullFace(GL_BACK);
    // glActiveTexture(GL_TEXTURE0);    with bound texture our font texture
    // glEnable(GL_TEXTURE_2D); // Only in case IMIMPL_SHADER_NONE is defined

    // These settings should not harm most applications
    // -----------------------------------------------------------------------------------------------------------------

#ifndef IMIMPL_SHADER_NONE
    size_t total_vtx_count = 0;
    for (int n = 0; n < cmd_lists_count; n++)   total_vtx_count += cmd_lists[n]->vtx_buffer.size();
    if (total_vtx_count == 0)   return;

    static int bufferNum = 0;
#if IMIMPL_NUM_ROUND_ROBIN_VERTEX_BUFFERS > 1
    if (++bufferNum == IMIMPL_NUM_ROUND_ROBIN_VERTEX_BUFFERS) bufferNum = 0;
    //fprintf(stderr,"Using buffer: %d\n",bufferNum);
#endif //IMIMPL_NUM_ROUND_ROBIN_VERTEX_BUFFERS
    glBindBuffer(GL_ARRAY_BUFFER, gImImplPrivateParams.vertexBuffers[bufferNum]);
    glBufferData(GL_ARRAY_BUFFER, total_vtx_count * sizeof(ImDrawVert), NULL, GL_STREAM_DRAW);

    GLintptr dataOffset = 0;
    for (int n = 0; n < cmd_lists_count; n++)   {
        const ImDrawList* cmd_list = cmd_lists[n];
        glBufferSubData(GL_ARRAY_BUFFER,dataOffset,cmd_list->vtx_buffer.size() * sizeof(ImDrawVert),&cmd_list->vtx_buffer[0]);
        dataOffset += cmd_list->vtx_buffer.size() * sizeof(ImDrawVert);
    }

    // Setup render state: alpha-blending enabled, no face culling (or GL_FRONT face culling), no depth testing, scissor enabled
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glCullFace(GL_FRONT);       // with this I can leave GL_CULL_FACE as it is
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    // Setup texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gImImplPrivateParams.fontTex);
    // Setup program and uniforms
    glUseProgram(gImImplPrivateParams.program);
    glUniform1i(gImImplPrivateParams.uniLocTexture, 0);
    // Setup orthographic projection matrix
    const float width = ImGui::GetIO().DisplaySize.x;
    const float height = ImGui::GetIO().DisplaySize.y;
    const float ortho[4][4] = {
        { 2.0f/width, 0.0f, 0.0f, 0.0f },
        { 0.0f, 2.0f/-height, 0.0f, 0.0f },
        { 0.0f, 0.0f, -1.0f, 0.0f },
        { -1.0f, 1.0f, 0.0f, 1.0f },
    };
    glUniformMatrix4fv(gImImplPrivateParams.uniLocOrthoMatrix, 1, GL_FALSE, &ortho[0][0]);

    glEnableVertexAttribArray(gImImplPrivateParams.attrLocPosition);
    glEnableVertexAttribArray(gImImplPrivateParams.attrLocUV);
    glEnableVertexAttribArray(gImImplPrivateParams.attrLocColour);

    glVertexAttribPointer(gImImplPrivateParams.attrLocPosition, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (const void*)(0));
    glVertexAttribPointer(gImImplPrivateParams.attrLocUV, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (const void*)(0 + 8));
    glVertexAttribPointer(gImImplPrivateParams.attrLocColour, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (const void*)(0 + 16));

    int vtx_offset = 0;
    for (int n = 0; n < cmd_lists_count; n++)
    {
        const ImDrawList* cmd_list = cmd_lists[n];
        const ImDrawCmd* pcmd_end = cmd_list->commands.end();
        for (const ImDrawCmd* pcmd = cmd_list->commands.begin(); pcmd != pcmd_end; pcmd++)  {
            glScissor((int)pcmd->clip_rect.x, (int)(height - pcmd->clip_rect.w), (int)(pcmd->clip_rect.z - pcmd->clip_rect.x), (int)(pcmd->clip_rect.w - pcmd->clip_rect.y));
            glDrawArrays(GL_TRIANGLES, vtx_offset, pcmd->vtx_count);
            vtx_offset += pcmd->vtx_count;
        }
    }
    glDisableVertexAttribArray(gImImplPrivateParams.attrLocPosition);
    glDisableVertexAttribArray(gImImplPrivateParams.attrLocUV);
    glDisableVertexAttribArray(gImImplPrivateParams.attrLocColour);
    glUseProgram(0);

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);
    glCullFace(GL_BACK);
    glDisable(GL_BLEND);
#else //IMIMPL_SHADER_NONE
    if (cmd_lists_count == 0)
        return;

    // We are using the OpenGL fixed pipeline to make the example code simpler to read!
    // A probable faster way to render would be to collate all vertices from all cmd_lists into a single vertex buffer.
    // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled, vertex/texcoord/color pointers.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glCullFace(GL_FRONT);       // with this I can leave GL_CULL_FACE as it is
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);

    // Setup texture
    glBindTexture(GL_TEXTURE_2D, gImImplPrivateParams.fontTex);
    glEnable(GL_TEXTURE_2D);

    // Setup orthographic projection matrix
    const float width = ImGui::GetIO().DisplaySize.x;
    const float height = ImGui::GetIO().DisplaySize.y;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0f, width, height, 0.0f, -1.0f, +1.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Render command lists
    for (int n = 0; n < cmd_lists_count; n++)
    {
        const ImDrawList* cmd_list = cmd_lists[n];
        const unsigned char* vtx_buffer = (const unsigned char*)cmd_list->vtx_buffer.begin();
        glVertexPointer(2, GL_FLOAT, sizeof(ImDrawVert), (void*)(vtx_buffer));
        glTexCoordPointer(2, GL_FLOAT, sizeof(ImDrawVert), (void*)(vtx_buffer+8));
        glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(ImDrawVert), (void*)(vtx_buffer+16));

        int vtx_offset = 0;
        const ImDrawCmd* pcmd_end = cmd_list->commands.end();
        for (const ImDrawCmd* pcmd = cmd_list->commands.begin(); pcmd != pcmd_end; pcmd++)
        {
            glScissor((int)pcmd->clip_rect.x, (int)(height - pcmd->clip_rect.w), (int)(pcmd->clip_rect.z - pcmd->clip_rect.x), (int)(pcmd->clip_rect.w - pcmd->clip_rect.y));
            glDrawArrays(GL_TRIANGLES, vtx_offset, pcmd->vtx_count);
            vtx_offset += pcmd->vtx_count;
        }
    }
    glDisable(GL_SCISSOR_TEST);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);
    glCullFace(GL_BACK);
    glDisable(GL_BLEND);
#endif //IMIMPL_SHADER_NONE
}

#ifndef _WIN32
#include <unistd.h>
#else //_WIN32
// Is there a header with ::Sleep(...) ?
#endif //_WIN32
static void WaitFor(unsigned int ms)    {
#ifdef _WIN32
  if (ms > 0) Sleep(ms);
#else
  // delta in microseconds
  useconds_t delta = (useconds_t) ms * 1000;
  // On some systems, the usleep argument must be < 1000000
  while (delta > 999999L)   {
    usleep(999999);
    delta -= 999999L;
  }
  if (delta > 0L) usleep(delta);
#endif
}



#endif //IMIMPL_RENDERDRAWLISTS_H




