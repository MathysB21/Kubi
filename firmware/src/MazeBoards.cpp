#include "MazeBoards.h"
#include <string.h>

// The full set is authored in TSK-421; these two exercise the renderer.
const MazeBoard MAZE_BOARDS[] = {
    { "First Steps", {
        "####################",
        "#S       #         #",
        "#        #         #",
        "#   ######   ###   #",
        "#        #     #   #",
        "#        #     #   #",
        "#####    #     #   #",
        "#        ####  #   #",
        "#              #   #",
        "#   ########   #   #",
        "#   #          #   #",
        "#   #    #######   #",
        "#   #             G#",
        "####################",
    } },
    { "Mind the Gap", {
        "####################",
        "#S   #        O    #",
        "#    #   ###       #",
        "#    #     #   #####",
        "#    ###   #       #",
        "#          #O      #",
        "######   ###   #   #",
        "#        #     #   #",
        "#   O    #   ###   #",
        "#   ######     #   #",
        "#        O     #   #",
        "#####    ####  # O #",
        "#              #  G#",
        "####################",
    } },
};
const int MAZE_BOARD_COUNT = sizeof(MAZE_BOARDS) / sizeof(MAZE_BOARDS[0]);

bool mazeBoardValid(const MazeBoard& b) {
    int starts = 0, goals = 0;
    for (int r = 0; r < MAZE_ROWS; r++) {
        if (!b.rows[r] || strlen(b.rows[r]) != MAZE_COLS) return false;
        for (int c = 0; c < MAZE_COLS; c++) {
            char ch = b.rows[r][c];
            bool edge = (r == 0 || r == MAZE_ROWS - 1 || c == 0 || c == MAZE_COLS - 1);
            if (edge && ch != '#') return false;
            if (ch == 'S') starts++;
            else if (ch == 'G') goals++;
            else if (ch != '#' && ch != ' ' && ch != 'O') return false;
        }
    }
    return starts == 1 && goals == 1;
}
