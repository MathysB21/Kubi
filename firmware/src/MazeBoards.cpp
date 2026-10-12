#include "MazeBoards.h"
#include <string.h>

// Ten hand-drawn boards, easiest first. Every one is checked for a hole-free
// route from S to G and cleared by an autopilot through the real physics.
const MazeBoard MAZE_BOARDS[] = {
    // 1. Warm-up: find the way round
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
    // 2. Long zig-zag, no traps
    { "Switchback", {
        "####################",
        "#S                 #",
        "################## #",
        "#                  #",
        "# ##################",
        "#                  #",
        "################## #",
        "#                  #",
        "# ##################",
        "#                  #",
        "################## #",
        "#                  #",
        "#G##################",
        "####################",
    } },
    // 3. First holes
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
    // 4. Open room, weave between posts
    { "Pillars", {
        "####################",
        "#S                 #",
        "#  #  #  #  #  #   #",
        "#     O        O   #",
        "#  #  #  #  #  #   #",
        "#        O         #",
        "#  #  #  #  #  #   #",
        "#  O           O   #",
        "#  #  #  #  #  #   #",
        "#       O          #",
        "#  #  #  #  #  #   #",
        "#            O    G#",
        "#                  #",
        "####################",
    } },
    // 5. Spiral in to the middle
    { "The Snail", {
        "####################",
        "#S                 #",
        "################## #",
        "#                # #",
        "# ############## # #",
        "# #            # # #",
        "# # ########## # # #",
        "# # #       G# # # #",
        "# # #          # # #",
        "# # ############ # #",
        "# #              # #",
        "# ################ #",
        "#                  #",
        "####################",
    } },
    // 6. Branches and two sneaky holes
    { "Crossroads", {
        "####################",
        "#S    #     #      #",
        "# ### # ### # #### #",
        "# #O  #   #   #    #",
        "# # ##### ##### ## #",
        "#   #       #   #  #",
        "### # ### # # ### ##",
        "#   #   # #   #    #",
        "# ##### # ##### ## #",
        "#     # #    O#  # #",
        "# ### # ##### ## # #",
        "#   #            #G#",
        "# #   ###### ##    #",
        "####################",
    } },
    // 7. Proper maze, dead ends
    { "Labyrinth", {
        "####################",
        "#S#           #    #",
        "# # ### # ### # ## #",
        "# #   # #   #   #  #",
        "# ### # ### ##### ##",
        "#   # #   #     #  #",
        "### # ### ##### ## #",
        "#   #   #     #    #",
        "# ##### ##### #### #",
        "#           #    # #",
        "##### # ### #### # #",
        "#     #   #    # # #",
        "# ####### # ##   #G#",
        "####################",
    } },
    // 8. A field of holes
    { "Hole in One", {
        "####################",
        "#S     O     O     #",
        "#   O     O     O  #",
        "#      O     O     #",
        "# O  O    O    O   #",
        "#    O  O    O   O #",
        "#  O    O  O    O  #",
        "#     O    O  O    #",
        "# O    O O    O  O #",
        "#   O     O  O     #",
        "#  O   O    O   O  #",
        "#    O   O     OOO #",
        "#  O    O   O  OG  #",
        "####################",
    } },
    // 9. Narrow lanes between hole rows
    { "Tightrope", {
        "####################",
        "#S  OOOOOOOOOOOO   #",
        "#                  #",
        "#OOOOOOOOOOOOOOO   #",
        "#                  #",
        "#   OOOOOOOOOOOOOOO#",
        "#                  #",
        "#OOOOOOOOOOOOOOO   #",
        "#                  #",
        "#   OOOOOOOOOOOOOOO#",
        "#                  #",
        "#OOOOOOOOOOOOOOO   #",
        "#                 G#",
        "####################",
    } },
    // 10. Finale: holes on every corner
    { "The Gauntlet", {
        "####################",
        "#S   O    #   O    #",
        "#### # ## # # # ## #",
        "#    #  O   #   #  #",
        "# ####### ####### ##",
        "#  O    #  O    #  #",
        "## # ## # ## ## ## #",
        "#  #  O   #   O    #",
        "# ##### ##### ######",
        "#   O #   O   #    #",
        "### # # ##### # ## #",
        "#   #   #   O   #O #",
        "# O   #   #   #   G#",
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
