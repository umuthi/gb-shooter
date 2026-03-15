#include <gbdk/platform.h>
#include <gbdk/metasprites.h>
#include <stdint.h>

#include "tiles.h"
#include "player.h"
#include "bullet.h"
#include "enemy.h"
#include "boss.h"
#include "collision.h"
#include "hud.h"
#include "sound.h"
#include "pickup.h"

/* Game states */
#define STATE_TITLE     0
#define STATE_PLAYING   1
#define STATE_GAMEOVER  2
#define STATE_PAUSED    3
#define STATE_BOSS      4
#define STATE_CONGRATS  5

/* Wave threshold to trigger the boss */
#define BOSS_WAVE       20

static uint8_t game_state;
static uint8_t frame_count;
static uint8_t gameover_timer;
static uint8_t congrats_timer;

/* Bomb flash animation state */
#define BOMB_FLASH_FRAMES  16
static uint8_t bomb_flash_timer;

/* Boss death screen-flash animation (plays after death anim completes) */
#define BOSS_DEATH_FLASH_FRAMES  30
static uint8_t boss_death_flash;

/* ---- Starfield (scrolling background) ---- */
static uint8_t scroll_y;

static uint8_t rand8(void) {
    static uint8_t seed = 0xAC;
    seed ^= seed << 3;
    seed ^= seed >> 5;
    return seed;
}

static void starfield_init(void) {
    uint8_t x, y;
    uint8_t row_buf[32];
    scroll_y = 0;
    /* BG map is ONLY ever stars — batch 32 tiles per row for speed */
    for (y = 0; y < 32; y++) {
        for (x = 0; x < 32; x++) {
            uint8_t r = rand8();
            if (r < 8)       row_buf[x] = TILE_STAR_LG;
            else if (r < 32) row_buf[x] = TILE_STAR_SM;
            else             row_buf[x] = TILE_BLANK;
        }
        set_bkg_tiles(0, y, 32, 1, row_buf);
    }
}

static void starfield_scroll(void) {
    scroll_y--;
    SCY_REG = scroll_y;
}

/* ---- Window overlay helpers ---- */

/* Shared black row buffer — filled once, reused everywhere */
static uint8_t black_row[20];

static void win_fill_row(uint8_t row, uint8_t tile) {
    uint8_t buf[20];
    uint8_t i;
    for (i = 0; i < 20; i++) buf[i] = tile;
    set_win_tiles(0, row, 20, 1, buf);
}

static void win_write_centered(uint8_t row, const uint8_t *tiles, uint8_t len) {
    uint8_t buf[20];
    uint8_t i;
    uint8_t start = (20 - len) >> 1;
    for (i = 0; i < 20; i++) buf[i] = TILE_BLACK;
    for (i = 0; i < len; i++) buf[start + i] = tiles[i];
    set_win_tiles(0, row, 20, 1, buf);
}

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

static void win_overlay_init(uint8_t kind) {
    uint8_t r;
    uint8_t i;

    /* Init shared black row once */
    for (i = 0; i < 20; i++) black_row[i] = TILE_BLACK;

    /* Black background for all 18 visible rows — reuse same buffer */
    for (r = 0; r < 18; r++)
        set_win_tiles(0, r, 20, 1, black_row);

    if (kind == 0) {
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
        for (i = 0; i < 20; i++)
            deco[i] = (i & 1) ? TILE_HEART : TILE_STAR_LG;

        win_fill_row(3, TILE_DASH);
        win_write_centered(5, shooter, 7);
        win_fill_row(7, TILE_DASH);
        set_win_tiles(0, 10, 20, 1, deco);
        win_fill_row(12, TILE_DASH);
        win_write_centered(14, pressa, 7);

    } else if (kind == 1) {
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

    } else {
        /* kind == 2: Congratulations screen */
        static const uint8_t great[] = {
            TILE_LETTER_G, TILE_LETTER_R, TILE_LETTER_E,
            TILE_LETTER_A, TILE_LETTER_T
        };
        static const uint8_t shot[] = {
            TILE_LETTER_S, TILE_LETTER_H, TILE_LETTER_O,
            TILE_LETTER_T
        };
        static const uint8_t pressa[] = {
            TILE_LETTER_P, TILE_LETTER_R, TILE_LETTER_E,
            TILE_LETTER_S, TILE_LETTER_S, TILE_BLANK,
            TILE_LETTER_A
        };
        uint8_t deco[20];
        for (i = 0; i < 20; i++)
            deco[i] = (i & 1) ? TILE_HEART : TILE_STAR_LG;

        win_fill_row(3, TILE_DASH);
        win_write_centered(5, great, 5);
        win_write_centered(7, shot, 4);
        win_fill_row(9, TILE_DASH);
        set_win_tiles(0, 11, 20, 1, deco);
        win_draw_score_row(13, score);
        win_fill_row(15, TILE_DASH);
        win_write_centered(17, pressa, 7);
    }

    move_win(HUD_WX, 0);
    SHOW_WIN;
}

/* ---- Title screen ---- */
static void title_init(void) {
    uint8_t i;
    for (i = 0; i < 40; i++) move_sprite(i, 0, 0);

    scroll_y = 0;
    SCY_REG = 0;
    bomb_flash_timer = 0;

    pickups_init();
    win_overlay_init(0);
    game_state = STATE_TITLE;
}

static void title_update(uint8_t joy) {
    if (joy & (J_START | J_A)) {
        game_stage = 1;
        game_state = STATE_PLAYING;
        player_init();
        bullets_init();
        enemies_init();
        enemy_bullets_init();
        enemies_spawn_wave();
        hud_init();
    }
}

/* ---- Game Over screen ---- */
static void gameover_init(void) {
    uint8_t i;
    for (i = 0; i < 29; i++) move_sprite(i, 0, 0);

    /* Restore palette in case bomb flash was active */
    BGP_REG = 0xE4;
    bomb_flash_timer = 0;

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

/* ---- Pause ---- */
static void pause_init(void) {
    static const uint8_t paused_txt[] = {
        TILE_LETTER_P, TILE_LETTER_A, TILE_LETTER_U,
        TILE_LETTER_S, TILE_LETTER_E, TILE_LETTER_D
    };
    uint8_t r, i;
    uint8_t blk[20];
    for (i = 0; i < 20; i++) blk[i] = TILE_BLACK;
    for (r = 0; r < 18; r++)
        set_win_tiles(0, r, 20, 1, blk);
    win_write_centered(8, paused_txt, 6);
    move_win(HUD_WX, 0);
    SHOW_WIN;
    game_state = STATE_PAUSED;
}

static void pause_update(uint8_t joy_pressed) {
    if (joy_pressed & J_SELECT) {
        hud_init();
        game_state = STATE_PLAYING;
    }
}

/* ---- Boss fight ---- */
static void boss_fight_init(void) {
    uint8_t i;
    /* Hide all enemy, enemy-bullet, and pickup sprites */
    for (i = 0; i < ENEMY_COUNT; i++)
        move_sprite(ENEMY_OAM_BASE + i, 0, 0);
    for (i = 0; i < ENEMY_BULLET_COUNT; i++)
        move_sprite(ENEMY_BULLET_BASE + i, 0, 0);
    pickups_init();

    /* Freeze scroll so BG row 0 stays at screen top for health bar */
    SCY_REG = 0;
    scroll_y = 0;

    BGP_REG = 0xE4;
    bomb_flash_timer  = 0;
    boss_death_flash  = 0;

    boss_init(game_stage);   /* stage 1 → boss 1,  stage 2 → boss 2 */
    hud_init();
    game_state = STATE_BOSS;
}

static void boss_fight_update(uint8_t joy, uint8_t joy_pressed) {
    uint8_t j;

    /* ---- Screen-flash after boss death animation completes ---- */
    if (boss_death_flash > 0) {
        boss_death_flash--;
        BGP_REG = ((boss_death_flash & 3) < 2) ? 0x00 : 0xE4;

        if (boss_death_flash == 0) {
            BGP_REG = 0xE4;
            for (j = 0; j < 40; j++) move_sprite(j, 0, 0);

            if (game_stage >= 2) {
                /* Both stages complete → Congratulations */
                win_overlay_init(2);
                congrats_timer = 0;
                game_state = STATE_CONGRATS;
            } else {
                /* Stage 1 complete → advance to stage 2 */
                uint8_t saved_lives  = player.lives;
                uint8_t saved_power  = player.power_level;
                uint8_t saved_bombs  = player.bombs;
                uint16_t saved_score = score;

                game_stage = 2;
                player_init();
                player.lives       = saved_lives;
                player.power_level = saved_power;
                player.bombs       = saved_bombs;

                bullets_init();
                enemies_init();
                enemy_bullets_init();
                enemies_spawn_wave();
                score = saved_score;
                hud_init();

                scroll_y = 0;
                SCY_REG  = 0;
                bomb_flash_timer = 0;
                starfield_init();
                game_state = STATE_PLAYING;
            }
        }
        return;
    }

    /* Pause — only when boss is alive (not during death anim) */
    if ((joy_pressed & J_SELECT) && !boss.dying) {
        pause_init();
        return;
    }

    /* Bomb flash */
    if (bomb_flash_timer > 0) {
        bomb_flash_timer--;
        if (bomb_flash_timer == 0)     BGP_REG = 0xE4;
        else if (bomb_flash_timer & 2) BGP_REG = 0x00;
        else                           BGP_REG = 0xE4;
    }

    /* Bomb — deals heavy damage to boss but leaves at least 1 HP */
    if ((joy_pressed & J_B) && player.bombs > 0 && bomb_flash_timer == 0 && !boss.dying) {
        player.bombs--;
        if (boss.active && boss.hp > 1) {
            uint8_t dmg = (boss.hp > 6) ? 5 : boss.hp - 1;
            boss_hit(dmg);
        }
        sfx_bomb();
        bomb_flash_timer = BOMB_FLASH_FRAMES;
        BGP_REG = 0x00;
    }

    player_update(joy);
    bullets_update();
    boss_update();
    collision_check_boss();
    hud_update();
    /* No starfield_scroll — background is frozen during boss fight */

    /* Play explosion SFX periodically during the death animation */
    if (boss.dying && ((boss.death_timer & 15) == 15)) {
        sfx_explosion();
    }

    /* Once death animation ends (boss_update sets active=0, dying=0) → screen flash */
    if (!boss.active && !boss.dying && boss.hp == 0) {
        boss.hp = 255;  /* prevent re-triggering */
        sfx_explosion();
        boss_death_flash = BOSS_DEATH_FLASH_FRAMES;
        BGP_REG = 0x00;
    }

    if (!player.alive) {
        boss_hide();
        gameover_init();
    }
}

static void congrats_update(uint8_t joy) {
    congrats_timer++;
    if (congrats_timer > 120 && (joy & (J_START | J_A))) {
        title_init();
    }
}

/* ---- Gameplay update ---- */
static void playing_update(uint8_t joy, uint8_t joy_pressed) {
    uint8_t j;

    /* Pause on SELECT */
    if (joy_pressed & J_SELECT) {
        pause_init();
        return;
    }

    /* Bomb flash animation — palette cycling, zero CPU cost */
    if (bomb_flash_timer > 0) {
        bomb_flash_timer--;
        if (bomb_flash_timer == 0) {
            BGP_REG = 0xE4;
        } else if (bomb_flash_timer & 2) {
            BGP_REG = 0x00;
        } else {
            BGP_REG = 0xE4;
        }
    }

    /* Bomb: B button clears all on-screen enemies */
    if ((joy_pressed & J_B) && player.bombs > 0 && bomb_flash_timer == 0) {
        player.bombs--;
        for (j = 0; j < ENEMY_COUNT; j++) {
            if (!enemies[j].active) continue;
            enemies[j].active = 0;
            move_sprite(ENEMY_OAM_BASE + j, 0, 0);
            if (enemies_alive > 0) enemies_alive--;
            hud_add_score(10);
        }
        sfx_bomb();
        bomb_flash_timer = BOMB_FLASH_FRAMES;
        BGP_REG = 0x00;
    }

    player_update(joy);
    bullets_update();
    enemies_update();
    enemy_bullets_update();
    pickups_update();
    collision_check();
    hud_update();
    starfield_scroll();

    if (enemies_alive == 0) {
        if (wave_num >= BOSS_WAVE) {
            boss_fight_init();
        } else {
            enemies_spawn_wave();
        }
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

    BGP_REG  = 0xE4;
    OBP0_REG = 0xE8;
    OBP1_REG = 0xFC;

    sound_init();
    starfield_init();

    DISPLAY_ON;
    SHOW_BKG;
    SHOW_SPRITES;

    frame_count = 0;
    prev_joy = 0;
    bomb_flash_timer = 0;
    congrats_timer = 0;
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
        case STATE_PAUSED:
            pause_update(joy_pressed);
            break;
        case STATE_BOSS:
            boss_fight_update(joy, joy_pressed);
            break;
        case STATE_CONGRATS:
            congrats_update(joy);
            break;
        }

        frame_count++;
    }
}
