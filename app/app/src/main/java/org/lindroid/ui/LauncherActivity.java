package org.lindroid.ui;

import androidx.appcompat.app.AlertDialog;

import android.os.Handler;
import android.os.Looper;
import android.app.Activity;
import android.app.ActivityOptions;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.hardware.display.DisplayManager;
import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.Display;
import android.view.View;
import android.widget.TextView;

import com.google.android.material.dialog.MaterialAlertDialogBuilder;

import java.io.File;
import java.io.FileNotFoundException;

public class LauncherActivity extends Activity {
    private static final String TAG = "LauncherActivity";
    private AudioSocketServer audioSocketServer;
    private ContainerManager containerManager;
    private static final String DEFAULT_CONTAINER_NAME = "default";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        containerManager = new ContainerManager();

        audioSocketServer = new AudioSocketServer();
        audioSocketServer.startServer();

        checkContainerAndStartDisplayActivities();
    }

    private void checkContainerAndStartDisplayActivities() {
        boolean containerExists = containerManager.containerExists(DEFAULT_CONTAINER_NAME);
        boolean isRunning = containerExists && containerManager.isRunning(DEFAULT_CONTAINER_NAME);

        if (!containerExists) {
            showCreateContainerDialog();
        } else if (!isRunning) {
            showStartContainerDialog();
        } else {
            startDisplayActivitiesOnAllDisplays();
        }
    }

    private void showCreateContainerDialog() {
        runOnUiThread(() -> {
            new MaterialAlertDialogBuilder(LauncherActivity.this)
                    .setTitle(R.string.no_container_title)
                    .setMessage(R.string.no_container_message)
                    .setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
                        public void onClick(DialogInterface dialog, int which) {
                            createContainer();
                            dialog.dismiss();
                            startDisplayActivitiesOnAllDisplays();
                        }
                    })
                    .setNegativeButton(android.R.string.cancel, new DialogInterface.OnClickListener() {
                        public void onClick(DialogInterface dialog, int which) {
                            dialog.dismiss();
                            finish();
                        }
                    })
                    .setIcon(R.drawable.ic_help)
                    .show();
        });
    }

    private void showStartContainerDialog() {
        runOnUiThread(() -> {
            new MaterialAlertDialogBuilder(LauncherActivity.this)
                    .setTitle(R.string.not_running_title)
                    .setMessage(R.string.not_running_message)
                    .setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
                        public void onClick(DialogInterface dialog, int which) {
                            startContainer();
                            dialog.dismiss();
                            finish();
                            startDisplayActivitiesOnAllDisplays();
                        }
                    })
                    .setNegativeButton(android.R.string.cancel, new DialogInterface.OnClickListener() {
                        public void onClick(DialogInterface dialog, int which) {
                            dialog.dismiss();
                            finish();
                        }
                    })
                    .setIcon(R.drawable.ic_help)
                    .show();
        });
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
            containerManager.addContainer(DEFAULT_CONTAINER_NAME, new File(getFilesDir(), "rootfs.tar.gz"));
            containerManager.start(DEFAULT_CONTAINER_NAME);
            new Handler(Looper.getMainLooper()).post(inner::dismiss);
        }).start();
    }

    private void startContainer() {
        containerManager.start(DEFAULT_CONTAINER_NAME);
    }

    private void startDisplayActivitiesOnAllDisplays() {
        DisplayManager displayManager = (DisplayManager) getSystemService(Context.DISPLAY_SERVICE);
        Display[] displays = displayManager.getDisplays();

        for (Display display : displays) {
            long displayId = display.getDisplayId();
            Log.d(TAG, "Starting DisplayActivity on display: " + displayId);
            launchNewDisplayActivity(displayId, DEFAULT_CONTAINER_NAME);
        }
    }

    private void launchNewDisplayActivity(long displayId, String containerName) {
        Intent intent = new Intent(this, DisplayActivity.class);
        intent.putExtra("displayID", displayId);
        intent.putExtra("containerName", containerName);
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_MULTIPLE_TASK | Intent.FLAG_ACTIVITY_NEW_DOCUMENT);
        ActivityOptions options = ActivityOptions.makeBasic();
        options.setLaunchDisplayId((int) displayId);

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
