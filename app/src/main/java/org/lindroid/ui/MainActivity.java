package org.lindroid.ui;

import static org.lindroid.ui.NativeLib.nativeDisplayDestroyed;
import static org.lindroid.ui.NativeLib.nativeSurfaceChanged;
import static org.lindroid.ui.NativeLib.nativeSurfaceCreated;
import static org.lindroid.ui.NativeLib.nativeSurfaceDestroyed;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import android.content.Intent;
import android.os.Bundle;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

public class MainActivity extends AppCompatActivity implements SurfaceHolder.Callback {
    private static final long DISPLAY_ID = 0;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        if (!HardwareService.isInstanceCreated()) {
            startService(new Intent(this, HardwareService.class));
        }
        SurfaceView sv = findViewById(R.id.surfaceView);
        SurfaceHolder sh = sv.getHolder();

        sh.addCallback(this);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        // nativeDisplayDestroyed(DISPLAY_ID); Lets never destroy primary display
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
}