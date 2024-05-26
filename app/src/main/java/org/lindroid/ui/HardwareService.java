package org.lindroid.ui;

import static org.lindroid.ui.NativeLib.nativeInitInputDevice;

import android.app.Service;
import android.content.Intent;
import android.os.IBinder;

import androidx.annotation.Nullable;

public class HardwareService extends Service {

    private static HardwareService instance = null;

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        super.onStartCommand(intent, flags, startId);
        instance = this;

        Thread composerThread = new Thread(NativeLib::nativeStartComposerService);
        composerThread.start();
        nativeInitInputDevice();

        return START_STICKY;
    }

    @Nullable
    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    public static boolean isInstanceCreated() {
        return instance != null;
    }

    @Override
    public void onDestroy() {
        instance = null;
    }
}
