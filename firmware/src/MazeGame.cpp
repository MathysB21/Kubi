#include "MazeGame.h"
#include <string.h>

MazeGame maze;

// Pixel-art palette: warm wood walls like the cube's chassis, dark floor
static const uint16_t COL_FLOOR       = 0x0861;  // near-black blue
static const uint16_t COL_WALL        = 0x8AC5;  // wood   #8B5A2B
static const uint16_t COL_WALL_LIGHT  = 0xC408;  // bevel  #C08040
static const uint16_t COL_WALL_SHADOW = 0x59C3;  // shadow #5A3A1A
static const uint16_t COL_HOLE        = 0x0000;
static const uint16_t COL_HOLE_RIM    = 0x2945;
static const uint16_t COL_GOAL_A      = 0xFEA0;  // gold
static const uint16_t COL_GOAL_B      = 0x8400;
static const uint16_t COL_BALL        = 0xE71C;  // soft white
static const uint16_t COL_BALL_SHADE  = 0x9CD3;
static const uint16_t COL_HUD         = 0x7BEF;

static const int   HUD_H = 16;
static const float BALL_RADIUS = 0.32f;  // cell units

MazeGame::MazeGame()
    : _board(0), _startX(1.5f), _startY(1.5f), _bx(1.5f), _by(1.5f),
      _cell(16), _ox(0), _oy(HUD_H), _drawnBallX(-1), _drawnBallY(-1) {
    memset(_cells, '#', sizeof(_cells));
}

void MazeGame::begin() {
    loadBoard(_board);
}

void MazeGame::loadBoard(int index) {
    // Skip invalid boards rather than trap the ball in one
    for (int tries = 0; tries < MAZE_BOARD_COUNT; tries++) {
        int i = ((index + tries) % MAZE_BOARD_COUNT + MAZE_BOARD_COUNT) % MAZE_BOARD_COUNT;
        if (mazeBoardValid(MAZE_BOARDS[i])) {
            _board = i;
            break;
        }
    }
    const MazeBoard& b = MAZE_BOARDS[_board];
    for (int r = 0; r < MAZE_ROWS; r++) {
        memcpy(_cells[r], b.rows[r], MAZE_COLS);
        for (int c = 0; c < MAZE_COLS; c++) {
            if (_cells[r][c] == 'S') {
                _startX = c + 0.5f;
                _startY = r + 0.5f;
            }
        }
    }
    _bx = _startX;
    _by = _startY;
    _drawnBallX = _drawnBallY = -1;
}

char MazeGame::cellAt(int c, int r) const {
    if (c < 0 || r < 0 || c >= MAZE_COLS || r >= MAZE_ROWS) return '#';
    return _cells[r][c];
}

void MazeGame::layout(TFT_eSPI& tft) {
    int w = tft.width();
    int h = tft.height();
    int cw = w / MAZE_COLS;
    int ch = (h - HUD_H) / MAZE_ROWS;
    _cell = cw < ch ? cw : ch;
    _ox = (w - _cell * MAZE_COLS) / 2;
    _oy = HUD_H + (h - HUD_H - _cell * MAZE_ROWS) / 2;
}

void MazeGame::drawCell(TFT_eSPI& tft, int c, int r) {
    int x = _ox + c * _cell;
    int y = _oy + r * _cell;
    int s = _cell;
    switch (cellAt(c, r)) {
        case '#':
            tft.fillRect(x, y, s, s, COL_WALL);
            tft.drawFastHLine(x, y, s, COL_WALL_LIGHT);
            tft.drawFastVLine(x, y, s, COL_WALL_LIGHT);
            tft.drawFastHLine(x, y + s - 1, s, COL_WALL_SHADOW);
            tft.drawFastVLine(x + s - 1, y, s, COL_WALL_SHADOW);
            break;
        case 'O':
            tft.fillRect(x, y, s, s, COL_FLOOR);
            tft.fillCircle(x + s / 2, y + s / 2, s * 2 / 5, COL_HOLE_RIM);
            tft.fillCircle(x + s / 2, y + s / 2, s * 2 / 5 - 1, COL_HOLE);
            break;
        case 'G': {
            // 4x4 gold checker flag
            int q = s / 4;
            for (int j = 0; j < 4; j++)
                for (int i = 0; i < 4; i++)
                    tft.fillRect(x + i * q, y + j * q, q, q, ((i + j) & 1) ? COL_GOAL_B : COL_GOAL_A);
            break;
        }
        default:
            tft.fillRect(x, y, s, s, COL_FLOOR);
            break;
    }
}

void MazeGame::drawBall(TFT_eSPI& tft, int px, int py) {
    int r = (int)(BALL_RADIUS * _cell + 0.5f);
    tft.fillCircle(px, py, r, COL_BALL_SHADE);
    tft.fillCircle(px - 1, py - 1, r - 1, COL_BALL);
    tft.drawPixel(px - r / 2, py - r / 2, TFT_WHITE);
}

// Repaint the board cells under the ball's previous bounding box
void MazeGame::restoreUnder(TFT_eSPI& tft, int px, int py) {
    int r = (int)(BALL_RADIUS * _cell + 0.5f) + 1;
    int c0 = (px - r - _ox) / _cell, c1 = (px + r - _ox) / _cell;
    int r0 = (py - r - _oy) / _cell, r1 = (py + r - _oy) / _cell;
    for (int rr = r0; rr <= r1; rr++)
        for (int cc = c0; cc <= c1; cc++)
            drawCell(tft, cc, rr);
}

void MazeGame::drawHud(TFT_eSPI& tft, bool full, uint32_t nowMs) {
    (void)nowMs;
    if (!full) return;
    char buf[40];
    snprintf(buf, sizeof(buf), "%d/%d  %s", _board + 1, MAZE_BOARD_COUNT, MAZE_BOARDS[_board].name);
    tft.setTextDatum(ML_DATUM);
    tft.setTextColor(COL_HUD, TFT_BLACK);
    tft.drawString(buf, _ox + 2, HUD_H / 2, 1);
}

void MazeGame::draw(TFT_eSPI& tft, bool full, uint32_t nowMs) {
    if (full) {
        layout(tft);
        for (int r = 0; r < MAZE_ROWS; r++)
            for (int c = 0; c < MAZE_COLS; c++)
                drawCell(tft, c, r);
        _drawnBallX = _drawnBallY = -1;
    }

    // Only the ball moves: restore what it covered, then draw it again
    int px = _ox + (int)(_bx * _cell + 0.5f);
    int py = _oy + (int)(_by * _cell + 0.5f);
    if (px != _drawnBallX || py != _drawnBallY) {
        if (_drawnBallX >= 0) restoreUnder(tft, _drawnBallX, _drawnBallY);
        drawBall(tft, px, py);
        _drawnBallX = px;
        _drawnBallY = py;
    }

    drawHud(tft, full, nowMs);
}
