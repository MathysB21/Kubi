// Kubi enclosure: 90 mm rounded cube, printed in PETG.
//
// Parts (set `part`, then F6 render and export STL; each comes out already in
// its print orientation):
//   front    front face + rim + four corner posts, printed face down
//   back     back face + rim + legs for the mid-plate, printed face down
//   midplate battery holder in front, perfboard behind; screws onto the legs
//   side     removable side panel, print 4, outer face down
//
// Frame of reference: cube resting on Face 1 (clock upright), screen towards
// you. x right, y up, z from the front face (z = 0) to the back face (z = S).
// The cube rolls around z: the screen is on the front, the USB-C port on the
// back, and the four removable panels are the faces it rests on.
//
// Units: mm. Everything below the parameter blocks derives from them.

/* [Render] */
part = "assembly"; // [assembly, exploded, front, back, midplate, side]
$fn = 48;

/* [Cube] */
S = 90;          // outer size
R = 6;           // edge and corner radius
t = 3;           // wall and panel thickness
rim = 8;         // depth of the front and back rims (>= R, so the panels stay flat)
post = 10;       // corner post width, from the outer face
lip = 3;         // inward thickness of the ledges the side panels rest on
ledge = 8;       // how far a panel overlaps the post ledges (holds the magnets)
ledge_z = 6;     // how far a panel overlaps the front and back ledges
gap = 0.25;      // fit clearance per side around each removable panel

/* [Magnets] */
mag_d = 5;       // magnet diameter: MEASURE YOURS
mag_h = 2;       // magnet thickness: MEASURE YOURS
mag_fit = 0.3;   // added to the pocket diameter for a press fit (+0.2 on depth)
mag_inset_z = 8; // magnet centres from the panel's front and back edges

/* [Screen: Waveshare 2" ST7789 module] */
view_w = 30.6;   // visible area, portrait
view_h = 40.8;
win_margin = 0.5;// window is this much bigger than the visible area on each side
pcb_w = 35;      // module PCB
pcb_h = 58;
view_dx = 0;     // centre of the visible area relative to the PCB centre: MEASURE
view_dy = 0;     //   (+x right, +y up, with the module as it sits on Face 1)
scr_y = 0;       // window centre height on the front face

/* [Speakers: Dell laptop pair] */
spk_w = 15;
spk_l = 60;
spk_x = 28;      // centre of each speaker from the cube centre (left and right)
grille_hole = 1.6;
grille_pitch = 3.2;
grille_w = 9;
grille_l = 50;

/* [Back: TP4056 USB-C] */
usb_x = 0;
usb_y = -30;
usb_w = 9.4;     // USB-C receptacle 8.94 x 3.26 plus clearance
usb_h = 3.7;
usb_wall = 1.2;  // back wall thinned around the port so the plug can seat fully

/* [Internal carrier] */
mid_z = 35;      // front face of the mid-plate; the battery holder (21 deep) sits in front
mid_t = 3;
mid_size = 70;
leg_xy = 29;     // leg centres from the cube centre
leg_d = 7;
screw_pilot = 2.6;  // M3 self-tapping into PETG
screw_clear = 3.4;
screw_head = 6;     // countersunk M3 head
corner_c = 36;      // back-plate screws, from the cube centre
pb_hole_dx = 55;    // perfboard mounting-hole spacing: MEASURE (40 x 60 board)
pb_hole_dy = 35;
pb_pilot = 1.8;     // M2 self-tapping
pb_stand = 4;

H = S / 2;

// ---------------------------------------------------------------- helpers

module rcube() {
    hull() for (x = [-1, 1], y = [-1, 1], z = [0, 1])
        translate([x * (H - R), y * (H - R), R + z * (S - 2 * R)]) sphere(R);
}

module shell() {
    difference() {
        rcube();
        translate([-(H - t), -(H - t), t]) cube([S - 2 * t, S - 2 * t, S - 2 * t]);
    }
}

module slab(z0, z1) { translate([-S, -S, z0]) cube([2 * S, 2 * S, z1 - z0]); }

module box(x0, x1, y0, y1, z0, z1) { translate([x0, y0, z0]) cube([x1 - x0, y1 - y0, z1 - z0]); }

// Children placed once per side face: +x, +y, -x, -y
module each_side() { for (a = [0, 90, 180, 270]) rotate([0, 0, a]) children(); }

// Children placed once per corner (mirrored into all four quadrants)
module each_corner() { for (sx = [-1, 1], sy = [-1, 1]) scale([sx, sy, 1]) children(); }

// Rounded rectangle. r is capped below half the short side: at exactly half,
// the inner square has zero width and offset() returns nothing.
module rrect(w, h, r) {
    rr = min(r, min(w, h) / 2 - 0.01);
    offset(rr) square([w - 2 * rr, h - 2 * rr], center = true);
}

// L-shaped brackets at the four corners of a w x h rectangle (locates a part)
module corner_brackets(w, h, len, th, ht) {
    each_corner() translate([w / 2, h / 2, 0]) {
        translate([0, -len, 0]) cube([th, len + th, ht]);
        translate([-len, 0, 0]) cube([len + th, th, ht]);
    }
}

module magnet_pocket() { cylinder(d = mag_d + mag_fit, h = mag_h + 0.2); }

// ---------------------------------------------------------------- front body

pcb_cx = -view_dx;
pcb_cy = scr_y - view_dy;

module posts_region() {
    each_corner() box(H - post, H + 1, H - post, H + 1, rim - 0.01, S - rim);
}

module screen_window() {
    w = view_w + 2 * win_margin;
    h = view_h + 2 * win_margin;
    translate([0, scr_y, 0]) hull() {
        translate([0, 0, -0.01]) linear_extrude(0.01) rrect(w + 2, h + 2, 1.5); // 1 mm chamfer at the face
        translate([0, 0, 1]) linear_extrude(t) rrect(w, h, 0.8);
    }
}

module grilles() {
    nx = floor(grille_w / grille_pitch) + 1;
    ny = floor(grille_l / grille_pitch) + 1;
    for (sx = [-1, 1], i = [0 : nx - 1], j = [0 : ny - 1])
        translate([sx * spk_x + (i - (nx - 1) / 2) * grille_pitch,
                   (j - (ny - 1) / 2) * grille_pitch, -1])
            cylinder(d = grille_hole, h = t + 2, $fn = 12);
}

module front_body() {
    difference() {
        union() {
            intersection() {
                shell();
                union() { slab(-1, rim); posts_region(); }
            }
            each_side() {
                // ledge along the front edge of each side opening
                box(H - t - lip, H - t, -(H - t), H - t, t, rim + ledge_z);
                // ledges along the posts, the full length of the opening
                for (sy = [-1, 1]) scale([1, sy, 1])
                    box(H - t - lip, H - t, H - post - ledge, H - t, t, S - rim);
            }
            // screw blocks at the top of each post, with a 45 degree underside
            each_corner() hull() {
                box(corner_c - 3, H - t, corner_c - 3, H - t, S - rim - 12, S - rim);
                box(H - t - lip, H - t, H - t - lip, H - t, S - rim - 19, S - rim - 18);
            }
            // locating brackets for the screen module and the speakers
            translate([pcb_cx, pcb_cy, t]) corner_brackets(pcb_w + 0.6, pcb_h + 0.6, 6, 1.2, 3);
            for (sx = [-1, 1]) translate([sx * spk_x, 0, t])
                corner_brackets(spk_w + 0.6, spk_l + 0.6, 6, 1.2, 4);
        }
        screen_window();
        grilles();
        // magnet pockets in the post ledges, open towards the panels
        each_side() for (sy = [-1, 1], z = [rim + mag_inset_z, S - rim - mag_inset_z])
            scale([1, sy, 1]) translate([H - t + 0.01, H - post - ledge / 2, z])
                rotate([0, -90, 0]) magnet_pocket();
        // pilot holes for the back screws
        each_corner() translate([corner_c, corner_c, S - rim - 11]) cylinder(d = screw_pilot, h = 12);
    }
}

// ---------------------------------------------------------------- back body

module usb_slot() {
    // through-slot for the receptacle, with the wall thinned around it from inside
    translate([usb_x, usb_y, S - t - 1]) linear_extrude(t + 2)
        hull() for (sx = [-1, 1]) translate([sx * (usb_w - usb_h) / 2, 0]) circle(d = usb_h);
    translate([usb_x, usb_y, S - t - 1]) linear_extrude(1 + t - usb_wall)
        rrect(usb_w + 7, usb_h + 6, 1);
}

module vents() {
    for (x = [-16, -8, 0, 8, 16])
        translate([x, -17, S - t - 1]) linear_extrude(t + 2) rrect(1.6, 14, 0.8);
}

module back_body() {
    difference() {
        union() {
            intersection() { shell(); slab(S - rim, S + 1); }
            // ledges along the back edge of each side opening, between the post ledges
            each_side() box(H - t - lip, H - t, -(H - post - ledge) + 0.3, (H - post - ledge) - 0.3,
                            S - rim - ledge_z, S - t);
            // corner blocks the back screws pass through
            each_corner() box(corner_c - 3, H - t, corner_c - 3, H - t, S - rim, S - t);
            // legs carrying the mid-plate
            each_corner() translate([leg_xy, leg_xy, mid_z + mid_t])
                cylinder(d = leg_d, h = S - t - (mid_z + mid_t) + 0.01);
        }
        // back screws, countersunk from outside
        each_corner() translate([corner_c, corner_c, S - rim - 1]) {
            cylinder(d = screw_clear, h = rim + 2);
            translate([0, 0, rim + 1 - 1.8]) cylinder(d1 = screw_clear, d2 = screw_head, h = 1.81);
        }
        each_corner() translate([leg_xy, leg_xy, mid_z + mid_t - 0.01]) cylinder(d = screw_pilot, h = 10);
        usb_slot();
        vents();
    }
}

// ---------------------------------------------------------------- mid-plate

module midplate() {
    difference() {
        union() {
            translate([0, 0, mid_z]) linear_extrude(mid_t) rrect(mid_size, mid_size, 4);
            // perfboard standoffs on the back
            each_corner() translate([pb_hole_dx / 2, pb_hole_dy / 2, mid_z + mid_t])
                cylinder(d = 5, h = pb_stand);
        }
        // screws into the legs, countersunk on the battery side
        each_corner() translate([leg_xy, leg_xy, mid_z - 0.01]) {
            cylinder(d = screw_clear, h = mid_t + 1);
            cylinder(d1 = screw_head, d2 = screw_clear, h = 1.8);
        }
        each_corner() translate([pb_hole_dx / 2, pb_hole_dy / 2, mid_z + mid_t - 0.01])
            cylinder(d = pb_pilot, h = pb_stand + 0.02);
        // two zip-tie straps around the battery holder (41 mm tall, centred)
        for (sx = [-1, 1], sy = [-1, 1])
            translate([sx * 22, sy * 23.6, mid_z + mid_t / 2]) cube([4.5, 2.2, mid_t + 2], center = true);
    }
}

// ---------------------------------------------------------------- side panel

// Panel coordinates: outer face on z = 0 (the print bed), x across the
// panel (world y), y along it (world z).
panel_w = 2 * (H - post) - 2 * gap;
panel_l = S - 2 * rim - 2 * gap;

module side_panel() {
    difference() {
        hull() { // 0.6 mm chamfer round the outer face
            translate([0, 0, 0]) linear_extrude(0.6) rrect(panel_w - 1.2, panel_l - 1.2, 0.5);
            translate([0, 0, 0.6]) linear_extrude(t - 0.6) rrect(panel_w, panel_l, 0.5);
        }
        for (sx = [-1, 1], sy = [-1, 1])
            translate([sx * (H - post - ledge / 2), sy * (panel_l / 2 - (mag_inset_z - gap)), t - mag_h - 0.2])
                magnet_pocket();
    }
}

// Panel placed on the +x face of the cube
module side_panel_placed() {
    multmatrix([[0, 0, -1, H], [1, 0, 0, 0], [0, 1, 0, S / 2], [0, 0, 0, 1]]) side_panel();
}

// ---------------------------------------------------------------- output

module assembly(e) {
    color("#d8d2c4") front_body();
    color("#c9c2b2") translate([0, 0, e]) back_body();
    color("#8f8a80") translate([0, 0, e / 2]) midplate();
    color("#e8e2d4") each_side() translate([e, 0, 0]) side_panel_placed();
}

if (part == "assembly") assembly(0);
else if (part == "exploded") assembly(40);
else if (part == "front") front_body();
else if (part == "back") translate([0, 0, S]) rotate([180, 0, 0]) back_body();
else if (part == "midplate") translate([0, 0, -mid_z]) midplate();
else if (part == "side") side_panel();
