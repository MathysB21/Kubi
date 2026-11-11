// The perfboard plan from docs/Kubi Architecture and Wiring Guide.md, section 5.
// Hole names are the coordinates printed on the board (see scad.ts).
import { NET_COLORS } from "./electronics";
import { parseHole, PITCH } from "./scad";

export type Net = keyof typeof NET_COLORS;

export const NET_NAMES: Record<Net, string> = {
  gnd: "Ground",
  v5: "5 V",
  v33: "3.3 V",
  bat: "Battery",
  spi: "Screen (SPI)",
  bl: "Backlight",
  i2c: "IMU (I2C)",
  audio: "Audio",
  sense: "Battery sense",
  speaker: "Speakers",
};

// ESP32 socket pins, column A to O (antenna end at column A)
export const SOCKET_ROW = { low: 2, high: 12 };
export const SOCKET_LOW = ["EN", "VP", "VN", "D34", "D35", "D32", "D33", "D25", "D26", "D27", "D14", "D12", "D13", "GND", "VIN"];
export const SOCKET_HIGH = ["D23", "D22", "TX0", "RX0", "D21", "D19", "D18", "D5", "TX2", "RX2", "D4", "D2", "D15", "GND", "3V3"];

export interface Pad {
  hole: string;
  label: string;
  kind: "male" | "female" | "pigtail";
  group: string;
}

const COL = "ABCDEFGHIJKLMNOPQRST";

export const PADS: Pad[] = [
  ...["VCC", "GND", "DIN", "CLK", "CS", "DC", "RST", "BL"].map((label, i): Pad => ({ hole: `${COL[i]}14`, label, kind: "male", group: "Screen cable" })),
  // DFRobot 10DOF V2, lying flat with its body off the row-14 edge: VIN lands on M14
  ...["SDA", "SCL", "GND", "VIN"].map((label, i): Pad => ({ hole: `${COL[i + 9]}14`, label, kind: "female", group: "IMU socket" })),
  ...["L", "R", "G"].map((label, i): Pad => ({ hole: `${COL[i + 14]}14`, label, kind: "pigtail", group: "To PAM8403 input" })),
  { hole: "S2", label: "5V in", kind: "pigtail", group: "From MT3608" },
  { hole: "T2", label: "GND in", kind: "pigtail", group: "From MT3608" },
  { hole: "S4", label: "5V amp", kind: "pigtail", group: "To PAM8403 power" },
  { hole: "T4", label: "GND amp", kind: "pigtail", group: "To PAM8403 power" },
  { hole: "S6", label: "BAT sense", kind: "pigtail", group: "From R1 at the battery" },
];

export interface BoardWire {
  id: string;
  from: string;
  to: string;
  net: Net;
  label: string;
  bridge?: boolean; // a solder bridge or bent leg between neighbouring holes
  // Bends between the two ends, as [column 1-20, row 1-14]. Fractions run between
  // the stubs. Without it the wire goes straight from end to end.
  via?: [number, number][];
}

// Solder-side wires, in the order the assembly guide solders them:
// ground, 3.3 V, screen, IMU, then the wires to the pigtail holes (their outside
// wire goes through first, so audio comes after the O14 / P14 pigtails).
export const BOARD_WIRES: BoardWire[] = [
  { id: "g1", from: "N2", to: "N12", net: "gnd", label: "ESP32 GND to GND" },
  { id: "g2", from: "N12", to: "L14", net: "gnd", label: "GND to IMU GND" },
  { id: "g3", from: "L14", to: "B14", via: [[11.3, 13.25], [2.7, 13.25]], net: "gnd", label: "IMU GND to screen GND (along row 13)" },
  { id: "g4", from: "D7", to: "N2", net: "gnd", label: "R2 bottom end to GND" },
  { id: "p1", from: "O12", to: "M14", net: "v33", label: "3V3 to IMU VIN" },
  { id: "p2", from: "M14", to: "A14", via: [[12.4, 12.75], [1.7, 12.75]], net: "v33", label: "IMU VIN to screen VCC (along row 13)" },
  { id: "b1", from: "D2", to: "D3", net: "sense", label: "D34 to R2 top end (R2's own leg, bent over)", bridge: true },
  { id: "s1", from: "A12", to: "C14", net: "spi", label: "D23 to screen DIN" },
  { id: "s2", from: "G12", to: "D14", net: "spi", label: "D18 to screen CLK" },
  { id: "s3", from: "H12", to: "E14", net: "spi", label: "D5 to screen CS" },
  { id: "s4", from: "L12", to: "F14", via: [[11, 13], [7, 13]], net: "spi", label: "D2 to screen DC" },
  { id: "s5", from: "K12", to: "G14", net: "spi", label: "D4 to screen RST" },
  { id: "s6", from: "F2", to: "H14", via: [[6, 10], [7.5, 11], [7.5, 13]], net: "bl", label: "D32 to screen BL" },
  { id: "i1", from: "E12", to: "J14", via: [[6, 12.9], [9, 12.9]], net: "i2c", label: "D21 to IMU SDA" },
  { id: "i2", from: "B12", to: "K14", via: [[3, 13.1], [10, 13.1]], net: "i2c", label: "D22 to IMU SCL" },
  { id: "p3", from: "O2", to: "S2", net: "v5", label: "VIN to 5V in (MT3608)" },
  { id: "p4", from: "S2", to: "S4", net: "v5", label: "5V in to 5V amp" },
  { id: "g5", from: "N2", to: "T2", via: [[15, 1], [19, 1]], net: "gnd", label: "GND to GND in (MT3608)" },
  { id: "g6", from: "T2", to: "T4", net: "gnd", label: "GND in to GND amp" },
  { id: "a1", from: "H2", to: "O14", via: [[15.5, 11], [15.5, 13]], net: "audio", label: "D25 (DAC1) to amp L" },
  { id: "a2", from: "I2", to: "P14", via: [[16, 11]], net: "audio", label: "D26 (DAC2) to amp R" },
  { id: "g7", from: "T4", to: "Q14", via: [[20, 13]], net: "gnd", label: "GND amp to amp input G (up the empty column T, clear of the row-13 wires)" },
  { id: "b2", from: "D2", to: "S6", via: [[5, 3]], net: "sense", label: "D34 to BAT sense pad" },
];

// R2 lies flat on the component side, under the ESP32, between these holes
export const R2 = { from: "D3", to: "D7" };

// Every point a wire passes through, as [column, row]
export function wirePoints(w: BoardWire): [number, number][] {
  const a = parseHole(w.from);
  const b = parseHole(w.to);
  return [[a.col, a.row], ...(w.via ?? []), [b.col, b.row]];
}

// Length to cut in mm: the route, plus strip length and a little slack, rounded up to 5 mm
export function cutLength(w: BoardWire): number {
  const pts = wirePoints(w);
  let pitches = 0;
  for (let i = 1; i < pts.length; i++) pitches += Math.hypot(pts[i][0] - pts[i - 1][0], pts[i][1] - pts[i - 1][1]);
  return Math.ceil((pitches * PITCH + 12) / 5) * 5;
}

// What sits in each occupied hole, so a hole can be named and avoided
export const OCCUPIED: Record<string, string> = Object.fromEntries([
  ...SOCKET_LOW.map((name, i) => [`${COL[i]}${SOCKET_ROW.low}`, name]),
  ...SOCKET_HIGH.map((name, i) => [`${COL[i]}${SOCKET_ROW.high}`, name]),
  ...PADS.map((p) => [p.hole, p.group === "Screen cable" ? `screen ${p.label}` : p.group === "IMU socket" ? `IMU ${p.label}` : p.label]),
  [R2.from, "R2"],
  [R2.to, "R2"],
]);

export const isPigtail = (hole: string) => PADS.some((p) => p.hole === hole && p.kind === "pigtail");

export interface OffBoardWire {
  id: string;
  from: string;
  to: string;
  net: Net;
  note?: string;
}

export const OFFBOARD_WIRES: OffBoardWire[] = [
  { id: "o1", from: "Battery holder +", to: "TP4056 B+", net: "bat", note: "Cells out while soldering. Keep it short." },
  { id: "o2", from: "Battery holder -", to: "TP4056 B-", net: "gnd" },
  { id: "o3", from: "R1 (10 kohm) on the holder + tab", to: "Perfboard S6", net: "sense", note: "R1 at the battery end, fully insulated." },
  { id: "o4", from: "TP4056 OUT+", to: "MT3608 IN+", net: "v5", note: "A power switch goes in this wire later." },
  { id: "o5", from: "TP4056 OUT-", to: "MT3608 IN-", net: "gnd" },
  { id: "o6", from: "MT3608 OUT+", to: "Perfboard S2 (5V in)", net: "v5", note: "Set the MT3608 to 5.0 V first, meter connected the whole time." },
  { id: "o7", from: "MT3608 OUT-", to: "Perfboard T2 (GND in)", net: "gnd" },
  { id: "o8", from: "Perfboard S4 / T4", to: "PAM8403 power + / -", net: "v5" },
  { id: "o9", from: "Perfboard O14 / P14 / Q14", to: "PAM8403 input L / R / G", net: "audio" },
  { id: "o10", from: "PAM8403 lout + / -", to: "Left speaker", net: "speaker", note: "Never to GND." },
  { id: "o11", from: "PAM8403 rout + / -", to: "Right speaker", net: "speaker", note: "Never to GND." },
  { id: "o12", from: "Screen 8-wire cable", to: "Perfboard A14-H14", net: "spi", note: "VCC (purple) on A14, BL (grey) on H14." },
];
