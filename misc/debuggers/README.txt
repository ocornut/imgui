
-----------------------------------------------------------------------------
 HELPER FILES FOR POPULAR DEBUGGERS
-----------------------------------------------------------------------------

imgui.gdb
    GDB: disable stepping into trivial functions (*1).
    (read comments inside file for details)

imgui.natstepfilter
    Visual Studio Debugger: disable stepping into trivial functions (*1).
    (read comments inside file for details)

imgui.natvis
    Visual Studio Debugger: describe Dear ImGui types for better display.
    With this, types like ImVector<> will be displayed nicely in the debugger.
    (read comments inside file for details)

imgui_lldb.py
    LLDB-based debuggers (*2): synthetic children provider and summaries for Dear ImGui types.
    With this, types like ImVector<> will be displayed nicely in the debugger.
    (read comments inside file for details)
    (*2) Xcode, Android Studio, may be used from VS Code, C++Builder, CLion, Eclipse etc.

-----------------------------------------------------------------------------
 NEW! DISABLED STEPPING INTO TRIVIAL FUNCTIONS
-----------------------------------------------------------------------------

Since 1.93.0 WIP (September 2026),
Dear ImGui uses a IM_NODEBUGSTEP macro to tag functions with __declspec(non_user_code)
or [[gnu::artificial]] to automatically tell debuggers to skip them.

(*1) Assuming the new IM_NODEBUGSTEP macro works, using imgui.natstepfilter etc.
may become unnecessary. Exact side effects on user experience yet to be clarified.

Additional references:
    https://maskray.me/blog/skipping-boring-functions-in-debuggers
    and https://news.ycombinator.com/item?id=42547576

