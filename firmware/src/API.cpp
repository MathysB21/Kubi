#include "API.h"
#include <Preferences.h>
#include <AsyncJson.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <WiFi.h>
#include "PomodoroManager.h"

// --- SHARED GLOBALS (Defined in main.cpp) ---
extern volatile KubiMode currentMode;
extern volatile float roomTemperature;
extern volatile int batteryPercentage;
extern String secretIcalUrl;
extern volatile bool isScreenOverrideActive;
extern String screenOverrideText;
extern bool clockAnalogView;

// Diagnostics
extern volatile float diagAccelX;
extern volatile float diagAccelY;
extern volatile float diagAccelZ;

void setupAPIRoutes(AsyncWebServer& server) {

    // =========================================================================
    // 1. GET /api/state
    // React calls this on mount and during polling intervals to fetch system state
    // =========================================================================
    server.on("/api/state", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncResponseStream *response = request->beginResponseStream("application/json");
        JsonDocument doc;

        // Active State Data Points
        doc["mode"] = (int)currentMode;
        doc["temp"] = roomTemperature;
        doc["battery"] = batteryPercentage;
        doc["icalUrl"] = secretIcalUrl;

        // Detailed Pomodoro State
        JsonObject pomoObj = doc["pomodoro"].to<JsonObject>();
        pomoObj["phase"]         = (int)pomodoro.getPhase();
        pomoObj["phaseName"]     = pomodoro.getPhaseName();
        pomoObj["remaining"]     = pomodoro.getRemainingSeconds();
        pomoObj["total"]         = pomodoro.getTotalSeconds();
        pomoObj["isPaused"]      = pomodoro.isPaused();
        pomoObj["isStarted"]     = pomodoro.isStarted();
        pomoObj["hasChimed"]     = pomodoro.hasChimed();
        pomoObj["cycle"]         = pomodoro.getCompletedCycles();
        pomoObj["cycleTarget"]   = pomodoro.getCycleTarget();
        pomoObj["focusMin"]      = pomodoro.getFocusMinutes();
        pomoObj["shortBreakMin"] = pomodoro.getShortBreakMinutes();
        pomoObj["longBreakMin"]  = pomodoro.getLongBreakMinutes();
        pomoObj["colorWork"]     = pomodoro.getColorWorkHex();
        pomoObj["colorShort"]    = pomodoro.getColorShortBreakHex();
        pomoObj["colorLong"]     = pomodoro.getColorLongBreakHex();

        // Convenience top-level fields for backwards compatibility
        doc["pomodoroFocus"] = pomodoro.getFocusMinutes();
        doc["pomodoroBreak"] = pomodoro.getShortBreakMinutes();
        doc["clockAnalog"] = clockAnalogView;

        serializeJson(doc, *response);
        request->send(response);
    });

    // =========================================================================
    // 2. POST /api/settings
    // React sends user configuration changes (Pomodoro, iCal URL, routines)
    // =========================================================================
    AsyncCallbackJsonWebHandler* settingsHandler = new AsyncCallbackJsonWebHandler("/api/settings", [](AsyncWebServerRequest *request, JsonVariant &json) {
        JsonObject jsonObj = json.as<JsonObject>();

        // 1. Mode Change
        if (jsonObj["mode"].is<int>()) {
            currentMode = (KubiMode)jsonObj["mode"].as<int>();
        }

        // 2. Calendar Sync URL
        if (jsonObj["icalUrl"].is<const char*>()) {
            secretIcalUrl = jsonObj["icalUrl"].as<String>();
            Preferences prefs;
            prefs.begin("kubi_settings", false);
            prefs.putString("icalUrl", secretIcalUrl);
            prefs.end();
        }

        // 3. Pomodoro Durations
        int focus = pomodoro.getFocusMinutes();
        int sBreak = pomodoro.getShortBreakMinutes();
        int lBreak = pomodoro.getLongBreakMinutes();
        int cycles = pomodoro.getCycleTarget();
        bool durationChanged = false;

        if (jsonObj["pomodoroFocus"].is<int>()) { focus = jsonObj["pomodoroFocus"].as<int>(); durationChanged = true; }
        if (jsonObj["focusMin"].is<int>())      { focus = jsonObj["focusMin"].as<int>(); durationChanged = true; }
        if (jsonObj["pomodoroBreak"].is<int>()) { sBreak = jsonObj["pomodoroBreak"].as<int>(); durationChanged = true; }
        if (jsonObj["shortBreakMin"].is<int>()) { sBreak = jsonObj["shortBreakMin"].as<int>(); durationChanged = true; }
        if (jsonObj["longBreakMin"].is<int>())  { lBreak = jsonObj["longBreakMin"].as<int>(); durationChanged = true; }
        if (jsonObj["cycleTarget"].is<int>())   { cycles = jsonObj["cycleTarget"].as<int>(); durationChanged = true; }

        if (durationChanged) {
            pomodoro.setDurations(focus, sBreak, lBreak, cycles);
        }

        // 4. Pomodoro Colors
        String cWork  = pomodoro.getColorWorkHex();
        String cShort = pomodoro.getColorShortBreakHex();
        String cLong  = pomodoro.getColorLongBreakHex();
        bool colorChanged = false;

        if (jsonObj["colorWork"].is<const char*>())  { cWork = jsonObj["colorWork"].as<String>(); colorChanged = true; }
        if (jsonObj["colorShort"].is<const char*>()) { cShort = jsonObj["colorShort"].as<String>(); colorChanged = true; }
        if (jsonObj["colorLong"].is<const char*>())  { cLong = jsonObj["colorLong"].as<String>(); colorChanged = true; }

        if (colorChanged) {
            pomodoro.setColors(cWork, cShort, cLong);
        }

        if (jsonObj["clockAnalog"].is<bool>()) {
            clockAnalogView = jsonObj["clockAnalog"].as<bool>();
            Preferences prefs;
            prefs.begin("kubi_settings", false);
            prefs.putBool("clockAnalog", clockAnalogView);
            prefs.end();
        }

        request->send(200, "application/json", "{\"status\":\"success\"}");
    });
    server.addHandler(settingsHandler);

    // =========================================================================
    // 3. POST /api/pomodoro/action
    // Direct dashboard controls: play, pause, toggle, skip, reset
    // =========================================================================
    AsyncCallbackJsonWebHandler* actionHandler = new AsyncCallbackJsonWebHandler("/api/pomodoro/action", [](AsyncWebServerRequest *request, JsonVariant &json) {
        JsonObject jsonObj = json.as<JsonObject>();
        if (jsonObj["action"].is<const char*>()) {
            String act = jsonObj["action"].as<String>();
            if (act == "play") {
                pomodoro.play();
            } else if (act == "pause") {
                pomodoro.pause();
            } else if (act == "toggle") {
                pomodoro.togglePause();
            } else if (act == "skip") {
                pomodoro.skipPhase();
            } else if (act == "reset") {
                pomodoro.resetCurrent();
            }
            request->send(200, "application/json", "{\"status\":\"success\"}");
        } else {
            request->send(400, "application/json", "{\"error\":\"missing action\"}");
        }
    });
    server.addHandler(actionHandler);

    // =========================================================================
    // 4. POST /api/override
    // Secret creator endpoint: push real-time text alert banners to Kubi's screen
    // =========================================================================
    AsyncCallbackJsonWebHandler* overrideHandler = new AsyncCallbackJsonWebHandler("/api/override", [](AsyncWebServerRequest *request, JsonVariant &json) {
        JsonObject jsonObj = json.as<JsonObject>();

        if (jsonObj["message"].is<const char*>()) {
            screenOverrideText = jsonObj["message"].as<String>();
            isScreenOverrideActive = true;
            Serial.printf("[OVERRIDE] Custom alert received: %s\n", screenOverrideText.c_str());
            request->send(200, "application/json", "{\"status\":\"alert_displayed\"}");
        } else {
            request->send(400, "application/json", "{\"error\":\"missing message field\"}");
        }
    });
    server.addHandler(overrideHandler);

    // =========================================================================
    // 5. GET /api/diagnostics
    // Real-time telemetry: IMU axes, temperature, battery, WiFi signal
    // =========================================================================
    server.on("/api/diagnostics", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncResponseStream *response = request->beginResponseStream("application/json");
        JsonDocument doc;

        doc["accelX"] = diagAccelX;
        doc["accelY"] = diagAccelY;
        doc["accelZ"] = diagAccelZ;
        doc["temp"] = roomTemperature;
        doc["battery"] = batteryPercentage;
        doc["rssi"] = WiFi.RSSI();
        doc["freeHeap"] = ESP.getFreeHeap();
        doc["uptime"] = millis() / 1000;
        doc["mode"] = (int)currentMode;

        serializeJson(doc, *response);
        request->send(response);
    });

    // =========================================================================
    // 6. SERVE STATIC REACT FRONTEND FROM LITTLEFS
    // Default file: index.html
    // =========================================================================
    server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
}
