package org.lindroid.ui;

import android.content.ContentResolver;
import android.net.Uri;
import android.os.IBinder;
import android.os.ParcelFileDescriptor;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.util.Log;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.CopyOnWriteArrayList;

import vendor.lindroid.perspective.IPerspective;

public class ContainerManager {
    private static final String TAG = "ContainerManager";
    private static IPerspective mPerspective;

    private static final Map<String, StringBuilder> logBuffers = new ConcurrentHashMap<>();
    private static final Map<String, Thread> logThreads = new ConcurrentHashMap<>();
    private static final Map<String, List<LogUpdateListener>> listeners = new ConcurrentHashMap<>();
    private static final Map<String, Boolean> runningContainers = new ConcurrentHashMap<>();

    public interface LogUpdateListener {
        void onLogUpdated(String containerName, String latestLog);
    }

    private ContainerManager() {} // no init

    public static boolean isPerspectiveAvailable() {
        if (mPerspective != null) {
            //if (mPerspective.isBinderAlive()) TODO
                return true;
            //mPerspective = null;
        }
        // Fetch the Perspective service
        final IBinder binder = ServiceManager.getService(Constants.PERSPECTIVE_SERVICE_NAME);
        if (binder == null) {
            Log.e(TAG, "Failed to get binder from ServiceManager");
            return false;
        }
        mPerspective = IPerspective.Stub.asInterface(binder);
        return true;
    }

    public static void addLogUpdateListener(String containerName, LogUpdateListener listener) {
        listeners.computeIfAbsent(containerName, k -> new CopyOnWriteArrayList<>()).add(listener);
    }

    public static void removeLogUpdateListener(String containerName, LogUpdateListener listener) {
        List<LogUpdateListener> containerListeners = listeners.get(containerName);
        if (containerListeners != null) {
            containerListeners.remove(listener);
        }
    }

    public static boolean startFetchingLogs(String containerName) {
        if (logThreads.containsKey(containerName)) {
            Log.w(TAG, "Log fetching already running for container: " + containerName);
            return false;
        }

        runningContainers.put(containerName, true);

        logBuffers.putIfAbsent(containerName, new StringBuilder());
        Thread logThread = new Thread(() -> {
            while (Boolean.TRUE.equals(runningContainers.get(containerName))) {
                try {
                    String logs = mPerspective.fetchLogs(containerName);

                    if (logs != null && !logs.trim().isEmpty()) {
                        // Append logs to buffer
                        synchronized (logBuffers.get(containerName)) {
                            logBuffers.get(containerName).append(logs).append("\n");
                        }

                        // Notify listeners
                        List<LogUpdateListener> containerListeners = listeners.get(containerName);
                        if (containerListeners != null) {
                            for (LogUpdateListener listener : containerListeners) {
                                listener.onLogUpdated(containerName, logs);
                            }
                        }
                    }

                    Thread.sleep(100);

                } catch (RemoteException | InterruptedException e) {
                    Log.e(TAG, "Error fetching logs for container: " + containerName, e);
                }
            }
            logThreads.remove(containerName);
        });
        logThreads.put(containerName, logThread);
        logThread.start();
        return true;
    }

    public static void stopFetchingLogs(String containerName) {
        if (!logThreads.containsKey(containerName)) {
            Log.w(TAG, "Log fetching is not running for container: " + containerName);
            return;
        }

        runningContainers.put(containerName, false);
        Thread logThread = logThreads.get(containerName);
        if (logThread != null) {
            logThread.interrupt();
            try {
                logThread.join();
            } catch (InterruptedException e) {
                Log.e(TAG, "Error stopping log thread for container: " + containerName, e);
            }
        }
        logThreads.remove(containerName);
    }

    public static String getBufferedLogs(String containerName) {
        StringBuilder buffer = logBuffers.get(containerName);
        if (buffer == null) return "";
        synchronized (buffer) {
            return buffer.toString();
        }
    }

    public static void clearLogBuffer(String containerName) {
        StringBuilder buffer = logBuffers.get(containerName);
        if (buffer != null) {
            synchronized (buffer) {
                buffer.setLength(0);
            }
        }
    }

    public static boolean isRunning(String containerName) {
        if (!isPerspectiveAvailable())
            return false;
        try {
            return mPerspective.isRunning(containerName);
        } catch (RemoteException e) {
            mPerspective = null;
            throw new RuntimeException(e);
        }
    }

    public static String isAtLeastOneRunning() {
        for (String id : listContainers()) {
            if (isRunning(id))
                return id;
        }
        return null;
    }

    public static boolean start(String containerName) {
        if (!isPerspectiveAvailable())
            return false;
        try {
            if (mPerspective.start(containerName, true)) {
                Log.d(TAG, "Container " + containerName + " started successfully.");
                return true;
            } else {
                Log.e(TAG, "Container " + containerName + " failed to start.");
                return false;
            }
        } catch (RemoteException e) {
            mPerspective = null;
            throw new RuntimeException(e);
        }
    }

    public static boolean stop(String containerName) {
        if (!isPerspectiveAvailable())
            return false;
        try {
            if (mPerspective.stop(containerName)) {
                Log.d(TAG, "Container " + containerName + " stopped successfully.");
                return true;
            } else {
                Log.e(TAG, "Container " + containerName + " failed to stop.");
                return false;
            }
        } catch (RemoteException e) {
            mPerspective = null;
            throw new RuntimeException(e);
        }
    }

    public static String getLog(String containerName) {
    Log.d(TAG, "getLog called for container: " + containerName);
        String logs="";
        try {
            logs = mPerspective.fetchLogs("default");
            Log.d(TAG, "Container Logs: " + logs);
        } catch (RemoteException e) {
            e.printStackTrace(); // Handle the exception or log it
            Log.d(TAG, "Failed to fetch logs: " + e.getMessage());
        }
        return logs;
    }

    public static boolean addContainer(String containerName, ContentResolver cr, Uri rootfs) {
        if (!isPerspectiveAvailable())
            return false;
        try (ParcelFileDescriptor pfd = cr.openFileDescriptor(rootfs, "r")) {
            if (mPerspective.addContainer(containerName, pfd)) {
                Log.d(TAG, "Container " + containerName + " added successfully.");
                return true;
            } else {
                Log.e(TAG, "Container " + containerName + " failed to be added.");
                return false;
            }
        } catch (RemoteException e) {
            mPerspective = null;
            throw new RuntimeException(e);
        } catch (IOException e) {
            Log.e(TAG, "IOException in addContainer", e);
            return false;
        }
    }

    public static boolean deleteContainer(String containerName) {
        if (!isPerspectiveAvailable())
            return false;
        try {
            if (mPerspective.deleteContainer(containerName)) {
                Log.d(TAG, "Container " + containerName + " deleted successfully.");
                return true;
            } else {
                Log.e(TAG, "Container " + containerName + " failed to be deleted.");
                return false;
            }
        } catch (RemoteException e) {
            mPerspective = null;
            throw new RuntimeException(e);
        }
    }

    public static List<String> listContainers() {
        if (!isPerspectiveAvailable())
            return new ArrayList<>();
        try {
            return mPerspective.listContainers();
        } catch (RemoteException e) {
            mPerspective = null;
            throw new RuntimeException(e);
        }
    }
}
