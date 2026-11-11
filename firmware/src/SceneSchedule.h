#pragma once
// Kubi picks his own diorama on Face 3 from the local date and time. Taps do
// not change it; the dashboard can show any scene for a while (a preview),
// after which Kubi goes back to his own pick. Shared with the simulator.
//
// Some scenes only turn up on their day: Birthday on Wilhelm's birthday
// (NVS "bday", MMDD, set from the dashboard) and Christmas from 1 to 26 Dec.
#include <string.h>
#include "KubiScenes.h"

inline int kubiSceneIndexById(const char* id) {
    for (size_t i = 0; i < ALL_KUBI_SCENE_COUNT; i++) {
        if (ALL_KUBI_SCENES[i] && strcmp(ALL_KUBI_SCENES[i]->id, id) == 0) return (int)i;
    }
    return -1;
}

// Rough Cape Town sunset, minutes after midnight, by month (0 = January)
inline int kubiSunsetMinute(int month) {
    static const int SUNSET[12] = { 1198, 1180, 1150, 1115, 1080, 1065, 1075, 1095, 1115, 1135, 1160, 1185 };
    return SUNSET[((month % 12) + 12) % 12];
}

// month 0-11, wday 0 = Sunday, birthdayMMDD 0 = not set
inline const char* kubiScheduledSceneId(int month, int mday, int wday, int hour, int minute,
                                        float roomTemp, int birthdayMMDD) {
    int m = hour * 60 + minute;
    bool weekend = wday == 0 || wday == 6;
    if (birthdayMMDD > 0 && (month + 1) * 100 + mday == birthdayMMDD) return "birthday";
    if (m >= 22 * 60 || m < (weekend ? 8 * 60 : 6 * 60)) return "sleeping_in_bed";
    if (month == 11 && mday <= 26) return "christmas";
    int sunset = kubiSunsetMinute(month);
    if (m >= sunset + 15) return "music";
    if (m >= sunset - 75) return "watching_sunset";
    if (!weekend) {
        if (m < 8 * 60) return "working_out_at_gym";
        if (m >= 12 * 60 && m < 13 * 60) return "sitting_by_cafe";
        return "working_from_home";
    }
    if (m < 11 * 60) return "sitting_by_cafe";
    bool summer = month >= 10 || month <= 2; // Nov-Mar, southern hemisphere
    if (m < 16 * 60 && (summer || roomTemp >= 27.0f)) return "beach_eating_ice_cream";
    return "laying_in_flower_field";
}

inline int kubiScheduledScene(int month, int mday, int wday, int hour, int minute,
                              float roomTemp, int birthdayMMDD) {
    int i = kubiSceneIndexById(kubiScheduledSceneId(month, mday, wday, hour, minute, roomTemp, birthdayMMDD));
    return i >= 0 ? i : 0;
}

// How long a scene picked in the dashboard stays up before Kubi takes over again
static const unsigned long KUBI_SCENE_PREVIEW_MS = 10UL * 60UL * 1000UL;
