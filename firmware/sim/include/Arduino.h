#pragma once

#include <cstdint>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <string>
#include <sstream>
#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <ctime>
#include <algorithm>
#include "pgmspace.h"

// Arduino constants
#ifdef _WIN32
#ifdef INPUT
#undef INPUT
#endif
#endif
#define HIGH 1
#define LOW  0
#define INPUT 0
#define OUTPUT 1
#define INPUT_PULLUP 2

#ifndef _WIN32
typedef bool boolean;
#endif
typedef uint8_t byte;

// Millis and Micros
inline uint32_t millis() {
    static auto start = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    return (uint32_t)std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
}

inline uint32_t micros() {
    static auto start = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    return (uint32_t)std::chrono::duration_cast<std::chrono::microseconds>(now - start).count();
}

inline void delay(uint32_t ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

inline void delayMicroseconds(uint32_t us) {
    std::this_thread::sleep_for(std::chrono::microseconds(us));
}

inline void yield() {}

// Arduino String wrapper
class String {
public:
    std::string _str;

    String() : _str("") {}
    String(const char* s) : _str(s ? s : "") {}
    String(const std::string& s) : _str(s) {}
    String(char c) : _str(1, c) {}
    String(int v) : _str(std::to_string(v)) {}
    String(unsigned int v) : _str(std::to_string(v)) {}
    String(long v) : _str(std::to_string(v)) {}
    String(unsigned long v) : _str(std::to_string(v)) {}
    String(float v, int decimals = 2) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%.*f", decimals, v);
        _str = buf;
    }
    String(double v, int decimals = 2) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%.*f", decimals, v);
        _str = buf;
    }

    const char* c_str() const { return _str.c_str(); }
    size_t length() const { return _str.length(); }
    bool isEmpty() const { return _str.empty(); }

    char operator[](size_t idx) const { return _str[idx]; }
    char& operator[](size_t idx) { return _str[idx]; }
    char charAt(size_t idx) const { return (idx < _str.length()) ? _str[idx] : 0; }

    bool operator==(const String& rhs) const { return _str == rhs._str; }
    bool operator==(const char* rhs) const { return _str == (rhs ? rhs : ""); }
    bool operator!=(const String& rhs) const { return _str != rhs._str; }
    bool operator!=(const char* rhs) const { return _str != (rhs ? rhs : ""); }
    bool operator<(const String& rhs) const { return _str < rhs._str; }
    bool operator>(const String& rhs) const { return _str > rhs._str; }
    bool operator<=(const String& rhs) const { return _str <= rhs._str; }
    bool operator>=(const String& rhs) const { return _str >= rhs._str; }
    bool operator<(const char* rhs) const { return _str < (rhs ? rhs : ""); }

    String operator+(const String& rhs) const { return String(_str + rhs._str); }
    String operator+(const char* rhs) const { return String(_str + (rhs ? rhs : "")); }
    String operator+(int rhs) const { return String(_str + std::to_string(rhs)); }
    String& operator+=(const String& rhs) { _str += rhs._str; return *this; }
    String& operator+=(const char* rhs) { if (rhs) _str += rhs; return *this; }
    String& operator+=(char c) { _str += c; return *this; }

    String substring(size_t from, size_t to = (size_t)-1) const {
        if (from >= _str.length()) return String("");
        if (to == (size_t)-1 || to > _str.length()) to = _str.length();
        if (to <= from) return String("");
        return String(_str.substr(from, to - from));
    }

    int toInt() const {
        try { return std::stoi(_str); } catch (...) { return 0; }
    }

    float toFloat() const {
        try { return std::stof(_str); } catch (...) { return 0.0f; }
    }

    int indexOf(char c, size_t from = 0) const {
        size_t pos = _str.find(c, from);
        return (pos == std::string::npos) ? -1 : (int)pos;
    }

    int indexOf(const String& s, size_t from = 0) const {
        size_t pos = _str.find(s._str, from);
        return (pos == std::string::npos) ? -1 : (int)pos;
    }

    bool startsWith(const String& prefix) const {
        if (prefix.length() > length()) return false;
        return _str.compare(0, prefix.length(), prefix._str) == 0;
    }

    void toLowerCase() {
        for (auto& c : _str) c = (char)std::tolower(c);
    }

    void toUpperCase() {
        for (auto& c : _str) c = (char)std::toupper(c);
    }
};

inline String operator+(const char* lhs, const String& rhs) {
    return String((lhs ? lhs : "") + rhs._str);
}

// Stream / Serial Mock
class SimSerial {
public:
    void begin(unsigned long baud) {}
    void print(const char* s) { std::cout << (s ? s : ""); std::cout.flush(); }
    void print(const String& s) { std::cout << s.c_str(); std::cout.flush(); }
    void print(int n) { std::cout << n; std::cout.flush(); }
    void print(float f) { std::cout << f; std::cout.flush(); }
    void println() { std::cout << std::endl; }
    void println(const char* s) { std::cout << (s ? s : "") << std::endl; }
    void println(const String& s) { std::cout << s.c_str() << std::endl; }
    void println(int n) { std::cout << n << std::endl; }
    void println(float f) { std::cout << f << std::endl; }

    template<typename... Args>
    void printf(const char* format, Args... args) {
        ::printf(format, args...);
        fflush(stdout);
    }
};

extern SimSerial Serial;

// LEDC PWM (Backlight control)
extern uint8_t sim_backlight_value;
inline void ledcSetup(uint8_t channel, uint32_t freq, uint8_t res) {}
inline void ledcAttachPin(uint8_t pin, uint8_t channel) {}
inline void ledcWrite(uint8_t channel, uint8_t value) {
    sim_backlight_value = value;
}

// Simulated Time
extern int sim_hour_override;
extern int sim_minute_override;

inline bool getLocalTime(struct tm* info) {
    time_t rawtime;
    time(&rawtime);
    struct tm* ptm = localtime(&rawtime);
    if (ptm && info) {
        *info = *ptm;
        if (sim_hour_override >= 0) info->tm_hour = sim_hour_override;
        if (sim_minute_override >= 0) info->tm_min = sim_minute_override;
        return true;
    }
    return false;
}

// ESP object stub
class SimESP {
public:
    uint32_t getFreeHeap() { return 184320; }
    void restart() { std::cout << "[SIM] ESP.restart() called" << std::endl; }
};

extern SimESP ESP;
