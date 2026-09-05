#pragma once
#include <Arduino.h>
#include <AudioOutputI2S.h>

enum KubiChime {
    CHIME_NONE = 0,
    CHIME_TAP_FEEDBACK,
    CHIME_POMODORO_DONE,
    CHIME_WAKE_PING,
    CHIME_SLAM_OUCH
};

class AudioManager {
public:
    AudioManager();

    void init();
    void loop(); // Call from Core 1

    void playChime(KubiChime chime);
    void stop();
    bool isPlaying() const { return _activeChime != CHIME_NONE; }
    void setVolume(float gain); // 0.0 to 1.0

private:
    AudioOutputI2S* _i2sOut;
    KubiChime _activeChime;
    uint32_t _noteStartTime;
    int _currentNoteIdx;
    float _currentFreq;
    uint32_t _phase;
    float _gain;

    void updateSynthesizer();
};

extern AudioManager audio;
