#define ALOG_TAG "LindroidNative"

#include <jni.h>
#include <string>

#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <android/native_window_jni.h>
#include <utils/Log.h>

#include <utils/StrongPointer.h>
#include <android_runtime/android_view_Surface.h>

#include "ComposerImpl.h"
#include "InputDevice.h"

using aidl::vendor::lindroid::composer::ComposerImpl;

using namespace android;

static std::shared_ptr<ComposerImpl> composer = nullptr;
static sp<InputDevice> inputDevice = nullptr;

extern "C" void
Java_org_lindroid_ui_NativeLib_nativeStartComposerService(
    JNIEnv *env, jclass /* clazz */) {
    ALOGI("Init native: Starting composer binder service...");

    composer = ndk::SharedRefBase::make<ComposerImpl>();

    binder_status_t status = AServiceManager_addService(composer->asBinder().get(), "vendor.lindroid.composer");
    if (status != STATUS_OK) {
        ALOGE("Could not register composer binder service");
    }

    ABinderProcess_joinThreadPool();
}

extern "C" void
Java_org_lindroid_ui_NativeLib_nativeSurfaceCreated(
    JNIEnv *env, jclass /* clazz */,
    jlong displayId, jobject surface) {

    sp<Surface> sf = android_view_Surface_getSurface(env, surface);
    if (sf == nullptr) {
        ALOGE("Get Surface ERROR!");
        return;
    }
    ANativeWindow* nativeWindow = ANativeWindow_fromSurface(env, surface);
    if (nativeWindow == nullptr) {
        ALOGE("Get ANativeWindow ERROR!");
        return;
    }
    if (composer == nullptr) {
        int tryCount = 0;
        while (composer == nullptr && tryCount < 10) {
            ALOGE("Composer is not initialized! Try again...");
            usleep(1000000);
            tryCount++;
        }
        ALOGE("Composer is not initialized!");
        return;
    }
    composer->onSurfaceCreated(displayId, sf, nativeWindow);
}

extern "C" void
Java_org_lindroid_ui_NativeLib_nativeSurfaceChanged(
    JNIEnv *env, jclass /* clazz */,
    jlong displayId, jobject surface, jint dpi, jfloat refresh) {

    sp<Surface> sf = android_view_Surface_getSurface(env, surface);
    if (sf == nullptr) {
        ALOGE("Get Surface ERROR!");
        return;
    }
    ANativeWindow *nativeWindow = ANativeWindow_fromSurface(env, surface);
    if (nativeWindow == nullptr) {
        ALOGE("Get ANativeWindow ERROR!");
        return;
    }
    if (composer == nullptr) {
        int tryCount = 0;
        while (composer == nullptr && tryCount < 10) {
            ALOGE("Composer is not initialized! Try again...");
            usleep(1000000);
            tryCount++;
        }
        ALOGE("Composer is not initialized!");
        return;
    }
    composer->onSurfaceChanged(displayId, sf, nativeWindow, dpi, refresh);
}

extern "C" void
Java_org_lindroid_ui_NativeLib_nativeSurfaceDestroyed(
    JNIEnv *env, jclass /* clazz */,
    jlong displayId, jobject surface) {

    sp<Surface> sf = android_view_Surface_getSurface(env, surface);
    if (sf == nullptr) {
        ALOGE("Get Surface ERROR!");
        return;
    }
    ANativeWindow *nativeWindow = ANativeWindow_fromSurface(env, surface);
    if (nativeWindow == nullptr) {
        ALOGE("Get ANativeWindow ERROR!");
        return;
    }
    if (composer == nullptr) {
        int tryCount = 0;
        while (composer == nullptr && tryCount < 10) {
            ALOGE("Composer is not initialized! Try again...");
            usleep(1000000);
            tryCount++;
        }
        ALOGE("Composer is not initialized!");
        return;
    }
    composer->onSurfaceDestroyed(displayId, sf, nativeWindow);

    ALOGE("SurfaceDestroyed");
}

extern "C" void
Java_org_lindroid_ui_NativeLib_nativeDisplayDestroyed(
    JNIEnv *env, jclass /* clazz */,
    jlong displayId) {

    if (composer == nullptr) {
        int tryCount = 0;
        while (composer == nullptr && tryCount < 10) {
            ALOGE("Composer is not initialized! Try again...");
            usleep(1000000);
            tryCount++;
        }
        ALOGE("Composer is not initialized!");
        return;
    }
    composer->onDisplayDestroyed(displayId);

    ALOGE("SurfaceDestroyed");
}

extern "C" void
Java_org_lindroid_ui_NativeLib_nativeInitInputDevice(
    JNIEnv *env, jclass /* clazz */) {

    inputDevice = new InputDevice();
}

extern "C" void
Java_org_lindroid_ui_NativeLib_nativeReconfigureInputDevice(
    JNIEnv *env, jclass /* clazz */,
    jlong displayId, jint width, jint height) {

    if (inputDevice == nullptr) {
        ALOGE("InputDevice is not initialized!");
        return;
    }

    inputDevice->reconfigure(displayId, width, height);
}

extern "C" void
Java_org_lindroid_ui_NativeLib_nativeStopInputDevice(
    JNIEnv *env, jclass /* clazz */,
    jlong displayId) {

    if (inputDevice == nullptr) {
        ALOGE("InputDevice is not initialized!");
        return;
    }

    inputDevice->stop(displayId);
}

extern "C" void
Java_org_lindroid_ui_NativeLib_nativeKeyEvent(
    JNIEnv *env, jclass /* clazz */,
    jlong displayId, jint keyCode, jboolean isDown) {

    if (inputDevice == nullptr) {
        ALOGE("InputDevice is not initialized!");
        return;
    }

    inputDevice->keyEvent(displayId, keyCode, isDown);
}

extern "C" void
Java_org_lindroid_ui_NativeLib_nativeTouchEvent(
    JNIEnv *env, jclass /* clazz */,
    jlong displayId, jint pointerId, jint action, jint pressure, jint x, jint y) {

    if (inputDevice == nullptr) {
        ALOGE("InputDevice is not initialized!");
        return;
    }

    inputDevice->touchEvent(displayId, pointerId, action, pressure, x, y);
}

extern "C" void
Java_org_lindroid_ui_NativeLib_nativePointerMotionEvent(
    JNIEnv *env, jclass /* clazz */,
    jlong displayId, jint x, jint y) {

    if (inputDevice == nullptr) {
        ALOGE("InputDevice is not initialized!");
        return;
    }

    inputDevice->pointerMotionEvent(displayId, x, y);
}

extern "C" void
Java_org_lindroid_ui_NativeLib_nativePointerButtonEvent(
    JNIEnv *env, jclass /* clazz */,
    jlong displayId, jint button, jint x, jint y, jboolean isDown) {

    if (inputDevice == nullptr) {
        ALOGE("InputDevice is not initialized!");
        return;
    }

    inputDevice->pointerButtonEvent(displayId, button, x, y, isDown);
}

extern "C" void
Java_org_lindroid_ui_NativeLib_nativePointerScrollEvent(
    JNIEnv *env, jclass /* clazz */,
    jlong displayId, jint value, jboolean isVertical) {

    if (inputDevice == nullptr) {
        ALOGE("InputDevice is not initialized!");
        return;
    }

    inputDevice->pointerScrollEvent(displayId, value, isVertical);
}
