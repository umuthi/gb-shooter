#!/usr/bin/env python3
"""
img_to_gb.py — Convert any image to a Game Boy DMG-compatible 160×96 PNG.

Resizes the input to 160×96 and quantizes to the 4 DMG shades using the
chosen dithering method. Output is a PNG ready for the image-to-tile-converter
agent (or tile_converter.py --bg).

Usage:
    python3 img_to_gb.py <image> [options]

Options:
    --out FILE          Output PNG path (default: <stem>_gb.png)
    --dither MODE       Dithering method: floyd (default), bayer, none
    --bayer-size N      Bayer matrix size: 2, 4 (default), 8
    --fit MODE          How to fit to 160×96: stretch (default), crop, pad
    --preview           Also save a 4× upscaled preview PNG for easy inspection

Examples:
    python3 img_to_gb.py boss_scene.jpg
    python3 img_to_gb.py intro.png --dither bayer --preview
    python3 img_to_gb.py portrait.png --dither none --fit crop
"""

import sys
import argparse
from pathlib import Path

try:
    from PIL import Image
except ImportError:
    sys.exit("Pillow is required: pip install Pillow")

# ── DMG palette ───────────────────────────────────────────────────────────────
# Four shades as sRGB values, lightest to darkest.
# These approximate the original DMG LCD colours.
DMG_SHADES = [
    (0xE0, 0xF0, 0xE0),  # shade 0 — white/lightest
    (0x88, 0xA0, 0x88),  # shade 1 — light gray
    (0x30, 0x50, 0x30),  # shade 2 — dark gray
    (0x08, 0x18, 0x08),  # shade 3 — black/darkest
]

TARGET_W, TARGET_H = 160, 96


# ── Fit helpers ───────────────────────────────────────────────────────────────

def fit_stretch(img):
    return img.resize((TARGET_W, TARGET_H), Image.LANCZOS)


def fit_crop(img):
    """Scale so the image fills 160×96, then centre-crop."""
    src_w, src_h = img.size
    scale = max(TARGET_W / src_w, TARGET_H / src_h)
    new_w = int(src_w * scale)
    new_h = int(src_h * scale)
    img = img.resize((new_w, new_h), Image.LANCZOS)
    left = (new_w - TARGET_W) // 2
    top  = (new_h - TARGET_H) // 2
    return img.crop((left, top, left + TARGET_W, top + TARGET_H))


def fit_pad(img):
    """Scale to fit within 160×96, pad remainder with black."""
    src_w, src_h = img.size
    scale = min(TARGET_W / src_w, TARGET_H / src_h)
    new_w = int(src_w * scale)
    new_h = int(src_h * scale)
    img = img.resize((new_w, new_h), Image.LANCZOS)
    canvas = Image.new("RGB", (TARGET_W, TARGET_H), (0, 0, 0))
    offset_x = (TARGET_W - new_w) // 2
    offset_y = (TARGET_H - new_h) // 2
    canvas.paste(img, (offset_x, offset_y))
    return canvas


# ── Luminance ─────────────────────────────────────────────────────────────────

def lum(r, g, b):
    return 0.2126 * r + 0.7152 * g + 0.0722 * b


# Pre-compute shade luminances once
_SHADE_LUMS = [lum(*s) for s in DMG_SHADES]


def nearest_shade(r, g, b):
    """Return the DMG shade index closest in luminance to (r, g, b)."""
    l = lum(r, g, b)
    return min(range(4), key=lambda i: abs(_SHADE_LUMS[i] - l))


# ── Dithering ─────────────────────────────────────────────────────────────────

def apply_floyd_steinberg(img):
    """Floyd-Steinberg error-diffusion dither to 4 DMG shades."""
    w, h = img.size
    # Work in float luminance per pixel
    pixels = list(img.getdata())
    lums = [lum(r, g, b) for r, g, b in pixels]
    err = lums[:]  # mutable copy

    out_pixels = []
    for y in range(h):
        for x in range(w):
            idx = y * w + x
            old = err[idx]
            # Clamp to [0, 255]
            old = max(0.0, min(255.0, old))
            # Find nearest shade
            shade_idx = min(range(4), key=lambda i: abs(_SHADE_LUMS[i] - old))
            new = _SHADE_LUMS[shade_idx]
            out_pixels.append(DMG_SHADES[shade_idx])
            e = old - new

            # Distribute error to neighbours
            if x + 1 < w:
                err[idx + 1]     += e * 7 / 16
            if y + 1 < h:
                if x > 0:
                    err[idx + w - 1] += e * 3 / 16
                err[idx + w]         += e * 5 / 16
                if x + 1 < w:
                    err[idx + w + 1] += e * 1 / 16

    out = Image.new("RGB", (w, h))
    out.putdata(out_pixels)
    return out


# Bayer matrices (values are thresholds in range [0, 1))
_BAYER = {
    2: [
        [0/4,  2/4],
        [3/4,  1/4],
    ],
    4: [
        [ 0/16,  8/16,  2/16, 10/16],
        [12/16,  4/16, 14/16,  6/16],
        [ 3/16, 11/16,  1/16,  9/16],
        [15/16,  7/16, 13/16,  5/16],
    ],
    8: [
        [ 0/64, 32/64,  8/64, 40/64,  2/64, 34/64, 10/64, 42/64],
        [48/64, 16/64, 56/64, 24/64, 50/64, 18/64, 58/64, 26/64],
        [12/64, 44/64,  4/64, 36/64, 14/64, 46/64,  6/64, 38/64],
        [60/64, 28/64, 52/64, 20/64, 62/64, 30/64, 54/64, 22/64],
        [ 3/64, 35/64, 11/64, 43/64,  1/64, 33/64,  9/64, 41/64],
        [51/64, 19/64, 59/64, 27/64, 49/64, 17/64, 57/64, 25/64],
        [15/64, 47/64,  7/64, 39/64, 13/64, 45/64,  5/64, 37/64],
        [63/64, 31/64, 55/64, 23/64, 61/64, 29/64, 53/64, 21/64],
    ],
}


def apply_bayer(img, size=4):
    """Ordered (Bayer) dither to 4 DMG shades."""
    matrix = _BAYER[size]
    w, h = img.size
    out_pixels = []

    # Shade thresholds: divide [0,255] into 4 bands
    # With Bayer we nudge the luminance by the matrix offset before snapping
    spread = 255 / 4  # spread per shade band

    for y in range(h):
        for x in range(w):
            r, g, b = img.getpixel((x, y))
            l = lum(r, g, b)
            threshold = matrix[y % size][x % size]
            # Adjust luminance by Bayer offset
            l_adj = l + (threshold - 0.5) * spread
            l_adj = max(0.0, min(255.0, l_adj))
            shade_idx = nearest_shade(int(l_adj), int(l_adj), int(l_adj))
            out_pixels.append(DMG_SHADES[shade_idx])

    out = Image.new("RGB", (w, h))
    out.putdata(out_pixels)
    return out


def apply_none(img):
    """Simple nearest-shade quantization with no dithering."""
    out_pixels = []
    for r, g, b in img.getdata():
        out_pixels.append(DMG_SHADES[nearest_shade(r, g, b)])
    out = Image.new("RGB", img.size)
    out.putdata(out_pixels)
    return out


# ── Main ──────────────────────────────────────────────────────────────────────

def main():
    parser = argparse.ArgumentParser(
        description="Convert any image to a 160×96 Game Boy DMG PNG"
    )
    parser.add_argument("image", help="Input image (PNG, JPG, etc.)")
    parser.add_argument("--out", default=None, help="Output PNG path")
    parser.add_argument(
        "--dither", choices=["floyd", "bayer", "none"], default="floyd",
        help="Dithering method (default: floyd)"
    )
    parser.add_argument(
        "--bayer-size", type=int, choices=[2, 4, 8], default=4,
        help="Bayer matrix size (default: 4)"
    )
    parser.add_argument(
        "--fit", choices=["stretch", "crop", "pad"], default="stretch",
        help="How to fit image to 160×96 (default: stretch)"
    )
    parser.add_argument(
        "--preview", action="store_true",
        help="Also save a 4× upscaled preview PNG"
    )
    args = parser.parse_args()

    src = Path(args.image)
    if not src.exists():
        sys.exit(f"File not found: {args.image}")

    out_path = Path(args.out) if args.out else src.with_name(src.stem + "_gb.png")

    # Load and convert to RGB
    img = Image.open(src).convert("RGB")

    # Fit to 160×96
    fit_fn = {"stretch": fit_stretch, "crop": fit_crop, "pad": fit_pad}[args.fit]
    img = fit_fn(img)
    assert img.size == (TARGET_W, TARGET_H)

    # Dither to 4 DMG shades
    if args.dither == "floyd":
        result = apply_floyd_steinberg(img)
    elif args.dither == "bayer":
        result = apply_bayer(img, size=args.bayer_size)
    else:
        result = apply_none(img)

    result.save(out_path)
    print(f"Saved: {out_path}  ({TARGET_W}×{TARGET_H}, 4-shade DMG palette)")

    if args.preview:
        preview_path = out_path.with_name(out_path.stem + "_preview.png")
        preview = result.resize(
            (TARGET_W * 4, TARGET_H * 4), Image.NEAREST
        )
        preview.save(preview_path)
        print(f"Preview: {preview_path}  ({TARGET_W*4}×{TARGET_H*4})")


if __name__ == "__main__":
    main()
