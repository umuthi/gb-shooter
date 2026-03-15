#!/usr/bin/env python3
"""
tile_converter.py — Convert a PNG spritesheet to GBDK 2bpp C tile arrays.

Usage:
    python3 tile_converter.py <image.png> [options]

Options:
    --name NAME       C array/comment prefix  (default: filename stem)
    --cols N          Tiles per row           (default: image_width / 8)
    --rows N          Tile rows to process    (default: image_height / 8)
    --sprite          Treat as sprite tiles   (transparency note added)
    --bg              Treat as BG/Window tiles (default)
    --shade0 RRGGBB   Force a specific color to shade 0 (lightest)
    --shade3 RRGGBB   Force a specific color to shade 3 (darkest)
    --tile-names a,b  Comma-separated label per tile
    --out FILE        Write output to file instead of stdout
    --start-index N   First tile index for comments (default: 0)

Examples:
    # Single 8x8 sprite
    python3 tile_converter.py player.png --name SPR_PLAYER --sprite

    # 4-tile horizontal spritesheet
    python3 tile_converter.py enemies.png --cols 4 --sprite --tile-names enemy_a,enemy_b,enemy_c,enemy_d

    # BG tile sheet, 10 columns
    python3 tile_converter.py tiles.png --cols 10 --bg
"""

import sys
import argparse
from pathlib import Path

try:
    from PIL import Image
except ImportError:
    sys.exit("Pillow is required: pip install Pillow")


# ── Luminance helper ──────────────────────────────────────────────────────

def luminance(r, g, b):
    """Perceptual luminance [0.0–1.0]."""
    return 0.2126 * r / 255 + 0.7152 * g / 255 + 0.0722 * b / 255


def hex_to_rgb(h):
    h = h.lstrip('#')
    return tuple(int(h[i:i+2], 16) for i in (0, 2, 4))


# ── Color quantization ────────────────────────────────────────────────────

def build_shade_map(pixels, force_shade0=None, force_shade3=None):
    """
    Map each unique color to a DMG shade index (0-3).

    Strategy:
      1. Collect all unique (r,g,b) colors from the image.
      2. Sort by luminance.
      3. If ≤4 colors, assign directly: lightest→0, darkest→3.
      4. If >4 colors, cluster into 4 buckets by luminance.

    force_shade0 / force_shade3: (r,g,b) tuples pinned to shade 0 or 3.
    """
    unique = set(pixels)

    # Honor forced pins
    pins = {}
    if force_shade0:
        pins[force_shade0] = 0
    if force_shade3:
        pins[force_shade3] = 3

    # Remove pinned colors from the free set
    free = sorted(unique - set(pins), key=lambda c: luminance(*c), reverse=True)

    # Shade slots: 0=lightest, 3=darkest
    available = [s for s in [0, 1, 2, 3] if s not in pins.values()]

    shade_map = dict(pins)

    if len(free) <= len(available):
        # Direct mapping — lightest free color → lowest available shade
        for color, shade in zip(free, sorted(available)):
            shade_map[color] = shade
    else:
        # Cluster: divide luminance range into len(available) buckets
        lums = [luminance(*c) for c in free]
        lo, hi = min(lums), max(lums)
        span = hi - lo if hi != lo else 1.0
        n = len(available)
        for color in free:
            lum = luminance(*color)
            bucket = int((lum - lo) / span * (n - 0.001))
            shade_map[color] = sorted(available)[bucket]

    return shade_map


# ── 2bpp encoder ──────────────────────────────────────────────────────────

def encode_tile(img, tx, ty, shade_map):
    """
    Encode one 8×8 tile at tile coords (tx, ty) into 16 bytes of 2bpp data.
    Returns list of 16 ints and the set of shade indices used.
    """
    result = []
    shades_used = set()

    for row in range(8):
        p0 = 0
        p1 = 0
        for col in range(8):
            px = tx * 8 + col
            py = ty * 8 + row
            pixel = img.getpixel((px, py))
            r, g, b = pixel[:3]

            # Find nearest color in shade_map
            color = (r, g, b)
            if color not in shade_map:
                # Snap to nearest by luminance
                lum = luminance(r, g, b)
                color = min(shade_map, key=lambda c: abs(luminance(*c) - lum))

            shade = shade_map[color]
            shades_used.add(shade)

            bit = 7 - col
            p0 |= (shade & 1) << bit
            p1 |= ((shade >> 1) & 1) << bit

        result.append(p0)
        result.append(p1)

    return result, shades_used


# ── Output formatter ──────────────────────────────────────────────────────

def format_tile(tile_bytes, name, index, is_sprite):
    label = f"/* {index}: {name}"
    if is_sprite:
        label += " — sprite (color 0 = transparent)"
    label += " */"

    hex_pairs = []
    for i in range(0, 16, 2):
        hex_pairs.append(f"0x{tile_bytes[i]:02X},0x{tile_bytes[i+1]:02X}")

    # 4 pairs per line
    lines = []
    for i in range(0, 8, 4):
        lines.append("    " + ", ".join(hex_pairs[i:i+4]) + ",")

    return label + "\n" + "\n".join(lines)


# ── Main ──────────────────────────────────────────────────────────────────

def main():
    parser = argparse.ArgumentParser(description="PNG → GBDK 2bpp C tile arrays")
    parser.add_argument("image", help="Input PNG file")
    parser.add_argument("--name", default=None, help="C array/comment prefix")
    parser.add_argument("--cols", type=int, default=None)
    parser.add_argument("--rows", type=int, default=None)
    parser.add_argument("--sprite", action="store_true")
    parser.add_argument("--bg", action="store_true")
    parser.add_argument("--shade0", default=None, help="Color (RRGGBB) to force as shade 0")
    parser.add_argument("--shade3", default=None, help="Color (RRGGBB) to force as shade 3")
    parser.add_argument("--tile-names", default=None, help="Comma-separated tile labels")
    parser.add_argument("--out", default=None, help="Output file (default: stdout)")
    parser.add_argument("--start-index", type=int, default=0, help="First tile comment index")
    args = parser.parse_args()

    path = Path(args.image)
    if not path.exists():
        sys.exit(f"File not found: {args.image}")

    img = Image.open(path).convert("RGBA")
    w, h = img.size

    if w % 8 != 0 or h % 8 != 0:
        sys.exit(f"Image size {w}×{h} is not a multiple of 8.")

    cols = args.cols or (w // 8)
    rows = args.rows or (h // 8)
    name = args.name or path.stem
    is_sprite = args.sprite

    tile_names = []
    if args.tile_names:
        tile_names = [t.strip() for t in args.tile_names.split(",")]

    # Convert to RGB for consistent processing
    rgb_img = img.convert("RGB")

    # Collect all pixel colors for shade map
    all_pixels = [rgb_img.getpixel((x, y)) for y in range(h) for x in range(w)]

    force0 = hex_to_rgb(args.shade0) if args.shade0 else None
    force3 = hex_to_rgb(args.shade3) if args.shade3 else None
    shade_map = build_shade_map(all_pixels, force0, force3)

    # Report detected palette
    palette_lines = ["/* Detected palette mapping:"]
    for color, shade in sorted(shade_map.items(), key=lambda x: x[1]):
        r, g, b = color
        lum = luminance(r, g, b)
        palette_lines.append(f"     RGB({r:3d},{g:3d},{b:3d}) lum={lum:.2f} → shade {shade}")
    palette_lines.append("*/")
    palette_report = "\n".join(palette_lines)

    # Encode tiles
    output_parts = [palette_report, ""]
    total = cols * rows
    for i in range(total):
        tx = i % cols
        ty = i // cols
        tile_bytes, shades_used = encode_tile(rgb_img, tx, ty, shade_map)
        tile_label = tile_names[i] if i < len(tile_names) else f"{name}_{i}" if total > 1 else name
        output_parts.append(format_tile(tile_bytes, tile_label, args.start_index + i, is_sprite))
        output_parts.append("")

    # Stats
    output_parts.append(f"/* Total: {total} tile(s), {total * 16} bytes */")

    output = "\n".join(output_parts)

    if args.out:
        Path(args.out).write_text(output)
        print(f"Written to {args.out}")
    else:
        print(output)


if __name__ == "__main__":
    main()
