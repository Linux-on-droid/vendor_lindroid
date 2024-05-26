package org.lindroid.ui;

import android.view.Surface;

public class NativeLib {
    static {
        System.loadLibrary("jni_lindroidui");
    }

    public static native void nativeStartComposerService();
    public static native void nativeSurfaceCreated(long displayId, Surface surface);
    public static native void nativeSurfaceChanged(long displayId, Surface surface);
    public static native void nativeSurfaceDestroyed(long displayId, Surface surface);
}
