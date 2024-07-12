package org.lindroid.ui;

import static org.lindroid.ui.Constants.PERSPECTIVE_SERVICE_NAME;
import static org.lindroid.ui.NativeLib.*;

import com.google.android.material.dialog.MaterialAlertDialogBuilder;
import androidx.annotation.NonNull;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;

import android.os.Handler;
import android.os.Looper;
import android.os.ParcelFileDescriptor;
import android.view.Display;
import android.view.InputDevice;
import android.os.Bundle;
import android.os.RemoteException;
import android.util.Log;
import android.os.IBinder;
import android.os.ServiceManager;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.PointerIcon;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.WindowInsets;
import android.view.WindowInsetsController;
import android.widget.TextView;

import java.io.File;
import java.io.FileNotFoundException;
import java.util.List;

import vendor.lindroid.perspective.IPerspective;

public class DisplayActivity extends AppCompatActivity implements SurfaceHolder.Callback, View.OnTouchListener, View.OnHoverListener, View.OnGenericMotionListener {
    private static final String TAG = "Lindroid";
    private String mContainerName = "default";
    private long mDisplayID = 0;
    private IPerspective mPerspective;
    private int mPreviousWidth = 0;
    private int mPreviousHeight = 0;
    private static Handler mHandler;
    private Runnable mSurfaceRunnable;


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.display_activity);
        final WindowInsetsController controller = getWindow().getInsetsController();
        if (controller != null) {
            controller.hide(WindowInsets.Type.statusBars() | WindowInsets.Type.navigationBars());
            controller.setSystemBarsBehavior(WindowInsetsController.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);
        }
    }

    public void start(long displayID, String containerName) {
        mDisplayID = displayID;
        mContainerName = containerName;

        if (mHandler == null)
            mHandler = new Handler(Looper.getMainLooper());
        SurfaceView sv = findViewById(R.id.surfaceView);
        SurfaceHolder sh = sv.getHolder();
        sv.setOnTouchListener(this);
        sv.setOnHoverListener(this);
        sv.setOnGenericMotionListener(this);
        sh.addCallback(this);

        // Hide pointer icon
        sv.setPointerIcon(PointerIcon.getSystemIcon(this, PointerIcon.TYPE_NULL));

        // Get perspective service
        final IBinder binder = ServiceManager.getService(PERSPECTIVE_SERVICE_NAME);
        if (binder == null) {
            Log.e(TAG, "Failed to get binder from ServiceManager");
            new MaterialAlertDialogBuilder(this)
                    .setTitle(R.string.unsupported_title)
                    .setMessage(R.string.unsupported_message)
                    .setPositiveButton(android.R.string.ok, (dialog, which) -> {
                        dialog.dismiss();
                        finish();
                    })
                    .setIcon(R.drawable.ic_warning)
                    .show();
            return;
        } else {
            mPerspective = IPerspective.Stub.asInterface(binder);
        }

        List<String> containers;
        try {
            containers = mPerspective.listContainers();
        } catch (RemoteException e) {
            Log.e(TAG, "RemoteException in listContainers", e);
            return;
        }
        if (containers.isEmpty()) {
            Log.d(TAG, "Found no containers");
            new MaterialAlertDialogBuilder(this)
                    .setTitle(R.string.no_container_title)
                    .setMessage(R.string.no_container_message)
                    .setPositiveButton(android.R.string.ok, (dialog, which) -> {
                        View v = LayoutInflater.from(this).inflate(R.layout.progressdialog, null);
                        ((TextView) v.findViewById(R.id.prog_message)).setText(R.string.creating_container_message);
                        AlertDialog inner = new MaterialAlertDialogBuilder(this)
                                .setCancelable(false)
                                .setTitle(R.string.creating_container_title)
                                .setView(v)
                                .show();
                        new Thread(() -> {
                            try {
                                mPerspective.addContainer(mContainerName,
                                        ParcelFileDescriptor.open(
                                                new File(getFilesDir(), "rootfs.tar.gz"),
                                                ParcelFileDescriptor.MODE_READ_ONLY));
                                mPerspective.start(mContainerName);
                            } catch (RemoteException | FileNotFoundException e) {
                                Log.e(TAG, "Exception in add", e);
                            }
                            new Handler(Looper.getMainLooper()).post(inner::dismiss);
                        }).start();
                    })
                    .setNegativeButton(android.R.string.cancel, (dialog, which) -> {
                        dialog.dismiss();
                        finish();
                    })
                    .setIcon(R.drawable.ic_help)
                    .show();
        } else {
            for (String name : containers) {
                Log.d(TAG, "Found container " + name);
            }

            // Check if container is running
            try {
                if(!mPerspective.isRunning(mContainerName)) {
                    new MaterialAlertDialogBuilder(this)
                            .setTitle(R.string.not_running_title)
                            .setMessage(R.string.not_running_message)
                            .setPositiveButton(android.R.string.ok, (dialog, which) -> {
                                try {
                                    mPerspective.start(mContainerName);
                                } catch (RemoteException e) {
                                    Log.e(TAG, "RemoteException in start", e);
                                }
                            })
                            .setNegativeButton(android.R.string.cancel, (dialog, which) -> {
                                dialog.dismiss();
                                finish();
                            })
                            .setIcon(R.drawable.ic_help)
                            .show();
                }
            } catch (RemoteException e) {
                Log.e(TAG, "RemoteException in isRunning", e);
            }
        }
    }

    @Override
    public void onBackPressed() {
        try {
            if(mPerspective.isRunning(mContainerName)) {
                new MaterialAlertDialogBuilder(this)
                        .setTitle(R.string.stop_title)
                        .setMessage(R.string.stop_message)
                        .setPositiveButton(R.string.yes, (dialog, which) -> {
                            try {
                                mPerspective.stop(mContainerName);
                            } catch (RemoteException e) {
                                Log.e(TAG, "RemoteException in start", e);
                            }
                            dialog.dismiss();
                            finish();
                        })
                        .setNeutralButton(android.R.string.cancel, (dialog, which) -> {
                            dialog.dismiss();
                        })
                        .setNegativeButton(R.string.no, (dialog, which) -> {
                            super.onBackPressed();
                        })
                        .show();
            } else {
                finish();
            }
        } catch (RemoteException e) {
            Log.e(TAG, "RemoteException in isRunning", e);
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        // Lets never destroy primary display
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

    @Override
    public boolean onTouch(View view, MotionEvent motionEvent) {
        if(motionEvent.getSource() == InputDevice.SOURCE_MOUSE) {
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
        mHandler.removeCallbacksAndMessages(null);

        mSurfaceRunnable = () -> {
            applySurfaceChanges(holder, format, w, h);
        };

        mHandler.postDelayed(mSurfaceRunnable, 200);
    }

    private void applySurfaceChanges(@NonNull SurfaceHolder holder, int format, int w, int h) {
        Surface surface = holder.getSurface();
        if (surface != null) {
            float refresh = 60.0f;
            try {
                refresh = getDisplay().getRefreshRate();
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
