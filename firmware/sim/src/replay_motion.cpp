// Replays /api/motion-log recordings through the real SensorManager (sim HAL,
// fake clock) and prints the gestures it reports. Run it after any change to
// gesture detection and compare with the expectations in sim/motion/README.md:
//   powershell -ExecutionPolicy Bypass -File firmware/sim/replay_motion.ps1
#include <cstdio>
#include <cstring>
#include "Arduino.h"
#include "Preferences.h"
#include "SensorManager.h"

SimSerial Serial;
std::map<std::string, std::map<std::string, std::string>> Preferences::_storage;
bool Preferences::_loaded = false;
float sim_accel_x = 0, sim_accel_y = 0, sim_accel_z = 9.8f;
float sim_temperature = 22.0f;

int main(int argc, char** argv) {
    for (int a = 1; a < argc; a++) {
        FILE* f = fopen(argv[a], "r");
        if (!f) { printf("cannot open %s\n", argv[a]); continue; }
        SensorManager sm;
        simFakeMillis() = 100000;
        sm.init();
        char line[128];
        fgets(line, sizeof line, f); // header
        // Settle on the first reading for 2 s, as on a desk before recording
        long t0 = -1; int x, y, z, d, tot, fl; long t;
        int taps = 0, shakes = 0, slams = 0;
        std::string events;
        bool primed = false;
        while (fgets(line, sizeof line, f)) {
            if (sscanf(line, "%ld,%d,%d,%d,%d,%d,%d", &t, &x, &y, &z, &d, &tot, &fl) != 7) continue;
            sim_accel_x = x / 100.0f; sim_accel_y = y / 100.0f; sim_accel_z = z / 100.0f;
            if (!primed) {
                for (int i = 0; i < 100; i++) { simFakeMillis() += 20; sm.loop(); sm.getRecentGesture(); }
                t0 = simFakeMillis() - t;
                primed = true;
            }
            simFakeMillis() = t0 + t;
            sm.loop();
            KubiGesture g = sm.getRecentGesture();
            char buf[32];
            if (g == GESTURE_TAP)   { taps++;   snprintf(buf, sizeof buf, " tap@%.2f", t / 1000.0); events += buf; }
            if (g == GESTURE_SHAKE) { shakes++; snprintf(buf, sizeof buf, " SHAKE@%.2f", t / 1000.0); events += buf; }
            if (g == GESTURE_SLAM)  { slams++;  snprintf(buf, sizeof buf, " SLAM@%.2f", t / 1000.0); events += buf; }
        }
        // Let a pending tap confirm
        for (int i = 0; i < 30; i++) { simFakeMillis() += 20; sm.loop(); if (sm.getRecentGesture() == GESTURE_TAP) { taps++; events += " tap@end"; } }
        fclose(f);
        const char* name = argv[a];
        for (const char* p = argv[a]; *p; p++) if (*p == '/' || *p == '\\') name = p + 1;
        printf("%-20s taps=%2d shakes=%d slams=%d |%s\n", name, taps, shakes, slams, events.c_str());
    }
    return 0;
}
