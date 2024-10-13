package org.lindroid.ui;

import android.annotation.SuppressLint;
import android.media.AudioAttributes;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioRecord;
import android.media.AudioTrack;
import android.media.MediaRecorder;
import android.net.LocalServerSocket;
import android.net.LocalSocket;
import android.system.ErrnoException;
import android.system.Os;
import android.system.OsConstants;
import android.util.Log;

import java.io.File;
import java.io.FileDescriptor;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.Arrays;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;

public class AudioSocketServer {
    private static final byte[] AUDIO_OUTPUT_PREFIX = new byte[] { 0x01 };
    private static final byte AUDIO_INPUT_PREFIX = 0x02;

    private static final String TAG = "AudioSocketServer";

    private final ExecutorService clientExecutor = Executors.newCachedThreadPool();
    private final ExecutorService audioExecutor = Executors.newSingleThreadExecutor();

    private LocalServerSocket serverSocket;
    private boolean isRunning = false;

    private AudioTrack audioTrack;
    private AudioRecord audioRecord;

    public AudioSocketServer() {
        clientExecutor.execute(() -> {
            try {
                // Remove existing socket file if it exists
                File socketFile = new File(Constants.SOCKET_PATH);
                if (socketFile.exists()) {
                    if (!socketFile.delete()) {
                        Log.w(TAG, "failed to delete socket");
                    }
                }

                try {
                    // Create a UNIX domain socket file descriptor
                    FileDescriptor socketFd = Os.socket(OsConstants.AF_UNIX, OsConstants.SOCK_STREAM, 0);

                    // Bind the socket to the file path
                    Os.bind(socketFd, Constants.createUnixSocketAddressObj(Constants.SOCKET_PATH));

                    // Set the socket to listen for incoming connections
                    Os.listen(socketFd, 50);

                    serverSocket = new LocalServerSocket(socketFd);
                    isRunning = true;
                    Log.i(TAG, "Server started at " + Constants.SOCKET_PATH);

                    // Initialize AudioTrack and AudioRecord
                    initializeAudioTrack();
                    initializeAudioRecord();

                    while (isRunning) {
                        LocalSocket clientSocket = serverSocket.accept();
                        Log.i(TAG, "Accepted client");

                        clientExecutor.execute(() -> handleClient(clientSocket));
                        audioExecutor.execute(() -> sendMicDataToSocket(clientSocket));
                    }
                } catch (ErrnoException e) {
                    Log.e(TAG, "Error setting up server socket", e);
                }
            } catch (IOException e) {
                Log.e(TAG, "Error starting server", e);
            }
        });
    }


    private void sendMicDataToSocket(LocalSocket clientSocket) {
        try (OutputStream outputStream = clientSocket.getOutputStream()) {
            byte[] buffer = new byte[AudioRecord.getMinBufferSize(48000, AudioFormat.CHANNEL_IN_MONO, AudioFormat.ENCODING_PCM_16BIT)];

            while (isRunning) {
                // Reset buffer before reading
                Arrays.fill(buffer, (byte) 0);

                // Add the prefix at the start of the buffer
                buffer[0] = AUDIO_INPUT_PREFIX;

                // Read data from the microphone
                int bytesRead = audioRecord.read(buffer, 1, buffer.length - 1);

                if (bytesRead > 0) {
                    // Add 1 to bytesRead to include the prefix in the output
                    outputStream.write(buffer, 0, bytesRead + 1);
                } else if (bytesRead < 0) {
                    Log.e(TAG, "Error reading microphone data: " + bytesRead);
                }
            }
        } catch (IOException e) {
            Log.e(TAG, "Error sending microphone data to client", e);
        }
    }

    private void initializeAudioTrack() {
        // Assuming audio is PCM 16-bit, 48000Hz, stereo
        int sampleRate = 48000;
        int channelConfig = AudioFormat.CHANNEL_OUT_STEREO;
        int audioFormat = AudioFormat.ENCODING_PCM_16BIT;

        int minBufferSize = AudioTrack.getMinBufferSize(sampleRate, channelConfig, audioFormat);

        audioTrack = new AudioTrack(
                new AudioAttributes.Builder()
                        .setContentType(AudioAttributes.CONTENT_TYPE_UNKNOWN)
                        .setUsage(AudioAttributes.USAGE_UNKNOWN)
                        .setIsContentSpatialized(true)
                        .build(),
                new AudioFormat.Builder()
                        .setSampleRate(sampleRate)
                        .setChannelMask(channelConfig)
                        .setEncoding(audioFormat)
                        .build(),
                minBufferSize,
                AudioTrack.MODE_STREAM,
                AudioManager.AUDIO_SESSION_ID_GENERATE
        );

        audioTrack.play();
    }

    @SuppressLint("MissingPermission") // we're system
    private void initializeAudioRecord() {
        int sampleRate = 48000;
        int channelConfig = AudioFormat.CHANNEL_IN_MONO;
        int audioFormat = AudioFormat.ENCODING_PCM_16BIT;
        int minBufferSize = AudioRecord.getMinBufferSize(sampleRate, channelConfig, audioFormat);

        audioRecord = new AudioRecord(
                MediaRecorder.AudioSource.MIC,
                sampleRate,
                channelConfig,
                audioFormat,
                minBufferSize
        );

        audioRecord.startRecording();
    }

    private void handleClient(LocalSocket clientSocket) {
        try (InputStream inputStream = clientSocket.getInputStream()) {
            byte[] buffer = new byte[10240];
            int bytesRead;

            while (isRunning) {
                bytesRead = inputStream.read(buffer);
                Log.i(TAG, "Bytes read from client: " + bytesRead);

                if (bytesRead > 0) {
                    byte prefix = buffer[0];
                    Log.i(TAG, "Received prefix: " + prefix);

                    // Write data to AudioTrack (if appropriate)
                    audioTrack.write(buffer, 1, bytesRead - 1);
                    Log.i(TAG, "Audio data written to AudioTrack: " + (bytesRead - 1) + " bytes");
                }
            }
        } catch (IOException e) {
            Log.e(TAG, "Error handling client", e);
        }
    }

    public void stopServer() {
        try {
            isRunning = false;
            audioExecutor.shutdownNow();
            clientExecutor.shutdownNow();

            if (!audioExecutor.awaitTermination(100, TimeUnit.MILLISECONDS)) {
                Log.w(TAG, "AudioExecutor shutdown timed out");
            }
            if (!clientExecutor.awaitTermination(100, TimeUnit.MILLISECONDS)) {
                Log.w(TAG, "ClientExecutor shutdown timed out");
            }

            if (serverSocket != null) {
                serverSocket.close();
            }
            if (audioTrack != null) {
                audioTrack.stop();
                audioTrack.release();
            }
            if (audioRecord != null) {
                audioRecord.stop();
                audioRecord.release();
            }
        } catch (InterruptedException | IOException e) {
            Log.e(TAG, "Error shutting down server", e);
        }
    }
}
