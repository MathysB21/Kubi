// Printed parts. The STLs are exported from kubi_enclosure.scad in their
// assembled positions (part = *_asm), so they need no placement here.
import frontUrl from "../../../enclosure/stl/assembly/front.stl?url";
import backUrl from "../../../enclosure/stl/assembly/back.stl?url";
import midUrl from "../../../enclosure/stl/assembly/midplate.stl?url";
import sideUrl from "../../../enclosure/stl/assembly/side.stl?url";

export interface EnclosurePart {
  id: "front" | "midplate" | "back" | "side";
  name: string;
  url: string;
  color: string;
  layer: number; // explode factor along the roll axis
  count: number;
  print: string;
  notes: string[];
}

export const ENCLOSURE_PARTS: EnclosurePart[] = [
  {
    id: "front",
    name: "Front body",
    url: frontUrl,
    color: "#d9d2c3",
    layer: 0,
    count: 1,
    print: "Front face down, brim on",
    notes: [
      "Front face with the screen window (chamfered) and two speaker grilles.",
      "Locating brackets on the inside for the screen module and both speakers.",
      "Four corner posts with ledges the side panels rest on, 16 magnet pockets in the ledges.",
      "Screw blocks on top of the posts take the 4 back screws (M3, 2.6 mm pilot).",
    ],
  },
  {
    id: "midplate",
    name: "Mid-plate",
    url: midUrl,
    color: "#8f897d",
    layer: 0.6,
    count: 1,
    print: "Battery side down",
    notes: [
      "Battery holder in front, strapped with two zip ties through the slots.",
      "Perfboard behind it on four solid 7 mm pads: glue it, or mark through its holes, drill 1.6 mm and use M2 screws.",
      "Four countersunk M3 screws from the battery side into the back's legs.",
      "70 mm square, leaving a 7 mm gap all round for wires front to back.",
    ],
  },
  {
    id: "back",
    name: "Back",
    url: backUrl,
    color: "#c8c0ae",
    layer: 1.5,
    count: 1,
    print: "Back face down, brim on",
    notes: [
      "USB-C slot for the TP4056, with the wall thinned to 1.2 mm round it so the plug seats fully.",
      "Five vent slots above the port: the charger gets warm.",
      "Four legs carry the mid-plate. Back plate holds the TP4056, MT3608 and PAM8403 on the inside.",
      "Screwed to the posts with 4 countersunk M3 screws.",
    ],
  },
  {
    id: "side",
    name: "Side panels",
    url: sideUrl,
    color: "#ebe5d6",
    layer: 0,
    count: 4,
    print: "Outer face down, print 4 (test one first)",
    notes: [
      "Rest on ledges, flush with the frame, 0.25 mm clearance all round.",
      "Four 5 x 2 mm magnets each, facing four in the frame: 32 in total.",
      "Frame magnets all one pole out, panel magnets the other: any panel fits any side.",
    ],
  },
];

// Side panel rotations about the roll axis (the STL is the panel on the +x side)
export const SIDE_ANGLES = [0, 90, 180, 270].map((d) => (d * Math.PI) / 180);
// How far panels move outwards at full explode, in EXPLODE_MM units
export const PANEL_OUT = 0.7;

// Magnet centres for the +x side (rotate for the others). Frame magnets sit in
// the post ledges flush with x = 42, panel magnets in the panel just outside it.
export const MAGNET = { d: 5, h: 2, frameX: 41, panelX: 43, ys: [-31, 31], zs: [16, 74] };
