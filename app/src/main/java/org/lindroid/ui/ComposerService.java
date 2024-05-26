package org.lindroid.ui;

import android.app.Service;
import android.content.Intent;
import android.os.IBinder;

import androidx.annotation.Nullable;

public class ComposerService extends Service {

    private static ComposerService instance = null;

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        super.onStartCommand(intent, flags, startId);
        instance = this;
        Thread thread = new Thread(NativeLib::nativeStartComposerService);
        thread.start();

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
