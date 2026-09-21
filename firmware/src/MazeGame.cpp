#include "MazeGame.h"
#include "FaceMap.h"
#include <string.h>
#include <math.h>

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

// --- Physics tuning (cell units; one cell is 16 px in landscape) ---
static const float STEP_S       = 0.005f; // fixed 200 Hz step
static const float MAX_FRAME_S  = 0.1f;   // drop time beyond this (stalls, face switch)
static const float GAIN         = 3.2f;   // cells/s^2 per m/s^2 of tilt (10 deg ~ 5.4 c/s^2)
static const float DEADZONE     = 0.35f;  // m/s^2 (~2 deg): sensor noise must not drift the ball
static const float DAMPING      = 1.1f;   // 1/s rolling drag
static const float MAX_SPEED    = 9.0f;   // cells/s; 0.045 cells per step << radius, no tunnelling
static const float RESTITUTION  = 0.35f;  // wall bounce
static const float FILTER_ALPHA = 0.35f;  // accelerometer low-pass per sample
static const uint32_t CALIBRATE_MS = 250;
static const int MAZE_FACE = 3;          // Face 4 slot (MODE_AMBIENT); frame from FaceMap.h

MazeGame::MazeGame()
    : _board(0), _startX(1.5f), _startY(1.5f), _bx(1.5f), _by(1.5f),
      _vx(0), _vy(0), _fx(0), _fy(0), _fz(0), _restX(0), _restY(0), _restZ(0),
      _haveFilter(false), _calibrating(true), _calStart(0),
      _calSumX(0), _calSumY(0), _calSumZ(0), _calN(0), _lastUpdateMs(0), _stepAccum(0),
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
    _vx = _vy = 0;
    _drawnBallX = _drawnBallY = -1;
    _calibrating = true;   // re-zero on the next update()
    _calStart = 0;
}

void MazeGame::startCalibration(uint32_t nowMs) {
    _calibrating = true;
    _calStart = nowMs;
    _calSumX = _calSumY = _calSumZ = 0;
    _calN = 0;
}

void MazeGame::update(float ax, float ay, float az, uint32_t nowMs) {
    if (!_haveFilter) {
        _fx = ax; _fy = ay; _fz = az;
        _haveFilter = true;
    } else {
        _fx += FILTER_ALPHA * (ax - _fx);
        _fy += FILTER_ALPHA * (ay - _fy);
        _fz += FILTER_ALPHA * (az - _fz);
    }

    if (_calibrating) {
        if (_calStart == 0) startCalibration(nowMs ? nowMs : 1);
        _calSumX += ax; _calSumY += ay; _calSumZ += az; _calN++;
        if (nowMs - _calStart >= CALIBRATE_MS && _calN > 0) {
            _restX = _calSumX / _calN;
            _restY = _calSumY / _calN;
            _restZ = _calSumZ / _calN;
            _calibrating = false;
            _lastUpdateMs = nowMs;
            _stepAccum = 0;
        }
        return;
    }

    // Tilt relative to rest, projected onto the maze face's screen axes
    // (FaceMap.h, shared with the sim's tilt injection).
    KVec3 up, right, out;
    screenFrame(MAZE_FACE, up, right, out);
    KVec3 d = { _fx - _restX, _fy - _restY, _fz - _restZ };
    float tiltX = -kDot(d, right);  // right side down -> ball rolls right
    float tiltY = -kDot(d, out);    // top edge away   -> ball rolls up (screen y is down)

    float mag = sqrtf(tiltX * tiltX + tiltY * tiltY);
    if (mag < DEADZONE) {
        tiltX = tiltY = 0;
    } else {
        float k = (mag - DEADZONE) / mag;  // continuous past the deadzone
        tiltX *= k;
        tiltY *= k;
    }

    float dt = (nowMs - _lastUpdateMs) / 1000.0f;
    _lastUpdateMs = nowMs;
    if (dt > MAX_FRAME_S) dt = MAX_FRAME_S;
    _stepAccum += dt;
    while (_stepAccum >= STEP_S) {
        step(STEP_S, tiltX, tiltY);
        _stepAccum -= STEP_S;
    }
}

void MazeGame::step(float dt, float tiltX, float tiltY) {
    _vx += (GAIN * tiltX - DAMPING * _vx) * dt;
    _vy += (GAIN * tiltY - DAMPING * _vy) * dt;
    float sp = sqrtf(_vx * _vx + _vy * _vy);
    if (sp > MAX_SPEED) {
        _vx *= MAX_SPEED / sp;
        _vy *= MAX_SPEED / sp;
    }
    _bx += _vx * dt;
    _by += _vy * dt;
    collide();
}

// Circle vs. wall cells: push out along the contact normal and reflect the
// normal velocity. Corners resolve naturally, so the ball rolls off edges.
void MazeGame::collide() {
    for (int pass = 0; pass < 2; pass++) {
        int c0 = (int)floorf(_bx - BALL_RADIUS), c1 = (int)floorf(_bx + BALL_RADIUS);
        int r0 = (int)floorf(_by - BALL_RADIUS), r1 = (int)floorf(_by + BALL_RADIUS);
        for (int r = r0; r <= r1; r++) {
            for (int c = c0; c <= c1; c++) {
                if (cellAt(c, r) != '#') continue;
                float nx = _bx < c ? c : (_bx > c + 1 ? c + 1 : _bx);
                float ny = _by < r ? r : (_by > r + 1 ? r + 1 : _by);
                float dx = _bx - nx, dy = _by - ny;
                float dist2 = dx * dx + dy * dy;
                if (dist2 >= BALL_RADIUS * BALL_RADIUS) continue;
                float dist = sqrtf(dist2);
                float ux, uy;
                if (dist > 1e-5f) {
                    ux = dx / dist; uy = dy / dist;
                } else {
                    // Centre inside the wall (should not happen): leave via the nearest face
                    float l = _bx - c, rr = c + 1 - _bx, t = _by - r, b = r + 1 - _by;
                    float m = fminf(fminf(l, rr), fminf(t, b));
                    ux = (m == l) ? -1.f : (m == rr) ? 1.f : 0.f;
                    uy = (m == t) ? -1.f : (m == b) ? 1.f : 0.f;
                    dist = -m;
                }
                float push = BALL_RADIUS - dist;
                _bx += ux * push;
                _by += uy * push;
                float vn = _vx * ux + _vy * uy;
                if (vn < 0) {
                    _vx -= (1.0f + RESTITUTION) * vn * ux;
                    _vy -= (1.0f + RESTITUTION) * vn * uy;
                }
            }
        }
    }
    // Last line of defence: never leave the board
    if (_bx < 1 + BALL_RADIUS) _bx = 1 + BALL_RADIUS;
    if (_by < 1 + BALL_RADIUS) _by = 1 + BALL_RADIUS;
    if (_bx > MAZE_COLS - 1 - BALL_RADIUS) _bx = MAZE_COLS - 1 - BALL_RADIUS;
    if (_by > MAZE_ROWS - 1 - BALL_RADIUS) _by = MAZE_ROWS - 1 - BALL_RADIUS;
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
