#!/usr/bin/env python3
"""
rom_previewer.py — Regenerate GB Shooter sprite preview PNGs by parsing
the current tiles.c, then diff against the existing sprites_preview/ images.

Usage:
    python3 tools/rom_previewer.py [--project-root DIR] [--out DIR] [--quiet]

Options:
    --project-root   Path to gb-shooter project root  (default: script's parent)
    --out            Output directory for PNGs         (default: <root>/sprites_preview)
    --quiet          Suppress per-tile progress output

Exit codes:
    0   No visual changes detected
    1   One or more preview images changed
    2   Error (missing files, parse failure, etc.)
"""

import sys
import re
import os
import argparse
import hashlib
from pathlib import Path

try:
    from PIL import Image, ImageDraw
except ImportError:
    sys.exit("Pillow is required: pip install Pillow")

import random

# ── Constants ─────────────────────────────────────────────────────────────

SCALE = 6

WHITE  = (255, 255, 255)
LGRAY  = (170, 170, 170)
DGRAY  = ( 85,  85,  85)
BLACK  = (  0,   0,   0)

SHADES = [WHITE, LGRAY, DGRAY, BLACK]

def decode_palette(reg):
    return [SHADES[(reg >> (i * 2)) & 3] for i in range(4)]

BGP  = decode_palette(0xE4)
OBP0 = decode_palette(0xE8)
OBP1 = decode_palette(0xFC)

# ── Parse tiles.c ─────────────────────────────────────────────────────────

def parse_tile_array(src, array_name):
    """
    Extract hex byte values from a C array like:
        const uint8_t name[] = { 0xAB,0x00, ... };
    Returns bytes object.
    """
    pattern = rf'const uint8_t\s+{re.escape(array_name)}\s*\[\s*\]\s*=\s*\{{([^}}]+)\}}'
    m = re.search(pattern, src, re.DOTALL)
    if not m:
        return None
    body = m.group(1)
    hex_vals = re.findall(r'0x([0-9A-Fa-f]{2})', body)
    return bytes(int(h, 16) for h in hex_vals)


def parse_tile_names(header_src, prefix):
    """
    Extract tile name→index mapping from defines like:
        #define TILE_LETTER_A  16
    """
    names = {}
    for m in re.finditer(rf'#define\s+({re.escape(prefix)}\w+)\s+(\d+)', header_src):
        names[int(m.group(2))] = m.group(1)
    return names


def parse_counts(header_src):
    bg_m  = re.search(r'#define\s+BG_TILES_COUNT\s+(\d+)',  header_src)
    spr_m = re.search(r'#define\s+SPR_TILES_COUNT\s+(\d+)', header_src)
    bg_count  = int(bg_m.group(1))  if bg_m  else 0
    spr_count = int(spr_m.group(1)) if spr_m else 0
    return bg_count, spr_count


# ── 2bpp decoder ──────────────────────────────────────────────────────────

def decode_tile(raw, offset):
    pixels = []
    for row in range(8):
        p0 = raw[offset + row * 2]
        p1 = raw[offset + row * 2 + 1]
        row_px = []
        for bit in range(7, -1, -1):
            row_px.append(((p1 >> bit) & 1) << 1 | ((p0 >> bit) & 1))
        pixels.append(row_px)
    return pixels


def draw_tile(img, tx, ty, tile_pixels, palette, transparent=False):
    for row in range(8):
        for col in range(8):
            cidx = tile_pixels[row][col]
            if transparent and cidx == 0:
                continue
            color = palette[cidx]
            px = (tx * 8 + col) * SCALE
            py = (ty * 8 + row) * SCALE
            for dy in range(SCALE):
                for dx in range(SCALE):
                    img.putpixel((px + dx, py + dy), color)


# ── Renderers ─────────────────────────────────────────────────────────────

TILE_PAD = 2
LABEL_H  = 14

def tile_cell_size():
    return (8 + TILE_PAD * 2) * SCALE, (8 + TILE_PAD * 2) * SCALE + LABEL_H


def render_sprite_sheet(spr_raw, spr_count, spr_names):
    COLS = 7
    ROWS = (spr_count + COLS - 1) // COLS
    cw, ch = tile_cell_size()
    img = Image.new("RGB", (COLS * cw, ROWS * ch), LGRAY)
    draw = ImageDraw.Draw(img)

    palettes = [OBP0] + [OBP1] * (spr_count - 1)
    palettes[0] = OBP0   # player
    for i in range(1, min(spr_count, 9)):
        palettes[i] = OBP1
    for i in range(9, spr_count):
        palettes[i] = OBP1  # boss tiles

    # Use OBP0 for index 0 (player) and 6 (heart)
    if spr_count > 6:
        palettes[6] = OBP0

    for i in range(spr_count):
        col = i % COLS
        row = i // COLS
        px_x = col * cw
        px_y = row * ch
        draw.rectangle([px_x, px_y, px_x + cw - 1, px_y + ch - 1], fill=WHITE)

        # Checkerboard background for transparent areas
        for sy in range(8):
            for sx in range(8):
                bx = px_x + (TILE_PAD + sx) * SCALE
                by = px_y + LABEL_H + TILE_PAD * SCALE + sy * SCALE
                checker = LGRAY if (sx + sy) % 2 == 0 else WHITE
                draw.rectangle([bx, by, bx + SCALE - 1, by + SCALE - 1], fill=checker)

        tile_pixels = decode_tile(spr_raw, i * 16)
        pal = palettes[i] if i < len(palettes) else OBP1

        for sy in range(8):
            for sx in range(8):
                cidx = tile_pixels[sy][sx]
                if cidx == 0:
                    continue
                color = pal[cidx]
                bx = px_x + (TILE_PAD + sx) * SCALE
                by = px_y + LABEL_H + TILE_PAD * SCALE + sy * SCALE
                draw.rectangle([bx, by, bx + SCALE - 1, by + SCALE - 1], fill=color)

        name = spr_names.get(i, f"spr_{i}")
        draw.text((px_x + 3, px_y + 2), name, fill=BLACK)

    # Boss composite
    if spr_count >= 13:
        boss_indices = [9, 10, 11, 12]
        col_start = 4
        row_start = (spr_count // COLS)
        px_x = col_start * cw
        px_y = row_start * ch
        if px_y + ch <= img.size[1]:
            draw.rectangle([px_x, px_y, px_x + cw * 2 - 1, px_y + ch - 1], fill=WHITE)
            draw.text((px_x + 3, px_y + 2), "Boss 16x16", fill=BLACK)
            for qi, qidx in enumerate(boss_indices):
                if qidx >= spr_count:
                    continue
                qrow, qcol = divmod(qi, 2)
                tile_pixels = decode_tile(spr_raw, qidx * 16)
                for sy in range(8):
                    for sx in range(8):
                        cidx = tile_pixels[sy][sx]
                        if cidx == 0:
                            continue
                        bx = px_x + TILE_PAD * SCALE + (qcol * 8 + sx) * SCALE
                        by = px_y + LABEL_H + TILE_PAD * SCALE + (qrow * 8 + sy) * SCALE
                        if bx < img.size[0] and by < img.size[1]:
                            draw.rectangle([bx, by, bx + SCALE - 1, by + SCALE - 1], fill=OBP1[cidx])

    return img


def render_bg_sheet(bg_raw, bg_count, bg_names):
    COLS = 10
    ROWS = (bg_count + COLS - 1) // COLS
    cw, ch = tile_cell_size()
    img = Image.new("RGB", (COLS * cw, ROWS * ch), LGRAY)
    draw = ImageDraw.Draw(img)

    for i in range(bg_count):
        col = i % COLS
        row = i // COLS
        px_x = col * cw
        px_y = row * ch
        draw.rectangle([px_x, px_y, px_x + cw - 1, px_y + ch - 1], fill=WHITE)

        tile_pixels = decode_tile(bg_raw, i * 16)
        for sy in range(8):
            for sx in range(8):
                cidx = tile_pixels[sy][sx]
                color = BGP[cidx]
                bx = px_x + (TILE_PAD + sx) * SCALE
                by = px_y + LABEL_H + TILE_PAD * SCALE + sy * SCALE
                draw.rectangle([bx, by, bx + SCALE - 1, by + SCALE - 1], fill=color)

        name = bg_names.get(i, f"bg_{i}")
        # Shorten common prefix for readability
        short = name.replace("TILE_LETTER_", "").replace("TILE_DIGIT_", "d").replace("TILE_", "")
        draw.text((px_x + 3, px_y + 2), short[:8], fill=BLACK)

    return img


# ── Screen renderers ──────────────────────────────────────────────────────

SCR_W, SCR_H = 160, 144

def new_screen():
    return Image.new("RGB", (SCR_W * SCALE, SCR_H * SCALE), WHITE)

def place_bg_tile(img, bg_raw, tile_idx, tx, ty):
    if tile_idx * 16 + 16 > len(bg_raw):
        return
    tile_pixels = decode_tile(bg_raw, tile_idx * 16)
    draw_tile(img, tx, ty, tile_pixels, BGP)

def place_spr_tile(img, spr_raw, tile_idx, sx, sy, pal=None):
    pal = pal or OBP1
    if tile_idx * 16 + 16 > len(spr_raw):
        return
    tile_pixels = decode_tile(spr_raw, tile_idx * 16)
    # Convert pixel position to tile coords for draw_tile
    for row in range(8):
        for col in range(8):
            cidx = tile_pixels[row][col]
            if cidx == 0:
                continue
            color = pal[cidx]
            px = (sx + col) * SCALE
            py = (sy + row) * SCALE
            if 0 <= px < img.size[0] - SCALE and 0 <= py < img.size[1] - SCALE:
                for dy in range(SCALE):
                    for dx in range(SCALE):
                        img.putpixel((px + dx, py + dy), color)

def make_starfield(bg_raw, seed=42):
    img = new_screen()
    rng = random.Random(seed)
    for ty in range(18):
        for tx in range(20):
            r = rng.randint(0, 255)
            if r < 8:    tidx = 2
            elif r < 32: tidx = 1
            else:        tidx = 0
            place_bg_tile(img, bg_raw, tidx, tx, ty)
    return img

def place_win_row(img, bg_raw, tile_row, win_ty):
    for tx, tidx in enumerate(tile_row):
        place_bg_tile(img, bg_raw, tidx, tx, win_ty)

def black_row():  return [14] * 20
def dash_row():   return [15] * 20
def centered(tiles, w=20): r=[14]*w; s=(w-len(tiles))//2; r[s:s+len(tiles)]=tiles; return r
def star_deco():  return [2 if i%2==0 else 13 for i in range(20)]

def render_screens(bg_raw, spr_raw, bg_count, spr_count):
    """Return dict of screen_name → PIL Image."""
    screens = {}

    L = {c: i for i, c in enumerate(
        [None,None,None,None,None,None,None,None,None,None,None,None,None,None,None,None,
         'A','E','G','H','M','O','P','R','S','T','V','B','U','D']
    )}
    # Build from tiles.h defines — use fixed known mapping
    LT = {'A':16,'E':17,'G':18,'H':19,'M':20,'O':21,'P':22,
          'R':23,'S':24,'T':25,'V':26,'B':27,'U':28,'D':29,' ':0}
    D0 = 3  # digit 0 tile index

    def ltiles(s):
        return [LT.get(c, 0) for c in s.upper()]

    def dtiles(n, w=4):
        return [D0 + int(c) for c in str(n).zfill(w)]

    # ── Gameplay ──
    sc = make_starfield(bg_raw)
    hud = [13,13,13,0,0, 0, 2,2,0,0,0, 0, 27, D0+2, 0,0, D0,D0+1,D0+3,D0+0]
    for tx in range(20): place_bg_tile(sc, bg_raw, 14, tx, 17)
    place_win_row(sc, bg_raw, hud, 17)
    place_spr_tile(sc, spr_raw, 0, 76, 112, OBP0)
    place_spr_tile(sc, spr_raw, 2, 20,  20, OBP1)
    place_spr_tile(sc, spr_raw, 2, 72,  32, OBP1)
    place_spr_tile(sc, spr_raw, 3,130,  16, OBP1)
    place_spr_tile(sc, spr_raw, 1, 80,  88, OBP1)
    place_spr_tile(sc, spr_raw, 1, 80,  64, OBP1)
    place_spr_tile(sc, spr_raw, 7, 50,  80, OBP1)
    screens["03_gameplay"] = sc

    # ── Title ──
    sc = make_starfield(bg_raw)
    for r in range(18): place_win_row(sc, bg_raw, black_row(), r)
    place_win_row(sc, bg_raw, dash_row(), 3)
    place_win_row(sc, bg_raw, centered(ltiles("SHOOTER")), 5)
    place_win_row(sc, bg_raw, dash_row(), 7)
    place_win_row(sc, bg_raw, star_deco(), 10)
    place_win_row(sc, bg_raw, dash_row(), 12)
    place_win_row(sc, bg_raw, centered(ltiles("PRESS A")), 14)
    screens["04_title"] = sc

    # ── Game Over ──
    sc = make_starfield(bg_raw)
    for r in range(18): place_win_row(sc, bg_raw, black_row(), r)
    place_win_row(sc, bg_raw, dash_row(), 3)
    place_win_row(sc, bg_raw, centered(ltiles("GAME OVER")), 5)
    place_win_row(sc, bg_raw, dash_row(), 7)
    place_win_row(sc, bg_raw, centered(dtiles(1340)), 9)
    place_win_row(sc, bg_raw, dash_row(), 11)
    place_win_row(sc, bg_raw, centered(ltiles("PRESS A")), 13)
    screens["05_gameover"] = sc

    # ── Paused ──
    sc = make_starfield(bg_raw)
    for r in range(18): place_win_row(sc, bg_raw, black_row(), r)
    place_win_row(sc, bg_raw, centered(ltiles("PAUSED")), 8)
    screens["06_paused"] = sc

    # ── Boss fight ──
    sc = make_starfield(bg_raw)
    BAR = 30
    for tx in range(20):
        place_bg_tile(sc, bg_raw, BAR if tx < 15 else 0, tx, 0)
    for tx in range(20): place_bg_tile(sc, bg_raw, 14, tx, 17)
    place_win_row(sc, bg_raw, hud, 17)
    if spr_count >= 13:
        for qi, qidx in enumerate([9,10,11,12]):
            qr, qc = divmod(qi, 2)
            place_spr_tile(sc, spr_raw, qidx, 72 + qc*8, 20 + qr*8, OBP1)
    place_spr_tile(sc, spr_raw, 1, 76, 48, OBP1)
    place_spr_tile(sc, spr_raw, 1, 84, 56, OBP1)
    place_spr_tile(sc, spr_raw, 0, 76, 112, OBP0)
    screens["07_boss_fight"] = sc

    # ── Congrats ──
    sc = make_starfield(bg_raw)
    for r in range(18): place_win_row(sc, bg_raw, black_row(), r)
    place_win_row(sc, bg_raw, dash_row(), 3)
    place_win_row(sc, bg_raw, centered(ltiles("GREAT")), 5)
    place_win_row(sc, bg_raw, centered(ltiles("SHOT")), 7)
    place_win_row(sc, bg_raw, dash_row(), 9)
    place_win_row(sc, bg_raw, star_deco(), 11)
    place_win_row(sc, bg_raw, centered(dtiles(1340)), 13)
    place_win_row(sc, bg_raw, dash_row(), 15)
    place_win_row(sc, bg_raw, centered(ltiles("PRESS A")), 17)
    screens["08_congrats"] = sc

    return screens


# ── Diff ──────────────────────────────────────────────────────────────────

def img_hash(img):
    return hashlib.md5(img.tobytes()).hexdigest()

def pixel_diff(img_a, img_b):
    """Return count of pixels that differ between two same-size images."""
    if img_a.size != img_b.size:
        return -1
    diff = 0
    for y in range(img_a.size[1]):
        for x in range(img_a.size[0]):
            if img_a.getpixel((x, y)) != img_b.getpixel((x, y)):
                diff += 1
    return diff


# ── Main ──────────────────────────────────────────────────────────────────

def main():
    parser = argparse.ArgumentParser(description="Regenerate GB Shooter preview PNGs")
    parser.add_argument("--project-root", default=None)
    parser.add_argument("--out", default=None)
    parser.add_argument("--quiet", action="store_true")
    args = parser.parse_args()

    # Locate project root
    if args.project_root:
        root = Path(args.project_root)
    else:
        root = Path(__file__).parent.parent  # tools/../ = project root

    tiles_c = root / "src" / "tiles.c"
    tiles_h = root / "src" / "tiles.h"

    if not tiles_c.exists():
        sys.exit(f"Cannot find {tiles_c}")
    if not tiles_h.exists():
        sys.exit(f"Cannot find {tiles_h}")

    src  = tiles_c.read_text()
    hsrc = tiles_h.read_text()

    bg_raw  = parse_tile_array(src, "bg_tiles")
    spr_raw = parse_tile_array(src, "sprite_tiles")

    if bg_raw is None:
        sys.exit("Could not parse bg_tiles[] from tiles.c")
    if spr_raw is None:
        sys.exit("Could not parse sprite_tiles[] from tiles.c")

    bg_count, spr_count = parse_counts(hsrc)
    if not args.quiet:
        print(f"Parsed {bg_count} BG tiles ({len(bg_raw)} bytes), "
              f"{spr_count} sprite tiles ({len(spr_raw)} bytes)")

    # Build name maps
    spr_name_map = {}
    for m in re.finditer(r'#define\s+(SPR_\w+)\s+(\d+)', hsrc):
        spr_name_map[int(m.group(2))] = m.group(1).replace("SPR_", "").lower()

    bg_name_map = {}
    for m in re.finditer(r'#define\s+(TILE_\w+)\s+(\d+)', hsrc):
        bg_name_map[int(m.group(2))] = m.group(1)

    out_dir = Path(args.out) if args.out else root / "sprites_preview"
    out_dir.mkdir(exist_ok=True)

    changed = []
    unchanged = []

    def save_and_diff(name, new_img):
        out_path = out_dir / f"{name}.png"
        if out_path.exists():
            old_img = Image.open(out_path).convert("RGB")
            new_rgb = new_img.convert("RGB")
            if img_hash(old_img) != img_hash(new_rgb):
                diff_px = pixel_diff(old_img, new_rgb)
                changed.append((name, diff_px))
            else:
                unchanged.append(name)
        else:
            changed.append((name, -1))  # new file
        new_img.save(out_path)
        if not args.quiet:
            status = "CHANGED" if any(n == name for n, _ in changed) else "unchanged"
            print(f"  {name}.png  [{status}]")

    if not args.quiet:
        print("\nRendering sprite sheets...")
    save_and_diff("01_sprites", render_sprite_sheet(spr_raw, spr_count, spr_name_map))
    save_and_diff("02_bg_tiles", render_bg_sheet(bg_raw, bg_count, bg_name_map))

    if not args.quiet:
        print("Rendering screens...")
    screens = render_screens(bg_raw, spr_raw, bg_count, spr_count)
    for name, img in sorted(screens.items()):
        save_and_diff(name, img)

    print(f"\n{'='*50}")
    if changed:
        print(f"CHANGED ({len(changed)}):")
        for name, diff_px in changed:
            if diff_px == -1:
                print(f"  + {name}.png  [new]")
            else:
                print(f"  ~ {name}.png  [{diff_px} pixels differ]")
    if unchanged:
        print(f"Unchanged ({len(unchanged)}): {', '.join(unchanged)}")

    return 1 if changed else 0


if __name__ == "__main__":
    sys.exit(main())
