#pragma once

#include <unordered_map>
#include <utils/Errors.h>
#include <utils/Mutex.h>
#include <utils/RefBase.h>

#include <linux/uinput.h>

#define UINPUT_DEVICE "/dev/uinput"

#define DEVICE_KEYBOARD 0
#define DEVICE_TOUCH 1
#define DEVICE_TABLET 2
#define DEVICE_TOUCH_STYLUS 3

using android::Mutex;
using android::RefBase;
using android::status_t;

struct UInputDevice {
    void getFD(int32_t device, int32_t *fd);
    status_t start(int32_t device, int64_t displayId);
    status_t stop(int32_t device);
    status_t inject(int32_t device, uint16_t type, uint16_t code, int32_t value);

    int32_t mWidth;
    int32_t mHeight;
    int32_t mFDKeyboard = -1;
    int32_t mFDTouch = -1;
    int32_t mFDTouchStylus = -1;
    int32_t mFDTablet = -1;
};

class InputDevice : public RefBase {
public:
    virtual status_t reconfigure(int64_t displayId, uint32_t width, uint32_t height);
    virtual status_t stop(int64_t displayId);

    virtual void keyEvent(int64_t displayId, uint32_t keyCode, bool isDown);
    virtual void touchEvent(int64_t displayId, int64_t pointerId, int32_t action, int32_t pressure, int32_t x, int32_t y);
    virtual void touchStylusButtonEvent(int64_t displayId, uint32_t button, bool isDown);
    virtual void touchStylusHoverEvent(int64_t displayId, int32_t action, int32_t x, int32_t y, int32_t distance, int32_t tilt_x, int32_t tilt_y);
    virtual void touchStylusEvent(int64_t displayId, int32_t action, int32_t pressure, int32_t x, int32_t y, int32_t tilt_x, int32_t tilt_y);
    virtual void pointerMotionEvent(int64_t displayId, int32_t x, int32_t y);
    virtual void pointerButtonEvent(int64_t displayId, uint32_t button, int32_t x, int32_t y, bool isDown);
    virtual void pointerScrollEvent(int64_t displayId, uint32_t value, bool isVertical);

private:
    status_t start(int64_t displayId, uint32_t width, uint32_t height);
    status_t start_async(int64_t displayId, uint32_t width, uint32_t height);

    Mutex mLock;
    std::unordered_map<int64_t, UInputDevice *> mInputs;
};
