#include "hud.h"
#include "tiles.h"
#include "player.h"
#include <gbdk/platform.h>

uint16_t score;

/*
 * HUD window row 0 layout (20 cols):
 *  Cols  0- 4: hearts  (TILE_HEART for each life, TILE_BLACK for empty)
 *  Col   5   : TILE_BLACK spacer
 *  Cols  6-10: power level (TILE_STAR_LG for each level, TILE_BLACK for empty)
 *  Col  11   : TILE_BLACK spacer
 *  Col  12   : TILE_LETTER_B
 *  Col  13   : bomb count digit (TILE_DIGIT_0 + player.bombs)
 *  Cols 14-15: TILE_BLACK spacers
 *  Cols 16-19: score digits
 */

void hud_draw_lives(uint8_t lives) {
    uint8_t buf[5];
    uint8_t i;
    for (i = 0; i < 5; i++) {
        buf[i] = (i < lives) ? TILE_HEART : TILE_BLACK;
    }
    set_win_tiles(0, 0, 5, 1, buf);
}

static void hud_draw_power(uint8_t power) {
    uint8_t buf[5];
    uint8_t i;
    for (i = 0; i < 5; i++) {
        buf[i] = (i < power) ? TILE_STAR_LG : TILE_BLACK;
    }
    set_win_tiles(6, 0, 5, 1, buf);
}

static void hud_draw_bombs(uint8_t bombs) {
    uint8_t buf[2];
    buf[0] = TILE_LETTER_B;
    buf[1] = TILE_DIGIT_0 + (bombs <= 9 ? bombs : 9);
    set_win_tiles(12, 0, 2, 1, buf);
}

void hud_draw_score(void) {
    uint16_t s = score;
    uint8_t digits[4];
    digits[3] = s % 10; s /= 10;
    digits[2] = s % 10; s /= 10;
    digits[1] = s % 10; s /= 10;
    digits[0] = s % 10;
    digits[0] += TILE_DIGIT_0;
    digits[1] += TILE_DIGIT_0;
    digits[2] += TILE_DIGIT_0;
    digits[3] += TILE_DIGIT_0;
    set_win_tiles(16, 0, 4, 1, digits);
}

void hud_add_score(uint8_t points) {
    score += points;
    if (score > 9999) score = 9999;
}

void hud_init(void) {
    uint8_t i;
    uint8_t black_row[20];
    score = 0;

    /* Fill entire window row with black tiles */
    for (i = 0; i < 20; i++) black_row[i] = TILE_BLACK;
    set_win_tiles(0, 0, 20, 1, black_row);

    /* Set spacer tiles at fixed positions */
    {
        uint8_t sp = TILE_BLACK;
        set_win_tiles(5, 0, 1, 1, &sp);
        set_win_tiles(11, 0, 1, 1, &sp);
        set_win_tiles(14, 0, 1, 1, &sp);
        set_win_tiles(15, 0, 1, 1, &sp);
    }

    /* Position window layer */
    move_win(HUD_WX, HUD_WY);
    SHOW_WIN;

    hud_draw_lives(player.lives);
    hud_draw_power(player.power_level);
    hud_draw_bombs(player.bombs);
    hud_draw_score();
}

void hud_update(void) {
    hud_draw_lives(player.lives);
    hud_draw_power(player.power_level);
    hud_draw_bombs(player.bombs);
    hud_draw_score();
}
