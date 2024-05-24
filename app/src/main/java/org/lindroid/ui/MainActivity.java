package org.lindroid.ui;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

public class MainActivity extends AppCompatActivity implements SurfaceHolder.Callback {
    static {
        System.loadLibrary("jni_lindroidui");
    }

    private static final long DISPLAY_ID = 0;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        Thread thread = new Thread(this::nativeInit);
        thread.start();
        SurfaceView sv = findViewById(R.id.surfaceView);
        SurfaceHolder sh = sv.getHolder();

        sh.addCallback(this);
    }

    @Override
    public void surfaceCreated(@NonNull SurfaceHolder holder) {
        Surface surface = holder.getSurface();
        if (surface != null) {
            nativeSurfaceCreated(DISPLAY_ID, surface);
        }
    }

    @Override
    public void surfaceChanged(@NonNull SurfaceHolder holder, int format, int w, int h) {
        Surface surface = holder.getSurface();
        if (surface != null) {
            nativeSurfaceChanged(DISPLAY_ID, surface);
        }
    }

    @Override
    public void surfaceDestroyed(@NonNull SurfaceHolder holder) {
        Surface surface = holder.getSurface();
        if (surface != null) {
            nativeSurfaceDestroyed(DISPLAY_ID, surface);
        }
    }

    public native void nativeInit();
    public native void nativeSurfaceCreated(long displayId, Surface surface);
    public native void nativeSurfaceChanged(long displayId, Surface surface);
    public native void nativeSurfaceDestroyed(long displayId, Surface surface);
}