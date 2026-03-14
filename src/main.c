#include <gbdk/platform.h>
#include <gbdk/metasprites.h>
#include <stdint.h>

#include "tiles.h"
#include "player.h"
#include "bullet.h"
#include "enemy.h"
#include "collision.h"
#include "hud.h"
#include "sound.h"
#include "pickup.h"

/* Game states */
#define STATE_TITLE     0
#define STATE_PLAYING   1
#define STATE_GAMEOVER  2

static uint8_t game_state;
static uint8_t frame_count;
static uint8_t gameover_timer;

/* ---- Starfield (scrolling background) ---- */
static uint8_t scroll_y;

static uint8_t rand8(void) {
    static uint8_t seed = 0xAC;
    seed ^= seed << 3;
    seed ^= seed >> 5;
    return seed;
}

static void starfield_init(void) {
    uint8_t x, y, tile;
    scroll_y = 0;
    /* BG map is ONLY ever stars — nothing else is written here */
    for (y = 0; y < 32; y++) {
        for (x = 0; x < 32; x++) {
            uint8_t r = rand8();
            if (r < 8)       tile = TILE_STAR_LG;
            else if (r < 32) tile = TILE_STAR_SM;
            else             tile = TILE_BLANK;
            set_bkg_tiles(x, y, 1, 1, &tile);
        }
    }
}

static void starfield_scroll(void) {
    scroll_y--;          /* decrement → viewport moves up → stars fall down */
    SCY_REG = scroll_y;
}

/* ---- Window overlay helpers ---- */

static void win_fill_row(uint8_t row, uint8_t tile) {
    uint8_t buf[20];
    uint8_t i;
    for (i = 0; i < 20; i++) buf[i] = tile;
    set_win_tiles(0, row, 20, 1, buf);
}

/* Center 'len' tiles from the 'tiles' array on the given window row */
static void win_write_centered(uint8_t row, const uint8_t *tiles, uint8_t len) {
    uint8_t buf[20];
    uint8_t i;
    uint8_t start = (20 - len) >> 1;
    for (i = 0; i < 20; i++) buf[i] = TILE_BLACK;
    for (i = 0; i < len; i++) buf[start + i] = tiles[i];
    set_win_tiles(0, row, 20, 1, buf);
}

/* 4-digit score row, centered */
static void win_draw_score_row(uint8_t row, uint16_t s) {
    uint8_t buf[20];
    uint8_t i;
    uint8_t d[4];
    d[3] = s % 10; s /= 10;
    d[2] = s % 10; s /= 10;
    d[1] = s % 10; s /= 10;
    d[0] = s % 10;
    for (i = 0; i < 20; i++) buf[i] = TILE_BLACK;
    buf[8]  = TILE_DIGIT_0 + d[0];
    buf[9]  = TILE_DIGIT_0 + d[1];
    buf[10] = TILE_DIGIT_0 + d[2];
    buf[11] = TILE_DIGIT_0 + d[3];
    set_win_tiles(0, row, 20, 1, buf);
}

/*
 * Full-screen Window overlay.
 * kind=0  →  Title screen
 * kind=1  →  Game Over screen
 *
 * The Window layer is screen-fixed (ignores SCY_REG), so it never
 * drifts. BG tile map stays stars-only throughout.
 */
static void win_overlay_init(uint8_t kind) {
    uint8_t r;

    /* Black background for all 18 visible rows */
    for (r = 0; r < 18; r++) win_fill_row(r, TILE_BLACK);

    if (kind == 0) {
        /*
         *  Title screen layout
         *  -------------------
         *  row  3  : ---- divider ----
         *  row  5  :     SHOOTER
         *  row  7  : ---- divider ----
         *  row 10  :  heart-star deco
         *  row 12  : ---- divider ----
         *  row 14  :     PRESS A
         */
        static const uint8_t shooter[] = {
            TILE_LETTER_S, TILE_LETTER_H, TILE_LETTER_O,
            TILE_LETTER_O, TILE_LETTER_T, TILE_LETTER_E,
            TILE_LETTER_R
        };
        static const uint8_t pressa[] = {
            TILE_LETTER_P, TILE_LETTER_R, TILE_LETTER_E,
            TILE_LETTER_S, TILE_LETTER_S, TILE_BLANK,
            TILE_LETTER_A
        };
        uint8_t deco[20];
        uint8_t i;
        for (i = 0; i < 20; i++)
            deco[i] = (i & 1) ? TILE_HEART : TILE_STAR_LG;

        win_fill_row(3, TILE_DASH);
        win_write_centered(5, shooter, 7);
        win_fill_row(7, TILE_DASH);
        set_win_tiles(0, 10, 20, 1, deco);
        win_fill_row(12, TILE_DASH);
        win_write_centered(14, pressa, 7);

    } else {
        /*
         *  Game Over screen layout
         *  -----------------------
         *  row  3  : ---- divider ----
         *  row  5  :    GAME OVER
         *  row  7  : ---- divider ----
         *  row  9  :      0000   (score)
         *  row 11  : ---- divider ----
         *  row 13  :     PRESS A
         */
        static const uint8_t gameover[] = {
            TILE_LETTER_G, TILE_LETTER_A, TILE_LETTER_M,
            TILE_LETTER_E, TILE_BLANK,
            TILE_LETTER_O, TILE_LETTER_V, TILE_LETTER_E,
            TILE_LETTER_R
        };
        static const uint8_t pressa[] = {
            TILE_LETTER_P, TILE_LETTER_R, TILE_LETTER_E,
            TILE_LETTER_S, TILE_LETTER_S, TILE_BLANK,
            TILE_LETTER_A
        };

        win_fill_row(3, TILE_DASH);
        win_write_centered(5, gameover, 9);
        win_fill_row(7, TILE_DASH);
        win_draw_score_row(9, score);
        win_fill_row(11, TILE_DASH);
        win_write_centered(13, pressa, 7);
    }

    move_win(HUD_WX, 0);
    SHOW_WIN;
}

/* ---- Title screen ---- */
static void title_init(void) {
    uint8_t i;
    for (i = 0; i < 40; i++) move_sprite(i, 0, 0);

    /* Reset scroll — no movement while overlay covers screen */
    scroll_y = 0;
    SCY_REG = 0;

    pickups_init();
    win_overlay_init(0);
    game_state = STATE_TITLE;
}

static void title_update(uint8_t joy) {
    if (joy & (J_START | J_A)) {
        game_state = STATE_PLAYING;
        player_init();
        bullets_init();
        enemies_init();
        enemies_spawn_wave();
        hud_init(); /* moves window to WY=136 for HUD strip */
    }
}

/* ---- Game Over screen ---- */
static void gameover_init(void) {
    uint8_t i;
    for (i = 0; i < 21; i++) move_sprite(i, 0, 0);

    pickups_init();
    win_overlay_init(1);
    gameover_timer = 0;
    game_state = STATE_GAMEOVER;
}

static void gameover_update(uint8_t joy) {
    gameover_timer++;
    if (gameover_timer > 120 && (joy & (J_START | J_A))) {
        title_init();
    }
}

/* ---- Gameplay update ---- */
static void playing_update(uint8_t joy, uint8_t joy_pressed) {
    uint8_t j;

    /* Bomb: B button uses one bomb to clear all on-screen enemies */
    if ((joy_pressed & J_B) && player.bombs > 0) {
        player.bombs--;
        for (j = 0; j < ENEMY_COUNT; j++) {
            if (!enemies[j].active) continue;
            enemies[j].active = 0;
            move_sprite(ENEMY_OAM_BASE + j, 0, 0);
            if (enemies_alive > 0) enemies_alive--;
            hud_add_score(10);
        }
        sfx_bomb();
    }

    player_update(joy);
    bullets_update();
    enemies_update();
    pickups_update();
    collision_check();
    hud_update();
    starfield_scroll(); /* only scroll during active play */

    if (enemies_alive == 0) {
        enemies_spawn_wave();
    }

    if (!player.alive) {
        gameover_init();
    }
}

/* ---- Main ---- */
void main(void) {
    static uint8_t prev_joy;
    uint8_t joy;
    uint8_t joy_pressed;

    DISPLAY_OFF;
    SPRITES_8x8;

    tiles_load();

    /* BGP:  color0→shade0(white bg)  color1→shade1(lightgray stars/text)
             color3→shade3(black overlay) */
    BGP_REG  = 0xE4;
    /* OBP0: color0→transparent  color1→shade2(darkgray)  — player */
    OBP0_REG = 0xE8;
    /* OBP1: color0→transparent  color1→shade3(black)     — bullets + enemies */
    OBP1_REG = 0xFC;

    sound_init();
    starfield_init();

    DISPLAY_ON;
    SHOW_BKG;
    SHOW_SPRITES;

    frame_count = 0;
    prev_joy = 0;
    SCY_REG = 0;
    SCX_REG = 0;

    title_init();

    while (1) {
        wait_vbl_done();

        joy = joypad();
        joy_pressed = joy & ~prev_joy;
        prev_joy = joy;

        switch (game_state) {
        case STATE_TITLE:
            title_update(joy);
            break;
        case STATE_PLAYING:
            playing_update(joy, joy_pressed);
            break;
        case STATE_GAMEOVER:
            gameover_update(joy);
            break;
        }

        frame_count++;
    }
}
