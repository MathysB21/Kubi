#!/usr/bin/env python3
"""
slice_spritesheet.py — Spritesheet Slicing & Animation Generator for Project Kubi

Converts multi-row spritesheets (even with different numbers of columns per row)
into individual animated GIF files or folders of PNG frames ready for Kubi's
firmware sprite pipeline (firmware/assets/sprites/ -> KubiSprites.cpp).

Usage Examples:
  # Using the built-in preset for Wilhelm's 4-tier Kubi sheet:
  python slice_spritesheet.py -i my_sheet.png --preset wilhelm

  # Custom row definition (name:cols or name:cols:top_y:bottom_y):
  python slice_spritesheet.py -i sheet.png -r "idle:4,walk:6,wave:4,desk:5"

  # Explicit pixel coordinates with target size and custom frame rate:
  python slice_spritesheet.py -i sheet.png -r "idle:4:0:185,walk:6:185:375" --target-size 64x64 --fps 8

  # Export both animated GIFs and folders of individual PNG frames:
  python slice_spritesheet.py -i sheet.png --preset wilhelm --format both
"""

import os
import sys
import json
import argparse
from pathlib import Path

try:
    from PIL import Image, ImageSequence
except ImportError:
    print("[ERROR] Pillow is required. Please install it with: pip install pillow")
    sys.exit(1)

# Default output directory relative to this script
SCRIPT_DIR = Path(__file__).resolve().parent
FIRMWARE_DIR = SCRIPT_DIR.parent
DEFAULT_OUTPUT_DIR = FIRMWARE_DIR / "assets" / "sprites"

# Built-in presets for common spritesheets
PRESETS = {
    "wilhelm": {
        "description": "Wilhelm 4-tier Kubi Spritesheet (Idle 4 cols, Walk 6 cols, Wave 4 cols, Desk 5 cols)",
        "rows": [
            {"name": "kubi_idle", "cols": 4, "top": 0,   "bottom": 185, "delay": 150},
            {"name": "kubi_walk", "cols": 6, "top": 185, "bottom": 375, "delay": 120},
            {"name": "kubi_wave", "cols": 4, "top": 375, "bottom": 565, "delay": 150},
            {"name": "kubi_desk", "cols": 5, "top": 565, "bottom": 764, "delay": 180},
        ]
    }
}

def parse_rows_arg(rows_str, img_height):
    """
    Parse a comma-separated row string.
    Format: 'name:cols[:top:bottom]'
    Example: 'idle:4,walk:6,wave:4,desk:5'
    """
    row_tokens = [token.strip() for token in rows_str.split(",") if token.strip()]
    num_rows = len(row_tokens)
    parsed = []

    for idx, token in enumerate(row_tokens):
        parts = token.split(":")
        if len(parts) < 2:
            raise ValueError(f"Invalid row format '{token}'. Expected 'name:cols' or 'name:cols:top:bottom'")
        name = parts[0].strip()
        cols = int(parts[1].strip())

        if len(parts) >= 4:
            top = int(parts[2].strip())
            bottom = int(parts[3].strip())
        else:
            # Divide image height evenly across rows
            row_h = img_height / num_rows
            top = int(idx * row_h)
            bottom = int((idx + 1) * row_h)

        parsed.append({
            "name": name,
            "cols": cols,
            "top": top,
            "bottom": bottom,
            "delay": 125
        })

    return parsed

def slice_and_save(img_path, row_configs, output_dir, fmt="gif", scale=1, target_size=None, default_delay=125, crop_padding=None):
    output_dir = Path(output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    with Image.open(img_path) as im:
        img_w, img_h = im.size
        print(f"[INFO] Loaded spritesheet: {img_path} ({img_w}x{img_h})")

        total_animations = 0
        total_frames = 0

        for row_idx, config in enumerate(row_configs):
            name = config.get("name", f"anim_row_{row_idx}")
            cols = config.get("cols", 1)
            top = config.get("top", 0)
            bottom = config.get("bottom", img_h)
            delay = config.get("delay", default_delay)

            # Bounds check
            top = max(0, min(top, img_h - 1))
            bottom = max(top + 1, min(bottom, img_h))
            row_height = bottom - top
            col_width = img_w / cols

            print(f"\n[ROW {row_idx + 1}] '{name}' -> {cols} frames (Y: {top}..{bottom}, height: {row_height}px, cell width: {col_width:.1f}px)")

            frames = []
            for col_idx in range(cols):
                left = int(col_idx * col_width)
                right = int((col_idx + 1) * col_width)

                # Apply optional crop padding: [pad_top, pad_bottom, pad_left, pad_right]
                c_left, c_top, c_right, c_bottom = left, top, right, bottom
                if crop_padding:
                    c_top += crop_padding.get("top", 0)
                    c_bottom -= crop_padding.get("bottom", 0)
                    c_left += crop_padding.get("left", 0)
                    c_right -= crop_padding.get("right", 0)

                frame = im.crop((c_left, c_top, c_right, c_bottom))

                # Nearest-neighbor integer scaling (preserves crisp pixel art)
                if scale > 1:
                    sw = frame.width * scale
                    sh = frame.height * scale
                    frame = frame.resize((sw, sh), Image.NEAREST)

                # Optional target size resize
                if target_size:
                    tw, th = target_size
                    frame = frame.resize((tw, th), Image.NEAREST)

                frames.append(frame)

            if not frames:
                continue

            # 1. Export as animated GIF
            if fmt in ("gif", "both"):
                gif_path = output_dir / f"{name}.gif"
                # Save as looping GIF with frame duration
                frames[0].save(
                    gif_path,
                    save_all=True,
                    append_images=frames[1:],
                    duration=delay,
                    loop=0,
                    optimize=False
                )
                print(f"  -> Generated GIF: {gif_path} ({frames[0].width}x{frames[0].height}, {len(frames)} frames, {delay}ms delay)")

            # 2. Export as individual PNG frames in a subfolder
            if fmt in ("png", "both"):
                png_folder = output_dir / name
                png_folder.mkdir(parents=True, exist_ok=True)
                for f_idx, frame in enumerate(frames):
                    frame_path = png_folder / f"frame_{f_idx:02d}.png"
                    frame.save(frame_path)
                print(f"  -> Generated PNG folder: {png_folder}/ ({len(frames)} frames)")

            total_animations += 1
            total_frames += len(frames)

        print(f"\n[DONE] Successfully processed {total_animations} animation(s) ({total_frames} total frames).")
        print(f"Destination: {output_dir}")

def main():
    parser = argparse.ArgumentParser(
        description="Slice multi-row spritesheets into individual animated GIFs or PNG sequences for Kubi."
    )
    parser.add_argument("-i", "--input", required=True, help="Path to spritesheet image (PNG, JPG, etc.)")
    parser.add_argument("-o", "--output", default=str(DEFAULT_OUTPUT_DIR), help=f"Output directory (default: {DEFAULT_OUTPUT_DIR})")
    parser.add_argument("-r", "--rows", help="Row definitions: 'name:cols[:top:bottom],...' (e.g. 'idle:4,walk:6,wave:4,desk:5')")
    parser.add_argument("-p", "--preset", choices=list(PRESETS.keys()), help="Use a built-in preset configuration")
    parser.add_argument("-c", "--config", help="Path to JSON file containing row configuration array")
    parser.add_argument("--format", choices=["gif", "png", "both"], default="gif", help="Output format: gif (default), png (folder of frames), or both")
    parser.add_argument("--fps", type=float, help="Frames per second (e.g. 8 for 125ms per frame)")
    parser.add_argument("--delay", type=int, default=125, help="Milliseconds per frame (default: 125ms)")
    parser.add_argument("--scale", type=int, default=1, help="Integer upscale factor using nearest-neighbor (e.g. 1, 2, 3)")
    parser.add_argument("-s", "--target-size", help="Target size WIDTHxHEIGHT (e.g. 64x64, 80x80)")
    parser.add_argument("--list-presets", action="store_true", help="List available built-in presets and exit")

    args = parser.parse_args()

    if args.list_presets:
        print("Available presets:")
        for key, p in PRESETS.items():
            print(f"  - {key}: {p['description']}")
        sys.exit(0)

    input_path = Path(args.input)
    if not input_path.exists():
        print(f"[ERROR] Input file not found: {input_path}")
        sys.exit(1)

    with Image.open(input_path) as test_img:
        img_h = test_img.height

    default_delay = int(1000.0 / args.fps) if args.fps else args.delay

    target_size = None
    if args.target_size:
        parts = args.target_size.lower().split("x")
        if len(parts) == 2:
            target_size = (int(parts[0]), int(parts[1]))
        else:
            print("[WARNING] Invalid target-size format. Expected WIDTHxHEIGHT, e.g. 64x64. Ignoring.")

    # Determine row configurations
    if args.config:
        with open(args.config, "r", encoding="utf-8") as f:
            row_configs = json.load(f)
    elif args.preset:
        preset_info = PRESETS[args.preset]
        print(f"[PRESET] Using preset '{args.preset}': {preset_info['description']}")
        row_configs = preset_info["rows"]
    elif args.rows:
        row_configs = parse_rows_arg(args.rows, img_h)
    else:
        print("[ERROR] Please specify either --preset, --rows, or --config.")
        print("Example: --preset wilhelm")
        print("Example: --rows 'idle:4,walk:6,wave:4,desk:5'")
        sys.exit(1)

    slice_and_save(
        img_path=input_path,
        row_configs=row_configs,
        output_dir=args.output,
        fmt=args.format,
        scale=args.scale,
        target_size=target_size,
        default_delay=default_delay
    )

if __name__ == "__main__":
    main()
