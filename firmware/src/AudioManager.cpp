#include "AudioManager.h"
#include <math.h>

#define I2S_BCLK_PIN 26
#define I2S_LRC_PIN  25
#define I2S_DIN_PIN  27
#define SAMPLE_RATE  22050

AudioManager audio;

struct Note {
    float freq;
    uint16_t durationMs;
};

// Chime Note Sequences
static const Note NOTES_TAP[] = {
    { 880.0f, 35 },
    { 0.0f, 0 }
};

static const Note NOTES_WAKE[] = {
    { 587.33f, 60 },
    { 880.0f, 100 },
    { 0.0f, 0 }
};

static const Note NOTES_POMO_DONE[] = {
    { 523.25f, 100 }, // C5
    { 659.25f, 100 }, // E5
    { 783.99f, 100 }, // G5
    { 1046.50f, 250 }, // C6
    { 0.0f, 0 }
};

static const Note NOTES_SLAM_OUCH[] = {
    { 349.23f, 80 },  // F4
    { 261.63f, 80 },  // C4
    { 174.61f, 160 }, // F3
    { 0.0f, 0 }
};

AudioManager::AudioManager()
    : _i2sOut(nullptr),
      _activeChime(CHIME_NONE),
      _noteStartTime(0),
      _currentNoteIdx(0),
      _currentFreq(0.0f),
      _phase(0),
      _gain(0.25f) {}

void AudioManager::init() {
    _i2sOut = new AudioOutputI2S();
    _i2sOut->SetPinout(I2S_BCLK_PIN, I2S_LRC_PIN, I2S_DIN_PIN);
    _i2sOut->SetRate(SAMPLE_RATE);
    _i2sOut->SetBitsPerSample(16);
    _i2sOut->SetChannels(2);
    _i2sOut->SetGain(_gain);
    Serial.println("[AUDIO] MAX98357A I2S Amplifier initialized (BCLK:26, LRC:25, DIN:27)");
}

void AudioManager::setVolume(float gain) {
    if (gain < 0.0f) gain = 0.0f;
    if (gain > 1.0f) gain = 1.0f;
    _gain = gain;
    if (_i2sOut) {
        _i2sOut->SetGain(_gain);
    }
}

void AudioManager::playChime(KubiChime chime) {
    _activeChime = chime;
    _currentNoteIdx = 0;
    _noteStartTime = millis();
    _phase = 0;

    const Note* sequence = nullptr;
    if (chime == CHIME_TAP_FEEDBACK)  sequence = NOTES_TAP;
    else if (chime == CHIME_WAKE_PING) sequence = NOTES_WAKE;
    else if (chime == CHIME_POMODORO_DONE) sequence = NOTES_POMO_DONE;
    else if (chime == CHIME_SLAM_OUCH) sequence = NOTES_SLAM_OUCH;

    if (sequence && sequence[0].durationMs > 0) {
        _currentFreq = sequence[0].freq;
    } else {
        stop();
    }
}

void AudioManager::stop() {
    _activeChime = CHIME_NONE;
    _currentFreq = 0.0f;
    _currentNoteIdx = 0;
    _phase = 0;
}

void AudioManager::loop() {
    if (_activeChime == CHIME_NONE || !_i2sOut) return;

    updateSynthesizer();
}

void AudioManager::updateSynthesizer() {
    const Note* sequence = nullptr;
    if (_activeChime == CHIME_TAP_FEEDBACK) sequence = NOTES_TAP;
    else if (_activeChime == CHIME_WAKE_PING) sequence = NOTES_WAKE;
    else if (_activeChime == CHIME_POMODORO_DONE) sequence = NOTES_POMO_DONE;
    else if (_activeChime == CHIME_SLAM_OUCH) sequence = NOTES_SLAM_OUCH;

    if (!sequence) {
        stop();
        return;
    }

    uint32_t elapsed = millis() - _noteStartTime;

    // Check if current note finished
    if (elapsed >= sequence[_currentNoteIdx].durationMs) {
        _currentNoteIdx++;
        if (sequence[_currentNoteIdx].durationMs == 0) {
            // Sequence complete
            stop();
            return;
        }
        _currentFreq = sequence[_currentNoteIdx].freq;
        _noteStartTime = millis();
    }

    // Synthesize square wave chunk into I2S output buffer
    if (_currentFreq > 20.0f) {
        uint32_t phaseInc = (uint32_t)((_currentFreq / (float)SAMPLE_RATE) * 4294967296.0f);
        int16_t sample[2];

        for (int i = 0; i < 64; i++) {
            int16_t val = (_phase < 0x80000000) ? 9000 : -9000;
            _phase += phaseInc;
            sample[0] = val;
            sample[1] = val;
            _i2sOut->ConsumeSample(sample);
        }
    }
}
