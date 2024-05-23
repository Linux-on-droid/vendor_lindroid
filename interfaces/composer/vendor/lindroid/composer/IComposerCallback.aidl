package vendor.lindroid.composer;

interface IComposerCallback {
    void onVsyncReceived(int sequenceId, long display, long timestamp);
    void onHotplugReceived(int sequenceId, long display, boolean connected, boolean primaryDisplay);
    void onRefreshReceived(int sequenceId, long display, long timestamp);

}
