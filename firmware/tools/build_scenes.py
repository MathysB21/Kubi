#!/usr/bin/env python3
"""
build_scenes.py — Converts the scenes into RLE-compressed RGB565 arrays for Kubi.

Takes the full-size art from art/1K/ (art/Scaled Down/ if a 1K file is missing),
box-averages it to 120x160, and reduces each scene to its own 64-colour palette
without dithering. When drawn on screen, each pixel is scaled 2x
nearest-neighbor to fill 240x320. All 10 scenes take about 185 KB of flash.

Kubi is a small part of every picture, so a plain palette for the whole scene
gave his blue away to the background (he came out grey-green, 2026-11-11).
His blues are weighted up when the palette is chosen (KUBI_WEIGHT).
Dithering is off: at 2x it reads as noise and breaks up the RLE runs.
"""

import os
import glob
import re
import colorsys
from pathlib import Path

# Without Pillow, skip regeneration and build with the committed KubiScenes.*.
try:
    from PIL import Image
except ImportError:
    Image = None

try:
    SCRIPT_DIR = Path(__file__).resolve().parent
except NameError:
    # PlatformIO SCons context: working directory is firmware/
    SCRIPT_DIR = (Path.cwd() / "tools").resolve()

FIRMWARE_DIR = SCRIPT_DIR.parent
PROJECT_ROOT = FIRMWARE_DIR.parent
SCALED_DIR = PROJECT_ROOT / "art" / "Scaled Down"
FULL_DIR = PROJECT_ROOT / "art" / "1K"

PALETTE_SIZE = 64   # indices are one byte in the RLE, so up to 256 would fit
KUBI_WEIGHT = 6     # extra copies of Kubi-blue pixels when choosing the palette

def is_kubi_blue(rgb):
    h, s, v = colorsys.rgb_to_hsv(*(c / 255.0 for c in rgb))
    return 0.50 <= h <= 0.60 and s >= 0.35 and v >= 0.45

# ---------------------------------------------------------------------------
# Light animation, drawn by DisplayManager over the static scene. Coordinates
# are in the 120x160 scene. Each frame only touches the pixels that change;
# the scene underneath is restored from a decoded copy in RAM.
#   eyes:     search box (x, y, w, h) around Kubi's eyes; the dark pixels in it
#             blink shut now and then (their bottom row stays as the lid line).
#             None where his eyes are already closed.
#   twinkle:  boxes whose bright pixels glint, the glint colour, and optionally
#             the brightness a pixel needs (0..1) and ms between glints.
#   ripple:   list of (box, hue range, centre): matching pixels (sound waves)
#             light up ring by ring outwards from the centre, then rest.
#   wiggle:   box and hue range of a limb that jiggles 1 px left and right.
#   particles: kind (rise/fall/drift), glyph, the box they live in, the box
#             they are born in, colours, how many at once, ticks between
#             births and speed (1/16 px per 100 ms tick).
# ---------------------------------------------------------------------------
GLYPHS = {
    "dot":      ["X"],
    "tall":     ["X", "X"],
    "z":        ["XXX", ".X.", "XXX"],
    "note":     [".XX", ".X.", "XX.", "XX."],
}
SCENE_ANIM = {
    "Beach eating ice cream": dict(eyes=(52, 90, 30, 12),
        twinkle=([(0, 50, 75, 22)], 0xFFFF)),
    "Birthday": dict(eyes=None,
        twinkle=([(24, 72, 34, 16)], 0xFFE0),
        particles=dict(kind="fall", glyph="tall", area=(45, 22, 62, 50), emit=(45, 22, 62, 3),
                       colors=[0xF800, 0xFFE0, 0x07FF, 0xF81F], max=6, every=4, speed=7)),
    "Christmas": dict(eyes=(75, 127, 15, 9),
        twinkle=([(55, 0, 58, 95), (0, 57, 18, 20)], 0xFFF0),
        particles=dict(kind="fall", glyph="dot", area=(31, 8, 17, 37), emit=(31, 8, 17, 2),
                       colors=[0xFFFF], max=6, every=3, speed=5)),
    "Laying in flower field": dict(eyes=(50, 100, 14, 10),
        particles=dict(kind="drift", glyph="dot", area=(0, 15, 120, 85), emit=(0, 15, 120, 85),
                       colors=[0xFFF5, 0xFFFF], max=6, every=8, speed=3)),
    "Music": dict(eyes=(39, 87, 14, 8),
        ripple=[((57, 86, 12, 28), (0.40, 0.58), (70, 98)),     # cyan waves, left speaker
                ((63, 108, 16, 7), (0.40, 0.58), (70, 98)),
                ((100, 97, 14, 27), (0.03, 0.13), (98, 104))],  # orange waves, right speaker
        wiggle=((52, 75, 10, 12), (0.42, 0.62)),                # the arm in the air
        particles=dict(kind="rise", glyph="note", area=(40, 38, 75, 45), emit=(72, 74, 30, 6),
                       colors=[0x47FF, 0xFFFF], max=3, every=12, speed=4)),
    "Sitting by cafe": dict(eyes=None,  # his eyes are a pixel each here
        particles=dict(kind="rise", glyph="dot", area=(52, 96, 14, 21), emit=(57, 115, 4, 1),
                       colors=[0xDEFB, 0xFFFF], max=5, every=3, speed=4)),
    "Sleeping in bed": dict(eyes=None,
        twinkle=([(85, 18, 35, 37)], 0xFFFF, 0.55, 70),  # stars below the moon, busier
        particles=dict(kind="rise", glyph="z", area=(50, 5, 40, 42), emit=(58, 40, 6, 3),
                       colors=[0xFFF0], max=2, every=22, speed=3)),
    "Watching sunset": dict(eyes=(46, 105, 6, 9),
        twinkle=([(25, 62, 95, 7), (0, 70, 120, 32)], 0xFFE8)),
    "Working from home": dict(eyes=(44, 82, 20, 10)),
    "Working out at gym": dict(eyes=(60, 98, 16, 9),
        particles=dict(kind="fall", glyph="tall", area=(50, 84, 32, 30), emit=(52, 86, 26, 2),
                       colors=[0x9F7F], max=2, every=12, speed=8)),
}
PARTICLE_KINDS = {"none": 0, "rise": 1, "fall": 2, "drift": 3}
MAX_TWINKLE_POINTS = 48

def _hsv(rgb):
    return colorsys.rgb_to_hsv(*(c / 255.0 for c in rgb))

def scene_animation(title, q):
    """Per-scene animation data from the quantized 120x160 image."""
    cfg = SCENE_ANIM.get(title, {})
    rgb = q.convert("RGB")
    W, H = rgb.size
    px = rgb.load()
    anim = {"eyes": [], "lid": 0, "sparks": [], "spark_color": 0xFFFF, "spark_ms": 150, "particles": None,
            "ripple": [], "rings": 0, "wiggle_a": [], "wiggle_b": []}

    box = cfg.get("eyes")
    if box:
        bx, by, bw, bh = box
        dark = [(x, y) for y in range(by, min(by + bh, H)) for x in range(bx, min(bx + bw, W))
                if _hsv(px[x, y])[2] < 0.35]
        bottom = {}
        for x, y in dark:
            bottom[x] = max(bottom.get(x, -1), y)
        anim["eyes"] = [(x, y) for x, y in dark if y != bottom[x]]  # bottom row = closed lid
        # Lid colour: the most common Kubi blue in the box
        counts = {}
        for y in range(by, min(by + bh, H)):
            for x in range(bx, min(bx + bw, W)):
                if is_kubi_blue(px[x, y]):
                    counts[px[x, y]] = counts.get(px[x, y], 0) + 1
        if counts:
            r, g, b = max(counts, key=counts.get)
            anim["lid"] = to_rgb565(r, g, b)
        else:
            anim["eyes"] = []

    tw = cfg.get("twinkle")
    if tw:
        rects, color = tw[0], tw[1]
        threshold = tw[2] if len(tw) > 2 else 0.80
        anim["spark_ms"] = tw[3] if len(tw) > 3 else 150
        cands = []
        for (rx, ry, rw, rh) in rects:
            for y in range(ry, min(ry + rh, H)):
                for x in range(rx, min(rx + rw, W)):
                    if _hsv(px[x, y])[2] >= threshold:
                        cands.append((x, y))
        if len(cands) > MAX_TWINKLE_POINTS:  # spread evenly over the candidates
            step = len(cands) / MAX_TWINKLE_POINTS
            cands = [cands[int(i * step)] for i in range(MAX_TWINKLE_POINTS)]
        anim["sparks"] = cands
        anim["spark_color"] = color

    def hue_match(p, hues):
        h, s_, v = _hsv(p)
        return hues[0] <= h <= hues[1] and s_ >= 0.35 and v >= 0.45

    rip = []
    for (rx, ry, rw, rh), hues, (cx, cy) in cfg.get("ripple", []):
        for y in range(ry, min(ry + rh, H)):
            for x in range(rx, min(rx + rw, W)):
                if hue_match(px[x, y], hues):
                    rip.append((x, y, ((x - cx) ** 2 + (y - cy) ** 2) ** 0.5))
    if rip:
        d0 = min(r[2] for r in rip)
        anim["ripple"] = [(x, y, int((d - d0) / 2)) for x, y, d in rip][:120]
        anim["rings"] = max(r[2] for r in anim["ripple"]) + 1

    wg = cfg.get("wiggle")
    if wg:
        (wx, wy, ww, wh), hues = wg
        idx = q.load()
        mask = {(x, y) for y in range(wy, min(wy + wh, H)) for x in range(wx, min(wx + ww, W))
                if hue_match(px[x, y], (hues[0], hues[1])) or
                   (hues[0] <= _hsv(px[x, y])[0] <= hues[1] and _hsv(px[x, y])[1] >= 0.3 and _hsv(px[x, y])[2] >= 0.25)}
        for key, dx in (("wiggle_a", 1), ("wiggle_b", -1)):
            frame = []
            for (x, y) in sorted(mask | {(mx + dx, my) for mx, my in mask}):
                sx = x - dx
                if 0 <= x < W and 0 <= sx < W and idx[sx, y] != idx[x, y]:
                    frame.append((x, y, idx[sx, y]))  # the arm moved here, or the row stretched over where it was
            anim[key] = frame

    p = cfg.get("particles")
    if p:
        rows = GLYPHS[p["glyph"]]
        mask = 0
        for gy, row in enumerate(rows):
            for gx, ch in enumerate(row):
                if ch == "X":
                    mask |= 1 << (gy * 4 + gx)
        anim["particles"] = dict(p, mask=mask, gw=max(len(r) for r in rows), gh=len(rows))
    return anim

def write_preview(out_dir, var_name, q, anim):
    """KUBI_SCENE_PREVIEW=<dir>: the scene at 3x with blink pixels red, glint
    points yellow, particle area green and birth box blue."""
    from PIL import ImageDraw
    out_dir.mkdir(parents=True, exist_ok=True)
    im = q.convert("RGB").resize((360, 480), Image.NEAREST)
    d = ImageDraw.Draw(im)
    for (x, y) in anim["eyes"]:
        d.rectangle([x * 3, y * 3, x * 3 + 2, y * 3 + 2], fill=(255, 0, 0))
    for (x, y) in anim["sparks"]:
        d.rectangle([x * 3, y * 3, x * 3 + 2, y * 3 + 2], outline=(255, 255, 0))
    for (x, y, r) in anim["ripple"]:
        d.rectangle([x * 3, y * 3, x * 3 + 2, y * 3 + 2], outline=(255, 0, 255))
    for (x, y, c) in anim["wiggle_a"]:
        d.rectangle([x * 3 + 1, y * 3 + 1, x * 3 + 1, y * 3 + 1], fill=(255, 255, 255))
    p = anim["particles"]
    if p:
        ax, ay, aw, ah = p["area"]; ex, ey, ew, eh = p["emit"]
        d.rectangle([ax * 3, ay * 3, (ax + aw) * 3 - 1, (ay + ah) * 3 - 1], outline=(0, 255, 0))
        d.rectangle([ex * 3, ey * 3, (ex + ew) * 3 - 1, (ey + eh) * 3 - 1], outline=(0, 128, 255))
    im.save(out_dir / f"anim_{var_name}.png")

def scene_palette(small):
    """A palette for this scene with Kubi's blues weighted up."""
    px = list(small.getdata())
    train = px + [p for p in px if is_kubi_blue(p)] * KUBI_WEIGHT
    w = small.width
    h = (len(train) + w - 1) // w
    timg = Image.new("RGB", (w, h))
    timg.putdata(train + [px[0]] * (w * h - len(train)))
    return timg.quantize(colors=PALETTE_SIZE, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.NONE)
INCLUDE_DIR = FIRMWARE_DIR / "include"
SRC_DIR = FIRMWARE_DIR / "src"

HEADER_FILE = INCLUDE_DIR / "KubiScenes.h"
SOURCE_FILE = SRC_DIR / "KubiScenes.cpp"

def to_rgb565(r, g, b):
    r5 = (r >> 3) & 0x1F
    g6 = (g >> 2) & 0x3F
    b5 = (b >> 3) & 0x1F
    return (r5 << 11) | (g6 << 5) | b5

def sanitize_name(name):
    clean = re.sub(r'[^a-zA-Z0-9_]', '_', name.lower())
    clean = re.sub(r'_+', '_', clean).strip('_')
    return clean

def build():
    if Image is None:
        print("[SCENE_BUILDER] Warning: Pillow is not installed, using the committed scenes. Run 'pip install pillow' to regenerate them.")
        return
    if not SCALED_DIR.exists():
        print(f"[SCENE_BUILDER] Error: {SCALED_DIR} does not exist!")
        return

    images = sorted(glob.glob(str(SCALED_DIR / "*.*")))
    if not images:
        print(f"[SCENE_BUILDER] No images found in {SCALED_DIR}")
        return

    scenes = []

    print(f"[SCENE_BUILDER] Found {len(images)} placeholder image(s). Processing...")

    for img_path in images:
        path = Path(img_path)
        base_name = path.stem
        # Format friendly title: strip 'Kubi - ' prefix if present
        title = base_name.replace("Kubi - ", "").strip()
        var_name = sanitize_name(title)

        full = FULL_DIR / path.name
        im = Image.open(full if full.exists() else img_path).convert("RGB")
        # Half resolution (120x160) for 2x nearest-neighbor chunky pixel art
        small = im.resize((120, 160), Image.Resampling.BOX)
        q = small.quantize(palette=scene_palette(small), dither=Image.Dither.NONE)
        pal_rgb = q.getpalette()[:PALETTE_SIZE*3]
        pal_565 = [to_rgb565(pal_rgb[i*3], pal_rgb[i*3+1], pal_rgb[i*3+2]) for i in range(PALETTE_SIZE)]
        pixels = list(q.getdata())

        # RLE encode: pairs of (count, palette_index)
        # where count is 1..255, index is 0..PALETTE_SIZE-1
        rle_runs = []
        cur_idx = pixels[0]
        cnt = 1
        for p in pixels[1:]:
            if p == cur_idx and cnt < 255:
                cnt += 1
            else:
                rle_runs.extend([cnt, cur_idx])
                cur_idx = p
                cnt = 1
        rle_runs.extend([cnt, cur_idx])

        size_kb = (len(rle_runs) + len(pal_565) * 2) / 1024.0
        scenes.append({
            "title": title,
            "var_name": var_name,
            "width": 120,
            "height": 160,
            "scale": 2,
            "palette": pal_565,
            "rle": rle_runs,
            "size_kb": size_kb,
            "anim": scene_animation(title, q),
        })
        preview_dir = os.environ.get("KUBI_SCENE_PREVIEW")
        if preview_dir:
            write_preview(Path(preview_dir), var_name, q, scenes[-1]["anim"])
        print(f"  -> Scene '{title}': 120x160 (scale 2x) -> {len(rle_runs)//2} runs ({size_kb:.1f} KB)")

    total_kb = sum(s["size_kb"] for s in scenes)
    print(f"[SCENE_BUILDER] Total compressed size for {len(scenes)} scenes: {total_kb:.1f} KB")

    # Generate KubiScenes.h
    header_content = """// Auto-generated by tools/build_scenes.py - DO NOT EDIT MANUALLY
#pragma once
#include <Arduino.h>
#include <pgmspace.h>

struct KubiAnimPoint { uint8_t x, y; };
struct KubiAnimPixel { uint8_t x, y, c; };

// Light animation over a scene, in scene pixels (see SCENE_ANIM in the script)
struct KubiSceneAnim {
    uint8_t eyeCount;              // blink: these pixels turn lidColor for a moment
    const KubiAnimPoint* eyes;
    uint16_t lidColor;
    uint8_t sparkCount;            // twinkle: one of these glints now and then
    const KubiAnimPoint* sparks;
    uint16_t sparkColor;
    uint8_t sparkEvery10;          // base ms between glints / 10
    uint8_t rippleCount;           // ripple: pixels light up ring by ring (c = ring)
    const KubiAnimPixel* ripple;
    uint8_t rippleRings;
    uint8_t wiggleCountA;          // wiggle: two frames of a limb shifted 1 px (c = palette index)
    const KubiAnimPixel* wiggleA;
    uint8_t wiggleCountB;
    const KubiAnimPixel* wiggleB;
    uint8_t particleKind;          // 0 none, 1 rise, 2 fall, 3 drift
    uint16_t glyphMask;            // 4x4 bitmask, bit (y * 4 + x)
    uint8_t glyphW, glyphH;
    uint8_t maxParticles;          // at once
    uint8_t spawnEvery;            // ticks (100 ms) between births
    uint8_t speed16;               // 1/16 scene pixel per tick
    uint8_t areaX, areaY, areaW, areaH;  // particles live here
    uint8_t emitX, emitY, emitW, emitH;  // and are born here
    uint8_t colorCount;
    uint16_t colors[4];
};

struct KubiScene {
    const char* id;
    const char* title;
    uint16_t width;      // Source width (e.g. 120)
    uint16_t height;     // Source height (e.g. 160)
    uint8_t scale;       // Integer scale factor (e.g. 2 for 240x320)
    uint8_t paletteSize;
    const uint16_t* palette;
    uint32_t rleLength;  // Total bytes in rleData (count, index pairs)
    const uint8_t* rleData;
    const KubiSceneAnim* anim;  // nullptr: static scene
};

"""
    for s in scenes:
        header_content += f"extern const KubiScene scene_{s['var_name']};\n"

    header_content += f"""
extern const KubiScene* const ALL_KUBI_SCENES[];
extern const size_t ALL_KUBI_SCENE_COUNT;
"""

    # Generate KubiScenes.cpp
    source_content = """// Auto-generated by tools/build_scenes.py - DO NOT EDIT MANUALLY
#include "KubiScenes.h"

"""
    for s in scenes:
        vn = s["var_name"]
        # Palette array
        source_content += f"static const uint16_t palette_{vn}[] PROGMEM = {{\n"
        pal_hex = [f"0x{c:04X}" for c in s["palette"]]
        for i in range(0, len(pal_hex), 8):
            source_content += "    " + ", ".join(pal_hex[i:i+8]) + ",\n"
        source_content += "};\n\n"

        # RLE data array
        source_content += f"static const uint8_t rle_{vn}[] PROGMEM = {{\n"
        rle_hex = [f"0x{b:02X}" for b in s["rle"]]
        for i in range(0, len(rle_hex), 16):
            source_content += "    " + ", ".join(rle_hex[i:i+16]) + ",\n"
        source_content += "};\n\n"

        # Animation
        a = s["anim"]
        def points(name, pts):
            if not pts:
                return "nullptr", ""
            body = ", ".join(f"{{{x}, {y}}}" for x, y in pts)
            return name, f"static const KubiAnimPoint {name}[] = {{ {body} }};\n"
        eyes_ref, eyes_def = points(f"eyes_{vn}", a["eyes"])
        sparks_ref, sparks_def = points(f"sparks_{vn}", a["sparks"])
        def pixels(name, pts):
            if not pts:
                return "nullptr", ""
            body = ", ".join(f"{{{x}, {y}, {c}}}" for x, y, c in pts)
            return name, f"static const KubiAnimPixel {name}[] = {{ {body} }};\n"
        rip_ref, rip_def = pixels(f"ripple_{vn}", a["ripple"])
        wa_ref, wa_def = pixels(f"wiggle_a_{vn}", a["wiggle_a"])
        wb_ref, wb_def = pixels(f"wiggle_b_{vn}", a["wiggle_b"])
        source_content += eyes_def + sparks_def + rip_def + wa_def + wb_def
        p = a["particles"]
        if p:
            colors = (p["colors"] + [0, 0, 0, 0])[:4]
            part = (f"{PARTICLE_KINDS[p['kind']]}, 0x{p['mask']:04X}, {p['gw']}, {p['gh']}, "
                    f"{p['max']}, {p['every']}, {p['speed']}, "
                    f"{p['area'][0]}, {p['area'][1]}, {p['area'][2]}, {p['area'][3]}, "
                    f"{p['emit'][0]}, {p['emit'][1]}, {p['emit'][2]}, {p['emit'][3]}, "
                    f"{len(p['colors'])}, {{ {', '.join(f'0x{c:04X}' for c in colors)} }}")
        else:
            part = "0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, { 0, 0, 0, 0 }"
        source_content += (f"static const KubiSceneAnim anim_{vn} = {{ "
                           f"{len(a['eyes'])}, {eyes_ref}, 0x{a['lid']:04X}, "
                           f"{len(a['sparks'])}, {sparks_ref}, 0x{a['spark_color']:04X}, {a['spark_ms'] // 10}, "
                           f"{len(a['ripple'])}, {rip_ref}, {a['rings']}, "
                           f"{len(a['wiggle_a'])}, {wa_ref}, {len(a['wiggle_b'])}, {wb_ref}, "
                           f"{part} }};\n\n")

        # Scene struct
        source_content += f"""const KubiScene scene_{vn} = {{
    "{vn}",
    "{s['title']}",
    {s['width']},
    {s['height']},
    {s['scale']},
    {len(s['palette'])},
    palette_{vn},
    {len(s['rle'])},
    rle_{vn},
    &anim_{vn}
}};

"""

    source_content += "const KubiScene* const ALL_KUBI_SCENES[] = {\n"
    for s in scenes:
        source_content += f"    &scene_{s['var_name']},\n"
    source_content += "};\n\n"
    source_content += f"const size_t ALL_KUBI_SCENE_COUNT = {len(scenes)};\n"

    with open(HEADER_FILE, "w", encoding="utf-8") as f:
        f.write(header_content)

    with open(SOURCE_FILE, "w", encoding="utf-8") as f:
        f.write(source_content)

    print(f"[SCENE_BUILDER] Generated {HEADER_FILE.name} and {SOURCE_FILE.name} successfully!")

if __name__ == "__main__":
    build()

# PlatformIO Extra Script Hook
try:
    Import("env")
    print("[SCENE_BUILDER] Running PlatformIO pre-build scene conversion...")
    build()
except NameError:
    pass
