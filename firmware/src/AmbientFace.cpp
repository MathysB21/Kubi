#include "AmbientFace.h"
#include <math.h>

AmbientFace ambient;

struct Rgb { float r, g, b; };

// Deep, warm-leaning night colours; each is the orb's brightest level
static const Rgb PALETTE[] = {
    {  40,  60, 200 },  // midnight blue
    {  20, 150, 150 },  // deep teal
    { 110,  50, 190 },  // violet
    { 210,  90,  40 },  // ember
    { 190,  40,  90 },  // rose
};
static const int PALETTE_N = sizeof(PALETTE) / sizeof(PALETTE[0]);

// 4x4 ordered dither: gives the glow its soft, pixel-art fringe
static const uint8_t BAYER4[4][4] = {
    {  0,  8,  2, 10 },
    { 12,  4, 14,  6 },
    {  3, 11,  1,  9 },
    { 15,  7, 13,  5 },
};
static const int LEVELS = 6;
static const uint32_t FRAME_MS  = 100;   // 10 Hz is plenty for something this slow
static const uint32_t BREATH_MS = 9000;  // one slow breath
static const float TWO_PI_F = 6.2831853f;

static uint16_t rgb565(float r, float g, float b) {
    return ((uint16_t)((uint8_t)r & 0xF8) << 8) | ((uint16_t)((uint8_t)g & 0xFC) << 3) | ((uint8_t)b >> 3);
}

void AmbientFace::draw(TFT_eSPI& tft, bool full, uint32_t now) {
    if (!full && now - _lastDrawMs < FRAME_MS) return;
    _lastDrawMs = now;

    int cols = tft.width() / CELL;
    int rows = tft.height() / CELL;
    if (full) {
        for (int i = 0; i < cols * rows; i++) _cells[i] = TFT_BLACK; // screen was just cleared
    }

    // Colour: cosine-eased blend from one palette stop to the next
    uint32_t t = now + _phaseOffsetMs;
    int stop = (t / STOP_MS) % PALETTE_N;
    float f = (float)(t % STOP_MS) / (float)STOP_MS;
    f = 0.5f - 0.5f * cosf(f * TWO_PI_F * 0.5f);
    const Rgb& a = PALETTE[stop];
    const Rgb& b = PALETTE[(stop + 1) % PALETTE_N];
    Rgb c = { a.r + (b.r - a.r) * f, a.g + (b.g - a.g) * f, a.b + (b.b - a.b) * f };

    // Breathing radius, in cells
    float breath = 0.5f - 0.5f * cosf((float)(now % BREATH_MS) / (float)BREATH_MS * TWO_PI_F);
    float radius = (float)(cols < rows ? cols : rows) * 0.52f * (0.82f + 0.18f * breath);
    float cx = cols * 0.5f;
    float cy = rows * 0.5f;

    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            float dx = x + 0.5f - cx;
            float dy = y + 0.5f - cy;
            float d = sqrtf(dx * dx + dy * dy) / radius;
            float intensity = d < 1.0f ? (1.0f - d) * (1.0f - d) : 0.0f;

            int level = (int)(intensity * LEVELS + BAYER4[y & 3][x & 3] / 16.0f);
            if (level > LEVELS) level = LEVELS;
            float k = (float)level / LEVELS;
            uint16_t colour = level ? rgb565(c.r * k, c.g * k, c.b * k) : TFT_BLACK;

            uint16_t& prev = _cells[y * cols + x];
            if (colour != prev) {
                tft.fillRect(x * CELL, y * CELL, CELL, CELL, colour);
                prev = colour;
            }
        }
    }
}
