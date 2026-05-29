#define ALOG_TAG "LindroidComposer"

#include <aidlcommonsupport/NativeHandle.h>
#include <android/hardware_buffer.h>
#include <android/native_window.h>
#include <private/android/AHardwareBufferHelpers.h>
#include <errno.h>
#include <poll.h>
#include <sys/prctl.h>
#include <type_traits>
#include <utils/Log.h>
#include <vndk/hardware_buffer.h>
#include <vndk/window.h>

#include "ComposerImpl.h"

using namespace android;

// Multi-android version vsync check support for DisplayEventReceiver
namespace {

static inline constexpr uint32_t fourcc_be(char c1, char c2, char c3, char c4) {
    return (uint32_t(uint8_t(c1)) << 24) |
           (uint32_t(uint8_t(c2)) << 16) |
           (uint32_t(uint8_t(c3)) <<  8) |
           (uint32_t(uint8_t(c4))      );
}

static inline constexpr uint32_t fourcc_le(char c1, char c2, char c3, char c4) {
    return (uint32_t(uint8_t(c1))      ) |
           (uint32_t(uint8_t(c2)) <<  8) |
           (uint32_t(uint8_t(c3)) << 16) |
           (uint32_t(uint8_t(c4)) << 24);
}

template <class T>
static inline constexpr uint32_t to_u32_event_type(T t) {
    if constexpr (std::is_enum_v<T>) {
        return static_cast<uint32_t>(static_cast<std::underlying_type_t<T>>(t));
    } else {
        return static_cast<uint32_t>(t);
    }
}

static inline constexpr bool is_vsync_event_type_u32(uint32_t v) {
    return v == fourcc_be('v','s','y','n') ||
           v == fourcc_le('v','s','y','n') ||
           v == static_cast<uint32_t>('vsyn');
}

template <class T>
static inline constexpr bool is_vsync_event_type(T t) {
    return is_vsync_event_type_u32(to_u32_event_type(t));
}

} // namespace

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
    ComposerDisplay* d = nullptr;
    {
        Mutex::Autolock _l(mLock);
        auto it = mDisplays.find(in_displayId);
        if (it == mDisplays.end() || it->second == nullptr) {
            return ndk::ScopedAStatus::ok();
        }
        d = it->second;
    }

    // No fence
    *_aidl_return = ndk::ScopedFileDescriptor();
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus ComposerImpl::setPowerMode(int64_t in_displayId, int32_t in_mode) {
    ALOGI("%s: Display: %" PRId64 " mode: %d", __FUNCTION__, in_displayId, in_mode);
    return ndk::ScopedAStatus::ok();
}

void ComposerImpl::onAppForegroundChanged(int64_t displayId, bool foreground) {
    ALOGI("%s: Display: %" PRId64 " foreground: %d", __FUNCTION__, displayId, foreground);
    if (mCallbacks == nullptr)
        return;
    mCallbacks->onAppForegroundChanged(mSequenceId, displayId, foreground);
}

ndk::ScopedAStatus ComposerImpl::setVsyncEnabled(int64_t in_displayId, int32_t in_enabled) {
    ALOGI("%s: Display: %" PRId64 " enabled: %d", __FUNCTION__, in_displayId, in_enabled);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus ComposerImpl::setBuffer(int64_t in_displayId, const HardwareBuffer &hardwareBuffer, const ::ndk::ScopedFileDescriptor &in_acquireFence, int32_t *_aidl_return) {
    Mutex::Autolock _l(mLock);
    auto display = mDisplays.find(in_displayId);
    if(!m_ui_running)
        m_ui_running = true;

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

ndk::ScopedAStatus ComposerImpl::getUiRunning(bool *_aidl_return) {
    *_aidl_return = m_ui_running;
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
    virtual void onBufferDetached(int slot) { }
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
    double rate = static_cast<double>(refresh);
    if (!(rate > 1.0 && rate < 1000.0)) {
        rate = 60.0; //60hz fallback
    }
    const int64_t periodNs = static_cast<int64_t>(llround(1000000000.0 / rate));
    displayConfig.vsyncPeriod = periodNs;

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
        targetDisplay->mVsyncThread.setCallback([this, displayId, targetDisplay](int64_t timestamp, uint32_t count32) {
            if (mCallbacks == nullptr)
                return;
            mCallbacks->onVsyncReceived(mSequenceId, displayId, timestamp);
       });
        targetDisplay->mVsyncThread.start(0, displayConfig.vsyncPeriod);
        mDisplays[displayId] = targetDisplay;
    }

    surface->connect(NATIVE_WINDOW_API_EGL, mDisplays[displayId]->listener, false);

    if (!mDisplays[displayId]->plugged && mCallbacks != nullptr) {
        mDisplays[displayId]->plugged = true;
        mCallbacks->onHotplugReceived(mSequenceId, displayId, true, displayId == 0);
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

void VsyncThread::start(int64_t firstVsync, int64_t period) {
    (void)firstVsync;
    (void)period;

    const status_t st = mReceiver.initCheck();
    LOG_ALWAYS_FATAL_IF(st != ::android::OK,
                        "DisplayEventReceiver initCheck failed (%d); no fallback permitted", st);
    mReceiverReady = true;
    mStarted = true;

    (void)mReceiver.setVsyncRate(0u);
    (void)mReceiver.requestNextVsync();
    mThread = std::thread(&VsyncThread::vsyncLoop, this);
}

void VsyncThread::stop() {
    {
        std::lock_guard<std::mutex> lock(mMutex);
        mStarted = false;
    }
    if (mThread.joinable())
        mThread.join();
}

void VsyncThread::setCallback(const vsync_callback_t &callback) {
    std::lock_guard<std::mutex> lock(mMutex);
    mCallback = callback;
}

void VsyncThread::vsyncLoop() {
    prctl(PR_SET_NAME, "VsyncThread", 0, 0, 0);

    std::unique_lock<std::mutex> lock(mMutex);
    if (!mStarted) return;
    LOG_ALWAYS_FATAL_IF(!mReceiverReady, "VsyncThread started without DisplayEventReceiver");

    (void)mReceiver.setVsyncRate(0u);
    (void)mReceiver.requestNextVsync();

    while (true) {
        auto cb = mCallback;
        lock.unlock();

        const int fd = mReceiver.getFd();
        LOG_ALWAYS_FATAL_IF(fd < 0, "DisplayEventReceiver fd invalid: %d", fd);

        struct pollfd pfd;
        pfd.fd = fd;
        pfd.events = POLLIN;
        pfd.revents = 0;

        const int pr = ::poll(&pfd, 1, 50 /*ms*/);
        if (pr < 0 && errno == EINTR) {
            lock.lock();
            if (!mStarted) break;
            continue;
        }
        if (pr > 0 && (pfd.revents & POLLIN)) {
            ::android::DisplayEventReceiver::Event evs[16];
            int64_t lastTs = 0;
            uint32_t lastCount32 = 0;

            for (;;) {
                const ssize_t n = mReceiver.getEvents(evs, std::size(evs));
                if (n <= 0) break;
                for (ssize_t i = 0; i < n; i++) {
                    if (is_vsync_event_type(evs[i].header.type)) {
                        lastTs = static_cast<int64_t>(evs[i].header.timestamp);
                        lastCount32 = evs[i].vsync.count;
                    }
                }
            }

            if (cb && lastTs != 0) {
                cb(lastTs, lastCount32);
            }
        }

        (void)mReceiver.requestNextVsync();

        lock.lock();
        if (!mStarted) break;
    }
}

} // namespace composer
} // namespace lindroid
} // namespace vendor
} // namespace aidl
