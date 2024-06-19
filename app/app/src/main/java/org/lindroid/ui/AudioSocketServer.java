package org.lindroid.ui;

import android.net.LocalServerSocket;
import android.net.LocalSocket;
import android.util.Log;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.concurrent.Executors;

public class AudioSocketServer {

    private static final String TAG = "AudioSocketServer";
    private static final String SOCKET_PATH = "/data/lindroid/mnt/audio_socket"; // Path to the Unix socket

    private LocalServerSocket serverSocket;
    private boolean isRunning = false;

    public void startServer() {
        Executors.newSingleThreadExecutor().execute(() -> {
            try {
                serverSocket = new LocalServerSocket(SOCKET_PATH);
                isRunning = true;
                Log.i(TAG, "Server started at " + SOCKET_PATH);

                while (isRunning) {
                    LocalSocket clientSocket = serverSocket.accept();
                    handleClient(clientSocket);
                }
            } catch (IOException e) {
                Log.e(TAG, "Error starting server", e);
            }
        });
    }

    private void handleClient(LocalSocket clientSocket) {
        Executors.newSingleThreadExecutor().execute(() -> {
            try (InputStream inputStream = clientSocket.getInputStream();
                 OutputStream outputStream = clientSocket.getOutputStream()) {
                byte[] buffer = new byte[1024];
                int bytesRead;

                while ((bytesRead = inputStream.read(buffer)) != -1) {
                    // Handle audio data here
                    Log.i(TAG, "Received data: " + bytesRead + " bytes");
                }
            } catch (IOException e) {
                Log.e(TAG, "Error handling client", e);
            }
        });
    }

    public void stopServer() {
        isRunning = false;
        if (serverSocket != null) {
            try {
                serverSocket.close();
            } catch (IOException e) {
                Log.e(TAG, "Error closing server", e);
            }
        }
    }
}
