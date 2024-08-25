package vendor.lindroid.composer;

interface IComposerCallback {
    oneway void onVsyncReceived(int sequenceId, long display, long timestamp);
    void onHotplugReceived(int sequenceId, long display, boolean connected, boolean primaryDisplay);
    oneway void onRefreshReceived(int sequenceId, long display);
}
