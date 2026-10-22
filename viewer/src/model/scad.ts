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

// Perfboard: 14 x 20 holes on a 60 x 40 mm board, 2.54 mm pitch.
// Columns 1-20 along x, rows A-N along y (A at the top).
export const PITCH = 2.54;
export const ROWS = "ABCDEFGHIJKLMN";
export const COLS = 20;
export const PERF = { x0: -30, x1: 30, y0: -20, y1: 20, z0: 42, z1: 43.6 };

export const holeX = (col: number) => -24.13 + (col - 1) * PITCH;
export const holeY = (row: string) => 16.51 - ROWS.indexOf(row) * PITCH;

export function parseHole(ref: string): { row: string; col: number } {
  return { row: ref[0], col: Number(ref.slice(1)) };
}

export function holePos(ref: string, z: number): Vec3 {
  const h = parseHole(ref);
  return [holeX(h.col), holeY(h.row), z];
}
