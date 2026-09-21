#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>
#include "MazeBoards.h"

// Face 4 (optional, replaces Ambient when face4Maze is set): tilt-maze game.
// Positions are in cell units (1.0 = one board cell), so the physics does not
// care how big the screen or the cells are.
class MazeGame {
public:
    MazeGame();

    // Entering the maze face: (re)load the current board with the ball on S.
    void begin();
    void loadBoard(int index);

    // full = the screen was just cleared. Otherwise only the ball (and HUD
    // text that changed) is repainted.
    void draw(TFT_eSPI& tft, bool full, uint32_t nowMs);

    void placeBall(float x, float y) { _bx = x; _by = y; }
    int  boardIndex() const { return _board; }
    float ballX() const { return _bx; }
    float ballY() const { return _by; }

private:
    int  _board;
    char _cells[MAZE_ROWS][MAZE_COLS];
    float _startX, _startY;
    float _bx, _by;                // ball centre, cell units

    // Renderer state (valid while the scene is unchanged)
    int _cell, _ox, _oy;           // cell size and board origin in px
    int _drawnBallX, _drawnBallY;  // px, -1 = not drawn

    char cellAt(int c, int r) const;
    void layout(TFT_eSPI& tft);
    void drawCell(TFT_eSPI& tft, int c, int r);
    void drawBall(TFT_eSPI& tft, int px, int py);
    void restoreUnder(TFT_eSPI& tft, int px, int py);
    void drawHud(TFT_eSPI& tft, bool full, uint32_t nowMs);
};

extern MazeGame maze;
