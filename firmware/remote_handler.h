#pragma once

#include <stdint.h>
#include <cppQueue.h>

#ifndef ALLOW_DYNAMIC_BUTTON_MAPPING
#define ALLOW_DYNAMIC_BUTTON_MAPPING
#endif

namespace RemoteHandler {

static constexpr uint8_t MaxRemoteId = 4;
static constexpr uint8_t ButtonActionQueueSize = 8;

static constexpr uint16_t ResetDuration = 200;
static constexpr uint16_t LongPressDuration = 2000;

static constexpr uint8_t ButtonActionDown = 1;
static constexpr uint8_t ButtonActionUpShort = 2;
static constexpr uint8_t ButtonActionUpLong = 3;
static constexpr uint8_t ButtonActionPress = 4;
static constexpr uint8_t ButtonActionReset = 5;

struct RemoteDataPacket {
    uint32_t address;
    uint8_t button;
};

struct RemoteButtonChange {
    uint8_t remoteId;
    uint8_t button;
    uint8_t action;
};

class RemoteHandler {
public:
    RemoteHandler()
        : buttonQueue(sizeof(RemoteButtonChange),
                      ButtonActionQueueSize,
                      FIFO,
                      true,
                      queueData,
                      sizeof(queueData))
    {
    }

    uint8_t HasNext() { return !buttonQueue.isEmpty(); }
    uint8_t Next(RemoteButtonChange* data) { return buttonQueue.pop(data); }

    void Begin() { }

    void SetRemoteAddress(uint8_t id, uint32_t address) {
        if(id < MaxRemoteId) {
            addresses[id] = address;
        }
    }

    void Update() {
        if(anythingWasPressed) {
            int now = millis();
            if(now - lastPacketTime > ResetDuration) {

                uint8_t action = ButtonActionUpShort;
                if((now - lastButtonDownTime) > LongPressDuration)
                    action = ButtonActionUpLong;
                QueueButton(lastRemoteId, lastButtonId, action);

                QueueButton(lastRemoteId, lastButtonId, ButtonActionReset);
                anythingWasPressed = 0;
            }
        }
    }

    void HandlePacket(const RemoteDataPacket *packet) {
        uint8_t remoteId;
        if(!FindRemoteByAddress(packet->address, remoteId)) {
            return;
        }

        int now = millis();
        lastPacketTime = now;

        uint8_t buttonId = packet->button & 0xF;
        uint8_t down = ((packet->button) >> 4) == buttonId;

        if(!anythingWasPressed) {
            QueueButton(remoteId, buttonId, ButtonActionDown);
            anythingWasPressed = 1;
            lastButtonDownTime = now;
            return;
        }

        if(lastButtonId != buttonId) {
            QueueButton(lastRemoteId, lastButtonId, ButtonActionReset);
            QueueButton(remoteId, buttonId, ButtonActionDown);
            anythingWasPressed = 1;
            lastButtonDownTime = now;
            return;
        }
        if(lastButtonId == buttonId) {
            uint8_t action = ButtonActionDown;
            if (!down) {
                if(now - lastButtonDownTime > LongPressDuration) {
                    action = ButtonActionUpLong;
                } else {
                    action = ButtonActionUpShort;
                }
            }

            if(action != lastAction) {
                QueueButton(remoteId, buttonId, action);
            }
        }
    }
private:
    uint32_t addresses[MaxRemoteId];

    uint8_t anythingWasPressed = 0;
    uint8_t lastRemoteId = 0;
    uint8_t lastButtonId = 0;
    uint8_t lastAction = 0;
    unsigned long lastButtonDownTime = 0;
    unsigned long lastPacketTime = 0;

    RemoteButtonChange queueData[ButtonActionQueueSize];
    cppQueue	buttonQueue;

    bool FindRemoteByAddress(uint32_t addr, uint8_t &remote) {
        for(uint8_t i = 0; i < MaxRemoteId; ++i) {
            if(addresses[i] == addr) {
                remote = i;
                return true;
            }

#ifdef ALLOW_DYNAMIC_BUTTON_MAPPING
            if(addresses[i] == 0) {
                SetRemoteAddress(i, addr);
                remote = i;
                return true;
            }
#endif
        }

        return false;
    }

    void QueueButton(uint8_t remote, uint8_t btn, uint8_t action) {
        RemoteButtonChange entry;
        lastRemoteId = entry.remoteId = remote;
        lastButtonId = entry.button = btn;
        lastAction = entry.action = action;
        buttonQueue.push(&entry);
    }
};

}
