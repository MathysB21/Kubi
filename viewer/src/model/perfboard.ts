// The perfboard plan from docs/Kubi Architecture and Wiring Guide.md, section 5.
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

// ESP32 socket pins, column 1 to 15 (antenna end at column 1)
export const SOCKET_B = ["EN", "VP", "VN", "D34", "D35", "D32", "D33", "D25", "D26", "D27", "D14", "D12", "D13", "GND", "VIN"];
export const SOCKET_K = ["D23", "D22", "TX0", "RX0", "D21", "D19", "D18", "D5", "TX2", "RX2", "D4", "D2", "D15", "GND", "3V3"];

export interface Pad {
  hole: string;
  label: string;
  kind: "male" | "female" | "pigtail";
  group: string;
}

export const PADS: Pad[] = [
  ...["VCC", "GND", "DIN", "CLK", "CS", "DC", "RST", "BL"].map((label, i): Pad => ({ hole: `N${i + 1}`, label, kind: "male", group: "Screen cable" })),
  ...["VCC", "GND", "SDA", "SCL"].map((label, i): Pad => ({ hole: `N${i + 10}`, label, kind: "female", group: "IMU socket" })),
  ...["L", "R", "G"].map((label, i): Pad => ({ hole: `N${i + 15}`, label, kind: "pigtail", group: "To PAM8403 input" })),
  { hole: "B19", label: "5V in", kind: "pigtail", group: "From MT3608" },
  { hole: "B20", label: "GND in", kind: "pigtail", group: "From MT3608" },
  { hole: "D19", label: "5V amp", kind: "pigtail", group: "To PAM8403 power" },
  { hole: "D20", label: "GND amp", kind: "pigtail", group: "To PAM8403 power" },
  { hole: "F19", label: "BAT sense", kind: "pigtail", group: "From R1 at the battery" },
];

export interface BoardWire {
  id: string;
  from: string;
  to: string;
  net: Net;
  label: string;
  bridge?: boolean; // a solder bridge or leg offcut between neighbouring holes
}

// Solder-side wires, in the order the assembly guide solders them:
// ground, 3.3 V, 5 V, screen, IMU, audio, battery sense.
export const BOARD_WIRES: BoardWire[] = [
  { id: "g1", from: "B14", to: "K14", net: "gnd", label: "ESP32 GND to GND" },
  { id: "g2", from: "B14", to: "B20", net: "gnd", label: "GND to GND in (MT3608)" },
  { id: "g3", from: "B20", to: "D20", net: "gnd", label: "GND in to GND amp" },
  { id: "g4", from: "K14", to: "N2", net: "gnd", label: "GND to screen GND" },
  { id: "g5", from: "K14", to: "N11", net: "gnd", label: "GND to IMU GND" },
  { id: "g6", from: "N11", to: "N17", net: "gnd", label: "GND to amp input G" },
  { id: "g7", from: "G4", to: "K14", net: "gnd", label: "R2 bottom end to GND" },
  { id: "p1", from: "K15", to: "N1", net: "v33", label: "3V3 to screen VCC" },
  { id: "p2", from: "K15", to: "N10", net: "v33", label: "3V3 to IMU VCC" },
  { id: "p3", from: "B15", to: "B19", net: "v5", label: "VIN to 5V in (MT3608)" },
  { id: "p4", from: "B19", to: "D19", net: "v5", label: "5V in to 5V amp" },
  { id: "s1", from: "K1", to: "N3", net: "spi", label: "D23 to screen DIN" },
  { id: "s2", from: "K7", to: "N4", net: "spi", label: "D18 to screen CLK" },
  { id: "s3", from: "K8", to: "N5", net: "spi", label: "D5 to screen CS" },
  { id: "s4", from: "K12", to: "N6", net: "spi", label: "D2 to screen DC" },
  { id: "s5", from: "K11", to: "N7", net: "spi", label: "D4 to screen RST" },
  { id: "s6", from: "B6", to: "N8", net: "bl", label: "D32 to screen BL" },
  { id: "i1", from: "K5", to: "N12", net: "i2c", label: "D21 to IMU SDA" },
  { id: "i2", from: "K2", to: "N13", net: "i2c", label: "D22 to IMU SCL" },
  { id: "a1", from: "B8", to: "N15", net: "audio", label: "D25 (DAC1) to amp L" },
  { id: "a2", from: "B9", to: "N16", net: "audio", label: "D26 (DAC2) to amp R" },
  { id: "b1", from: "B4", to: "C4", net: "sense", label: "D34 to R2 top end (solder bridge)", bridge: true },
  { id: "b2", from: "B4", to: "F19", net: "sense", label: "D34 to BAT sense pad" },
];

// R2 lies flat on the component side, between these holes
export const R2 = { from: "C4", to: "G4" };

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
  { id: "o3", from: "R1 (10 kohm) on the holder + lead", to: "Perfboard F19", net: "sense", note: "R1 at the battery end, fully insulated." },
  { id: "o4", from: "TP4056 OUT+", to: "MT3608 IN+", net: "v5", note: "A power switch goes in this wire later." },
  { id: "o5", from: "TP4056 OUT-", to: "MT3608 IN-", net: "gnd" },
  { id: "o6", from: "MT3608 OUT+", to: "Perfboard B19 (5V in)", net: "v5", note: "Set the MT3608 to 5.0 V first." },
  { id: "o7", from: "MT3608 OUT-", to: "Perfboard B20 (GND in)", net: "gnd" },
  { id: "o8", from: "Perfboard D19 / D20", to: "PAM8403 power + / -", net: "v5" },
  { id: "o9", from: "Perfboard N15 / N16 / N17", to: "PAM8403 input L / R / G", net: "audio" },
  { id: "o10", from: "PAM8403 lout + / -", to: "Left speaker", net: "speaker", note: "Never to GND." },
  { id: "o11", from: "PAM8403 rout + / -", to: "Right speaker", net: "speaker", note: "Never to GND." },
  { id: "o12", from: "Screen 8-wire cable", to: "Perfboard N1-N8", net: "spi", note: "VCC on N1." },
];
