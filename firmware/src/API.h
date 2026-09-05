#pragma once
#include <ESPAsyncWebServer.h>

// Kubi Orientation & Operating Modes
enum KubiMode {
  MODE_CLOCK_IDLE       = 0, // Face 1 Up: Minimal clock + next event / ticker on tap
  MODE_POMODORO        = 1, // Face 2 Up: Focus timer with chimes
  MODE_MASCOT_ROUTINE  = 2, // Face 3 Up: Kubi jelly animations & room temperature
  MODE_SCHEDULE_AGENDA = 3, // Face 4 Up: 3-day Google Calendar agenda
  MODE_NIGHT_SLEEP     = 4, // Night mode / screen off (tap wakes 30s)
  MODE_WIFI_SETUP      = 5  // Captive portal configuration
};

// Declare the function that will attach all our routes
void setupAPIRoutes(AsyncWebServer& server);