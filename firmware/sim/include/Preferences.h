#pragma once
#include "Arduino.h"
#include <map>
#include <string>

class Preferences {
public:
    static std::map<std::string, std::map<std::string, std::string>> _storage;

    std::string _currentNamespace;

    bool begin(const char* name, bool readOnly = false) {
        _currentNamespace = name ? name : "";
        return true;
    }

    void end() {}

    int getInt(const char* key, int defaultValue = 0) {
        if (!key) return defaultValue;
        auto& ns = _storage[_currentNamespace];
        auto it = ns.find(key);
        if (it != ns.end()) {
            try { return std::stoi(it->second); } catch (...) {}
        }
        return defaultValue;
    }

    String getString(const char* key, const String& defaultValue = "") {
        if (!key) return defaultValue;
        auto& ns = _storage[_currentNamespace];
        auto it = ns.find(key);
        if (it != ns.end()) {
            return String(it->second);
        }
        return defaultValue;
    }

    size_t putInt(const char* key, int value) {
        if (!key) return 0;
        _storage[_currentNamespace][key] = std::to_string(value);
        return sizeof(int);
    }

    size_t putString(const char* key, const String& value) {
        if (!key) return 0;
        _storage[_currentNamespace][key] = value.c_str();
        return value.length();
    }
};
