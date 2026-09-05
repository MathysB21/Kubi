#include <Arduino.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <Preferences.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <WiFiManager.h>
#include <LittleFS.h>
#include "time.h"
#include "API.h"
#include "DisplayManager.h"
#include "SensorManager.h"
#include "AudioManager.h"
#include "PomodoroManager.h"
#include <vector>

// =============================================================================
// KUBI: DESK COMPANION CUBE (ESP32 DUAL-CORE ARCHITECTURE)
// =============================================================================

// --- SHARED RUNTIME STATE GLOBALS ---
volatile KubiMode currentMode            = MODE_CLOCK_IDLE; // Active face mode
volatile float    roomTemperature        = 21.5f;           // BMP280 temperature
volatile int      batteryPercentage      = 100;             // Battery telemetry
String            secretIcalUrl          = "";              // Google Calendar iCal link
volatile bool     isScreenOverrideActive = false;           // Brother's secret text alert flag
String            screenOverrideText     = "";              // Custom alert banner text

// Telemetry Diagnostics
volatile float diagAccelX = 0.0f;
volatile float diagAccelY = 0.0f;
volatile float diagAccelZ = 1.0f;

// --- SCHEDULE CACHE ---
static std::vector<String> cachedAgenda;
static int schedulePage = 0;
static bool clockShowDetails = false;
bool clockAnalogView = false;
static uint32_t clockDetailsTimeout = 0;

// --- NETWORK & TIME CONFIG ---
const char* ntpServer          = "pool.ntp.org";
const int   daylightOffset_sec = 0;
volatile int tzOffset          = 2; // Default UTC+2

Preferences preferences;
AsyncWebServer server(80);

// --- FREERTOS TASK HANDLES ---
TaskHandle_t TaskCore0Network;
TaskHandle_t TaskCore1Hardware;

// --- WIFI MANAGER TCP FIX & CALLBACKS ---
bool justSavedConfig = false;

void saveConfigCallback() {
  justSavedConfig = true;
}

// =============================================================================
// MULTI-NETWORK MEMORY (Preferences)
// =============================================================================
void saveNetworkToMemory(String ssid, String pass) {
  Preferences wifiPrefs;
  wifiPrefs.begin("wifi_memory", false);

  int count = wifiPrefs.getInt("count", 0);

  // Check if SSID is already stored
  for (int i = 0; i < count; i++) {
    String ssidKey = "ssid_" + String(i);
    if (wifiPrefs.getString(ssidKey.c_str(), "") == ssid) {
      String passKey = "pass_" + String(i);
      wifiPrefs.putString(passKey.c_str(), pass);
      wifiPrefs.end();
      return;
    }
  }

  // FIFO shift if capacity (5 networks) is reached
  if (count >= 5) {
    for (int i = 1; i < 5; i++) {
      String oldSsidKey = "ssid_" + String(i);
      String oldPassKey = "pass_" + String(i);
      String newSsidKey = "ssid_" + String(i - 1);
      String newPassKey = "pass_" + String(i - 1);
      wifiPrefs.putString(newSsidKey.c_str(), wifiPrefs.getString(oldSsidKey.c_str(), ""));
      wifiPrefs.putString(newPassKey.c_str(), wifiPrefs.getString(oldPassKey.c_str(), ""));
    }
    count = 4;
  }

  String sKey = "ssid_" + String(count);
  String pKey = "pass_" + String(count);
  wifiPrefs.putString(sKey.c_str(), ssid);
  wifiPrefs.putString(pKey.c_str(), pass);

  wifiPrefs.putInt("count", count + 1);
  wifiPrefs.end();
  Serial.printf("[WIFI] Saved network to memory: %s\n", ssid.c_str());
}

void connectToWiFi() {
  Serial.println("\n[WIFI] Checking for Known Networks...");

  WiFiMulti wifiMulti;

  // 1. Add native ESP32 NVS network
  if (WiFi.SSID().length() > 0) {
    wifiMulti.addAP(WiFi.SSID().c_str(), WiFi.psk().c_str());
  }

  // 2. Load custom multi-network credentials from Preferences
  Preferences wifiPrefs;
  wifiPrefs.begin("wifi_memory", false);
  int count = wifiPrefs.getInt("count", 0);
  for (int i = 0; i < count; i++) {
    String s = wifiPrefs.getString(("ssid_" + String(i)).c_str(), "");
    String p = wifiPrefs.getString(("pass_" + String(i)).c_str(), "");
    if (s.length() > 0) {
      wifiMulti.addAP(s.c_str(), p.c_str());
      Serial.printf("[WIFI] Loaded Known Network: %s\n", s.c_str());
    }
  }
  wifiPrefs.end();

  WiFi.mode(WIFI_STA);
  Serial.print("[WIFI] Connecting via WiFiMulti...");

  // 3. Fast multi-AP connection attempt (~10 seconds)
  bool connected = false;
  uint32_t startAttempt = millis();
  while (millis() - startAttempt < 10000) {
    if (wifiMulti.run() == WL_CONNECTED) {
      connected = true;
      break;
    }
    delay(500);
    Serial.print(".");
  }

  // 4. Skip WiFiManager if successfully connected
  if (connected) {
    Serial.println("\n[WIFI] Connected to known network!");
    Serial.print("[WIFI] IP Address: ");
    Serial.println(WiFi.localIP());
    return;
  }

  // 5. Fallback: Launch Kubi Captive Setup Portal
  Serial.println("\n[WIFI] No known networks in range. Initializing Kubi Setup Portal...");
  display.drawBootScreen("Hotspot: Kubi-Setup");
  WiFiManager wifiManager;

  // Pre-load known SSIDs into portal script
  String jsNetworks = "<script>var savedNetworks = {";
  wifiPrefs.begin("wifi_memory", false);
  count = wifiPrefs.getInt("count", 0);
  for (int i = 0; i < count; i++) {
    String s = wifiPrefs.getString(("ssid_" + String(i)).c_str(), "");
    String p = wifiPrefs.getString(("pass_" + String(i)).c_str(), "");
    if (s.length() > 0) {
      if (i > 0) jsNetworks += ",";
      jsNetworks += "\"" + s + "\":\"" + p + "\"";
    }
  }
  jsNetworks += "};</script>";
  wifiPrefs.end();

  wifiManager.setCustomHeadElement(jsNetworks.c_str());
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.setConfigPortalTimeout(180); // 3 minutes timeout

  // Broadcast Kubi-Setup hotspot
  if (!wifiManager.autoConnect("Kubi-Setup")) {
    Serial.println("[WIFI] Connection failed or portal timed out. Rebooting...");
    delay(3000);
    ESP.restart();
  }

  // 6. Warm boot flush after saving new credentials to clear locked sockets
  if (justSavedConfig) {
    String newSSID = WiFi.SSID();
    String newPass = WiFi.psk();
    if (newSSID.length() > 0) {
      saveNetworkToMemory(newSSID, newPass);
    }
    Serial.println("[WIFI] Credentials saved! Warm-booting to flush TCP sockets...");
    delay(1000);
    ESP.restart();
  }

  WiFi.mode(WIFI_STA);
  Serial.println("\n[WIFI] WiFi Connected!");
  Serial.print("[WIFI] IP Address: ");
  Serial.println(WiFi.localIP());
}

// =============================================================================
// OVER-THE-AIR (OTA) UPDATES
// =============================================================================
void setupOTA() {
  ArduinoOTA.setHostname("kubi");

  ArduinoOTA.onStart([]() {
    Serial.println("\n--- [OTA] UPDATE STARTED ---");
    display.drawBootScreen("OTA Updating...");
    LittleFS.end(); // Unmount filesystem before OTA flash rewrite
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("\n--- [OTA] UPDATE COMPLETE ---");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("[OTA] Progress: %u%%\r", (progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("[OTA] Error[%u]\n", error);
  });

  ArduinoOTA.begin();
  Serial.println("[OTA] Kubi OTA Ready!");
}

// =============================================================================
// CORE 0 TASK: NETWORKING & BACKGROUND SYNC
// Handles WiFi monitoring, AsyncWebServer, and Google Calendar sync
// =============================================================================
void core0NetworkTask(void * parameter) {
  uint32_t lastCalendarSync = 0;
  const uint32_t CALENDAR_SYNC_INTERVAL_MS = 3600000; // 1 Hour

  for (;;) {
    // 1. Maintain WiFi status
    if (WiFi.status() != WL_CONNECTED) {
      // Reconnection monitor
    }

    // 2. Background Google Calendar iCal worker
    if (secretIcalUrl.length() > 0 && (millis() - lastCalendarSync > CALENDAR_SYNC_INTERVAL_MS || lastCalendarSync == 0)) {
      lastCalendarSync = millis();
      Serial.println("[CALENDAR] Hourly sync triggered");
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

// =============================================================================
// CORE 1 TASK: HARDWARE, SENSORS & UI LOOP
// High-frequency (~50-60Hz) hardware orchestration: IMU, Audio, Display
// =============================================================================
void core1HardwareTask(void * parameter) {
  uint32_t lastRenderTime = 0;
  uint32_t lastPomoTick   = 0;
  int previousFace = -1;

  for (;;) {
    uint32_t now = millis();

    // 1. Poll Sensors & Audio Engine
    sensors.loop();
    audio.loop();
    display.loop();

    // 2. Second-by-second Pomodoro Countdown Tick
    if (now - lastPomoTick >= 1000) {
      lastPomoTick = now;
      pomodoro.tick();
    }

    // 3. Read live telemetry
    sensors.getAcceleration((float&)diagAccelX, (float&)diagAccelY, (float&)diagAccelZ);
    roomTemperature = sensors.getTemperature();

    // 4. Orientation / Face Up State Machine
    int activeFace = sensors.getActiveFace();
    if (activeFace != previousFace) {
      previousFace = activeFace;
      if (activeFace >= 0 && activeFace <= 3) {
        currentMode = (KubiMode)activeFace;
        display.setRotationForFace(activeFace);
        Serial.printf("[KUBI ORIENTATION] >>> Face %d UP: Switched to %s (Screen rotated upright) <<<\n",
          activeFace + 1,
          activeFace == 0 ? "Face 1 (Clock Idle)" :
          activeFace == 1 ? "Face 2 (Pomodoro Timer)" :
          activeFace == 2 ? "Face 3 (Mascot & Temp)" : "Face 4 (Schedule Agenda)"
        );

        // Entering Pomodoro: start/resume timer
        if (currentMode == MODE_POMODORO) {
          pomodoro.play();
        }
        clockShowDetails = false;
      }
    }

    // 5. Gesture Handling (Streamlined: Tap = Dismiss/Pause/Play, Shake = Skip)
    KubiGesture gesture = sensors.getRecentGesture();
    if (gesture != GESTURE_NONE) {
      if (display.isSleeping()) {
        display.wakeScreen();
        audio.playChime(CHIME_WAKE_PING);
      } else {
        switch (gesture) {
          case GESTURE_TAP:
            if (isScreenOverrideActive) {
              isScreenOverrideActive = false; // Dismiss override
              audio.playChime(CHIME_TAP_FEEDBACK);
            } else if (currentMode == MODE_POMODORO) {
              // Tap: Dismiss chime if ringing, else toggle Pause/Play
              pomodoro.handleTap();
            } else if (currentMode == MODE_CLOCK_IDLE) {
              audio.playChime(CHIME_TAP_FEEDBACK);
            } else if (currentMode == MODE_SCHEDULE_AGENDA) {
              schedulePage = (schedulePage + 1) % 2; // Cycle page
              audio.playChime(CHIME_TAP_FEEDBACK);
            }
            break;

          case GESTURE_SHAKE:
            if (currentMode == MODE_POMODORO) {
              // Shake: Skip to next phase
              pomodoro.handleShake();
            } else if (currentMode == MODE_CLOCK_IDLE) {
              clockAnalogView = !clockAnalogView;
              preferences.begin("kubi_settings", false);
              preferences.putBool("clockAnalog", clockAnalogView);
              preferences.end();
              audio.playChime(CHIME_TAP_FEEDBACK);
              Serial.printf("[CLOCK] Shake detected -> Switched to %s clock view (saved to NVS)\n", clockAnalogView ? "analog" : "digital");
            }
            break;

          case GESTURE_SLAM:
            audio.playChime(CHIME_SLAM_OUCH);
            Serial.println("[MASCOT] Ouch! Don't slam the desk!");
            break;

          default:
            break;
        }
      }
    }

    // Auto collapse clock details
    if (clockShowDetails && now > clockDetailsTimeout) {
      clockShowDetails = false;
    }

    // 6. Display Rendering (~20Hz tick)
    if (now - lastRenderTime >= 50 && !display.isSleeping()) {
      lastRenderTime = now;

      if (isScreenOverrideActive) {
        display.drawOverrideAlert(screenOverrideText);
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
            display.drawClockFace(currentHour, currentMin, currentWday, currentMday, currentMon, clockShowDetails, clockAnalogView, "Design Review 14:00", "^ AAPL +1.2% | BTC $92k");
            break;

          case MODE_POMODORO:
            display.drawPomodoroFace(
              pomodoro.getRemainingSeconds(),
              pomodoro.getTotalSeconds(),
              pomodoro.getPhaseName(),
              pomodoro.isPaused(),
              pomodoro.getPhaseColor(),
              pomodoro.getCompletedCycles(),
              pomodoro.getCycleTarget()
            );
            break;

          case MODE_MASCOT_ROUTINE:
            display.drawMascotFace(roomTemperature, currentHour);
            break;

          case MODE_SCHEDULE_AGENDA:
            display.drawScheduleFace(cachedAgenda, schedulePage);
            break;

          default:
            break;
        }
      }
    }

    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

// =============================================================================
// ARDUINO SETUP
// =============================================================================
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=================================");
  Serial.println("       PROJECT KUBI BOOTING      ");
  Serial.println("=================================");

  // 1. Initialize Hardware Drivers & Managers
  display.init();
  display.drawBootScreen("Booting Sensors...");

  sensors.init();
  audio.init();
  pomodoro.init();

  // Play power-up chime
  audio.playChime(CHIME_WAKE_PING);

  // 2. Connect to WiFi or launch captive portal
  display.drawBootScreen("Connecting to WiFi...");
  connectToWiFi();

  // 3. Start OTA Listener
  setupOTA();

  // 4. Force clean Station mode to reclaim TCP socket
  WiFi.mode(WIFI_STA);
  delay(500);

  // 5. Start mDNS Responder (http://kubi.local)
  if (!MDNS.begin("kubi")) {
    Serial.println("[MDNS] Error setting up MDNS responder!");
  } else {
    Serial.println("[MDNS] Responder started: http://kubi.local");
  }

  // 6. Mount LittleFS Internal Filesystem
  if (!LittleFS.begin(true)) {
    Serial.println("[FS] ERROR: Failed to mount LittleFS!");
  } else {
    Serial.println("[FS] LittleFS mounted successfully.");
  }

  // 7. Load Persistent Settings from NVS
  preferences.begin("kubi_settings", false);
  secretIcalUrl   = preferences.getString("icalUrl", "");
  tzOffset        = preferences.getInt("tzOffset", 2);
  clockAnalogView = preferences.getBool("clockAnalog", false);
  preferences.end();

  // 8. Sync Clock via NTP
  display.drawBootScreen("Syncing Time...");
  configTime(tzOffset * 3600, daylightOffset_sec, ntpServer);
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    Serial.println("[NTP] Time successfully synchronized.");
  }

  // Seed sample schedule items
  cachedAgenda.push_back("Today 10:00 - Team Sync");
  cachedAgenda.push_back("Today 14:00 - Design Review");
  cachedAgenda.push_back("Tomorrow 09:30 - Sprint Planning");
  cachedAgenda.push_back("Fri 18:00 - Wilhelm's 21st Party!");

  // 9. Attach REST API Routes & Start Web Server
  setupAPIRoutes(server);
  server.begin();
  Serial.println("[HTTP] AsyncWebServer listening on port 80");

  display.drawBootScreen("Kubi Ready!");
  delay(500);

  // 10. Launch Dual-Core FreeRTOS Tasks
  xTaskCreatePinnedToCore(core0NetworkTask,  "Core0Network",  10000, NULL, 1, &TaskCore0Network,  0);
  xTaskCreatePinnedToCore(core1HardwareTask, "Core1Hardware", 10000, NULL, 1, &TaskCore1Hardware, 1);
  Serial.println("[RTOS] Dual-core workers initialized.");
}

// =============================================================================
// ARDUINO LOOP
// =============================================================================
void loop() {
  // Over-The-Air firmware updates
  ArduinoOTA.handle();

  delay(10);
}
