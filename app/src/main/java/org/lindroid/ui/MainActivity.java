package org.lindroid.ui;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

public class MainActivity extends AppCompatActivity implements SurfaceHolder.Callback {

    // Used to load the 'ui' library on application startup.
    static {
        System.loadLibrary("lindroidui");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        // Example of a call to a native method
        SurfaceView sv = findViewById(R.id.surfaceView);
        SurfaceHolder sh = sv.getHolder();

        sh.addCallback(this);

    }

    /**
     * A native method that is implemented by the 'ui' native library,
     * which is packaged with this application.
     */
    public native void nativeSurfaceCreated(Surface surface);
    public native void nativeSurfaceChanged(Surface surface);

    @Override
    public void surfaceCreated(@NonNull SurfaceHolder holder) {
        Surface surface = holder.getSurface();
        if (surface != null) {
            nativeSurfaceCreated(surface);
        }
    }

    @Override
    public void surfaceChanged(@NonNull SurfaceHolder holder, int format, int w, int h) {
        Surface surface = holder.getSurface();
        if (surface != null) {
            nativeSurfaceChanged(surface);
        }
    }

    @Override
    public void surfaceDestroyed(@NonNull SurfaceHolder surfaceHolder) {

    }
}