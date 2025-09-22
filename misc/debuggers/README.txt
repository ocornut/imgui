
HELPER FILES FOR POPULAR DEBUGGERS

imgui.gdb
    GDB: disable stepping into trivial functions.
    (read comments inside file for details)

imgui.natstepfilter
    Visual Studio Debugger: disable stepping into trivial functions.
    (read comments inside file for details)

imgui.natvis
    Visual Studio Debugger: describe Dear ImGui types for better display.
    With this, types like ImVector<> will be displayed nicely in the debugger.
    (read comments inside file for details)

imgui_lldb.py
    LLDB-based debuggers (*): synthetic children provider and summaries for Dear ImGui types.
    With this, types like ImVector<> will be displayed nicely in the debugger.
    (read comments inside file for details)
    (*) Xcode, Android Studio, may be used from VS Code, C++Builder, CLion, Eclipse etc.
