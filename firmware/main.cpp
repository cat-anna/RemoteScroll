
#include <Keyboard.h>
#include <cppQueue.h>

#include "serial_print.h"
#include "decoder.h"
#include "wheel.h"
#include "remote_handler.h"

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
using Decoder = RemoteDecoder<RemoteDecoderConfig>;

Decoder remoteDecoder;
Wheel::MouseWheel wheelHandler;
RemoteHandler::RemoteHandler remoteHandler;

inline void WriteReceivedPacket(Decoder::QueueEntry &entry) {
    uint8_t bytes = entry.bitLength / 8;
    SerialPrintf("RECV len=%d data=", (int)bytes);
    for(uint8_t i = 0; i < bytes; ++i) {
      printHex(entry.data[i]);
    }
    Serial.println();
}

inline void WriteButtonAction(RemoteHandler::RemoteButtonChange &entry) {
  SerialPrintf("BTN remote=%d button=%d action=%d\n", (int)entry.remoteId, (int)entry.button, (int)entry.action);
}

void HandleButton(uint8_t remote, uint8_t btn, uint8_t action) {
  if(remote != 0) {
    return;
  }

  static constexpr uint8_t ButtonUp = 0x8;
  static constexpr uint8_t ButtonDown = 0xC;
  static constexpr uint8_t ButtonStop = 0xA;

  using namespace RemoteHandler;
  using namespace Wheel;

  uint8_t down = action == ButtonActionDown;

  switch(btn){
    case ButtonUp:
      wheelHandler.SetValue(down ? WheelUp : WheelStop);
      return;
    case ButtonDown:
      wheelHandler.SetValue(down ? WheelDown : WheelStop);
      return;

    case ButtonStop:
      if(action == ButtonActionUpLong)
        Keyboard.write('b');
      if(action == ButtonActionUpShort)
        Keyboard.write('n');
      return;
  }
}

inline void ProcessIncomingPackets() {
  Decoder::QueueEntry entry;
  while(remoteDecoder.Next(&entry))
  {
    WriteReceivedPacket(entry);
    uint8_t bytes = entry.bitLength / 8;
    if(bytes == sizeof(RemoteHandler::RemoteDataPacket)){
      remoteHandler.HandlePacket((RemoteHandler::RemoteDataPacket*)entry.data);
    }
  }
}

inline void ProcessButtonChanges() {
  RemoteHandler::RemoteButtonChange entry;
  while(remoteHandler.Next(&entry))
  {
    WriteButtonAction(entry);
    HandleButton(entry.remoteId, entry.button, entry.action);
  }
}

void setup() {
  Serial.begin(9600);

  Mouse.begin();
  Keyboard.begin();

  remoteDecoder.Begin();
  wheelHandler.Begin();

  remoteHandler.Begin();
  remoteHandler.SetRemoteAddress(0, 0xA0B3C325);

  sei();
}

void loop() {
  wheelHandler.Update();
  remoteHandler.Update();
  ProcessIncomingPackets();
  ProcessButtonChanges();
}
