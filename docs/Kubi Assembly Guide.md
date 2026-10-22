# Kubi Assembly Guide

Step-by-step soldering and assembly for the 90 mm cube. Hole positions (B4, N1-N8, F19, ...) refer to the perfboard plan in `Kubi Architecture and Wiring Guide.md`, section 5. Work through the stages in order: each one ends with a check, so a mistake is caught before the next stage builds on it.

## Before you start

Tools: soldering iron and solder, flush cutters, wire strippers, multimeter (with continuity beep), a marker, tape or heat shrink, hot glue.

Parts: ESP32 DevKit, Waveshare 2" screen with its 8-wire cable, the ADXL345/BMP280 board, PAM8403, two laptop speakers, bare 2 x 18650 holder (parallel), two INR18650-26E cells, TP4056 USB-C (protected), MT3608, 40 x 60 mm perfboard, female socket strips, 8 male header pins, two 10 kΩ resistors, 26 AWG stranded wire.

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
6. Set the MT3608 to 5 V. Put the multimeter on MT3608 OUT+ / OUT-. Turn the small brass trimmer a few turns and watch the reading; if it climbs, turn the other way. These are multi-turn trimmers, so it can take 10-20 turns before anything moves. Stop at 5.0-5.1 V. Do not continue until it reads that.
7. Plug USB-C into the TP4056: red LED = charging, green = full. Unplug.
8. Remove the cells. Solder a red ~10 cm wire to MT3608 OUT+ and a black one to OUT-. Label them "5V" and "GND".

Check: cells in, MT3608 output reads 5.0-5.1 V, the SENSE wire reads about half the cell voltage to GND after R2 is added in stage 2 (right now it reads the cell voltage through R1, which is expected). Cells out again before stage 2.

## Stage 2: Perfboard

Orientation: hold the board with the 20-hole side left to right. Mark columns 1-20 along the top edge and rows A-N down the left side (A at the top). Hole A1 is top left.

1. Cut two 15-way strips from the female sockets: cut through the 16th socket (it is lost), then file the cut edge smooth.
2. Cut a 4-way female strip for the IMU the same way.
3. Solder the ESP32 sockets using the ESP32 itself as the jig: push the ESP32 into both strips, then place the strips into rows B and K, columns 1-15, with the ESP32's antenna end (the end with EN and D23) at column 1, hanging off the board edge. Turn it over, solder one pin at each corner, check everything sits flat and square, then solder the remaining pins. Pull the ESP32 out before doing anything else: it should not sit through the rest of the soldering.
4. Solder 8 male header pins into N1-N8 (short end through the board, long end up), for the screen cable.
5. Solder the 4-way female strip into N10-N13 for the IMU.
6. R2 (10 kΩ): lay it flat on the component side between the sockets, from C4 to G4, body touching the board. On the solder side, bridge C4 to B4 (the D34 socket pin) with a blob of solder or a leg offcut. G4 joins the ground wire in the next step.
7. Wire the solder side, in this order, one wire at a time, ticking off the wire list in the wiring guide:
   1. Ground: B14, K14, B20, D20, N2, N11, N17 and G4 (R2) all joined. One continuous wire hopping between holes works well.
   2. 3.3 V: K15 to N1 and N10.
   3. 5 V: B15 to B19 and D19.
   4. Screen: K1-N3, K7-N4, K8-N5, K12-N6, K11-N7, B6-N8.
   5. IMU: K5-N12, K2-N13.
   6. Audio: B8-N15, B9-N16.
   7. Battery sense: B4 to F19.
   Strip 2-3 mm, tin the end, keep wires flat against the board, and keep them away from the antenna end (column 1).
8. Pigtails (external wires soldered straight into the board, ~12 cm, labelled with tape):
   - B19 "5V IN" and B20 "GND IN" (go to the MT3608 later)
   - D19 "AMP 5V", D20 "AMP GND"
   - N15 "AMP L", N16 "AMP R", N17 "AMP G"
   - F19 is where the SENSE wire from R1 will land.
9. Optional bulk capacitor: if you have an electrolytic of 100-470 µF rated 10 V or more, add it across 5 V and GND near B19/B20, lying on its side (the stripe marks -). It helps against brownouts during WiFi bursts. Skip the small capacitor on the sense node: with 10 kΩ it is not needed, and an electrolytic is too tall to sit under the ESP32.

Check with the ESP32 out of its sockets, multimeter on continuity/resistance:
- Every wire in the list beeps end to end.
- No beep between 5 V and GND, 3.3 V and GND, or 5 V and 3.3 V (measure B15-B14, K15-K14, B15-K15).
- No beep between neighbouring socket holes on either row.
- F19 to ground shows ~10 kΩ (R2).

## Stage 3: Bench test (everything connected, outside the enclosure)

1. Speakers: identify each speaker's pair of wires (~3-4 Ω between them). Solder one speaker to PAM8403 lout + / lout -, the other to rout + / rout -. Neither ever touches GND.
2. PAM8403: AMP 5V to power +, AMP GND to power -, AMP L to input L, AMP R to input R, AMP G to input G. Turn its knob to about a quarter.
3. MT3608: "5V" to the perfboard's 5V IN pigtail, "GND" to GND IN (solder and insulate, or twist and tape for the bench test).
4. SENSE wire from R1 into F19, soldered.
5. Plug the IMU board into N10-N13 (VCC, GND, SDA, SCL order: check its labels), the screen cable onto N1-N8 (VCC on N1), and the ESP32 into its sockets (antenna end at column 1).
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
3. Perfboard onto its mounts in the middle. It must not be able to move: the IMU on it decides which face is up.
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
