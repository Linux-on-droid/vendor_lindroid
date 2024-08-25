package org.lindroid.ui;

import android.annotation.SuppressLint;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.net.SocketAddress;

public class Constants {
    public static final String PERSPECTIVE_SERVICE_NAME = "perspective";
    public static final String SOCKET_PATH = "/data/lindroid/mnt/audio_socket";
    public static final String NOTIFICATION_CHANNEL_ID = "service";
    public static final int NOTIFICATION_ID = 1;

    @SuppressLint({ "PrivateApi", "BlockedPrivateApi" })
    public static SocketAddress createUnixSocketAddressObj(String path) {
        try {
            Class<?> myClass = Class.forName("android.system.UnixSocketAddress");
            Method method = myClass.getDeclaredMethod("createFileSystem", String.class);
            return (SocketAddress) method.invoke(null, path);
        } catch (NoSuchMethodException | ClassNotFoundException | IllegalAccessException |
                 InvocationTargetException e) {
            throw new RuntimeException(e);
        }
    }
}
