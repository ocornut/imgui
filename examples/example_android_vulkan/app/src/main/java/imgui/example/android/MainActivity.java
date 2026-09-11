package imgui.example.android;

import android.app.NativeActivity;
import android.content.Context;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.inputmethod.InputMethodManager;
import java.util.concurrent.LinkedBlockingQueue;

/**
 * Minimal NativeActivity subclass that adds:
 * 1. showSoftInput() — show the on-screen keyboard (called from C++ via JNI)
 * 2. hideSoftInput() — hide the on-screen keyboard (called from C++ via JNI)
 * 3. pollUnicodeChar() — return queued unicode chars (called from C++ via JNI)
 */
public class MainActivity extends NativeActivity {

    private final LinkedBlockingQueue<Integer> mUnicodeCharQueue = new LinkedBlockingQueue<>();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    // Called from native code via JNI
    public void showSoftInput() {
        InputMethodManager imm = (InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE);
        if (imm != null)
            imm.showSoftInput(getWindow().getDecorView(), 0);
    }

    // Called from native code via JNI
    public void hideSoftInput() {
        InputMethodManager imm = (InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE);
        if (imm != null)
            imm.hideSoftInputFromWindow(getWindow().getDecorView().getWindowToken(), 0);
    }

    // Intercept key events to get unicode character values
    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        if (event.getAction() == KeyEvent.ACTION_DOWN) {
            int unicodeChar = event.getUnicodeChar();
            if (unicodeChar != 0)
                mUnicodeCharQueue.offer(unicodeChar);
        } else if (event.getAction() == KeyEvent.ACTION_MULTIPLE) {
            String characters = event.getCharacters();
            if (characters != null) {
                for (int i = 0; i < characters.length(); i++) {
                    mUnicodeCharQueue.offer((int) characters.charAt(i));
                }
            }
        }
        return super.dispatchKeyEvent(event);
    }

    // Called from native code via JNI — returns 0 when queue is empty
    public int pollUnicodeChar() {
        Integer val = mUnicodeCharQueue.poll();
        return (val != null) ? val : 0;
    }

    // Called from native code via JNI
    public String getClipboardText() {
        final java.util.concurrent.CountDownLatch latch = new java.util.concurrent.CountDownLatch(1);
        final String[] result = new String[1];
        result[0] = "";

        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                try {
                    android.content.ClipboardManager clipboard = (android.content.ClipboardManager) getSystemService(
                            Context.CLIPBOARD_SERVICE);
                    if (clipboard != null && clipboard.hasPrimaryClip()) {
                        android.content.ClipData clip = clipboard.getPrimaryClip();
                        if (clip != null && clip.getItemCount() > 0) {
                            CharSequence text = clip.getItemAt(0).getText();
                            if (text != null)
                                result[0] = text.toString();
                        }
                    }
                } finally {
                    latch.countDown();
                }
            }
        });

        try {
            latch.await();
        } catch (InterruptedException e) {
            e.printStackTrace();
        }

        return result[0];
    }

    // Called from native code via JNI
    public void setClipboardText(final String text) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                android.content.ClipboardManager clipboard = (android.content.ClipboardManager) getSystemService(
                        Context.CLIPBOARD_SERVICE);
                if (clipboard != null) {
                    android.content.ClipData clip = android.content.ClipData.newPlainText("clipboard", text);
                    clipboard.setPrimaryClip(clip);
                }
            }
        });
    }
}
