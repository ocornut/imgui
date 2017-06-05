// USAGE:
/*
1) Compile this cpp file
2) In your main.cpp just add (without including any additional file: there's no header!):
extern bool ImGuiSaveStyle(const char* filename,const ImGuiStyle& style);
extern bool ImGuiLoadStyle(const char* filename,ImGuiStyle& style);
3) Use them together with ImGui::GetStyle() to save/load the current style.
   ImGui::GetStyle() returns a reference of the current style that can be set/get.

Please note that other style options are not globally serializable because they are "window flags",
that must be set on a per-window basis (for example border,titlebar,scrollbar,resizable,movable,per-window alpha).

To edit and save a style, you can use the default ImGui example and append to the "Debug" window the following code:
            ImGui::Text("\n");
            ImGui::Text("Please modify the current style in:");
            ImGui::Text("ImGui Test->Window Options->Style Editor");
            static bool loadCurrentStyle = false;
            static bool saveCurrentStyle = false;
            static bool resetCurrentStyle = false;
            loadCurrentStyle = ImGui::Button("Load Saved Style");
            saveCurrentStyle = ImGui::Button("Save Current Style");
            resetCurrentStyle = ImGui::Button("Reset Current Style");
            if (loadCurrentStyle)   {
                if (!ImGuiLoadStyle("./myimgui.style",ImGui::GetStyle()))   {
                    fprintf(stderr,"Warning: \"./myimgui.style\" not present.\n");
                }
            }
            if (saveCurrentStyle)   {
                if (!ImGuiSaveStyle("./myimgui.style",ImGui::GetStyle()))   {
                    fprintf(stderr,"Warning: \"./myimgui.style\" cannot be saved.\n");
                }
            }
            if (resetCurrentStyle)  ImGui::GetStyle() = ImGuiStyle();
*/

#include <imgui.h>

// From <imgui.cpp>:--------------------------------------------------------
#include <stdio.h>      // vsnprintf
#define IM_ARRAYSIZE(_ARR)          ((int)(sizeof(_ARR)/sizeof(*_ARR)))
static size_t ImFormatString(char* buf, size_t buf_size, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int w = vsnprintf(buf, buf_size, fmt, args);
    va_end(args);
    buf[buf_size-1] = 0;
    return (w == -1) ? buf_size : (size_t)w;
}
//---------------------------------------------------------------------------

#include <string.h>



bool ImGuiSaveStyle(const char* filename,const ImGuiStyle& style)
{
    // Write .style file
    FILE* f = fopen(filename, "wt");
    if (!f)  return false;
 
    fprintf(f, "[Alpha]\n%1.3f\n", style.Alpha);
    fprintf(f, "[WindowPadding]\n%1.3f %1.3f\n", style.WindowPadding.x,style.WindowPadding.y);
    fprintf(f, "[WindowMinSize]\n%1.3f %1.3f\n", style.WindowMinSize.x,style.WindowMinSize.y);
    fprintf(f, "[FramePadding]\n%1.3f %1.3f\n", style.FramePadding.x,style.FramePadding.y);
    fprintf(f, "[ItemSpacing]\n%1.3f %1.3f\n", style.ItemSpacing.x,style.ItemSpacing.y);
    fprintf(f, "[ItemInnerSpacing]\n%1.3f %1.3f\n", style.ItemInnerSpacing.x,style.ItemInnerSpacing.y);
    fprintf(f, "[TouchExtraPadding]\n%1.3f %1.3f\n", style.TouchExtraPadding.x,style.TouchExtraPadding.y);
    fprintf(f, "[AutoFitPadding]\n%1.3f %1.3f\n", style.AutoFitPadding.x,style.AutoFitPadding.y);
    fprintf(f, "[WindowFillAlphaDefault]\n%1.3f\n", style.WindowFillAlphaDefault);
    fprintf(f, "[WindowRounding]\n%1.3f\n", style.WindowRounding);
    fprintf(f, "[TreeNodeSpacing]\n%1.3f\n", style.TreeNodeSpacing);
    fprintf(f, "[ColumnsMinSpacing]\n%1.3f\n", style.ColumnsMinSpacing);
    fprintf(f, "[ScrollBarWidth]\n%1.3f\n", style.ScrollBarWidth);
	     
    for (size_t i = 0; i != ImGuiCol_COUNT; i++)
    {
		const ImVec4& c = style.Colors[i];
        fprintf(f, "[%s]\n", ImGui::GetStyleColorName(i));//ImGuiColNames[i]);
        fprintf(f, "%1.3f %1.3f %1.3f %1.3f\n",c.x,c.y,c.z,c.w);
    }

	fprintf(f,"\n");
    fclose(f);
    
    return true;
}

bool ImGuiLoadStyle(const char* filename,ImGuiStyle& style)
{
	// Load .style file
   	if (!filename)  return false;

    // Load file into memory
    FILE* f;
    if ((f = fopen(filename, "rt")) == NULL) return false;
    if (fseek(f, 0, SEEK_END))  {
       fclose(f); 
       return false; 
    }
    const long f_size_signed = ftell(f);
    if (f_size_signed == -1)    {
       fclose(f); 
       return false; 
    }
    size_t f_size = (size_t)f_size_signed;
    if (fseek(f, 0, SEEK_SET))  {
       fclose(f); 
       return false; 
    }
    char* f_data = (char*)ImGui::MemAlloc(f_size+1);
    f_size = fread(f_data, 1, f_size, f); // Text conversion alter read size so let's not be fussy about return value
    fclose(f);
    if (f_size == 0)    {
        ImGui::MemFree(f_data);
        return false;
    }
    f_data[f_size] = 0;

    // Parse file in memory
    char name[128];name[0]='\0';
    const char* buf_end = f_data + f_size;
    for (const char* line_start = f_data; line_start < buf_end; )
    {
        const char* line_end = line_start;
        while (line_end < buf_end && *line_end != '\n' && *line_end != '\r')
            line_end++;
        
        if (name[0]=='\0' && line_start[0] == '[' && line_end > line_start && line_end[-1] == ']')
        {        
            ImFormatString(name, IM_ARRAYSIZE(name), "%.*s", line_end-line_start-2, line_start+1);
            //fprintf(stderr,"name: %s\n",name);  // dbg
        }
        else if (name[0]!='\0')
        {

            float *pf[4]= {0,0,0,0};
            int npf = 0;

            // parsing 'name' here by filling the fields above
            {
                if      (strcmp(name, "Alpha")==0)                     {npf=1;pf[0]=&style.Alpha;}
                else if (strcmp(name, "WindowPadding")==0)             {npf=2;pf[0]=&style.WindowPadding.x;pf[1]=&style.WindowPadding.y;}
                else if (strcmp(name, "WindowMinSize")==0)             {npf=2;pf[0]=&style.WindowMinSize.x;pf[1]=&style.WindowMinSize.y;}
                else if (strcmp(name, "FramePadding")==0)              {npf=2;pf[0]=&style.FramePadding.x;pf[1]=&style.FramePadding.y;}
                else if (strcmp(name, "ItemSpacing")==0)               {npf=2;pf[0]=&style.ItemSpacing.x;pf[1]=&style.ItemSpacing.y;}
                else if (strcmp(name, "ItemInnerSpacing")==0)          {npf=2;pf[0]=&style.ItemInnerSpacing.x;pf[1]=&style.ItemInnerSpacing.y;}
                else if (strcmp(name, "TouchExtraPadding")==0)         {npf=2;pf[0]=&style.TouchExtraPadding.x;pf[1]=&style.TouchExtraPadding.y;}
                else if (strcmp(name, "AutoFitPadding")==0)            {npf=2;pf[0]=&style.AutoFitPadding.x;pf[1]=&style.AutoFitPadding.y;}
                else if (strcmp(name, "WindowFillAlphaDefault")==0)    {npf=1;pf[0]=&style.WindowFillAlphaDefault;}
                else if (strcmp(name, "WindowRounding")==0)            {npf=1;pf[0]=&style.WindowRounding;}
                else if (strcmp(name, "TreeNodeSpacing")==0)           {npf=1;pf[0]=&style.TreeNodeSpacing;}
                else if (strcmp(name, "ColumnsMinSpacing")==0)         {npf=1;pf[0]=&style.ColumnsMinSpacing;}
                else if (strcmp(name, "ScrollBarWidth")==0)            {npf=1;pf[0]=&style.ScrollBarWidth;}
                // all the colors here
                else {
                    for (int j=0;j<ImGuiCol_COUNT;j++)    {
                        if (strcmp(name,ImGui::GetStyleColorName(j))==0)    {
                            npf = 4;
                            ImVec4& color = style.Colors[j];
                            pf[0]=&color.x;pf[1]=&color.y;pf[2]=&color.z;pf[3]=&color.w;
                            break;
                        }
                    }
                }
            }

            //fprintf(stderr,"name: %s npf=%d\n",name,npf);  // dbg
            // parsing values here and filling pf[]
            float x,y,z,w;
            switch (npf)	{
        		case 1:
                if (sscanf(line_start, "%f", &x) == npf)	{
	                *pf[0] = x;
                }
                else fprintf(stderr,"Warning in ImGui::LoadStyle(\"%s\"): skipped [%s] (parsing error).\n",filename,name); 
                break;         	
        		case 2:
                if (sscanf(line_start, "%f %f", &x, &y) == npf)	{
	                *pf[0] = x;*pf[1] = y;
                }
                else fprintf(stderr,"Warning in ImGui::LoadStyle(\"%s\"): skipped [%s] (parsing error).\n",filename,name); 
                break; 
        		case 3:
                if (sscanf(line_start, "%f %f %f", &x, &y, &z) == npf)	{
	                *pf[0] = x;*pf[1] = y;*pf[2] = z;
                }
                else fprintf(stderr,"Warning in ImGui::LoadStyle(\"%s\"): skipped [%s] (parsing error).\n",filename,name); 
                break; 
        		case 4:
                if (sscanf(line_start, "%f %f %f %f", &x, &y, &z, &w) == npf)	{
	                *pf[0] = x;*pf[1] = y;*pf[2] = z;*pf[3] = w;
                }
                else fprintf(stderr,"Warning in ImGui::LoadStyle(\"%s\"): skipped [%s] (parsing error).\n",filename,name); 
                break;                                        	
        		default:
        		fprintf(stderr,"Warning in ImGui::LoadStyle(\"%s\"): skipped [%s] (unknown field).\n",filename,name); 
        		break;
        	}
        	/*
            // Same reference code from <imgui.cpp> to help parsing
            float x, y;
            int i;
            if (sscanf(line_start, "Pos=%f,%f", &x, &y) == 2)
                settings->Pos = ImVec2(x, y);
            else if (sscanf(line_start, "Size=%f,%f", &x, &y) == 2)
                settings->Size = ImMax(ImVec2(x, y), g.Style.WindowMinSize);
            else if (sscanf(line_start, "Collapsed=%d", &i) == 1)
                settings->Collapsed = (i != 0);
            */
            //---------------------------------------------------------------------------------    
            name[0]='\0'; // mandatory
        }

        line_start = line_end+1;
    }

    // Release memory
    ImGui::MemFree(f_data);
    return true;
}

