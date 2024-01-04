#pragma once

#include <Mouse.h>

#define WHEEL_TICK 100

class MouseWheel {
public:
    void Begin() {}

    void Update() {
        int now = millis();
        if(now - lastTick > WHEEL_TICK) {
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

