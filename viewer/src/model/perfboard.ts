// The perfboard plan from docs/Kubi Architecture and Wiring Guide.md, section 5.
// Hole names are the coordinates printed on the board (see scad.ts).
import { NET_COLORS } from "./electronics";

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
}

// Solder-side wires, in the order the assembly guide solders them:
// ground, 3.3 V, screen, IMU, audio, then the wires to the pigtail holes.
export const BOARD_WIRES: BoardWire[] = [
  { id: "g1", from: "N2", to: "N12", net: "gnd", label: "ESP32 GND to GND" },
  { id: "g2", from: "N12", to: "L14", net: "gnd", label: "GND to IMU GND" },
  { id: "g3", from: "L14", to: "B14", net: "gnd", label: "IMU GND to screen GND (along row 13)" },
  { id: "g4", from: "D7", to: "N2", net: "gnd", label: "R2 bottom end to GND" },
  { id: "p1", from: "O12", to: "M14", net: "v33", label: "3V3 to IMU VIN" },
  { id: "p2", from: "M14", to: "A14", net: "v33", label: "IMU VIN to screen VCC (along row 13)" },
  { id: "s1", from: "A12", to: "C14", net: "spi", label: "D23 to screen DIN" },
  { id: "s2", from: "G12", to: "D14", net: "spi", label: "D18 to screen CLK" },
  { id: "s3", from: "H12", to: "E14", net: "spi", label: "D5 to screen CS" },
  { id: "s4", from: "L12", to: "F14", net: "spi", label: "D2 to screen DC" },
  { id: "s5", from: "K12", to: "G14", net: "spi", label: "D4 to screen RST" },
  { id: "s6", from: "F2", to: "H14", net: "bl", label: "D32 to screen BL" },
  { id: "i1", from: "E12", to: "J14", net: "i2c", label: "D21 to IMU SDA" },
  { id: "i2", from: "B12", to: "K14", net: "i2c", label: "D22 to IMU SCL" },
  { id: "a1", from: "H2", to: "O14", net: "audio", label: "D25 (DAC1) to amp L" },
  { id: "a2", from: "I2", to: "P14", net: "audio", label: "D26 (DAC2) to amp R" },
  { id: "b1", from: "D2", to: "D3", net: "sense", label: "D34 to R2 top end (R2's own leg, bent over)", bridge: true },
  { id: "p3", from: "O2", to: "S2", net: "v5", label: "VIN to 5V in (MT3608)" },
  { id: "p4", from: "S2", to: "S4", net: "v5", label: "5V in to 5V amp" },
  { id: "g5", from: "N2", to: "T2", net: "gnd", label: "GND to GND in (MT3608)" },
  { id: "g6", from: "T2", to: "T4", net: "gnd", label: "GND in to GND amp" },
  { id: "g7", from: "L14", to: "Q14", net: "gnd", label: "IMU GND to amp input G" },
  { id: "b2", from: "D2", to: "S6", net: "sense", label: "D34 to BAT sense pad" },
];

// R2 lies flat on the component side, under the ESP32, between these holes
export const R2 = { from: "D3", to: "D7" };

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
