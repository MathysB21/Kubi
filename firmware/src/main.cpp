#include <Arduino.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <FastLED.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_TSL2591.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include "time.h"
#include <Dusk2Dawn.h>
#include <Preferences.h>
#include "API.h"
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <WiFiManager.h>
#include <SPIFFS.h>

// --- HARDWARE PINS & CONFIG ---
#define NUM_LEDS 144
#define DATA_PIN 13
// SDA is GPIO 21, SCL is GPIO 22 (Default hardware I2C, handled by the Wire library)

// Touch Pins
#define TOUCH_PROXIMITY 4
#define TOUCH_BTN_PWR_BRIGHT 15 // Button 1
#define TOUCH_BTN_TEMP 14       // Button 2
#define TOUCH_BTN_MODE 27       // Button 3

CRGB leds[NUM_LEDS];
Adafruit_TSL2591 tsl = Adafruit_TSL2591(2591);
Preferences preferences;
AsyncWebServer server(80); // Create web server on port 80

// --- WIFI & NTP CONFIG ---
const char* ntpServer  = "pool.ntp.org";
const int   daylightOffset_sec = 0;
bool isFirstBoot = true;

// --- ALARM CONFIG ---
volatile int alarmHour = 7;      // Default to 7:30 AM
volatile int alarmMinute = 30;

// --- ADVANCED CONFIGURATIONS & TIMEOUTS ---
volatile int cfgBrightMode = 2; // 0 = Variable, 1 = Stepped, 2 = Hybrid (Default)
volatile int nightLightBright = 128;  // Default 50% — resets each time Night Light is entered
volatile int cfgMaxGlow = 255;        // Default 100% proximity fillup
volatile int cfgTouchThreshold = 35;  // Default capacitive trigger limit
volatile int cfgAmbientThreshold = 100; // Default darkness limit for proximity
volatile int cfgProxThreshold = 30; // Default proximity capacitance value
volatile int cfgSundownHour = 18;
volatile int cfgSundownMinute = 0;
volatile int cfgModeTimeout = 40; // Minutes before Auto-exiting a fade state
volatile bool cfgProxEnabled = true;  // Set false to fully disable proximity hover sensing

// These will automatically update every midnight
volatile int dynamicSunriseHour = 7; 
volatile int dynamicSunriseMinute = 30;
volatile int dynamicSundownHour = 19;
volatile int dynamicSundownMinute = 0;
volatile bool forceTimeRecalc = false;

// --- FEATURE FLAGS & DURATIONS ---
volatile bool isSunriseEnabled = true;
volatile bool isSundownEnabled = true;
volatile int cfgSunriseDuration = 30; // In Minutes
volatile int cfgSundownDuration = 30; // In Minutes

// --- LOCATION SETTINGS ---
volatile float geoLat = -33.9321; // Defaults to Stellenbosch
volatile float geoLon = 18.8602;
volatile int tzOffset = 2;

// --- 3-STATE AUTOMATION ARRAYS (0=OFF, 1=AUTO, 2=MANUAL) ---
// Default: Sun Off, Mon-Fri Auto, Sat Off
volatile uint8_t sunriseDays[7] = {0, 1, 1, 1, 1, 1, 0}; 
// Default: All days Auto
volatile uint8_t sundownDays[7] = {1, 1, 1, 1, 1, 1, 1};

// --- TRIGGER TARGETS ---
volatile int triggerSunriseHour = 7; 
volatile int triggerSunriseMinute = 30;
volatile int triggerSundownHour = 18;
volatile int triggerSundownMinute = 0;

volatile bool isDaytime = true;

// --- PURE PLANETARY TIMES ---
volatile int planetDawnH = 6;
volatile int planetDawnM = 0;
volatile int planetDuskH = 18;
volatile int planetDuskM = 0;

// --- DIAGNOSTICS MEMORY ---
volatile int diagProx = 0;
volatile int diagB1 = 0;
volatile int diagB2 = 0;
volatile int diagB3 = 0;

// --- WIFI MANAGER TCP FIX ---
bool justSavedConfig = false;

void saveConfigCallback() {
  justSavedConfig = true;
}

const uint32_t SUNRISE_DURATION_MS = 1800000;
const uint32_t SUNDOWN_DURATION_MS = 1800000;
volatile uint32_t transitionStartTime = 0;

// --- FINITE STATE MACHINE ---
// NOTE: This now comes from the API file
// enum LampState { 
//   STATE_AUTO_DAY,     // Default daytime (Listening)
//   STATE_AUTO_NIGHT,   // Default nighttime (Listening)
//   STATE_MANUAL_DAY,   // Brightness 0-100%, Temp unlocked
//   STATE_MANUAL_NIGHT, // Brightness capped at 50%, Temp locked to Amber
//   STATE_NIGHT_LIGHT,  // Fixed 5% Amber
//   STATE_SUNRISE,      // Fading Up
//   STATE_SUNDOWN,      // Fading Down
//   STATE_PROXIMITY,    // The FillUp Glow
//   STATE_AWAY          // Long-hold lockdown
// };
// We use 'volatile' because this variable is shared between the two CPU cores
volatile LampState currentState = STATE_WIFI_SETUP; // Changed default from OFF
volatile LampState preProximityState = STATE_AUTO_DAY; 
volatile uint16_t ambientLight = 0; // Shared variable for the room's brightness

// --- DYNAMIC TARGET GLOBALS (THE WWA PARADIGM) ---
volatile int userTargetBright = 13;  // Safe dim default — prevents full-brightness white flash on boot
volatile int userTargetTemp = 0;     // 0 = Amber, 1 = Warm White, 2 = Cool White

// --- SUNDOWN SNAPSHOT MEMORY ---
// We now take a snapshot of the exact channel mix when sundown starts
volatile float snapSundownA = 255.0;
volatile float snapSundownW = 0.0;
volatile float snapSundownC = 0.0;
volatile uint8_t snapSundownBright = 255;

// --- ANIMATION GLOBALS ---
volatile bool playLimitWave = false;
volatile int waveDirection = 1; 
volatile bool playFillUp = false;
volatile bool renderAsGradient = false;    // Keeps gradient alive during fade out
volatile bool forceBrightnessSync = false; // Prevents flash when clicking ON during proximity

// --- FREERTOS TASK HANDLES ---
TaskHandle_t TaskLogic;
TaskHandle_t TaskLEDs;
TaskHandle_t TaskSensor;

// The gradient constraints
const uint8_t PROX_START_B = 60;
const uint8_t PROX_END_B = 10;

// ==========================================
// ABSTRACTION: THE MEXICAN WAVE FUNCTION
// ==========================================
void executeMexicanWave(float baseA, float baseW, float baseC, uint8_t baseBright, int direction) {
  int startIdx = (direction == 1) ? 0 : NUM_LEDS - 1;
  int endIdx = (direction == 1) ? NUM_LEDS + 25 : -25;
  int step = direction;

  // Calculate the master dimming multiplier once
  float masterScale = baseBright / 255.0;

  for (int i = startIdx; i != endIdx; i += step) {
    // Redraw the base background so it stays lit (using our new WWA math)
    fill_solid(leds, NUM_LEDS, CRGB((uint8_t)(baseA * masterScale), (uint8_t)(baseW * masterScale), (uint8_t)(baseC * masterScale)));
    
    // Draw the moving shadow
    for (int j = 0; j < 26; j++) {
      int shadowPixel = i - (j * step);
      if (shadowPixel >= 0 && shadowPixel < NUM_LEDS) {
        leds[shadowPixel].nscale8(50); 
      }
    }
    
    FastLED.setBrightness(255); // Keep FastLED master maxed, our masterScale handles the dimming
    FastLED.show();
    vTaskDelay(5 / portTICK_PERIOD_MS); // Speed of the wave
  }
}

// ==========================================
// ABSTRACTION: THE PROXIMITY FILLUP
// ==========================================
void executeFillUp(uint8_t startB, uint8_t endB, int speed, float tA, float tW, float tC) {
  FastLED.setBrightness(cfgMaxGlow); // Force master brightness to MAX so the gradient handles the dimming
  FastLED.clear();
  
  for (int i = 0; i < NUM_LEDS; i++) {
    // Dynamically calculate this pixel's brightness based on its index
    uint8_t pixelB = map(i, 0, NUM_LEDS - 1, startB, endB);
    float scale = pixelB / 255.0;
    
    // Apply the WWA gradient
    leds[i] = CRGB((uint8_t)(tA * scale), (uint8_t)(tW * scale), (uint8_t)(tC * scale));
    FastLED.show();
    vTaskDelay(speed / portTICK_PERIOD_MS); 
  }
}

// ==========================================
// ABSTRACTION: THE SUNRISE ALGORITHM
// ==========================================
// Calculates exact color and brightness based on how much time has passed
void calculateSunriseProgress(uint32_t elapsedMs, uint32_t totalDurationMs, 
                              float &tA, float &tW, float &tC, uint8_t &tB) {
  if (elapsedMs > totalDurationMs) elapsedMs = totalDurationMs;
  float progress = (float)elapsedMs / (float)totalDurationMs; // 0.0 to 1.0

  tB = (uint8_t)(progress * 255.0); // Smooth master brightness

  // PHASE 1 (0-60%): Pure Amber Glow
  if (progress < 0.6) {
    tA = 255.0; tW = 0.0; tC = 0.0;
  } 
  // PHASE 2 (60-100%): Crossfade into Warm White
  else {
    float mix = (progress - 0.6) * 2.5; // Scale the remaining 40% to a 0.0-1.0 multiplier
    tA = 255.0 * (1.0 - mix);
    tW = 255.0 * mix;
    tC = 0.0;
  }
}

// ==========================================
// ABSTRACTION: THE SUNDOWN ALGORITHM
// ==========================================
void calculateSundownProgress(uint32_t elapsedMs, uint32_t totalDurationMs, 
                              float sA, float sW, float sC, uint8_t sB, 
                              float &tA, float &tW, float &tC, uint8_t &tB) {
  if (elapsedMs > totalDurationMs) elapsedMs = totalDurationMs;
  float progress = (float)elapsedMs / (float)totalDurationMs;
  float reverseProgress = 1.0 - progress;

  tB = (uint8_t)(reverseProgress * (float)sB); // Fade out master brightness

  // Morph the color mix down to pure Amber over the first 20% of the fade out
  if (progress < 0.2) {
      float morph = progress * 5.0; // 0.0 to 1.0
      tA = sA + ((255.0 - sA) * morph);
      tW = sW * (1.0 - morph);
      tC = sC * (1.0 - morph);
  } else {
      tA = 255.0; tW = 0.0; tC = 0.0;
  }
}

// ==========================================
// ABSTRACTION: WI-FI LUNG FILL ANIMATION (WWA)
// ==========================================
void executeLungFill(uint32_t t) {
  float progress = 0.0;
  float fillLevel = 0.0; 

  // PHASE 1: Fade Up (0 to 3.0 seconds)
  if (t < 3000) { 
    progress = (float)t / 3000.0;
    float ease = (1.0 - cos(progress * PI)) / 2.0; 
    fillLevel = ease * NUM_LEDS;
  } 
  // PHASE 2: Hold Peak (3.0 to 3.5 seconds)
  else if (t < 3500) {
    fillLevel = NUM_LEDS;
  } 
  // PHASE 3: Fade Down (3.5 to 6.5 seconds)
  else if (t < 6500) {
    progress = (float)(t - 3500) / 3000.0;
    float ease = (1.0 - cos(progress * PI)) / 2.0;
    fillLevel = NUM_LEDS - (ease * NUM_LEDS);
  } 
  // PHASE 4: Hold Valley (6.5 to 7.0 seconds)
  else {
    fillLevel = 0.0;
  }

  FastLED.clear();
  
  const uint8_t peakAmber = 255; // Full brightness for the filled "lung"
  const uint8_t baseAmber = 15;  // Faint glow so the empty strip never looks dead

  // Sub-pixel smooth rendering
  int fullPixels = (int)fillLevel;
  float fraction = fillLevel - fullPixels;

  for (int i = 0; i < NUM_LEDS; i++) {
    if (i < fullPixels) {
      // Fully filled pixels
      leds[i] = CRGB(peakAmber, 0, 0);
    } else if (i == fullPixels) {
      // The leading-edge pixel (creates the smooth liquid effect)
      uint8_t blendedAmber = baseAmber + (uint8_t)((peakAmber - baseAmber) * fraction);
      leds[i] = CRGB(blendedAmber, 0, 0);
    } else {
      // Empty pixels (faint baseline glow)
      leds[i] = CRGB(baseAmber, 0, 0); 
    }
  }

  FastLED.setBrightness(255); // Maximize visibility, the sub-pixel math handles the dimming
  FastLED.show();
}

// ==========================================
// ABSTRACTION: 8-STEP BRIGHTNESS CALCULATOR
// Steps: 13(5%), 47, 82, 116, 151, 185, 220, 255(100%)
// ==========================================
int getNextBrightStep(int currentBright, bool goUp) {
  const uint8_t steps[8] = {13, 47, 82, 116, 151, 185, 220, 255};
  
  if (goUp) {
    for (int i = 0; i < 8; i++) {
      if (steps[i] > currentBright) return steps[i];
    }
    return 255; // Ceiling
  } else {
    for (int i = 7; i >= 0; i--) {
      if (steps[i] < currentBright) return steps[i];
    }
    return 13; // Floor (5%)
  }
}

// ==========================================
// CORE 1: DEDICATED LED ANIMATION ENGINE
// ==========================================
void ledTaskCode(void * parameter) {
  uint8_t currentBrightness = 0;
  uint8_t targetBrightness = 0;

  // The independent WWA channel trackers
  float currentA = 255.0, currentW = 0.0, currentC = 0.0;
  float targetA = 255.0, targetW = 0.0, targetC = 0.0;

  static LampState lastState = STATE_AUTO_DAY; // Used for the sundown
  static bool hasCompletedSetupFade = false;

  for(;;) {
    // --- WI-FI SETUP BREATHING ANIMATION ---
    if (currentState == STATE_WIFI_SETUP) {
      uint32_t t = millis() % 7000;
      executeLungFill(t);
      delay(15);
      continue; 
    }
    else if (!hasCompletedSetupFade) {
      // --- THE GRACEFUL HANDOFF ---
      // First let the current breath cycle drain to its natural valley (t >= 6500ms)
      // so the lamp visibly "exhales" to near-black before fading out — looks intentional.
      Serial.println("ANIMATION: Waiting for breath valley before fade...");
      uint32_t valleyDeadline = millis() + 7000; // safety: max 1 full cycle
      while (millis() < valleyDeadline) {
        uint32_t t = millis() % 7000;
        executeLungFill(t);
        delay(15);
        if (t >= 6500) break; // Valley reached — strip is near-empty
      }

      // Gracefully fade whatever tiny residual remains to black
      Serial.println("ANIMATION: Setup complete, gracefully fading out...");
      for (int step = 0; step < 60; step++) {
        fadeToBlackBy(leds, NUM_LEDS, 12);
        FastLED.show();
        delay(15);
      }
      
      FastLED.clear();
      FastLED.show();
      
      hasCompletedSetupFade = true;
      currentBrightness = 0; // Sync the environment baseline
    }

    // --- SNAPSHOT INTERCEPTOR ---
    if (currentState == STATE_SUNDOWN && lastState != STATE_SUNDOWN) {
			snapSundownA = currentA;
      snapSundownW = currentW;
      snapSundownC = currentC;
      snapSundownBright = currentBrightness;

			Serial.println("ANIMATION: Sundown Snapshot Captured.");
    }
    lastState = currentState;

    // --- TRIGGER THE FILLUP ANIMATION ---
    if (playFillUp) {
      // Snap current color instantly to Amber
      currentA = 255.0; currentW = 0.0; currentC = 0.0;

      executeFillUp(PROX_START_B, PROX_END_B, 5, currentA, currentW, currentC);
      playFillUp = false;
      currentBrightness = cfgMaxGlow; // Lock master brightness High so the gradient shows
    }

    // --- TRIGGER THE WAVE FUNCTION ---
    if (playLimitWave) {
      executeMexicanWave(currentA, currentW, currentC, currentBrightness, waveDirection);
      playLimitWave = false; // Reset the flag so it only plays once per trigger
    }

    // --- PREVENT PROXIMITY -> MANUAL FLASH ---
    if (forceBrightnessSync) {
      currentBrightness = PROX_START_B; // Drops master to match perceived gradient brightness
      forceBrightnessSync = false;
    }

    // --- 1. DETERMINE TARGETS BASED ON TIME & STATE ---
    if (currentState == STATE_SUNRISE) {
      // SUNRISE
      uint32_t elapsed = millis() - transitionStartTime;
      calculateSunriseProgress(elapsed, (cfgSunriseDuration * 60000), targetA, targetW, targetC, targetBrightness);
    } 
    else if (currentState == STATE_SUNDOWN) {
      // SUNDOWN
      uint32_t elapsed = millis() - transitionStartTime;
      calculateSundownProgress(elapsed, (cfgSundownDuration * 60000), snapSundownA, snapSundownW, snapSundownC, snapSundownBright, targetA, targetW, targetC, targetBrightness);
    } 
    else {
      // Normal State Handling
      switch(currentState) {
        case STATE_AUTO_DAY: 
        case STATE_AUTO_NIGHT:
        case STATE_AWAY:
          targetBrightness = 0; 
          break;
        case STATE_PROXIMITY: 
          targetBrightness = cfgMaxGlow;
          break;
        case STATE_MANUAL_DAY: 
          targetBrightness = userTargetBright; 
          break;
        case STATE_MANUAL_NIGHT: 
          // CAP AT 50% BRIGHTNESS (128 out of 255)
          targetBrightness = (userTargetBright < 128) ? userTargetBright : 128;
          break;
        case STATE_NIGHT_LIGHT: 
          // Uses userTargetBright, seeded to nightLightBright when entered
          targetBrightness = userTargetBright; 
          break;
        default: break;
      }

      // 2. Determine Target Color
      // Manual Day, Manual Night, and Night Light all honour userTargetTemp
      if (currentState == STATE_MANUAL_DAY || currentState == STATE_MANUAL_NIGHT || currentState == STATE_NIGHT_LIGHT) {
        if (userTargetTemp == 0) {
          targetA = 255.0; targetW = 0.0; targetC = 0.0; // Amber
        } else if (userTargetTemp == 1) {
          targetA = 0.0; targetW = 255.0; targetC = 0.0; // Warm White
        } else {
          targetA = 0.0; targetW = 0.0; targetC = 255.0; // Cool White
        }
      } else {
        // Auto, Proximity, transitions — force pure Amber
        targetA = 255.0; targetW = 0.0; targetC = 0.0;
      }
    }

    // THE SHUTDOWN FIX: If we are turning off, lock the target colors so they don't weirdly shift during the fadeout
    if (targetBrightness == 0) {
        targetA = currentA; targetW = currentW; targetC = currentC;
    }

    // --- 3. SMOOTH DIMMING MATH ---
    if (currentBrightness < targetBrightness) {
      currentBrightness++;
    } else if (currentBrightness > targetBrightness) {
      // If fading out the gradient, drop brightness 5x faster
      if ((currentState == STATE_AUTO_DAY || currentState == STATE_AUTO_NIGHT) && renderAsGradient) {
        currentBrightness = (currentBrightness > 5) ? currentBrightness - 5 : 0;
      } else {
        currentBrightness--;
      }
    }

    // CLEANUP: Turn off gradient renderer when fully black
    if ((currentState == STATE_AUTO_DAY || currentState == STATE_AUTO_NIGHT) && currentBrightness == 0) {
      renderAsGradient = false;
    }

    // --- 4. SMOOTH WWA CROSSFADING ---
    float fadeSpeed = 3.0; // The larger the number, the faster the temperature glides
    if (currentA < targetA) currentA = min(targetA, currentA + fadeSpeed);
    if (currentA > targetA) currentA = max(targetA, currentA - fadeSpeed);
    
    if (currentW < targetW) currentW = min(targetW, currentW + fadeSpeed);
    if (currentW > targetW) currentW = max(targetW, currentW - fadeSpeed);
    
    if (currentC < targetC) currentC = min(targetC, currentC + fadeSpeed);
    if (currentC > targetC) currentC = max(targetC, currentC - fadeSpeed);

    // --- 5. CONDITIONAL RENDERING ---
    // Keep rendering the gradient even if we are fading to OFF
    // Multiply the channels by the master brightness scale (0.0 to 1.0)
    float masterScale = currentBrightness / 255.0;
    if (renderAsGradient) {
      for (int i = 0; i < NUM_LEDS; i++) {
        uint8_t pixelB = map(i, 0, NUM_LEDS - 1, PROX_START_B, PROX_END_B);
        // BUG 12b FIX: Combine the gradient scale WITH the master dimming scale!
        float finalScale = (pixelB / 255.0) * masterScale;
        leds[i] = CRGB((uint8_t)(currentA * finalScale), (uint8_t)(currentW * finalScale), (uint8_t)(currentC * finalScale));
      }
    } else {
      CRGB finalColor = CRGB((uint8_t)(currentA * masterScale), (uint8_t)(currentW * masterScale), (uint8_t)(currentC * masterScale));
      fill_solid(leds, NUM_LEDS, finalColor);
    }
    
    // We override FastLED's master brightness because we handled the math manually above for supreme smoothness
    FastLED.setBrightness(255); 
    FastLED.show();
    
    // 6. Dynamic Fade Speed
    // If we are fading in the dark/proximity ranges, slow the loop down so it doesn't snap.
    // If we are blasting up to 200 for manual mode, speed it up!
    int loopSpeed = (currentBrightness < 40) ? 10 : 5; 
    vTaskDelay(loopSpeed / portTICK_PERIOD_MS);
  }
}

// ======================================================
// CORE 0: NTP POLLING, ASTRONOMY & SENSORS (Background)
// ======================================================
void sensorTaskCode(void * parameter) {
  struct tm timeinfo;
  int lastCalculatedDay = -1; // Memory to ensure we only run the heavy math once a day

  for(;;) {
    // Re-apply sensor config each tick — harmless if already correct, but guarantees
    // recovery if the TSL2591 lost its settings after a power-blip on the sensor rail.
    tsl.setGain(TSL2591_GAIN_MED);
    tsl.setTiming(TSL2591_INTEGRATIONTIME_100MS);
    ambientLight = tsl.getLuminosity(TSL2591_VISIBLE);
    
    // Check the local time from the NTP server
    if (getLocalTime(&timeinfo)) {
      // --- 1. THE MIDNIGHT ASTRONOMY CALCULATOR ---
      if (timeinfo.tm_mday != lastCalculatedDay || forceTimeRecalc) {

        Dusk2Dawn dynamicSky(geoLat, geoLon, tzOffset);

        // 1A. Always calculate Mother Earth's actual time so the touch buttons know the state
        int planetSunriseMins = dynamicSky.sunrise(timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, false);
        int planetSundownMins = dynamicSky.sunset(timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, false);

        // SAVE THE PURE TIMES FOR THE REACT UI
        planetDawnH = planetSunriseMins / 60;
        planetDawnM = planetSunriseMins % 60;
        planetDuskH = planetSundownMins / 60;
        planetDuskM = planetSundownMins % 60;

        // 1B. Evaluate Today's 3-State Triggers
        int todaySR = sunriseDays[timeinfo.tm_wday];
        int todaySD = sundownDays[timeinfo.tm_wday];

        // ROUTE SUNRISE (Calculate the exact start time by subtracting the duration)
				// 0 = OFF, 1 = AUTO, 2 = ON
        if (todaySR == 0) {
          triggerSunriseHour = 25; // Disabled
          dynamicSunriseHour = 0;  // UI Disabled
          dynamicSunriseMinute = 0;
        } else {
          // Get the target time in raw minutes
          int targetMins = (todaySR == 1) ? planetSunriseMins : ((alarmHour * 60) + alarmMinute);
          
          // Subtract the duration to find the START time
          int startMins = targetMins - cfgSunriseDuration;
          if (startMins < 0) startMins += 1440; 
          
          triggerSunriseHour = startMins / 60;
          triggerSunriseMinute = startMins % 60;

          // UPDATE THE UI VARIABLES (The Target Time)
          dynamicSunriseHour = targetMins / 60;
          dynamicSunriseMinute = targetMins % 60;
        }

        // ROUTE SUNDOWN (Starts exactly on time)
        if (todaySD == 0) {
          triggerSundownHour = 25;
          dynamicSundownHour = 0; 
          dynamicSundownMinute = 0;
        } else {
          // Get the exact target time
          int targetMins = (todaySD == 1) ? planetSundownMins : ((cfgSundownHour * 60) + cfgSundownMinute);
          
          triggerSundownHour = targetMins / 60;
          triggerSundownMinute = targetMins % 60;
          
          // UPDATE THE UI VARIABLES (The Target Time)
          dynamicSundownHour = targetMins / 60;
          dynamicSundownMinute = targetMins % 60;
        }

        lastCalculatedDay = timeinfo.tm_mday;
        forceTimeRecalc = false;
      }

      // --- 1.5 DETERMINE CURRENT DAY/NIGHT STATUS ---
      // We use the actual Planet variables here, so the touch buttons never break!
      Dusk2Dawn dynamicSky(geoLat, geoLon, tzOffset);
      int planetSunriseMins = dynamicSky.sunrise(timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, false);
      int planetSundownMins = dynamicSky.sunset(timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, false);
      
      int currentMins = timeinfo.tm_hour * 60 + timeinfo.tm_min;
      isDaytime = (currentMins >= planetSunriseMins && currentMins < planetSundownMins);

      // --- 1.75 INITIAL BOOT STATE SYNC ---
      static bool isFirstStateSync = true;
      if (isFirstStateSync) {
				currentState = isDaytime ? STATE_AUTO_DAY : STATE_AUTO_NIGHT;
				preProximityState = currentState; // Keep proximity memory in sync
				isFirstStateSync = false;
				Serial.printf("BOOT SYNC: Lamp initialized to %s\n", isDaytime ? "Auto Day" : "Auto Night");
      }

      // --- 2. TIME-BASED STATE TRANSITIONS ---
      // Trigger Sunrise (ONLY IF ENABLED)
      if (isSunriseEnabled && timeinfo.tm_hour == triggerSunriseHour && timeinfo.tm_min == triggerSunriseMinute) {
        // Only trigger if we aren't already manually using the lamp during the day
        if (currentState != STATE_SUNRISE && currentState != STATE_MANUAL_DAY && currentState != STATE_AWAY) {
          currentState = STATE_SUNRISE;
          transitionStartTime = millis();
          Serial.println("⏰ EVENT: Commencing Sunrise Mode");
        }
      }

      // Trigger Sundown (ONLY IF ENABLED)
      if (isSundownEnabled && timeinfo.tm_hour == triggerSundownHour && timeinfo.tm_min == triggerSundownMinute) {
        // Skip sundown fade if the lamp is already in Auto_Day (just quietly snap to Auto_Night)
        if (currentState == STATE_AUTO_DAY) {
          currentState = STATE_AUTO_NIGHT;
          Serial.println("⏰ EVENT: Quietly transitioned to Auto Night");
        } 
        else if (currentState != STATE_SUNDOWN && currentState != STATE_AUTO_NIGHT && currentState != STATE_AWAY) {
          // If the lamp is actively on, run the Sundown fade sequence
          currentState = STATE_SUNDOWN;
          transitionStartTime = millis();
          Serial.println("⏰ EVENT: Commencing Sundown Fade");
        }
      }

      // --- 3. MODE TIMEOUT SAFETY NET ---
      uint32_t currentModeDuration = millis() - transitionStartTime;
      uint32_t timeoutLimitMs = cfgModeTimeout * 60000;

      if (currentState == STATE_SUNRISE && (currentModeDuration > timeoutLimitMs)) {
        currentState = STATE_AUTO_DAY;
        Serial.println("TIMEOUT: Sunrise mode completed -> Auto Day");
      }
      
      if (currentState == STATE_SUNDOWN && (currentModeDuration > timeoutLimitMs)) {
        currentState = STATE_AUTO_NIGHT;
        Serial.println("TIMEOUT: Sundown mode completed -> Auto Night");
      }
    }
    
    vTaskDelay(1000 / portTICK_PERIOD_MS); // Run once a second
  }
}

// ==========================================
// CORE 0: LOGIC AND TOUCH ENGINE
// ==========================================
void logicTaskCode(void * parameter) {
  // --- BUTTON 1: POWER / BRIGHTNESS ---
  bool b1Touched = false; 
  uint32_t b1Time = 0; 
  bool b1Handled = false; 
  bool dimmingUp = true; 
  bool hitBoundary = false; 
  uint32_t lastWaveTime = 0;

  // --- BUTTON 2: TEMPERATURE ---
  bool b2Touched = false; 
  uint32_t b2Time = 0; 
  bool b2Handled = false; 
  int tempIndex = 0;
  const uint8_t hues[] = {25, 40, 160}; 
  const uint8_t sats[] = {255, 150, 10};

  // --- BUTTON 3: MODE ---
  bool b3Touched = false; 
  uint32_t b3Time = 0; 
  bool b3Handled = false;

  // --- PROXIMITY TIMER ---
  uint32_t lastProxTriggerTime = 0;
  const uint32_t PROX_TIMEOUT_MS = 10000; // 10 seconds

  // --- MOVING MEDIAN FILTER ---
  const int numReadings = 5;
  int proxReadings[numReadings] = {60, 60, 60, 60, 60}; // Seed with high (un-triggered) values
  int readIndex = 0;
  
  for(;;) {
    // --- PROXIMITY MEDIAN FILTER ---
    proxReadings[readIndex] = touchRead(TOUCH_PROXIMITY);
    readIndex = (readIndex + 1) % numReadings;

    // Create a temporary array to sort
    int sorted[numReadings];
    memcpy(sorted, proxReadings, sizeof(proxReadings));
    
    // Simple bubble sort
    for (int i = 0; i < numReadings - 1; i++) {
        for (int j = 0; j < numReadings - i - 1; j++) {
            if (sorted[j] > sorted[j+1]) {
                int temp = sorted[j];
                sorted[j] = sorted[j+1];
                sorted[j+1] = temp;
            }
        }
    }

    // The median is the true value, ignoring the wild 0s and 40s
    int valProx = sorted[numReadings / 2];

    // Save to globals for the Diagnostics API
    diagProx = valProx;
    diagB1   = touchRead(TOUCH_BTN_PWR_BRIGHT);
    diagB2   = touchRead(TOUCH_BTN_TEMP);
    diagB3   = touchRead(TOUCH_BTN_MODE);

    // Continue using them for logic
    int valB1   = diagB1;
    int valB2   = diagB2;
    int valB3   = diagB3;
    
    // --------------------------------------------------------
    // 1. BRIGHTNESS BUTTON (Purely intensity) (GPIO 15)
    // --------------------------------------------------------
    static uint32_t lastWallHitTime = 0;
    static uint32_t lastStepScrollTime = 0;

    if (valB1 < cfgTouchThreshold) {
      // FINGER DOWN
      if (!b1Touched) { 
        b1Touched = true; 
        b1Time = millis(); 
        b1Handled = false; 
        hitBoundary = false;
      }

      // THE DEAD ZONE: If the lamp is completely off, ignore the button.
      if (currentState == STATE_AUTO_DAY || currentState == STATE_AUTO_NIGHT || currentState == STATE_AWAY) {
         // Do nothing. Wait for user to let go.
      } else {
        // HOLD LOGIC (>400ms)
        if (millis() - b1Time > 400) {
          
          // FIRST MOMENT OF HOLD TRIGGER
          if (!b1Handled) {
             // Take control from automated animations
             if (currentState != STATE_MANUAL_DAY && currentState != STATE_MANUAL_NIGHT && currentState != STATE_NIGHT_LIGHT) {
                 if (renderAsGradient) forceBrightnessSync = true;
                 currentState = isDaytime ? STATE_MANUAL_DAY : STATE_MANUAL_NIGHT;
                 renderAsGradient = false;
             }
             // If in strict Stepped Mode, holding immediately reverses direction before auto-scrolling
             if (cfgBrightMode == 1) {
                 dimmingUp = !dimmingUp;
             }
          }
          b1Handled = true;

          int maxLimit = 255; // All modes should have full range
          int minLimit = 13; // 5% absolute floor

          if (cfgBrightMode == 1) {
            // ==========================================
            // MODE 1: STEPPED (Hold = Auto-scroll 350ms)
            // ==========================================
            if (millis() - lastStepScrollTime > 350) {
                int nextStep = getNextBrightStep(userTargetBright, dimmingUp);
                if (nextStep > maxLimit) nextStep = maxLimit;
                
                if (userTargetBright == nextStep) { 
                    playLimitWave = true;
                    waveDirection = dimmingUp ? 1 : -1;
                } else {
                    userTargetBright = nextStep;
                }
                lastStepScrollTime = millis();
            }
          } else {
            // ==========================================
            // MODE 0 / 2: VARIABLE & HYBRID (Hold = Smooth Fade)
            // ==========================================
            if (!hitBoundary) {
              if (dimmingUp) {
                if (userTargetBright < maxLimit) userTargetBright += 5;
                else {
                  userTargetBright = maxLimit;
                  hitBoundary = true;
                  playLimitWave = true;
                  waveDirection = 1;
                  lastWaveTime = millis();
                }
              } else {
                if (userTargetBright > minLimit) userTargetBright -= 5;
                else {
                  userTargetBright = minLimit;
                  hitBoundary = true;
                  playLimitWave = true;
                  waveDirection = -1;
                  lastWaveTime = millis();
                }
              }
            } else {
              // 3-SECOND LIMIT HEARTBEAT
              if (millis() - lastWaveTime > 3000) {
                playLimitWave = true;
                lastWaveTime = millis();
              }
            }
          }
          vTaskDelay(20 / portTICK_PERIOD_MS); 
        }
      }
    } else { 
      // FINGER RELEASED
      if (b1Touched) {
        // TAP LOGIC (<400ms)
        if (!b1Handled && (millis() - b1Time < 400)) {
            
            // Only process tap if lamp is actually ON
            if (currentState != STATE_AUTO_DAY && currentState != STATE_AUTO_NIGHT && currentState != STATE_AWAY) {
                
                // Intercept automations
                if (currentState != STATE_MANUAL_DAY && currentState != STATE_MANUAL_NIGHT && currentState != STATE_NIGHT_LIGHT) {
                     if (renderAsGradient) forceBrightnessSync = true;
                     currentState = isDaytime ? STATE_MANUAL_DAY : STATE_MANUAL_NIGHT;
                     renderAsGradient = false;
                }

                if (cfgBrightMode == 0) {
                    // MODE 0: VARIABLE (Tap does absolutely nothing)
                    Serial.println("TAP: Ignored (Variable Mode Active)");
                } else {
                    // ==========================================
                    // MODE 1 & 2: STEPPED & HYBRID (Tap = 1 Step)
                    // ==========================================
                    int maxLimit = 255; // All modes should have full range
                    int nextStep = getNextBrightStep(userTargetBright, dimmingUp);
                    if (nextStep > maxLimit) nextStep = maxLimit;

                    if (userTargetBright == nextStep) {
                        // HIT THE WALL
                        playLimitWave = true;
                        waveDirection = dimmingUp ? 1 : -1;
                        
                        // 5-SECOND REVERSAL WINDOW
                        if (millis() - lastWallHitTime < 5000) {
                            dimmingUp = !dimmingUp; 
                            userTargetBright = getNextBrightStep(userTargetBright, dimmingUp); 
                            lastWallHitTime = 0; // Clear window
                            Serial.println("TAP: 5s Window Reversal Triggered");
                        } else {
                            lastWallHitTime = millis(); // Open window
                            Serial.println("TAP: Wall hit. Window opened.");
                        }
                    } else {
                        // SUCCESSFUL STEP
                        userTargetBright = nextStep;
                        lastWallHitTime = 0; // Clear window if they successfully moved
                    }
                }
            }
        }

        // If held in Variable/Hybrid, reverse direction for NEXT time. 
        if (b1Handled && (cfgBrightMode == 0 || cfgBrightMode == 2)) {
          dimmingUp = !dimmingUp; 
        }

        // Save user brightness value (Need to open and close prefs to keep things atomic)
        preferences.begin("lamp_settings", false);
        preferences.putInt("userBright", userTargetBright);
        preferences.end();

        b1Touched = false;
        hitBoundary = false; 
      }
    }

    // --------------------------------------------------------
    // 2. TEMPERATURE BUTTON (GPIO 14)
    // --------------------------------------------------------
    if (valB2 < cfgTouchThreshold) {
      if (!b2Touched) { 
        b2Touched = true; 
        b2Time = millis(); 
        b2Handled = false; 
      }
    } else {
      // FINGER RELEASED (TAP ONLY)
      if (b2Touched) {
        if (!b2Handled && (millis() - b2Time < 400)) {
          // Temp button works in any active (lit) manual state
          if (currentState == STATE_MANUAL_DAY || currentState == STATE_MANUAL_NIGHT || currentState == STATE_NIGHT_LIGHT) {
            tempIndex = (userTargetTemp + 1) % 3;
            userTargetTemp = tempIndex;
            Serial.println("TAP: Temp Cycled");

            // Save the user's last chosen temperature (Need to open and close prefs to keep things atomic)
            preferences.begin("lamp_settings", false);
            preferences.putInt("userTemp", userTargetTemp);
            preferences.end();
          }
        }
        b2Touched = false;
      }
    }

    // --------------------------------------------------------
    // 3. MODE BUTTON (GPIO 27)
    // --------------------------------------------------------
    if (valB3 < cfgTouchThreshold) {
      if (!b3Touched) { 
        b3Touched = true; 
        b3Time = millis(); 
        b3Handled = false; 
      } else {
        uint32_t heldTime = millis() - b3Time;
        
        // 1-SECOND HOLD: Transition Overrides
        if (heldTime > 1000 && !b3Handled) {
          if (currentState == STATE_SUNRISE) {
						b3Handled = true; 
						currentState = STATE_AUTO_DAY; 
						Serial.println("HOLD: Sunrise Skipped -> Auto Day (Sleeping In)");
          } else if (currentState == STATE_SUNDOWN) {
						b3Handled = true; 
						currentState = STATE_AUTO_NIGHT; 
						Serial.println("HOLD: Sundown Skipped -> Auto Night (Already Asleep)");
          }
        }
        
        // 3-SECOND LONG HOLD: Away Profile Toggle
        if (heldTime > 3000) {
          b3Handled = true; 
          
          if (currentState != STATE_AWAY) {
            currentState = STATE_AWAY;
            Serial.println("LONG HOLD: Away Profile Activated");
          } else {
            currentState = STATE_AUTO_DAY; 
            Serial.println("LONG HOLD: Away Profile Deactivated -> Auto Day");
          }
        }
      }
    } else {
      // FINGER RELEASED (TAP LOGIC)
      if (b3Touched) {
        if (!b3Handled && (millis() - b3Time < 400)) {
          // Only play the confirmation wave when interrupting an automated sequence.
          // Don't play it when turning OFF — the lamp is about to fade to black anyway,
          // and at night-light brightness the wave wrongly flashes brighter.
          if (currentState == STATE_SUNRISE || currentState == STATE_SUNDOWN) {
            playLimitWave = true;
          }

          if (currentState == STATE_AUTO_DAY) {
						if (renderAsGradient) forceBrightnessSync = true;
						currentState = STATE_MANUAL_DAY; renderAsGradient = false;
          } 
          else if (currentState == STATE_AUTO_NIGHT) {
            // Seed brightness + colour to defaults each time Night Light is entered
            userTargetTemp = 0;               // Default: Amber
            userTargetBright = nightLightBright; // Seed from saved default (50%)
            currentState = STATE_NIGHT_LIGHT;
          } 
          else if (currentState == STATE_PROXIMITY) {
            currentState = isDaytime ? STATE_MANUAL_DAY : STATE_NIGHT_LIGHT;
          } 
          else if (currentState == STATE_MANUAL_DAY || currentState == STATE_NIGHT_LIGHT || currentState == STATE_MANUAL_NIGHT) {
						// THE SMART FALLBACK: Don't guess, check the sun!
						currentState = isDaytime ? STATE_AUTO_DAY : STATE_AUTO_NIGHT;
          }
          else if (currentState == STATE_SUNRISE) {
						currentState = STATE_MANUAL_DAY;
						userTargetBright = 255; // Restore 100% brightness since sunrise was interrupted
						Serial.println("TAP: Sunrise Interrupted -> Manual Day");
          } 
          else if (currentState == STATE_SUNDOWN) {
						currentState = STATE_NIGHT_LIGHT;
						Serial.println("TAP: Sundown Interrupted -> Night Light");
          }
        }
        b3Touched = false;
      }
    }

    // --------------------------------------------------------
    // 4. PROXIMITY HOVER (GPIO 4)
    // --------------------------------------------------------
    // Decoupled from time. Only listens if enabled and lamp is in a pure Auto state.
    if (cfgProxEnabled && (currentState == STATE_AUTO_DAY || currentState == STATE_AUTO_NIGHT)) {
      
      // If hand is near AND room is dark
      if (valProx < cfgProxThreshold && ambientLight < cfgAmbientThreshold) { 
        lastProxTriggerTime = millis(); // Reset the 10s timer

        if (currentState != STATE_PROXIMITY) {
          preProximityState = currentState; // MEMORY: Save if we were OFF or SLEEP
          currentState = STATE_PROXIMITY;
          playFillUp = true; // Fire the entrance animation!
          renderAsGradient = true; // Turn gradient rendering ON
          Serial.println("PROXIMITY: FillUp Triggered");
        }
      } 
    }

    // While in Proximity, check if we need to time out
    if (currentState == STATE_PROXIMITY) {
      if (valProx < cfgProxThreshold && ambientLight < cfgAmbientThreshold) {
        lastProxTriggerTime = millis(); // Keep the timer alive as long as the hand is there
      }
      else if (millis() - lastProxTriggerTime > PROX_TIMEOUT_MS) {
        currentState = preProximityState; // RESTORE: Safely go back to Auto Day or Auto Night
        Serial.println("PROXIMITY: Fading to Auto State");
      }
    }
    
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}

// ==========================================
// SETUP
// ==========================================
void saveNetworkToMemory(String ssid, String pass) {
  Preferences wifiPrefs;
  wifiPrefs.begin("wifi_memory", false);
  
  int count = wifiPrefs.getInt("count", 0);
  
  // Check if it already exists
  for (int i = 0; i < count; i++) {
    String ssidKey = "ssid_" + String(i);
    if (wifiPrefs.getString(ssidKey.c_str(), "") == ssid) {
      String passKey = "pass_" + String(i);
      wifiPrefs.putString(passKey.c_str(), pass);
      wifiPrefs.end();
      return;
    }
  }
  
  // If not, add it. Limit to 5 networks.
  if (count >= 5) {
    for (int i = 1; i < 5; i++) {
      String oldSsidKey = "ssid_" + String(i);
      String oldPassKey = "pass_" + String(i);
      String newSsidKey = "ssid_" + String(i-1);
      String newPassKey = "pass_" + String(i-1);
      wifiPrefs.putString(newSsidKey.c_str(), wifiPrefs.getString(oldSsidKey.c_str(), ""));
      wifiPrefs.putString(newPassKey.c_str(), wifiPrefs.getString(oldPassKey.c_str(), ""));
    }
    count = 4;
  }
  
  // Add the new one at 'count'
  String sKey = "ssid_" + String(count);
  String pKey = "pass_" + String(count);
  wifiPrefs.putString(sKey.c_str(), ssid);
  wifiPrefs.putString(pKey.c_str(), pass);
  
  wifiPrefs.putInt("count", count + 1);
  wifiPrefs.end();
  Serial.println("Saved new network to memory: " + ssid);
}

void connectToWiFi() {
  Serial.println("\nChecking for Known Networks...");
  
  WiFiMulti wifiMulti;
  
  // 1. Add native ESP32 NVS network just in case it's not in our list yet
  if (WiFi.SSID().length() > 0) {
    wifiMulti.addAP(WiFi.SSID().c_str(), WiFi.psk().c_str());
  }
  
  // 2. Load our custom multi-network list from Preferences
  Preferences wifiPrefs;
  wifiPrefs.begin("wifi_memory", false);
  int count = wifiPrefs.getInt("count", 0);
  for(int i = 0; i < count; i++) {
    String ssidKey = "ssid_" + String(i);
    String passKey = "pass_" + String(i);
    String s = wifiPrefs.getString(ssidKey.c_str(), "");
    String p = wifiPrefs.getString(passKey.c_str(), "");
    if(s.length() > 0) {
      wifiMulti.addAP(s.c_str(), p.c_str());
      Serial.println("Loaded Known Network: " + s);
    }
  }
  wifiPrefs.end();

  WiFi.mode(WIFI_STA);
  Serial.print("Connecting via WiFiMulti...");
  
  // 3. Try to connect for ~10 seconds
  bool connected = false;
  uint32_t startAttempt = millis();
  while(millis() - startAttempt < 10000) {
    if(wifiMulti.run() == WL_CONNECTED) {
      connected = true;
      break;
    }
    delay(500);
    Serial.print(".");
  }

  // 4. If connected via WiFiMulti, we're done! Skip WiFiManager.
  if (connected) {
    Serial.println("\nSuccessfully connected to a known network!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    return;
  }
  
  // 5. If WiFiMulti failed, boot WiFiManager portal
  Serial.println("\nNo known networks found in range. Initializing Wi-Fi Manager...");
  WiFiManager wifiManager;

  // Build the JavaScript payload for the auto-fill feature
  String jsNetworks = "<script>var savedNetworks = {";
  wifiPrefs.begin("wifi_memory", false);
  count = wifiPrefs.getInt("count", 0);
  for(int i = 0; i < count; i++) {
    String s = wifiPrefs.getString(("ssid_" + String(i)).c_str(), "");
    String p = wifiPrefs.getString(("pass_" + String(i)).c_str(), "");
    if(s.length() > 0) {
      if (i > 0) jsNetworks += ",";
      jsNetworks += "\"" + s + "\":\"" + p + "\"";
    }
  }
  jsNetworks += "};</script>";
  wifiPrefs.end();

  wifiManager.setCustomHeadElement(jsNetworks.c_str());

  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.setConfigPortalTimeout(180); // 3 minutes

  if (!wifiManager.autoConnect("Sunrise-Setup")) {
    Serial.println("Failed to connect or hit timeout. Rebooting...");
    delay(3000);
    ESP.restart();
  }

  // 6. THE WARM BOOT FLUSH
  if (justSavedConfig) {
    // Save the new network to our Preferences memory!
    String newSSID = WiFi.SSID();
    String newPass = WiFi.psk();
    
    if (newSSID.length() > 0) {
      saveNetworkToMemory(newSSID, newPass);
    }
    
    Serial.println("WIFI: Credentials saved! Warm-booting to flush Safari TCP sockets...");
    delay(1000); 
    ESP.restart(); 
  }

  // 7. SAFE TO PROCEED 
  WiFi.mode(WIFI_STA);
  Serial.println("\nWiFi Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void setupOTA () {
  // --- OVER-THE-AIR (OTA) SETUP ---
  ArduinoOTA.setHostname("sunrise"); // Matches your mDNS name
  // ArduinoOTA.setPassword("admin"); // Optional: Uncomment to require a password for uploads

  ArduinoOTA.onStart([]() {
    Serial.println("\n--- OTA UPDATE STARTED ---");
    // If you are using SPIFFS/LittleFS, you need to unmount it before an OTA update!
    SPIFFS.end(); 
    
    // Optional visual feedback: turn LEDs off or to a specific color
    FastLED.clear();
    FastLED.show();
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("\n--- OTA UPDATE COMPLETE ---");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });

  ArduinoOTA.begin();
  Serial.println("OTA Ready!");
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // --- 1. START LED CORE FIRST (For Setup Animation) ---
  // We initialize the LEDs and pin the task to Core 1 immediately so it 
  // can run the breathing animation while WiFiManager blocks Core 0.
  // FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS); // OLD, this is for RGB strip
  FastLED.addLeds<SK6812, DATA_PIN, BRG>(leds, NUM_LEDS);
  // FastLED.setMaxPowerInVoltsAndMilliamps(5, 400); // PC SAFETY LIMIT
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 3500); // For when on 4A PSU, not PC
  FastLED.clear();
  FastLED.show();
  xTaskCreatePinnedToCore(ledTaskCode, "TaskLEDs", 10000, NULL, 1, &TaskLEDs, 1);

  // --- 2. CONNECT TO WIFI ---
  connectToWiFi(); // WiFiManager blocks here while waiting for credentials

  // --- 2.5 START OTA LISTENING ---
  setupOTA();

  // --- 2.75 THE NUCLEAR TCP PORT 80 GHOST FIX ---
  // WiFiManager leaves the ESP32 in "AP + Station" mode. 
  // Forcing it strictly into "Station" mode physically tears down the AP interface 
  // and instantly reclaims the locked TCP socket.
  WiFi.mode(WIFI_STA);
  delay(1000); // Give the silicon a moment to flush the memory registers

  // --- 3. START mDNS RESPONDER ---
  // This broadcasts the lamp as "sunrise.local" on the network
  if (!MDNS.begin("sunrise")) {
    Serial.println("Error setting up MDNS responder!");
  } else {
    Serial.println("mDNS responder started! You can now reach the lamp at http://sunrise.local");
  }
  
  // --- 4 NVS Variables ---
  preferences.begin("lamp_settings", false);

  alarmHour = preferences.getInt("alarmHour", 7); 
  alarmMinute = preferences.getInt("alarmMinute", 30);
  isSunriseEnabled = preferences.getBool("sunriseEnabled", true);
  isSundownEnabled = preferences.getBool("sundownEnabled", true);
  cfgSunriseDuration = preferences.getInt("sunriseDuration", 30);
  cfgSundownDuration = preferences.getInt("sundownDuration", 30);
  nightLightBright = preferences.getInt("nightLightBright", 128); // Default 50%
  cfgProxEnabled = preferences.getBool("proxEnabled", true);
  cfgMaxGlow = preferences.getInt("cfgMaxGlow", 255);
  cfgTouchThreshold = preferences.getInt("touchThreshold", 30);  // NVS key ≤15 chars
  cfgProxThreshold = preferences.getInt("proxThreshold", 30);    // NVS key ≤15 chars
  cfgAmbientThreshold = preferences.getInt("ambientThresh", 100); // NVS key ≤15 chars
  cfgSundownHour = preferences.getInt("cfgSundownHour", 18);
  cfgSundownMinute = preferences.getInt("sundownMinute", 00);    // NVS key ≤15 chars
  cfgModeTimeout = preferences.getInt("cfgModeTimeout", 40);
  cfgBrightMode = preferences.getInt("cfgBrightMode", 2);
  userTargetBright = preferences.getInt("userBright", 128); // Remember last used brightness (~50% Brightness default)
  userTargetTemp = preferences.getInt("userTemp", 1); // Remember last used temperature (Warm White by default)

  // Safe Float Reads (Bypasses the nvs_get_blob error)
  if (preferences.isKey("geoLat")) {
      geoLat = preferences.getFloat("geoLat", -33.9321);
  }
  if (preferences.isKey("geoLon")) {
      geoLon = preferences.getFloat("geoLon", 18.8602);
  }
  // Safe Array Reads (Bypasses the nvs_get_blob error)
  if (preferences.isKey("sunriseDays")) {
      preferences.getBytes("sunriseDays", (void*)sunriseDays, 7);
  }
  if (preferences.isKey("sundownDays")) {
      preferences.getBytes("sundownDays", (void*)sundownDays, 7);
  }
  tzOffset = preferences.getInt("tzOffset", 2);

  // Close the handle after all reads so that each API write (preferences.putInt, etc.)
  // opens/closes its own atomic transaction, guaranteeing commits survive a hard power-off.
  preferences.end();

  // --- 5. SYNC TIME ---
  configTime(tzOffset * 3600, daylightOffset_sec, ntpServer);
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time from NTP server");
  } else {
    Serial.println("Time successfully synced via NTP!");
  }

  // --- 6. MOUNT INTERNAL FILE SYSTEM ---
  // The 'true' formats the memory if it has never been used before
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  // --- 7. WEB SERVER CONNECT ---
  setupAPIRoutes(server);
  server.begin();

  // --- 8. HARDWARE INIT (Sensors) ---
  if (tsl.begin()) {
    Serial.println("TSL2591 Sensor Found!");
    tsl.setGain(TSL2591_GAIN_MED); 
    tsl.setTiming(TSL2591_INTEGRATIONTIME_100MS); 
  } else {
    Serial.println("ERROR: No TSL2591 sensor found. Check wiring.");
  }

  // --- 9. SPIN UP REMAINING FREE-RTOS CORES ---
  // The LED task is already running on Core 1, so we just spin up the logic/sensor tasks here.
  xTaskCreatePinnedToCore(logicTaskCode, "TaskLogic", 10000, NULL, 1, &TaskLogic, 0);
  xTaskCreatePinnedToCore(sensorTaskCode, "TaskSensor", 10000, NULL, 1, &TaskSensor, 0);
}

void loop() {
  // Listen for Over-The-Air firmware updates
  ArduinoOTA.handle();
  
  // A tiny delay prevents the empty loop from triggering the hardware watchdog
  delay(10);
}