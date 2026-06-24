#define ALOG_TAG "LindroidComposer"

#include <aidlcommonsupport/NativeHandle.h>
#include <cutils/native_handle.h>
#include <android/hardware_buffer.h>
#include <android/native_window.h>
#include <private/android/AHardwareBufferHelpers.h>
#include <errno.h>
#include <fcntl.h>
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

}

static inline void close_if_valid(int fd) {
    if (fd >= 0) ::close(fd);
}

static inline void destroy_cloned_handle(native_handle_t* h) {
    if (!h) return;
    native_handle_delete(h);
}

static inline float frame_rate_from_display_config(const DisplayConfiguration& cfg) {
    if (cfg.vsyncPeriod <= 0) return 60.0f;
    double hz = 1e9 / static_cast<double>(cfg.vsyncPeriod);
    if (!(hz > 1.0 && hz < 1000.0)) hz = 60.0;
    return static_cast<float>(hz);
}

namespace aidl {
namespace vendor {
namespace lindroid {
namespace composer {

ndk::ScopedAStatus ComposerImpl::registerCallback(const std::shared_ptr<IComposerCallback> &in_cb, int32_t sequenceId) {
    ALOGI("%s: sequenceId: %d", __FUNCTION__, sequenceId);
    std::vector<int64_t> hotplugDisplays;
    {
        Mutex::Autolock _l(mLock);
        mSequenceId = sequenceId;
        mCallbacks = in_cb;
        hotplugDisplays.reserve(mDisplays.size());
        for (auto &entry : mDisplays) {
            ComposerDisplay* d = entry.second;
            if (d && d->nativeWindow != nullptr) {
                hotplugDisplays.push_back(entry.first);
                d->plugged = true;
            }
        }
    }
    for (int64_t displayId : hotplugDisplays) {
        if (mCallbacks != nullptr) {
            mCallbacks->onHotplugReceived(mSequenceId, displayId, true, displayId == 0);
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
    Mutex::Autolock _l(mLock);
    auto display = mDisplays.find(in_displayId);
    if (display != mDisplays.end() && display->second != nullptr) {
        *_aidl_return = display->second->displayConfig;
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus ComposerImpl::acceptChanges(int64_t in_displayId) {
    //ALOGI("%s: Display: %" PRId64 "", __FUNCTION__, in_displayId);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus ComposerImpl::getReleaseFence(int64_t in_displayId, ndk::ScopedFileDescriptor *_aidl_return) {
    (void)in_displayId;
    *_aidl_return = ndk::ScopedFileDescriptor();
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus ComposerImpl::present(int64_t in_displayId, ndk::ScopedFileDescriptor *_aidl_return) {
    Mutex::Autolock _l(mLock);
    auto it = mDisplays.find(in_displayId);
    if (it == mDisplays.end() || it->second == nullptr) {
        *_aidl_return = ndk::ScopedFileDescriptor();
        return ndk::ScopedAStatus::ok();
    }

    ComposerDisplay* d = it->second;
    int fd = -1;
    {
        std::lock_guard<std::mutex> lock(d->mFenceLock);
        if (d->mPresentFenceFd >= 0 && fcntl(d->mPresentFenceFd, F_GETFD) >= 0) {
            fd = d->mPresentFenceFd;
            d->mPresentFenceFd = -1;
        } else {
            d->mPresentFenceFd = -1;
        }
    }
    if (fd >= 0) {
        *_aidl_return = ndk::ScopedFileDescriptor(fd);
    } else {
        *_aidl_return = ndk::ScopedFileDescriptor();
    }
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
    int acquireFd = in_acquireFence.get() >= 0 ? ::dup(in_acquireFence.get()) : -1;
    ComposerDisplay* display = nullptr;
    {
        Mutex::Autolock _l(mLock);
        if (!m_ui_running)
            m_ui_running = true;
        auto it = mDisplays.find(in_displayId);
        if (it == mDisplays.end() || it->second->surface == nullptr) {
            close_if_valid(acquireFd);
            return ndk::ScopedAStatus::ok();
        }
        display = it->second;
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
    int numFds = nativeHandle ? nativeHandle->numFds : 0;
    int numInts = nativeHandle ? nativeHandle->numInts : 0;
    AHardwareBuffer *ahwb = nullptr;
    const status_t status = AHardwareBuffer_createFromHandle(
        &desc, nativeHandle, AHARDWAREBUFFER_CREATE_FROM_HANDLE_METHOD_CLONE, &ahwb);
    destroy_cloned_handle(nativeHandle);
    nativeHandle = nullptr;
    if (status != NO_ERROR) {
        ALOGE("%s: createFromHandle failed!", __FUNCTION__);
        close_if_valid(acquireFd);
        *_aidl_return = static_cast<int32_t>(status);
    }
    if (display->surfaceControl == nullptr) {
        ALOGE("%s: surfaceControl is null for display %" PRId64, __FUNCTION__, in_displayId);
        close_if_valid(acquireFd);
        AHardwareBuffer_release(ahwb);
        *_aidl_return = NO_ERROR;
        return ndk::ScopedAStatus::ok();
    }

    {
        Mutex::Autolock _l(mLock);
        auto it = mDisplays.find(in_displayId);
        if (it == mDisplays.end() || it->second == nullptr || it->second->surfaceControl == nullptr) {
            close_if_valid(acquireFd);
            AHardwareBuffer_release(ahwb);
            *_aidl_return = NO_ERROR;
            return ndk::ScopedAStatus::ok();
        }
        display = it->second;
        ASurfaceControl* surfaceControl = display->surfaceControl;
        const float contentRate = frame_rate_from_display_config(display->displayConfig);

        ASurfaceTransaction* transaction = ASurfaceTransaction_create();

        ASurfaceTransaction_setOnComplete(transaction, display,
            [](void* ctx, ASurfaceTransactionStats* stats) {
                auto* d = static_cast<ComposerDisplay*>(ctx);
                if (!d) return;
                const int fd = ASurfaceTransactionStats_getPresentFenceFd(stats);
                const int storeFd = fd >= 0 ? ::dup(fd) : -1;
                if (fd >= 0) ::close(fd);
                int oldFd = -1;
                {
                    std::lock_guard<std::mutex> lock(d->mFenceLock);
                    oldFd = d->mPresentFenceFd;
                    d->mPresentFenceFd = storeFd;
                }
                if (oldFd >= 0) ::close(oldFd);
            });

        ASurfaceTransaction_setFrameRateWithChangeStrategy(
            transaction,
            surfaceControl,
            contentRate,
            ANATIVEWINDOW_FRAME_RATE_COMPATIBILITY_FIXED_SOURCE,
            ANATIVEWINDOW_CHANGE_FRAME_RATE_ONLY_IF_SEAMLESS);

        ASurfaceTransaction_setBuffer(transaction, surfaceControl, ahwb, acquireFd);
        acquireFd = -1;
        ASurfaceTransaction_setVisibility(
            transaction, surfaceControl, ASURFACE_TRANSACTION_VISIBILITY_SHOW);

        ASurfaceTransaction_apply(transaction);
        ASurfaceTransaction_delete(transaction);
    }

    AHardwareBuffer_release(ahwb);
    close_if_valid(acquireFd);

    *_aidl_return = NO_ERROR;

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus ComposerImpl::getUiRunning(bool *_aidl_return) {
    *_aidl_return = m_ui_running;
    return ndk::ScopedAStatus::ok();
}

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
    bool needHotplug = false;
    std::shared_ptr<IComposerCallback> callbacks;
    int32_t sequenceId = 0;

    {
        Mutex::Autolock _l(mLock);

        ANativeWindow* previousNativeWindow = nullptr;
        ComposerDisplay* targetDisplay = nullptr;
        auto display = mDisplays.find(displayId);

        if (display != mDisplays.end() && display->second != nullptr) {
            targetDisplay = display->second;
            previousNativeWindow = targetDisplay->nativeWindow;

            const bool geometryChanged =
                targetDisplay->displayConfig.width != displayConfig.width ||
                targetDisplay->displayConfig.height != displayConfig.height;
            const bool refreshChanged =
                targetDisplay->displayConfig.vsyncPeriod != displayConfig.vsyncPeriod;

            if (targetDisplay->plugged && (geometryChanged || refreshChanged)) {
                needRefresh = true;
            }

            targetDisplay->nativeWindow = nativeWindow;
            targetDisplay->surface = surface;
            targetDisplay->displayConfig = displayConfig;
        } else {
            targetDisplay = new ComposerDisplay();
            targetDisplay->nativeWindow = nativeWindow;
            targetDisplay->surface = surface;
            targetDisplay->displayConfig = displayConfig;
            targetDisplay->mVsyncThread.setCallback([this, displayId, targetDisplay](int64_t timestamp, uint32_t count32) {
                (void)count32;
                if (mCallbacks == nullptr)
                    return;
                mCallbacks->onVsyncReceived(mSequenceId, displayId, timestamp);
            });
            targetDisplay->mVsyncThread.start(0, displayConfig.vsyncPeriod);
            mDisplays[displayId] = targetDisplay;
        }

        if (targetDisplay->surfaceControl == nullptr || previousNativeWindow != nativeWindow) {
            if (targetDisplay->surfaceControl) {
                ASurfaceControl_release(targetDisplay->surfaceControl);
                targetDisplay->surfaceControl = nullptr;
            }
            targetDisplay->surfaceControl = ASurfaceControl_createFromWindow(nativeWindow, "LindroidDisplay");
        }

        if (!targetDisplay->plugged && mCallbacks != nullptr) {
            targetDisplay->plugged = true;
            needHotplug = true;
        }

        callbacks = mCallbacks;
        sequenceId = mSequenceId;
    }

    if (callbacks != nullptr && needHotplug) {
        callbacks->onHotplugReceived(sequenceId, displayId, true, displayId == 0);
        callbacks->onRefreshReceived(sequenceId, displayId);
    } else if (callbacks != nullptr && needRefresh) {
        callbacks->onRefreshReceived(sequenceId, displayId);
    }
}

void ComposerImpl::onSurfaceDestroyed(int64_t displayId, sp<Surface> surface, ANativeWindow *nativeWindow) {
    ALOGI("%s", __FUNCTION__);
    ComposerDisplay* display = nullptr;
    {
        Mutex::Autolock _l(mLock);
        auto it = mDisplays.find(displayId);
        if (it == mDisplays.end()) return;
        display = it->second;
        display->nativeWindow = nullptr;
        display->surface = nullptr;
        if (display->surfaceControl) {
            ASurfaceControl_release(display->surfaceControl);
            display->surfaceControl = nullptr;
        }
        int oldFd = -1;
        {
            std::lock_guard<std::mutex> fl(display->mFenceLock);
            oldFd = display->mPresentFenceFd;
            display->mPresentFenceFd = -1;
        }
        if (oldFd >= 0) ::close(oldFd);
    }
}

void ComposerImpl::onDisplayDestroyed(int64_t displayId) {
    ALOGI("%s", __FUNCTION__);
    ComposerDisplay* display = nullptr;
    {
        Mutex::Autolock _l(mLock);
        auto it = mDisplays.find(displayId);
        if (it == mDisplays.end()) return;
        display = it->second;
        display->nativeWindow = nullptr;
        display->surface = nullptr;
        if (display->surfaceControl) {
            ASurfaceControl_release(display->surfaceControl);
            display->surfaceControl = nullptr;
        }
        display->plugged = false;
        display->mVsyncThread.stop();
        int oldFd = -1;
        {
            std::lock_guard<std::mutex> fl(display->mFenceLock);
            oldFd = display->mPresentFenceFd;
            display->mPresentFenceFd = -1;
        }
        if (oldFd >= 0) ::close(oldFd);
    }

    if (mCallbacks != nullptr)
        mCallbacks->onHotplugReceived(mSequenceId, displayId, false, displayId == 0);

    {
        Mutex::Autolock _l(mLock);
        mDisplays.erase(displayId);
    }
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
