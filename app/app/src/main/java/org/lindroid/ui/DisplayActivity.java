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
import static org.lindroid.ui.NativeLib.nativeTouchStylusButtonEvent;
import static org.lindroid.ui.NativeLib.nativeTouchStylusHoverEvent;
import static org.lindroid.ui.NativeLib.nativeTouchStylusEvent;

import static org.lindroid.ui.NativeLib.nativeSetAppForeground;
import static org.lindroid.ui.NativeLib.nativeGetUiRunning;
import android.annotation.SuppressLint;
import android.content.Context;
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
import android.graphics.SurfaceTexture;
import android.view.TextureView;
import android.view.View;
import android.view.WindowInsets;
import android.view.WindowInsetsController;
import android.graphics.Canvas;
import android.view.Choreographer;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.activity.OnBackPressedCallback;

import com.google.android.material.dialog.MaterialAlertDialogBuilder;

import java.util.List;
import java.util.ArrayList;
import java.util.Objects;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class DisplayActivity extends AppCompatActivity implements TextureView.SurfaceTextureListener,
        View.OnTouchListener,
        View.OnHoverListener,
        View.OnGenericMotionListener {
    private static final String TAG = "DisplayActivity";
    private static Handler mHandler; // globally makes sure the calls are ordered
    private String mContainerName;
    private int mDisplayID = 0;
    private int mPreviousWidth = 0;
    private int mPreviousHeight = 0;
    private int mPreviousDensityDpi = 0;
    private float mPreviousRefresh = 0.0f;
    private Runnable mSurfaceRunnable;
    private OnBackPressedCallback backCallback;
    private ExecutorService teardownExecutor = Executors.newSingleThreadExecutor();
    private TextureView mTextureView;
    private Surface mCurrentSurface;
    private Handler mDpmsHandler;
    private Runnable mDpmsOffRunnable;

    private final List<String> displayedLogs = new ArrayList<>();
    private int scrollOffset = 0;
    private static final int LINE_HEIGHT = 20;
    private int visibleLines;

    @Override
    @SuppressLint("ClickableViewAccessibility") // use screen reader inside linux
    protected void onCreate(Bundle savedInstanceState) {
        int displayID = getIntent().getIntExtra("displayID", 0);
        String containerName = getIntent().getStringExtra("containerName");
        super.onCreate(savedInstanceState);
        if (HardwareService.getInstance() == null) {
            startForegroundService(new Intent(this, HardwareService.class));
        }
        mTextureView = new TextureView(this);
        mTextureView.setOpaque(true);
        setContentView(mTextureView);
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
        mDpmsHandler = new Handler(Looper.getMainLooper());
        mDpmsOffRunnable = () -> nativeSetAppForeground(mDisplayID, false);
        mTextureView.setOnTouchListener(this);
        mTextureView.setOnHoverListener(this);
        mTextureView.setOnGenericMotionListener(this);
        mTextureView.setSurfaceTextureListener(this);

        // Hide pointer icon
        mTextureView.setPointerIcon(PointerIcon.getSystemIcon(this, PointerIcon.TYPE_NULL));

        // Register for log updates
        ContainerManager.addLogUpdateListener(mContainerName, this::onLogUpdated);
        ContainerManager.startFetchingLogs(mContainerName);

        // Back pressed handling for stopping container
        backCallback = new OnBackPressedCallback(true) {
            @Override
            public void handleOnBackPressed() {
                if (mDisplayID == 0 && mContainerName != null && ContainerManager.isRunning(mContainerName)) {
                    MaterialAlertDialogBuilder builder =
                            new MaterialAlertDialogBuilder(DisplayActivity.this);
                    builder.setTitle(R.string.stop_title);
                    builder.setMessage(R.string.stop_message);
                    builder.setPositiveButton(R.string.yes, (dialog, which) -> {
                        teardownExecutor.execute(() -> {
                            try {
                                DisplayActivity.this.stopService(new Intent(
                                        DisplayActivity.this, HardwareService.class));
                                ContainerManager.stop(mContainerName);
                            } catch (Exception e) {
                                Log.e(TAG, "Failure stopping container", e);
                            } finally {
                                runOnUiThread(DisplayActivity.this::finish);
                            }
                        });
                    });
                    builder.setNegativeButton(R.string.no, (dialog, which) -> {
                        backCallback.setEnabled(false);
                        getOnBackPressedDispatcher().onBackPressed();
                        backCallback.setEnabled(true);
                    });
                    builder.setNeutralButton(android.R.string.cancel, (dialog, which) -> {});
                    builder.show();
                } else {
                    backCallback.setEnabled(false);
                    getOnBackPressedDispatcher().onBackPressed();
                    backCallback.setEnabled(true);
                }
            }
        };
        getOnBackPressedDispatcher().addCallback(this, backCallback);
    }

    private void drawLogs() {
        if(nativeGetUiRunning())
            return;
        Canvas canvas = mTextureView.lockCanvas();
        if (canvas != null) {
            try {
                // clear the canvas
                canvas.drawColor(android.graphics.Color.BLACK);

                android.graphics.Paint paint = new android.graphics.Paint();
                paint.setColor(android.graphics.Color.WHITE);
                paint.setTextSize(24);
                paint.setAntiAlias(true);
                paint.setTypeface(android.graphics.Typeface.MONOSPACE);

                int surfaceHeight = canvas.getHeight();
                int lineHeight = (int) paint.getTextSize() + LINE_HEIGHT;
                visibleLines = surfaceHeight / lineHeight;

                int startLine = Math.max(0, displayedLogs.size() - visibleLines - scrollOffset);

                int y = lineHeight;
                for (int i = startLine; i < displayedLogs.size(); i++) {
                    canvas.drawText(displayedLogs.get(i), 10, y, paint);
                    y += lineHeight;
                }
            } finally {
                mTextureView.unlockCanvasAndPost(canvas);
            }
        }
    }

    private void onLogUpdated(String containerName, String latestLog) {
        Log.d(TAG, "New log for container " + containerName + ": " + latestLog);

        runOnUiThread(() -> {
            synchronized (displayedLogs) {
                String[] newLines = latestLog.split("\n");
                for (String line : newLines) {
                    displayedLogs.add(line);
                }

                if (displayedLogs.size() > visibleLines) {
                    scrollOffset = 0;
                } else {
                    scrollOffset = Math.max(0, visibleLines - displayedLogs.size());
                }
            }
            // draw the logs
            drawLogs();
        });
    }

    @Override
    protected void onResume() {
        super.onResume();
        mDpmsHandler.removeCallbacks(mDpmsOffRunnable);
        nativeSetAppForeground(mDisplayID, true);
    }

    @Override
    protected void onPause() {
        super.onPause();
        mDpmsHandler.postDelayed(mDpmsOffRunnable, 1000);
    }

    @Override
    protected void onDestroy() {
        if (teardownExecutor != null) teardownExecutor.shutdownNow();
        super.onDestroy();
        // Destroyed in HardwareService when user decides to stop container
        if (mDisplayID != 0) {
            nativeDisplayDestroyed(mDisplayID);
            nativeStopInputDevice(mDisplayID);
        }
        if (ContainerManager.isAtLeastOneRunning() == null && HardwareService.getInstance() != null)
            stopService(new Intent(this, HardwareService.class));

        // Stop log fetching and unregister listener
        ContainerManager.stopFetchingLogs(mContainerName);
        ContainerManager.removeLogUpdateListener(mContainerName, this::onLogUpdated);
        if (mCurrentSurface != null) {
            mCurrentSurface.release();
            mCurrentSurface = null;
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
        if (motionEvent.getToolType(0)  == MotionEvent.TOOL_TYPE_STYLUS) {
            int distance = (int) motionEvent.getAxisValue(MotionEvent.AXIS_DISTANCE);
            float tilt = motionEvent.getAxisValue(MotionEvent.AXIS_TILT);
            float orientation = motionEvent.getAxisValue(MotionEvent.AXIS_ORIENTATION);


            int tiltX = (int) (tilt * Math.cos(orientation));
            int tiltY = (int) (tilt * Math.sin(orientation));
            int action = motionEvent.getActionMasked();
            nativeTouchStylusHoverEvent(mDisplayID, action, x, y, distance, tiltX, tiltY);
        } else {
            nativePointerMotionEvent(mDisplayID, x, y);
        }
        return true;
    }

    @SuppressLint("ClickableViewAccessibility") // see above
    @Override
    public boolean onTouch(View view, MotionEvent motionEvent) {
        if (motionEvent.getSource() == InputDevice.SOURCE_MOUSE) {
            onGenericMotion(view, motionEvent);
            return onHover(view, motionEvent);
        }

        if (motionEvent.getToolType(0)  == MotionEvent.TOOL_TYPE_STYLUS) {
            int x = (int) motionEvent.getX(0);
            int y = (int) motionEvent.getY(0);
            int pressure = (int) (motionEvent.getPressure(0) * 4096);
            float tilt = motionEvent.getAxisValue(MotionEvent.AXIS_TILT); // Tilt angle in radians
            float orientation = motionEvent.getAxisValue(MotionEvent.AXIS_ORIENTATION); // Orientation angle in radians

            // Calculate tilt_x and tilt_y using the tilt and orientation values
            int tiltX = (int) (tilt * Math.cos(orientation) * 90);
            int tiltY = (int) (tilt * Math.sin(orientation) * 90);
            int action = motionEvent.getActionMasked();
            nativeTouchStylusEvent(mDisplayID, action, pressure, x, y, tiltX, tiltY);
            return true;
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
    public void onSurfaceTextureAvailable(@NonNull SurfaceTexture surfaceTexture, int width, int height) {
        mCurrentSurface = new Surface(surfaceTexture);
        nativeSurfaceCreated(mDisplayID, mCurrentSurface);
        triggerSurfaceChanged(mCurrentSurface, width, height);
    }

    @Override
    public void onSurfaceTextureSizeChanged(@NonNull SurfaceTexture surfaceTexture, int width, int height) {
        if (mCurrentSurface != null) {
            triggerSurfaceChanged(mCurrentSurface, width, height);
        }
    }

    @Override
    public boolean onSurfaceTextureDestroyed(@NonNull SurfaceTexture surfaceTexture) {
        if (mCurrentSurface != null) {
            nativeSurfaceDestroyed(mDisplayID, mCurrentSurface);
            mCurrentSurface.release();
            mCurrentSurface = null;
        }
        return true;
    }

    @Override
    public void onSurfaceTextureUpdated(@NonNull SurfaceTexture surfaceTexture) {
    }

    private void triggerSurfaceChanged(Surface surface, int w, int h) {
        if (mSurfaceRunnable != null)
            mHandler.removeCallbacks(mSurfaceRunnable);
        mSurfaceRunnable = () -> applySurfaceChanges(surface, w, h);
        mHandler.postDelayed(mSurfaceRunnable, 200);
    }

    private void applySurfaceChanges(Surface surface, int w, int h) {
        if (surface != null && surface.isValid()) {
            float refresh = 60.0f;
            try {
                refresh = Objects.requireNonNull(getDisplay()).getRefreshRate();
            } catch (Exception e) {
                Log.e(TAG, "Failed to get display refresh rate", e);
            }
            int densityDpi = getResources().getConfiguration().densityDpi;

            boolean sizeChanged = (mPreviousWidth != w || mPreviousHeight != h);
            boolean densityChanged = (mPreviousDensityDpi != densityDpi);
            boolean refreshChanged = (Float.compare(mPreviousRefresh, refresh) != 0);

            if (sizeChanged || densityChanged || refreshChanged) {
                nativeSurfaceChanged(mDisplayID, surface, densityDpi, refresh);

                if (sizeChanged) {
                    nativeReconfigureInputDevice(mDisplayID, w, h);
                    mPreviousWidth = w;
                    mPreviousHeight = h;
                }

                mPreviousDensityDpi = densityDpi;
                mPreviousRefresh = refresh;
            }
        }
    }
}
