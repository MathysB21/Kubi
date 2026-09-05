#pragma once
#include "Arduino.h"
#include <cstdint>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

// Standard 16-bit RGB565 Colors
#define TFT_BLACK       0x0000
#define TFT_NAVY        0x000F
#define TFT_DARKGREEN   0x03E0
#define TFT_DARKCYAN    0x03EF
#define TFT_MAROON      0x7800
#define TFT_PURPLE      0x780F
#define TFT_OLIVE       0x7BE0
#define TFT_LIGHTGREY   0xD69A
#define TFT_DARKGREY    0x7BEF
#define TFT_BLUE        0x001F
#define TFT_GREEN       0x07E0
#define TFT_CYAN        0x07FF
#define TFT_RED         0xF800
#define TFT_MAGENTA     0xF81F
#define TFT_YELLOW      0xFFE0
#define TFT_WHITE       0xFFFF
#define TFT_ORANGE      0xFDA0
#define TFT_GREENYELLOW 0xB7E0
#define TFT_PINK        0xFE19
#define TFT_BROWN       0x9A60
#define TFT_GOLD        0xFEA0
#define TFT_SILVER      0xC618
#define TFT_SKYBLUE     0x867D
#define TFT_VIOLET      0x915C

// Text Datums
#define TL_DATUM 0 // Top left
#define TC_DATUM 1 // Top centre
#define TR_DATUM 2 // Top right
#define ML_DATUM 3 // Middle left
#define MC_DATUM 4 // Middle centre
#define MR_DATUM 5 // Middle right
#define BL_DATUM 6 // Bottom left
#define BC_DATUM 7 // Bottom centre
#define BR_DATUM 8 // Bottom right

extern uint16_t sim_framebuffer[240 * 320];
extern uint8_t sim_screen_rotation;

class TFT_eSPI {
public:
    uint8_t _rotation;
    uint8_t _textDatum;
    uint16_t _textColor;
    uint16_t _textBgColor;
    bool _invert;

    TFT_eSPI()
        : _rotation(0),
          _textDatum(TL_DATUM),
          _textColor(TFT_WHITE),
          _textBgColor(TFT_BLACK),
          _invert(false) {}

    void init() {
        _rotation = 0;
        sim_screen_rotation = 0;
        fillScreen(TFT_BLACK);
    }

    void invertDisplay(bool i) {
        _invert = i;
    }

    void setRotation(uint8_t r) {
        _rotation = r % 4;
        sim_screen_rotation = _rotation;
    }

    int32_t width() const {
        return (_rotation == 0 || _rotation == 2) ? 240 : 320;
    }

    int32_t height() const {
        return (_rotation == 0 || _rotation == 2) ? 320 : 240;
    }

    void setTextDatum(uint8_t datum) {
        _textDatum = datum;
    }

    void setTextColor(uint16_t fg, uint16_t bg = TFT_BLACK) {
        _textColor = fg;
        _textBgColor = bg;
    }

    // Direct pixel write into 240x320 panel buffer
    inline void drawPixel(int32_t x, int32_t y, uint16_t color) {
        if (x < 0 || x >= width() || y < 0 || y >= height()) return;

        int32_t px = x;
        int32_t py = y;

        if (_rotation == 0) {
            px = x;
            py = y;
        } else if (_rotation == 1) {
            px = 239 - y;
            py = x;
        } else if (_rotation == 2) {
            px = 239 - x;
            py = 319 - y;
        } else if (_rotation == 3) {
            px = y;
            py = 319 - x;
        }

        if (px >= 0 && px < 240 && py >= 0 && py < 320) {
            sim_framebuffer[py * 240 + px] = color;
        }
    }

    void fillScreen(uint16_t color) {
        for (int i = 0; i < 240 * 320; i++) {
            sim_framebuffer[i] = color;
        }
    }

    void drawFastHLine(int32_t x, int32_t y, int32_t w, uint16_t color) {
        if (w < 0) { x += w; w = -w; }
        for (int32_t i = 0; i < w; i++) {
            drawPixel(x + i, y, color);
        }
    }

    void drawFastVLine(int32_t x, int32_t y, int32_t h, uint16_t color) {
        if (h < 0) { y += h; h = -h; }
        for (int32_t i = 0; i < h; i++) {
            drawPixel(x, y + i, color);
        }
    }

    void drawLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t color) {
        if (x0 == x1) {
            if (y0 > y1) std::swap(y0, y1);
            drawFastVLine(x0, y0, y1 - y0 + 1, color);
            return;
        }
        if (y0 == y1) {
            if (x0 > x1) std::swap(x0, x1);
            drawFastHLine(x0, y0, x1 - x0 + 1, color);
            return;
        }
        int32_t dx = std::abs(x1 - x0);
        int32_t dy = -std::abs(y1 - y0);
        int32_t sx = x0 < x1 ? 1 : -1;
        int32_t sy = y0 < y1 ? 1 : -1;
        int32_t err = dx + dy;
        while (true) {
            drawPixel(x0, y0, color);
            if (x0 == x1 && y0 == y1) break;
            int32_t e2 = 2 * err;
            if (e2 >= dy) { err += dy; x0 += sx; }
            if (e2 <= dx) { err += dx; y0 += sy; }
        }
    }

    void fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color) {
        for (int32_t j = 0; j < h; j++) {
            drawFastHLine(x, y + j, w, color);
        }
    }

    void drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color) {
        drawFastHLine(x, y, w, color);
        drawFastHLine(x, y + h - 1, w, color);
        drawFastVLine(x, y, h, color);
        drawFastVLine(x + w - 1, y, h, color);
    }

    void drawCircle(int32_t x0, int32_t y0, int32_t r, uint16_t color) {
        int32_t f = 1 - r;
        int32_t ddF_x = 1;
        int32_t ddF_y = -2 * r;
        int32_t x = 0;
        int32_t y = r;

        drawPixel(x0, y0 + r, color);
        drawPixel(x0, y0 - r, color);
        drawPixel(x0 + r, y0, color);
        drawPixel(x0 - r, y0, color);

        while (x < y) {
            if (f >= 0) {
                y--;
                ddF_y += 2;
                f += ddF_y;
            }
            x++;
            ddF_x += 2;
            f += ddF_x;

            drawPixel(x0 + x, y0 + y, color);
            drawPixel(x0 - x, y0 + y, color);
            drawPixel(x0 + x, y0 - y, color);
            drawPixel(x0 - x, y0 - y, color);
            drawPixel(x0 + y, y0 + x, color);
            drawPixel(x0 - y, y0 + x, color);
            drawPixel(x0 + y, y0 - x, color);
            drawPixel(x0 - y, y0 - x, color);
        }
    }

    void fillCircle(int32_t x0, int32_t y0, int32_t r, uint16_t color) {
        drawFastVLine(x0, y0 - r, 2 * r + 1, color);
        int32_t f = 1 - r;
        int32_t ddF_x = 1;
        int32_t ddF_y = -2 * r;
        int32_t x = 0;
        int32_t y = r;

        while (x < y) {
            if (f >= 0) {
                y--;
                ddF_y += 2;
                f += ddF_y;
            }
            x++;
            ddF_x += 2;
            f += ddF_x;

            drawFastVLine(x0 + x, y0 - y, 2 * y + 1, color);
            drawFastVLine(x0 - x, y0 - y, 2 * y + 1, color);
            drawFastVLine(x0 + y, y0 - x, 2 * x + 1, color);
            drawFastVLine(x0 - y, y0 - x, 2 * x + 1, color);
        }
    }

    void fillRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint16_t color) {
        if (r <= 0) { fillRect(x, y, w, h, color); return; }
        fillRect(x + r, y, w - 2 * r, h, color);
        fillCircleHelper(x + w - r - 1, y + r, r, 1, h - 2 * r - 1, color);
        fillCircleHelper(x + r, y + r, r, 2, h - 2 * r - 1, color);
    }

    void drawRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint16_t color) {
        if (r <= 0) { drawRect(x, y, w, h, color); return; }
        drawFastHLine(x + r, y, w - 2 * r, color);
        drawFastHLine(x + r, y + h - 1, w - 2 * r, color);
        drawFastVLine(x, y + r, h - 2 * r, color);
        drawFastVLine(x + w - 1, y + r, h - 2 * r, color);
        drawCircleHelper(x + r, y + r, r, 1, color);
        drawCircleHelper(x + w - r - 1, y + r, r, 2, color);
        drawCircleHelper(x + w - r - 1, y + h - r - 1, r, 4, color);
        drawCircleHelper(x + r, y + h - r - 1, r, 8, color);
    }

    int16_t drawString(const String& string, int32_t poX, int32_t poY, uint8_t font);

private:
    void drawCircleHelper(int32_t x0, int32_t y0, int32_t r, uint8_t cornername, uint16_t color);
    void fillCircleHelper(int32_t x0, int32_t y0, int32_t r, uint8_t cornername, int32_t delta, uint16_t color);
    void draw7SegDigit(int32_t x, int32_t y, char digit, int32_t w, int32_t h, int32_t thick, uint16_t color);
    void drawChar(int32_t x, int32_t y, char c, uint16_t fg, uint16_t bg, uint8_t size);
};
