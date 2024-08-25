package org.lindroid.ui;

import android.app.Activity;
import android.app.ActivityOptions;
import android.content.Context;
import android.content.Intent;
import android.hardware.display.DisplayManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.view.Display;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.TextView;

import androidx.appcompat.app.AlertDialog;

import com.google.android.material.dialog.MaterialAlertDialogBuilder;

import java.io.File;

public class LauncherActivity extends Activity {
    private static final String TAG = "LauncherActivity";
    private static final String DEFAULT_CONTAINER_NAME = "default";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        boolean containerExists = ContainerManager.containerExists(DEFAULT_CONTAINER_NAME);
        boolean isRunning = containerExists && ContainerManager.isRunning(DEFAULT_CONTAINER_NAME);

        if (isRunning) {
            startDisplayActivitiesOnAllDisplays(DEFAULT_CONTAINER_NAME);
            return;
        }
        setContentView(R.layout.launcher);
    }

    private void showCreateContainerDialog() {
        runOnUiThread(() ->
                new MaterialAlertDialogBuilder(LauncherActivity.this)
                        .setTitle(R.string.no_container_title)
                        .setMessage(R.string.no_container_message)
                        .setPositiveButton(android.R.string.ok, (dialog, which) -> {
                            createContainer();
                            startDisplayActivitiesOnAllDisplays(DEFAULT_CONTAINER_NAME);
                        })
                        .setNegativeButton(android.R.string.cancel, (dialog, which) -> finish())
                        .setIcon(R.drawable.ic_help)
                        .show()
        );
    }

    private void showStartContainerDialog() {
        runOnUiThread(() -> new MaterialAlertDialogBuilder(LauncherActivity.this)
                .setTitle(R.string.not_running_title)
                .setMessage(R.string.not_running_message)
                .setPositiveButton(android.R.string.ok, (dialog, which) -> {
                    startOrError(() -> startDisplayActivitiesOnAllDisplays(DEFAULT_CONTAINER_NAME));
                })
                .setNegativeButton(android.R.string.cancel, (dialog, which) -> finish())
                .setIcon(R.drawable.ic_help)
                .show());
    }

    private void createContainer() {
        View v = LayoutInflater.from(this).inflate(R.layout.progressdialog, null);
        ((TextView) v.findViewById(R.id.prog_message)).setText(R.string.creating_container_message);
        AlertDialog inner = new MaterialAlertDialogBuilder(this)
                .setCancelable(false)
                .setTitle(R.string.creating_container_title)
                .setView(v)
                .show();
        new Thread(() -> {
            if (ContainerManager.addContainer(DEFAULT_CONTAINER_NAME,
                    new File(getFilesDir(), "rootfs.tar.gz"))) {
                startOrError(inner::dismiss);
            } else {
                runOnUiThread(() -> new MaterialAlertDialogBuilder(LauncherActivity.this)
                        .setTitle(R.string.failed_to_create)
                        .setMessage(R.string.failed_to_create_msg)
                        .setPositiveButton(android.R.string.ok, (dialog, which) -> {})
                        .setCancelable(false)
                        .setIcon(R.drawable.ic_warning)
                        .show());
            }
        }).start();
    }

    private void startOrError(Runnable next) {
        new Thread(() -> {
            if (!ContainerManager.start(DEFAULT_CONTAINER_NAME)) {
                runOnUiThread(next);
            } else {
                runOnUiThread(() -> new MaterialAlertDialogBuilder(LauncherActivity.this)
                        .setTitle(R.string.failed_to_start)
                        .setMessage(R.string.failed_to_start_msg)
                        .setPositiveButton(android.R.string.ok, (dialog, which) -> {})
                        .setCancelable(false)
                        .setIcon(R.drawable.ic_warning)
                        .show());
            }
        }).start();
    }

    private void startDisplayActivitiesOnAllDisplays(String containerName) {
        DisplayManager displayManager = (DisplayManager) getSystemService(Context.DISPLAY_SERVICE);
        Display[] displays = displayManager.getDisplays();
        // int currentId = Objects.requireNonNull(getDisplay()).getDisplayId();

        // TODO don't do this on foldable's other internal displays
        for (Display display : displays) {
            if ((display.getFlags() & Display.FLAG_PRIVATE) != 0)
                continue;
            int displayId = display.getDisplayId();
            Log.d(TAG, "Starting DisplayActivity on display: " + displayId);
            Intent intent = new Intent(this, DisplayActivity.class);
            intent.putExtra("displayID", displayId);
            intent.putExtra("containerName", containerName);
            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_MULTIPLE_TASK | Intent.FLAG_ACTIVITY_NEW_DOCUMENT);
            ActivityOptions options = ActivityOptions.makeBasic();
            options.setLaunchDisplayId(displayId);

            startActivity(intent, options.toBundle());
        }
        finish();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
    }
}
