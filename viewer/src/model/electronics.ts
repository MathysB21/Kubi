// Electronics placed inside the cube, in the OpenSCAD frame (see scad.ts).
// Positions follow docs/Kubi Architecture and Wiring Guide.md sections 4-5 and
// the enclosure model; sizes are the measured or listed part sizes.
import { holePos, holeX, holeY, PERF, type Vec3 } from "./scad";

export type Shape =
  | { kind: "box"; min: Vec3; max: Vec3; color: string; metal?: boolean }
  | { kind: "cyl"; center: Vec3; r: number; len: number; axis: "x" | "y" | "z"; color: string; metal?: boolean }
  | { kind: "screen"; min: Vec3; max: Vec3 }; // the lit visible area of the display

export interface Component {
  id: string;
  name: string;
  layer: number; // explode factor along the roll axis
  shapes: Shape[];
  label: Vec3; // where its 3D label sits
  size: string;
  where: string;
  connections: string[];
  notes: string[];
}

const box = (min: Vec3, max: Vec3, color: string, metal = false): Shape => ({ kind: "box", min, max, color, metal });
const cyl = (center: Vec3, r: number, len: number, axis: "x" | "y" | "z", color: string, metal = false): Shape =>
  ({ kind: "cyl", center, r, len, axis, color, metal });

const rowB = holeY("B");
const rowK = holeY("K");
const rowN = holeY("N");
const PT = PERF.z1; // perfboard top surface

export const COMPONENTS: Component[] = [
  {
    id: "screen",
    name: "Screen (Waveshare 2\", ST7789)",
    layer: 0.2,
    shapes: [
      { kind: "screen", min: [-15.3, -20.4, 2.95], max: [15.3, 20.4, 3.0] },
      box([-17.5, -24, 3.0], [17.5, 24, 4.6], "#0b0b0d"),
      box([-17.5, -29, 4.6], [17.5, 29, 6.2], "#1e3a8a"),
      box([-6, -27.5, 6.2], [6, -22, 12], "#e5e7eb"),
    ],
    label: [0, 33, 6],
    size: "PCB 58 x 35 mm, glass 48 x 35, visible area 40.8 x 30.6",
    where: "Behind the front window, portrait, held by the four corner brackets. Glass centred on the PCB.",
    connections: ["8-wire cable from its plug to perfboard N1-N8 (VCC, GND, DIN, CLK, CS, DC, RST, BL)"],
    notes: ["Plug position on the back of the PCB is approximate in this view.", "Hot glue at the brackets once it sits square in the window."],
  },
  {
    id: "speakers",
    name: "Speakers (2 x Dell laptop, 4 ohm 1 W)",
    layer: 0.2,
    shapes: [
      box([-35.5, -30, 3], [-20.5, 30, 7], "#27272a"),
      box([20.5, -30, 3], [35.5, 30, 7], "#27272a"),
      box([-30, 27, 7], [-26, 30, 8], "#a1a1aa", true),
      box([26, 27, 7], [30, 30, 8], "#a1a1aa", true),
    ],
    label: [0, -36, 5],
    size: "60 x 15 x 4 mm each",
    where: "Either side of the screen, sound openings against the grilles in the front face. The front never rests on the desk, so they are never muffled.",
    connections: ["Left: PAM8403 lout + / -", "Right: PAM8403 rout + / -", "Neither speaker wire ever goes to GND."],
    notes: ["Keep their plastic housings on: that is their enclosure.", "Hot glue in the brackets."],
  },
  {
    id: "battery",
    name: "2 x 18650 in a bare holder (parallel)",
    layer: 0.45,
    shapes: [
      box([-39, -20.5, 33], [39, 20.5, 35], "#18181b"),
      box([-39, -20.5, 14.2], [-36.5, 20.5, 33], "#18181b"),
      box([36.5, -20.5, 14.2], [39, 20.5, 33], "#18181b"),
      cyl([0, -10.25, 24], 9.25, 65, "x", "#d48bb4"),
      cyl([0, 10.25, 24], 9.25, 65, "x", "#d48bb4"),
      cyl([38.5, -6, 27], 1.2, 6, "x", "#d6b88a"),
    ],
    label: [0, 0, 12],
    size: "Holder 78 x 41 x 21 mm, INR18650-26E cells (3.7 V, 2600 mAh each)",
    where: "Strapped to the front of the mid-plate with two zip ties, centred on the roll axis so the cube rolls evenly.",
    connections: [
      "+ lead to TP4056 B+, - lead to TP4056 B-",
      "R1 (10 kohm) soldered onto the + lead at the holder, then a wire to perfboard F19 (battery sense)",
    ],
    notes: ["Wired in parallel: both cells in, about 3.7 V across the leads (not 7.4 V).", "Cells out whenever you solder on the power side."],
  },
  {
    id: "perfboard",
    name: "Perfboard (40 x 60 mm, 14 x 20 holes)",
    layer: 0.85,
    shapes: [
      box([PERF.x0, PERF.y0, PERF.z0], [PERF.x1, PERF.y1, PERF.z1], "#b08a52"),
      box([holeX(1) - 1.27, rowB - 1.27, PT], [holeX(15) + 1.27, rowB + 1.27, PT + 8.5], "#111"),
      box([holeX(1) - 1.27, rowK - 1.27, PT], [holeX(15) + 1.27, rowK + 1.27, PT + 8.5], "#111"),
      box([holeX(1) - 1.27, rowN - 1.27, PT], [holeX(8) + 1.27, rowN + 1.27, PT + 2.5], "#111"),
      box([holeX(1) - 0.4, rowN - 0.32, PT + 2.5], [holeX(8) + 0.4, rowN + 0.32, PT + 8.5], "#d4a017", true),
      box([holeX(10) - 1.27, rowN - 1.27, PT], [holeX(13) + 1.27, rowN + 1.27, PT + 8.5], "#111"),
      cyl([holeX(4), (holeY("C") + holeY("G")) / 2, PT + 1.2], 1.2, 6.5, "y", "#d6b88a"),
    ],
    label: [0, -26, 47],
    size: "60 x 40 x 1.6 mm",
    where: "On the four pads at the back of the mid-plate, component side facing the back plate.",
    connections: [
      "ESP32 sockets: rows B and K, columns 1-15",
      "Screen cable header N1-N8, IMU socket N10-N13",
      "Amp input pigtails N15-N17, power pigtails B19/B20, amp power D19/D20, battery sense F19",
      "R2 (10 kohm) flat from C4 to G4, C4 bridged to B4",
    ],
    notes: ["Only flat parts under the ESP32: it sits ~10 mm above the board.", "See the Perfboard view for every hole and wire."],
  },
  {
    id: "esp32",
    name: "ESP32 DevKit (30-pin, CH340, USB-C)",
    layer: 1.0,
    shapes: [
      box([-33.1, -11.46, PT + 8.5], [17.9, 16.54, PT + 10.1], "#111827"),
      box([-32.6, -6.46, PT + 10.1], [-26.6, 11.54, PT + 11.1], "#1f2937"),
      box([-26.6, -6.46, PT + 10.1], [-7.1, 11.54, PT + 13.3], "#c7c7cc", true),
      box([13.5, -1.96, PT + 10.1], [18.3, 7.04, PT + 13.3], "#c7c7cc", true),
    ],
    label: [-8, 2.5, 62],
    size: "51 x 28 mm, pin rows 9 holes (22.9 mm) apart",
    where: "Plugged into the two sockets, antenna end (EN / D23) at column 1, overhanging the board edge.",
    connections: [
      "SPI to screen: D23 DIN, D18 CLK, D5 CS, D2 DC, D4 RST, D32 BL",
      "I2C to IMU: D21 SDA, D22 SCL",
      "DAC audio: D25 left, D26 right",
      "Battery sense: D34 (ADC1)",
      "Power: VIN 5 V from the MT3608, 3V3 out to screen and IMU",
    ],
    notes: ["Pull it out of its sockets while soldering the rest.", "Recovery is always possible over its USB-C with the cube open."],
  },
  {
    id: "imu",
    name: "IMU board (ADXL345 + BMP280)",
    layer: 1.0,
    shapes: [
      box([-5.2, rowN - 0.8, PT + 8.5], [12.8, rowN + 0.8, PT + 34.5], "#2563eb"),
      box([0, rowN + 0.8, PT + 20], [5, rowN + 1.8, PT + 25], "#111"),
    ],
    label: [4, -22, 76],
    size: "~26 x 18 mm, standing upright",
    where: "Plugged into the 4-way socket N10-N13 so it is held rigidly: it decides which face is up.",
    connections: ["VCC N10 (3.3 V), GND N11, SDA N12 (D21), SCL N13 (D22)", "ADXL345 at 0x53, BMP280 at 0x77"],
    notes: ["After assembly, rerun firmware/tools/capture_faces.ps1: its position relative to the screen changed."],
  },
  {
    id: "tp4056",
    name: "TP4056 charger (USB-C, protected)",
    layer: 1.35,
    shapes: [
      box([-13.65, -33.25, 59], [13.65, -31.65, 87], "#1e40af"),
      box([-4.47, -31.65, 79.7], [4.47, -28.35, 87], "#c7c7cc", true),
      box([-3, -31.65, 68], [3, -30.4, 73], "#111"),
    ],
    label: [0, -38, 66],
    size: "28 x 27.3 mm",
    where: "Standing on its edge against the back plate, USB-C pushed into the slot until flush with the outside. Glue generously.",
    connections: ["B+ / B- to the battery holder", "OUT+ / OUT- to the MT3608 IN+ / IN-"],
    notes: ["Its USB-C is the cube's charging port.", "Over-discharge cut-off 2.5 V; the firmware will shut down earlier, at ~3.3 V."],
  },
  {
    id: "mt3608",
    name: "MT3608 boost (set to 5.0 V)",
    layer: 1.35,
    shapes: [
      box([-18.5, 21, 85.4], [18.5, 38, 87], "#1d4ed8"),
      cyl([4, 29.5, 82.7], 3.2, 5.4, "z", "#3f3f46"),
      box([-15, 23, 78], [-6, 30, 85.4], "#2563eb"),
    ],
    label: [0, 40, 80],
    size: "37 x 17 x 14 mm",
    where: "Flat on the inside of the back plate, above the perfboard.",
    connections: ["IN+ / IN- from TP4056 OUT+ / OUT-", "OUT+ / OUT- to perfboard B19 (5V in) / B20 (GND in)"],
    notes: ["Measure and set to 5.0-5.1 V before anything is connected to its output."],
  },
  {
    id: "pam8403",
    name: "PAM8403 amp (volume knob inside)",
    layer: 1.35,
    shapes: [
      box([-30, -2, 85.4], [-8, 18, 87], "#166534"),
      box([-24, 3, 78], [-14, 13, 85.4], "#15803d"),
      cyl([-19, 8, 72], 3, 12, "z", "#a3a3a3", true),
      cyl([-11, 14, 82], 3, 6.8, "z", "#1e3a8a"),
    ],
    label: [-19, 8, 68],
    size: "~22 x 20 mm, knob ~20 mm tall",
    where: "Flat on the back plate, off-centre: its knob must not sit over the upright IMU.",
    connections: [
      "power + / - from perfboard D19 / D20",
      "input L / R / G from perfboard N15 / N16 / N17",
      "lout + / - to the left speaker, rout + / - to the right",
    ],
    notes: ["Set the knob once so 100 % on the dashboard's Sound slider is the loudest you want."],
  },
];

// ------------------------------------------------------------------ 3D wires

export interface WirePoint {
  p: Vec3;
  layer: number;
}

export interface Wire3D {
  id: string;
  label: string;
  color: string;
  r?: number;
  points: WirePoint[];
}

const w = (x: number, y: number, z: number, layer: number): WirePoint => ({ p: [x, y, z], layer });

export const NET_COLORS = {
  gnd: "#71717a",
  v5: "#ef4444",
  v33: "#f97316",
  bat: "#dc2626",
  spi: "#3b82f6",
  bl: "#06b6d4",
  i2c: "#a855f7",
  audio: "#22c55e",
  sense: "#eab308",
  speaker: "#14b8a6",
} as const;

const screenPins = [
  { label: "VCC", color: NET_COLORS.v33 },
  { label: "GND", color: NET_COLORS.gnd },
  { label: "DIN", color: NET_COLORS.spi },
  { label: "CLK", color: NET_COLORS.spi },
  { label: "CS", color: NET_COLORS.spi },
  { label: "DC", color: NET_COLORS.spi },
  { label: "RST", color: NET_COLORS.spi },
  { label: "BL", color: NET_COLORS.bl },
];

export const WIRES_3D: Wire3D[] = [
  {
    id: "bat-plus",
    label: "Battery + to TP4056 B+",
    color: NET_COLORS.bat,
    points: [w(38.5, -6, 30, 0.45), w(37.5, -37, 40, 0.6), w(14, -37, 64, 1.2), w(-6, -31.7, 61, 1.35)],
  },
  {
    id: "bat-minus",
    label: "Battery - to TP4056 B-",
    color: "#3f3f46",
    points: [w(38.5, -2, 30, 0.45), w(36.5, -38, 42, 0.6), w(10, -38, 64, 1.2), w(6, -31.7, 61, 1.35)],
  },
  {
    id: "sense",
    label: "R1 at the battery + to F19 (battery sense)",
    color: NET_COLORS.sense,
    points: [w(38.5, -6, 27, 0.45), w(36, -25, 39.5, 0.6), w(28, 3.8, 40.8, 0.85), { p: holePos("F19", PERF.z0), layer: 0.85 }],
  },
  {
    id: "tp-out-plus",
    label: "TP4056 OUT+ to MT3608 IN+",
    color: NET_COLORS.v5,
    points: [w(-11, -31.7, 61, 1.35), w(-36, -30, 70, 1.35), w(-36, 30, 80, 1.35), w(-17, 34, 85.4, 1.35)],
  },
  {
    id: "tp-out-minus",
    label: "TP4056 OUT- to MT3608 IN-",
    color: NET_COLORS.gnd,
    points: [w(11, -31.7, 61, 1.35), w(0, -37, 66, 1.35), w(-35, -33, 72, 1.35), w(-35, 26, 81, 1.35), w(-17, 25, 85.4, 1.35)],
  },
  {
    id: "mt-out-plus",
    label: "MT3608 OUT+ to B19 (5V in)",
    color: NET_COLORS.v5,
    points: [w(17, 34, 85.4, 1.35), w(26, 22, 70, 1.1), { p: holePos("B19", PERF.z1), layer: 0.85 }],
  },
  {
    id: "mt-out-minus",
    label: "MT3608 OUT- to B20 (GND in)",
    color: NET_COLORS.gnd,
    points: [w(17, 25, 85.4, 1.35), w(27.5, 18, 70, 1.1), { p: holePos("B20", PERF.z1), layer: 0.85 }],
  },
  {
    id: "amp-power-plus",
    label: "D19 (5V) to PAM8403 power +",
    color: NET_COLORS.v5,
    points: [{ p: holePos("D19", PERF.z1), layer: 0.85 }, w(20, 20, 62, 1.1), w(-2, 19.5, 78, 1.3), w(-9, 15, 85.4, 1.35)],
  },
  {
    id: "amp-power-minus",
    label: "D20 (GND) to PAM8403 power -",
    color: NET_COLORS.gnd,
    points: [{ p: holePos("D20", PERF.z1), layer: 0.85 }, w(23, 19, 62, 1.1), w(0, 18.5, 78, 1.3), w(-9, 11, 85.4, 1.35)],
  },
  ...(["N15", "N16", "N17"] as const).map((ref, i): Wire3D => ({
    id: `amp-in-${ref}`,
    label: `${ref} to PAM8403 input ${["L", "R", "G"][i]}`,
    color: i === 2 ? NET_COLORS.gnd : NET_COLORS.audio,
    points: [
      { p: holePos(ref, PERF.z1), layer: 0.85 },
      w(14 + i, -22, 52, 0.9),
      w(i * 1.5, -24, 70, 1.2),
      w(-26 + i * 1.2, -6, 82, 1.35),
      w(-29 + i * 3, -1, 85.4, 1.35),
    ],
  })),
  {
    id: "spk-left",
    label: "PAM8403 lout + / - to the left speaker",
    color: NET_COLORS.speaker,
    points: [w(-29, 17.5, 85.4, 1.35), w(-37, 37, 70, 1.1), w(-37, 37, 20, 0.4), w(-28, 31, 6, 0.2)],
  },
  {
    id: "spk-right",
    label: "PAM8403 rout + / - to the right speaker",
    color: NET_COLORS.speaker,
    points: [w(-25, 17.5, 85.4, 1.35), w(-20, 37.5, 72, 1.2), w(37, 37, 60, 0.9), w(37, 37, 20, 0.4), w(28, 31, 6, 0.2)],
  },
  ...screenPins.map((pin, i): Wire3D => {
    const x = holeX(i + 1);
    const xp = -4.4 + i * 1.25;
    return {
      id: `screen-${pin.label}`,
      label: `Screen cable ${pin.label} to N${i + 1}`,
      color: pin.color,
      r: 0.35,
      points: [
        { p: [x, rowN, PT + 8.5], layer: 0.85 },
        w(x * 0.6, -36.5 - i * 0.3, 48, 0.75),
        w(x * 0.3, -36.5 - i * 0.3, 20, 0.4),
        w(xp, -24.75, 12, 0.2),
      ],
    };
  }),
];
