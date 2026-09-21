#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>
#include <string>
#include <sstream>
#include <mutex>
#include <cstring>

#include "httplib.h"
#include "Arduino.h"
#include "Preferences.h"
#include "Wire.h"
#include "TFT_eSPI.h"
#include "DisplayManager.h"
#include "SensorManager.h"
#include "AudioManager.h"
#include "PomodoroManager.h"
#include "AmbientFace.h"
#include "MazeGame.h"
#include "FaceMap.h"
#include "API.h"
#include "ArduinoJson.h"

// --- Global Simulation Controls ---
float sim_accel_x = 0.0f;
float sim_accel_y = 0.0f;
float sim_accel_z = 9.8f;
float sim_temperature = 22.0f;
float sim_tilt_roll = 0.0f;   // degrees, + = right side down (see FaceMap.h tiltedGravity)
float sim_tilt_pitch = 0.0f;  // degrees, + = top edge away
int sim_hour_override = -1;
int sim_minute_override = -1;
uint8_t sim_backlight_value = 255;
std::string sim_last_chime_name = "NONE";
uint32_t sim_last_chime_time = 0;
static std::atomic<KubiGesture> sim_injected_gesture(GESTURE_NONE);

// Serial & ESP Stubs
SimSerial Serial;
SimESP ESP;
TwoWire Wire;
std::map<std::string, std::map<std::string, std::string>> Preferences::_storage;
bool Preferences::_loaded = false;

// Shared Kubi runtime state globals (matching main.cpp)
volatile KubiMode currentMode            = MODE_CLOCK_IDLE;
volatile float    roomTemperature        = 22.0f;
volatile int      batteryPercentage      = 100;
volatile bool     isScreenOverrideActive = false;
volatile bool     factoryResetRequested  = false;

// Mirrors main.cpp first-connect address screen
static uint32_t addressScreenUntil = 0;
static String   addressScreenIp;

// Mirrors main.cpp setOverrideText()/getOverrideText() (portMUX there)
static char       overrideTextBuf[OVERRIDE_TEXT_MAX] = "";
static std::mutex overrideTextMutex;

void setOverrideText(const char* text) {
    size_t len = strnlen(text, OVERRIDE_TEXT_MAX - 1);
    std::lock_guard<std::mutex> lock(overrideTextMutex);
    memcpy(overrideTextBuf, text, len);
    overrideTextBuf[len] = '\0';
}

String getOverrideText() {
    char local[OVERRIDE_TEXT_MAX];
    {
        std::lock_guard<std::mutex> lock(overrideTextMutex);
        memcpy(local, overrideTextBuf, OVERRIDE_TEXT_MAX);
    }
    return String(local);
}

volatile float diagAccelX = 0.0f;
volatile float diagAccelY = 0.0f;
volatile float diagAccelZ = 9.8f;

// --- CLOCK SETTINGS ---
static bool clockAnalogView = false;

// --- FACE 4 --- (mirrors main.cpp)
bool face4Maze = false;

// Helper: Convert 16-bit RGB565 to 32-bit RGBA (for web canvas)
static void convertRGB565toRGBA32(const uint16_t* src, uint8_t* dst, int count) {
    for (int i = 0; i < count; i++) {
        uint16_t p = src[i];
        // ST7789 RGB565: RRRRR GGGGGG BBBBB
        uint8_t r = ((p >> 11) & 0x1F) * 255 / 31;
        uint8_t g = ((p >> 5) & 0x3F) * 255 / 63;
        uint8_t b = (p & 0x1F) * 255 / 31;

        dst[i * 4 + 0] = r;
        dst[i * 4 + 1] = g;
        dst[i * 4 + 2] = b;
        dst[i * 4 + 3] = 255;
    }
}

// Background Hardware Simulation Thread
static std::atomic<bool> sim_running(true);

void simFactoryReset();

void hardwareSimulationThread() {
    std::cout << "[SIM] Core 1 Hardware Loop Started." << std::endl;
    uint32_t lastRenderTime = 0;
    uint32_t lastPomoTick = 0;
    uint32_t lastMotionSeen = 0;
    int previousFace = -1;
    bool mazeWasActive = false;
    bool mazeWasLocked = false;

    while (sim_running) {
        uint32_t now = millis();

        if (factoryResetRequested) {
            simFactoryReset();
            now = millis();
        }

        // 1. Poll Sensors & Display (audio runs in its own thread, like main.cpp audioTask)
        sensors.loop();
        display.loop();

        // Synchronize active firmware chime to simulator telemetry
        static KubiChime prevActiveChime = CHIME_NONE;
        KubiChime curChime = audio.getActiveChime();
        if (curChime != prevActiveChime) {
            prevActiveChime = curChime;
            if (curChime != CHIME_NONE) {
                sim_last_chime_time = now;
                switch (curChime) {
                    case CHIME_TAP_FEEDBACK: sim_last_chime_name = "CHIME_TAP_FEEDBACK"; break;
                    case CHIME_WAKE_PING: sim_last_chime_name = "CHIME_WAKE_PING"; break;
                    case CHIME_POMODORO_DONE: sim_last_chime_name = "CHIME_POMODORO_DONE"; break;
                    case CHIME_POMODORO_LONG_BREAK: sim_last_chime_name = "CHIME_POMODORO_LONG_BREAK"; break;
                    case CHIME_SLAM_OUCH: sim_last_chime_name = "CHIME_SLAM_OUCH"; break;
                    default: break;
                }
            }
        }

        // 2. 1-second Pomodoro Countdown Tick
        if (now - lastPomoTick >= 1000) {
            lastPomoTick = now;
            pomodoro.tick();
        }

        // 3. Telemetry sync
        sensors.getAcceleration((float&)diagAccelX, (float&)diagAccelY, (float&)diagAccelZ);
        roomTemperature = sensors.getTemperature();

        // 4. Orientation / Face Up State Machine
        int activeFace = sensors.getActiveFace();
        if (activeFace != previousFace) {
            previousFace = activeFace;
            if (activeFace >= 0 && activeFace <= 3) {
                currentMode = (KubiMode)activeFace;
                display.setRotationForFace(activeFace);
                std::cout << "[KUBI ORIENTATION] >>> Face " << (activeFace + 1)
                          << " UP: Screen rotated upright <<<" << std::endl;

                sim_injected_gesture = GESTURE_NONE;
                sensors.getRecentGesture();
                display.noteActivity();
            }
        }

        // Mirrors main.cpp: rotation follows currentMode, applied on this thread only
        display.setRotationForFace((int)currentMode);

        bool mazeActive = mazeFaceActive(currentMode);
        if (mazeActive && !mazeWasActive) maze.begin();
        mazeWasActive = mazeActive;
        // Face lock while playing (mirrors main.cpp)
        bool mazeLocked = mazeActive && maze.isPlaying(now);
        sensors.setFaceSettleTime(mazeLocked ? FACE_SETTLE_LOCKED : FACE_SETTLE_MS);
        if (mazeLocked != mazeWasLocked) {
            std::cout << "[MAZE] Face lock " << (mazeLocked ? "on" : "off") << std::endl;
            mazeWasLocked = mazeLocked;
        }
        if (mazeActive) {
            float ax, ay, az;
            sensors.getAcceleration(ax, ay, az);
            maze.update(ax, ay, az, now);
        }

        // 5. Gesture Handling
        KubiGesture gesture = sensors.getRecentGesture();
        KubiGesture injected = sim_injected_gesture.exchange(GESTURE_NONE);
        if (injected != GESTURE_NONE) {
            gesture = injected;
        }
        if (gesture != GESTURE_NONE) {
            if (display.noteActivity()) {
                std::cout << "[SIM GESTURE] Gesture woke the screen (swallowed)." << std::endl;
            } else {
                switch (gesture) {
                    case GESTURE_TAP:
                        std::cout << "[SIM GESTURE] Tap detected." << std::endl;
                        if (addressScreenUntil) {
                            addressScreenUntil = 0;
                            audio.playChime(CHIME_TAP_FEEDBACK);
                            sim_last_chime_name = "CHIME_TAP_FEEDBACK";
                            sim_last_chime_time = now;
                        } else if (isScreenOverrideActive) {
                            isScreenOverrideActive = false;
                            audio.playChime(CHIME_TAP_FEEDBACK);
                            sim_last_chime_name = "CHIME_TAP_FEEDBACK";
                            sim_last_chime_time = now;
                        } else if (currentMode == MODE_POMODORO) {
                            pomodoro.handleTap();
                            sim_last_chime_name = "CHIME_TAP_FEEDBACK";
                            sim_last_chime_time = now;
                        } else if (currentMode == MODE_CLOCK_IDLE) {
                            audio.playChime(CHIME_TAP_FEEDBACK);
                            sim_last_chime_name = "CHIME_TAP_FEEDBACK";
                            sim_last_chime_time = now;
                        } else if (currentMode == MODE_AMBIENT && !face4Maze) {
                            ambient.nextColour(); // silent, mirrors main.cpp
                        }
                        break;

                    case GESTURE_SHAKE:
                        std::cout << "[SIM GESTURE] Shake detected." << std::endl;
                        if (currentMode == MODE_POMODORO) {
                            pomodoro.handleShake();
                        } else if (currentMode == MODE_CLOCK_IDLE) {
                            clockAnalogView = !clockAnalogView;
                            Preferences prefs;
                            prefs.begin("kubi_settings", false);
                            prefs.putBool("clockAnalog", clockAnalogView);
                            prefs.end();
                            audio.playChime(CHIME_TAP_FEEDBACK);
                            sim_last_chime_name = "CHIME_TAP_FEEDBACK";
                            sim_last_chime_time = now;
                            std::cout << "[SIM CLOCK] Shake toggled view -> " << (clockAnalogView ? "analog" : "digital") << " (saved to Preferences)" << std::endl;
                        }
                        break;

                    case GESTURE_SLAM:
                        std::cout << "[SIM GESTURE] Desk slam detected!" << std::endl;
                        audio.playChime(CHIME_SLAM_OUCH);
                        sim_last_chime_name = "CHIME_SLAM_OUCH";
                        sim_last_chime_time = now;
                        break;

                    default:
                        break;
                }
            }
        }

        // 5b. Inactivity sleep (mirrors main.cpp)
        uint32_t motionTime = sensors.getLastMotionTime();
        if (motionTime != lastMotionSeen) {
            lastMotionSeen = motionTime;
            display.noteActivity();
        }
        if (addressScreenUntil && (int32_t)(now - addressScreenUntil) >= 0) {
            addressScreenUntil = 0;
        }
        bool bannerUp = isScreenOverrideActive || addressScreenUntil;
        display.setSleepAllowed(modeMaySleep(currentMode) && !bannerUp);
        display.setAwakeBrightness(currentMode == MODE_AMBIENT && !face4Maze && !bannerUp ? AmbientFace::BACKLIGHT : 255);

        // 6. Display Rendering: same ticks as hardware (was 40 ms here, 50 there)
        uint32_t renderTick = mazeActive ? RENDER_TICK_MAZE_MS : RENDER_TICK_MS;
        if (now - lastRenderTime >= renderTick && !display.isSleeping()) {
            lastRenderTime = now;

            if (isScreenOverrideActive) {
                display.drawOverrideAlert(getOverrideText());
            } else if (addressScreenUntil) {
                display.drawAddressScreen(addressScreenIp);
            } else {
                struct tm timeinfo;
                int currentHour = 12, currentMin = 0;
                int currentWday = 4, currentMday = 3, currentMon = 8;
                if (getLocalTime(&timeinfo)) {
                    currentHour = timeinfo.tm_hour;
                    currentMin = timeinfo.tm_min;
                    currentWday = timeinfo.tm_wday;
                    currentMday = timeinfo.tm_mday;
                    currentMon = timeinfo.tm_mon;
                }

                switch (currentMode) {
                    case MODE_CLOCK_IDLE:
                        display.drawClockFace(currentHour, currentMin, currentWday, currentMday, currentMon, clockAnalogView);
                        break;

                    case MODE_POMODORO:
                        display.drawPomodoroFace(
                            pomodoro.getRemainingSeconds(),
                            pomodoro.getTotalSeconds(),
                            pomodoro.getPhaseName(),
                            pomodoro.isPaused(),
                            pomodoro.isStarted(),
                            pomodoro.getPhaseColor(),
                            pomodoro.getCompletedCycles(),
                            pomodoro.getCycleTarget()
                        );
                        break;

                    case MODE_MASCOT_ROUTINE:
                        display.drawMascotFace(roomTemperature, currentHour);
                        break;

                    case MODE_AMBIENT:
                        if (face4Maze) display.drawMazeFace();
                        else display.drawAmbientFace();
                        break;

                    default:
                        break;
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

// Mirrors main.cpp performFactoryReset(). The sim cannot reboot, so after the
// wipe it reloads defaults and replays onboarding: the setup screen (as if
// the portal were up) and then the first-connect address screen.
void simFactoryReset() {
    std::cout << "[SIM RESET] Factory reset: wiping kubi_settings and wifi_memory" << std::endl;
    factoryResetRequested = false;
    display.setAwakeBrightness(255);
    display.setSleep(false);
    display.drawResetScreen();

    Preferences prefs;
    prefs.begin("kubi_settings", false);
    prefs.clear();
    prefs.end();
    prefs.begin("wifi_memory", false);
    prefs.clear();
    prefs.end();
    std::this_thread::sleep_for(std::chrono::milliseconds(2500));

    // "Reboot": defaults as a fresh cube would load them
    clockAnalogView = false;
    display.setSleepTimeoutMinutes(5);
    pomodoro.init();
    isScreenOverrideActive = false;

    display.drawSetupScreen();
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    addressScreenIp = "127.0.0.1";
    addressScreenUntil = millis() + ADDRESS_SCREEN_MS;
}

// Mirrors main.cpp audioTask: paced by the mock I2S queue draining at the sample rate
void audioThreadMain() {
    while (sim_running) {
        if (!audio.pump()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }
}

// Add CORS headers helper
void addCors(httplib::Response& res) {
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS, PUT, DELETE");
    res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "    KUBI DESKTOP C++ SIMULATOR RUNNER   " << std::endl;
    std::cout << "========================================" << std::endl;

    // 1. Initialize Drivers & Load Settings
    Preferences prefs;
    prefs.begin("kubi_settings", false);
    clockAnalogView = prefs.getBool("clockAnalog", false);
    face4Maze = prefs.getBool("face4Maze", false);
    display.setSleepTimeoutMinutes(prefs.getInt("sleepMin", 5));
    prefs.end();

    display.init();
    sensors.init();
    audio.init();
    pomodoro.init();

    // 2. Start hardware simulation loop and audio pump
    std::thread audioThread(audioThreadMain);
    std::thread hwThread(hardwareSimulationThread);

    // 3. Setup HTTP server
    httplib::Server svr;

    // CORS pre-flight
    svr.Options(".*", [](const httplib::Request& req, httplib::Response& res) {
        addCors(res);
        res.status = 204;
    });

    // -------------------------------------------------------------------------
    // 1. GET /api/state (React Dashboard API)
    // -------------------------------------------------------------------------
    svr.Get("/api/state", [](const httplib::Request& req, httplib::Response& res) {
        addCors(res);
        JsonDocument doc;

        doc["mode"] = (int)currentMode;
        doc["temp"] = roomTemperature;
        doc["battery"] = batteryPercentage;

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
        pomoObj["colorWork"]     = pomodoro.getColorWorkHex().c_str();
        pomoObj["colorShort"]    = pomodoro.getColorShortBreakHex().c_str();
        pomoObj["colorLong"]     = pomodoro.getColorLongBreakHex().c_str();

        doc["pomodoroFocus"] = pomodoro.getFocusMinutes();
        doc["pomodoroBreak"] = pomodoro.getShortBreakMinutes();
        doc["clockAnalog"] = clockAnalogView;
        doc["sleepTimeoutMin"] = display.getSleepTimeoutMinutes();
        doc["isSleeping"] = display.isSleeping();
        doc["face4Maze"] = face4Maze;

        std::string jsonStr;
        serializeJson(doc, jsonStr);
        res.set_content(jsonStr, "application/json");
    });

    // -------------------------------------------------------------------------
    // 2. POST /api/settings (React Dashboard API)
    // -------------------------------------------------------------------------
    svr.Post("/api/settings", [](const httplib::Request& req, httplib::Response& res) {
        addCors(res);
        JsonDocument json;
        DeserializationError err = deserializeJson(json, req.body);
        if (err) {
            res.status = 400;
            res.set_content("{\"error\":\"invalid json\"}", "application/json");
            return;
        }

        JsonObject jsonObj = json.as<JsonObject>();

        // Mirrors API.cpp: the hardware thread applies the rotation
        if (jsonObj["mode"].is<int>()) {
            int mode = jsonObj["mode"].as<int>();
            if (mode >= MODE_CLOCK_IDLE && mode <= MODE_AMBIENT) {
                currentMode = (KubiMode)mode;
                display.requestWake();
            }
        }

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

        String cWork  = pomodoro.getColorWorkHex();
        String cShort = pomodoro.getColorShortBreakHex();
        String cLong  = pomodoro.getColorLongBreakHex();
        bool colorChanged = false;

        if (jsonObj["colorWork"].is<const char*>())  { cWork = jsonObj["colorWork"].as<const char*>(); colorChanged = true; }
        if (jsonObj["colorShort"].is<const char*>()) { cShort = jsonObj["colorShort"].as<const char*>(); colorChanged = true; }
        if (jsonObj["colorLong"].is<const char*>())  { cLong = jsonObj["colorLong"].as<const char*>(); colorChanged = true; }

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

        if (jsonObj["face4Maze"].is<bool>()) {
            face4Maze = jsonObj["face4Maze"].as<bool>();
            Preferences prefs;
            prefs.begin("kubi_settings", false);
            prefs.putBool("face4Maze", face4Maze);
            prefs.end();
        }

        if (jsonObj["sleepTimeoutMin"].is<int>()) {
            display.setSleepTimeoutMinutes(jsonObj["sleepTimeoutMin"].as<int>());
            Preferences prefs;
            prefs.begin("kubi_settings", false);
            prefs.putInt("sleepMin", display.getSleepTimeoutMinutes());
            prefs.end();
        }

        res.set_content("{\"status\":\"success\"}", "application/json");
    });

    // -------------------------------------------------------------------------
    // 3. POST /api/pomodoro/action
    // -------------------------------------------------------------------------
    svr.Post("/api/pomodoro/action", [](const httplib::Request& req, httplib::Response& res) {
        addCors(res);
        JsonDocument json;
        DeserializationError err = deserializeJson(json, req.body);
        if (err) {
            res.status = 400;
            res.set_content("{\"error\":\"invalid json\"}", "application/json");
            return;
        }

        JsonObject jsonObj = json.as<JsonObject>();
        if (jsonObj["action"].is<const char*>()) {
            std::string act = jsonObj["action"].as<const char*>();
            if (act == "play") pomodoro.play();
            else if (act == "pause") pomodoro.pause();
            else if (act == "toggle") pomodoro.togglePause();
            else if (act == "skip") pomodoro.skipPhase();
            else if (act == "reset") pomodoro.resetCurrent();

            res.set_content("{\"status\":\"success\"}", "application/json");
        } else {
            res.status = 400;
            res.set_content("{\"error\":\"missing action\"}", "application/json");
        }
    });

    // -------------------------------------------------------------------------
    // 4. POST /api/override
    // -------------------------------------------------------------------------
    svr.Post("/api/override", [](const httplib::Request& req, httplib::Response& res) {
        addCors(res);
        JsonDocument json;
        deserializeJson(json, req.body);
        JsonObject jsonObj = json.as<JsonObject>();

        if (jsonObj["message"].is<const char*>()) {
            setOverrideText(jsonObj["message"].as<const char*>());
            isScreenOverrideActive = true;
            display.requestWake();
            std::cout << "[OVERRIDE] Alert received: " << getOverrideText().c_str() << std::endl;
            res.set_content("{\"status\":\"alert_displayed\"}", "application/json");
        } else {
            res.status = 400;
            res.set_content("{\"error\":\"missing message\"}", "application/json");
        }
    });

    // -------------------------------------------------------------------------
    // 5. GET /api/diagnostics
    // -------------------------------------------------------------------------
    svr.Get("/api/diagnostics", [](const httplib::Request& req, httplib::Response& res) {
        addCors(res);
        JsonDocument doc;

        doc["accelX"] = diagAccelX;
        doc["accelY"] = diagAccelY;
        doc["accelZ"] = diagAccelZ;
        doc["temp"] = roomTemperature;
        doc["battery"] = batteryPercentage;
        doc["rssi"] = -52;
        doc["freeHeap"] = 184320;
        doc["uptime"] = millis() / 1000;
        doc["mode"] = (int)currentMode;

        std::string jsonStr;
        serializeJson(doc, jsonStr);
        res.set_content(jsonStr, "application/json");
    });

    // -------------------------------------------------------------------------
    // 6. POST /api/factory-reset (mirrors API.cpp)
    // -------------------------------------------------------------------------
    svr.Post("/api/factory-reset", [](const httplib::Request& req, httplib::Response& res) {
        addCors(res);
        JsonDocument json;
        deserializeJson(json, req.body);
        const char* confirm = json["confirm"] | "";
        if (strcmp(confirm, FACTORY_RESET_CONFIRM) == 0) {
            factoryResetRequested = true;
            res.set_content("{\"status\":\"resetting\"}", "application/json");
        } else {
            res.status = 400;
            res.set_content("{\"error\":\"confirm required\"}", "application/json");
        }
    });

    // -------------------------------------------------------------------------
    // 7. GET /sim/frame (Raw 32-bit RGBA Framebuffer: 240 x 320 x 4 = 307,200 bytes)
    // -------------------------------------------------------------------------
    svr.Get("/sim/frame", [](const httplib::Request& req, httplib::Response& res) {
        addCors(res);
        static std::vector<uint8_t> rgbaBuffer(240 * 320 * 4);
        convertRGB565toRGBA32(sim_framebuffer, rgbaBuffer.data(), 240 * 320);
        res.set_content((const char*)rgbaBuffer.data(), rgbaBuffer.size(), "application/octet-stream");
    });

    // -------------------------------------------------------------------------
    // 7. POST /sim/inject (Sensor, Gesture & Time Injection from Workbench UI)
    // -------------------------------------------------------------------------
    svr.Post("/sim/inject", [](const httplib::Request& req, httplib::Response& res) {
        addCors(res);
        JsonDocument json;
        deserializeJson(json, req.body);
        JsonObject obj = json.as<JsonObject>();

        if (obj["accelX"].is<float>()) sim_accel_x = obj["accelX"].as<float>();
        if (obj["accelY"].is<float>()) sim_accel_y = obj["accelY"].as<float>();
        if (obj["accelZ"].is<float>()) sim_accel_z = obj["accelZ"].as<float>();
        if (obj["temp"].is<float>())   sim_temperature = obj["temp"].as<float>();
        if (obj["battery"].is<int>())  batteryPercentage = obj["battery"].as<int>();

        if (obj["hour"].is<int>())     sim_hour_override = obj["hour"].as<int>();
        if (obj["minute"].is<int>())   sim_minute_override = obj["minute"].as<int>();

        // Quick Face Orientation Selection
        if (obj["face"].is<int>()) {
            int face = obj["face"].as<int>();
            if (face >= 0 && face < FACE_COUNT) {
                // Same table the firmware classifies with (FaceMap.h)
                gravityForFace(face, sim_accel_x, sim_accel_y, sim_accel_z);
                sim_tilt_roll = 0.0f;
                sim_tilt_pitch = 0.0f;

                currentMode = (KubiMode)face; // rotation applied by the hardware thread
                sim_injected_gesture = GESTURE_NONE;
                sensors.getRecentGesture();
            }
        }

        // Continuous tilt (workbench tilt pad / arrow keys): degrees relative to
        // the current face resting flat. The frame comes from FaceMap.h, the
        // same one the maze steers with. Clamped well short of 45 degrees so
        // tilting never reads as a different face.
        if (obj["tiltRoll"].is<float>() || obj["tiltPitch"].is<float>()) {
            if (obj["tiltRoll"].is<float>())  sim_tilt_roll  = obj["tiltRoll"].as<float>();
            if (obj["tiltPitch"].is<float>()) sim_tilt_pitch = obj["tiltPitch"].as<float>();
            if (sim_tilt_roll > 35.0f) sim_tilt_roll = 35.0f;
            if (sim_tilt_roll < -35.0f) sim_tilt_roll = -35.0f;
            if (sim_tilt_pitch > 35.0f) sim_tilt_pitch = 35.0f;
            if (sim_tilt_pitch < -35.0f) sim_tilt_pitch = -35.0f;
            KVec3 g = tiltedGravity((int)currentMode, sim_tilt_roll, sim_tilt_pitch);
            sim_accel_x = g.x;
            sim_accel_y = g.y;
            sim_accel_z = g.z;
        }

        // Preview the first-connect address screen without a reset
        if (obj["showAddress"].is<bool>()) {
            addressScreenIp = "127.0.0.1";
            addressScreenUntil = obj["showAddress"].as<bool>() ? millis() + ADDRESS_SCREEN_MS : 0;
        }

        // Gesture Triggers
        if (obj["gesture"].is<const char*>()) {
            std::string g = obj["gesture"].as<const char*>();
            if (g == "tap") {
                sim_injected_gesture = GESTURE_TAP;
            } else if (g == "slam") {
                sim_injected_gesture = GESTURE_SLAM;
            } else if (g == "shake") {
                sim_injected_gesture = GESTURE_SHAKE;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(15));
        }

        JsonDocument respDoc;
        respDoc["status"] = "injected";
        respDoc["lastChime"] = sim_last_chime_name;
        respDoc["lastChimeTime"] = sim_last_chime_time;
        std::string respJson;
        serializeJson(respDoc, respJson);
        res.set_content(respJson, "application/json");
    });

    // -------------------------------------------------------------------------
    // 8. GET /sim/state
    // -------------------------------------------------------------------------
    svr.Get("/sim/state", [](const httplib::Request& req, httplib::Response& res) {
        addCors(res);
        JsonDocument doc;

        doc["accelX"] = sim_accel_x;
        doc["accelY"] = sim_accel_y;
        doc["accelZ"] = sim_accel_z;
        doc["tiltRoll"] = sim_tilt_roll;
        doc["tiltPitch"] = sim_tilt_pitch;
        doc["temp"] = sim_temperature;
        doc["battery"] = batteryPercentage;
        doc["rotation"] = sim_screen_rotation;
        doc["backlight"] = sim_backlight_value;
        doc["lastChime"] = sim_last_chime_name;
        doc["lastChimeTime"] = sim_last_chime_time;
        doc["isSleeping"] = display.isSleeping();
        doc["mode"] = (int)currentMode;
        doc["isAnalog"] = clockAnalogView;
        doc["pixelWrites"] = sim_pixel_writes;
        doc["audioReady"] = audio.isReady();
        if (const AudioOutputI2S* out = audio.getOutput()) {
            doc["audioSamples"] = out->getSamplesWritten();
            doc["audioUnderruns"] = out->getUnderruns();
        }

        struct tm ti;
        if (getLocalTime(&ti)) {
            doc["hour"] = ti.tm_hour;
            doc["minute"] = ti.tm_min;
        }

        std::string jsonStr;
        serializeJson(doc, jsonStr);
        res.set_content(jsonStr, "application/json");
    });

    std::cout << "[SIM] HTTP API listening at http://127.0.0.1:8080" << std::endl;
    svr.listen("127.0.0.1", 8080);

    sim_running = false;
    if (hwThread.joinable()) hwThread.join();
    if (audioThread.joinable()) audioThread.join();
    return 0;
}
