
#include <cppQueue.h>

// #define DECODER_DEBUG

struct DefaultConfig {
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

template<
    typename Config = DefaultConfig
>
class RemoteDecoder {
public:
    RemoteDecoder()
        : inputQueue(sizeof(QueueEntry), Config::QueueSize, FIFO, true, queueData, sizeof(queueData))
    {
    }

    void Begin() {
        self = this;
        pinMode(Config::DigitalPin, INPUT);
        attachInterrupt(Config::Interrupt, HandleInterrupt, CHANGE);
    }

    struct QueueEntry {
        uint8_t bitLength;
        uint8_t data[Config::MaxPacketSize];
    };

    uint8_t HasNext() { return !inputQueue.isEmpty(); }
    uint8_t Next(QueueEntry* data) { return inputQueue.pop(data); }

private:
    static constexpr uint8_t BitDurationTestValueZero = 0;
    static constexpr uint8_t BitDurationTestValueOne = 1;
    static constexpr uint8_t BitDurationTestValueFailure = 2;

    QueueEntry queueData[Config::QueueSize];
    cppQueue	inputQueue;

    QueueEntry inputBuffer;
    uint8_t receiving = 0;
    uint8_t currentBit = 0;
    uint8_t lastValue = 0;
    unsigned long lastTime = 0;

    static RemoteDecoder* self;
    static void HandleInterrupt() {
        uint8_t value = digitalRead(Config::DigitalPin);
        long time = micros();
        self->HandleChange(value, time);
    }

    void HandleChange(uint8_t value, long time) {
        if (value == lastValue) {
            return;
        }
        lastValue = value;

        const char falling_edge = (value == 0);
        const unsigned long duration = time - lastTime;
        lastTime = time;

        if (receiving) {
            if (falling_edge) {
                ReceiveBit(duration);
            } else {
                CheckReceiveCompleted(duration);
            }
        } else {
            if(falling_edge) {
                TryStartReceive(duration);
            }
        }
    }

    void ReceiveBit(unsigned long duration) {
        if (currentBit == Config::MaxPacketSize * 8) {
            StopReceive();
            return;
        }

        switch(TestDataBitDuration(duration)){
            case BitDurationTestValueFailure:
                StopReceive();
                return;
            case BitDurationTestValueOne: {
                uint8_t byte = currentBit >> 3;
                uint8_t bit = currentBit & 0b111;
                inputBuffer.data[byte] |= (1 << bit);
            }
            // fall-through
            case BitDurationTestValueZero:
                ++currentBit;
                return;
        }
    }
    void CheckReceiveCompleted(unsigned long duration) {
        if(TestStopBitDuration(duration)) {
            if (currentBit > 0) {
                inputBuffer.bitLength = currentBit;
                inputQueue.push(&inputBuffer);
            }
            StopReceive();
        }
    }
    void TryStartReceive(unsigned long duration) {
        if (TestStartBitDuration(duration)) {
            for(uint8_t i = 0; i < sizeof(inputBuffer.data); ++i) {
                inputBuffer.data[i] = 0;
            }
            currentBit = 0;
            receiving = 1;
        }
    }

    void StopReceive() {
        receiving = 0;
        currentBit = 0;
    }

    inline uint8_t TestStartBitDuration(unsigned long duration) {
#ifdef DECODER_DEBUG
        if(duration > 4000){
            Serial.print("START ");
            Serial.print(duration);
            Serial.println(" ");
        }
#endif
        enum {
            Min = Config::StartBitDuration - Config::StartBitDurationMargin,
            Max = Config::StartBitDuration + Config::StartBitDurationMargin,
        };
        return (duration > Min) && (duration < Max);
    }
    inline uint8_t TestStopBitDuration(unsigned long duration) {
#ifdef DECODER_DEBUG
        if(duration > 7000){
            Serial.print("STOP ");
            Serial.print(duration);
            Serial.println(" ");
        }
#endif
        enum {
            Min = Config::StopBitDuration - Config::StopBitDurationMargin,
            Max = Config::StopBitDuration + Config::StopBitDurationMargin,
        };
        return (duration > Min) && (duration < Max);
    }

    inline uint8_t TestDataBitDuration(unsigned long duration) {
        enum {
            MinZero = Config::ZeroBitDuration - Config::DataBitDurationMargin,
            MaxZero = Config::ZeroBitDuration + Config::DataBitDurationMargin,

            MinOne = Config::OneBitDuration - Config::DataBitDurationMargin,
            MaxOne = Config::OneBitDuration + Config::DataBitDurationMargin,
        };
        if ((duration > MinZero) && (duration < MaxZero)) {
            return BitDurationTestValueZero;
        }
        if ((duration > MinOne) && (duration < MaxOne)) {
            return BitDurationTestValueOne;
        }

        return BitDurationTestValueFailure;
    }

};

template<typename Config>
RemoteDecoder<Config>* RemoteDecoder<Config>::self;
