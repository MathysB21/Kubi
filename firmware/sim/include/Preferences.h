#pragma once
#include "Arduino.h"
#include <map>
#include <string>
#include <fstream>

class Preferences {
public:
    static std::map<std::string, std::map<std::string, std::string>> _storage;
    static bool _loaded;

    std::string _currentNamespace;

    static void _ensureLoaded() {
        if (_loaded) return;
        _loaded = true;
        std::ifstream f("kubi_sim_prefs.txt");
        if (!f.is_open()) return;
        std::string line;
        while (std::getline(f, line)) {
            auto colon = line.find(':');
            auto eq = line.find('=');
            if (colon != std::string::npos && eq != std::string::npos && eq > colon) {
                std::string ns = line.substr(0, colon);
                std::string k = line.substr(colon + 1, eq - colon - 1);
                std::string v = line.substr(eq + 1);
                _storage[ns][k] = v;
            }
        }
    }

    static void _save() {
        std::ofstream f("kubi_sim_prefs.txt");
        if (!f.is_open()) return;
        for (const auto& nsPair : _storage) {
            for (const auto& kv : nsPair.second) {
                f << nsPair.first << ":" << kv.first << "=" << kv.second << "\n";
            }
        }
    }

    bool begin(const char* name, bool readOnly = false) {
        _ensureLoaded();
        _currentNamespace = name ? name : "";
        return true;
    }

    void end() {
        _save();
    }

    int getInt(const char* key, int defaultValue = 0) {
        if (!key) return defaultValue;
        _ensureLoaded();
        auto& ns = _storage[_currentNamespace];
        auto it = ns.find(key);
        if (it != ns.end()) {
            try { return std::stoi(it->second); } catch (...) {}
        }
        return defaultValue;
    }

    String getString(const char* key, const String& defaultValue = "") {
        if (!key) return defaultValue;
        _ensureLoaded();
        auto& ns = _storage[_currentNamespace];
        auto it = ns.find(key);
        if (it != ns.end()) {
            return String(it->second);
        }
        return defaultValue;
    }

    size_t putInt(const char* key, int value) {
        if (!key) return 0;
        _ensureLoaded();
        _storage[_currentNamespace][key] = std::to_string(value);
        _save();
        return sizeof(int);
    }

    size_t putString(const char* key, const String& value) {
        if (!key) return 0;
        _ensureLoaded();
        _storage[_currentNamespace][key] = value.c_str();
        _save();
        return value.length();
    }

    bool getBool(const char* key, bool defaultValue = false) {
        if (!key) return defaultValue;
        _ensureLoaded();
        auto& ns = _storage[_currentNamespace];
        auto it = ns.find(key);
        if (it != ns.end()) {
            return (it->second == "1" || it->second == "true");
        }
        return defaultValue;
    }

    size_t putBool(const char* key, bool value) {
        if (!key) return 0;
        _ensureLoaded();
        _storage[_currentNamespace][key] = value ? "1" : "0";
        _save();
        return sizeof(bool);
    }
};
