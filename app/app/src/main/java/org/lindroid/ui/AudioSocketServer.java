package org.lindroid.ui;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.net.LocalServerSocket;
import android.net.LocalSocket;
import android.system.ErrnoException;
import android.system.Os;
import android.system.OsConstants;
import android.system.UnixSocketAddress;
import android.util.Log;

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

    private AudioTrack audioTrack;

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

                    // Initialize AudioTrack
                    initializeAudioTrack();

                    while (isRunning) {
                        LocalSocket clientSocket = serverSocket.accept();
                        Log.i(TAG, "Accepted client");
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

    private void initializeAudioTrack() {
        // Assuming audio is PCM 16-bit, 48000Hz, mono
        int sampleRate = 48000;
        int channelConfig = AudioFormat.CHANNEL_OUT_STEREO;
        int audioFormat = AudioFormat.ENCODING_PCM_16BIT;

        int minBufferSize = AudioTrack.getMinBufferSize(sampleRate, channelConfig, audioFormat);
        audioTrack = new AudioTrack(
                AudioManager.STREAM_MUSIC,
                sampleRate,
                channelConfig,
                audioFormat,
                minBufferSize,
                AudioTrack.MODE_STREAM
        );

        audioTrack.play();
    }

    private void handleClient(LocalSocket clientSocket) {
        Executors.newSingleThreadExecutor().execute(() -> {
            try (InputStream inputStream = clientSocket.getInputStream();
                 OutputStream outputStream = clientSocket.getOutputStream()) {
                byte[] buffer = new byte[1024];
                int bytesRead;

                while ((bytesRead = inputStream.read(buffer)) != -1) {
                    // Play the received audio data
                    audioTrack.write(buffer, 0, bytesRead);
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
        if (audioTrack != null) {
            audioTrack.stop();
            audioTrack.release();
        }
    }
}
