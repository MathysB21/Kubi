// The assembly order from docs/Kubi Assembly Guide.md, as a checklist.

export interface Stage {
  id: string;
  title: string;
  intro?: string;
  steps: { id: string; text: string; warn?: boolean }[];
  check?: string;
}

export const SAFETY = [
  "Take the cells out of the holder whenever you solder anything on the power side.",
  "Never solder onto an 18650 cell itself: solder to the holder's leads or tabs.",
  "Insulate every joint on the battery side (heat shrink or tape) before the cells go in.",
  "Red = +, black = -. Check every power wire with the multimeter before trusting its colour.",
];

export const STAGES: Stage[] = [
  {
    id: "power",
    title: "1. Power chain (on the bench)",
    steps: [
      { id: "p1", text: "Measure each cell on its own. If they differ by more than 0.1 V, charge the lower one alone first (after step 2)." },
      { id: "p2", text: "Cells out. Solder the holder's + lead to TP4056 B+ and - to B- (~8 cm)." },
      { id: "p3", text: "Solder R1 (10 kohm) onto the holder's + lead right at the holder. Add a ~12 cm wire to its other leg, insulate everything, label it SENSE." },
      { id: "p4", text: "Solder TP4056 OUT+ to MT3608 IN+ and OUT- to IN- (~5 cm). Leave the MT3608 output unconnected." },
      { id: "p5", text: "Insert both cells." },
      { id: "p6", text: "Set the MT3608: multimeter on OUT+ / OUT-, turn the brass trimmer (it can take 10-20 turns) until it reads 5.0-5.1 V. Do not continue until it does.", warn: true },
      { id: "p7", text: "Plug USB-C into the TP4056: red LED = charging, green = full. Unplug." },
      { id: "p8", text: "Cells out. Solder a red ~10 cm wire to MT3608 OUT+ and a black one to OUT-. Label them 5V and GND." },
    ],
    check: "Cells in: MT3608 output reads 5.0-5.1 V. Cells out again before stage 2.",
  },
  {
    id: "perfboard",
    title: "2. Perfboard",
    intro: "Hold the board with the 20-hole side left to right. Mark columns 1-20 along the top and rows A-N down the left, A1 top left.",
    steps: [
      { id: "b1", text: "Cut two 15-way strips from the sockets (cut through the 16th, file smooth) and one 4-way strip for the IMU." },
      { id: "b2", text: "Push the ESP32 into both strips, place them in rows B and K, columns 1-15, antenna end (EN / D23) at column 1 hanging off the edge. Tack the corners, check flat, solder the rest. Pull the ESP32 out." },
      { id: "b3", text: "Solder 8 male header pins into N1-N8 (long end up) for the screen cable." },
      { id: "b4", text: "Solder the 4-way female strip into N10-N13 for the IMU." },
      { id: "b5", text: "R2 (10 kohm) flat on the component side from C4 to G4. On the solder side, bridge C4 to B4 (D34)." },
      { id: "b6", text: "Wire the solder side in order: ground, 3.3 V, 5 V, screen, IMU, audio, battery sense. Tick them off in the Perfboard view." },
      { id: "b7", text: "Pigtails, ~12 cm and labelled: B19 5V IN, B20 GND IN, D19 AMP 5V, D20 AMP GND, N15 AMP L, N16 AMP R, N17 AMP G." },
      { id: "b8", text: "Optional: a 100-470 uF / 10 V+ electrolytic across 5 V and GND near B19 / B20, on its side, stripe to GND." },
    ],
    check: "ESP32 out: every wire beeps end to end; no beep between 5 V, 3.3 V and GND (B15-B14, K15-K14, B15-K15) or between neighbouring socket holes; F19 to GND reads ~10 kohm.",
  },
  {
    id: "bench",
    title: "3. Bench test (outside the enclosure)",
    steps: [
      { id: "t1", text: "Solder the speakers to PAM8403 lout + / - and rout + / -. Neither touches GND." },
      { id: "t2", text: "PAM8403: power + / - to AMP 5V / AMP GND, inputs L / R / G to AMP L / R / G. Knob to a quarter." },
      { id: "t3", text: "MT3608 5V / GND to the perfboard's 5V IN / GND IN pigtails." },
      { id: "t4", text: "SENSE wire from R1 into F19, soldered." },
      { id: "t5", text: "Plug in the IMU (N10-N13, VCC first), the screen cable (VCC on N1) and the ESP32 (antenna end at column 1)." },
      { id: "t6", text: "Insert the cells: the screen lights and boots. If not, a cell out immediately and recheck the 5 V wiring.", warn: true },
      { id: "t7", text: "Serial monitor: ADXL345 at 0x53, BMP280 at 0x77, Internal DAC output started, WiFi, OTA ready." },
      { id: "t8", text: "Flash firmware and dashboard over WiFi (upload, then uploadfs)." },
      { id: "t9", text: "Test: tap chime, dashboard Sound slider (set the PAM knob so 100 % is your max), shake on Clock, USB-C charging while running." },
    ],
  },
  {
    id: "enclosure",
    title: "4. Into the enclosure",
    steps: [
      { id: "e1", text: "Front: screen into its brackets behind the window, speakers into theirs facing the grilles. Hot glue." },
      { id: "e2", text: "Mid-plate: battery holder on the front with two zip ties, perfboard on the pads behind (glue, or drill 1.6 mm for M2)." },
      { id: "e3", text: "Back: TP4056 on its edge with USB-C flush in the slot, glued well; MT3608 and PAM8403 flat on the plate, the knob clear of the IMU." },
      { id: "e4", text: "Screw the mid-plate onto the back's legs (4 x M3 from the battery side)." },
      { id: "e5", text: "Tape over module undersides that could touch anything; hot glue at every wire's solder joint for strain relief." },
      { id: "e6", text: "Lower the back assembly into the front body and screw it to the posts (4 x M3)." },
      { id: "e7", text: "Glue the magnets: frame all one pole out, panels the other. Press the panels in." },
      { id: "e8", text: "Recalibrate: powershell -ExecutionPolicy Bypass -File firmware/tools/capture_faces.ps1 -Cube <ip>, paste the output to Claude, flash, check every face." },
    ],
  },
];
