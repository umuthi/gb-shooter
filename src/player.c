#include "player.h"
#include "bullet.h"
#include "sound.h"
#include "tiles.h"
#include "hud.h"
#include <gbdk/platform.h>

Player player;

void player_init(void) {
    player.x = PLAYER_START_X;
    player.y = PLAYER_START_Y;
    player.lives = PLAYER_LIVES_INIT;
    player.inv_timer = 0;
    player.shoot_cooldown = 0;
    player.alive = 1;
    player.power_level = 1;
    player.bombs = 0;
    player.dev_mode = 0;
    move_sprite(PLAYER_OAM_SLOT, player.x, player.y);
    set_sprite_tile(PLAYER_OAM_SLOT, SPR_PLAYER);
    set_sprite_prop(PLAYER_OAM_SLOT, 0x00U); /* OBP0 → shade 2 (dark gray) */
}

void player_update(uint8_t joy) {
    if (!player.alive) return;

    /* Movement */
    if ((joy & J_LEFT) && player.x > PLAYER_LEFT_MIN)
        player.x -= PLAYER_SPEED;
    if ((joy & J_RIGHT) && player.x < PLAYER_RIGHT_MAX)
        player.x += PLAYER_SPEED;
    if ((joy & J_UP) && player.y > PLAYER_TOP_MIN)
        player.y -= PLAYER_SPEED;
    if ((joy & J_DOWN) && player.y < PLAYER_BOTTOM_MAX)
        player.y += PLAYER_SPEED;

    /* Shooting */
    if (player.shoot_cooldown > 0)
        player.shoot_cooldown--;

    if ((joy & J_A) && player.shoot_cooldown == 0) {
        static const int8_t spread_table[] = {
            0,
            -6, 6,
            -10, 0, 10,
            -12, -4, 4, 12,
            -14, -7, 0, 7, 14
        };
        static const uint8_t spread_start[] = {0, 1, 3, 6, 10};
        uint8_t i;
        uint8_t any_spawned = 0;
        uint8_t pl = player.power_level;
        if (pl < 1) pl = 1;
        if (pl > 5) pl = 5;
        for (i = 0; i < pl; i++) {
            int16_t bx16;
            uint8_t bx;
            bx16 = (int16_t)player.x + (int16_t)spread_table[spread_start[pl - 1] + i];
            if (bx16 < 8) bx16 = 8;
            if (bx16 > 152) bx16 = 152;
            bx = (uint8_t)bx16;
            if (bullet_spawn(bx, player.y - 8))
                any_spawned = 1;
        }
        if (any_spawned) {
            /* Higher power = slightly longer cooldown to limit active bullets */
            player.shoot_cooldown = 10 + (pl << 1);
            sfx_shoot();
        }
    }

    /* Invincibility timer */
    if (player.inv_timer > 0)
        player.inv_timer--;

    player_draw();
}

void player_hit(void) {
    if (player.dev_mode) return;     /* invincible in developer mode */
    if (player.inv_timer > 0) return;
    player.lives--;
    player.inv_timer = PLAYER_INV_FRAMES;
    sfx_player_hit();
    /* NORMAL/HARD: reset power and bombs on every hit */
    if (difficulty >= DIFFICULTY_NORMAL) {
        player.power_level = 1;
        player.bombs = 0;
    }
    /* Break the score multiplier streak */
    score_multiplier = 1;
    if (player.lives == 0) {
        player.alive = 0;
        move_sprite(PLAYER_OAM_SLOT, 0, 0); /* hide off screen */
    }
}

void player_draw(void) {
    if (!player.alive) return;
    set_sprite_tile(PLAYER_OAM_SLOT, SPR_PLAYER + anim_frame);
    /* Blink during invincibility: hide every other 8 frames */
    if (player.inv_timer > 0 && (player.inv_timer & 8)) {
        move_sprite(PLAYER_OAM_SLOT, 0, 0);
    } else {
        move_sprite(PLAYER_OAM_SLOT, player.x, player.y);
    }
}
