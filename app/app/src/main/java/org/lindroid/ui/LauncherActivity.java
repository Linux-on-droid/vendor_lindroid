package org.lindroid.ui;

import android.app.Activity;
import android.app.ActivityOptions;
import android.content.Context;
import android.content.Intent;
import android.hardware.display.DisplayManager;
import android.net.Uri;
import android.os.Bundle;
import android.util.Log;
import android.view.Display;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AlertDialog;
import androidx.recyclerview.widget.RecyclerView;

import com.google.android.material.dialog.MaterialAlertDialogBuilder;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URL;
import java.util.List;
import java.util.stream.Collectors;

public class LauncherActivity extends Activity {
    private static final String TAG = "LauncherActivity";
    private static final int SAF_ROOTFS_REQUEST = 1000;
    private String mNewName;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.launcher);
        new Thread(() -> {
            String runningContainer = ContainerManager.isAtLeastOneRunning();
            if (runningContainer != null) {
                runOnUiThread(() -> startDisplayActivitiesOnAllDisplays(runningContainer));
                return;
            }
            updateAdapter();
        }).start();
    }

    private void updateAdapter() {
        List<String> containers = ContainerManager.listContainers();
        List<Boolean> running = containers.stream().map(ContainerManager::isRunning)
                .collect(Collectors.toUnmodifiableList());
        runOnUiThread(() -> {
            Log.i(TAG, "Setting adapter with container count " + containers.size());
            ((RecyclerView) findViewById(R.id.recycler)).setAdapter(new RecyclerView.Adapter<ViewHolder>() {
                @NonNull
                @Override
                public ViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
                    if (viewType == 1) {
                        return new AddViewHolder(LayoutInflater.from(LauncherActivity.this)
                                .inflate(android.R.layout.simple_list_item_1, parent, false));
                    }
                    return new ContainerViewHolder(LayoutInflater.from(LauncherActivity.this)
                            .inflate(R.layout.container_entry, parent, false));
                }

                @Override
                public void onBindViewHolder(@NonNull ViewHolder holder, int position) {
                    if (holder instanceof ContainerViewHolder cvh) {
                        cvh.mContainerName.setText(containers.get(position));
                        cvh.setRunning(running.get(position));
                        cvh.onClick = () -> {
                            // TODO start/stop
                        };
                    } else if (holder instanceof AddViewHolder avh) {
                        avh.onClick = () -> {
                            // TODO ask for name
                            String name = "default";
                            if (!ContainerManager.listContainers().contains(name)) {
                                mNewName = name;
                                /*Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
                                intent.addCategory(Intent.CATEGORY_OPENABLE);
                                intent.setType("*\/*");
                                startActivityForResult(intent, SAF_ROOTFS_REQUEST);*/
                                createContainer(name, Uri.parse("https://github.com/Linux-on-droid/lindroid-rootfs/releases/download/nightly/lindroid-rootfs-arm64-plasma.zip.tar.gz"));
                            } else {
                                // TODO duplicate warning
                            }
                        };
                    }
                }

                @Override
                public void onViewRecycled(@NonNull ViewHolder holder) {
                    if (holder instanceof ContainerViewHolder cvh) {
                        cvh.onClick = null;
                    } else if (holder instanceof AddViewHolder avh) {
                        avh.onClick = null;
                    }
                }

                @Override
                public int getItemCount() {
                    return containers.size() + 1;
                }

                @Override
                public int getItemViewType(int position) {
                    return position == getItemCount() - 1 ? 1 : 0;
                }
            });
        });
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (requestCode == SAF_ROOTFS_REQUEST) {
            if (resultCode == Activity.RESULT_OK && data != null && data.getData() != null) {
                createContainer(mNewName, data.getData());
            } else {
                Toast.makeText(this, R.string.saf_pick_failed, Toast.LENGTH_LONG).show();
            }
            mNewName = null;
            return;
        }
        super.onActivityResult(requestCode, resultCode, data);
    }

    private void createContainer(String containerName, Uri rootfs) {
        if ("http".equals(rootfs.getScheme()) || "https".equals(rootfs.getScheme())) {
            View v = LayoutInflater.from(this).inflate(R.layout.progressdialog, null);
            final TextView tv = v.findViewById(R.id.prog_message);
            tv.setText(R.string.dl_connecting);
            AlertDialog inner = new MaterialAlertDialogBuilder(this)
                    .setCancelable(false)
                    .setTitle(R.string.downloading)
                    .setView(v)
                    .show();
            new Thread(() -> {
                File f = new File(getCacheDir(), String.valueOf(System.currentTimeMillis()));
                try {
                    HttpURLConnection conn = (HttpURLConnection) new URL(rootfs.toString()).openConnection();
                    final long contentLength = conn.getContentLengthLong();
                    long readBytes = 0;
                    long lastReportedReadBytes = 0;
                    final byte[] b = new byte[1024 * 1024];
                    try (InputStream s = conn.getInputStream()) {
                        try (OutputStream s2 = new FileOutputStream(f)) {
                            int r;
                            while ((r = s.read(b)) != -1) {
                                s2.write(b, 0, r);
                                if (r >= 0) readBytes += r;
                                if (lastReportedReadBytes < readBytes - 1024 * 1024) {
                                    lastReportedReadBytes = readBytes;
                                    final long rb = readBytes;
                                    runOnUiThread(() -> tv.setText(getString(R.string.progress,
                                            rb / (1024 * 1024),
                                            contentLength / (1024 * 1024))));
                                }
                            }
                        }
                    }
                } catch (IOException e) {
                    Log.e(TAG, Log.getStackTraceString(e));
                    if (f.exists() && !f.delete())
                        Log.e(TAG, "failed to delete temp file");
                    runOnUiThread(() ->
                        Toast.makeText(this, R.string.download_failed, Toast.LENGTH_LONG).show()
                    );
                    return;
                } finally {
                    runOnUiThread(inner::dismiss);
                }
                runOnUiThread(() -> createContainer(containerName, Uri.fromFile(f)));
            });
            return;
        }
        View v = LayoutInflater.from(this).inflate(R.layout.progressdialog, null);
        ((TextView) v.findViewById(R.id.prog_message)).setText(R.string.creating_container_message);
        AlertDialog inner = new MaterialAlertDialogBuilder(this)
                .setCancelable(false)
                .setTitle(R.string.creating_container_title)
                .setView(v)
                .show();
        new Thread(() -> {
            if (ContainerManager.addContainer(containerName, getContentResolver(), rootfs)) {
                startOrError(containerName, () -> {
                    updateAdapter();
                    inner.dismiss();
                });
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

    private void startOrError(String containerName, Runnable next) {
        new Thread(() -> {
            if (!ContainerManager.start(containerName)) {
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

    private static abstract class ViewHolder extends RecyclerView.ViewHolder {
        public ViewHolder(@NonNull View itemView) {
            super(itemView);
        }
    }

    private static class ContainerViewHolder extends ViewHolder {

        public final TextView mContainerName;
        public final TextView mRunningIndicator;
        public Runnable onClick; // TODO hook it up

        public ContainerViewHolder(View root) {
            super(root);
            mContainerName = root.findViewById(R.id.containerName);
            mRunningIndicator = root.findViewById(R.id.runningIndicator);
        }

        public void setRunning(boolean running) {
            mRunningIndicator.setText(itemView.getContext().getString(running ? R.string.running :
                    R.string.not_running));
        }
    }

    private static class AddViewHolder extends ViewHolder implements View.OnClickListener {

        public Runnable onClick;

        public AddViewHolder(View root) {
            super(root);
            root.setOnClickListener(this);
            ((TextView) root).setText(R.string.add_container);
        }

        @Override
        public void onClick(View v) {
            if (onClick != null) {
                onClick.run();
            }
        }
    }
}
