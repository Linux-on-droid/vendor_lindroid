#define ALOG_TAG "LindroidComposer"

#include <aidlcommonsupport/NativeHandle.h>
#include <android/hardware_buffer.h>
#include <android/native_window.h>
#include <private/android/AHardwareBufferHelpers.h>
#include <sys/prctl.h>
#include <utils/Log.h>
#include <vndk/hardware_buffer.h>
#include <vndk/window.h>

#include "ComposerImpl.h"

using namespace android;

namespace aidl {
namespace vendor {
namespace lindroid {
namespace composer {

ndk::ScopedAStatus ComposerImpl::registerCallback(const std::shared_ptr<IComposerCallback> &in_cb, int32_t sequenceId) {
    ALOGI("%s: sequenceId: %d", __FUNCTION__, sequenceId);
    Mutex::Autolock _l(mLock);

    mSequenceId = sequenceId;
    mCallbacks = in_cb;
    for (auto &display : mDisplays) {
        if (display.second->nativeWindow != nullptr) {
            mCallbacks->onHotplugReceived(mSequenceId, display.first, true, display.first == 0);
            display.second->plugged = true;
        }
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
    auto display = mDisplays.find(in_displayId);
    if (display != mDisplays.end()) {
        *_aidl_return = display->second->displayConfig;
    }
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

ndk::ScopedAStatus ComposerImpl::setPowerMode(int64_t in_displayId, int32_t in_mode) {
    ALOGI("%s: Display: %" PRId64 " mode: %d", __FUNCTION__, in_displayId, in_mode);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus ComposerImpl::setVsyncEnabled(int64_t in_displayId, int32_t in_enabled) {
    ALOGI("%s: Display: %" PRId64 " enabled: %d", __FUNCTION__, in_displayId, in_enabled);
    auto display = mDisplays.find(in_displayId);
    if (display != mDisplays.end()) {
        if (in_enabled) {
            display->second->mVsyncThread.start();
        } else {
            display->second->mVsyncThread.stop();
        }
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus ComposerImpl::setBuffer(int64_t in_displayId, const HardwareBuffer &hardwareBuffer, const ::ndk::ScopedFileDescriptor &in_acquireFence, int32_t *_aidl_return) {
    auto display = mDisplays.find(in_displayId);
    if (display != mDisplays.end()) {
        if (display->second->surface == nullptr) {
            //ALOGE("%s: Get Surface Failed!", __FUNCTION__);
            return ndk::ScopedAStatus::ok();
        }
    } else {
        // ALOGE("%s: Get Display Failed!", __FUNCTION__);
        return ndk::ScopedAStatus::ok();
    }
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
    if (mDisplays[in_displayId]->surface == nullptr) {
        // ALOGE("%s: Get Surface Failed!", __FUNCTION__);
        return ndk::ScopedAStatus::ok();
    }
    *_aidl_return = mDisplays[in_displayId]->surface->attachBuffer(buffer);
    if (*_aidl_return == NO_ERROR) {
        if (mDisplays[in_displayId]->nativeWindow == nullptr) {
            // ALOGE("%s: Get NativeWindow Failed!", __FUNCTION__);
            return ndk::ScopedAStatus::ok();
        }
        *_aidl_return = ANativeWindow_queueBuffer(mDisplays[in_displayId]->nativeWindow, buffer, -1);
    }
    AHardwareBuffer_release(ahwb);

    return ndk::ScopedAStatus::ok();
}

class DisplayListener : public SurfaceListener {
public:
    DisplayListener(ComposerDisplay *targetDisplay) : targetDisplay(targetDisplay) {}

    virtual ~DisplayListener() = default;
    virtual void onBufferReleased() {
        AHardwareBuffer *rawSourceBuffer;
        int rawSourceFence;
        float texTransform[16];

        status_t err = ANativeWindow_getLastQueuedBuffer(targetDisplay->nativeWindow, &rawSourceBuffer, &rawSourceFence, texTransform);
        if (err == NO_ERROR) {
            if (rawSourceBuffer != nullptr) {
                AHardwareBuffer_release(rawSourceBuffer);
            }
        }
    }
    virtual bool needsReleaseNotify() { return true; }
    virtual void onBuffersDiscarded(const std::vector<sp<GraphicBuffer>>& buffers) { }
private:
    ComposerDisplay *targetDisplay;
};

void ComposerImpl::onSurfaceCreated(int64_t displayId, sp<Surface> surface, ANativeWindow *nativeWindow) {
    if (nativeWindow == nullptr) {
        ALOGE("%s: Get ANativeWindow ERROR!", __FUNCTION__);
        return;
    }
    if (surface == nullptr) {
        ALOGE("%s: Get Surface ERROR!", __FUNCTION__);
        return;
    }
    ALOGI("%s: Display: %" PRId64 ", Width: %d, Height: %d", __FUNCTION__, displayId, ANativeWindow_getWidth(nativeWindow), ANativeWindow_getHeight(nativeWindow));
    //TODO: Do something with this information
}

void ComposerImpl::onSurfaceChanged(int64_t displayId, sp<Surface> surface, ANativeWindow *nativeWindow, int dpi, float refresh) {
    if (nativeWindow == nullptr) {
        ALOGE("%s: Get ANativeWindow ERROR!", __FUNCTION__);
        return;
    }
    if (surface == nullptr) {
        ALOGE("%s: Get Surface ERROR!", __FUNCTION__);
        return;
    }
    ALOGI("%s: Display: %" PRId64 ", Width: %d, Height: %d, dpi: %d, refreshRate: %f", __FUNCTION__, 
        displayId, ANativeWindow_getWidth(nativeWindow), ANativeWindow_getHeight(nativeWindow), dpi, refresh);
    DisplayConfiguration displayConfig;
    displayConfig.configId = 0;
    displayConfig.displayId = displayId;
    displayConfig.width = ANativeWindow_getWidth(nativeWindow);
    displayConfig.height = ANativeWindow_getHeight(nativeWindow);
    displayConfig.dpi.x = dpi;
    displayConfig.dpi.y = dpi;
    displayConfig.vsyncPeriod = 10E8 / refresh;

    bool needRefresh = false;
    auto display = mDisplays.find(displayId);
    if (display != mDisplays.end()) {
        if (display->second->plugged &&
            (display->second->displayConfig.width != displayConfig.width ||
             display->second->displayConfig.height != displayConfig.height)) {
            needRefresh = true;
        }
        display->second->nativeWindow = nativeWindow;
        display->second->surface = surface;
        display->second->displayConfig = displayConfig;
    } else {
        ComposerDisplay *targetDisplay = new ComposerDisplay();
        targetDisplay->nativeWindow = nativeWindow;
        targetDisplay->surface = surface;
        targetDisplay->displayConfig = displayConfig;
        targetDisplay->plugged = false;
        targetDisplay->listener = new DisplayListener(targetDisplay);
        targetDisplay->mVsyncThread.setCallback([&](int64_t timestamp) {
            if (mCallbacks == nullptr)
                return;
            mCallbacks->onVsyncReceived(mSequenceId, displayId, timestamp);
        });
        mDisplays[displayId] = targetDisplay;
    }

    surface->connect(NATIVE_WINDOW_API_EGL, false, mDisplays[displayId]->listener);

    if (!mDisplays[displayId]->plugged && mCallbacks != nullptr) {
        mCallbacks->onHotplugReceived(mSequenceId, displayId, true, displayId == 0);
        mDisplays[displayId]->plugged = true;
    }
    if (needRefresh && mCallbacks != nullptr) {
        mCallbacks->onRefreshReceived(mSequenceId, displayId);
    }
}

void ComposerImpl::onSurfaceDestroyed(int64_t displayId, sp<Surface> surface, ANativeWindow *nativeWindow) {
    ALOGI("%s", __FUNCTION__);
    auto display = mDisplays.find(displayId);
    if (display != mDisplays.end()) {
        display->second->surface = nullptr;
    }
}

void ComposerImpl::onDisplayDestroyed(int64_t displayId) {
    ALOGI("%s", __FUNCTION__);
    auto display = mDisplays.find(displayId);
    if (display != mDisplays.end()) {
        display->second->surface = nullptr;
        display->second->plugged = false;
        display->second->mVsyncThread.stop();
        if (mCallbacks != nullptr)
            mCallbacks->onHotplugReceived(mSequenceId, displayId, false, displayId == 0);
    }
    mDisplays.erase(displayId);
}

void VsyncThread::start() {
    if (mStarted)
        return;

    mStarted = true;
    mThread = std::thread(&VsyncThread::vsyncLoop, this);
}

void VsyncThread::stop() {
    {
        std::lock_guard<std::mutex> lock(mMutex);
        if (!mStarted)
            return;

        mStarted = false;
    }
    mThread.join();
}

void VsyncThread::setCallback(const vsync_callback_t &callback) {
    std::lock_guard<std::mutex> lock(mMutex);
    mCallback = callback;
}

void VsyncThread::onVsync(int64_t frameTimeNanos) {
    std::lock_guard<std::mutex> lock(mMutex);
    ALOGV("VsyncThread(%" PRId64 ")", frameTimeNanos);
    if (mCallback) {
        mCallback(frameTimeNanos);
    }
    if (mStarted) {
        scheduleNextFrameCallback();
    }
}

void VsyncThread::scheduleNextFrameCallback() {
    AChoreographer_postFrameCallback64(mChoreographer, [](int64_t frameTimeNanos, void *data) {
        reinterpret_cast<VsyncThread *>(data)->onVsync(frameTimeNanos);
    }, this);
}

void VsyncThread::vsyncLoop() {
    prctl(PR_SET_NAME, "VsyncThread", 0, 0, 0);

    int outFd, outEvents;
    void *outData;
    std::lock_guard<std::mutex> lock(mMutex);
    mLooper = ALooper_prepare(0);
    if (!mLooper) {
        ALOGE("ALooper_prepare failed");
        return;
    }

    mChoreographer = AChoreographer_getInstance();
    if (!mChoreographer) {
        ALOGE("AChoreographer_getInstance failed");
        return;
    }

    if (mStarted)
        scheduleNextFrameCallback();

    while (mStarted) {
        // mutex should be unlocked before sleeping on pollAll
        mMutex.unlock();
        ALooper_pollAll(-1, &outFd, &outEvents, &outData);
        mMutex.lock();
    }
    ALOGI("Terminating Vsync Looper thread");
}

} // namespace composer
} // namespace lindroid
} // namespace vendor
} // namespace aidl
