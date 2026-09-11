# Response to Review Feedback on PR #9469

Thank you for the detailed review — this is exactly the kind of feedback that makes the Android backend better. I agree with most of your points and have made significant changes. Here's my response to each issue:

---

## 1. Display rotation only queried in Init — should be handled in full

**Agreed and fixed.** Rotation is now re-queried in `NewFrame()` whenever the window size changes. The backend caches the last known width/height and only calls JNI when a change is detected (avoids JNI overhead per frame).

```cpp
// In NewFrame:
static int last_w = 0, last_h = 0;
int w = ANativeWindow_getWidth(g_Window);
int h = ANativeWindow_getHeight(g_Window);
if (w != last_w || h != last_h) {
    last_w = w; last_h = h;
    ImGui_ImplAndroid_JniRefreshDisplayMetrics(); // re-query orientation, DPI, etc.
}
```

## 2. Showing/hiding keyboard should NOT be moved into the backend — at least not in current form

**Partially agreed.** I've made the JNI keyboard handling **opt-in** via a new `ImGui_ImplAndroid_SetJniEnabled(bool)` API. When disabled (default), the backend does NOT auto-show/hide the keyboard — the application retains full control, exactly as before. This preserves compatibility with ImRAD and any custom keyboard logic.

The keyboard type (number, text, email, etc.) and action button customization are now exposed via:

```cpp
IMGUI_IMPL_API void ImGui_ImplAndroid_SetKeyboardType(int input_type); // Android InputType constants
IMGUI_IMPL_API void ImGui_ImplAndroid_SetKeyboardAction(int ime_action); // Android IME action constants
```

These are passed through to the JNI layer when the backend handles the keyboard, so apps that DO opt in still get customization.

## 3. Keyboard behavior differences (scroll, button press, release timing)

**Agreed — this is a real problem.** The backend no longer force-hides the keyboard in `NewFrame`. Instead:

- `WantTextInput` state is exposed via `ImGui_ImplAndroid_GetWantTextInput()` so the app can poll it and decide WHEN to hide.
- The backend only shows the keyboard on the rising edge of `WantTextInput` (touch release detection is left to the app).
- Added `ImGui_ImplAndroid_ResubmitTextInput()` helper that re-asserts `io.WantTextInput` — useful for your "resubmit on scroll" pattern.

## 4. Back button should hide keyboard regardless of WantTextInput

**Fixed.** `HandleInputEvent` now intercepts `AKEYCODE_BACK` when the keyboard is visible and hides it, consuming the event. This matches standard Android behavior.

## 5. dispatchKeyEvent doesn't work well — use TextWatcher/OnEditorActionListener

**Acknowledged.** The backend now supports an **alternative input path** via JNI callbacks:

```cpp
// Application registers these in MainActivity.kt:
// - onInputCharacter(int unicodeChar) → called from TextWatcher
// - onEditorAction(int actionId) → called from OnEditorActionListener

// Backend polls these via JNI (same mechanism as before, but now optional):
IMGUI_IMPL_API void ImGui_ImplAndroid_PollUnicodeChars(); // call from your MainLoopStep if desired
IMGUI_IMPL_API void ImGui_ImplAndroid_SetUnicodePollEnabled(bool enabled); // default false
```

The key insight: **the backend provides the JNI plumbing, but the app decides WHEN and WHETHER to call it.** This is the inverse of the original PR which forced it in `NewFrame`.

## 6. GetAssetData and font loading section removed

**Fixed.** `GetAssetData()` is restored in `main.cpp` (it was accidentally deleted in the cleanup). The font loading section is also restored — it was removed because the backend was supposed to handle assets, but that's overreach. The backend should NOT be in the business of loading fonts.

## 7. Crash when switching apps — APP_CMD_TERM_WINDOW handling

**Fixed.** The example now handles `APP_CMD_TERM_WINDOW` correctly:

```cpp
case APP_CMD_TERM_WINDOW:
    // Only release EGL surface, don't destroy context
    // Recreate surface when APP_CMD_INIT_WINDOW arrives again
    eglDestroySurface(display, surface);
    surface = EGL_NO_SURFACE;
    g_Initialized = false; // but keep context alive
    break;
```

The key change: `eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT)` is called before destroying, and the EGL context is preserved for reuse.

## 8. NavBar height / bottom area / long press with haptic feedback

**Partially addressed:**

- **NavBar height**: Added `ImGui_ImplAndroid_GetNavBarHeight()` and `ImGui_ImplAndroid_GetBottomInset()` JNI queries. These are exposed but the app decides whether to use them (configurable, as you requested).
- **Long press**: Added `ImGui_ImplAndroid_SetLongPressCallback()` — the backend detects long-press (500ms default) and calls your callback, where you can trigger haptic feedback via JNI or C++.

---

## Summary of changes to PR #9469

| Area | Before | After |
|------|--------|-------|
| Rotation | Only in Init | Re-queried on resize in NewFrame |
| Keyboard auto-show/hide | Forced in NewFrame | Opt-in via `SetJniEnabled()` |
| Keyboard type/action | Not customizable | `SetKeyboardType()` / `SetKeyboardAction()` |
| Keyboard hide timing | Backend decides | App polls `GetWantTextInput()` |
| Back button | Not handled | Hides keyboard when visible |
| Unicode input | Forced via dispatchKeyEvent | Optional `PollUnicodeChars()` |
| GetAssetData | Removed | Restored in example |
| APP_CMD_TERM_WINDOW | Crashes | Proper EGL surface lifecycle |
| NavBar height | Not supported | `GetNavBarHeight()` / `GetBottomInset()` |
| Long press | Not supported | `SetLongPressCallback()` |

The core philosophy shift: **the backend provides JNI plumbing and utilities, but the APPLICATION decides when to use them.** This makes it compatible with ImRAD-generated code, custom keyboard logic, and the standard Android lifecycle.
