
#include <Mouse.h>
#include <Keyboard.h>
#include <stdarg.h>
#include <cppQueue.h>

cppQueue	buttonQueue(1, 64, FIFO, true);	// Instantiate queue

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

#define RECV_PIN 2
#define RECV_INT 1

#define EDGE_UP 1
#define EDGE_DOWN 0

#define MAX_BITS 64
#define EXPECTED_LEN 40

#define TEST_DURATION_BIT(dur) ((dur) > 550)
#define TEST_DURATION_END(dur) ((dur) > 7000)
#define TEST_DURATION_START(dur) ((dur) > 4500)

struct RemotePacket {
  int32_t address;
  unsigned char button;
};

void HandleData(struct RemotePacket* data) {
  RemotePacket packet = *data;

  if(packet.address == 0xA0B3C325) {
    // Serial.print("BUTTON ");
    // Serial.print(packet.button, HEX);
    // Serial.println();
    buttonQueue.push(&packet.button);
  }
}

void handleInterrupt() {
  static unsigned long lastTime = 0;
  static char lastValue = 0;
  static char receiving = 0;
  static unsigned char currentBit = 0;
  static unsigned char receivedData[MAX_BITS / 8];

  const char value = digitalRead(RECV_PIN);

  if (value == lastValue) {
    return;
  }
  lastValue = value;

  const char edge_down = (value == 0);

  const long time = micros();
  const unsigned int duration = time - lastTime;
  lastTime = time;

  if (receiving) {
    if (edge_down) {
      if (currentBit < MAX_BITS) {
        char bit = TEST_DURATION_BIT(duration);
        receivedData[currentBit >> 3] |= (bit << (currentBit & 0b111));
        ++currentBit;
      }
    } else {
      if(TEST_DURATION_END(duration)) {
        if (currentBit == EXPECTED_LEN) {
          HandleData((RemotePacket*)receivedData);
        }
        receiving = 0;
      }
    }
  } else {
    if(edge_down && TEST_DURATION_START(duration)) {
      receiving = 1;
      currentBit = 0;
      for(int i = 0; i < sizeof(receivedData); ++i)
        receivedData[i] = 0;
    }
  }
}

void setup() {
  // pinMode(mouseWheel, INPUT);
  // initialize mouse control:
  Mouse.begin();
  Keyboard.begin();

  Serial.begin(9600);

  pinMode(RECV_PIN, INPUT);
  attachInterrupt(RECV_INT, handleInterrupt, CHANGE);
  sei();
}

#define WHEEL_TICK 100

int lastWheelTick = 0;
signed char wheelValue = 0;

int lastTick = 0;

unsigned char currentButton = 0;
int lastButtonTick = 0;
int lastButtonDownTick = 0;

#define ACTION_DOWN 1
#define ACTION_UP 2
#define ACTION_PRESS 4
#define ACTION_RESET 5

#define BTN_UP   0x88
#define BTN_STP  0xAA
#define BTN_DWN  0xCC

#define LONG_PRESS_DURATION (2000)

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

void loop() {
  int now = millis();

  if(now - lastWheelTick > WHEEL_TICK) {
    lastWheelTick = now;
    Mouse.move(0, 0, wheelValue);
  }

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

    // Serialprint("H %x %x %x\n", (int)currentButton, (int)button, (int)releaseBtn);

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
