# GB Shooter — Development Workflow & Lessons Learned

A repeatable reference for building Game Boy DMG ROMs with GBDK-2020.

---

## Environment Setup

```bash
# 1. Download GBDK-2020
cd ~
wget https://github.com/gbdk-2020/gbdk-2020/releases/download/4.5.0/gbdk-linux64.tar.gz
tar -xzf gbdk-linux64.tar.gz && mv ~/gbdk ~/gbdk-2020

# 2. Add to ~/.bashrc
export GBDK_HOME="$HOME/gbdk-2020"
export PATH="$GBDK_HOME/bin:$PATH"

# 3. Install mGBA emulator
sudo apt update && sudo apt install -y mgba-qt

# 4. Verify
lcc -v   # should report GBDK 4.5.0
```

---

## Build Workflow

```bash
make          # compile all sources → build/shooter.gb
make run      # compile + launch in mGBA
make clean    # remove all build artifacts
```

Always verify ROM size after build:
```bash
wc -c build/shooter.gb   # 32768 = 32KB (no MBC), 65536 = 64KB (MBC1)
```

---

## Hardware Constraints

| Resource | Limit | Notes |
|----------|-------|-------|
| Screen | 160×144, 4 shades | shade 0 = white/transparent, 3 = black |
| OAM slots | 40 total | max 10 sprites per scanline |
| Sprite size | 8×8 only | 16×16 = 4 OAM slots as metasprite |
| ROM (no MBC) | 32 KB | switch to MBC1 (`-Wl-yt0x01`) to grow |
| RAM | 8 KB | no malloc; all pools statically allocated |
| CPU | ~1 MHz effective | no floats, minimize 16-bit math in loops |

---

## SDCC Gotchas

These bit us at least once each:

1. **No C99 compound literals** — `(uint8_t[]){1,2,3}` → compiler error 254.
   Use named `static const uint8_t arr[] = {1,2,3};` instead.

2. **Declare all variables at block top** — mixed declarations mid-block can fail.

3. **Use `uint8_t` everywhere in inner loops** — SDCC generates much faster code for 8-bit types.

4. **Signed arithmetic on `uint8_t`** — cast explicitly:
   ```c
   x = (uint8_t)((int8_t)x + delta);  /* delta is int8_t */
   ```

5. **No floats** — ever. Use lookup tables or fixed-point math.

---

## 2bpp Tile Format

Each 8×8 tile = 16 bytes (2 bytes per scanline row):

```
(plane0_byte, plane1_byte) × 8 rows
color_index = (plane1_bit << 1) | plane0_bit
```

| Color index | Plane 0 | Plane 1 | Typical use |
|-------------|---------|---------|-------------|
| 0 | 0x00 | 0x00 | Background / sprite transparent |
| 1 | pixel | 0x00 | Stars, text, sprites (shade via palette) |
| 3 | 0xFF | 0xFF | Solid black overlay tiles |

---

## Layer Strategy

| Layer | Use for | Affected by scroll? |
|-------|---------|---------------------|
| BG | Starfield; boss health bar (frozen) | Yes — SCY_REG / SCX_REG |
| Window | HUD (WY=136); overlays (WY=0) | No |
| Sprites OBP0 | Player | No |
| Sprites OBP1 | Bullets, enemies, boss, pickups | No |

**Key insight:** The window covers from (WX, WY) to the bottom-right corner — you can't have two separate window strips. To show a health bar at the top *and* a HUD at the bottom simultaneously, freeze the BG scroll (`SCY_REG = 0`) and write the health bar into BG row 0 using `set_bkg_tiles()`.

**Starfield scroll direction:** `scroll_y--; SCY_REG = scroll_y;` makes stars fall *downward* (viewport moves up). Incrementing scrolls the opposite way.

---

## OAM Slot Allocation

```
Slot 0       player
Slots 1–8    player bullets   (BULLET_COUNT = 8)
Slots 9–16   enemies          (ENEMY_COUNT  = 8)
Slots 17–20  pickups          (PICKUP_COUNT = 4)
Slots 21–24  boss (2×2)       (4 tiles)
Slots 25–28  boss bullets     (BOSS_BULLET_COUNT = 4)
```

Allocate statically. Hiding a sprite = `move_sprite(slot, 0, 0)`.

---

## Palette Configuration

```c
BGP_REG  = 0xE4;  /* BG:  color0→white, color1→lightgray, color3→black */
OBP0_REG = 0xE8;  /* OBP0 sprites (player): color1→darkgray             */
OBP1_REG = 0xFC;  /* OBP1 sprites (enemies, bullets, boss): color1→black */
```

Assign palette per sprite slot with `set_sprite_prop(slot, 0x10U)` for OBP1, `0x00U` for OBP0.

**Screen flash trick:** Cycle `BGP_REG` between `0x00` (all-white) and `0xE4` (normal) over N frames — zero CPU overhead for a full-screen visual effect.

---

## Performance Rules

1. **HUD dirty flags** — only redraw HUD sections when their value changes. `set_win_tiles()` is a VRAM write; calling it every frame for unchanged values was the primary cause of slowdown at max bullet count.

2. **Cache struct pointers** — `Entity *e = &pool[i];` before the loop body, not `pool[i].field` repeatedly.

3. **Inline collision macros** — the bullet × enemy inner loop runs 64 times per frame; a function call overhead adds up. Use a `#define AABB_HIT(...)` macro.

4. **Batch BG tile writes** — `set_bkg_tiles(0, y, 32, 1, row_buf)` instead of 32 individual `set_bkg_tile()` calls. For the 32×32 starfield this saves ~992 function calls on init.

5. **Scale shoot cooldown with power level** — without this, max power floods the bullet pool and saturates the OAM scanline budget simultaneously.

---

## Game State Machine Pattern

```c
#define STATE_TITLE    0
#define STATE_PLAYING  1
#define STATE_GAMEOVER 2
#define STATE_PAUSED   3
#define STATE_BOSS     4
#define STATE_CONGRATS 5
```

Each state has `_init()` + `_update(joy, joy_pressed)`. Always transition through `_init()` to reset sprites and variables cleanly.

Edge-triggered input (detect button-press, not button-hold):
```c
joy_pressed = joy & ~prev_joy;
prev_joy = joy;
```

---

## Sound (Direct Register SFX)

No audio library required. Write NR registers directly on the frame a sound triggers:

```c
/* Shoot — CH1 rising sweep */
NR10_REG = 0x1A; NR11_REG = 0x80; NR12_REG = 0xF2;
NR13_REG = 0xC0; NR14_REG = 0x87;

/* Explosion — CH4 noise */
NR41_REG = 0x00; NR42_REG = 0xF5; NR43_REG = 0x43; NR44_REG = 0x80;
```

Channels: CH1 (NR1x) = square+sweep, CH2 (NR2x) = square, CH4 (NR4x) = noise LFSR.

---

## Milestone Checklist

Use this for every meaningful feature addition:

- [ ] Plan the OAM slot allocation before writing sprite code
- [ ] Check tile count vs `BG_TILES_COUNT` / `SPR_TILES_COUNT` before adding tiles
- [ ] Verify all new tiles use the correct color-index convention
- [ ] `make` → zero errors, zero warnings
- [ ] Visual test in mGBA: walk through the feature manually
- [ ] `wc -c build/shooter.gb` → size is expected; switch to MBC1 if approaching 32KB
- [ ] `git add <specific files>` — never `git add .` blindly
- [ ] `git commit` with a message explaining *why*, not just *what*
- [ ] `git push`

---

## Repeatable Project Template

For a new GB ROM project:

```
project/
├── Makefile          (copy from this repo, update SRCS)
├── src/
│   ├── main.c        (state machine, game loop)
│   ├── tiles.c/h     (all tile data + index constants)
│   ├── player.c/h    (input, movement, shooting)
│   ├── bullet.c/h    (pool-based bullet management)
│   ├── enemy.c/h     (wave configs, movement patterns)
│   ├── collision.c/h (AABB hit detection)
│   ├── hud.c/h       (window layer UI)
│   └── sound.c/h     (direct register SFX)
└── build/
    └── game.gb       (generated)
```

Build order: tiles → player → bullet → enemy → collision → hud → sound → main. Each module is self-contained with a clear `.h` interface and `extern` globals for shared state (pools, counters).
