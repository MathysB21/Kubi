# 2026-09-06: 4-Item Screen Pagination and Physical Gesture Navigation for Schedule Agenda

## 1. Context & Motivation
In Project Kubi's zero-button interaction paradigm, navigation through days and events must be completely physical. Previously, a gentle tap simply cycled between two hardcoded sample pages across an unorganized event list.

The schedule face requires:
1. Strict 4-item screen pagination (exactly 4 cards visible at a time).
2. Tapping logic that progresses through sub-pages of the current day before advancing to the next day.
3. Shaking the cube on the Schedule Face to instantly jump back to `Schedule (today)`.

## 2. Key Architecture & Design Decisions

### 2.1 4-Item Pagination State Machine
Each day contains $N$ events. The total sub-pages required is:
$$\text{totalSubPages} = \max\left(1, \left\lfloor\frac{N + 3}{4}\right\rfloor\right)$$

In [`ScheduleManager::handleTap()`](file:///c:/Development/Kubi/firmware/src/ScheduleManager.cpp):
```cpp
void ScheduleManager::handleTap() {
    if (!_hasIcs || _days.empty()) return;

    const auto& currentItems = _days[_currentDayIndex].items;
    int totalSubPages = ((int)currentItems.size() + 3) / 4;
    if (totalSubPages < 1) totalSubPages = 1;

    if (_currentSubPage + 1 < totalSubPages) {
        // More items remaining in today's list: show next 4 items
        _currentSubPage++;
    } else {
        // Reached end of today's items: advance to next day, reset subpage
        _currentSubPage = 0;
        _currentDayIndex = (_currentDayIndex + 1) % _days.size();
    }
}
```
If a day has $\le 4$ items, `totalSubPages == 1`, so tapping immediately advances to the next day.
When reaching the end of the final day, `_currentDayIndex` wraps back to 0 (`Schedule (today)`).

### 2.2 Shake-to-Today Gesture (`GESTURE_SHAKE`)
In [`main.cpp`](file:///c:/Development/Kubi/firmware/src/main.cpp) and [`sim_main.cpp`](file:///c:/Development/Kubi/firmware/sim/src/sim_main.cpp):
```cpp
case GESTURE_SHAKE:
    if (currentMode == MODE_POMODORO) {
        pomodoro.handleShake();
    } else if (currentMode == MODE_CLOCK_IDLE) {
        clockAnalogView = !clockAnalogView;
        ...
    } else if (currentMode == MODE_SCHEDULE_AGENDA) {
        schedule.handleShake();
        audio.playChime(CHIME_TAP_FEEDBACK);
    }
    break;
```
`schedule.handleShake()` unconditionally resets `_currentDayIndex = 0` and `_currentSubPage = 0`. An audio chime provides tactile auditory feedback that the view jumped back to Today.

### 2.3 Companion Web Testing Tools
To support rapid verification in the Digital Twin workbench and companion dashboard without manual iCal calendar setup:
- `POST /api/calendar/sample`: Injects a sample calendar containing:
  - Day 0: 6 items (triggers sub-page paging + marquee ticker).
  - Day 1: 3 items (Tomorrow).
  - Day 2: 2 items (e.g. Wed).
  - Day 3: 1 item.
  - Day 4: 1 item.
- `POST /api/calendar/ics`: Ingests raw `.ics` strings from uploaded files.
- `DELETE /api/calendar`: Purges calendar data to test the `"No calendar connected, that's sad"` state.

## 3. Code Touchpoints & Files
- [`ScheduleManager.h`](file:///c:/Development/Kubi/firmware/src/ScheduleManager.h): State variables `_currentDayIndex`, `_currentSubPage`, and method declarations.
- [`ScheduleManager.cpp`](file:///c:/Development/Kubi/firmware/src/ScheduleManager.cpp): Implementation of `handleTap()`, `handleShake()`, and `loadSampleSchedule()`.
- [`main.cpp`](file:///c:/Development/Kubi/firmware/src/main.cpp): Core 1 hardware loop gesture routing for `MODE_SCHEDULE_AGENDA`.
- [`sim_main.cpp`](file:///c:/Development/Kubi/firmware/sim/src/sim_main.cpp): Digital Twin desktop runner gesture routing.
- [`App.tsx`](file:///c:/Development/Kubi/dashboard/src/App.tsx): Dashboard testing buttons ("Load Sample", "Empty Cal", "Clear ICS").

## 4. Gotchas & Verification
- **False Tap Suppression**: Remember that switching to Face 4 causes high accelerometer delta transients (`deltaMag > 7.0f`). In `main.cpp`, `sensors.getRecentGesture()` is always flushed upon an orientation change so physical cube rotation does not immediately register as an agenda tap.
- **Verification**:
  1. Load sample schedule.
  2. Tap cube once: verify Day 0 transitions from items 1–4 to items 5–6.
  3. Tap cube again: verify view advances to `Schedule (Tomorrow)`.
  4. Tap cube again: verify view advances to `Schedule (Wed)`.
  5. Shake cube: verify instant return to `Schedule (today)` page 0 with audio feedback.
