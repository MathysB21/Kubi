#include <Arduino.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <esp_core_dump.h>
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
#include "MazeGame.h"
#include "FaceMap.h"
#include "KubiScenes.h"
#include "Battery.h"
#include "SceneSchedule.h"
#include <Wire.h>
#include <esp_sleep.h>
#include <driver/rtc_io.h>

// =============================================================================
// KUBI: DESK COMPANION CUBE (ESP32 DUAL-CORE ARCHITECTURE)
// =============================================================================

// --- SHARED RUNTIME STATE GLOBALS ---
volatile KubiMode currentMode            = MODE_CLOCK_IDLE; // Active face mode
volatile float    roomTemperature        = 21.5f;           // BMP280 temperature
volatile float    roomPressure           = 0.0f;            // BMP280 pressure, hPa
volatile int      batteryPercentage      = -1;              // From the cell voltage (Battery.h); -1 = not known yet
volatile float    batteryVoltage         = 0.0f;            // Smoothed cell voltage, 0 until the first read
volatile bool     isScreenOverrideActive = false;           // Brother's secret text alert flag
volatile bool     factoryResetRequested  = false;           // Set by POST /api/factory-reset
volatile bool     otaInProgress          = false;           // Set by the OTA callbacks (loop() task)

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

// --- FACE 3 ---
int mascotSceneIndex = 0; // Diorama on the Mascot face, 0..ALL_KUBI_SCENE_COUNT-1

// Kubi picks the scene himself (SceneSchedule.h). The dashboard can show one
// for KUBI_SCENE_PREVIEW_MS: API.cpp sets these, the hardware loop applies them.
volatile int      mascotPreviewScene = -1;
volatile uint32_t mascotPreviewAt    = 0;
volatile uint32_t mascotPreviewMs    = KUBI_SCENE_PREVIEW_MS; // Creator panel can hold longer
volatile int      birthdayMMDD       = 0;  // NVS "bday", 0 = not set

// Tap or shake on Face 3: Kubi notices (blinks); the scene stays his choice
static void pokeMascot() {
  display.pokeScene();
  audio.playChime(CHIME_TAP_FEEDBACK);
}

// --- MAZE (screen up) ---
int mazeViewFace = 0; // last upright face: the maze draws the way it did

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

  // These callbacks run in loop()'s task, not the hardware task: never touch
  // the TFT here. Drawing from both tasks corrupts TFT_eSPI's shared SPI lock
  // flag and asserts in FreeRTOS (every OTA attempt rebooted the cube).
  // The hardware task sees otaInProgress and draws the update screen itself.
  ArduinoOTA.onStart([]() {
    Serial.println("\n--- [OTA] UPDATE STARTED ---");
    otaInProgress = true;
    // Only a filesystem image rewrites LittleFS; a firmware update writes
    // the other app slot and the dashboard keeps serving meanwhile.
    if (ArduinoOTA.getCommand() == U_SPIFFS) LittleFS.end();
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("\n--- [OTA] UPDATE COMPLETE ---");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("[OTA] Progress: %u%%\r", (progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("[OTA] Error[%u]\n", error);
    otaInProgress = false; // back to the faces; the old firmware keeps running
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

// --- NAP: Kubi's "off" without a power switch ---
// The dashboard asks for it (POST /api/nap sets napRequested; the hardware
// loop does the rest). Screen, WiFi and sound go off and the ESP32 deep-sleeps.
// The ADXL345 stays powered with its activity detector latching any pickup;
// every NAP_CHECK_S the ESP32 wakes, reads that latch before anything else in
// setup(), and goes straight back to sleep unless Kubi was moved. Its INT pin
// is not wired, hence the polling. The amp, boost converter and DevKit keep
// drawing a little, so a nap lasts about a week, not months.
volatile bool napRequested = false;
RTC_DATA_ATTR static bool napping = false;
#define NAP_CHECK_S        2
#define NAP_ADXL_ADDR      0x53
#define NAP_ACT_THRESHOLD  8      // 62.5 mg/LSB: a 0.5 g change counts as being picked up
static const gpio_num_t NAP_HOLD_PINS[] = { GPIO_NUM_32, GPIO_NUM_25, GPIO_NUM_26 }; // backlight, DAC L, DAC R

static uint8_t napAdxlRead(uint8_t reg, bool& ok) {
  Wire.beginTransmission(NAP_ADXL_ADDR);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0 || Wire.requestFrom(NAP_ADXL_ADDR, 1) != 1) { ok = false; return 0; }
  return Wire.read();
}

static void napAdxlWrite(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(NAP_ADXL_ADDR);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}

// Backlight and both amp inputs held low while asleep: dark screen, no hiss.
// Wakes after `seconds` for the check in setup() (nap or battery sleep).
[[noreturn]] static void holdPinsAndSleep(uint32_t seconds) {
  for (gpio_num_t p : NAP_HOLD_PINS) {
    rtc_gpio_init(p);
    rtc_gpio_set_direction(p, RTC_GPIO_MODE_OUTPUT_ONLY);
    rtc_gpio_set_level(p, 0);
    rtc_gpio_hold_en(p);
  }
  esp_sleep_enable_timer_wakeup((uint64_t)seconds * 1000000ULL);
  esp_deep_sleep_start();
  for (;;) {}
}

[[noreturn]] static void napSleep() { holdPinsAndSleep(NAP_CHECK_S); }

static void releaseNapPins() {
  for (gpio_num_t p : NAP_HOLD_PINS) {
    rtc_gpio_hold_dis(p);
    rtc_gpio_deinit(p);
  }
}

// First thing in setup(): during a nap, go back to sleep unless Kubi was moved
static void checkNap() {
  if (napping) {
    if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TIMER) {
      Wire.begin(21, 22);
      bool ok = true;
      uint8_t source = napAdxlRead(0x30, ok); // INT_SOURCE; reading clears the latch
      if (ok && !(source & 0x10)) napSleep();  // no activity: keep napping
    }
    napping = false; // picked up (or the sensor did not answer): wake properly
  }
  releaseNapPins();
}

static void enterNap() {
  Serial.println("[NAP] Good night: sleeping until picked up");
  display.requestWake();
  display.drawBootScreen("Good night!");
  audio.playChime(CHIME_TAP_FEEDBACK);
  vTaskDelay(pdMS_TO_TICKS(1500));

  bool ok = true;
  napAdxlWrite(0x24, NAP_ACT_THRESHOLD);                // THRESH_ACT
  napAdxlWrite(0x27, 0xF0);                             // ACT_INACT_CTL: AC-coupled, X Y Z
  napAdxlWrite(0x2E, napAdxlRead(0x2E, ok) | 0x10);     // INT_ENABLE: activity
  napAdxlRead(0x30, ok);                                // clear whatever latched so far

  display.powerDown();
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  napping = true;
  Serial.flush();
  napSleep();
}

// --- BATTERY ---
// GPIO 34 (ADC1, so it works alongside WiFi) reads the cell through R1/R2.
#define BATTERY_SENSE_PIN   34
#define BATTERY_READ_MS     2000
// Smoothing, charging detection and the time estimate: BatteryMonitor (Battery.h)
static BatteryMonitor battery;
volatile int batteryStateCode   = BATTERY_DRAINING; // BatteryState, for the API
volatile int batteryMinutesLeft = -1;
// Charge reminders (NVS "alertHi"/"alertLo", 0 = off): Kubi chimes and shows
// a banner once per charge or discharge. The charger itself always fills to 100%.
volatile int chargeAlertHigh = 0;
volatile int chargeAlertLow  = 0;

static float readBatteryVolts() {
  uint32_t mv = 0;
  for (int i = 0; i < 32; i++) mv += analogReadMilliVolts(BATTERY_SENSE_PIN);
  return (mv / 32.0f) / 1000.0f / BATTERY_DIVIDER_RATIO;
}

// Quiet hours for the charge reminders: the 20 % ping woke the house at 01:20
// (2026-11-18). At night the banner still goes up, but without the chime and
// without waking a sleeping screen. Local time; unknown time counts as day.
#define REMINDER_QUIET_FROM_HOUR 22
#define REMINDER_QUIET_TO_HOUR    7
static bool reminderQuietHours() {
  struct tm t;
  if (!getLocalTime(&t, 0)) return false;
  return t.tm_hour >= REMINDER_QUIET_FROM_HOUR || t.tm_hour < REMINDER_QUIET_TO_HOUR;
}

// --- BATTERY SLEEP: the low-battery shutdown ---
// The charger board's protection only cuts the cells off at ~2.5 V, deeper
// than an 18650 should go, and Kubi ran them down to it overnight
// (2026-11-19). Now, once BatteryMonitor::empty() (draining, average under
// BATTERY_EMPTY_V for a minute), Kubi says so and deep-sleeps. Picking him up
// does not wake him; only charging does: every BATTERY_SLEEP_CHECK_S the
// ESP32 reads the cell before anything else in setup() and boots only on a
// clear rise since the last check (plugging in lifts the reading ~0.08 V,
// then it climbs) or once the cell is back above BATTERY_WAKE_V. The first
// check only records, because the cell recovers ~0.1 V when the load stops.
// Asleep, the boost converter, amp and DevKit still draw a few mA, so this
// buys many hours below 3.3 V rather than protecting the cells forever.
RTC_DATA_ATTR static bool  batterySleeping = false;
RTC_DATA_ATTR static float batterySleepLastV = 0;
#define BATTERY_SLEEP_CHECK_S 60
#define BATTERY_WAKE_STEP_V   0.05f
#define BATTERY_WAKE_V        3.75f
static volatile bool batteryEmpty = false;

// First thing in setup(): asleep for the battery, stay asleep unless charging
static void checkBatterySleep() {
  if (!batterySleeping) return;
  if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TIMER) {
    float v = readBatteryVolts();
    bool rising = batterySleepLastV > 0 && v - batterySleepLastV >= BATTERY_WAKE_STEP_V;
    if (!rising && v < BATTERY_WAKE_V) {
      batterySleepLastV = v;
      holdPinsAndSleep(BATTERY_SLEEP_CHECK_S);
    }
  }
  batterySleeping = false; // charging, recharged, or reset: start up properly
}

static bool reminderQuietHours();

static void enterBatterySleep() {
  Serial.printf("[BATTERY] Empty (%.2f V): sleeping until charged\n", battery.voltage());
  display.requestWake();
  display.drawBootScreen("Battery empty - charge me!");
  if (!reminderQuietHours()) audio.playChime(CHIME_WAKE_PING);
  vTaskDelay(pdMS_TO_TICKS(5000)); // long enough to read it
  display.powerDown();
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  batterySleeping = true;
  batterySleepLastV = 0;
  Serial.flush();
  holdPinsAndSleep(BATTERY_SLEEP_CHECK_S);
}

static void batteryReminder(const char* text) {
  char banner[OVERRIDE_TEXT_MAX];
  composeBanner(banner, sizeof(banner), "Kubi's battery", text); // calm title, not the alert one
  setOverrideText(banner);
  isScreenOverrideActive = true; // tap dismisses it, like any banner
  bool quiet = reminderQuietHours();
  if (!quiet) {
    display.requestWake();
    audio.playChime(CHIME_WAKE_PING);
  }
  Serial.printf("[BATTERY] %s%s\n", text, quiet ? " (quiet hours: no chime)" : "");
}

static void updateBattery() {
  static bool highDone = false, lowDone = false;
  static BatteryState lastState = BATTERY_DRAINING;

  battery.update(readBatteryVolts(), millis());
  // Not shown during the warm-up (-1 = not known yet), or the charger's own
  // 4.2 V at power-on flashes up as 100 %
  batteryVoltage = battery.warming() ? 0.0f : battery.voltage();
  batteryPercentage = battery.warming() ? -1 : battery.percent();
  // 4 = "checking": just started, not yet sure whether the charger is in
  batteryStateCode = (battery.state() == BATTERY_DISCONNECTED || battery.sure()) ? battery.state() : 4;
  batteryMinutesLeft = battery.minutesLeft();
  if (battery.empty()) batteryEmpty = true; // the hardware loop shuts down

  // Keep the learned drain/charge rates across restarts (NVS "drainRate",
  // "chargeRate"), checked every 10 minutes and written only when one moved by
  // more than 0.05 %/h (a few writes a day once it has settled)
  static uint32_t lastRateSave = 0;
  static float savedDrain = -1, savedCharge = -1;
  if (savedDrain < 0) {
    savedDrain = battery.learnedDrainPerHour(); // what setup() loaded from NVS
    savedCharge = battery.learnedChargePerHour();
  }
  if (millis() - lastRateSave >= 10UL * 60UL * 1000UL) {
    lastRateSave = millis();
    float d = battery.learnedDrainPerHour(), c = battery.learnedChargePerHour();
    if (fabsf(d - savedDrain) > 0.05f || fabsf(c - savedCharge) > 0.05f) {
      Preferences prefs;
      prefs.begin("kubi_settings", false);
      prefs.putFloat("drainRate", d);
      prefs.putFloat("chargeRate", c);
      prefs.end();
      savedDrain = d;
      savedCharge = c;
      Serial.printf("[BATTERY] Learned rates saved: drain %.2f %%/h, charge %.2f %%/h\n", d, c);
    }
  }

  if (battery.warming()) return; // no reminders from a number not known yet
  BatteryState st = battery.state();
  if (st != lastState) {
    if (st == BATTERY_DRAINING) highDone = false; // unplugged: arm the unplug reminder for next time
    else lowDone = false;                         // plugged in: arm the low reminder
    lastState = st;
  }
  char msg[48];
  if (chargeAlertHigh > 0 && st != BATTERY_DRAINING && !highDone && batteryPercentage >= chargeAlertHigh) {
    highDone = true;
    snprintf(msg, sizeof(msg), "Battery %d%% - unplug me!", batteryPercentage);
    batteryReminder(msg);
  }
  if (chargeAlertLow > 0 && st == BATTERY_DRAINING && battery.sure() && !lowDone && batteryPercentage <= chargeAlertLow) {
    lowDone = true;
    snprintf(msg, sizeof(msg), "Battery %d%% - plug me in!", batteryPercentage);
    batteryReminder(msg);
  }
}

// =============================================================================
// CORE 1 TASK: HARDWARE, SENSORS & UI LOOP
// High-frequency (~50-60Hz) hardware orchestration: IMU, Audio, Display
// =============================================================================
void core1HardwareTask(void * parameter) {
  uint32_t lastRenderTime = 0;
  uint32_t lastPomoTick   = 0;
  uint32_t lastBatteryRead = 0;
  uint32_t lastMotionSeen = 0;
  int previousFace = -1;
  bool mazeWasActive = false;
  bool mazeWasLocked = false;
  bool otaScreenShown = false;

  for (;;) {
    uint32_t now = millis();

    if (factoryResetRequested) {
      performFactoryReset();
    }
    if (napRequested) {
      enterNap(); // does not return
    }
    if (batteryEmpty) {
      enterBatterySleep(); // does not return
    }

    // A WiFi update is being written (see setupOTA): show it and pause the
    // faces. Success reboots; an error clears the flag and the faces return.
    if (otaInProgress) {
      if (!otaScreenShown) {
        display.requestWake();
        display.drawBootScreen("Updating...");
        otaScreenShown = true;
      }
      display.loop(); // backlight fade-in if it was asleep
      vTaskDelay(pdMS_TO_TICKS(20));
      continue;
    }
    if (otaScreenShown) {
      otaScreenShown = false;
      display.invalidate();
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
    roomPressure = sensors.getPressure();
    if (lastBatteryRead == 0 || now - lastBatteryRead >= BATTERY_READ_MS) {
      lastBatteryRead = now;
      updateBattery();
    }

    // 4. Orientation / Face Up State Machine
    int activeFace = sensors.getActiveFace();
    if (activeFace != previousFace) {
      previousFace = activeFace;
      if (activeFace >= 0 && activeFace < FACE_COUNT) {
        currentMode = (KubiMode)activeFace;
        Serial.printf("[KUBI ORIENTATION] >>> Face %d UP: Switched to %s (Screen rotated upright) <<<\n",
          activeFace + 1,
          activeFace == 0 ? "Face 1 (Clock Idle)" :
          activeFace == 1 ? "Face 2 (Pomodoro Timer)" :
          activeFace == 2 ? "Face 3 (Mascot & Temp)" : "Face 4 (Ambient)"
        );
      } else if (activeFace == FACE_SCREEN_UP) {
        currentMode = MODE_MAZE;
        Serial.printf("[KUBI ORIENTATION] >>> Screen UP: Maze, drawn as Face %d <<<\n", mazeViewFace + 1);
      }
      if (activeFace >= 0) {
        sensors.getRecentGesture(); // Flush any transient gesture during orientation transition
        display.noteActivity();
      }
    }
    // The maze faces whoever tipped the cube back: remember the last upright
    // face (the dashboard can set one too).
    if (currentMode != MODE_MAZE) mazeViewFace = (int)currentMode;

    // Keep rotation in sync with currentMode, which the dashboard can also
    // change. No-op unless it differs; TFT access stays on this core.
    display.setRotationForFace(rotationFaceFor(currentMode));

    // Entering the maze face starts the current board afresh
    bool mazeActive = mazeFaceActive(currentMode);
    if (mazeActive && !mazeWasActive) {
      maze.setViewFace(mazeViewFace);
      maze.begin();
    }
    mazeWasActive = mazeActive;
    // Face lock while playing: leaving needs a 2 s hold on another face, and
    // the maze lets go by itself after MAZE_IDLE_UNLOCK_MS without input.
    bool mazeLocked = mazeActive && maze.isPlaying(now);
    sensors.setFaceSettleTime(mazeLocked ? FACE_SETTLE_LOCKED : FACE_SETTLE_MS);
    if (mazeLocked != mazeWasLocked) {
      Serial.printf("[MAZE] Face lock %s\n", mazeLocked ? "on" : "off");
      mazeWasLocked = mazeLocked;
    }
    if (mazeActive) {
      float ax, ay, az;
      sensors.getAcceleration(ax, ay, az);
      maze.update(ax, ay, az, now);
    }

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
            } else if (currentMode == MODE_MASCOT_ROUTINE) {
              pokeMascot();
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
            } else if (currentMode == MODE_MASCOT_ROUTINE) {
              pokeMascot();
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
    // A held scene (dashboard or Creator preview) keeps the screen awake
    display.setSleepAllowed(modeMaySleep(currentMode) && !bannerUp && mascotPreviewScene < 0);
    display.setAwakeBrightness(currentMode == MODE_AMBIENT && !bannerUp ? AmbientFace::BACKLIGHT : 255);

    // 6. Display Rendering (~20Hz tick)
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
        bool timeValid = getLocalTime(&timeinfo);
        if (timeValid) {
          currentHour = timeinfo.tm_hour;
          currentMin = timeinfo.tm_min;
          currentWday = timeinfo.tm_wday;
          currentMday = timeinfo.tm_mday;
          currentMon = timeinfo.tm_mon;
        }

        // Kubi's scene pick, kept current on every face so the dashboard can show it
        if (mascotPreviewScene >= 0 && now - mascotPreviewAt < mascotPreviewMs) {
          mascotSceneIndex = mascotPreviewScene;
        } else {
          mascotPreviewScene = -1;
          if (timeValid) {
            mascotSceneIndex = kubiScheduledScene(currentMon, currentMday, currentWday, currentHour, currentMin,
                                                  roomTemperature, birthdayMMDD);
          }
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
            display.drawMascotFace(roomTemperature, currentHour, mascotSceneIndex, true);
            break;

          case MODE_AMBIENT:
            display.drawAmbientFace();
            break;

          case MODE_MAZE:
            display.drawMazeFace();
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
  checkBatterySleep(); // before anything else: these checks must be quick
  checkNap();
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=================================");
  Serial.println("       PROJECT KUBI BOOTING      ");
  Serial.println("=================================");

  // A crash writes a core dump to the coredump partition; it stays until the
  // next crash overwrites it. Decode steps are in AGENTS.md.
  size_t dumpAddr = 0, dumpSize = 0;
  if (esp_core_dump_image_get(&dumpAddr, &dumpSize) == ESP_OK) {
    Serial.printf("[CRASH] A core dump from an earlier crash is stored (%u bytes at 0x%X)\n",
                  (unsigned)dumpSize, (unsigned)dumpAddr);
  }

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
  birthdayMMDD = preferences.getInt("bday", 0);
  chargeAlertHigh = preferences.getInt("alertHi", 0);
  chargeAlertLow = preferences.getInt("alertLo", 0);
  battery.setLearnedRates(preferences.getFloat("drainRate", 0.0f), preferences.getFloat("chargeRate", 0.0f));
  display.setSleepTimeoutMinutes(preferences.getInt("sleepMin", 5));
  audio.setVolume(preferences.getInt("volume", 25) / 100.0f); // dashboard "Sound"
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
