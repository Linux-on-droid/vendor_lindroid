package vendor.lindroid.composer;

import vendor.lindroid.composer.IComposerCallback;
import vendor.lindroid.composer.DisplayConfiguration;
import android.hardware.graphics.common.HardwareBuffer;

interface IComposer {
    void registerCallback(in IComposerCallback cb, int sequenceId);
    void onHotplug(long displayId, boolean connected);

    // Display
    void requestDisplay(long displayId);
    DisplayConfiguration getActiveConfig(long displayId);
    void acceptChanges(long displayId);
    @nullable ParcelFileDescriptor getReleaseFence(long displayId);
    @nullable ParcelFileDescriptor present(long displayId);
    void setPowerMode(long displayId, int mode);
    void setVsyncEnabled(long displayId, int enabled);
    //void validate() 

    // Layer
    int setBuffer(long displayId, in HardwareBuffer buffer, in @nullable ParcelFileDescriptor fenceFd);
    //void setBlendMode(int mode);
    //setColor
    //setCompositionType
    //setDataspace
    //setDisplayFrame
    //setPlaneAlpha
    //setSidebandStream
    //setSourceCrop
    //setTransform
    //setVisibleRegion
}
