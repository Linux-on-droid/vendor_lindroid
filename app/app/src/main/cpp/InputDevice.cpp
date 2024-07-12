#include <linux/input-event-codes.h>
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

struct UInputOptions {
    int cmd;
    int bit;
};

static const UInputOptions kKeyboardOptions[] = {
    {UI_SET_EVBIT, EV_KEY},
    {UI_SET_EVBIT, EV_SYN},
};

static const UInputOptions kTouchOptions[] = {
    {UI_SET_EVBIT, EV_ABS},
    {UI_SET_ABSBIT, ABS_MT_POSITION_X},
    {UI_SET_ABSBIT, ABS_MT_POSITION_Y},
    {UI_SET_ABSBIT, ABS_MT_TRACKING_ID},
    {UI_SET_EVBIT, EV_SYN},
    {UI_SET_PROPBIT, INPUT_PROP_DIRECT},
};

static const UInputOptions kTouchStylusOptions[] = {
    {UI_SET_EVBIT, EV_ABS},
    {UI_SET_EVBIT, EV_KEY},
    {UI_SET_ABSBIT, ABS_X},
    {UI_SET_ABSBIT, ABS_Y},
    {UI_SET_ABSBIT, ABS_PRESSURE},
    {UI_SET_EVBIT, EV_SYN},
    {UI_SET_PROPBIT, INPUT_PROP_DIRECT},
};

static const UInputOptions kTabletOptions[] = {
    {UI_SET_EVBIT, EV_KEY},
    {UI_SET_EVBIT, EV_REP},
    {UI_SET_EVBIT, EV_REL},
    {UI_SET_RELBIT, REL_WHEEL},
    {UI_SET_RELBIT, REL_HWHEEL},
    {UI_SET_EVBIT, EV_ABS},
    {UI_SET_ABSBIT, ABS_X},
    {UI_SET_ABSBIT, ABS_Y},
    {UI_SET_EVBIT, EV_SYN},
    {UI_SET_PROPBIT, INPUT_PROP_DIRECT},
};

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

void UInputDevice::getFD(int32_t device, int32_t *fd) {
    switch (device) {
    case DEVICE_KEYBOARD:
        *fd = mFDKeyboard;
        break;
    case DEVICE_TOUCH:
        *fd = mFDTouch;
        break;
    case DEVICE_TOUCH_STYLUS:
        *fd = mFDTouchStylus;
        break;
    case DEVICE_TABLET:
        *fd = mFDTablet;
        break;
    default:
        *fd = -1;
        break;
    }
};

status_t UInputDevice::start(int32_t device, int64_t displayId) {
    int32_t fd = -1;
    int err = 0;
    struct uinput_setup usetup;
    struct uinput_abs_setup uabs;

    getFD(device, &fd);
    if (fd >= 0) {
        ALOGE("Input device already open!");
        return NO_INIT;
    }

    std::string name = "Lindroid-Keyboard-" + std::to_string(displayId);
    if (device == DEVICE_TOUCH) {
        name = "Lindroid-Touch-" + std::to_string(displayId);
     } else if (device == DEVICE_TOUCH_STYLUS) {
        name = "Lindroid-Stylus-" + std::to_string(displayId);
    } else if (device == DEVICE_TABLET) {
        name = "Lindroid-Tablet-" + std::to_string(displayId);
    }

    if (device == DEVICE_KEYBOARD) {
        mFDKeyboard = open(UINPUT_DEVICE, O_WRONLY | O_NONBLOCK);
        fd = mFDKeyboard;
    } else if (device == DEVICE_TOUCH) {
        mFDTouch = open(UINPUT_DEVICE, O_WRONLY | O_NONBLOCK);
        fd = mFDTouch;
    } else if (device == DEVICE_TOUCH_STYLUS) {
        mFDTouchStylus = open(UINPUT_DEVICE, O_WRONLY | O_NONBLOCK);
        fd = mFDTouchStylus;
    } else if (device == DEVICE_TABLET) {
        mFDTablet = open(UINPUT_DEVICE, O_WRONLY | O_NONBLOCK);
        fd = mFDTablet;
    }
    if (fd < 0) {
        ALOGE("Failed to open %s: err=%d", UINPUT_DEVICE, fd);
        return NO_INIT;
    }

    auto options = kKeyboardOptions;
    auto optionsSize = sizeof(kKeyboardOptions) / sizeof(kKeyboardOptions[0]);
    if (device == DEVICE_TOUCH) {
        options = kTouchOptions;
        optionsSize = sizeof(kTouchOptions) / sizeof(kTouchOptions[0]);
    } else if (device == DEVICE_TOUCH_STYLUS) {
        options = kTouchStylusOptions;
        optionsSize = sizeof(kTouchStylusOptions) / sizeof(kTouchStylusOptions[0]);
    } else if (device == DEVICE_TABLET) {
        options = kTabletOptions;
        optionsSize = sizeof(kTabletOptions) / sizeof(kTabletOptions[0]);
    }

    unsigned int idx = 0;
    for (idx = 0; idx < optionsSize; idx++) {
        if (ioctl(fd, options[idx].cmd, options[idx].bit) < 0) {
            ALOGE("uinput ioctl failed: %d %d", options[idx].cmd, options[idx].bit);
            goto err_ioctl;
        }
    }

    if (device == DEVICE_KEYBOARD) {
        for (idx = 0; idx < 255; idx++) {
            if (ioctl(fd, UI_SET_KEYBIT, idx) < 0) {
                ALOGE("UI_SET_KEYBIT failed");
                goto err_ioctl;
            }
        }
    } else if (device == DEVICE_TABLET) {
        for (idx = BTN_MOUSE; idx < BTN_JOYSTICK; idx++) {
            if (ioctl(fd, UI_SET_KEYBIT, idx) < 0) {
                ALOGE("UI_SET_KEYBIT failed");
                goto err_ioctl;
            }
        }
    } else if (device == DEVICE_TOUCH_STYLUS) {
        ioctl(fd, UI_SET_KEYBIT, BTN_TOUCH);
        ioctl(fd, UI_SET_KEYBIT, BTN_DIGI);
        err = setup_abs(fd, ABS_PRESSURE, 0, 4096, 1);
        if (err < 0)
            goto err_ioctl;
    }

    if (device == DEVICE_TABLET || device == DEVICE_TOUCH || device == DEVICE_TOUCH_STYLUS) {
        err = setup_abs(fd, ABS_X, 0, mWidth, 12);
        if (err < 0)
            goto err_ioctl;
        err = setup_abs(fd, ABS_Y, 0, mHeight, 13);
        if (err < 0)
            goto err_ioctl;
    }
    if (device == DEVICE_TOUCH) {
        err = setup_abs(fd, ABS_MT_POSITION_X, 0, mWidth, 12);
        if (err < 0)
            goto err_ioctl;
        err = setup_abs(fd, ABS_MT_POSITION_Y, 0, mHeight, 13);
        if (err < 0)
            goto err_ioctl;
        err = setup_abs(fd, ABS_MT_SLOT, 0, 10 /* MAX_TOUCHPOINTS */, 0);
        if (err < 0)
            goto err_ioctl;
        err = setup_abs(fd, ABS_MT_TRACKING_ID, 0, 10 /* MAX_TOUCHPOINTS */, 0);
        if (err < 0)
            goto err_ioctl;
    }

    memset(&usetup, 0, sizeof(usetup));
    strncpy(usetup.name, name.c_str(), UINPUT_MAX_NAME_SIZE);
    usetup.id.bustype = BUS_VIRTUAL;
    usetup.id.vendor = 10;
    usetup.id.product = 10;
    if (device == DEVICE_TOUCH)
        usetup.id.product = 11;
    else if (device == DEVICE_TABLET)
        usetup.id.product = 13;
    else if (device == DEVICE_TABLET)
        usetup.id.product = 12;
    usetup.id.version = static_cast<unsigned short>(displayId);
    if (ioctl(fd, UI_DEV_SETUP, &usetup) == -1) {
        ALOGE("UI_DEV_SETUP failed");
        goto err_ioctl;
    }

    if (ioctl(fd, UI_DEV_CREATE) == -1) {
        ALOGE("UI_DEV_CREATE failed");
        goto err_ioctl;
    }

    ALOGD("Virtual input device display %" PRId64 " created successfully (%dx%d)", displayId, mWidth, mHeight);
    return NO_ERROR;

err_ioctl:
    int prev_errno = errno;
    ::close(fd);
    errno = prev_errno;
    fd = -1;
    return NO_INIT;
};

status_t UInputDevice::stop(int32_t device) {
    ALOGE("Stop device: %d", device);
    int32_t fd = -1;
    int err = 0;
    getFD(device, &fd);
    if (fd < 0) {
        ALOGE("Input device not open!");
        return OK;
    }

    err = ioctl(fd, UI_DEV_DESTROY);
    if (err < 0) {
        ALOGE("UI_DEV_DESTROY failed");
    }
    close(fd);
    fd = -1;
    return OK;
};

status_t UInputDevice::inject(int32_t device, uint16_t type, uint16_t code, int32_t value) {
    int32_t fd = -1;
    getFD(device, &fd);
    if (fd < 0)
        return NO_INIT;

    struct input_event event;
    memset(&event, 0, sizeof(event));
    gettimeofday(&event.time, 0); /* This should not be able to fail ever.. */
    event.type = type;
    event.code = code;
    event.value = value;
    if (write(fd, &event, sizeof(event)) != sizeof(event))
        return BAD_VALUE;
    return OK;
}

status_t InputDevice::start_async(int64_t displayId, uint32_t width, uint32_t height) {
    // don't block the caller since this can take a few seconds
    std::async(&InputDevice::start, this, displayId, width, height);

    return NO_ERROR;
}

status_t InputDevice::start(int64_t displayId, uint32_t width, uint32_t height) {
    Mutex::Autolock _l(mLock);

    status_t err = 0;
    auto input = mInputs.find(displayId);
    if (input == mInputs.end()) {
        mInputs[displayId] = new UInputDevice();
        input = mInputs.find(displayId);
        input->second->mWidth = width;
        input->second->mHeight = height;
    }

    err = input->second->start(DEVICE_KEYBOARD, displayId);
    if (err < 0)
        return err;
    err = input->second->start(DEVICE_TOUCH, displayId);
    if (err < 0)
        return err;
    err = input->second->start(DEVICE_TOUCH_STYLUS, displayId);
    if (err < 0)
        return err;
    err = input->second->start(DEVICE_TABLET, displayId);
    return err;
}

status_t InputDevice::reconfigure(int64_t displayId, uint32_t width, uint32_t height) {
    stop(displayId);
    return start_async(displayId, width, height);
}

status_t InputDevice::stop(int64_t displayId) {
    Mutex::Autolock _l(mLock);

    status_t err = 0;
    auto input = mInputs.find(displayId);
    if (input != mInputs.end()) {
        err = input->second->stop(DEVICE_KEYBOARD);
        if (err < 0)
            return err;
        err = input->second->stop(DEVICE_TOUCH);
        if (err < 0)
            return err;
        err = input->second->stop(DEVICE_TOUCH_STYLUS);
        if (err < 0)
            return err;
        err = input->second->stop(DEVICE_TABLET);
        if (err < 0)
            return err;
    }
    mInputs.erase(displayId);

    return OK;
}

void InputDevice::keyEvent(int64_t displayId, uint32_t keyCode, bool isDown) {
    // ALOGE("Key event: %d, %d", keyCode, isDown);
    Mutex::Autolock _l(mLock);
    auto input = mInputs.find(displayId);
    if (input != mInputs.end()) {
        input->second->inject(DEVICE_KEYBOARD, EV_KEY, keyCode, isDown ? 1 : 0);
        input->second->inject(DEVICE_KEYBOARD, EV_SYN, SYN_REPORT, 0);
    }
}

void InputDevice::touchEvent(int64_t displayId, int64_t pointerId, int32_t action, int32_t pressure, int32_t x, int32_t y) {
    // ALOGE("Touch event: %d, %d, %d, %d, %d", (int)pointerId, action, pressure, x, y);
    Mutex::Autolock _l(mLock);
    auto input = mInputs.find(displayId);
    if (input != mInputs.end()) {
        if (action == AMOTION_EVENT_ACTION_MOVE || action == AMOTION_EVENT_ACTION_DOWN) {
            input->second->inject(DEVICE_TOUCH, EV_ABS, ABS_MT_SLOT, pointerId);
            input->second->inject(DEVICE_TOUCH, EV_ABS, ABS_MT_TRACKING_ID, pointerId);
            input->second->inject(DEVICE_TOUCH, EV_ABS, ABS_MT_POSITION_X, x);
            input->second->inject(DEVICE_TOUCH, EV_ABS, ABS_MT_POSITION_Y, y);
            input->second->inject(DEVICE_TOUCH, EV_ABS, ABS_MT_PRESSURE, pressure);
            input->second->inject(DEVICE_TOUCH, EV_SYN, SYN_MT_REPORT, 0);
            input->second->inject(DEVICE_TOUCH, EV_SYN, SYN_REPORT, 0);
        } else if (action == AMOTION_EVENT_ACTION_UP) {
            input->second->inject(DEVICE_TOUCH, EV_ABS, ABS_MT_SLOT, pointerId);
            input->second->inject(DEVICE_TOUCH, EV_ABS, ABS_MT_TRACKING_ID, -1);
            input->second->inject(DEVICE_TOUCH, EV_SYN, SYN_MT_REPORT, 0);
            input->second->inject(DEVICE_TOUCH, EV_SYN, SYN_REPORT, 0);
        }
    }
}

void InputDevice::touchStylusButtonEvent(int64_t displayId, uint32_t button, bool isDown) {
    // ALOGE("Stylus button event: %d, %d, %d, %d, %d", (int)pointerId, action, pressure, x, y);
    Mutex::Autolock _l(mLock);
    auto input = mInputs.find(displayId);
    if (input != mInputs.end()) {
        input->second->inject(DEVICE_TOUCH_STYLUS, EV_KEY, button, isDown ? 1 : 0);
        input->second->inject(DEVICE_TOUCH_STYLUS, EV_SYN, SYN_REPORT, 0);
    }
}

void InputDevice::touchStylusHoverEvent(int64_t displayId, int32_t action, int32_t x, int32_t y, int32_t distance, int32_t tilt_x, int32_t tilt_y) {
    // ALOGE("Stylus hover event: %d, %d, %d, %d, %d", (int)pointerId, action, pressure, x, y);
    Mutex::Autolock _l(mLock);
    auto input = mInputs.find(displayId);
    if (input != mInputs.end()) {
        if(action == AMOTION_EVENT_ACTION_HOVER_ENTER) {
            input->second->inject(DEVICE_TOUCH_STYLUS, EV_KEY, BTN_DIGI, 1);
        } else if (action == AMOTION_EVENT_ACTION_HOVER_EXIT) {
            input->second->inject(DEVICE_TOUCH_STYLUS, EV_KEY, BTN_DIGI, 0);
        }
        input->second->inject(DEVICE_TOUCH_STYLUS, EV_ABS, ABS_X, x);
        input->second->inject(DEVICE_TOUCH_STYLUS, EV_ABS, ABS_Y, y);
        input->second->inject(DEVICE_TOUCH_STYLUS, EV_ABS, ABS_DISTANCE, distance);
        input->second->inject(DEVICE_TOUCH_STYLUS, EV_ABS, ABS_TILT_X, tilt_x);
        input->second->inject(DEVICE_TOUCH_STYLUS, EV_ABS, ABS_TILT_X, tilt_y);
        input->second->inject(DEVICE_TOUCH_STYLUS, EV_ABS, ABS_PRESSURE, 0);
        input->second->inject(DEVICE_TOUCH_STYLUS, EV_SYN, SYN_REPORT, 0);
    }
}

void InputDevice::touchStylusEvent(int64_t displayId, int32_t action, int32_t pressure, int32_t x, int32_t y, int32_t tilt_x, int32_t tilt_y) {
    // ALOGE("Stylus touch event: %d, %d, %d, %d, %d", (int)pointerId, action, pressure, x, y);
    Mutex::Autolock _l(mLock);
    auto input = mInputs.find(displayId);
    if (input != mInputs.end()) {
        if (action == AMOTION_EVENT_ACTION_DOWN) {
            input->second->inject(DEVICE_TOUCH_STYLUS, EV_KEY, BTN_TOUCH, 1);
            input->second->inject(DEVICE_TOUCH_STYLUS, EV_KEY, BTN_DIGI, 1);
        }
        if (action == AMOTION_EVENT_ACTION_MOVE || action == AMOTION_EVENT_ACTION_DOWN) {
            input->second->inject(DEVICE_TOUCH_STYLUS, EV_ABS, ABS_X, x);
            input->second->inject(DEVICE_TOUCH_STYLUS, EV_ABS, ABS_Y, y);
            input->second->inject(DEVICE_TOUCH_STYLUS, EV_ABS, ABS_PRESSURE, pressure);
            input->second->inject(DEVICE_TOUCH_STYLUS, EV_ABS, ABS_TILT_X, tilt_x);
            input->second->inject(DEVICE_TOUCH_STYLUS, EV_ABS, ABS_TILT_X, tilt_y);
            input->second->inject(DEVICE_TOUCH_STYLUS, EV_SYN, SYN_REPORT, 0);
        } else if (action == AMOTION_EVENT_ACTION_UP) {
            input->second->inject(DEVICE_TOUCH_STYLUS, EV_KEY, BTN_TOUCH, 0);
            input->second->inject(DEVICE_TOUCH_STYLUS, EV_KEY, BTN_DIGI, 0);
            input->second->inject(DEVICE_TOUCH_STYLUS, EV_ABS, ABS_PRESSURE, 0);
            input->second->inject(DEVICE_TOUCH_STYLUS, EV_SYN, SYN_REPORT, 0);
        }
    }
}

void InputDevice::pointerMotionEvent(int64_t displayId, int32_t x, int32_t y) {
    // ALOGE("Pointer motion event: %d, %d", x, y);
    Mutex::Autolock _l(mLock);
    auto input = mInputs.find(displayId);
    if (input != mInputs.end()) {
        input->second->inject(DEVICE_TABLET, EV_ABS, ABS_X, x);
        input->second->inject(DEVICE_TABLET, EV_ABS, ABS_Y, y);
        input->second->inject(DEVICE_TABLET, EV_SYN, SYN_REPORT, 0);
    }
}

void InputDevice::pointerButtonEvent(int64_t displayId, uint32_t button, int32_t x, int32_t y, bool isDown) {
    // ALOGE("Pointer button event: %d, %d, %d, %d", button, x, y, isDown);
    Mutex::Autolock _l(mLock);
    auto input = mInputs.find(displayId);
    if (input != mInputs.end()) {
        input->second->inject(DEVICE_TABLET, EV_ABS, ABS_X, x);
        input->second->inject(DEVICE_TABLET, EV_ABS, ABS_Y, y);
        input->second->inject(DEVICE_TABLET, EV_SYN, SYN_REPORT, 0);

        input->second->inject(DEVICE_TABLET, EV_KEY, button, isDown ? 1 : 0);
        input->second->inject(DEVICE_TABLET, EV_SYN, SYN_REPORT, 0);
    }
}

void InputDevice::pointerScrollEvent(int64_t displayId, uint32_t value, bool isVertical) {
    // ALOGE("Pointer scroll event: %d, %d", value, isVertical ? 1 : 0);
    Mutex::Autolock _l(mLock);
    auto input = mInputs.find(displayId);
    if (input != mInputs.end()) {
        input->second->inject(DEVICE_TABLET, EV_REL, isVertical ? REL_WHEEL : REL_HWHEEL, value);
        input->second->inject(DEVICE_TABLET, EV_SYN, SYN_REPORT, 0);
    }
}
