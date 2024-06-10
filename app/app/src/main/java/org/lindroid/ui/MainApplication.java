package org.lindroid.ui;

import android.app.Application;
import android.content.Intent;

import com.google.android.material.color.DynamicColors;

public class MainApplication extends Application {
    @Override
    public void onCreate() {
        super.onCreate();
        // Apply dynamic color
        DynamicColors.applyToActivitiesIfAvailable(this);

        if (!HardwareService.isInstanceCreated()) {
            startService(new Intent(this, HardwareService.class));
        }
    }
}
