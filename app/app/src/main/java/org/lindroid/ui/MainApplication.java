package org.lindroid.ui;

import android.app.Application;
import android.content.Intent;
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

        if (!HardwareService.isInstanceCreated()) {
            startService(new Intent(this, HardwareService.class));
        }
    }
}
