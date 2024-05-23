#pragma once

#include <gui/Surface.h>

#include <aidl/android/hardware/graphics/common/HardwareBuffer.h>
#include <aidl/vendor/lindroid/composer/BnComposer.h>
#include <aidl/vendor/lindroid/composer/DisplayConfiguration.h>
#include <aidl/vendor/lindroid/composer/IComposerCallback.h>

using aidl::android::hardware::graphics::common::HardwareBuffer;
using aidl::vendor::lindroid::composer::DisplayConfiguration;
using aidl::vendor::lindroid::composer::IComposerCallback;
using android::sp;
using android::Surface;

namespace aidl {
namespace vendor {
namespace lindroid {
namespace composer {

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

    void onSurfaceCreated(sp<Surface> surface, ANativeWindow* nativeWindow);
    void onSurfaceChanged(sp<Surface> surface, ANativeWindow* nativeWindow);
    void onSurfaceDestroyed(sp<Surface> surface, ANativeWindow* nativeWindow);

private:
    std::shared_ptr<IComposerCallback> mCallbacks;
    sp<Surface> mSurface;
    ANativeWindow *mNativeWindow;
    DisplayConfiguration mDisplayConfig;
    bool plugged;
};

} // namespace composer
} // namespace lindroid
} // namespace vendor
} // namespace aidl
