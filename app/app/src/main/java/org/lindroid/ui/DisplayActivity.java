package org.lindroid.ui;

import static org.lindroid.ui.NativeLib.nativeDisplayDestroyed;
import static org.lindroid.ui.NativeLib.nativeKeyEvent;
import static org.lindroid.ui.NativeLib.nativePointerButtonEvent;
import static org.lindroid.ui.NativeLib.nativePointerMotionEvent;
import static org.lindroid.ui.NativeLib.nativePointerScrollEvent;
import static org.lindroid.ui.NativeLib.nativeReconfigureInputDevice;
import static org.lindroid.ui.NativeLib.nativeStopInputDevice;
import static org.lindroid.ui.NativeLib.nativeSurfaceChanged;
import static org.lindroid.ui.NativeLib.nativeSurfaceCreated;
import static org.lindroid.ui.NativeLib.nativeSurfaceDestroyed;
import static org.lindroid.ui.NativeLib.nativeTouchEvent;

import android.annotation.SuppressLint;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.PointerIcon;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.WindowInsets;
import android.view.WindowInsetsController;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import com.google.android.material.dialog.MaterialAlertDialogBuilder;

import java.util.Objects;

public class DisplayActivity extends AppCompatActivity implements SurfaceHolder.Callback,
        View.OnTouchListener,
        View.OnHoverListener,
        View.OnGenericMotionListener {
    private static final String TAG = "DisplayActivity";
    private static Handler mHandler; // globally makes sure the calls are ordered
    private String mContainerName;
    private int mDisplayID = 0;
    private int mPreviousWidth = 0;
    private int mPreviousHeight = 0;
    private Runnable mSurfaceRunnable;

    @Override
    @SuppressLint("ClickableViewAccessibility") // use screen reader inside linux
    protected void onCreate(Bundle savedInstanceState) {
        int displayID = getIntent().getIntExtra("displayID", 0);
        String containerName = getIntent().getStringExtra("containerName");
        super.onCreate(savedInstanceState);
        if (HardwareService.getInstance() == null) {
            startForegroundService(new Intent(this, HardwareService.class));
        }
        SurfaceView mSurfaceView = new SurfaceView(this);
        setContentView(mSurfaceView);
        final WindowInsetsController controller = getWindow().getInsetsController();
        if (controller != null) {
            controller.hide(WindowInsets.Type.statusBars() | WindowInsets.Type.navigationBars());
            controller.setSystemBarsBehavior(WindowInsetsController.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);
        }
        Log.d(TAG, "Starting container: " + containerName + " on display: " + displayID);
        mDisplayID = displayID;
        mContainerName = containerName;

        if (mHandler == null)
            mHandler = new Handler(Looper.getMainLooper());
        SurfaceHolder sh = mSurfaceView.getHolder();
        mSurfaceView.setOnTouchListener(this);
        mSurfaceView.setOnHoverListener(this);
        mSurfaceView.setOnGenericMotionListener(this);
        sh.addCallback(this);

        // Hide pointer icon
        mSurfaceView.setPointerIcon(PointerIcon.getSystemIcon(this, PointerIcon.TYPE_NULL));
    }

    @Override
    public void onBackPressed() {
        if (mDisplayID == 0 && mContainerName != null && ContainerManager.isRunning(mContainerName)) {
            new MaterialAlertDialogBuilder(this)
                    .setTitle(R.string.stop_title)
                    .setMessage(R.string.stop_message)
                    .setPositiveButton(R.string.yes, (dialog, which) -> {
                        HardwareService hs = HardwareService.getInstance();
                        if (hs != null) {
                            hs.stopSelf();
                        }
                        ContainerManager.stop(mContainerName);
                        finish();
                    })
                    .setNeutralButton(android.R.string.cancel, (dialog, which) -> {})
                    .setNegativeButton(R.string.no, (dialog, which) -> {
                        super.onBackPressed();
                    })
                    .show();
        } else {
            finish();
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        // Destroyed in HardwareService when user decides to stop container
        if (mDisplayID != 0) {
            nativeDisplayDestroyed(mDisplayID);
            nativeStopInputDevice(mDisplayID);
        }
    }

    @Override
    public boolean onGenericMotion(View view, MotionEvent motionEvent) {
        if (motionEvent.getAction() == MotionEvent.ACTION_SCROLL) {
            int vScroll = (int) motionEvent.getAxisValue(MotionEvent.AXIS_VSCROLL);
            int hScroll = (int) motionEvent.getAxisValue(MotionEvent.AXIS_HSCROLL);

            if (vScroll != 0)
                nativePointerScrollEvent(mDisplayID, vScroll, true);
            else if (hScroll != 0)
                nativePointerScrollEvent(mDisplayID, hScroll, false);
        }

        if (motionEvent.getAction() == MotionEvent.ACTION_BUTTON_PRESS ||
                motionEvent.getAction() == MotionEvent.ACTION_BUTTON_RELEASE) {
            int x = (int) motionEvent.getX();
            int y = (int) motionEvent.getY();
            boolean isDown = motionEvent.getAction() == MotionEvent.ACTION_BUTTON_PRESS;
            switch (motionEvent.getActionButton()) {
                case MotionEvent.BUTTON_PRIMARY:
                    nativePointerButtonEvent(mDisplayID, 0x110, x, y, isDown);
                    break;
                case MotionEvent.BUTTON_SECONDARY:
                    nativePointerButtonEvent(mDisplayID, 0x111, x, y, isDown);
                    break;
                case MotionEvent.BUTTON_TERTIARY:
                    nativePointerButtonEvent(mDisplayID, 0x112, x, y, isDown);
                    break;
                case MotionEvent.BUTTON_BACK:
                    nativePointerButtonEvent(mDisplayID, 0x116, x, y, isDown);
                    break;
                case MotionEvent.BUTTON_FORWARD:
                    nativePointerButtonEvent(mDisplayID, 0x115, x, y, isDown);
                    break;
            }
        }
        return true;
    }

    @Override
    public boolean onHover(View view, MotionEvent motionEvent) {
        int x = (int) motionEvent.getX(0);
        int y = (int) motionEvent.getY(0);
        nativePointerMotionEvent(mDisplayID, x, y);
        return true;
    }

    @SuppressLint("ClickableViewAccessibility") // see above
    @Override
    public boolean onTouch(View view, MotionEvent motionEvent) {
        if (motionEvent.getSource() == InputDevice.SOURCE_MOUSE) {
            onGenericMotion(view, motionEvent);
            return onHover(view, motionEvent);
        }

        int pointerCount = motionEvent.getPointerCount();
        for (int i = 0; i < pointerCount; i++) {
            int pointerId = motionEvent.getPointerId(i);
            int action = motionEvent.getActionMasked();
            int x = (int) motionEvent.getX(i);
            int y = (int) motionEvent.getY(i);
            int pressure = (int) motionEvent.getPressure(i);
            if (action == MotionEvent.ACTION_MOVE ||
                    action == MotionEvent.ACTION_DOWN ||
                    action == MotionEvent.ACTION_UP)
                nativeTouchEvent(mDisplayID, pointerId, action, pressure, x, y);
        }
        return true;
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        nativeKeyEvent(mDisplayID, event.getScanCode(), true);
        return super.onKeyDown(keyCode, event);
    }

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event) {
        nativeKeyEvent(mDisplayID, event.getScanCode(), false);
        return super.onKeyUp(keyCode, event);
    }

    @Override
    public void onPointerCaptureChanged(boolean hasCapture) {
        super.onPointerCaptureChanged(hasCapture);
        // TODO implement pointer capture
    }

    @Override
    public void surfaceCreated(@NonNull SurfaceHolder holder) {
        Surface surface = holder.getSurface();
        if (surface != null) {
            nativeSurfaceCreated(mDisplayID, surface);
        }
    }

    @Override
    public void surfaceChanged(@NonNull SurfaceHolder holder, int format, int w, int h) {
        mHandler.removeCallbacksAndMessages(mSurfaceRunnable);
        mSurfaceRunnable = () -> applySurfaceChanges(holder, format, w, h);
        mHandler.postDelayed(mSurfaceRunnable, 200);
    }

    private void applySurfaceChanges(@NonNull SurfaceHolder holder, int format, int w, int h) {
        // TODO format?
        Surface surface = holder.getSurface();
        if (surface != null) {
            float refresh = 60.0f;
            try {
                refresh = Objects.requireNonNull(getDisplay()).getRefreshRate();
            } catch (Exception e) {
                Log.e(TAG, "Failed to get display refresh rate", e);
            }
            nativeSurfaceChanged(mDisplayID, surface, getResources().getConfiguration().densityDpi, refresh - 6);
            if (mPreviousWidth != w || mPreviousHeight != h) {
                nativeReconfigureInputDevice(mDisplayID, w, h);
                mPreviousWidth = w;
                mPreviousHeight = h;
            }
        }
    }

    @Override
    public void surfaceDestroyed(@NonNull SurfaceHolder holder) {
        Surface surface = holder.getSurface();
        if (surface != null) {
            nativeSurfaceDestroyed(mDisplayID, surface);
        }
    }
}
