package org.lindroid.ui;

import android.os.IBinder;
import android.os.ParcelFileDescriptor;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.util.Log;
import java.io.File;
import java.io.FileNotFoundException;
import java.util.List;

import vendor.lindroid.perspective.IPerspective;

public class ContainerManager {
    private static final String TAG = "ContainerManager";
    private final IPerspective mPerspective;

    public ContainerManager() {
        // Fetch the Perspective service
        final IBinder binder = ServiceManager.getService(Constants.PERSPECTIVE_SERVICE_NAME);
        if (binder == null) {
            Log.e(TAG, "Failed to get binder from ServiceManager");
            throw new RuntimeException("Failed to obtain Perspective service");
        } else {
            mPerspective = IPerspective.Stub.asInterface(binder);
        }
    }

    public boolean isRunning(String containerName) {
        try {
            return mPerspective.isRunning(containerName);
        } catch (RemoteException e) {
            Log.e(TAG, "RemoteException in isRunning", e);
            return false;
        }
    }

    public boolean start(String containerName) {
        try {
            if (mPerspective.start(containerName)) {
                Log.d(TAG, "Container " + containerName + " started successfully.");
                return true;
            } else {
                Log.e(TAG, "Container " + containerName + " failed to start.");
                return false;
            }
        } catch (RemoteException e) {
            Log.e(TAG, "RemoteException in start", e);
            return false;
        }
    }

    public boolean stop(String containerName) {
        try {
            if (mPerspective.stop(containerName)) {
                Log.d(TAG, "Container " + containerName + " stopped successfully.");
                return true;
            } else {
                Log.e(TAG, "Container " + containerName + " failed to stop.");
                return false;
            }
        } catch (RemoteException e) {
            Log.e(TAG, "RemoteException in stop", e);
            return false;
        }
    }

    public boolean addContainer(String containerName, File rootfsFile) {
        try {
            ParcelFileDescriptor pfd = ParcelFileDescriptor.open(rootfsFile,
                    ParcelFileDescriptor.MODE_READ_ONLY);
            if (mPerspective.addContainer(containerName, pfd)) {
                Log.d(TAG, "Container " + containerName + " added successfully.");
                return true;
            } else {
                Log.e(TAG, "Container " + containerName + " failed to be added.");
                return false;
            }
        } catch (RemoteException e) {
            Log.e(TAG, "RemoteException in addContainer", e);
            return false;
        } catch (FileNotFoundException e) {
            Log.e(TAG, "FileNotFoundException in addContainer", e);
            return false;
        }
    }

    public List<String> listContainers() {
        try {
            return mPerspective.listContainers();
        } catch (RemoteException e) {
            Log.e(TAG, "RemoteException in listContainers", e);
            return null;
        }
    }

    public Boolean containerExists(String containerName) {
        try {
            List<String> containers = listContainers();
            if (containers == null) return null;
            return containers.contains(containerName);
        } catch (Exception e) {
            Log.e(TAG, "Exception in containerExists", e);
            return null;
        }
    }
}
