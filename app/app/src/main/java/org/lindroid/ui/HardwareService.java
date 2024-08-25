package org.lindroid.ui;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.Service;
import android.content.Intent;
import android.os.Handler;
import android.os.IBinder;

import androidx.annotation.Nullable;

public class HardwareService extends Service {

    private static HardwareService instance = null;
    private final Handler handler = new Handler(getMainLooper());
    private AudioSocketServer audioSocketServer;
    private boolean started = false;

    public static boolean isInstanceCreated() {
        return instance != null;
    }

    @Override
    public void onCreate() {
        super.onCreate();
        NotificationManager nm = getSystemService(NotificationManager.class);
        NotificationChannel c = new NotificationChannel(Constants.NOTIFICATION_CHANNEL_ID,
                getString(R.string.notification_channel_name),
                NotificationManager.IMPORTANCE_LOW);
        c.setBlockable(true);
        c.enableLights(false);
        c.enableVibration(false);
        c.setSound(null, null);
        c.setShowBadge(true);
        nm.createNotificationChannel(c);
        handler.postDelayed(new Runnable() {
            @Override
            public void run() {
                // Activity is supposed to clean us up, but if it doesn't happen, let's not
                // waste user's battery life.
                if (!ContainerManager.isAtLeastOneRunning())
                    stopSelf();
                else
                    handler.postDelayed(this, 5 * 60 * 1000);
            }
        }, 5 * 60 * 1000);
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        super.onStartCommand(intent, flags, startId);
        if (!started) {
            started = true;
            if (instance != null) {
                throw new RuntimeException("detected bug/memory leak?");
            }
            instance = this;
            audioSocketServer = new AudioSocketServer();
            audioSocketServer.startServer();
        }
        startForeground(Constants.NOTIFICATION_ID,
                new Notification.Builder(this, Constants.NOTIFICATION_CHANNEL_ID)
                        .setContentTitle(getString(R.string.running_notif_title))
                        .setContentText(getString(R.string.running_notif_message))
                        .setSmallIcon(R.drawable.ic_cute_penguin)
                        .build());
        return START_NOT_STICKY;
    }

    @Nullable
    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    @Override
    public void onDestroy() {
        handler.removeCallbacksAndMessages(null);
        if (audioSocketServer != null)
            audioSocketServer.stopServer();
        instance = null;
    }
}
