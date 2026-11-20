#include "AudioManager.h"
#include <math.h>

// Speaker output, chosen at build time (platformio.ini):
//  - KUBI_AUDIO_INTERNAL_DAC: the ESP32's own 8-bit DAC on GPIO 25 (left) and
//    GPIO 26 (right), into an analog amp (PAM8403) and the speakers.
//  - otherwise: an I2S class-D amp (MAX98357A) on BCLK 26 / LRC 25 / DIN 27.
#define I2S_BCLK_PIN 26
#define I2S_LRC_PIN  25
#define I2S_DIN_PIN  27
#define SAMPLE_RATE  22050

// Internal DAC only: the DAC rests at 0 V and a chime is a pulse wave from
// 0 V up, never a square wave around a raised level. History, measured on the
// real cube (2026-11-11), because both obvious alternatives fail:
//  - Fading to mid-scale around each chime: the fade went through the amp's
//    ~24 dB as a speaker kick, and with WiFi start-up it browned out the ESP32.
//  - Holding a raised level between chimes: whenever a flash write (OTA, NVS,
//    LittleFS) stalls the audio task, the DMA underruns and tx_desc_auto_clear
//    drops the DAC to 0 V; at a high knob setting that kick tripped the
//    charger's battery protection. A raised level also carries the 3.3 V
//    rail's noise (the DAC divides that rail), heard as static with a beat.
// At 0 V an underrun changes nothing, the rail noise is ~0, and the only step
// a chime makes is the tone's own edge. Peak-to-peak is the same as before.
// The tone is scaled here (the library gain stays 1.0): at the default volume
// a pulse is about 9 DAC steps high, ~35 at full volume, ~0.2 W into 4 ohm
// with the PAM8403's knob fully up. Its knob sets the rest.
#define DAC_VOLUME_SCALE 0.5f

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

static const Note NOTES_POMO_LONG_BREAK[] = {
    { 523.25f, 90 },   // C5
    { 659.25f, 90 },   // E5
    { 783.99f, 90 },   // G5
    { 1046.50f, 130 }, // C6
    { 1174.66f, 100 }, // D6
    { 1318.51f, 120 }, // E6
    { 1567.98f, 350 }, // G6
    { 0.0f, 0 }
};

static const Note NOTES_SLAM_OUCH[] = {
    { 349.23f, 80 },  // F4
    { 261.63f, 80 },  // C4
    { 174.61f, 160 }, // F3
    { 0.0f, 0 }
};

// Frames rendered per pump() call. The DMA queue holds 8 x 128 frames (~46 ms).
#define PUMP_BLOCK_FRAMES 128
#define SQUARE_AMPLITUDE  9000
// If the DMA queue refuses samples for this long, I2S is not running.
#define I2S_STALL_TIMEOUT_MS 200

static const Note* sequenceFor(KubiChime chime) {
    switch (chime) {
        case CHIME_TAP_FEEDBACK:        return NOTES_TAP;
        case CHIME_WAKE_PING:           return NOTES_WAKE;
        case CHIME_POMODORO_DONE:       return NOTES_POMO_DONE;
        case CHIME_POMODORO_LONG_BREAK: return NOTES_POMO_LONG_BREAK;
        case CHIME_SLAM_OUCH:           return NOTES_SLAM_OUCH;
        default:                        return nullptr;
    }
}

AudioManager::AudioManager()
    : _i2sOut(nullptr),
      _ready(false),
      _gain(0.25f),
      _requestedChime(CHIME_NONE),
      _requestSeq(0),
      _handledSeq(0),
      _activeChime(CHIME_NONE),
      _sequence(nullptr),
      _noteIdx(0),
      _noteSamplesLeft(0),
      _phase(0),
      _phaseInc(0),
#ifdef KUBI_AUDIO_INTERNAL_DAC
      _dacMode(true) {}
#else
      _dacMode(false) {}
#endif

bool AudioManager::init() {
    if (_dacMode) {
        _i2sOut = new AudioOutputI2S(0, AudioOutputI2S::INTERNAL_DAC);
    } else {
        _i2sOut = new AudioOutputI2S();
        _i2sOut->SetPinout(I2S_BCLK_PIN, I2S_LRC_PIN, I2S_DIN_PIN);
    }
    _i2sOut->SetRate(SAMPLE_RATE);
    _i2sOut->SetBitsPerSample(16);
    _i2sOut->SetChannels(2);
    _i2sOut->SetGain(_dacMode ? 1.0f : _gain);
    _ready = _i2sOut->begin();
    if (!_ready) {
        Serial.println("[AUDIO] ERROR: I2S begin() failed, chimes disabled");
    } else if (_dacMode) {
        Serial.println("[AUDIO] Internal DAC output started (GPIO 25 left, GPIO 26 right) -> analog amp");
    } else {
        Serial.println("[AUDIO] MAX98357A I2S output started (BCLK:26, LRC:25, DIN:27)");
    }
    return _ready;
}

void AudioManager::setVolume(float gain) {
    if (gain < 0.0f) gain = 0.0f;
    if (gain > 1.0f) gain = 1.0f;
    _gain = gain;
    // The DAC path scales in renderSample(); its library gain stays at 1.0
    if (_i2sOut && !_dacMode) {
        _i2sOut->SetGain(_gain);
    }
}

void AudioManager::playChime(KubiChime chime) {
    _requestedChime = chime;
    _requestSeq++;
}

void AudioManager::stop() {
    playChime(CHIME_NONE);
}

KubiChime AudioManager::getActiveChime() const {
    // A request not yet picked up by the audio task already counts as playing,
    // so callers polling right after playChime() see it.
    if (_requestSeq.load() != _handledSeq) return _requestedChime.load();
    return _activeChime.load();
}

void AudioManager::startSequence(KubiChime chime) {
    _sequence = sequenceFor(chime);
    _noteIdx = 0;
    _phase = 0;
    if (_sequence && _sequence[0].durationMs > 0) {
        _activeChime = chime;
        loadNote();
    } else {
        _sequence = nullptr;
        _activeChime = CHIME_NONE;
    }
}

void AudioManager::loadNote() {
    const Note& n = _sequence[_noteIdx];
    _noteSamplesLeft = (uint32_t)n.durationMs * SAMPLE_RATE / 1000;
    _phaseInc = (n.freq > 20.0f) ? (uint32_t)((n.freq / (float)SAMPLE_RATE) * 4294967296.0f) : 0;
}

bool AudioManager::writeFrame(int16_t value) {
    int16_t sample[2] = { value, value };
    uint32_t waitStart = millis();
    while (!_i2sOut->ConsumeSample(sample)) {
        if (millis() - waitStart > I2S_STALL_TIMEOUT_MS) return false;
        delay(1); // DMA queue full: yield until a buffer drains
    }
    return true;
}

// The DAC's 0 V as a sample: the library adds 0x8000 and the DAC takes the
// high byte, so -32768 is code 0, the same level the idle DMA writes.
#define DAC_ZERO_SAMPLE (-32768)

// Sample of the current note at the current volume. I2S: a square wave
// around 0. DAC: a pulse wave between 0 V and twice the amplitude, so rests,
// gaps and the end of a chime all sit at the idle 0 V.
int16_t AudioManager::toneSample() {
    if (_dacMode) {
        int32_t amp = (int32_t)(SQUARE_AMPLITUDE * _gain * DAC_VOLUME_SCALE);
        bool high = _phaseInc && (_phase < 0x80000000u);
        _phase += _phaseInc;
        return (int16_t)(DAC_ZERO_SAMPLE + (high ? 2 * amp : 0));
    }
    if (!_phaseInc) return 0;
    int16_t val = (_phase < 0x80000000u) ? SQUARE_AMPLITUDE : -SQUARE_AMPLITUDE;
    _phase += _phaseInc;
    return val;
}

bool AudioManager::pump() {
    uint32_t seq = _requestSeq.load();
    if (seq != _handledSeq) {
        KubiChime requested = _requestedChime.load();
        startSequence(requested);
        _handledSeq = seq;
    }

    // Idle: nothing to play. The DMA falls silent on its own once its tail
    // drains (tx_desc_auto_clear): 0 for I2S, 0 V for the DAC.
    if (!_sequence) return false;

    if (!_ready) {
        // Output is dead: drop the chime so isPlaying() does not stick.
        _sequence = nullptr;
        _activeChime = CHIME_NONE;
        return false;
    }

    for (int i = 0; i < PUMP_BLOCK_FRAMES; i++) {
        if (_noteSamplesLeft == 0) {
            _noteIdx++;
            if (_sequence[_noteIdx].durationMs == 0) {
                // Sequence complete; the DMA's auto-clear does the rest
                _sequence = nullptr;
                _activeChime = CHIME_NONE;
                return true;
            }
            loadNote();
        }
        int16_t val = toneSample();
        _noteSamplesLeft--;

        if (!writeFrame(val)) {
            Serial.println("[AUDIO] ERROR: I2S queue stalled, chimes disabled");
            _ready = false;
            return true;
        }
    }
    return true;
}
