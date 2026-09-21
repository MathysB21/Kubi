#pragma once
#include <cstdint>
#include <chrono>

// Desktop stand-in for ESP8266Audio's AudioOutputI2S.
//
// Deliberately strict so the sim catches the bug classes the real driver has:
//  - ConsumeSample() returns false until begin() has been called (as i2sOn does).
//  - Samples drain at the configured rate from a queue the size of the real
//    DMA ring (8 buffers x 128 frames). ConsumeSample() returns false while the
//    queue is full, so an unpaced writer is throttled like on hardware.
//  - Underruns (the queue ran dry while a writer was mid-stream) are counted,
//    so a throughput-starved synth shows up in /sim/state as audioUnderruns.
class AudioOutputI2S {
public:
    AudioOutputI2S(int port = 0, int output_mode = 0, int dma_buf_count = 8, int use_apll = 0)
        : _capacity(dma_buf_count * 128) {}
    virtual ~AudioOutputI2S() {}

    bool SetPinout(int bclk, int lrclk, int din) { return true; }
    bool SetRate(int hz) { _rate = hz; return true; }
    bool SetBitsPerSample(int bits) { return true; }
    bool SetChannels(int ch) { return true; }
    bool SetGain(float gain) { return true; }

    bool begin() {
        _on = true;
        _queued = 0.0;
        _t0 = Clock::now();
        _lastWrite = _t0;
        return true;
    }

    bool stop() {
        if (!_on) return false;
        _on = false;
        return true;
    }

    bool ConsumeSample(int16_t sample[2]) {
        if (!_on) return false;

        auto now = Clock::now();
        double played = std::chrono::duration<double>(now - _t0).count() * _rate;
        if (_queued < played) {
            // Queue ran dry. If the previous write was recent the writer is
            // mid-stream and failed to keep up: that is an audible gap.
            double sinceLastMs = std::chrono::duration<double, std::milli>(now - _lastWrite).count();
            if (_samplesWritten > 0 && sinceLastMs < 20.0) _underruns++;
            _queued = played;
        }
        if (_queued - played >= _capacity) return false;

        _queued += 1.0;
        _samplesWritten++;
        _lastWrite = now;
        return true;
    }

    // --- Sim-only telemetry ---
    bool isStarted() const { return _on; }
    uint32_t getUnderruns() const { return _underruns; }
    uint64_t getSamplesWritten() const { return _samplesWritten; }

private:
    using Clock = std::chrono::steady_clock;
    bool _on = false;
    int _rate = 44100;
    double _capacity;
    double _queued = 0.0;
    Clock::time_point _t0;
    Clock::time_point _lastWrite;
    uint32_t _underruns = 0;
    uint64_t _samplesWritten = 0;
};
