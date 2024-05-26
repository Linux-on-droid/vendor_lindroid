#define ALOG_TAG "LindroidInput"

#include <android/input.h>
#include <fcntl.h>
#include <future>
#include <inttypes.h>
#include <linux/input.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <utils/Log.h>

#include "InputDevice.h"

using namespace android;

static const struct UInputOptions {
    int cmd;
    int bit;
} kOptions[] = {
    {UI_SET_EVBIT, EV_KEY},
    {UI_SET_EVBIT, EV_REP},
    {UI_SET_EVBIT, EV_REL},
    {UI_SET_RELBIT, REL_WHEEL},
    {UI_SET_RELBIT, REL_HWHEEL},
    {UI_SET_EVBIT, EV_ABS},
    {UI_SET_ABSBIT, ABS_X},
    {UI_SET_ABSBIT, ABS_Y},
    {UI_SET_ABSBIT, ABS_MT_POSITION_X},
    {UI_SET_ABSBIT, ABS_MT_POSITION_Y},
    {UI_SET_ABSBIT, ABS_MT_TRACKING_ID},
    {UI_SET_EVBIT, EV_SYN},
    {UI_SET_PROPBIT, INPUT_PROP_DIRECT},
};

status_t InputDevice::start_async(int64_t displayId, uint32_t width, uint32_t height) {
    // don't block the caller since this can take a few seconds
    std::async(&InputDevice::start, this, displayId, width, height);

    return NO_ERROR;
}

static int setup_abs(int32_t fd, __u16 chan, int32_t min, int32_t max, int32_t res) {
    int err = 0;
    err = ioctl(fd, UI_SET_ABSBIT, chan);
    if (err < 0) {
        ALOGE("UI_SET_ABSBIT failed, code: %d", chan);
        return err;
    }

    struct uinput_abs_setup s = {
        .code = chan,
        .absinfo = {.minimum = min, .maximum = max, .resolution = res},
    };

    err = ioctl(fd, UI_ABS_SETUP, &s);
    if (err < 0) {
        ALOGE("UI_ABS_SETUP failed, code: %d", chan);
    }
    return err;
}

status_t InputDevice::start(int64_t displayId, uint32_t width, uint32_t height) {
    Mutex::Autolock _l(mLock);

    int err = 0;
    struct uinput_setup usetup;
    struct uinput_abs_setup uabs;
    std::string name = "LindroidInput-" + std::to_string(displayId);

    auto input = mInputs.find(displayId);
    if (input != mInputs.end()) {
        if (input->second->mFD >= 0) {
            ALOGE("Input device already open!");
            return NO_INIT;
        }
    } else {
        mInputs[displayId] = new UInputDevice();
        input = mInputs.find(displayId);
    }

    input->second->mFD = open(UINPUT_DEVICE, O_WRONLY | O_NONBLOCK);
    if (input->second->mFD < 0) {
        ALOGE("Failed to open %s: err=%d", UINPUT_DEVICE, input->second->mFD);
        return NO_INIT;
    }

    unsigned int idx = 0;
    for (idx = 0; idx < sizeof(kOptions) / sizeof(kOptions[0]); idx++) {
        if (ioctl(input->second->mFD, kOptions[idx].cmd, kOptions[idx].bit) < 0) {
            ALOGE("uinput ioctl failed: %d %d", kOptions[idx].cmd, kOptions[idx].bit);
            goto err_ioctl;
        }
    }

    for (idx = 0; idx < KEY_MAX; idx++) {
        if (ioctl(input->second->mFD, UI_SET_KEYBIT, idx) < 0) {
            ALOGE("UI_SET_KEYBIT failed");
            goto err_ioctl;
        }
    }

    err = setup_abs(input->second->mFD, ABS_X, 0, width, 12);
    if (err < 0)
        goto err_ioctl;
    err = setup_abs(input->second->mFD, ABS_MT_POSITION_X, 0, width, 12);
    if (err < 0)
        goto err_ioctl;
    err = setup_abs(input->second->mFD, ABS_Y, 0, height, 13);
    if (err < 0)
        goto err_ioctl;
    err = setup_abs(input->second->mFD, ABS_MT_POSITION_Y, 0, height, 13);
    if (err < 0)
        goto err_ioctl;
    err = setup_abs(input->second->mFD, ABS_MT_SLOT, 0, 10 /* MAX_TOUCHPOINTS */, 0);
    if (err < 0)
        goto err_ioctl;
    err = setup_abs(input->second->mFD, ABS_MT_TRACKING_ID, 0, 10 /* MAX_TOUCHPOINTS */, 0);
    if (err < 0)
        goto err_ioctl;

    memset(&usetup, 0, sizeof(usetup));
    strncpy(usetup.name, name.c_str(), UINPUT_MAX_NAME_SIZE);
    usetup.id.bustype = BUS_VIRTUAL;
    usetup.id.vendor = 10;
    usetup.id.product = 10;
    usetup.id.version = static_cast<unsigned short>(displayId);
    if (ioctl(input->second->mFD, UI_DEV_SETUP, &usetup) == -1) {
        ALOGE("UI_DEV_SETUP failed");
        goto err_ioctl;
    }

    if (ioctl(input->second->mFD, UI_DEV_CREATE) == -1) {
        ALOGE("UI_DEV_CREATE failed");
        goto err_ioctl;
    }

    ALOGD("Virtual input device display %" PRId64 " created successfully (%dx%d)", displayId, width, height);
    return NO_ERROR;

err_ioctl:
    int prev_errno = errno;
    ::close(input->second->mFD);
    errno = prev_errno;
    input->second->mFD = -1;
    return NO_INIT;
}

status_t InputDevice::reconfigure(int64_t displayId, uint32_t width, uint32_t height) {
    stop(displayId);
    return start_async(displayId, width, height);
}

status_t InputDevice::stop(int64_t displayId) {
    Mutex::Autolock _l(mLock);
    auto input = mInputs.find(displayId);
    if (input != mInputs.end()) {
        if (input->second->mFD < 0) {
            return OK;
        }

        ioctl(input->second->mFD, UI_DEV_DESTROY);
        close(input->second->mFD);
        input->second->mFD = -1;
    }

    return OK;
}

status_t UInputDevice::inject(uint16_t type, uint16_t code, int32_t value) {
    struct input_event event;
    memset(&event, 0, sizeof(event));
    gettimeofday(&event.time, 0); /* This should not be able to fail ever.. */
    event.type = type;
    event.code = code;
    event.value = value;
    if (write(mFD, &event, sizeof(event)) != sizeof(event)) return BAD_VALUE;
    return OK;
}

void InputDevice::keyEvent(int64_t displayId, uint32_t keyCode, bool isDown) {
    // ALOGE("Key event: %d, %d", keyCode, isDown);
    Mutex::Autolock _l(mLock);
    auto input = mInputs.find(displayId);
    if (input != mInputs.end()) {
        input->second->inject(EV_KEY, keyCode, isDown ? 1 : 0);
        input->second->inject(EV_SYN, SYN_REPORT, 0);
    }
}

void InputDevice::touchEvent(int64_t displayId, int64_t pointerId, int32_t action, int32_t pressure, int32_t x, int32_t y) {
    // ALOGE("Touch event: %d, %d, %d, %d, %d", (int)pointerId, action, pressure, x, y);
    Mutex::Autolock _l(mLock);
    auto input = mInputs.find(displayId);
    if (input != mInputs.end()) {
        if (action == AMOTION_EVENT_ACTION_MOVE || action == AMOTION_EVENT_ACTION_DOWN) {
            input->second->inject(EV_ABS, ABS_MT_SLOT, pointerId);
            input->second->inject(EV_ABS, ABS_MT_TRACKING_ID, pointerId);
            input->second->inject(EV_ABS, ABS_MT_POSITION_X, x);
            input->second->inject(EV_ABS, ABS_MT_POSITION_Y, y);
            input->second->inject(EV_ABS, ABS_MT_PRESSURE, pressure);
            input->second->inject(EV_SYN, SYN_MT_REPORT, 0);
            input->second->inject(EV_SYN, SYN_REPORT, 0);
        } else if (action == AMOTION_EVENT_ACTION_UP) {
            input->second->inject(EV_ABS, ABS_MT_SLOT, pointerId);
            input->second->inject(EV_ABS, ABS_MT_TRACKING_ID, -1);
            input->second->inject(EV_SYN, SYN_MT_REPORT, 0);
            input->second->inject(EV_SYN, SYN_REPORT, 0);
        }
    }
}

void InputDevice::pointerMotionEvent(int64_t displayId, int32_t x, int32_t y) {
    // ALOGE("Pointer motion event: %d, %d", x, y);
    Mutex::Autolock _l(mLock);
    auto input = mInputs.find(displayId);
    if (input != mInputs.end()) {
        input->second->inject(EV_ABS, ABS_X, x);
        input->second->inject(EV_ABS, ABS_Y, y);
        input->second->inject(EV_SYN, SYN_REPORT, 0);
    }
}

void InputDevice::pointerButtonEvent(int64_t displayId, uint32_t button, int32_t x, int32_t y, bool isDown) {
    // ALOGE("Pointer button event: %d, %d, %d, %d", button, x, y, isDown);
    Mutex::Autolock _l(mLock);
    auto input = mInputs.find(displayId);
    if (input != mInputs.end()) {
        input->second->inject(EV_ABS, ABS_X, x);
        input->second->inject(EV_ABS, ABS_Y, y);
        input->second->inject(EV_SYN, SYN_REPORT, 0);

        input->second->inject(EV_KEY, button, isDown ? 1 : 0);
        input->second->inject(EV_SYN, SYN_REPORT, 0);
    }
}

void InputDevice::pointerScrollEvent(int64_t displayId, uint32_t value, bool isVertical) {
    // ALOGE("Pointer scroll event: %d, %d", value, isVertical ? 1 : 0);
    Mutex::Autolock _l(mLock);
    auto input = mInputs.find(displayId);
    if (input != mInputs.end()) {
        input->second->inject(EV_REL, isVertical ? REL_WHEEL : REL_HWHEEL, value);
        input->second->inject(EV_SYN, SYN_REPORT, 0);
    }
}
