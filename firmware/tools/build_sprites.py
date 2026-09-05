import os
import sys
import re
from pathlib import Path

try:
    from PIL import Image, ImageSequence
except ImportError:
    print("[SPRITE_BUILDER] Warning: Pillow is not installed. Run 'pip install pillow' to enable automatic sprite conversion.")
    sys.exit(0)

# Paths relative to the firmware directory
try:
    SCRIPT_DIR = Path(__file__).resolve().parent
except NameError:
    # PlatformIO SCons context: working directory is firmware/
    SCRIPT_DIR = (Path.cwd() / "tools").resolve()
FIRMWARE_DIR = SCRIPT_DIR.parent
ASSETS_DIR = FIRMWARE_DIR / "assets" / "sprites"
INCLUDE_DIR = FIRMWARE_DIR / "include"
SRC_DIR = FIRMWARE_DIR / "src"

HEADER_FILE = INCLUDE_DIR / "KubiSprites.h"
SOURCE_FILE = SRC_DIR / "KubiSprites.cpp"

TRANSPARENT_COLOR_565 = 0x0000 # Black as mask, or 0xF81F (Magenta)

def to_rgb565(r, g, b, a=255):
    """Convert 8-bit RGBA to 16-bit RGB565 format."""
    if a < 128:
        return TRANSPARENT_COLOR_565
    r5 = (r >> 3) & 0x1F
    g6 = (g >> 2) & 0x3F
    b5 = (b >> 3) & 0x1F
    return (r5 << 11) | (g6 << 5) | b5

def sanitize_name(name):
    """Sanitize filename to valid C++ identifier."""
    return re.sub(r'[^a-zA-Z0-9_]', '_', name.lower())

def process_image(img):
    """Convert an image frame into a list of uint16_t RGB565 values."""
    img = img.convert("RGBA")
    w, h = img.size
    pixels = []
    for p in img.getdata():
        if isinstance(p, (tuple, list)):
            r, g, b = p[0], p[1], p[2]
            a = p[3] if len(p) > 3 else 255
        else:
            r = g = b = p
            a = 255
        pixels.append(to_rgb565(r, g, b, a))
    return w, h, pixels

def build():
    if not ASSETS_DIR.exists():
        ASSETS_DIR.mkdir(parents=True, exist_ok=True)

    animations = []

    # 1. Process GIF animations in assets/sprites/
    for gif_path in sorted(ASSETS_DIR.glob("*.gif")):
        anim_name = sanitize_name(gif_path.stem)
        try:
            with Image.open(gif_path) as im:
                frames_data = []
                delays = []
                w, h = im.size
                for frame in ImageSequence.Iterator(im):
                    duration = frame.info.get("duration", 100)
                    if duration <= 0:
                        duration = 100
                    delays.append(duration)
                    fw, fh, pixels = process_image(frame)
                    frames_data.append(pixels)

                avg_delay = int(sum(delays) / len(delays)) if delays else 100
                animations.append({
                    "name": anim_name,
                    "width": w,
                    "height": h,
                    "frame_count": len(frames_data),
                    "frame_delay": avg_delay,
                    "frames": frames_data
                })
                print(f"[SPRITE_BUILDER] Processed GIF: {gif_path.name} -> {len(frames_data)} frames ({w}x{h}, {avg_delay}ms)")
        except Exception as e:
            print(f"[SPRITE_BUILDER] Error reading {gif_path.name}: {e}")

    # 2. Process Folders containing PNG frames (e.g. assets/sprites/coffee/01.png, 02.png)
    for folder in sorted([d for d in ASSETS_DIR.iterdir() if d.is_dir()]):
        png_files = sorted(folder.glob("*.png"))
        if not png_files:
            continue
        anim_name = sanitize_name(folder.name)
        frames_data = []
        w, h = 0, 0
        for png_path in png_files:
            try:
                with Image.open(png_path) as im:
                    w, h, pixels = process_image(im)
                    frames_data.append(pixels)
            except Exception as e:
                print(f"[SPRITE_BUILDER] Error reading {png_path.name}: {e}")

        if frames_data:
            animations.append({
                "name": anim_name,
                "width": w,
                "height": h,
                "frame_count": len(frames_data),
                "frame_delay": 100, # Default 10fps
                "frames": frames_data
            })
            print(f"[SPRITE_BUILDER] Processed PNG folder: {folder.name}/ -> {len(frames_data)} frames ({w}x{h})")

    # 3. Process standalone PNG images (single-frame sprites)
    for png_path in sorted(ASSETS_DIR.glob("*.png")):
        sprite_name = sanitize_name(png_path.stem)
        try:
            with Image.open(png_path) as im:
                w, h, pixels = process_image(im)
                animations.append({
                    "name": sprite_name,
                    "width": w,
                    "height": h,
                    "frame_count": 1,
                    "frame_delay": 0,
                    "frames": [pixels]
                })
                print(f"[SPRITE_BUILDER] Processed single PNG: {png_path.name} ({w}x{h})")
        except Exception as e:
            print(f"[SPRITE_BUILDER] Error reading {png_path.name}: {e}")

    # 4. Generate C++ Header & Source
    INCLUDE_DIR.mkdir(parents=True, exist_ok=True)
    SRC_DIR.mkdir(parents=True, exist_ok=True)

    header_content = """// Auto-generated by tools/build_sprites.py - DO NOT EDIT MANUALLY
#pragma once
#include <Arduino.h>
#include <pgmspace.h>

struct KubiSpriteAnimation {
    const char* name;
    uint16_t width;
    uint16_t height;
    uint16_t frameCount;
    uint16_t frameDelayMs;
    const uint16_t* const* frames;
};

"""

    source_content = """// Auto-generated by tools/build_sprites.py - DO NOT EDIT MANUALLY
#include "KubiSprites.h"

"""

    if not animations:
        header_content += "// [NOTE] No sprite files found in assets/sprites/. Place your .gif or .png files there.\n"
        header_content += "extern const KubiSpriteAnimation KUBI_SPRITES[];\n"
        header_content += "extern const size_t KUBI_SPRITE_COUNT;\n"
        source_content += "const KubiSpriteAnimation KUBI_SPRITES[] = {};\n"
        source_content += "const size_t KUBI_SPRITE_COUNT = 0;\n"
    else:
        for anim in animations:
            name = anim["name"]
            w = anim["width"]
            h = anim["height"]
            count = anim["frame_count"]

            header_content += f"// Animation: {name} ({w}x{h}, {count} frames)\n"
            header_content += f"extern const KubiSpriteAnimation anim_{name};\n\n"

            # Write frame data arrays to source file
            for f_idx, frame in enumerate(anim["frames"]):
                source_content += f"static const uint16_t frame_{name}_{f_idx}[] PROGMEM = {{\n"
                # Write in rows of 16 values for compact readability
                hex_values = [f"0x{v:04X}" for v in frame]
                for i in range(0, len(hex_values), 16):
                    source_content += "    " + ", ".join(hex_values[i:i+16]) + ",\n"
                source_content += "};\n\n"

            # Write array of frame pointers
            source_content += f"static const uint16_t* const frames_{name}[] PROGMEM = {{\n"
            for f_idx in range(count):
                source_content += f"    frame_{name}_{f_idx},\n"
            source_content += "};\n\n"

            # Write animation struct
            source_content += f"const KubiSpriteAnimation anim_{name} = {{\n"
            source_content += f'    "{name}", {w}, {h}, {count}, {anim["frame_delay"]}, frames_{name}\n'
            source_content += "};\n\n"

        header_content += "extern const KubiSpriteAnimation* const ALL_KUBI_ANIMATIONS[];\n"
        header_content += f"extern const size_t ALL_KUBI_ANIMATION_COUNT;\n"

        source_content += "const KubiSpriteAnimation* const ALL_KUBI_ANIMATIONS[] = {\n"
        for anim in animations:
            source_content += f'    &anim_{anim["name"]},\n'
        source_content += "};\n"
        source_content += f"const size_t ALL_KUBI_ANIMATION_COUNT = {len(animations)};\n"

    with open(HEADER_FILE, "w", encoding="utf-8") as f:
        f.write(header_content)

    with open(SOURCE_FILE, "w", encoding="utf-8") as f:
        f.write(source_content)

    print(f"[SPRITE_BUILDER] Successfully generated {HEADER_FILE.name} and {SOURCE_FILE.name} with {len(animations)} animation(s).")

if __name__ == "__main__":
    build()

# PlatformIO Extra Script Hook
try:
    Import("env")
    print("[SPRITE_BUILDER] Running PlatformIO pre-build sprite conversion...")
    build()
except:
    pass
