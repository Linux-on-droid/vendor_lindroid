package org.lindroid.ui;

import android.view.Surface;

public class NativeLib {
    static {
        System.loadLibrary("jni_lindroidui");
    }

    public static native void nativeStartComposerService();
    public static native void nativeSurfaceCreated(long displayId, Surface surface);
    public static native void nativeSurfaceChanged(long displayId, Surface surface, int dpi, float refresh);
    public static native void nativeSurfaceDestroyed(long displayId, Surface surface);
    public static native void nativeDisplayDestroyed(long displayId);

    public static native void nativeInitInputDevice();
    public static native void nativeReconfigureInputDevice(long displayId, int width, int height);
    public static native void nativeStopInputDevice(long displayId);
    public static native void nativeKeyEvent(long displayId, int keyCode, boolean isDown);
    public static native void nativeTouchEvent(long displayId, int pointerId, int action, int pressure, int x, int y);
    public static native void nativeTouchStylusButtonEvent(long displayId, int button, boolean isDown);
    public static native void nativeTouchStylusHoverEvent(long displayId, int action, int x, int y, int distance, int tilt_x, int tilt_y);
    public static native void nativeTouchStylusEvent(long displayId, int action, int pressure, int x, int y, int tilt_x, int tilt_y);
    public static native void nativePointerMotionEvent(long displayId, int x, int y);
    public static native void nativePointerButtonEvent(long displayId, int button, int x, int y, boolean isDown);
    public static native void nativePointerScrollEvent(long displayId, int value, boolean isVertical);

}
