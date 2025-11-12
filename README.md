# _Truly_ Bloat free ImGUI with only GLFW + Vulkan/GL3 backend
## Also fixed GLFW window wayland mouse position scaling bug on HiDPI screens
## On wayland, you must define `IMGUI_IMPL_GLFW_DISABLE_X11` before compiling ImGUI
### NOTE: you might want to change `glfwGetPrimaryMonitor()`

```diff
diff --git a/backends/imgui_impl_glfw.cpp b/backends/imgui_impl_glfw.cpp
index 796b35b8..320c96e3 100644
--- a/backends/imgui_impl_glfw.cpp
+++ b/backends/imgui_impl_glfw.cpp
@@ -550,8 +550,12 @@ void ImGui_ImplGlfw_CursorPosCallback(GLFWwindow* window, double x, double y)
         x += window_x;
         y += window_y;
     }
-    io.AddMousePosEvent((float)x, (float)y);
-    bd->LastValidMousePos = ImVec2((float)x, (float)y);
+
+    float xscale, yscale;
+    glfwGetMonitorContentScale(glfwGetPrimaryMonitor(), &xscale, &yscale);
+
+    io.AddMousePosEvent(static_cast<float>(x) / xscale, static_cast<float>(y) / yscale);
+    bd->LastValidMousePos = ImVec2(static_cast<float>(x) / xscale, static_cast<float>(y) / yscale);
 }
 
 // Workaround: X11 seems to send spurious Leave/Enter events which would make us lose our position,
```
