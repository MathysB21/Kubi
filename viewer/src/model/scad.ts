// Every position in the viewer is in the OpenSCAD model's frame
// (enclosure/kubi_enclosure.scad): cube resting on Face 1, screen at the front,
// x across, y up, z from the front face (0) to the back face (90), in mm.
// <ScadSpace> maps this frame into the three.js scene, so the STLs, the
// electronics and the wires all line up with the printed parts.

export type Vec3 = [number, number, number];

export const S = 90; // cube size
export const H = S / 2;
export const T = 3; // wall thickness

// How far a layer with explode factor 1.0 travels backwards at full explode.
export const EXPLODE_MM = 60;

// Perfboard: 14 x 20 holes on a 60 x 40 mm board, 2.54 mm pitch. Holes are
// named by the coordinates printed on the board: column letter A-T along x,
// row number 1-14 along y, so "O12" is column O, row 12. Seen from the back
// (component side towards the back plate), column A is on the left and row 1
// at the bottom; row 14 holds the screen and IMU headers, towards the top.
export const PITCH = 2.54;
export const COL_LETTERS = "ABCDEFGHIJKLMNOPQRST";
export const COLS = 20;
export const ROWS = 14;
export const PERF = { x0: -30, x1: 30, y0: -20, y1: 20, z0: 42, z1: 43.6 };

// col: 1-20 (A-T), row: 1-14. Fractions are allowed for outlines.
export const holeX = (col: number) => -24.13 + (col - 1) * PITCH;
export const holeY = (row: number) => -16.51 + (row - 1) * PITCH;

export function parseHole(ref: string): { col: number; row: number } {
  return { col: COL_LETTERS.indexOf(ref[0]) + 1, row: Number(ref.slice(1)) };
}

export function holePos(ref: string, z: number): Vec3 {
  const h = parseHole(ref);
  return [holeX(h.col), holeY(h.row), z];
}
