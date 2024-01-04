
#include <Keyboard.h>
#include <stdarg.h>
#include <cppQueue.h>

#include "decoder.h"
#include "wheel.h"

struct RemoteDecoderConfig {
    enum {
        Interrupt = 1,
        DigitalPin = 2,

        QueueSize = 32,
        MaxPacketSize = 8,

        StartBitDuration = 4500,
        StopBitDuration = 7900,

        StartBitDurationMargin = 500,
        StopBitDurationMargin = 300,

        DataBitDurationMargin = 100,
        ZeroBitDuration = 350,
        OneBitDuration = 700,
    };
};
using Decoder = RemoteDecoder<RemoteDecoderConfig> ;
Decoder remoteDecoder;

MouseWheel wheelHandler;

void Serialprint(const char* input...) {
  va_list args;
  va_start(args, input);
  for (const char* i = input; *i != 0; ++i) {
    if (*i != '%') {
      Serial.print(*i);
      continue;
    }
    switch (*(++i)) {
      case '%': Serial.print('%'); break;
      case 's': Serial.print(va_arg(args, char*)); break;
      case 'd': Serial.print(va_arg(args, int), DEC); break;
      case 'b': Serial.print(va_arg(args, int), BIN); break;
      case 'o': Serial.print(va_arg(args, int), OCT); break;
      case 'x': Serial.print(va_arg(args, int), HEX); break;
      case 'f': Serial.print(va_arg(args, double), 2); break;
    }
  }
  va_end(args);
}

void p(unsigned char X) {
   if (X < 16) {Serial.print("0");}
   Serial.print(X, HEX);
}

inline void WriteReceivedPacket(Decoder::QueueEntry &entry) {
    uint8_t bytes = entry.bitLength / 8;
    Serialprint("RECV len=%d data=", (int)bytes);
    for(uint8_t i = 0; i < bytes; ++i) {
      p(entry.data[i]);
    }
    Serial.println();
}

// unsigned char currentButton = 0;
// int lastButtonTick = 0;
// int lastButtonDownTick = 0;

#define ACTION_DOWN 1
#define ACTION_UP 2
#define ACTION_PRESS 4
#define ACTION_RESET 5

#define BTN_UP   0x88
#define BTN_STP  0xAA
#define BTN_DWN  0xCC

#define LONG_PRESS_DURATION (2000)

// struct RemotePacket {
//   int32_t address;
//   unsigned char button;
// };

// void HandleData(struct RemotePacket* data) {
//   RemotePacket packet = *data;

//   if(packet.address == 0xA0B3C325) {
//     // Serial.print("BUTTON ");
//     // Serial.print(packet.button, HEX);
//     // Serial.println();
//     buttonQueue.push(&packet.button);
//   }
// }

#if 0
void HandleButton(unsigned char btn, unsigned char action, unsigned char longPress) {
  Serialprint("BTN %d %x %d\n", (int)action, (int)btn, (int)longPress);

  switch(btn){
    case BTN_UP:
      if (action == ACTION_DOWN)
        wheelValue = 1;
      else
        wheelValue = 0;
      return;

    case BTN_DWN:
      if (action == ACTION_DOWN)
        wheelValue = -1;
      else
        wheelValue = 0;
      return;

    case BTN_STP:
      if(action == ACTION_RESET) {
        Keyboard.write(longPress ? 'b' : 'n');
      }
      return;
  }
}

#endif

void setup() {
  Serial.begin(9600);

  Mouse.begin();
  Keyboard.begin();

  remoteDecoder.Begin();
  wheelHandler.Begin();

  sei();
}

void loop() {
  wheelHandler.Update();

  Decoder::QueueEntry entry;
  while(remoteDecoder.Next(&entry))
  {
    WriteReceivedPacket(entry);


  }
}

#if 0

void loop2() {
  int now = millis();

  if((currentButton != 0) && (now - lastButtonTick > 200)) {
      HandleButton(currentButton, ACTION_RESET, (now - lastButtonDownTick) > LONG_PRESS_DURATION);
      currentButton = 0;
  }

  while(!buttonQueue.isEmpty())
  {
    unsigned char button;
    if(!buttonQueue.pop(&button)) {
      break;
    }

    lastButtonTick = now;

    unsigned char releaseBtn = (button & 0xF) | ((~button) & 0xF0);

    if(currentButton == releaseBtn) {
      HandleButton(currentButton, ACTION_UP, (now - lastButtonDownTick) > LONG_PRESS_DURATION);
      currentButton = button;
      continue;
    }
    else if(currentButton != button) {
      HandleButton(button, ACTION_DOWN, 0);
      lastButtonDownTick = now;
      currentButton = button;
      continue;
    // } else {
      // HandleButton(button, ACTION_PRESS);
      // continue;
    }
  }
}
#endif