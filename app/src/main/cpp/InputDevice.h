#pragma once

#include <unordered_map>
#include <utils/Errors.h>
#include <utils/Mutex.h>
#include <utils/RefBase.h>

#include <linux/uinput.h>

#define UINPUT_DEVICE "/dev/uinput"

using android::Mutex;
using android::RefBase;
using android::status_t;

struct UInputDevice {
    status_t inject(uint16_t type, uint16_t code, int32_t value);

    int32_t mFD = -1;
};

class InputDevice : public RefBase {
public:
    virtual status_t reconfigure(int64_t displayId, uint32_t width, uint32_t height);
    virtual status_t stop(int64_t displayId);

    virtual void keyEvent(int64_t displayId, uint32_t keyCode, bool isDown);
    virtual void touchEvent(int64_t displayId, int64_t pointerId, int32_t action, int32_t pressure, int32_t x, int32_t y);
    virtual void pointerMotionEvent(int64_t displayId, int32_t x, int32_t y);
    virtual void pointerButtonEvent(int64_t displayId, uint32_t button, int32_t x, int32_t y, bool isDown);
    virtual void pointerScrollEvent(int64_t displayId, uint32_t value, bool isVertical);

private:
    status_t start(int64_t displayId, uint32_t width, uint32_t height);
    status_t start_async(int64_t displayId, uint32_t width, uint32_t height);

    Mutex mLock;
    std::unordered_map<int64_t, UInputDevice *> mInputs;
};
