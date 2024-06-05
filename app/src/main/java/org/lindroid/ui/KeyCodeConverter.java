package org.lindroid.ui;

import android.util.SparseIntArray;
import android.view.KeyEvent;

public class KeyCodeConverter {

    private static final SparseIntArray keyCodeMap = new SparseIntArray();

    static {
        // Map Android KeyEvent key codes to Linux kernel input key codes
        // Letters
        keyCodeMap.put(KeyEvent.KEYCODE_A, 30);
        keyCodeMap.put(KeyEvent.KEYCODE_B, 48);
        keyCodeMap.put(KeyEvent.KEYCODE_C, 46);
        keyCodeMap.put(KeyEvent.KEYCODE_D, 32);
        keyCodeMap.put(KeyEvent.KEYCODE_E, 18);
        keyCodeMap.put(KeyEvent.KEYCODE_F, 33);
        keyCodeMap.put(KeyEvent.KEYCODE_G, 34);
        keyCodeMap.put(KeyEvent.KEYCODE_H, 35);
        keyCodeMap.put(KeyEvent.KEYCODE_I, 23);
        keyCodeMap.put(KeyEvent.KEYCODE_J, 36);
        keyCodeMap.put(KeyEvent.KEYCODE_K, 37);
        keyCodeMap.put(KeyEvent.KEYCODE_L, 38);
        keyCodeMap.put(KeyEvent.KEYCODE_M, 50);
        keyCodeMap.put(KeyEvent.KEYCODE_N, 49);
        keyCodeMap.put(KeyEvent.KEYCODE_O, 24);
        keyCodeMap.put(KeyEvent.KEYCODE_P, 25);
        keyCodeMap.put(KeyEvent.KEYCODE_Q, 16);
        keyCodeMap.put(KeyEvent.KEYCODE_R, 19);
        keyCodeMap.put(KeyEvent.KEYCODE_S, 31);
        keyCodeMap.put(KeyEvent.KEYCODE_T, 20);
        keyCodeMap.put(KeyEvent.KEYCODE_U, 22);
        keyCodeMap.put(KeyEvent.KEYCODE_V, 47);
        keyCodeMap.put(KeyEvent.KEYCODE_W, 17);
        keyCodeMap.put(KeyEvent.KEYCODE_X, 45);
        keyCodeMap.put(KeyEvent.KEYCODE_Y, 21);
        keyCodeMap.put(KeyEvent.KEYCODE_Z, 44);

        // Numbers
        keyCodeMap.put(KeyEvent.KEYCODE_0, 11);
        keyCodeMap.put(KeyEvent.KEYCODE_1, 2);
        keyCodeMap.put(KeyEvent.KEYCODE_2, 3);
        keyCodeMap.put(KeyEvent.KEYCODE_3, 4);
        keyCodeMap.put(KeyEvent.KEYCODE_4, 5);
        keyCodeMap.put(KeyEvent.KEYCODE_5, 6);
        keyCodeMap.put(KeyEvent.KEYCODE_6, 7);
        keyCodeMap.put(KeyEvent.KEYCODE_7, 8);
        keyCodeMap.put(KeyEvent.KEYCODE_8, 9);
        keyCodeMap.put(KeyEvent.KEYCODE_9, 10);

        // Auxiliary keys
        keyCodeMap.put(KeyEvent.KEYCODE_ALT_LEFT, 56);
        keyCodeMap.put(KeyEvent.KEYCODE_ALT_RIGHT, 100);
        keyCodeMap.put(KeyEvent.KEYCODE_CTRL_LEFT, 29);
        keyCodeMap.put(KeyEvent.KEYCODE_CTRL_RIGHT, 97);
        keyCodeMap.put(KeyEvent.KEYCODE_SHIFT_LEFT, 42);
        keyCodeMap.put(KeyEvent.KEYCODE_SHIFT_RIGHT, 54);
        keyCodeMap.put(KeyEvent.KEYCODE_TAB, 15);
        keyCodeMap.put(KeyEvent.KEYCODE_SPACE, 57);
        keyCodeMap.put(KeyEvent.KEYCODE_ENTER, 28);
        keyCodeMap.put(KeyEvent.KEYCODE_BACK, 14);
        keyCodeMap.put(KeyEvent.KEYCODE_DEL, 111);
        keyCodeMap.put(KeyEvent.KEYCODE_FORWARD_DEL, 14);
        keyCodeMap.put(KeyEvent.KEYCODE_ESCAPE, 1);
        keyCodeMap.put(KeyEvent.KEYCODE_CAPS_LOCK, 58);
        keyCodeMap.put(KeyEvent.KEYCODE_NUM_LOCK, 69);
        keyCodeMap.put(KeyEvent.KEYCODE_SCROLL_LOCK, 70);

        // Arrow keys
        keyCodeMap.put(KeyEvent.KEYCODE_DPAD_UP, 103);
        keyCodeMap.put(KeyEvent.KEYCODE_DPAD_DOWN, 108);
        keyCodeMap.put(KeyEvent.KEYCODE_DPAD_LEFT, 105);
        keyCodeMap.put(KeyEvent.KEYCODE_DPAD_RIGHT, 106);

        // Function keys
        keyCodeMap.put(KeyEvent.KEYCODE_F1, 59);
        keyCodeMap.put(KeyEvent.KEYCODE_F2, 60);
        keyCodeMap.put(KeyEvent.KEYCODE_F3, 61);
        keyCodeMap.put(KeyEvent.KEYCODE_F4, 62);
        keyCodeMap.put(KeyEvent.KEYCODE_F5, 63);
        keyCodeMap.put(KeyEvent.KEYCODE_F6, 64);
        keyCodeMap.put(KeyEvent.KEYCODE_F7, 65);
        keyCodeMap.put(KeyEvent.KEYCODE_F8, 66);
        keyCodeMap.put(KeyEvent.KEYCODE_F9, 67);
        keyCodeMap.put(KeyEvent.KEYCODE_F10, 68);
        keyCodeMap.put(KeyEvent.KEYCODE_F11, 87);
        keyCodeMap.put(KeyEvent.KEYCODE_F12, 88);

        // Additional keys (Media, Volume, etc.)
        keyCodeMap.put(KeyEvent.KEYCODE_MEDIA_PLAY, 164);
        keyCodeMap.put(KeyEvent.KEYCODE_MEDIA_PAUSE, 164);
        keyCodeMap.put(KeyEvent.KEYCODE_MEDIA_STOP, 166);
        keyCodeMap.put(KeyEvent.KEYCODE_MEDIA_NEXT, 163);
        keyCodeMap.put(KeyEvent.KEYCODE_MEDIA_PREVIOUS, 165);
        keyCodeMap.put(KeyEvent.KEYCODE_VOLUME_UP, 115);
        keyCodeMap.put(KeyEvent.KEYCODE_VOLUME_DOWN, 114);
        keyCodeMap.put(KeyEvent.KEYCODE_VOLUME_MUTE, 113);

        // Symbols and punctuation
        keyCodeMap.put(KeyEvent.KEYCODE_COMMA, 51);
        keyCodeMap.put(KeyEvent.KEYCODE_PERIOD, 52);
        keyCodeMap.put(KeyEvent.KEYCODE_MINUS, 12);
        keyCodeMap.put(KeyEvent.KEYCODE_EQUALS, 13);
        keyCodeMap.put(KeyEvent.KEYCODE_LEFT_BRACKET, 26);
        keyCodeMap.put(KeyEvent.KEYCODE_RIGHT_BRACKET, 27);
        keyCodeMap.put(KeyEvent.KEYCODE_BACKSLASH, 43);
        keyCodeMap.put(KeyEvent.KEYCODE_SEMICOLON, 39);
        keyCodeMap.put(KeyEvent.KEYCODE_APOSTROPHE, 40);
        keyCodeMap.put(KeyEvent.KEYCODE_GRAVE, 41); // Backtick
        keyCodeMap.put(KeyEvent.KEYCODE_SLASH, 53);

        // More mappings can be added as needed
    }

    public static int convertKeyCode(int androidKeyCode) {
        return keyCodeMap.get(androidKeyCode, -1); // -1 if the key code is not found
    }
}
