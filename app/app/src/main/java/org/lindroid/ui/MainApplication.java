package org.lindroid.ui;

import static org.lindroid.ui.NativeLib.nativeInitInputDevice;

import android.app.Application;
import android.util.Log;

import com.google.android.material.color.DynamicColors;

public class MainApplication extends Application {
    @Override
    public void onCreate() {
        super.onCreate();
        if (!getFilesDir().exists() && getFilesDir().mkdir()) {
            Log.e("Lindroid", "failed to create files dir");
        }

        // Apply dynamic color
        DynamicColors.applyToActivitiesIfAvailable(this);

        // We're alive until we aren't, so this is fine to put here
        Thread composerThread = new Thread(NativeLib::nativeStartComposerService);
        composerThread.start();
        nativeInitInputDevice();
    }
}
