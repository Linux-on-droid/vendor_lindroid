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

using aidl::vendor::lindroid::composer::ComposerImpl;

using namespace android;

static std::shared_ptr<ComposerImpl> composer = nullptr;

extern "C" void
Java_org_lindroid_ui_MainActivity_nativeInit(
    JNIEnv *env,
    jobject /* this */) {
    ALOGI("Init native: Starting composer binder service...");

    composer = ndk::SharedRefBase::make<ComposerImpl>();

    binder_status_t status = AServiceManager_addService(composer->asBinder().get(), "vendor.lindroid.composer");
    if (status != STATUS_OK) {
        ALOGE("Could not register composer binder service");
    }

    ABinderProcess_joinThreadPool();
}

extern "C" void
Java_org_lindroid_ui_MainActivity_nativeSurfaceCreated(
    JNIEnv *env,
    jobject /* this */,
    jobject surface) {

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
        ALOGE("Composer is not initialized!");
        return;
    }
    composer->onSurfaceCreated(sf, nativeWindow);

    ALOGI("SurfaceCreated: Width: %d, Height: %d", ANativeWindow_getWidth(nativeWindow), ANativeWindow_getHeight(nativeWindow));
}

extern "C" void
Java_org_lindroid_ui_MainActivity_nativeSurfaceChanged(
    JNIEnv *env,
    jobject /* this */,
    jobject surface) {

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
        ALOGE("Composer is not initialized!");
        return;
    }
    composer->onSurfaceChanged(sf, nativeWindow);

    ALOGE("SurfaceChanged: Width: %d, Height: %d", ANativeWindow_getWidth(nativeWindow), ANativeWindow_getHeight(nativeWindow));
}

extern "C" void
Java_org_lindroid_ui_MainActivity_nativeSurfaceDestroyed(
    JNIEnv *env,
    jobject /* this */,
    jobject surface) {

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
        ALOGE("Composer is not initialized!");
        return;
    }
    composer->onSurfaceDestroyed(sf, nativeWindow);

    ALOGE("SurfaceDestroyed");
}
