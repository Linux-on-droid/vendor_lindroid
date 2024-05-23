#include <jni.h>
#include <string>

#include <android/log.h>
#include <android/native_window_jni.h>

//int ANativeWindow_dequeueBuffer(ANativeWindow* window, ANativeWindowBuffer** buffer, int* fenceFd);

extern "C" {
#define LOG_TAG "LindroidNative"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static ANativeWindow* g_nativeWindow = NULL;

void
Java_org_lindroid_ui_MainActivity_nativeSurfaceCreated(
        JNIEnv *env,
        jobject /* this */
        ,jobject surface) {

    g_nativeWindow = ANativeWindow_fromSurface(env, surface);
    if (g_nativeWindow == NULL) {
        LOGE("Get ANativeWindow ERROR!");
    }


    LOGE("Width: %d, Height: %d", ANativeWindow_getWidth(g_nativeWindow), ANativeWindow_getHeight(g_nativeWindow));


}

void
Java_org_lindroid_ui_MainActivity_nativeSurfaceChanged(
        JNIEnv *env,
        jobject /* this */
        ,jobject surface) {

    g_nativeWindow = ANativeWindow_fromSurface(env, surface);
    if (g_nativeWindow == NULL) {
        LOGE("Get ANativeWindow ERROR!");
    }


    LOGE("Width: %d, Height: %d", ANativeWindow_getWidth(g_nativeWindow), ANativeWindow_getHeight(g_nativeWindow));


}

}