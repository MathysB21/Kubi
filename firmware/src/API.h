#pragma once
#include <ESPAsyncWebServer.h>

// Shared FSM States
enum LampState { 
  STATE_AUTO_DAY,     // 0
  STATE_AUTO_NIGHT,   // 1
  STATE_MANUAL_DAY,   // 2
  STATE_MANUAL_NIGHT, // 3
  STATE_NIGHT_LIGHT,  // 4
  STATE_SUNRISE,      // 5
  STATE_SUNDOWN,      // 6
  STATE_PROXIMITY,    // 7
  STATE_AWAY,         // 8
  STATE_WIFI_SETUP
};

// Declare the function that will attach all our routes
void setupAPIRoutes(AsyncWebServer& server);