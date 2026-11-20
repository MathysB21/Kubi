// Maze board checker: proves every board in MazeBoards.cpp can be played.
//
//   powershell -ExecutionPolicy Bypass -File firmware/sim/maze_check.ps1
//
// Builds against the real MazeGame/MazeBoards sources and the sim HAL, and
// runs from a temp directory so the Preferences mock (which writes
// kubi_sim_prefs.txt in the working directory) never touches the sim's prefs.
//
// For each board it checks:
//   1. Shape: mazeBoardValid(), every row 20 chars, exactly one S and one G.
//   2. Reachability: BFS over 4-connected floor (not wall, not hole) from S to
//      G. Prints the shortest route length, the hole count and the number of
//      dead ends (floor cells with exactly one open neighbour, S/G excluded).
//      "tight" is the longest run of route cells with holes on both sides
//      (keep new boards at 3 or less: longer needs luck, not skill; board 9,
//      Tightrope, is the one deliberate exception at 12).
//   3. Autopilot: drives the ball through the REAL MazeGame physics by feeding
//      accelerometer readings from tiltedGravityFlat() (view face 1, the
//      landscape view, not transposed), tilt clamped to 12 degrees. It follows
//      a route that prefers cells away from holes, straightened where the
//      line keeps clear of walls and holes, slowing near holes and corners.
//      Passing = MAZE_WON within 180 s simulated with zero falls.
//   4. Shaky run (information only, not part of PASS): the same pilot hurried
//      to 1.6x its speed, with a 120 ms reaction delay, a 20 Hz control rate
//      and sensor noise. Falls there mean the board punishes rushing; the
//      original Gauntlet and Tightrope fall there too, so it marks "hard",
//      not "unfair". Printed as "shaky".
//
// The run starts with a mapping self-test on board 1 (roll right must roll
// the ball right, top edge up must roll it down), so a sign change in
// FaceMap.h or MazeGame.cpp is caught before any board result is trusted.
//
// Options:
//   --show N   print board N with the autopilot's route marked '.'
//   --grade    difficulty ladder: the shaky pilot at 1.6x, 2x and 2.5x speed,
//              three seeds each, falls per speed and a score (used to order
//              the boards easiest first; information only)
//
// Adding a board: append it to MazeBoards.cpp, run this, check the board
// passes, that "tight" is 3 or less, and look at --show N to see the route is
// the one you meant. Exit code is non-zero if any board fails 1-3 or the
// mapping test fails.
#include <cstdio>
#include <cstring>
#include <cmath>
#include <vector>
#include <queue>
#include <random>
#include <algorithm>
#include "Arduino.h"
#include "Preferences.h"
#include "AudioManager.h"
#include "FaceMap.h"
#include "MazeBoards.h"
#include "MazeGame.h"

SimSerial Serial;
std::map<std::string, std::map<std::string, std::string>> Preferences::_storage;
bool Preferences::_loaded = false;
uint8_t sim_backlight_value = 255;
int sim_hour_override = -1;
int sim_minute_override = -1;

// MazeGame only plays chimes; a silent stand-in keeps the audio thread out.
AudioManager::AudioManager() : _i2sOut(nullptr), _ready(false), _gain(1.0f), _requestedChime(CHIME_NONE),
    _requestSeq(0), _handledSeq(0), _activeChime(CHIME_NONE), _sequence(nullptr), _noteIdx(0),
    _noteSamplesLeft(0), _phase(0), _phaseInc(0), _dacMode(false) {}
void AudioManager::playChime(KubiChime) {}
AudioManager audio;

// Physics constants mirrored from MazeGame.cpp (controller use only)
static const float GAIN = 3.2f, DEADZONE = 0.35f, DAMPING = 1.1f;
static const int VIEW_FACE = 1;
static const float MAX_TILT_DEG = 12.0f;

struct Cell { int c, r; };

static char at(const MazeBoard& b, int c, int r) {
    if (c < 0 || r < 0 || c >= MAZE_COLS || r >= MAZE_ROWS) return '#';
    return b.rows[r][c];
}
static bool open(const MazeBoard& b, int c, int r) {
    char ch = at(b, c, r);
    return ch != '#' && ch != 'O';
}
static int holesNear(const MazeBoard& b, int c, int r, bool diag) {
    int n = 0;
    for (int dr = -1; dr <= 1; dr++)
        for (int dc = -1; dc <= 1; dc++) {
            if (!dr && !dc) continue;
            if (!diag && dr && dc) continue;
            if (diag && !(dr && dc)) continue;
            if (at(b, c + dc, r + dr) == 'O') n++;
        }
    return n;
}

static const int DC[4] = { 1, -1, 0, 0 }, DR[4] = { 0, 0, 1, -1 };

// Shortest (weighted) route S -> G; weight 0 gives plain BFS length.
static std::vector<Cell> route(const MazeBoard& b, Cell s, Cell g, float holeWeight) {
    const int N = MAZE_COLS * MAZE_ROWS;
    std::vector<float> dist(N, 1e9f);
    std::vector<int> prev(N, -1);
    typedef std::pair<float, int> QE;
    std::priority_queue<QE, std::vector<QE>, std::greater<QE>> q;
    int si = s.r * MAZE_COLS + s.c, gi = g.r * MAZE_COLS + g.c;
    dist[si] = 0;
    q.push({ 0, si });
    while (!q.empty()) {
        QE e = q.top(); q.pop();
        if (e.first > dist[e.second]) continue;
        int c = e.second % MAZE_COLS, r = e.second / MAZE_COLS;
        for (int k = 0; k < 4; k++) {
            int nc = c + DC[k], nr = r + DR[k];
            if (!open(b, nc, nr)) continue;
            float w = 1.0f + holeWeight * (holesNear(b, nc, nr, false) + 0.4f * holesNear(b, nc, nr, true));
            int ni = nr * MAZE_COLS + nc;
            if (dist[e.second] + w < dist[ni]) {
                dist[ni] = dist[e.second] + w;
                prev[ni] = e.second;
                q.push({ dist[ni], ni });
            }
        }
    }
    std::vector<Cell> path;
    if (dist[gi] >= 1e9f) return path;
    for (int i = gi; i != -1; i = prev[i]) path.push_back({ i % MAZE_COLS, i / MAZE_COLS });
    std::reverse(path.begin(), path.end());
    return path;
}

static int deadEnds(const MazeBoard& b) {
    int n = 0;
    for (int r = 1; r < MAZE_ROWS - 1; r++)
        for (int c = 1; c < MAZE_COLS - 1; c++) {
            if (at(b, c, r) != ' ') continue;
            int k = 0;
            for (int d = 0; d < 4; d++) if (open(b, c + DC[d], r + DR[d])) k++;
            if (k == 1) n++;
        }
    return n;
}

// Longest run of route cells with a hole on both sides across the direction of travel
static int tightRun(const MazeBoard& b, const std::vector<Cell>& p) {
    int best = 0, run = 0;
    for (size_t i = 0; i < p.size(); i++) {
        int dc, dr;
        if (i + 1 < p.size()) { dc = p[i + 1].c - p[i].c; dr = p[i + 1].r - p[i].r; }
        else if (i > 0) { dc = p[i].c - p[i - 1].c; dr = p[i].r - p[i - 1].r; }
        else { dc = 1; dr = 0; }
        // perpendicular neighbours
        bool both = at(b, p[i].c + dr, p[i].r + dc) == 'O' && at(b, p[i].c - dr, p[i].r - dc) == 'O';
        run = both ? run + 1 : 0;
        best = std::max(best, run);
    }
    return best;
}

// Clearance of a straight segment: distance to walls and hole centres
static bool segmentClear(const MazeBoard& b, float x0, float y0, float x1, float y1) {
    float len = hypotf(x1 - x0, y1 - y0);
    int n = std::max(2, (int)(len / 0.05f));
    for (int i = 0; i <= n; i++) {
        float t = (float)i / n, x = x0 + (x1 - x0) * t, y = y0 + (y1 - y0) * t;
        int cc = (int)floorf(x), rr = (int)floorf(y);
        for (int r = rr - 1; r <= rr + 1; r++)
            for (int c = cc - 1; c <= cc + 1; c++) {
                char ch = at(b, c, r);
                if (ch == '#') {
                    float nx = std::min(std::max(x, (float)c), (float)c + 1);
                    float ny = std::min(std::max(y, (float)r), (float)r + 1);
                    if (hypotf(x - nx, y - ny) < 0.42f) return false;
                } else if (ch == 'O') {
                    if (hypotf(x - (c + 0.5f), y - (r + 0.5f)) < 0.85f) return false;
                }
            }
    }
    return true;
}
static float holeDistance(const MazeBoard& b, float x0, float y0, float x1, float y1) {
    float best = 99;
    float len = hypotf(x1 - x0, y1 - y0);
    int n = std::max(2, (int)(len / 0.1f));
    for (int i = 0; i <= n; i++) {
        float t = (float)i / n, x = x0 + (x1 - x0) * t, y = y0 + (y1 - y0) * t;
        for (int r = (int)y - 2; r <= (int)y + 2; r++)
            for (int c = (int)x - 2; c <= (int)x + 2; c++)
                if (at(b, c, r) == 'O') best = std::min(best, hypotf(x - (c + 0.5f), y - (r + 0.5f)));
    }
    return best;
}

struct Waypoint { float x, y, vmax, vend; };

static std::vector<Waypoint> waypoints(const MazeBoard& b, const std::vector<Cell>& p) {
    std::vector<Waypoint> w;
    size_t a = 0;
    w.push_back({ p[0].c + 0.5f, p[0].r + 0.5f, 0, 0 });
    while (a + 1 < p.size()) {
        size_t best = a + 1;
        for (size_t k = a + 2; k < p.size(); k++) {
            if (segmentClear(b, p[a].c + 0.5f, p[a].r + 0.5f, p[k].c + 0.5f, p[k].r + 0.5f)) best = k;
            else if (k > a + 6) break;  // keep searching a little past a blocked one
        }
        w.push_back({ p[best].c + 0.5f, p[best].r + 0.5f, 0, 0 });
        a = best;
    }
    for (size_t i = 1; i < w.size(); i++) {
        float hd = holeDistance(b, w[i - 1].x, w[i - 1].y, w[i].x, w[i].y);
        w[i].vmax = hd < 1.2f ? 1.8f : hd < 1.7f ? 2.6f : 4.5f;
        if (i + 1 < w.size()) {
            float ax = w[i].x - w[i - 1].x, ay = w[i].y - w[i - 1].y;
            float bx = w[i + 1].x - w[i].x, by = w[i + 1].y - w[i].y;
            float cosT = (ax * bx + ay * by) / (hypotf(ax, ay) * hypotf(bx, by));
            w[i].vend = cosT > 0.95f ? 1.5f : 0.35f;
        } else {
            w[i].vend = 0.3f;
        }
    }
    return w;
}

// Board-axis tilt (m/s^2, what MazeGame sees as tiltX/tiltY) to a reading
static KVec3 readingFor(float tiltX, float tiltY) {
    float roll = asinf(std::max(-1.0f, std::min(1.0f, tiltX / 9.8f))) * 57.29578f;
    float pitch = -asinf(std::max(-1.0f, std::min(1.0f, tiltY / 9.8f))) * 57.29578f;
    return tiltedGravityFlat(VIEW_FACE, roll, pitch);
}

struct PilotOpts { int controlMs; int delayMs; float noise; unsigned seed; float speed; };
struct RunResult { bool won; float seconds; int falls; };

static void settle(MazeGame& g, uint32_t& now, int ms) {
    KVec3 lv = tiltedGravityFlat(VIEW_FACE, 0, 0);
    for (int t = 0; t < ms; t += 5) { now += 5; simFakeMillis() = now; g.update(lv.x, lv.y, lv.z, now); }
}

static RunResult fly(int index, const std::vector<Waypoint>& wp, const PilotOpts& o) {
    MazeGame g;
    g.setViewFace(VIEW_FACE);
    g.loadBoard(index);
    uint32_t now = 1000;
    settle(g, now, 600);
    RunResult res = { false, 0, 0 };
    std::mt19937 rng(o.seed);
    std::normal_distribution<float> nd(0.0f, 1.0f);
    const float maxTilt = 9.8f * sinf(MAX_TILT_DEG * 0.01745329f);
    size_t k = 1;
    float px = g.ballX(), py = g.ballY();
    struct Cmd { uint32_t at; float tx, ty; };
    std::vector<Cmd> pending;
    float curTx = 0, curTy = 0;
    uint32_t nextControl = now;
    MazeState last = g.state();
    uint32_t start = now;
    while (now - start < 180000) {
        float bx = g.ballX(), by = g.ballY();
        float vx = (bx - px) / 0.005f, vy = (by - py) / 0.005f;
        px = bx; py = by;
        if (now >= nextControl) {
            nextControl = now + o.controlMs;
            if (k >= wp.size()) k = wp.size() - 1;
            const Waypoint& a = wp[k - 1];
            const Waypoint& t = wp[k];
            float sx = t.x - a.x, sy = t.y - a.y, sl = hypotf(sx, sy);
            float ux = sx / sl, uy = sy / sl;          // along the segment
            float nx = -uy, ny = ux;                   // across it
            float along = (bx - a.x) * ux + (by - a.y) * uy;
            float lat = (bx - a.x) * nx + (by - a.y) * ny;
            float vAlong = vx * ux + vy * uy, vLat = vx * nx + vy * ny;
            float remain = sl - along;
            float vdes = o.speed * std::min(t.vmax, sqrtf(t.vend * t.vend + 2.0f * 2.5f * std::max(0.0f, remain)));
            float aAlong = 4.0f * (vdes - vAlong) + DAMPING * vAlong;
            float aLat = -14.0f * lat - 6.5f * vLat + DAMPING * vLat;
            float ax = aAlong * ux + aLat * nx, ay = aAlong * uy + aLat * ny;
            float am = hypotf(ax, ay);
            float tx = 0, ty = 0;
            if (am > 0.05f) {
                float tilt = std::min(maxTilt, am / GAIN + DEADZONE);
                tx = ax / am * tilt; ty = ay / am * tilt;
            }
            pending.push_back({ now + (uint32_t)o.delayMs, tx, ty });
            if (remain < 0.12f && k + 1 < wp.size()) k++;
        }
        while (!pending.empty() && pending.front().at <= now) {
            curTx = pending.front().tx; curTy = pending.front().ty;
            pending.erase(pending.begin());
        }
        KVec3 rd = readingFor(curTx, curTy);
        if (o.noise > 0) { rd.x += o.noise * nd(rng); rd.y += o.noise * nd(rng); rd.z += o.noise * nd(rng); }
        now += 5;
        simFakeMillis() = now;
        g.update(rd.x, rd.y, rd.z, now);
        MazeState s = g.state();
        if (s == MAZE_FALLING && last != MAZE_FALLING) res.falls++;
        if (s == MAZE_RUNNING && last == MAZE_FALLING) { k = 1; pending.clear(); curTx = curTy = 0; px = g.ballX(); py = g.ballY(); }
        last = s;
        if (s == MAZE_WON) { res.won = true; res.seconds = g.elapsedMs(now) / 1000.0f; break; }
    }
    if (!res.won) res.seconds = (now - start) / 1000.0f;
    return res;
}

// Roll right must move the ball right (+x), pitch top-edge-down must move it up (-y)
static bool mappingTest() {
    bool ok = true;
    for (int axis = 0; axis < 2; axis++) {
        MazeGame g;
        g.setViewFace(VIEW_FACE);
        g.loadBoard(0);
        uint32_t now = 1000;
        settle(g, now, 600);
        float x0 = g.ballX(), y0 = g.ballY();
        // Board 1 start (1,1): open to the right and down, so test right and down.
        // Down is pitch < 0 (top edge up).
        KVec3 rd = axis == 0 ? tiltedGravityFlat(VIEW_FACE, 8, 0) : tiltedGravityFlat(VIEW_FACE, 0, -8);
        for (int t = 0; t < 250; t += 5) { now += 5; simFakeMillis() = now; g.update(rd.x, rd.y, rd.z, now); }
        float dx = g.ballX() - x0, dy = g.ballY() - y0;
        bool good = axis == 0 ? (dx > 0.05f && fabsf(dy) < 0.01f) : (dy > 0.05f && fabsf(dx) < 0.01f);
        printf("mapping: %s -> ball moved (%+.3f, %+.3f) %s\n", axis == 0 ? "roll +8 (right down)" : "pitch -8 (top up)  ",
               dx, dy, good ? "ok" : "WRONG");
        ok = ok && good;
    }
    // Our own conversion: tiltY +2 must roll the ball down the board
    {
        MazeGame g;
        g.setViewFace(VIEW_FACE);
        g.loadBoard(0);
        uint32_t now = 1000;
        settle(g, now, 600);
        KVec3 rd = readingFor(0, 2.0f);
        for (int t = 0; t < 250; t += 5) { now += 5; simFakeMillis() = now; g.update(rd.x, rd.y, rd.z, now); }
        bool good = g.ballY() - 1.5f > 0.05f;
        printf("mapping: readingFor(tiltY +2) -> ball dy %+.3f %s\n", g.ballY() - 1.5f, good ? "ok" : "WRONG");
        ok = ok && good;
    }
    return ok;
}

// --show N: print board N (1-based) with the autopilot's route marked '.'
static void showBoard(int i) {
    const MazeBoard& b = MAZE_BOARDS[i];
    Cell s = { 1, 1 }, gl = { 1, 1 };
    for (int r = 0; r < MAZE_ROWS; r++)
        for (int c = 0; c < MAZE_COLS && b.rows[r][c]; c++) {
            if (b.rows[r][c] == 'S') s = { c, r };
            if (b.rows[r][c] == 'G') gl = { c, r };
        }
    std::vector<Cell> safe = route(b, s, gl, 4.0f);
    char grid[MAZE_ROWS][MAZE_COLS + 1];
    for (int r = 0; r < MAZE_ROWS; r++) { memcpy(grid[r], b.rows[r], MAZE_COLS); grid[r][MAZE_COLS] = 0; }
    for (const Cell& c : safe) if (grid[c.r][c.c] == ' ') grid[c.r][c.c] = '.';
    printf("%d. %s (route %d cells)\n", i + 1, b.name, (int)safe.size() - 1);
    for (int r = 0; r < MAZE_ROWS; r++) printf("   %s\n", grid[r]);
}

int main(int argc, char** argv) {
    if (argc == 3 && !strcmp(argv[1], "--show")) {
        int n = atoi(argv[2]);
        if (n >= 1 && n <= MAZE_BOARD_COUNT) showBoard(n - 1);
        return 0;
    }
    // --grade: a rough difficulty ladder. The shaky pilot at three speeds,
    // three seeds each; prints falls per speed (x = never finished).
    if (argc == 2 && !strcmp(argv[1], "--grade")) {
        simFakeMillis() = 1000;
        const float speeds[3] = { 1.6f, 2.0f, 2.5f };
        for (int i = 0; i < MAZE_BOARD_COUNT; i++) {
            const MazeBoard& b = MAZE_BOARDS[i];
            Cell s = { 1, 1 }, gl = { 1, 1 };
            for (int r = 0; r < MAZE_ROWS; r++)
                for (int c = 0; c < MAZE_COLS; c++) {
                    if (b.rows[r][c] == 'S') s = { c, r };
                    if (b.rows[r][c] == 'G') gl = { c, r };
                }
            std::vector<Cell> safe = route(b, s, gl, 4.0f);
            if (safe.empty()) continue;
            std::vector<Waypoint> wp = waypoints(b, safe);
            printf("%3d %-20s", i + 1, b.name);
            int score = 0;
            float t = 0;
            for (int k = 0; k < 3; k++) {
                int falls = 0, lost = 0;
                for (unsigned seed = 0; seed < 3; seed++) {
                    RunResult r = fly(i, wp, { 50, 120, 0.4f, 77u + seed * 31u + i, speeds[k] });
                    falls += r.falls;
                    if (!r.won) lost++;
                    if (k == 0) t += r.seconds / 3;
                }
                score += falls + 10 * lost;
                printf("  x%.2f: %3d falls%s", speeds[k], falls, lost ? (lost == 3 ? " xxx" : " x  ") : "    ");
            }
            printf("  | %5.1fs  score %d\n", t, score);
        }
        return 0;
    }
    simFakeMillis() = 1000;
    bool mapOk = mappingTest();
    int failed = 0;
    printf("\n  # name                 valid bfs holes dead tight | autopilot         | shaky\n");
    for (int i = 0; i < MAZE_BOARD_COUNT; i++) {
        const MazeBoard& b = MAZE_BOARDS[i];
        bool rowsOk = true;
        int sCount = 0, gCount = 0, holes = 0;
        Cell s = { 1, 1 }, gl = { 1, 1 };
        for (int r = 0; r < MAZE_ROWS; r++) {
            if (!b.rows[r] || strlen(b.rows[r]) != MAZE_COLS) { rowsOk = false; continue; }
            for (int c = 0; c < MAZE_COLS; c++) {
                char ch = b.rows[r][c];
                if (ch == 'S') { sCount++; s = { c, r }; }
                if (ch == 'G') { gCount++; gl = { c, r }; }
                if (ch == 'O') holes++;
            }
        }
        bool valid = rowsOk && sCount == 1 && gCount == 1 && mazeBoardValid(b);
        if (!valid) {
            printf("%3d %-20s  NO   (rows %s, S %d, G %d)  FAIL\n", i + 1, b.name, rowsOk ? "ok" : "BAD", sCount, gCount);
            failed++;
            continue;
        }
        std::vector<Cell> bfs = route(b, s, gl, 0);
        if (bfs.empty()) {
            printf("%3d %-20s  yes  no route S->G  FAIL\n", i + 1, b.name);
            failed++;
            continue;
        }
        std::vector<Cell> safe = route(b, s, gl, 4.0f);
        int tight = std::min(tightRun(b, bfs), tightRun(b, safe));
        std::vector<Waypoint> wp = waypoints(b, safe);
        RunResult ap = fly(i, wp, { 5, 0, 0, 1, 1.0f });
        RunResult sh = fly(i, wp, { 50, 120, 0.4f, 1234u + i, 1.6f });
        bool pass = ap.won && ap.falls == 0;
        if (!pass) failed++;
        char apTxt[40], shTxt[40];
        if (ap.won) snprintf(apTxt, sizeof apTxt, "won %5.1fs falls %d", ap.seconds, ap.falls);
        else snprintf(apTxt, sizeof apTxt, "TIMEOUT falls %d", ap.falls);
        if (sh.won) snprintf(shTxt, sizeof shTxt, "%5.1fs falls %d", sh.seconds, sh.falls);
        else snprintf(shTxt, sizeof shTxt, "timeout falls %d", sh.falls);
        printf("%3d %-20s  yes %4d %5d %4d %5d | %-17s | %-16s %s\n", i + 1, b.name, (int)bfs.size() - 1, holes,
               deadEnds(b), tight, apTxt, shTxt, pass ? "PASS" : "FAIL");
    }
    printf("\n%s: %d of %d boards pass%s\n", (failed == 0 && mapOk) ? "PASS" : "FAIL", MAZE_BOARD_COUNT - failed,
           MAZE_BOARD_COUNT, mapOk ? "" : " (steering mapping test FAILED)");
    return (failed == 0 && mapOk) ? 0 : 1;
}
