package org.lindroid.ui;

import android.os.IBinder;
import android.os.ParcelFileDescriptor;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.util.Log;

import java.io.File;
import java.io.IOException;
import java.util.List;

import vendor.lindroid.perspective.IPerspective;

public class ContainerManager {
    private static final String TAG = "ContainerManager";
    private static IPerspective mPerspective;

    private ContainerManager() {} // no init

    private static void getPerspectiveIfNeeded() {
        if (mPerspective != null) return;
        // Fetch the Perspective service
        final IBinder binder = ServiceManager.getService(Constants.PERSPECTIVE_SERVICE_NAME);
        if (binder == null) {
            Log.e(TAG, "Failed to get binder from ServiceManager");
            throw new RuntimeException("Failed to obtain Perspective service");
        } else {
            mPerspective = IPerspective.Stub.asInterface(binder);
        }
    }

    public static boolean isRunning(String containerName) {
        getPerspectiveIfNeeded();
        try {
            return mPerspective.isRunning(containerName);
        } catch (RemoteException e) {
            mPerspective = null;
            throw new RuntimeException(e);
        }
    }

    public static boolean isAtLeastOneRunning() {
        for (String id : listContainers()) {
            if (isRunning(id))
                return true;
        }
        return false;
    }

    public static boolean start(String containerName) {
        getPerspectiveIfNeeded();
        try {
            if (mPerspective.start(containerName)) {
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
        getPerspectiveIfNeeded();
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

    public static boolean addContainer(String containerName, File rootfsFile) {
        getPerspectiveIfNeeded();
        try (ParcelFileDescriptor pfd = ParcelFileDescriptor.open(rootfsFile,
                ParcelFileDescriptor.MODE_READ_ONLY)) {
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

    public static List<String> listContainers() {
        getPerspectiveIfNeeded();
        try {
            return mPerspective.listContainers();
        } catch (RemoteException e) {
            mPerspective = null;
            throw new RuntimeException(e);
        }
    }

    public static boolean containerExists(String containerName) {
        return listContainers().contains(containerName);
    }
}
