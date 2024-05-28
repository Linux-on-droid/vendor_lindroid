#pragma once

#include <gui/Surface.h>
#include <ui/Fence.h>
#include <ui/GraphicBuffer.h>
#include <utils/Mutex.h>

#include <aidl/android/hardware/graphics/common/HardwareBuffer.h>
#include <aidl/vendor/lindroid/composer/BnComposer.h>
#include <aidl/vendor/lindroid/composer/DisplayConfiguration.h>
#include <aidl/vendor/lindroid/composer/IComposerCallback.h>

#define MAX_DEQUEUEABLE_BUFFERS 5

using aidl::android::hardware::graphics::common::HardwareBuffer;
using aidl::vendor::lindroid::composer::DisplayConfiguration;
using aidl::vendor::lindroid::composer::IComposerCallback;
using android::Mutex;
using android::sp;
using android::Surface;
using android::SurfaceListener;

namespace aidl {
namespace vendor {
namespace lindroid {
namespace composer {

struct ComposerDisplay {
    sp<Surface> surface;
    ANativeWindow *nativeWindow;
    DisplayConfiguration displayConfig;
    bool plugged;
    sp<SurfaceListener> listener;
};

class ComposerImpl : public BnComposer {
public:
    virtual ndk::ScopedAStatus registerCallback(const std::shared_ptr<IComposerCallback> &in_cb) override;
    virtual ndk::ScopedAStatus onHotplug(int64_t in_displayId, bool in_connected) override;
    virtual ndk::ScopedAStatus requestDisplay(int64_t in_displayId) override;
    virtual ndk::ScopedAStatus getActiveConfig(int64_t in_displayId, DisplayConfiguration *_aidl_return) override;
    virtual ndk::ScopedAStatus acceptChanges(int64_t in_displayId) override;
    virtual ndk::ScopedAStatus getReleaseFence(int64_t in_displayId, ndk::ScopedFileDescriptor *_aidl_return) override;
    virtual ndk::ScopedAStatus present(int64_t in_displayId, ndk::ScopedFileDescriptor *_aidl_return) override;
    virtual ndk::ScopedAStatus setPowerMode(int32_t in_mode) override;
    virtual ndk::ScopedAStatus setVsyncEnabled(int32_t in_enabled) override;
    virtual ndk::ScopedAStatus setBuffer(int64_t in_displayId, const HardwareBuffer &in_buffer, int32_t in_fenceFd, int32_t *_aidl_return) override;

    void onSurfaceCreated(int64_t displayId, sp<Surface> surface, ANativeWindow *nativeWindow);
    void onSurfaceChanged(int64_t displayId, sp<Surface> surface, ANativeWindow *nativeWindow);
    void onSurfaceDestroyed(int64_t displayId, sp<Surface> surface, ANativeWindow *nativeWindow);
    void onDisplayDestroyed(int64_t displayId);

private:
    Mutex mLock;

    std::shared_ptr<IComposerCallback> mCallbacks;
    std::unordered_map<int64_t, ComposerDisplay*> mDisplays;
};

} // namespace composer
} // namespace lindroid
} // namespace vendor
} // namespace aidl
