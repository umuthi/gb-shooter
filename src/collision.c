#include "collision.h"
#include "bullet.h"
#include "enemy.h"
#include "player.h"
#include "pickup.h"
#include "boss.h"
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

/* AABB test for bullet (8x8) vs boss (16x16). */
#define BOSS_HIT(bx, by) \
    (!((bx)+7 <= (boss.x)+1 || (boss.x)+17 <= (bx)+1 || \
       (by)+7 <= (boss.y)+1 || (boss.y)+17 <= (by)+1))

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
    uint8_t px, py;

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
        px = player.x; py = player.y;
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
        px = player.x; py = player.y;
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

    /* Enemy bullets vs player — only active in stage 2+ */
    if (player.alive && player.inv_timer == 0 && game_stage >= 2) {
        px = player.x; py = player.y;
        for (i = 0; i < ENEMY_BULLET_COUNT; i++) {
            if (!enemy_bullets[i].active) continue;
            if (AABB_HIT(px, py, enemy_bullets[i].x, enemy_bullets[i].y)) {
                enemy_bullets[i].active = 0;
                move_sprite(ENEMY_BULLET_BASE + i, 0, 0);
                player_hit();
                break;
            }
        }
    }
}

/*
 * Enemy-only collision during final-boss phase 2.
 * Handles: player bullets vs enemies, enemy body vs player,
 *          enemy bullets vs player, player vs pickups.
 * Call in addition to collision_check_boss() during phase 2.
 */
void collision_check_enemies(void) {
    uint8_t i, j;
    uint8_t bx, by, ex, ey;
    uint8_t px, py;

    /* Player bullets vs enemies */
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

    /* Enemy body vs player */
    if (player.alive && player.inv_timer == 0) {
        px = player.x; py = player.y;
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

    /* Enemy bullets vs player */
    if (player.alive && player.inv_timer == 0) {
        px = player.x; py = player.y;
        for (i = 0; i < ENEMY_BULLET_COUNT; i++) {
            if (!enemy_bullets[i].active) continue;
            if (AABB_HIT(px, py, enemy_bullets[i].x, enemy_bullets[i].y)) {
                enemy_bullets[i].active = 0;
                move_sprite(ENEMY_BULLET_BASE + i, 0, 0);
                player_hit();
                break;
            }
        }
    }

    /* Player vs pickups (dropped by enemies killed during phase 2) */
    if (player.alive) {
        px = player.x; py = player.y;
        for (i = 0; i < PICKUP_COUNT; i++) {
            if (!pickups[i].active) continue;
            if (AABB_HIT(px, py, pickups[i].x, pickups[i].y)) {
                if (pickups[i].type == PICKUP_TYPE_HEART) {
                    if (player.lives < PLAYER_LIVES_MAX) player.lives++;
                } else if (pickups[i].type == PICKUP_TYPE_POWER) {
                    if (player.power_level < 5) player.power_level++;
                } else if (pickups[i].type == PICKUP_TYPE_BOMB) {
                    if (player.bombs < 5) player.bombs++;
                }
                pickups[i].active = 0;
                move_sprite(PICKUP_OAM_BASE + i, 0, 0);
                sfx_pickup();
            }
        }
    }
}

/*
 * Boss-phase collision: player bullets vs boss, boss bullets vs player,
 * boss body vs player.  Call instead of collision_check() during STATE_BOSS.
 */
void collision_check_boss(void) {
    uint8_t i;
    uint8_t bx, by;

    if (!boss.active || boss.dying) return;

    /* Player bullets vs boss */
    for (i = 0; i < BULLET_COUNT; i++) {
        if (!bullets[i].active) continue;
        bx = bullets[i].x;
        by = bullets[i].y;
        if (BOSS_HIT(bx, by)) {
            bullets[i].active = 0;
            move_sprite(BULLET_OAM_BASE + i, 0, 0);
            boss_hit(1);
            hud_add_score(5);
            sfx_explosion();
        }
    }

    /* Boss body vs player */
    if (player.alive && player.inv_timer == 0) {
        if (BOSS_HIT(player.x, player.y))
            player_hit();
    }

    /* Boss bullets vs player */
    if (player.alive && player.inv_timer == 0) {
        bx = player.x; by = player.y;
        for (i = 0; i < BOSS_BULLET_COUNT; i++) {
            if (!boss_bullets[i].active) continue;
            if (AABB_HIT(bx, by, boss_bullets[i].x, boss_bullets[i].y)) {
                boss_bullets[i].active = 0;
                move_sprite(BOSS_BULLET_BASE + i, 0, 0);
                player_hit();
                break;
            }
        }
    }
}
