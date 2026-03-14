#include "collision.h"
#include "bullet.h"
#include "enemy.h"
#include "player.h"
#include "pickup.h"
#include "hud.h"
#include "sound.h"
#include <gbdk/platform.h>

/* Returns 1 if two 6x6 AABB overlap.
   Sprites are positioned by their top-left; hardware offset handled by move_sprite.
   We use a 1px inset hitbox: effective box is [x+1 .. x+6], [y+1 .. y+6] */
uint8_t aabb_hit(uint8_t ax, uint8_t ay, uint8_t bx, uint8_t by) {
    uint8_t ax1 = ax + 1, ay1 = ay + 1;
    uint8_t bx1 = bx + 1, by1 = by + 1;
    /* No overlap if one box is entirely to the side of the other */
    if (ax1 + 6 <= bx1) return 0;
    if (bx1 + 6 <= ax1) return 0;
    if (ay1 + 6 <= by1) return 0;
    if (by1 + 6 <= ay1) return 0;
    return 1;
}

void collision_check(void) {
    uint8_t i, j;

    /* Bullet vs Enemy */
    for (i = 0; i < BULLET_COUNT; i++) {
        if (!bullets[i].active) continue;
        for (j = 0; j < ENEMY_COUNT; j++) {
            if (!enemies[j].active) continue;
            if (aabb_hit(bullets[i].x, bullets[i].y, enemies[j].x, enemies[j].y)) {
                /* Kill bullet */
                bullets[i].active = 0;
                move_sprite(BULLET_OAM_BASE + i, 0, 0);
                /* Kill enemy — try to spawn a pickup */
                pickup_try_spawn(enemies[j].x, enemies[j].y);
                enemies[j].active = 0;
                move_sprite(ENEMY_OAM_BASE + j, 0, 0);
                if (enemies_alive > 0) enemies_alive--;
                /* Score */
                hud_add_score(10);
                sfx_explosion();
                break;
            }
        }
    }

    /* Enemy vs Player */
    if (player.alive && player.inv_timer == 0) {
        for (j = 0; j < ENEMY_COUNT; j++) {
            if (!enemies[j].active) continue;
            if (aabb_hit(player.x, player.y, enemies[j].x, enemies[j].y)) {
                player_hit();
                /* Also destroy the enemy */
                enemies[j].active = 0;
                move_sprite(ENEMY_OAM_BASE + j, 0, 0);
                if (enemies_alive > 0) enemies_alive--;
                break;
            }
        }
    }

    /* Player vs Pickups — always collectible regardless of inv_timer */
    if (player.alive) {
        for (i = 0; i < PICKUP_COUNT; i++) {
            if (!pickups[i].active) continue;
            if (aabb_hit(player.x, player.y, pickups[i].x, pickups[i].y)) {
                if (pickups[i].type == PICKUP_TYPE_HEART) {
                    if (player.lives < PLAYER_LIVES_MAX)
                        player.lives++;
                } else if (pickups[i].type == PICKUP_TYPE_POWER) {
                    if (player.power_level < 5)
                        player.power_level++;
                } else if (pickups[i].type == PICKUP_TYPE_BOMB) {
                    if (player.bombs < 5)
                        player.bombs++;
                }
                pickups[i].active = 0;
                move_sprite(PICKUP_OAM_BASE + i, 0, 0);
                sfx_pickup();
            }
        }
    }
}
