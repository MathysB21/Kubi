#include "API.h"
#include <Preferences.h>
#include <AsyncJson.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <WiFi.h>
#include "PomodoroManager.h"
#include "DisplayManager.h"
#include "AudioManager.h"
#include "KubiScenes.h"
#include "SceneSchedule.h"
#include "SensorManager.h"
#include <esp_system.h>

// --- SHARED GLOBALS (Defined in main.cpp) ---
extern volatile KubiMode currentMode;
extern volatile float roomTemperature;
extern volatile int batteryPercentage;
extern volatile float batteryVoltage;
extern volatile int batteryStateCode;
extern volatile int batteryMinutesLeft;
extern volatile int chargeAlertHigh;
extern volatile int chargeAlertLow;
extern volatile float roomPressure;

static const char* batteryStateName(int s) {
    // 3 = cells cut off by the charger's protection; 4 = just started, not
    // yet sure whether it is charging (main.cpp updateBattery)
    return s == 4 ? "checking" : s == 3 ? "disconnected" : s == 2 ? "full" : s == 1 ? "charging" : "draining";
}
extern volatile bool isScreenOverrideActive;
extern volatile bool factoryResetRequested;
extern bool clockAnalogView;
extern int mascotSceneIndex;
extern volatile int mascotPreviewScene;
extern volatile uint32_t mascotPreviewAt;
extern volatile uint32_t mascotPreviewMs;
extern volatile int birthdayMMDD;
extern volatile bool napRequested;

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
        doc["batteryVoltage"] = roundf(batteryVoltage * 100.0f) / 100.0f;
        doc["batteryState"] = batteryStateName(batteryStateCode);
        doc["batteryMinutes"] = batteryMinutesLeft; // to empty or to full, -1 = still estimating
        doc["pressure"] = roundf(roomPressure * 10.0f) / 10.0f;
        doc["chargeAlertHigh"] = chargeAlertHigh;
        doc["chargeAlertLow"] = chargeAlertLow;
        doc["mascotScene"] = mascotSceneIndex;
        doc["mascotPreview"] = mascotPreviewScene >= 0; // false: Kubi's own pick
        if (mascotPreviewScene >= 0) {
            uint32_t gone = millis() - mascotPreviewAt;
            doc["mascotPreviewLeftMin"] = gone < mascotPreviewMs ? (int)((mascotPreviewMs - gone + 59999UL) / 60000UL) : 0;
        }
        if (birthdayMMDD > 0) {
            char bday[8];
            snprintf(bday, sizeof(bday), "%02d-%02d", birthdayMMDD / 100, birthdayMMDD % 100);
            doc["birthday"] = bday;
        }

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
        doc["sleepTimeoutMin"] = display.getSleepTimeoutMinutes();
        doc["isSleeping"] = display.isSleeping();
        doc["volume"] = (int)lroundf(audio.getVolume() * 100.0f);

        serializeJson(doc, *response);
        request->send(response);
    });

    // =========================================================================
    // 2. POST /api/settings
    // React sends user configuration changes (face, Pomodoro, clock, sleep)
    // =========================================================================
    AsyncCallbackJsonWebHandler* settingsHandler = new AsyncCallbackJsonWebHandler("/api/settings", [](AsyncWebServerRequest *request, JsonVariant &json) {
        JsonObject jsonObj = json.as<JsonObject>();

        // 1. Mode Change
        // The core-1 loop applies the matching screen rotation; never touch the
        // TFT from this (AsyncTCP) task.
        if (jsonObj["mode"].is<int>()) {
            int mode = jsonObj["mode"].as<int>();
            if (mode >= MODE_CLOCK_IDLE && mode <= MODE_AMBIENT) {
                currentMode = (KubiMode)mode;
                display.requestWake();
            }
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

        // Mascot diorama scene: a preview for KUBI_SCENE_PREVIEW_MS (or
        // "mascotPreviewMin" from the Creator panel), then Kubi picks his own
        // again (-1 hands back at once). The hardware loop applies it; nothing
        // here touches the display. Any scene works, Birthday and Christmas
        // included; the saved birthday date is never touched.
        if (jsonObj["mascotScene"].is<int>() && ALL_KUBI_SCENE_COUNT > 0) {
            int n = (int)ALL_KUBI_SCENE_COUNT;
            int want = jsonObj["mascotScene"].as<int>();
            if (want < 0) {
                mascotPreviewScene = -1;
            } else {
                int minutes = jsonObj["mascotPreviewMin"] | 0;
                if (minutes > MASCOT_PREVIEW_MAX_MIN) minutes = MASCOT_PREVIEW_MAX_MIN;
                mascotPreviewMs = minutes > 0 ? (uint32_t)minutes * 60000UL : KUBI_SCENE_PREVIEW_MS;
                mascotPreviewAt = millis();
                mascotPreviewScene = want % n;
                mascotSceneIndex = want % n;
            }
        }

        // Charge reminders: 0 = off, else the percent to remind at
        for (const char* key : { "chargeAlertHigh", "chargeAlertLow" }) {
            if (!jsonObj[key].is<int>()) continue;
            int pct = jsonObj[key].as<int>();
            pct = pct <= 0 ? 0 : (pct > 100 ? 100 : pct);
            bool high = key[11] == 'H';
            if (high) chargeAlertHigh = pct; else chargeAlertLow = pct;
            Preferences prefs;
            prefs.begin("kubi_settings", false);
            prefs.putInt(high ? "alertHi" : "alertLo", pct);
            prefs.end();
        }

        // Wilhelm's birthday, "MM-DD" ("" clears it): the Birthday scene's day
        if (jsonObj["birthday"].is<const char*>()) {
            const char* b = jsonObj["birthday"].as<const char*>();
            int mm = 0, dd = 0;
            int value = (sscanf(b, "%d-%d", &mm, &dd) == 2 && mm >= 1 && mm <= 12 && dd >= 1 && dd <= 31) ? mm * 100 + dd : 0;
            birthdayMMDD = value;
            Preferences prefs;
            prefs.begin("kubi_settings", false);
            prefs.putInt("bday", value);
            prefs.end();
        }

        // Chime volume, 0-100 %. A tap chime confirms the new level, so the
        // dashboard slider can be set by ear (playChime is safe from any task).
        if (jsonObj["volume"].is<int>()) {
            int v = constrain(jsonObj["volume"].as<int>(), 0, 100);
            audio.setVolume(v / 100.0f);
            Preferences prefs;
            prefs.begin("kubi_settings", false);
            prefs.putInt("volume", v);
            prefs.end();
            audio.playChime(CHIME_TAP_FEEDBACK);
        }

        // 5. Screen sleep timeout (minutes, 0 = never)
        if (jsonObj["sleepTimeoutMin"].is<int>()) {
            display.setSleepTimeoutMinutes(jsonObj["sleepTimeoutMin"].as<int>());
            Preferences prefs;
            prefs.begin("kubi_settings", false);
            prefs.putInt("sleepMin", display.getSleepTimeoutMinutes());
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
            const char* message = jsonObj["message"].as<const char*>();
            if (!*message) {
                isScreenOverrideActive = false; // empty message: take the banner down
                request->send(200, "application/json", "{\"status\":\"alert_cleared\"}");
                return;
            }
            char text[OVERRIDE_TEXT_MAX];
            composeBanner(text, sizeof(text), jsonObj["title"] | "", message);
            setOverrideText(text);
            isScreenOverrideActive = true; // Set after the text so the reader never sees a stale banner
            display.requestWake();
            Serial.printf("[OVERRIDE] Custom alert received: %s\n", getOverrideText().c_str());
            request->send(200, "application/json", "{\"status\":\"alert_displayed\"}");
        } else {
            request->send(400, "application/json", "{\"error\":\"missing message field\"}");
        }
    });
    server.addHandler(overrideHandler);

    // =========================================================================
    // 4b. POST /api/chime {"chime": "tap"|"done"|"fanfare"|"ping"|"ouch"}
    // Creator panel: play any chime. playChime() is safe from any task.
    // =========================================================================
    AsyncCallbackJsonWebHandler* chimeHandler = new AsyncCallbackJsonWebHandler("/api/chime", [](AsyncWebServerRequest *request, JsonVariant &json) {
        KubiChime chime = chimeByName(json["chime"] | "");
        if (chime == CHIME_NONE) {
            request->send(400, "application/json", "{\"error\":\"unknown chime\"}");
            return;
        }
        audio.playChime(chime);
        request->send(200, "application/json", "{\"status\":\"playing\"}");
    });
    server.addHandler(chimeHandler);

    // =========================================================================
    // 4c. Motion recorder, for tuning gestures on the real cube
    // POST /api/motion-log {"seconds": 1-30} starts a recording.
    // GET  /api/motion-log: {"state":0|1} while idle or recording; once done,
    // CSV "t,x,y,z,delta,total,flags" (ms; centi-m/s^2; MotionFlag bits).
    // =========================================================================
    AsyncCallbackJsonWebHandler* motionLogStart = new AsyncCallbackJsonWebHandler("/api/motion-log", [](AsyncWebServerRequest *request, JsonVariant &json) {
        int seconds = json["seconds"] | 20;
        if (seconds < 1) seconds = 1;
        if (seconds > 30) seconds = 30;
        sensors.requestMotionLog((uint16_t)seconds);
        request->send(200, "application/json", "{\"status\":\"recording\"}");
    });
    motionLogStart->setMethod(HTTP_POST); // GET below downloads the log
    server.addHandler(motionLogStart);
    server.on("/api/motion-log", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (sensors.motionLogState() != 2) {
            char buf[32];
            snprintf(buf, sizeof(buf), "{\"state\":%d}", sensors.motionLogState());
            request->send(200, "application/json", buf);
            return;
        }
        // Chunked: up to 1500 lines would not fit one buffer
        AsyncWebServerResponse* response = request->beginChunkedResponse("text/csv",
            [](uint8_t* out, size_t maxLen, size_t index) -> size_t {
                static int row = 0;
                if (index == 0) row = -1; // header first
                int count = 0;
                const MotionSample* log = sensors.motionLog(count);
                size_t len = 0;
                while (row < count) {
                    char line[64];
                    int n;
                    if (row < 0) n = snprintf(line, sizeof(line), "t,x,y,z,delta,total,flags\n");
                    else {
                        const MotionSample& m = log[row];
                        n = snprintf(line, sizeof(line), "%lu,%d,%d,%d,%d,%d,%u\n", (unsigned long)m.t,
                                     m.x, m.y, m.z, m.delta, m.total, (unsigned)m.flags);
                    }
                    if (len + n > maxLen) break;
                    memcpy(out + len, line, n);
                    len += n;
                    row++;
                }
                return len;
            });
        request->send(response);
    });

    // =========================================================================
    // 5. GET /api/diagnostics
    // Real-time telemetry: IMU axes, temperature, battery, WiFi signal
    // =========================================================================
    server.on("/api/diagnostics", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncResponseStream *response = request->beginResponseStream("application/json");
        JsonDocument doc;

        doc["cpuMhz"] = getCpuFrequencyMhz();
        doc["accelX"] = diagAccelX;
        doc["accelY"] = diagAccelY;
        doc["accelZ"] = diagAccelZ;
        doc["temp"] = roomTemperature;
        doc["battery"] = batteryPercentage;
        doc["batteryVoltage"] = roundf(batteryVoltage * 100.0f) / 100.0f;
        doc["batteryState"] = batteryStateName(batteryStateCode);
        doc["batteryMinutes"] = batteryMinutesLeft; // to empty or to full, -1 = still estimating
        doc["pressure"] = roundf(roomPressure * 10.0f) / 10.0f;
        doc["chargeAlertHigh"] = chargeAlertHigh;
        doc["chargeAlertLow"] = chargeAlertLow;
        doc["rssi"] = WiFi.RSSI();
        doc["freeHeap"] = ESP.getFreeHeap();
        doc["uptime"] = millis() / 1000;
        // Why the last restart happened (an unexpected one shows as panic,
        // brownout or watchdog rather than poweron/software)
        static const char* const RESET_NAMES[] = { "unknown", "poweron", "external", "software", "panic",
            "int_wdt", "task_wdt", "wdt", "deepsleep", "brownout", "sdio" };
        int rr = (int)esp_reset_reason();
        doc["resetReason"] = (rr >= 0 && rr <= 10) ? RESET_NAMES[rr] : "other";
        doc["mode"] = (int)currentMode;

        serializeJson(doc, *response);
        request->send(response);
    });

    // =========================================================================
    // POST /api/nap: Kubi says good night and deep-sleeps until picked up.
    // Only sets the flag; the hardware loop draws, configures and sleeps.
    // =========================================================================
    server.on("/api/nap", HTTP_POST, [](AsyncWebServerRequest *request) {
        napRequested = true;
        request->send(200, "application/json", "{\"status\":\"napping\"}");
    });

    // =========================================================================
    // 6. POST /api/factory-reset
    // Wipes settings, saved networks and stored WiFi, then restarts into setup.
    // Requires {"confirm":"ERASE"} so a stray request cannot trigger it.
    // =========================================================================
    AsyncCallbackJsonWebHandler* resetHandler = new AsyncCallbackJsonWebHandler("/api/factory-reset", [](AsyncWebServerRequest *request, JsonVariant &json) {
        JsonObject jsonObj = json.as<JsonObject>();
        const char* confirm = jsonObj["confirm"] | "";
        if (strcmp(confirm, FACTORY_RESET_CONFIRM) == 0) {
            factoryResetRequested = true; // hardware loop does the wipe after we reply
            request->send(200, "application/json", "{\"status\":\"resetting\"}");
        } else {
            request->send(400, "application/json", "{\"error\":\"confirm required\"}");
        }
    });
    server.addHandler(resetHandler);

    // =========================================================================
    // 7. SERVE STATIC REACT FRONTEND FROM LITTLEFS /www
    // Only the web build lives under /www (Vite outDir firmware/data/www), so
    // nothing else on the filesystem is reachable over HTTP.
    // =========================================================================
    server.serveStatic("/", LittleFS, "/www/").setDefaultFile("index.html");

    // Client-side routes (/manual) have no file: hand them the SPA shell.
    server.onNotFound([](AsyncWebServerRequest *request) {
        if (request->method() == HTTP_GET && !request->url().startsWith("/api/")) {
            request->send(LittleFS, "/www/index.html", "text/html");
        } else {
            request->send(404, "application/json", "{\"error\":\"not found\"}");
        }
    });
}
