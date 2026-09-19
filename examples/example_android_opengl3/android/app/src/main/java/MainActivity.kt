package imgui.example.android

import android.app.NativeActivity
import android.content.Context
import android.view.inputmethod.InputMethodManager
import android.view.KeyEvent
import java.util.concurrent.LinkedBlockingQueue

// Minimal Kotlin activity extending NativeActivity.
// The three methods below (showSoftInput, hideSoftInput, pollUnicodeChar) are called
// from C++ via JNI by imgui_impl_android.cpp. They are required because Android's
// native API does not provide: (1) showing/hiding the soft keyboard, (2) getting
// Unicode characters from key events.
//
// The application developer does NOT write this code — it ships with the imgui
// example and is self-contained. The imgui_impl_android backend calls these
// methods automatically.
class MainActivity : NativeActivity() {
    private var unicodeCharQueue: LinkedBlockingQueue<Int> = LinkedBlockingQueue()

    fun showSoftInput() {
        val imm = getSystemService(Context.INPUT_METHOD_SERVICE) as InputMethodManager
        imm.showSoftInput(window.decorView, 0)
    }

    fun hideSoftInput() {
        val imm = getSystemService(Context.INPUT_METHOD_SERVICE) as InputMethodManager
        imm.hideSoftInputFromWindow(window.decorView.windowToken, 0)
    }

    // Native Android key events don't expose getUnicodeChar() to C code.
    // We intercept them here and queue the Unicode chars for the C++ backend to poll.
    override fun dispatchKeyEvent(event: KeyEvent): Boolean {
        if (event.action == KeyEvent.ACTION_DOWN)
            unicodeCharQueue.offer(event.getUnicodeChar(event.metaState))
        return super.dispatchKeyEvent(event)
    }

    fun pollUnicodeChar(): Int = unicodeCharQueue.poll() ?: 0
}
