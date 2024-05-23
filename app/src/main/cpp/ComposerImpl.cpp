#define ALOG_TAG "LindroidComposer"

#include <aidlcommonsupport/NativeHandle.h>
#include <android/hardware_buffer.h>
#include <android/native_window.h>
#include <gui/IProducerListener.h>
#include <private/android/AHardwareBufferHelpers.h>
#include <ui/Fence.h>
#include <utils/Log.h>
#include <vndk/hardware_buffer.h>
#include <vndk/window.h>

#include "ComposerImpl.h"

using namespace android;

namespace aidl {
namespace vendor {
namespace lindroid {
namespace composer {

ndk::ScopedAStatus ComposerImpl::registerCallback(const std::shared_ptr<IComposerCallback> &in_cb) {
    mCallbacks = in_cb;
    if (!plugged && mNativeWindow != nullptr) {
        mCallbacks->onHotplugReceived(0, 0, true, true);
        plugged = true;
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus ComposerImpl::onHotplug(int64_t in_displayId, bool in_connected) {
    ALOGI("%s: Display: %" PRId64 ", Connected: %d", __FUNCTION__, in_displayId, in_connected);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus ComposerImpl::requestDisplay(int64_t in_displayId) {
    ALOGI("%s: Display: %" PRId64 "", __FUNCTION__, in_displayId);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus ComposerImpl::getActiveConfig(int64_t in_displayId, DisplayConfiguration *_aidl_return) {
    ALOGI("%s: Display: %" PRId64 "", __FUNCTION__, in_displayId);
    *_aidl_return = mDisplayConfig;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus ComposerImpl::acceptChanges(int64_t in_displayId) {
    //ALOGI("%s: Display: %" PRId64 "", __FUNCTION__, in_displayId);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus ComposerImpl::getReleaseFence(int64_t in_displayId, ndk::ScopedFileDescriptor *_aidl_return) {
    //ALOGI("%s: Display: %" PRId64 "", __FUNCTION__, in_displayId);
    sp<Fence> fence = Fence::NO_FENCE;
    if (fence->isValid()) {
        *_aidl_return = ndk::ScopedFileDescriptor(fence->dup());
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus ComposerImpl::present(int64_t in_displayId, ndk::ScopedFileDescriptor *_aidl_return) {
    //ALOGI("%s: Display: %" PRId64 "", __FUNCTION__, in_displayId);
    sp<Fence> fence = Fence::NO_FENCE;
    if (fence->isValid()) {
        *_aidl_return = ndk::ScopedFileDescriptor(fence->dup());
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus ComposerImpl::setPowerMode(int32_t in_mode) {
    ALOGI("%s: mode: %d", __FUNCTION__, in_mode);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus ComposerImpl::setVsyncEnabled(int32_t in_enabled) {
    ALOGI("%s: enabled: %d", __FUNCTION__, in_enabled);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus ComposerImpl::setBuffer(int64_t in_displayId, const HardwareBuffer &hardwareBuffer, int32_t fenceFd, int32_t *_aidl_return) {
    native_handle_t *nativeHandle = makeFromAidl(hardwareBuffer.handle);
    const AHardwareBuffer_Desc desc{
        .width = static_cast<uint32_t>(hardwareBuffer.description.width),
        .height = static_cast<uint32_t>(hardwareBuffer.description.height),
        .layers = static_cast<uint32_t>(hardwareBuffer.description.layers),
        .format = static_cast<uint32_t>(hardwareBuffer.description.format),
        .usage = (static_cast<uint64_t>(hardwareBuffer.description.usage) | GraphicBuffer::USAGE_HW_TEXTURE),
        .stride = static_cast<uint32_t>(hardwareBuffer.description.stride),
    };
    AHardwareBuffer *ahwb = nullptr;
    const status_t status = AHardwareBuffer_createFromHandle(
        &desc, nativeHandle, AHARDWAREBUFFER_CREATE_FROM_HANDLE_METHOD_CLONE, &ahwb);
    if (status != NO_ERROR) {
        ALOGE("%s: createFromHandle failed!", __FUNCTION__);
        *_aidl_return = static_cast<int32_t>(status);
    }
    ANativeWindowBuffer *buffer = AHardwareBuffer_to_ANativeWindowBuffer(ahwb);
    if (mSurface == nullptr) {
        ALOGE("%s: Get Surface Failed!", __FUNCTION__);
        return ndk::ScopedAStatus::ok();
    }
    *_aidl_return = mSurface->attachBuffer(buffer);
    if (*_aidl_return == NO_ERROR) {
        if (mNativeWindow == nullptr) {
            ALOGE("%s: Get NativeWindow Failed!", __FUNCTION__);
            return ndk::ScopedAStatus::ok();
        }
        *_aidl_return = ANativeWindow_queueBuffer(mNativeWindow, buffer, -1);
    }

    return ndk::ScopedAStatus::ok();
}

void ComposerImpl::onSurfaceCreated(sp<Surface> surface, ANativeWindow *nativeWindow) {
    if (nativeWindow == nullptr) {
        ALOGE("%s: Get ANativeWindow ERROR!", __FUNCTION__);
        return;
    }
    if (surface == nullptr) {
        ALOGE("%s: Get Surface ERROR!", __FUNCTION__);
        return;
    }
    ALOGI("SurfaceCreated: Width: %d, Height: %d", ANativeWindow_getWidth(nativeWindow), ANativeWindow_getHeight(nativeWindow));
    //TODO: Do something with this information
}

void ComposerImpl::onSurfaceChanged(sp<Surface> surface, ANativeWindow *nativeWindow) {
    if (nativeWindow == nullptr) {
        ALOGE("%s: Get ANativeWindow ERROR!", __FUNCTION__);
        return;
    }
    if (surface == nullptr) {
        ALOGE("%s: Get Surface ERROR!", __FUNCTION__);
        return;
    }
    ALOGI("SurfaceChanged: Width: %d, Height: %d", ANativeWindow_getWidth(nativeWindow), ANativeWindow_getHeight(nativeWindow));
    ANativeWindow_setAutoRefresh(nativeWindow, true);
    mDisplayConfig.configId = 0;
    mDisplayConfig.displayId = 0;
    mDisplayConfig.width = ANativeWindow_getWidth(nativeWindow);
    mDisplayConfig.height = ANativeWindow_getHeight(nativeWindow);
    mDisplayConfig.dpi.x = 320;
    mDisplayConfig.dpi.y = 320;
    mDisplayConfig.vsyncPeriod = 16666667; // 60Hz

    sp<IProducerListener> listener = new StubProducerListener();
    surface->connect(NATIVE_WINDOW_API_EGL, listener);
    surface->getIGraphicBufferProducer()->allowAllocation(true);
    mNativeWindow = nativeWindow;
    mSurface = surface;
    if (!plugged && mCallbacks != nullptr) {
        mCallbacks->onHotplugReceived(0, 0, true, true);
        plugged = true;
    }
}

void ComposerImpl::onSurfaceDestroyed(sp<Surface> surface, ANativeWindow *nativeWindow) {
    mNativeWindow = nullptr;
    mSurface = nullptr;
    //Do something?
    ALOGI("%s", __FUNCTION__);
}

} // namespace composer
} // namespace lindroid
} // namespace vendor
} // namespace aidl
