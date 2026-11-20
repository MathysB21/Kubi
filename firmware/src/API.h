#pragma once
#include <ESPAsyncWebServer.h>
#include <string.h>
#include <stdio.h>
#include "AudioManager.h"

// Kubi Orientation & Operating Modes
enum KubiMode {
  MODE_CLOCK_IDLE       = 0, // Face 1 Up: digital or analog clock (shake toggles)
  MODE_POMODORO        = 1, // Face 2 Up: Focus timer with chimes
  MODE_MASCOT_ROUTINE  = 2, // Face 3 Up: Kubi jelly animations & room temperature
  MODE_AMBIENT         = 3, // Face 4 Up: slow ambient glow
  MODE_MAZE            = 4  // Lying on its back, screen to the sky: tilt maze
};

// The maze is drawn the way the last upright face drew (mazeViewFace), so it
// reads right for whoever tipped the cube back. The hardware loop keeps it.
extern int mazeViewFace;
inline bool mazeFaceActive(KubiMode mode) {
  return mode == MODE_MAZE;
}
// The face whose screen rotation the display uses for this mode
inline int rotationFaceFor(KubiMode mode) {
  return mode == MODE_MAZE ? mazeViewFace : (int)mode;
}

// Faces that may sleep after the inactivity timeout (decided 21 Sep):
// Clock and Pomodoro never sleep, Ambient is exempt; Mascot and Maze sleep.
inline bool modeMaySleep(KubiMode mode) {
  return mode == MODE_MASCOT_ROUTINE || mazeFaceActive(mode);
}

// Render tick: the maze needs >= 25 fps (decided 21 Sep); only its ball
// repaints, so 30 fps is cheap. Everything else is fine at 20 fps.
#define RENDER_TICK_MS      50
#define RENDER_TICK_MAZE_MS 33

// Override banner text is written by the web server task and drawn by the
// hardware loop on the other core. Never share a String across tasks: always
// go through these, which copy a bounded char buffer under a lock.
#define OVERRIDE_TEXT_MAX 128
void setOverrideText(const char* text);
String getOverrideText();

// POST /api/override {"title"?, "message"}: the banner text is "title\nmessage"
// (DisplayManager::drawOverrideAlert splits it; no title = the alert title).
// An empty message takes the banner down.
inline void composeBanner(char* out, size_t n, const char* title, const char* message) {
  if (title && *title) snprintf(out, n, "%s\n%s", title, message ? message : "");
  else snprintf(out, n, "%s", message ? message : "");
}

// POST /api/chime {"chime": name}: the Creator panel plays any chime.
// CHIME_NONE for an unknown name.
inline KubiChime chimeByName(const char* name) {
  if (!name) return CHIME_NONE;
  if (!strcmp(name, "tap"))     return CHIME_TAP_FEEDBACK;
  if (!strcmp(name, "done"))    return CHIME_POMODORO_DONE;
  if (!strcmp(name, "fanfare")) return CHIME_POMODORO_LONG_BREAK;
  if (!strcmp(name, "ping"))    return CHIME_WAKE_PING;
  if (!strcmp(name, "ouch"))    return CHIME_SLAM_OUCH;
  return CHIME_NONE;
}

// Scene previews: 10 minutes from the dashboard picker, longer from the
// Creator panel ("mascotPreviewMin", 1..MAX). The screen stays awake while one
// is showing.
#define MASCOT_PREVIEW_MAX_MIN 720

// Nap wake log (main.cpp checkNap, NVS "napLog"), newest first
enum NapWakeReason : uint8_t {
    NAP_WAKE_NONE = 0,          // empty slot
    NAP_WAKE_MOVED = 1,         // the accelerometer's activity latch (> 0.5 g change)
    NAP_WAKE_SENSOR_SILENT = 2, // the accelerometer did not answer: woke to be safe
    NAP_WAKE_OTHER = 3          // not a timer wake (reset button, power)
};
struct NapWakeRecord {
    uint32_t at;      // epoch seconds of the wake (0 = clock unknown)
    uint32_t start;   // epoch seconds the nap began (0 = unknown)
    uint8_t  reason;  // NapWakeReason
    uint8_t  axes;    // bit 2 X, bit 1 Y, bit 0 Z: which axes set off the activity
    uint8_t  pad[2];
};
#define NAP_LOG_MAX 5

// First connect after setup: how long the "open kubi.local" screen stays up
// unless tapped away.
#define ADDRESS_SCREEN_MS (5UL * 60UL * 1000UL)

// Factory reset: body must be {"confirm":"ERASE"}. The handler only raises a
// flag; the hardware loop shows the reset screen, wipes and restarts.
#define FACTORY_RESET_CONFIRM "ERASE"

// Declare the function that will attach all our routes
void setupAPIRoutes(AsyncWebServer& server);