package org.lindroid.ui;

import android.net.LocalServerSocket;
import android.net.LocalSocket;
import android.util.Log;

import android.system.Os;
import android.system.OsConstants;
import android.system.UnixSocketAddress;
import android.system.ErrnoException;
import android.os.ParcelFileDescriptor;
import java.io.File;
import java.io.FileDescriptor;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.concurrent.Executors;

public class AudioSocketServer {

    private static final String TAG = "AudioSocketServer";
    private static final String SOCKET_PATH = "/data/lindroid/mnt/audio_socket";

    private LocalServerSocket serverSocket;
    private boolean isRunning = false;

    public void startServer() {
        Executors.newSingleThreadExecutor().execute(() -> {
            try {
                // Remove existing socket file if it exists
                File socketFile = new File(SOCKET_PATH);
                if (socketFile.exists()) {
                    socketFile.delete();
                }

                try {
                    // Create a UNIX domain socket file descriptor
                    FileDescriptor socketFd = Os.socket(OsConstants.AF_UNIX, OsConstants.SOCK_STREAM, 0);

                    // Bind the socket to the file path
                    Os.bind(socketFd, UnixSocketAddress.createFileSystem(SOCKET_PATH));

                    // Set the socket to listen for incoming connections
                    Os.listen(socketFd, 50);

                    serverSocket = new LocalServerSocket(socketFd);
                    isRunning = true;
                    Log.i(TAG, "Server started at " + SOCKET_PATH);

                    while (isRunning) {
                        LocalSocket clientSocket = serverSocket.accept();
                        handleClient(clientSocket);
                    }
                } catch (ErrnoException e) {
                    Log.e(TAG, "Error setting up server socket", e);
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
                    //TBD: Handle audio data here
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
