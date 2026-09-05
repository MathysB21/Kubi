#pragma once
#include <cstdint>

class TwoWire {
public:
    bool begin(int sda, int scl) { return true; }
    void setClock(uint32_t freq) {}
};

extern TwoWire Wire;
