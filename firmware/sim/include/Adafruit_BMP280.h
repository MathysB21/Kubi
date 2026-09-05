#pragma once
#include <cstdint>

extern float sim_temperature;

class Adafruit_BMP280 {
public:
    Adafruit_BMP280() {}

    bool begin(uint8_t addr = 0x76, uint8_t chipid = 0x58) {
        return true;
    }

    float readTemperature() {
        return sim_temperature;
    }

    float readPressure() {
        return 101325.0f;
    }
};
