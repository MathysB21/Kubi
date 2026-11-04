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

// Internal DAC only. Silence there is mid-scale (1.65 V) while sound plays,
// but the DMA idles at 0 V; a 1.65 V step through the amp's ~24 dB is a loud
// pop at every chime. So the level fades up before the first note and back
// down after the last, slowly enough to sit below what the amp's input
// coupling and the speakers pass. Tune on the real speaker if a thump remains.
#define DAC_RAMP_IN_MS   60
#define DAC_RAMP_OUT_MS  200
// The DAC path has the amp's ~24 dB gain after it and no gain stage of the
// library's (that is kept at 1.0 so the fades can reach 0 V), so the tone is
// scaled here: at the default volume this is about +/-4 DAC steps, ~0.2 W
// into 4 ohm with the PAM8403's knob fully up. Its knob sets the rest.
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
      _dacMode(true),
      _dcLevel(0.0f) {}
#else
      _dacMode(false),
      _dcLevel(1.0f) {}
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

// Square-wave sample of the current note, at the current volume
int16_t AudioManager::toneSample() {
    if (!_phaseInc) return 0;
    int32_t amp = _dacMode ? (int32_t)(SQUARE_AMPLITUDE * _gain * DAC_VOLUME_SCALE) : SQUARE_AMPLITUDE;
    int16_t val = (_phase < 0x80000000u) ? amp : -amp;
    _phase += _phaseInc;
    return val;
}

// DAC idle level as a sample: _dcLevel 0 is the DMA's idle 0 V (-32768 after
// the library's +0x8000), 1 is mid-scale. Raised-cosine shape, so the fade
// starts and ends without a corner.
int16_t AudioManager::dcOffset() const {
    if (!_dacMode) return 0;
    float shape = 0.5f - 0.5f * cosf(3.14159265f * _dcLevel);
    return (int16_t)(-32768.0f * (1.0f - shape));
}

bool AudioManager::pump() {
    uint32_t seq = _requestSeq.load();
    if (seq != _handledSeq) {
        KubiChime requested = _requestedChime.load();
        startSequence(requested);
        _handledSeq = seq;
    }

    // Idle: nothing to play and (DAC) already faded down to the DMA's 0 V
    if (!_sequence && (!_dacMode || _dcLevel <= 0.0f)) return false;

    if (!_ready) {
        // Output is dead: drop the chime so isPlaying() does not stick.
        _sequence = nullptr;
        _activeChime = CHIME_NONE;
        return false;
    }

    const float rampIn  = 1000.0f / (DAC_RAMP_IN_MS * (float)SAMPLE_RATE);
    const float rampOut = 1000.0f / (DAC_RAMP_OUT_MS * (float)SAMPLE_RATE);

    for (int i = 0; i < PUMP_BLOCK_FRAMES; i++) {
        int32_t val;
        if (_sequence && _dcLevel < 1.0f) {
            // DAC: fade up to mid-scale before the first note
            _dcLevel += rampIn;
            if (_dcLevel > 1.0f) _dcLevel = 1.0f;
            val = dcOffset();
        } else if (_sequence) {
            if (_noteSamplesLeft == 0) {
                _noteIdx++;
                if (_sequence[_noteIdx].durationMs == 0) {
                    // Sequence complete. I2S: tx_desc_auto_clear gives
                    // silence after the tail drains. DAC: fade down first.
                    _sequence = nullptr;
                    _activeChime = CHIME_NONE;
                    if (!_dacMode) return true;
                    continue;
                }
                loadNote();
            }
            val = dcOffset() + toneSample();
            _noteSamplesLeft--;
        } else {
            // DAC: fade back down to 0 V, where the idle DMA leaves it
            _dcLevel -= rampOut;
            if (_dcLevel < 0.0f) _dcLevel = 0.0f;
            val = dcOffset();
        }

        if (val > 32767) val = 32767;
        if (val < -32768) val = -32768;
        if (!writeFrame((int16_t)val)) {
            Serial.println("[AUDIO] ERROR: I2S queue stalled, chimes disabled");
            _ready = false;
            return true;
        }
        if (!_sequence && _dcLevel <= 0.0f) return true;
    }
    return true;
}
