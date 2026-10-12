#pragma once
#include <ESPAsyncWebServer.h>

// Kubi Orientation & Operating Modes
enum KubiMode {
  MODE_CLOCK_IDLE       = 0, // Face 1 Up: digital or analog clock (shake toggles)
  MODE_POMODORO        = 1, // Face 2 Up: Focus timer with chimes
  MODE_MASCOT_ROUTINE  = 2, // Face 3 Up: Kubi jelly animations & room temperature
  MODE_AMBIENT         = 3  // Face 4 Up: slow ambient glow (the maze may replace it)
};

// Face 4 shows Ambient by default; the maze replaces it when face4Maze is set
// (kubi_settings/face4Maze, /api/settings). Go/no-go for shipping it: 2 Nov.
extern bool face4Maze;
inline bool mazeFaceActive(KubiMode mode) {
  return mode == MODE_AMBIENT && face4Maze;
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

// First connect after setup: how long the "open kubi.local" screen stays up
// unless tapped away.
#define ADDRESS_SCREEN_MS (5UL * 60UL * 1000UL)

// Factory reset: body must be {"confirm":"ERASE"}. The handler only raises a
// flag; the hardware loop shows the reset screen, wipes and restarts.
#define FACTORY_RESET_CONFIRM "ERASE"

// Declare the function that will attach all our routes
void setupAPIRoutes(AsyncWebServer& server);