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

import androidx.annotation.NonNull;
import androidx.appcompat.app.AlertDialog;
import androidx.recyclerview.widget.RecyclerView;

import com.google.android.material.dialog.MaterialAlertDialogBuilder;

import java.io.File;
import java.util.List;
import java.util.stream.Collectors;

public class LauncherActivity extends Activity {
    private static final String TAG = "LauncherActivity";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        new Thread(() -> {
            String runningContainer = ContainerManager.isAtLeastOneRunning();
            if (runningContainer != null) {
                runOnUiThread(() -> startDisplayActivitiesOnAllDisplays(runningContainer));
                return;
            }
            List<String> containers = ContainerManager.listContainers();
            List<Boolean> running = containers.stream().map(ContainerManager::isRunning)
                    .collect(Collectors.toUnmodifiableList());
            runOnUiThread(() -> {
                setContentView(R.layout.launcher);
                ((RecyclerView) findViewById(R.id.recycler)).setAdapter(new RecyclerView.Adapter<ViewHolder>() {
                    @NonNull
                    @Override
                    public ViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
                        if (viewType == 1) {
                            return new AddViewHolder(LayoutInflater.from(LauncherActivity.this)
                                    .inflate(R.layout.add_entry, parent, false));
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
                                // TODO download/pick file
                                if (!ContainerManager.listContainers().contains("default"))
                                    createContainer("default",
                                        Uri.fromFile(new File(getFilesDir(), "rootfs.tar.gz")));
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
                        return containers.size();
                    }

                    @Override
                    public int getItemViewType(int position) {
                        return position == getItemCount() - 1 ? 1 : 0;
                    }
                });
            });
        });
    }

    private void createContainer(String containerName, Uri rootfs) {
        View v = LayoutInflater.from(this).inflate(R.layout.progressdialog, null);
        ((TextView) v.findViewById(R.id.prog_message)).setText(R.string.creating_container_message);
        AlertDialog inner = new MaterialAlertDialogBuilder(this)
                .setCancelable(false)
                .setTitle(R.string.creating_container_title)
                .setView(v)
                .show();
        new Thread(() -> {
            if (ContainerManager.addContainer(containerName, getContentResolver(), rootfs)) {
                startOrError(containerName, inner::dismiss);
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
        }

        @Override
        public void onClick(View v) {
            if (onClick != null) {
                onClick.run();
            }
        }
    }
}
