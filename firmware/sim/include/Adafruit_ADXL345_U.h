#pragma once
#include "Adafruit_Sensor.h"

#define ADXL345_RANGE_16_G 3

extern float sim_accel_x;
extern float sim_accel_y;
extern float sim_accel_z;

class Adafruit_ADXL345_Unified {
public:
    Adafruit_ADXL345_Unified(int32_t sensorID = -1) {}

    bool begin(uint8_t i2caddr = 0x53) {
        return true;
    }

    void setRange(uint8_t range) {}

    bool getEvent(sensors_event_t* event) {
        if (!event) return false;
        event->acceleration.x = sim_accel_x;
        event->acceleration.y = sim_accel_y;
        event->acceleration.z = sim_accel_z;
        return true;
    }
};
