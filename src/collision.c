#include "collision.h"
#include "bullet.h"
#include "enemy.h"
#include "player.h"
#include "pickup.h"
#include "hud.h"
#include "sound.h"
#include <gbdk/platform.h>

/*
 * Inline AABB test — 6x6 hitbox (1px inset from 8x8 sprite).
 * Evaluates to 1 if the two boxes overlap, 0 otherwise.
 * Using a macro avoids function-call overhead in the hot inner loop.
 */
#define AABB_HIT(ax, ay, bx, by) \
    (!(((ax)+7 <= (bx)+1) || ((bx)+7 <= (ax)+1) || \
       ((ay)+7 <= (by)+1) || ((by)+7 <= (ay)+1)))

/* Keep the function for any external callers */
uint8_t aabb_hit(uint8_t ax, uint8_t ay, uint8_t bx, uint8_t by) {
    if (ax + 7 <= bx + 1) return 0;
    if (bx + 7 <= ax + 1) return 0;
    if (ay + 7 <= by + 1) return 0;
    if (by + 7 <= ay + 1) return 0;
    return 1;
}

void collision_check(void) {
    uint8_t i, j;
    uint8_t bx, by, ex, ey;

    /* Bullet vs Enemy */
    for (i = 0; i < BULLET_COUNT; i++) {
        if (!bullets[i].active) continue;
        bx = bullets[i].x;
        by = bullets[i].y;
        for (j = 0; j < ENEMY_COUNT; j++) {
            if (!enemies[j].active) continue;
            ex = enemies[j].x;
            ey = enemies[j].y;
            if (AABB_HIT(bx, by, ex, ey)) {
                bullets[i].active = 0;
                move_sprite(BULLET_OAM_BASE + i, 0, 0);
                pickup_try_spawn(ex, ey);
                enemies[j].active = 0;
                move_sprite(ENEMY_OAM_BASE + j, 0, 0);
                if (enemies_alive > 0) enemies_alive--;
                hud_add_score(10);
                sfx_explosion();
                break;
            }
        }
    }

    /* Enemy vs Player */
    if (player.alive && player.inv_timer == 0) {
        uint8_t px = player.x, py = player.y;
        for (j = 0; j < ENEMY_COUNT; j++) {
            if (!enemies[j].active) continue;
            if (AABB_HIT(px, py, enemies[j].x, enemies[j].y)) {
                player_hit();
                enemies[j].active = 0;
                move_sprite(ENEMY_OAM_BASE + j, 0, 0);
                if (enemies_alive > 0) enemies_alive--;
                break;
            }
        }
    }

    /* Player vs Pickups — always collectible regardless of inv_timer */
    if (player.alive) {
        uint8_t px = player.x, py = player.y;
        for (i = 0; i < PICKUP_COUNT; i++) {
            if (!pickups[i].active) continue;
            if (AABB_HIT(px, py, pickups[i].x, pickups[i].y)) {
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
