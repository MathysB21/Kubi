#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>
#include "MazeBoards.h"

// Face 4 (optional, replaces Ambient when face4Maze is set): tilt-maze game.
// Positions are in cell units (1.0 = one board cell), so the physics does not
// care how big the screen or the cells are.
// While the maze is being played the face is locked (see FACE_SETTLE_LOCKED).
// Without clear tilt input for this long the lock lets go, so a cube left on
// the maze face can never be stuck there.
#define MAZE_IDLE_UNLOCK_MS 45000

enum MazeState : uint8_t {
    MAZE_READY = 0,   // ball on S, timer waits for the first deliberate tilt
    MAZE_RUNNING,     // timer running
    MAZE_FALLING,     // dropping into a hole; respawns on S, timer keeps running
    MAZE_WON          // at the goal: time + best shown, then the next board
};

class MazeGame {
public:
    MazeGame();

    // Entering the maze face: (re)load the current board with the ball on S.
    void begin();
    void loadBoard(int index);

    // Feed the raw accelerometer (m/s^2) every loop iteration while the maze
    // face is up. Steering is the tilt RELATIVE to the resting vector captured
    // when the board starts, so the cube never has to sit perfectly level.
    void update(float ax, float ay, float az, uint32_t nowMs);

    // full = the screen was just cleared. Otherwise only the ball (and HUD
    // text that changed) is repainted.
    void draw(TFT_eSPI& tft, bool full, uint32_t nowMs);

    // True while someone is actively playing: clear tilt within MAZE_IDLE_UNLOCK_MS.
    bool isPlaying(uint32_t nowMs) const {
        return _lastInputMs != 0 && nowMs - _lastInputMs < MAZE_IDLE_UNLOCK_MS;
    }

    MazeState state() const { return _state; }
    uint32_t elapsedMs(uint32_t nowMs) const;
    uint32_t bestMs() const { return _best; }       // 0 = no best yet
    void placeBall(float x, float y) { _bx = x; _by = y; }
    int  boardIndex() const { return _board; }
    float ballX() const { return _bx; }
    float ballY() const { return _by; }

private:
    int  _board;
    char _cells[MAZE_ROWS][MAZE_COLS];
    float _startX, _startY;
    float _bx, _by;                // ball centre, cell units
    float _vx, _vy;                // cells/s

    // Input: low-passed accelerometer and the calibrated resting vector
    float _fx, _fy, _fz;
    float _restX, _restY, _restZ;
    bool  _haveFilter;
    bool  _calibrating;
    uint32_t _calStart;
    float _calSumX, _calSumY, _calSumZ;
    int   _calN;
    uint32_t _lastUpdateMs;
    float _stepAccum;
    uint32_t _lastInputMs;         // 0 = no input since the board started

    // Game state
    MazeState _state;
    uint32_t _stateSince;
    uint32_t _runStart;
    uint32_t _finishMs;
    uint32_t _best;
    bool     _newBest;
    float    _ballScale;           // 1 = normal, shrinks while falling

    void startCalibration(uint32_t nowMs);
    void step(float dt, float tiltX, float tiltY);
    void collide();
    void checkCells(uint32_t nowMs);
    void setState(MazeState s, uint32_t nowMs) { _state = s; _stateSince = nowMs; }

    // Renderer state (valid while the scene is unchanged)
    int _cell, _ox, _oy;           // cell size and board origin in px
    int _drawnBallX, _drawnBallY;  // px, -1 = not drawn
    int _drawnBallR;
    int _drawnTenths;
    MazeState _drawnState;
    bool _needFull;                // board changed: repaint everything

    char cellAt(int c, int r) const;
    void layout(TFT_eSPI& tft);
    void drawCell(TFT_eSPI& tft, int c, int r);
    void drawBall(TFT_eSPI& tft, int px, int py, int r);
    void restoreUnder(TFT_eSPI& tft, int px, int py, int r);
    void drawHud(TFT_eSPI& tft, bool full, uint32_t nowMs);
    void drawWinBanner(TFT_eSPI& tft);
};

extern MazeGame maze;
