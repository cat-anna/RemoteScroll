#pragma once

#include <Mouse.h>

namespace Wheel
{

static constexpr int8_t WheelTick = 100;

static constexpr int8_t WheelUp = 1;
static constexpr int8_t WheelDown = -1;
static constexpr int8_t WheelStop = 0;

class MouseWheel {
public:
    void Begin() {}

    void Update() {
        int now = millis();
        if((now - lastTick) > WheelTick) {
            lastTick = now;
            Mouse.move(0, 0, wheelValue);
        }
    }
    void SetValue(signed char value) {
        wheelValue = value;
    }
private:
    unsigned long lastTick = 0;
    signed char wheelValue = 0;
};

} // namespace Wheel
