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
      { id: "p6", text: "Set the MT3608: meter clipped to OUT+ / OUT- the whole time, turn the brass trimmer half a turn at a time until it reads 5.0-5.1 V. Never above 5.5 V. Do not continue until it does.", warn: true },
      { id: "p7", text: "Plug USB-C into the TP4056: red LED = charging, green = full. Unplug." },
      { id: "p8", text: "Cells out. Solder a red ~10 cm wire to MT3608 OUT+ and a black one to OUT-. Label them 5V and GND." },
    ],
    check: "Cells in: MT3608 output reads 5.0-5.1 V. Cells out again before stage 2.",
  },
  {
    id: "perfboard",
    title: "2. Perfboard",
    intro: "Use the board's printed coordinates: printed side up, column A on the left, row 01 at the bottom. Turn it over side to side, so the letters read T to A underneath.",
    steps: [
      { id: "b1", text: "Cut two 15-way strips from the sockets (cut through the 16th, file smooth). The leftover 4-way piece is the IMU socket." },
      { id: "b2", text: "Push the ESP32 into both strips, chip up, drop them into A2-O2 and A12-O12, antenna end (EN / D23) at column A off the edge, EN in A2. Tack A2, O2, A12, O12, check flat, solder the rest. Pull the ESP32 out, trim to 2 mm, check neighbours." },
      { id: "b3", text: "Solder 8 male header pins into A14-H14 (long end up) for the screen cable." },
      { id: "b4", text: "Solder the 4-way female piece into J14-M14. Test-fit the IMU lying flat, body off the row-14 edge: VIN lands on M14." },
      { id: "b5", text: "R2 (10 kohm) flat on the component side from D3 to D7. On the solder side, bend the D3 leg onto D2 (D34) and solder it." },
      { id: "b6", text: "Wire the solder side in order: ground, 3.3 V, screen, IMU, audio. Leave the pigtail holes empty. Tick them off in the Perfboard view." },
      { id: "b7", text: "Outside wires, ~12 cm and labelled, through first: S2 5V IN, T2 GND IN, S4 AMP 5V, T4 AMP GND, O14 AMP L, P14 AMP R, Q14 AMP G, SENSE in S6. Then the board wires to their ends." },
      { id: "b8", text: "Optional: a 100-470 uF / 10 V+ electrolytic across 5 V and GND near S2 / T2, on its side, stripe to GND." },
    ],
    check: "ESP32 out: every wire beeps end to end; no beep between 5 V, 3.3 V and GND (O2-N2, O12-N12, O2-O12) or between neighbouring socket holes; S6 to GND reads ~10 kohm.",
  },
  {
    id: "bench",
    title: "3. Bench test (outside the enclosure)",
    steps: [
      { id: "t1", text: "Solder the speakers to PAM8403 lout + / - and rout + / -. Neither touches GND." },
      { id: "t2", text: "PAM8403: power + / - to AMP 5V / AMP GND, inputs L / R / G to AMP L / R / G. Knob to a quarter." },
      { id: "t3", text: "MT3608 5V / GND to the perfboard's 5V IN / GND IN pigtails." },
      { id: "t4", text: "SENSE wire from R1 in S6 (soldered in stage 2)." },
      { id: "t5", text: "Plug in the IMU (J14-M14, VIN on M14, lying off the board edge), the screen cable (VCC, purple, on A14) and the ESP32 (antenna end at column A)." },
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
      { id: "e2", text: "Mid-plate: battery holder on the front with two zip ties, perfboard on the pads behind, row 14 towards the top, glued 1-2 mm away from the top so the flat IMU clears the panel." },
      { id: "e3", text: "Back: TP4056 on its edge with USB-C flush in the slot, glued well; MT3608 and PAM8403 flat on the plate, the knob clear of the ESP32 and the IMU." },
      { id: "e4", text: "Screw the mid-plate onto the back's legs (4 x M3 from the battery side)." },
      { id: "e5", text: "Tape over module undersides that could touch anything; hot glue at every wire's solder joint for strain relief." },
      { id: "e6", text: "Lower the back assembly into the front body and screw it to the posts (4 x M3)." },
      { id: "e7", text: "Glue the magnets: frame all one pole out, panels the other. Press the panels in." },
      { id: "e8", text: "Recalibrate: powershell -ExecutionPolicy Bypass -File firmware/tools/capture_faces.ps1 -Cube <ip>, paste the output to Claude, flash, check every face." },
    ],
  },
];
