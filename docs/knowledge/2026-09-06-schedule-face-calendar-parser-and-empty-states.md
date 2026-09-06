# 2026-09-06: Schedule Face Calendar Parser, 5-Day Range Ceiling, and Empty State Messaging

## 1. Context & Motivation
Previously, Face 4 (Schedule Face) displayed static hardcoded mock agenda items (`"Today 10:00 - Team Sync"`, etc.) seeded in memory. The header was rendered in `TFT_GOLD` as `"SCHEDULE (3 DAYS)"`. When no events were present, it rendered a generic placeholder `"No upcoming events"`.

The schedule face was redesigned to:
- Use vibrant pink styling (`TFT_PINK`, `0xFE19`) for all schedule header text.
- Dynamically format day titles relative to the current calendar date (`Schedule (today)`, `Schedule (Tomorrow)`, and `Schedule (<3-letter Day>)`).
- Enforce a strict **5-day ceiling** to preserve precious RAM and prevent CPU exhaustion on ESP32 microcontrollers.
- Completely remove mock agenda data and provide distinct empty states:
  - When no calendar/ICS feed is connected: `"No calendar connected,\nthat's sad"`
  - When a calendar is connected but has no events: `"Nothing happening,\nI guess"`

## 2. Key Architecture & Design Decisions

### 2.1 Dedicated `ScheduleManager` Component
Rather than cluttering `main.cpp` or `DisplayManager.cpp` with calendar handling, a dedicated [`ScheduleManager`](file:///c:/Development/Kubi/firmware/src/ScheduleManager.h) class was introduced following the architectural conventions of `PomodoroManager` and `SensorManager`:
- Manages an array of 5 `ScheduleDay` structs (day offsets 0 through 4).
- Parses RFC-5545 `.ics` content on-device into lightweight `ScheduleItem` records (`time` and `name`).
- Handles storage persistence: LittleFS (`/calendar.ics`) on ESP32 silicon, and file `kubi_sim_calendar.ics` in desktop simulation.

### 2.2 Day Title Formatting & 5-Day Ceiling
The title reflects the current active day:
- **Day 0**: `"Schedule (today)"` (note lowercase "today" per specification).
- **Day 1**: `"Schedule (Tomorrow)"`.
- **Days 2–4**: `"Schedule (" + DAY_NAMES[(wday + offset) % 7] + ")"`, e.g., `Schedule (Wed)`, `Schedule (Thu)`, `Schedule (Fri)`.
Events occurring further than 4 days ahead ($diffDays \ge 5$) or in the past ($diffDays < 0$) are filtered out during ingestion.

### 2.3 Lightweight RFC-5545 Ingestion
Kubi embeds a standalone iCalendar parser:
1. **Line Unfolding**: Unfolds RFC-5545 CRLF/LF followed by space or tab (` ` or `\t`).
2. **`VEVENT` Scanning**: Captures `DTSTART` and `SUMMARY`.
3. **Date Math via Civil Calendar**: Uses Howard Hinnant's standard algorithm (`getDaysFromCivil`) to compute exact day deltas without timezone conversion overhead or floating-point epoch math.
4. **Unescaping & Time Formatting**: Unescapes `\,`, `\;`, and `\\`. Formats start timestamps to `HH:MM` or `"All day"`.
5. **Chronological Sorting**: Sorts events within each day chronologically by `time`.

### 2.4 Empty State Distinctions
- **`!hasIcs`**: Centered across two lines:
  ```
  No calendar connected,
  that's sad
  ```
- **`hasIcs && (!hasEvents || items.empty())`**: Centered across two lines:
  ```
  Nothing happening,
  I guess
  ```

## 3. Code Touchpoints & Files
- [`ScheduleManager.h`](file:///c:/Development/Kubi/firmware/src/ScheduleManager.h): Structs `ScheduleItem`, `ScheduleDay`, and `ScheduleManager` interface.
- [`ScheduleManager.cpp`](file:///c:/Development/Kubi/firmware/src/ScheduleManager.cpp): Calendar parser, day titles, 5-day ceiling, and file persistence.
- [`DisplayManager.h`](file:///c:/Development/Kubi/firmware/src/DisplayManager.h): Signature for `drawScheduleFace(dayTitle, items, hasIcs, hasEvents)`.
- [`DisplayManager.cpp`](file:///c:/Development/Kubi/firmware/src/DisplayManager.cpp): Pink header, empty state two-line typography, and card rendering.
- [`API.cpp`](file:///c:/Development/Kubi/firmware/src/API.cpp): `/api/state` telemetry (`hasIcs`, `hasEvents`, `scheduleDay`), `/api/calendar/ics`, `/api/calendar/sample`, and `DELETE /api/calendar`.
- [`App.tsx`](file:///c:/Development/Kubi/dashboard/src/App.tsx): Dashboard upload, sample loader, and status badges.

## 4. Gotchas & Verification
- **Header Color**: Ensure `TFT_PINK` (`0xFE19`) is defined in both mock and production headers.
- **`<sstream>` on Embedded ESP32**: Include `<sstream>` unconditionally; isolating it under `#else` causes compilation failures in GCC Xtensa.
- **Verification**:
  - `pio run` builds cleanly for ESP32.
  - Test via Dashboard: clicking "Clear ICS" sets "No calendar connected, that's sad"; clicking "Empty Cal" sets "Nothing happening, I guess".
