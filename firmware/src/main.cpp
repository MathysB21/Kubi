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
#include "AmbientFace.h"

// =============================================================================
// KUBI: DESK COMPANION CUBE (ESP32 DUAL-CORE ARCHITECTURE)
// =============================================================================

// --- SHARED RUNTIME STATE GLOBALS ---
volatile KubiMode currentMode            = MODE_CLOCK_IDLE; // Active face mode
volatile float    roomTemperature        = 21.5f;           // BMP280 temperature
volatile int      batteryPercentage      = 100;             // Battery telemetry
volatile bool     isScreenOverrideActive = false;           // Brother's secret text alert flag
volatile bool     factoryResetRequested  = false;           // Set by POST /api/factory-reset

// First connect after setup: "open kubi.local" screen (0 = not showing)
static uint32_t addressScreenUntil = 0;
static String   addressScreenIp;

// Custom alert banner text: see setOverrideText()/getOverrideText()
static char        overrideTextBuf[OVERRIDE_TEXT_MAX] = "";
static portMUX_TYPE overrideTextMux = portMUX_INITIALIZER_UNLOCKED;

void setOverrideText(const char* text) {
  // Bounded memcpy only: no heap allocation while interrupts are masked
  size_t len = strnlen(text, OVERRIDE_TEXT_MAX - 1);
  portENTER_CRITICAL(&overrideTextMux);
  memcpy(overrideTextBuf, text, len);
  overrideTextBuf[len] = '\0';
  portEXIT_CRITICAL(&overrideTextMux);
}

String getOverrideText() {
  char local[OVERRIDE_TEXT_MAX];
  portENTER_CRITICAL(&overrideTextMux);
  memcpy(local, overrideTextBuf, OVERRIDE_TEXT_MAX);
  portEXIT_CRITICAL(&overrideTextMux);
  return String(local);
}

// Telemetry Diagnostics
volatile float diagAccelX = 0.0f;
volatile float diagAccelY = 0.0f;
volatile float diagAccelZ = 1.0f;

// --- CLOCK SETTINGS ---
bool clockAnalogView = false;

// --- NETWORK & TIME CONFIG ---
const char* ntpServer          = "pool.ntp.org";
const int   daylightOffset_sec = 0;
volatile int tzOffset          = 2; // Default UTC+2

Preferences preferences;
AsyncWebServer server(80);

// --- FREERTOS TASK HANDLES ---
TaskHandle_t TaskCore1Hardware;
TaskHandle_t TaskAudio;

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

// Stored password for a known SSID, looked up on the device (never sent to
// the portal page). Checks wifi_memory, then the ESP32's own saved network.
static String nativeSsid, nativePass;

String lookupSavedPassword(const String& ssid) {
  Preferences wifiPrefs;
  wifiPrefs.begin("wifi_memory", true);
  int count = wifiPrefs.getInt("count", 0);
  for (int i = 0; i < count; i++) {
    if (wifiPrefs.getString(("ssid_" + String(i)).c_str(), "") == ssid) {
      String p = wifiPrefs.getString(("pass_" + String(i)).c_str(), "");
      wifiPrefs.end();
      return p;
    }
  }
  wifiPrefs.end();
  return (ssid == nativeSsid) ? nativePass : String("");
}

// Appends s as a double-quoted JS string literal. Escapes quotes and
// backslashes, and emits < > & and control characters as \u00XX so an SSID
// can never close the surrounding <script> or inject markup.
static void appendJsString(String& out, const String& s) {
  out += '"';
  for (size_t i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c == '"' || c == '\\') {
      out += '\\';
      out += c;
    } else if ((uint8_t)c < 0x20 || c == '<' || c == '>' || c == '&') {
      char esc[8];
      snprintf(esc, sizeof(esc), "\\u%04x", (uint8_t)c);
      out += esc;
    } else {
      out += c;
    }
  }
  out += '"';
}

void connectToWiFi() {
  Serial.println("\n[WIFI] Checking for Known Networks...");

  WiFiMulti wifiMulti;

  // 1. Add native ESP32 NVS network
  nativeSsid = WiFi.SSID();
  nativePass = WiFi.psk();
  if (nativeSsid.length() > 0) {
    wifiMulti.addAP(nativeSsid.c_str(), nativePass.c_str());
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
  display.drawSetupScreen();
  WiFiManager wifiManager;

  // Tell the portal which SSIDs Kubi already knows: names only. The hotspot is
  // open, so passwords must never reach the page; a blank password for a known
  // SSID is filled in on the device by lookupSavedPassword().
  String jsNetworks = "<script>var knownNetworks=[";
  bool first = true;
  auto addKnown = [&](const String& s) {
    if (s.length() == 0) return;
    if (!first) jsNetworks += ",";
    appendJsString(jsNetworks, s);
    first = false;
  };
  wifiPrefs.begin("wifi_memory", true);
  count = wifiPrefs.getInt("count", 0);
  bool nativeListed = false;
  for (int i = 0; i < count; i++) {
    String s = wifiPrefs.getString(("ssid_" + String(i)).c_str(), "");
    if (s == nativeSsid) nativeListed = true;
    addKnown(s);
  }
  wifiPrefs.end();
  if (!nativeListed) addKnown(nativeSsid);
  jsNetworks += "];</script>";

  wifiManager.setCustomHeadElement(jsNetworks.c_str());
  wifiManager.setSavedPasswordResolver(lookupSavedPassword);
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
    // After the warm boot, show where the dashboard lives
    Preferences kubiPrefs;
    kubiPrefs.begin("kubi_settings", false);
    kubiPrefs.putBool("showAddr", true);
    kubiPrefs.end();
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
// FACTORY RESET
// Erases everything Kubi knows about its owner and restarts into setup mode.
// Runs on the hardware task (never the web server task) and does not return.
// =============================================================================
void performFactoryReset() {
  Serial.println("[RESET] Factory reset: wiping kubi_settings, wifi_memory and stored WiFi");
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
  WiFi.disconnect(true, true); // Wi-Fi off + erase the ESP32's own saved credentials

  delay(2500);
  ESP.restart();
}

// =============================================================================
// CORE 1 TASK: HARDWARE, SENSORS & UI LOOP
// High-frequency (~50-60Hz) hardware orchestration: IMU, Audio, Display
// =============================================================================
void core1HardwareTask(void * parameter) {
  uint32_t lastRenderTime = 0;
  uint32_t lastPomoTick   = 0;
  uint32_t lastMotionSeen = 0;
  int previousFace = -1;

  for (;;) {
    uint32_t now = millis();

    if (factoryResetRequested) {
      performFactoryReset();
    }

    // 1. Poll Sensors & Display (audio runs in its own task)
    sensors.loop();
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
          activeFace == 2 ? "Face 3 (Mascot & Temp)" : "Face 4 (Ambient)"
        );

        sensors.getRecentGesture(); // Flush any transient gesture during orientation transition
        display.noteActivity();
      }
    }

    // Keep rotation in sync with currentMode, which the dashboard can also
    // change. No-op unless it differs; TFT access stays on this core.
    display.setRotationForFace((int)currentMode);

    // 5. Gesture Handling (Streamlined: Tap = Dismiss/Pause/Play, Shake = Skip)
    KubiGesture gesture = sensors.getRecentGesture();
    if (gesture != GESTURE_NONE) {
      if (display.noteActivity()) {
        // Gesture only woke the screen: swallow it, silently (it may be night)
      } else {
        switch (gesture) {
          case GESTURE_TAP:
            if (addressScreenUntil) {
              addressScreenUntil = 0; // Owner has the address: back to the faces
              audio.playChime(CHIME_TAP_FEEDBACK);
            } else if (isScreenOverrideActive) {
              isScreenOverrideActive = false; // Dismiss override
              audio.playChime(CHIME_TAP_FEEDBACK);
            } else if (currentMode == MODE_POMODORO) {
              // Tap: Dismiss chime if ringing, else toggle Pause/Play
              pomodoro.handleTap();
            } else if (currentMode == MODE_CLOCK_IDLE) {
              audio.playChime(CHIME_TAP_FEEDBACK);
            } else if (currentMode == MODE_AMBIENT) {
              ambient.nextColour(); // silent: this face is for dark rooms
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

    // 5b. Inactivity sleep: any movement wakes silently; only some faces sleep
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
    display.setAwakeBrightness(currentMode == MODE_AMBIENT && !bannerUp ? AmbientFace::BACKLIGHT : 255);

    // 6. Display Rendering (~20Hz tick)
    if (now - lastRenderTime >= 50 && !display.isSleeping()) {
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
            display.drawAmbientFace();
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
// AUDIO TASK
// Feeds the I2S DMA queue. pump() blocks (1 ms yields) while the queue is
// full, so this task is paced by the 22,050 Hz sample clock rather than by the
// 10 ms hardware loop, which could only supply ~29% of the samples needed.
// =============================================================================
void audioTask(void * parameter) {
  for (;;) {
    if (!audio.pump()) {
      vTaskDelay(5 / portTICK_PERIOD_MS); // idle: poll for the next chime request
    }
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
  // Start audio immediately so the boot chime plays during WiFi connect.
  // Priority 2 on core 1: above the hardware loop, it sleeps in the DMA wait.
  xTaskCreatePinnedToCore(audioTask, "Audio", 3072, NULL, 2, &TaskAudio, 1);
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
  tzOffset        = preferences.getInt("tzOffset", 2);
  clockAnalogView = preferences.getBool("clockAnalog", false);
  display.setSleepTimeoutMinutes(preferences.getInt("sleepMin", 5));
  bool firstConnect = preferences.getBool("showAddr", false);
  if (firstConnect) preferences.remove("showAddr");
  // Retired with the calendar face (TSK-422): drop leftovers from older firmware
  preferences.remove("icalUrl");
  preferences.end();
  if (LittleFS.exists("/calendar.ics")) {
    LittleFS.remove("/calendar.ics");
  }

  // 8. Sync Clock via NTP
  display.drawBootScreen("Syncing Time...");
  configTime(tzOffset * 3600, daylightOffset_sec, ntpServer);
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    Serial.println("[NTP] Time successfully synchronized.");
  }

  // 9. Attach REST API Routes & Start Web Server
  setupAPIRoutes(server);
  server.begin();
  Serial.println("[HTTP] AsyncWebServer listening on port 80");

  String ip = WiFi.localIP().toString();
  if (firstConnect) {
    addressScreenIp = ip;
    addressScreenUntil = millis() + ADDRESS_SCREEN_MS;
  }
  display.drawBootScreen("Ready at " + ip);
  delay(1500);

  // 10. Launch the hardware/UI task. Networking runs in the WiFi and AsyncTCP
  // tasks on core 0; audio has its own task (started above).
  xTaskCreatePinnedToCore(core1HardwareTask, "Core1Hardware", 10000, NULL, 1, &TaskCore1Hardware, 1);
  Serial.println("[RTOS] Hardware task started.");
}

// =============================================================================
// ARDUINO LOOP
// =============================================================================
void loop() {
  // Over-The-Air firmware updates
  ArduinoOTA.handle();

  delay(10);
}
