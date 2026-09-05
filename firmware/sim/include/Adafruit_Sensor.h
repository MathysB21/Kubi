#pragma once
#include <cstdint>

struct sensors_vec_t {
    float x;
    float y;
    float z;
};

struct sensors_event_t {
    int32_t version;
    int32_t sensor_id;
    int32_t type;
    int32_t reserved0;
    int32_t timestamp;
    union {
        float data[4];
        sensors_vec_t acceleration;
        sensors_vec_t magnetic;
        sensors_vec_t orientation;
        sensors_vec_t gyro;
        float temperature;
        float distance;
        float light;
        float pressure;
        float relative_humidity;
        float current;
        float voltage;
        float color;
    };
};
