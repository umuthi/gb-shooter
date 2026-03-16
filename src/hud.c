#include "hud.h"
#include "tiles.h"
#include "player.h"
#include <gbdk/platform.h>

uint16_t score;
uint8_t  score_multiplier;

/* Cached values — only redraw the section that actually changed */
static uint8_t  prev_lives;
static uint8_t  prev_power;
static uint8_t  prev_bombs;
static uint16_t prev_score;
static uint8_t  prev_dev;
static uint8_t  prev_multiplier;

/*
 * HUD window layout (cols 0-19):
 *  0-1   H + digit  (lives)
 *  2     gap
 *  3-4   P + digit  (power)
 *  5     gap
 *  6-7   B + digit  (bombs)
 *  8     gap
 *  9-10  X + digit  (multiplier; blank when x1)
 *  11    gap
 *  12-16 score      (5 digits, 0-65535)
 *  17    gap
 *  18    D          (dev mode indicator)
 *  19    gap
 */

static void hud_draw_lives(uint8_t lives) {
    uint8_t buf[2];
    buf[0] = TILE_LETTER_H;
    buf[1] = TILE_DIGIT_0 + (lives <= 9 ? lives : 9);
    set_win_tiles(0, 0, 2, 1, buf);
}

static void hud_draw_power(uint8_t power) {
    uint8_t buf[2];
    buf[0] = TILE_LETTER_P;
    buf[1] = TILE_DIGIT_0 + (power <= 9 ? power : 9);
    set_win_tiles(3, 0, 2, 1, buf);
}

static void hud_draw_bombs(uint8_t bombs) {
    uint8_t buf[2];
    buf[0] = TILE_LETTER_B;
    buf[1] = TILE_DIGIT_0 + (bombs <= 9 ? bombs : 9);
    set_win_tiles(6, 0, 2, 1, buf);
}

static void hud_draw_multiplier(uint8_t mult) {
    uint8_t buf[2];
    if (mult > 1) {
        buf[0] = TILE_LETTER_X;
        buf[1] = TILE_DIGIT_0 + (mult <= 9 ? mult : 9);
    } else {
        buf[0] = TILE_BLACK;
        buf[1] = TILE_BLACK;
    }
    set_win_tiles(9, 0, 2, 1, buf);
}

static void hud_draw_score(void) {
    uint16_t s = score;
    uint8_t digits[5];
    digits[4] = TILE_DIGIT_0 + (s % 10); s /= 10;
    digits[3] = TILE_DIGIT_0 + (s % 10); s /= 10;
    digits[2] = TILE_DIGIT_0 + (s % 10); s /= 10;
    digits[1] = TILE_DIGIT_0 + (s % 10); s /= 10;
    digits[0] = TILE_DIGIT_0 + (s % 10);
    set_win_tiles(12, 0, 5, 1, digits);
}

/* Points per enemy kill, scaled by difficulty */
uint8_t hud_enemy_pts(void) {
    if (difficulty == DIFFICULTY_HARD)   return 40;
    if (difficulty == DIFFICULTY_NORMAL) return 20;
    return 10;
}

/* Points per boss bullet hit, scaled by difficulty */
uint8_t hud_boss_pts(void) {
    if (difficulty == DIFFICULTY_HARD)   return 20;
    if (difficulty == DIFFICULTY_NORMAL) return 10;
    return 5;
}

void hud_add_score(uint8_t points) {
    uint16_t add = (uint16_t)points * score_multiplier;
    if (add > (uint16_t)(65535U - score))
        score = 65535U;
    else
        score += add;
}

void hud_init(void) {
    uint8_t i;
    uint8_t black_row[20];
    score = 0;
    score_multiplier = 1;

    /* Fill entire window row with black tiles */
    for (i = 0; i < 20; i++) black_row[i] = TILE_BLACK;
    set_win_tiles(0, 0, 20, 1, black_row);

    /* Position window layer */
    move_win(HUD_WX, HUD_WY);
    SHOW_WIN;

    /* Force-draw everything on init */
    prev_lives      = 0xFF;
    prev_power      = 0xFF;
    prev_bombs      = 0xFF;
    prev_score      = 0xFFFF;
    prev_dev        = 0xFF;
    prev_multiplier = 0xFF;
    hud_update();
}

void hud_update(void) {
    uint8_t lives = player.lives;
    uint8_t power = player.power_level;
    uint8_t bombs = player.bombs;

    if (lives != prev_lives) {
        hud_draw_lives(lives);
        prev_lives = lives;
    }
    if (power != prev_power) {
        hud_draw_power(power);
        prev_power = power;
    }
    if (bombs != prev_bombs) {
        hud_draw_bombs(bombs);
        prev_bombs = bombs;
    }
    if (score_multiplier != prev_multiplier) {
        hud_draw_multiplier(score_multiplier);
        prev_multiplier = score_multiplier;
    }
    if (score != prev_score) {
        hud_draw_score();
        prev_score = score;
    }
    /* Dev mode indicator at col 18 */
    if (player.dev_mode != prev_dev) {
        uint8_t d = player.dev_mode ? TILE_LETTER_D : TILE_BLACK;
        set_win_tiles(18, 0, 1, 1, &d);
        prev_dev = player.dev_mode;
    }
}
