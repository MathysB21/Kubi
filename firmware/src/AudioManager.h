#pragma once
#include <Arduino.h>
#include <AudioOutputI2S.h>
#include <atomic>

enum KubiChime {
    CHIME_NONE = 0,
    CHIME_TAP_FEEDBACK,
    CHIME_POMODORO_DONE,
    CHIME_POMODORO_LONG_BREAK,
    CHIME_WAKE_PING,
    CHIME_SLAM_OUCH
};

struct Note;

// Square-wave chime synthesiser.
//
// Threading: playChime()/stop() may be called from any task (the hardware loop,
// or the AsyncTCP task via PomodoroManager). They only post a request. All
// synthesis happens in pump(), which must be driven by ONE dedicated audio
// task/thread (main.cpp audioTask, sim_main.cpp audio thread).
class AudioManager {
public:
    AudioManager();

    // Creates the I2S output and calls begin(). Returns false if the output
    // could not be started; chimes are then silently dropped.
    bool init();
    bool isReady() const { return _ready; }

    // Renders one block and pushes it to I2S, waiting (1 ms yields) while the
    // DMA queue is full, so the caller is paced by the real sample clock.
    // Returns false when idle; the caller should then sleep a few ms.
    bool pump();

    void playChime(KubiChime chime);
    void stop();
    bool isPlaying() const { return getActiveChime() != CHIME_NONE; }
    KubiChime getActiveChime() const;
    void setVolume(float gain); // 0.0 to 1.0
    const AudioOutputI2S* getOutput() const { return _i2sOut; }

private:
    AudioOutputI2S* _i2sOut;
    bool _ready;
    float _gain;

    // Cross-task request mailbox
    std::atomic<KubiChime> _requestedChime;
    std::atomic<uint32_t>  _requestSeq;
    uint32_t _handledSeq;
    std::atomic<KubiChime> _activeChime;

    // Synth state, touched only by pump()
    const Note* _sequence;
    int _noteIdx;
    uint32_t _noteSamplesLeft;
    uint32_t _phase;
    uint32_t _phaseInc;

    void startSequence(KubiChime chime);
    void loadNote();
    bool writeFrame(int16_t value);
};

extern AudioManager audio;
