package org.lindroid.ui;

import static org.lindroid.ui.Constants.CONTAINER_NAME;

import android.os.Bundle;

public class PrimaryDisplayActivity extends DisplayActivity {
    private AudioSocketServer audioSocketServer;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        start(0, CONTAINER_NAME);
        audioSocketServer = new AudioSocketServer();
        audioSocketServer.startServer();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (audioSocketServer != null) {
            audioSocketServer.stopServer();
        }
    }
}
