# 2026-09-06: Embedded Finance Ticker Marquee and Dual-Side Clipping Masks

## 1. Context & Motivation
On Face 4 (Schedule Agenda), agenda items are displayed as cards containing an event's start time and summary. In 320×240 widescreen landscape, the card width is 290px. After allocating space for the static time prefix (e.g. `10:00 - `) and card padding, only ~175px remains for the title.

Long event names (such as `"Design Review with Mobile Engineering Team"`) previously overflowed outside the screen or were truncated awkwardly. The requirement called for a stock-market / finance ticker effect where the time and dash remain fixed in place while overflowing event titles scroll smoothly to the left.

## 2. Key Architecture & Design Decisions

### 2.1 Fixed Prefix vs. Scrolling Suffix
Each list item is partitioned into two distinct visual segments:
1. **Static Segment**: `timePrefix = time + " - "` (e.g. `10:00 - `) drawn at `x = 25`.
2. **Dynamic Segment**: `name` drawn starting at `nameStartX = 25 + prefixWidth`.

If the pixel width of `name` is less than or equal to the available width (`availWidth = nameEndX - nameStartX`), it is rendered statically without animation.

### 2.2 Marquee Scrolling Mathematics
When `nameWidth > availWidth`, horizontal scrolling is calculated based on system uptime:
$$\Delta x = \left(\frac{\text{millis}()}{35}\right) \pmod{\text{unitWidth}}$$
where `unitWidth` is the measured pixel width of `name + "      "`. By doubling the string (`tickerStr = unitStr + unitStr`) and applying modulo wrapping, the ticker seamlessly loops indefinitely with zero jump cuts.

### 2.3 Hardware-Agnostic Dual-Side Clipping Masks
In embedded graphics with `TFT_eSPI`, hardware viewport clipping (`setViewport`) is not universally available or consistent across versions, and software sprites consume large blocks of SRAM ($290 \times 38 \times 2 = 22\text{ KB}$ per card).

Instead, a zero-overhead **dual-side masking** technique was implemented:
1. The full card background is drawn with `fillRoundRect(15, y, cardW, cardH, 8, 0x18E3)`.
2. The overflowing ticker string is drawn across the row: `_tft.drawString(tickerStr, nameStartX - offset, textY, 2)`.
3. **Left Masking**: A filled rectangle in the card's background color (`0x18E3`) is drawn over `[15 .. nameStartX]`:
   ```cpp
   _tft.fillRect(15, y, nameStartX - 15, cardH, 0x18E3);
   _tft.drawString(timePrefix, 25, textY, 2);
   ```
   This masks any scrolled characters before they can touch or overwrite the static time and dash.
4. **Right Masking**: A filled rectangle in the card background color is drawn from `nameEndX` to the inner card edge:
   ```cpp
   _tft.fillRect(nameEndX, y, (w - 15) - nameEndX, cardH, 0x18E3);
   ```
5. **Screen Margin Clear**: Black margins outside the card boundary are cleared to guarantee round corners and black background integrity:
   ```cpp
   _tft.fillRect(0, y, 15, cardH, TFT_BLACK);
   _tft.fillRect(w - 15, y, 15, cardH, TFT_BLACK);
   ```

## 3. Code Touchpoints & Files
- [`DisplayManager.cpp`](file:///c:/Development/Kubi/firmware/src/DisplayManager.cpp): Implementation of `drawScheduleFace` with card rendering, ticker math, and dual-side masks.
- [`TFT_eSPI_Mock.cpp`](file:///c:/Development/Kubi/firmware/sim/src/TFT_eSPI_Mock.cpp): Added `TFT_eSPI::textWidth` to match real `TFT_eSPI` font width calculation in the desktop simulator.
- [`TFT_eSPI.h`](file:///c:/Development/Kubi/firmware/sim/include/TFT_eSPI.h): Declaration of `textWidth(const String&, uint8_t)`.

## 4. Gotchas & Verification
- **Speed & Refresh Rate**: The display loop ticks at ~20–25Hz. At `millis() / 35`, the ticker moves ~28 pixels per second (~2.3 characters/second in Font 2), providing optimal legibility.
- **Card Background Color Match**: The mask rectangles must match the exact 16-bit RGB565 color (`0x18E3`) of the card to appear completely invisible.
- **Verification**:
  - Load a schedule with an event title $> 20$ characters (e.g. `Design Review with Mobile Engineering Team`).
  - Observe in both simulator and physical hardware that `14:00 - ` stays static on the left, while the long title scrolls smoothly without overflowing the card.
