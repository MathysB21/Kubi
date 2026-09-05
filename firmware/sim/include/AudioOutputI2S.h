#pragma once
#include <cstdint>

class AudioOutputI2S {
public:
    AudioOutputI2S() {}
    virtual ~AudioOutputI2S() {}

    bool SetPinout(int bclk, int lrclk, int din) { return true; }
    bool SetRate(int hz) { return true; }
    bool SetBitsPerSample(int bits) { return true; }
    bool SetChannels(int ch) { return true; }
    bool SetGain(float gain) { return true; }
    bool ConsumeSample(int16_t sample[2]) { return true; }
};
