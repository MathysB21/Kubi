#include "TFT_eSPI.h"
#include <cstring>
#include <cstdio>

uint16_t sim_framebuffer[240 * 320] = {0};
uint8_t sim_screen_rotation = 0;

void TFT_eSPI::drawCircleHelper(int32_t x0, int32_t y0, int32_t r, uint8_t cornername, uint16_t color) {
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
        if (cornername & 0x4) {
            drawPixel(x0 + x, y0 + y, color);
            drawPixel(x0 + y, y0 + x, color);
        }
        if (cornername & 0x2) {
            drawPixel(x0 + x, y0 - y, color);
            drawPixel(x0 + y, y0 - x, color);
        }
        if (cornername & 0x8) {
            drawPixel(x0 - y, y0 + x, color);
            drawPixel(x0 - x, y0 + y, color);
        }
        if (cornername & 0x1) {
            drawPixel(x0 - y, y0 - x, color);
            drawPixel(x0 - x, y0 - y, color);
        }
    }
}

void TFT_eSPI::fillCircleHelper(int32_t x0, int32_t y0, int32_t r, uint8_t cornername, int32_t delta, uint16_t color) {
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

        if (cornername & 0x1) {
            drawFastVLine(x0 + x, y0 - y, 2 * y + 1 + delta, color);
            drawFastVLine(x0 + y, y0 - x, 2 * x + 1 + delta, color);
        }
        if (cornername & 0x2) {
            drawFastVLine(x0 - x, y0 - y, 2 * y + 1 + delta, color);
            drawFastVLine(x0 - y, y0 - x, 2 * x + 1 + delta, color);
        }
    }
}

// 7-segment digit drawing
void TFT_eSPI::draw7SegDigit(int32_t x, int32_t y, char digit, int32_t w, int32_t h, int32_t thick, uint16_t color) {
    if (digit == ':') {
        int dotSize = thick + 1;
        int cx = x + (w - dotSize) / 2;
        fillRect(cx, y + h / 3 - dotSize / 2, dotSize, dotSize, color);
        fillRect(cx, y + (2 * h) / 3 - dotSize / 2, dotSize, dotSize, color);
        return;
    }

    // Segments: 0=A, 1=B, 2=C, 3=D, 4=E, 5=F, 6=G
    // A: top, B: top-right, C: bot-right, D: bot, E: bot-left, F: top-left, G: middle
    static const uint8_t segPatterns[10] = {
        0b00111111, // 0: A B C D E F
        0b00000110, // 1: B C
        0b01011011, // 2: A B D E G
        0b01001111, // 3: A B C D G
        0b01100110, // 4: B C F G
        0b01101101, // 5: A C D F G
        0b01111101, // 6: A C D E F G
        0b00000111, // 7: A B C
        0b01111111, // 8: A B C D E F G
        0b01101111  // 9: A B C D F G
    };

    if (digit < '0' || digit > '9') return;
    uint8_t p = segPatterns[digit - '0'];

    int halfH = h / 2;

    // A: Top
    if (p & 0x01) fillRect(x + thick, y, w - 2 * thick, thick, color);
    // B: Top-right
    if (p & 0x02) fillRect(x + w - thick, y + thick, thick, halfH - thick, color);
    // C: Bot-right
    if (p & 0x04) fillRect(x + w - thick, y + halfH, thick, halfH - thick, color);
    // D: Bot
    if (p & 0x08) fillRect(x + thick, y + h - thick, w - 2 * thick, thick, color);
    // E: Bot-left
    if (p & 0x10) fillRect(x, y + halfH, thick, halfH - thick, color);
    // F: Top-left
    if (p & 0x20) fillRect(x, y + thick, thick, halfH - thick, color);
    // G: Middle
    if (p & 0x40) fillRect(x + thick, y + halfH - thick / 2, w - 2 * thick, thick, color);
}

// 5x7 Standard ASCII font bitmap
extern const unsigned char font5x7[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, // Space (32)
    0x00, 0x00, 0x5F, 0x00, 0x00, // !
    0x00, 0x07, 0x00, 0x07, 0x00, // "
    0x14, 0x7F, 0x14, 0x7F, 0x14, // #
    0x24, 0x2A, 0x7F, 0x2A, 0x12, // $
    0x23, 0x13, 0x08, 0x64, 0x62, // %
    0x36, 0x49, 0x55, 0x22, 0x50, // &
    0x00, 0x05, 0x03, 0x00, 0x00, // '
    0x00, 0x1C, 0x22, 0x41, 0x00, // (
    0x00, 0x41, 0x22, 0x1C, 0x00, // )
    0x08, 0x2A, 0x1C, 0x2A, 0x08, // *
    0x08, 0x08, 0x3E, 0x08, 0x08, // +
    0x00, 0x50, 0x30, 0x00, 0x00, // ,
    0x08, 0x08, 0x08, 0x08, 0x08, // -
    0x00, 0x60, 0x60, 0x00, 0x00, // .
    0x20, 0x10, 0x08, 0x04, 0x02, // /
    0x3E, 0x51, 0x49, 0x45, 0x3E, // 0
    0x00, 0x42, 0x7F, 0x40, 0x00, // 1
    0x42, 0x61, 0x51, 0x49, 0x46, // 2
    0x21, 0x41, 0x45, 0x4B, 0x31, // 3
    0x18, 0x14, 0x12, 0x7F, 0x10, // 4
    0x27, 0x45, 0x45, 0x45, 0x39, // 5
    0x3C, 0x4A, 0x49, 0x49, 0x30, // 6
    0x01, 0x71, 0x09, 0x05, 0x03, // 7
    0x36, 0x49, 0x49, 0x49, 0x36, // 8
    0x06, 0x49, 0x49, 0x29, 0x1E, // 9
    0x00, 0x36, 0x36, 0x00, 0x00, // :
    0x00, 0x56, 0x36, 0x00, 0x00, // ;
    0x00, 0x08, 0x14, 0x22, 0x41, // <
    0x14, 0x14, 0x14, 0x14, 0x14, // =
    0x41, 0x22, 0x14, 0x08, 0x00, // >
    0x02, 0x01, 0x51, 0x09, 0x06, // ?
    0x32, 0x49, 0x79, 0x41, 0x3E, // @
    0x7E, 0x11, 0x11, 0x11, 0x7E, // A
    0x7F, 0x49, 0x49, 0x49, 0x36, // B
    0x3E, 0x41, 0x41, 0x41, 0x22, // C
    0x7F, 0x41, 0x41, 0x22, 0x1C, // D
    0x7F, 0x49, 0x49, 0x49, 0x41, // E
    0x7F, 0x09, 0x09, 0x01, 0x01, // F
    0x3E, 0x41, 0x41, 0x51, 0x32, // G
    0x7F, 0x08, 0x08, 0x08, 0x7F, // H
    0x00, 0x41, 0x7F, 0x41, 0x00, // I
    0x20, 0x40, 0x41, 0x3F, 0x01, // J
    0x7F, 0x08, 0x14, 0x22, 0x41, // K
    0x7F, 0x40, 0x40, 0x40, 0x40, // L
    0x7F, 0x02, 0x04, 0x02, 0x7F, // M
    0x7F, 0x04, 0x08, 0x10, 0x7F, // N
    0x3E, 0x41, 0x41, 0x41, 0x3E, // O
    0x7F, 0x09, 0x09, 0x09, 0x06, // P
    0x3E, 0x41, 0x51, 0x21, 0x5E, // Q
    0x7F, 0x09, 0x19, 0x29, 0x46, // R
    0x46, 0x49, 0x49, 0x49, 0x31, // S
    0x01, 0x01, 0x7F, 0x01, 0x01, // T
    0x3F, 0x40, 0x40, 0x40, 0x3F, // U
    0x1F, 0x20, 0x40, 0x20, 0x1F, // V
    0x7F, 0x20, 0x18, 0x20, 0x7F, // W
    0x63, 0x14, 0x08, 0x14, 0x63, // X
    0x03, 0x04, 0x78, 0x04, 0x03, // Y
    0x61, 0x51, 0x49, 0x45, 0x43, // Z
    0x00, 0x7F, 0x41, 0x41, 0x00, // [
    0x02, 0x04, 0x08, 0x10, 0x20, // backslash
    0x00, 0x41, 0x41, 0x7F, 0x00, // ]
    0x04, 0x02, 0x01, 0x02, 0x04, // ^
    0x40, 0x40, 0x40, 0x40, 0x40, // _
    0x00, 0x01, 0x02, 0x04, 0x00, // `
    0x20, 0x54, 0x54, 0x54, 0x78, // a
    0x7F, 0x48, 0x44, 0x44, 0x38, // b
    0x38, 0x44, 0x44, 0x44, 0x20, // c
    0x38, 0x44, 0x44, 0x48, 0x7F, // d
    0x38, 0x54, 0x54, 0x54, 0x18, // e
    0x08, 0x7E, 0x09, 0x01, 0x02, // f
    0x08, 0x14, 0x54, 0x54, 0x3C, // g
    0x7F, 0x08, 0x04, 0x04, 0x78, // h
    0x00, 0x44, 0x7D, 0x40, 0x00, // i
    0x20, 0x40, 0x44, 0x3D, 0x00, // j
    0x00, 0x7F, 0x10, 0x28, 0x44, // k
    0x00, 0x41, 0x7F, 0x40, 0x00, // l
    0x7C, 0x04, 0x18, 0x04, 0x78, // m
    0x7C, 0x08, 0x04, 0x04, 0x78, // n
    0x38, 0x44, 0x44, 0x44, 0x38, // o
    0x7C, 0x14, 0x14, 0x14, 0x08, // p
    0x08, 0x14, 0x14, 0x18, 0x7C, // q
    0x7C, 0x08, 0x04, 0x04, 0x08, // r
    0x48, 0x54, 0x54, 0x54, 0x20, // s
    0x04, 0x3F, 0x44, 0x40, 0x20, // t
    0x3C, 0x40, 0x40, 0x20, 0x7C, // u
    0x1C, 0x20, 0x40, 0x20, 0x1C, // v
    0x3C, 0x40, 0x30, 0x40, 0x3C, // w
    0x44, 0x28, 0x10, 0x28, 0x44, // x
    0x0C, 0x50, 0x50, 0x50, 0x3C, // y
    0x44, 0x64, 0x54, 0x4C, 0x44, // z
    0x00, 0x08, 0x36, 0x41, 0x00, // {
    0x00, 0x00, 0x7F, 0x00, 0x00, // |
    0x00, 0x41, 0x36, 0x08, 0x00, // }
    0x08, 0x08, 0x2A, 0x1C, 0x08  // ~
};

void TFT_eSPI::drawChar(int32_t x, int32_t y, char c, uint16_t fg, uint16_t bg, uint8_t size) {
    if (c < 32 || c > 126) c = ' ';
    int idx = (c - 32) * 5;

    for (int col = 0; col < 5; col++) {
        uint8_t line = font5x7[idx + col];
        for (int row = 0; row < 7; row++) {
            if (line & (1 << row)) {
                if (size == 1) drawPixel(x + col, y + row, fg);
                else fillRect(x + col * size, y + row * size, size, size, fg);
            }
        }
    }
}

int16_t TFT_eSPI::drawString(const String& string, int32_t poX, int32_t poY, uint8_t font) {
    const char* str = string.c_str();
    int len = (int)strlen(str);
    if (len == 0) return 0;

    int totalW = 0;
    int totalH = 0;

    if (font == 7) {
        // Large 7-segment font (digits & colon)
        int digitW = 28;
        int digitH = 48;
        int thick = 5;
        int colonW = 14;
        int spacing = 6;

        for (int i = 0; i < len; i++) {
            if (str[i] == ':') totalW += colonW + spacing;
            else totalW += digitW + spacing;
        }
        totalH = digitH;

        int sx = poX;
        int sy = poY;

        switch (_textDatum) {
            case MC_DATUM: sx -= totalW / 2; sy -= totalH / 2; break;
            case TC_DATUM: sx -= totalW / 2; break;
            case BC_DATUM: sx -= totalW / 2; sy -= totalH; break;
            case TR_DATUM: sx -= totalW; break;
            case MR_DATUM: sx -= totalW; sy -= totalH / 2; break;
            case BR_DATUM: sx -= totalW; sy -= totalH; break;
            case ML_DATUM: sy -= totalH / 2; break;
            case BL_DATUM: sy -= totalH; break;
            default: break;
        }

        int curX = sx;
        for (int i = 0; i < len; i++) {
            if (str[i] == ':') {
                draw7SegDigit(curX, sy, ':', colonW, digitH, thick, _textColor);
                curX += colonW + spacing;
            } else {
                draw7SegDigit(curX, sy, str[i], digitW, digitH, thick, _textColor);
                curX += digitW + spacing;
            }
        }
        return totalW;
    }

    // Text fonts 1, 2, 4, 6
    uint8_t scale = 1;
    int charW = 6;
    int charH = 8;

    if (font == 1) { scale = 1; charW = 6; charH = 8; }
    else if (font == 2) { scale = 2; charW = 12; charH = 16; }
    else if (font == 4) { scale = 3; charW = 18; charH = 24; }
    else if (font == 6) { scale = 5; charW = 30; charH = 40; }
    else { scale = 2; charW = 12; charH = 16; }

    totalW = len * charW;
    totalH = charH;

    int sx = poX;
    int sy = poY;

    switch (_textDatum) {
        case MC_DATUM: sx -= totalW / 2; sy -= totalH / 2; break;
        case TC_DATUM: sx -= totalW / 2; break;
        case BC_DATUM: sx -= totalW / 2; sy -= totalH; break;
        case TR_DATUM: sx -= totalW; break;
        case MR_DATUM: sx -= totalW; sy -= totalH / 2; break;
        case BR_DATUM: sx -= totalW; sy -= totalH; break;
        case ML_DATUM: sy -= totalH / 2; break;
        case BL_DATUM: sy -= totalH; break;
        default: break;
    }

    for (int i = 0; i < len; i++) {
        drawChar(sx + i * charW, sy, str[i], _textColor, _textBgColor, scale);
    }

    return totalW;
}
