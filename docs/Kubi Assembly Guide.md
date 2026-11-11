# Kubi Assembly Guide

Step-by-step soldering and assembly for the 90 mm cube. Hole positions (D2, A14-H14, S6, ...) are the coordinates printed on the perfboard (component side, column letter A-T, row number 01-14) and refer to the perfboard plan in `Kubi Architecture and Wiring Guide.md`, section 5. Work through the stages in order: each one ends with a check, so a mistake is caught before the next stage builds on it.

## Before you start

Tools: soldering iron and solder, flush cutters, wire strippers, multimeter (with continuity beep), a marker, tape or heat shrink, hot glue.

Parts: ESP32 DevKit, Waveshare 2" screen with its 8-wire cable, the ADXL345/BMP280 board (DFRobot 10DOF V2), PAM8403, two laptop speakers, bare 2 x 18650 holder (parallel), two INR18650-26E cells, TP4056 USB-C (protected; the PK4056 clone in use has the same pads: OUT+, B+, B-, OUT- opposite the USB-C, B+ and OUT+ on one net), MT3608, 40 x 60 mm perfboard, female socket strips, 8 male header pins, two 10 kΩ resistors, 26 AWG stranded wire.

Battery rules for the whole build:

- Take the cells out of the holder whenever you solder anything on the power side. Put them back only when a step says so.
- Never solder onto an 18650 cell itself. Solder to the holder's leads or tabs.
- Every joint on the battery side gets insulated (heat shrink or tape) before the cells go in. The cells can deliver several amps into a short.
- Red = +, black = -. Check every power wire with the multimeter before trusting its colour.

## Stage 1: Power chain (on the bench, no perfboard yet)

1. Match the cells. Measure each cell on its own. If they differ by more than 0.1 V, charge the lower one alone first, once step 2 is done (only that cell in the holder, USB-C into the TP4056, wait for green). Two cells in parallel at different voltages push a large current into each other the moment they touch.
2. With the cells out, solder the holder's + lead to TP4056 B+ and its - lead to B-. Length ~8 cm: in the cube the holder sits behind the screen and the TP4056 on the back wall.
3. Battery-sense resistor R1 (10 kΩ): solder one leg of R1 onto the holder's + lead, as close to the holder as possible (wrap the leg around the stripped wire or onto the holder's tab, then solder). Solder a ~12 cm wire to R1's other leg. Cover R1 and both joints completely with heat shrink or tape. Label the loose end "SENSE".
4. Solder TP4056 OUT+ to MT3608 IN+ and OUT- to IN- (~5 cm). Leave the MT3608 output unconnected.
5. Insert both cells. The TP4056 may blink or stay dark without USB; that is fine.
6. Set the MT3608 to 5 V. Clip the multimeter to MT3608 OUT+ / OUT- and keep it connected the whole time: never turn the trimmer without watching the reading. Turn the small brass trimmer half a turn at a time and watch which way the voltage moves (on the board in use, counter-clockwise raised it); if it climbs past 5.5 V, turn back at once. Stop at 5.0-5.1 V. Do not continue until it reads that. One MT3608 died during this step: turned without the meter attached, the output overshot, and afterwards it only passed the battery voltage through (~3.9 V at both trimmer end stops).
7. Plug USB-C into the TP4056: red LED = charging, green = full. Unplug.
8. Remove the cells. Solder a red ~10 cm wire to MT3608 OUT+ and a black one to OUT-. Label them "5V" and "GND".

Check: cells in, MT3608 output reads 5.0-5.1 V, the SENSE wire reads about half the cell voltage to GND after R2 is added in stage 2 (right now it reads the cell voltage through R1, which is expected). Cells out again before stage 2.

## Stage 2: Perfboard

Orientation: use the coordinates printed on the component side (the ESP32 side), column A on the left, row 01 at the bottom. Every hole name in this guide is the component-side one. The solder side has its own printed letters that name different holes: turned over side to side like a page, row 01 stays at the bottom and they still read A to T from left to right, so each column has a different letter on each side (A and T swap, B and S, C and R, D and Q, E and P, F and O, G and N, H and M, I and L, J and K). The Wire by wire view in the build viewer shows the solder-side names.

After every joint or wire, beep it against its neighbours. Each step below ends with the pairs that must stay silent.

1. Cut two 15-way strips from the female sockets: cut through the 16th socket (it is lost), then file the cut edge smooth. The leftover 4-way piece is the IMU socket.
2. Solder the ESP32 sockets using the ESP32 itself as the jig: push the ESP32 into both strips, chip up, then drop the strips into A2-O2 and A12-O12 with the antenna end (EN / D23) at column A, hanging off the board edge. EN must be in A2 and VIN in O2. Turn it over, tack the corners A2, O2, A12 and O12, check everything sits flat and square, then solder the rest. Pull the ESP32 out, trim the legs to a ~2 mm stub (flush cutters, flat side away from the board, keep the offcuts), and check that no two neighbouring holes in either row beep. A joint that balls up on the pin has not wetted the pad: flux, and reheat pad and pin together for 2-3 s.
3. Screen pins: 8 male header pins into A14-H14, short end through the board, long end up. Tack A14, straighten, tack H14, solder the rest. A14-B14 ... G14-H14 silent.
4. IMU socket: the 4-way female piece into J14-M14. Trim to 2 mm. J14 ... M14 and H14-J14 silent. Then test-fit the ESP32 and the IMU: the IMU must lie with its body off the board's row-14 edge, away from the ESP32. Note which hole its VIN pin lands in (M14 on the board in use) and adjust the IMU and 3.3 V wires if it is different.
5. R2 (10 kΩ): measure it, then lay it flat on the component side from D3 to D7, body touching the board. On the solder side, solder D7 and trim it, then bend the D3 leg over onto D2's stub (the D34 socket pin) and solder it there: that leg is the bridge. D2-D3 beeps; D2-C2 and D2-E2 silent; D2 to D7 reads ~10 kΩ. Plug the ESP32 in to check it seats over R2, then pull it out again.
6. Wire the solder side, in this order, one wire at a time, ticking off the wire list in the wiring guide. Leave S2, T2, S4, T4, S6 and O14-Q14 empty for now (step 7).
   1. Ground: N2-N12, N12-L14, L14-B14 (along the empty row 13, so it does not lie on the screen-pin stubs), D7-N2. N2-O2 and N12-O12 silent: those neighbours are VIN and 3V3.
   2. 3.3 V: O12-M14, then M14-A14 along row 13. Only one wire on O12, because N12 next to it is GND. O12-N12, M14-L14, A14-B14, O12-N2 and O12-O2 silent.
   3. Screen: A12-C14, G12-D14, H12-E14, L12-F14, K12-G14, F2-H14. These cross the row-13 ground wire: solder at the tip of each stub and keep the iron off the ground wire. Beep each one against GND straight away.
   4. IMU: E12-J14 (SDA), B12-K14 (SCL).
   Strip 2-3 mm, tin the end, keep wires flat against the board, and keep them away from the antenna end (column A). Every screen and IMU wire has to cross the ground and 3.3 V wires in row 13: cross them at right angles, where the pile is lowest, and never lay a wire along the top of another. There are about 4 mm behind the board on its pads; if the crossings stack higher, mount the board on 2.5-3 mm spacers (see stage 4).
7. Outside wires (soldered straight into the board, ~12 cm, labelled with tape). Each one goes through its hole first; then solder the board wire to its end on the solder side:
   - S2 "5V IN" and T2 "GND IN" (go to the MT3608 later), then O2-S2 and N2-T2
   - S4 "AMP 5V", T4 "AMP GND", then S2-S4 and T2-T4
   - O14 "AMP L", P14 "AMP R", Q14 "AMP G", then H2-O14 (D25), I2-P14 (D26) and T4-Q14 (up the empty column T, so it stays off the wires crossing row 13)
   - S6: the SENSE wire from R1, then D2-S6
8. Optional bulk capacitor: if you have an electrolytic of 100-470 µF rated 10 V or more, add it across 5 V and GND near S2/T2, lying on its side (the stripe marks -). It helps against brownouts during WiFi bursts. Skip the small capacitor on the sense node: with 10 kΩ it is not needed, and an electrolytic is too tall to sit under the ESP32.

Check with the ESP32 out of its sockets, multimeter on continuity/resistance:
- Every wire in the list beeps end to end.
- No beep between 5 V and GND, 3.3 V and GND, or 5 V and 3.3 V (measure O2-N2, O12-N12, O2-O12).
- No beep between neighbouring socket holes on either row.
- S6 to ground shows ~10 kΩ (R2).

## Stage 3: Bench test (everything connected, outside the enclosure)

1. Speakers: identify each speaker's pair of wires (~3-4 Ω between them). Solder one speaker to PAM8403 lout + / lout -, the other to rout + / rout -. Neither ever touches GND.
2. PAM8403: AMP 5V to power +, AMP GND to power -, AMP L to input L, AMP R to input R, AMP G to input G. Turn its knob to about a quarter.
3. MT3608: "5V" to the perfboard's 5V IN pigtail, "GND" to GND IN (solder and insulate, or twist and tape for the bench test).
4. SENSE wire from R1 in S6 (soldered in stage 2).
5. Plug the IMU board into J14-M14 (VIN on M14, its body off the board edge), the screen cable onto A14-H14 (VCC, purple, on A14), and the ESP32 into its sockets (antenna end at column A).
6. Insert the cells. The screen should light and boot. If nothing happens, remove a cell immediately and recheck the 5 V wiring.
7. Plug the ESP32's USB into the PC and open the serial monitor (safe while on battery). Expect:
   - `[IMU] ADXL345 Accelerometer detected at 0x53`
   - `[BMP] BMP280 detected at 0x77`
   - `[AUDIO] Internal DAC output started`
   - WiFi connected, `[OTA] Kubi OTA Ready!`
8. Flash the latest firmware and dashboard over WiFi (both are needed: the dashboard gained the volume slider):
   ```powershell
   & "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -d firmware -t upload --upload-port 192.168.10.36
   & "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -d firmware -t uploadfs --upload-port 192.168.10.36
   ```
9. Test:
   - Tap on the Clock face: a short chime from both speakers. Listen for a thump at the start or end, hiss, or buzz, and note it.
   - Dashboard > Sound: move the slider, let go, the cube chimes at the new level. Set the PAM8403 knob so 100 % on the slider is the loudest you would ever want; that is its final position.
   - Shake on the Clock face toggles analog/digital.
   - Plug USB-C into the TP4056 while running: red LED, and the cube keeps running.

## Stage 4: Into the enclosure (after printing)

1. Front: screen into its window (cable towards the inside), speakers into their grille pockets either side, facing out.
2. Battery holder behind the screen, centred, 78 mm side to side. Route the screen cable around it.
3. Perfboard onto its pads on the mid-plate, component side towards the back, row 14 (screen and IMU headers) towards the top on Face 1, away from the TP4056. Glue it a millimetre or two away from the top so the flat IMU clears the top panel (the panels have pockets for it). The pads leave 4 mm behind the board (about 2.5 mm where the zip ties cross, under columns A-C and R-T). If the wires on the back stack higher, put a 2.5-3 mm spacer under each corner, such as the plastic strip from a bit of male header with the pins pulled out: the IMU stays inside its panel pockets up to about 3 mm extra, and the ESP32 still clears the amp knob. It must not be able to move: the IMU on it decides which face is up.
4. Back wall: TP4056 with its USB-C in the slot, MT3608, PAM8403 (knob already set). Tape over the underside of each module wherever it could touch another part or wire.
5. Strain relief: a dab of hot glue where each wire leaves a solder joint, and on the speaker leads.
6. Close it up, power on, and check every face rolls to the right screen.

Then, because the IMU now sits differently than on the breadboard, recalibrate:

```powershell
powershell -ExecutionPolicy Bypass -File firmware/tools/capture_faces.ps1 -Cube 192.168.10.36
```

Paste the output to Claude to update `FaceMap.h`, flash over WiFi, and check that each face is upright, a shake on the Clock face works, and the maze steers the right way.

## After assembly (firmware)

- Battery percentage and a low-battery shutdown at ~3.3 V, now that the divider on GPIO 34 exists (TSK-391).
- Tune DAC_RAMP_IN_MS / DAC_RAMP_OUT_MS if the speakers thump.
- Later, optional parts: a slide switch in the TP4056 OUT+ to MT3608 IN+ wire (on/off), and load sharing so the cells are not held at 4.2 V while it lives plugged in.
