#pragma once

// Maze boards: hand-drawn ASCII, MAZE_COLS x MAZE_ROWS.
//   '#' wall   ' ' floor   'S' ball start (exactly one)
//   'G' goal (exactly one)   'O' hole (falls back to S)
// Corridors must be at least one cell wide; the ball is smaller than a cell.

#define MAZE_COLS 20
#define MAZE_ROWS 14

struct MazeBoard {
    const char* name;
    const char* rows[MAZE_ROWS];
};

extern const MazeBoard MAZE_BOARDS[];
extern const int MAZE_BOARD_COUNT;

// True if the board has the right shape, exactly one S and one G, and a
// closed outer wall. Invalid boards are skipped rather than trapping the ball.
bool mazeBoardValid(const MazeBoard& b);
