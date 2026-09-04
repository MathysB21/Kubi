#include "API.h"
#include <Preferences.h>
#include <AsyncJson.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>

// Link to the active FSM variables running in main.cpp
extern volatile int userTargetBright;
extern volatile int alarmHour;
extern volatile int alarmMinute;
extern volatile LampState currentState;
extern volatile int dynamicSunriseHour;
extern volatile int dynamicSunriseMinute;
extern volatile int dynamicSundownHour;
extern volatile int dynamicSundownMinute;
extern volatile uint8_t sunriseDays[7];
extern volatile uint8_t sundownDays[7];
extern volatile bool isDaytime;
extern volatile bool forceTimeRecalc;

extern volatile int userTargetTemp;

extern volatile int nightLightBright;
extern volatile int cfgMaxGlow;
extern volatile int cfgTouchThreshold;
extern volatile int cfgProxThreshold;
extern volatile int cfgAmbientThreshold;
extern volatile int cfgSunriseDuration;
extern volatile int cfgSundownDuration;
extern volatile int cfgSundownHour;
extern volatile int cfgSundownMinute;
extern volatile int cfgModeTimeout;
extern volatile int cfgBrightMode;

extern volatile bool isSunriseEnabled;
extern volatile bool isSundownEnabled;
extern volatile int planetDawnH, planetDawnM, planetDuskH, planetDuskM;
extern volatile float geoLat;
extern volatile float geoLon;
extern volatile int tzOffset;

// Diagnostics
extern volatile int diagProx, diagB1, diagB2, diagB3;
extern volatile uint16_t ambientLight;
extern volatile bool cfgProxEnabled;



void setupAPIRoutes(AsyncWebServer& server) {

    // --- GET /api/state ---
    // React calls this on mount to get the current lamp values
    server.on("/api/state", HTTP_GET, [](AsyncWebServerRequest *request){
        AsyncResponseStream *response = request->beginResponseStream("application/json");
        JsonDocument doc; // ArduinoJson v7 syntax
        
        doc["brightness"] = userTargetBright;
        doc["alarmHour"] = alarmHour;
        doc["alarmMinute"] = alarmMinute;
        doc["isSunriseEnabled"] = isSunriseEnabled;
        doc["isSundownEnabled"] = isSundownEnabled;
        doc["sunriseDuration"] = cfgSunriseDuration;
        doc["sundownDuration"] = cfgSundownDuration;
        doc["mode"] = (int)currentState;
        doc["dawnH"] = dynamicSunriseHour;
        doc["dawnM"] = dynamicSunriseMinute;
        doc["duskH"] = dynamicSundownHour;
        doc["duskM"] = dynamicSundownMinute;
        doc["temp"] = userTargetTemp;

        JsonArray srArray = doc["sunriseDays"].to<JsonArray>();
        for(int i=0; i<7; i++) srArray.add(sunriseDays[i]);
        
        JsonArray sdArray = doc["sundownDays"].to<JsonArray>();
        for(int i=0; i<7; i++) sdArray.add(sundownDays[i]);
        
        doc["nightLightBright"] = nightLightBright;
        doc["cfgProxEnabled"] = cfgProxEnabled;
        doc["cfgMaxGlow"] = cfgMaxGlow;
        doc["cfgTouchThreshold"] = cfgTouchThreshold;
        doc["cfgProxThreshold"] = cfgProxThreshold;
        doc["cfgAmbientThreshold"] = cfgAmbientThreshold;
        doc["cfgSundownHour"] = cfgSundownHour;
        doc["cfgSundownMinute"] = cfgSundownMinute;
        doc["cfgModeTimeout"] = cfgModeTimeout;
        doc["cfgBrightMode"] = cfgBrightMode;

        doc["geoLat"] = geoLat;
        doc["geoLon"] = geoLon;
        doc["tzOffset"] = tzOffset;
        doc["planetDawnH"] = planetDawnH;
        doc["planetDawnM"] = planetDawnM;
        doc["planetDuskH"] = planetDuskH;
        doc["planetDuskM"] = planetDuskM;
        
        serializeJson(doc, *response);
        request->send(response);
    });

    // --- POST /api/settings ---
    // React hits this when Shae moves a slider or saves a time
    AsyncCallbackJsonWebHandler* handler = new AsyncCallbackJsonWebHandler("/api/settings", [](AsyncWebServerRequest *request, JsonVariant &json) {
        JsonObject jsonObj = json.as<JsonObject>();

        // 1. Master Brightness
        if (jsonObj["brightness"].is<int>()) {
            userTargetBright = jsonObj["brightness"].as<int>();
        }

        // --- 1.1 TEMPERATURE ---
        if (jsonObj["temp"].is<int>()) {
            userTargetTemp = jsonObj["temp"].as<int>();
        }

        // 1.2 Mode
        if (jsonObj["mode"].is<int>()) {
            int requestedMode = jsonObj["mode"].as<int>();
            
            if (requestedMode == 0) {
                currentState = isDaytime ? STATE_AUTO_DAY : STATE_AUTO_NIGHT; 
            } else if (requestedMode == 2) {
                currentState = isDaytime ? STATE_MANUAL_DAY : STATE_MANUAL_NIGHT;
            } else if (requestedMode == 4) {
                // Night Light: reset to Amber + 50% defaults each time it is activated
                userTargetTemp = 0;
                nightLightBright = 128;
                userTargetBright = 128;
                currentState = STATE_NIGHT_LIGHT;
            } else {
                currentState = (LampState)requestedMode;
            }
        }

        // 2. Update all runtime variables first, then commit everything to NVS in one pass.
        // Opening and closing the Preferences handle once per request (rather than once per key)
        // is required for reliability on the ESP32: the Preferences class is not designed to have
        // its handle recycled many times on a shared global, especially from an async task context.

        if (jsonObj["alarmHour"].is<int>() && jsonObj["alarmMinute"].is<int>()) {
            alarmHour   = jsonObj["alarmHour"].as<int>();
            alarmMinute = jsonObj["alarmMinute"].as<int>();
            forceTimeRecalc = true;
        }
        if (jsonObj["isSunriseEnabled"].is<bool>()) {
            isSunriseEnabled = jsonObj["isSunriseEnabled"].as<bool>();
        }
        if (jsonObj["isSundownEnabled"].is<bool>()) {
            isSundownEnabled = jsonObj["isSundownEnabled"].as<bool>();
        }
        if (jsonObj["sunriseDuration"].is<int>()) {
            cfgSunriseDuration = jsonObj["sunriseDuration"].as<int>();
        }
        if (jsonObj["sundownDuration"].is<int>()) {
            cfgSundownDuration = jsonObj["sundownDuration"].as<int>();
        }
        if (jsonObj["sunriseDays"].is<JsonArray>()) {
            JsonArray arr = jsonObj["sunriseDays"].as<JsonArray>();
            for(int i=0; i<7; i++) sunriseDays[i] = arr[i].as<int>();
            forceTimeRecalc = true;
        }
        if (jsonObj["sundownDays"].is<JsonArray>()) {
            JsonArray arr = jsonObj["sundownDays"].as<JsonArray>();
            for(int i=0; i<7; i++) sundownDays[i] = arr[i].as<int>();
            forceTimeRecalc = true;
        }
        if (jsonObj["geoLat"].is<float>() && jsonObj["geoLon"].is<float>()) {
            geoLat = jsonObj["geoLat"].as<float>();
            geoLon = jsonObj["geoLon"].as<float>();
            forceTimeRecalc = true;
        }
        if (jsonObj["tzOffset"].is<int>()) {
            tzOffset = jsonObj["tzOffset"].as<int>();
            configTime(tzOffset * 3600, 0, "pool.ntp.org");
            forceTimeRecalc = true;
        }
        if (jsonObj["nightLightBright"].is<int>()) {
            nightLightBright = jsonObj["nightLightBright"].as<int>();
            if (currentState == STATE_NIGHT_LIGHT) {
                userTargetBright = nightLightBright;
            }
        }
        if (jsonObj["cfgProxEnabled"].is<bool>()) {
            cfgProxEnabled = jsonObj["cfgProxEnabled"].as<bool>();
        }
        if (jsonObj["cfgMaxGlow"].is<int>()) {
            cfgMaxGlow = jsonObj["cfgMaxGlow"].as<int>();
        }
        if (jsonObj["cfgTouchThreshold"].is<int>()) {
            cfgTouchThreshold = jsonObj["cfgTouchThreshold"].as<int>();
        }
        if (jsonObj["cfgProxThreshold"].is<int>()) {
            cfgProxThreshold = jsonObj["cfgProxThreshold"].as<int>();
        }
        if (jsonObj["cfgAmbientThreshold"].is<int>()) {
            cfgAmbientThreshold = jsonObj["cfgAmbientThreshold"].as<int>();
        }
        if (jsonObj["cfgSundownHour"].is<int>()) {
            cfgSundownHour   = jsonObj["cfgSundownHour"].as<int>();
            cfgSundownMinute = jsonObj["cfgSundownMinute"].as<int>();
            forceTimeRecalc = true;
        }
        if (jsonObj["cfgModeTimeout"].is<int>()) {
            cfgModeTimeout = jsonObj["cfgModeTimeout"].as<int>();
        }
        if (jsonObj["cfgBrightMode"].is<int>()) {
            cfgBrightMode = jsonObj["cfgBrightMode"].as<int>();
        }

        // --- SINGLE ATOMIC NVS COMMIT ---
        // Open once, write every persistent key, close once.
        Preferences prefs;
        prefs.begin("lamp_settings", false);
        prefs.putInt("alarmHour",        alarmHour);
        prefs.putInt("alarmMinute",      alarmMinute);
        prefs.putBool("sunriseEnabled",  isSunriseEnabled);
        prefs.putBool("sundownEnabled",  isSundownEnabled);
        prefs.putInt("sunriseDuration",  cfgSunriseDuration);
        prefs.putInt("sundownDuration",  cfgSundownDuration);
        prefs.putBytes("sunriseDays",    (const void*)sunriseDays, 7);
        prefs.putBytes("sundownDays",    (const void*)sundownDays, 7);
        prefs.putFloat("geoLat",         geoLat);
        prefs.putFloat("geoLon",         geoLon);
        prefs.putInt("tzOffset",         tzOffset);
        prefs.putInt("nightLightBright", nightLightBright);
        prefs.putBool("proxEnabled",     cfgProxEnabled);
        prefs.putInt("cfgMaxGlow",       cfgMaxGlow);
        prefs.putInt("touchThreshold",   cfgTouchThreshold);  // renamed: 14 chars
        prefs.putInt("proxThreshold",    cfgProxThreshold);    // renamed: 13 chars
        prefs.putInt("ambientThresh",    cfgAmbientThreshold); // renamed: 13 chars
        prefs.putInt("cfgSundownHour",   cfgSundownHour);
        prefs.putInt("sundownMinute",    cfgSundownMinute);    // renamed: 13 chars
        prefs.putInt("cfgModeTimeout",   cfgModeTimeout);
        prefs.putInt("cfgBrightMode",    cfgBrightMode);
        prefs.putInt("userBright",       userTargetBright);
        prefs.putInt("userTemp",         userTargetTemp);
        prefs.end();

        request->send(200, "application/json", "{\"status\":\"success\"}");
    });

    // --- GET /api/diagnostics ---
    // A lightweight, manual-only fetch for raw hardware telemetry
    server.on("/api/diagnostics", HTTP_GET, [](AsyncWebServerRequest *request){
        AsyncResponseStream *response = request->beginResponseStream("application/json");
        JsonDocument doc; 
        
        doc["prox"] = diagProx;
        doc["b1"] = diagB1;
        doc["b2"] = diagB2;
        doc["b3"] = diagB3;
        doc["ambient"] = ambientLight;
        doc["stateCode"] = (int)currentState;
        
        serializeJson(doc, *response);
        request->send(response);
    });
    
    server.addHandler(handler);

    // --- SERVE THE REACT FRONTEND ---
    // This tells the server: "If someone asks for a file, look in SPIFFS. If they just go to '/', give them index.html."
    server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");
}