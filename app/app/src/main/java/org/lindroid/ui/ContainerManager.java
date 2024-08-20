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
    private static final String PERSPECTIVE_SERVICE_NAME = "perspective";
    private IPerspective mPerspective;

    public ContainerManager() {
        // Fetch the Perspective service
        final IBinder binder = ServiceManager.getService(PERSPECTIVE_SERVICE_NAME);
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

    public void start(String containerName) {
        try {
            mPerspective.start(containerName);
            Log.d(TAG, "Container " + containerName + " started successfully.");
        } catch (RemoteException e) {
            Log.e(TAG, "RemoteException in start", e);
        }
    }

    public void stop(String containerName) {
        try {
            mPerspective.stop(containerName);
            Log.d(TAG, "Container " + containerName + " stopped successfully.");
        } catch (RemoteException e) {
            Log.e(TAG, "RemoteException in stop", e);
        }
    }

    public void addContainer(String containerName, File rootfsFile) {
        try {
            ParcelFileDescriptor pfd = ParcelFileDescriptor.open(rootfsFile, ParcelFileDescriptor.MODE_READ_ONLY);
            mPerspective.addContainer(containerName, pfd);
            Log.d(TAG, "Container " + containerName + " added successfully.");
        } catch (RemoteException e) {
            Log.e(TAG, "RemoteException in addContainer", e);
        } catch (FileNotFoundException e) {
            Log.e(TAG, "FileNotFoundException in addContainer", e);
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

    public boolean containerExists(String containerName) {
        try {
            List<String> containers = listContainers();
            return containers != null && containers.contains(containerName);
        } catch (Exception e) {
            Log.e(TAG, "Exception in containerExists", e);
            return false;
        }
    }
}
