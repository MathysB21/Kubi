#pragma once
#include <ESPAsyncWebServer.h>

// Kubi Orientation & Operating Modes
enum KubiMode {
  MODE_CLOCK_IDLE       = 0, // Face 1 Up: Minimal clock + next event / ticker on tap
  MODE_POMODORO        = 1, // Face 2 Up: Focus timer with chimes
  MODE_MASCOT_ROUTINE  = 2, // Face 3 Up: Kubi jelly animations & room temperature
  MODE_SCHEDULE_AGENDA = 3  // Face 4 Up: 3-day Google Calendar agenda
};

// Faces that may sleep after the inactivity timeout (decided 21 Sep):
// Clock and Pomodoro never sleep, and Face 4 (Ambient) is exempt.
inline bool modeMaySleep(KubiMode mode) {
  return mode == MODE_MASCOT_ROUTINE;
}

// Declare the function that will attach all our routes
void setupAPIRoutes(AsyncWebServer& server);