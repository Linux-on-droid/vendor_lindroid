package org.lindroid.ui;

import android.app.Activity;
import android.app.ActivityOptions;
import android.content.Context;
import android.content.Intent;
import android.hardware.display.DisplayManager;
import android.os.Bundle;
import android.view.Display;
import android.util.Log;

public class LauncherActivity extends Activity {
    private static final String TAG = "LauncherActivity";
    private AudioSocketServer audioSocketServer;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        audioSocketServer = new AudioSocketServer();
        audioSocketServer.startServer();

        startDisplayActivitiesOnAllDisplays();
        finish();
    }

    private void startDisplayActivitiesOnAllDisplays() {
        DisplayManager displayManager = (DisplayManager) getSystemService(Context.DISPLAY_SERVICE);
        Display[] displays = displayManager.getDisplays();

        for (Display display : displays) {
            long displayId = display.getDisplayId();
            Log.d(TAG, "Starting DisplayActivity on display: " + displayId);
            launchNewDisplayActivity(displayId, "default");
        }
    }

    private void launchNewDisplayActivity(long displayId, String containerName) {
        Intent intent = new Intent(this, DisplayActivity.class);
        intent.putExtra("displayID", displayId);
        intent.putExtra("containerName", containerName);
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_MULTIPLE_TASK | Intent.FLAG_ACTIVITY_NEW_DOCUMENT);
        ActivityOptions options = ActivityOptions.makeBasic();
        options.setLaunchDisplayId((int)displayId);

        startActivity(intent, options.toBundle());
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (audioSocketServer != null) {
            audioSocketServer.stopServer();
        }
    }
}
