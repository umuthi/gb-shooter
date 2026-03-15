#include "enemy.h"
#include "player.h"
#include "tiles.h"
#include <gbdk/platform.h>

Enemy       enemies[ENEMY_COUNT];
EnemyBullet enemy_bullets[ENEMY_BULLET_COUNT];
uint8_t     wave_num;
uint8_t     enemies_alive;
uint8_t     game_stage;     /* set by main.c; 1 or 2 */

/* Fast LCG for enemy fire randomness — avoids importing rand from main.c */
static uint8_t erand_seed = 0x37;
static uint8_t erand(void) {
    erand_seed ^= erand_seed << 3;
    erand_seed ^= erand_seed >> 5;
    return erand_seed;
}

typedef struct {
    uint8_t count;
    uint8_t pattern;
    uint8_t speed_y;
} WaveConfig;

#define WAVE_COUNT 5
static const WaveConfig waves[WAVE_COUNT] = {
    {3, PATTERN_STRAIGHT, 1},
    {4, PATTERN_ZIGZAG,   1},
    {5, PATTERN_STRAIGHT, 2},
    {4, PATTERN_SWOOP,    1},
    {6, PATTERN_SINE,     1},
};

void enemies_init(void) {
    uint8_t i;
    wave_num     = 0;
    enemies_alive = 0;
    for (i = 0; i < ENEMY_COUNT; i++) {
        enemies[i].active = 0;
        move_sprite(ENEMY_OAM_BASE + i, 0, 0);
        set_sprite_tile(ENEMY_OAM_BASE + i, SPR_ENEMY_A);
        set_sprite_prop(ENEMY_OAM_BASE + i, 0x10U);
    }
}

void enemy_bullets_init(void) {
    uint8_t i;
    for (i = 0; i < ENEMY_BULLET_COUNT; i++) {
        enemy_bullets[i].active = 0;
        move_sprite(ENEMY_BULLET_BASE + i, 0, 0);
        set_sprite_tile(ENEMY_BULLET_BASE + i, SPR_BULLET);
        set_sprite_prop(ENEMY_BULLET_BASE + i, 0x00U); /* OBP0 = dark gray, distinct from player */
    }
}

void enemies_spawn_wave(void) {
    uint8_t i;
    const WaveConfig *w = &waves[wave_num % WAVE_COUNT];
    uint8_t count = w->count;
    uint8_t pat   = w->pattern;
    uint8_t sy    = w->speed_y;
    Enemy *e;

    if (count > ENEMY_COUNT) count = ENEMY_COUNT;

    enemies_alive = 0;
    for (i = 0; i < ENEMY_COUNT; i++) {
        enemies[i].active = 0;
        move_sprite(ENEMY_OAM_BASE + i, 0, 0);
    }

    for (i = 0; i < count; i++) {
        e = &enemies[i];
        e->active     = 1;
        e->x          = 20 + (i * (140 / count));
        e->y          = 10 + (i * 4);
        e->pattern    = pat;
        e->speed_y    = sy;
        e->timer      = i << 3;
        e->dir        = i & 1;
        e->speed_x    = (pat == PATTERN_DIAGONAL) ? ((i & 1) ? 1 : -1) : 0;
        e->tile       = (i & 1) ? SPR_ENEMY_B : SPR_ENEMY_A;
        e->fire_timer = 70 + (uint8_t)(i * 18);
        set_sprite_tile(ENEMY_OAM_BASE + i, e->tile);
        set_sprite_prop(ENEMY_OAM_BASE + i, 0x10U);
        move_sprite(ENEMY_OAM_BASE + i, e->x, e->y);
        enemies_alive++;
    }

    /* Stage 3+: one kamikaze enemy joins every wave */
    if (game_stage >= 3 && count < ENEMY_COUNT) {
        e = &enemies[count];
        e->active     = 1;
        e->x          = 64 + (uint8_t)(wave_num << 3) % 64; /* vary start x */
        e->y          = 0;
        e->pattern    = PATTERN_KAMIKAZE;
        e->speed_y    = 1;  /* slow hover until lock-on at timer==20 */
        e->timer      = 0;
        e->dir        = 0;  /* 0=not yet locked, set to 1 when charging */
        e->speed_x    = 0;
        e->tile       = SPR_ENEMY_A;
        e->fire_timer = 0;  /* kamikazes do not shoot */
        /* Y-flip + OBP1: makes the sprite look like it's diving */
        set_sprite_tile(ENEMY_OAM_BASE + count, e->tile);
        set_sprite_prop(ENEMY_OAM_BASE + count, 0x50U);
        move_sprite(ENEMY_OAM_BASE + count, e->x, e->y);
        enemies_alive++;
    }

    wave_num++;
}

void enemy_bullets_update(void) {
    uint8_t i;
    EnemyBullet *b;
    for (i = 0; i < ENEMY_BULLET_COUNT; i++) {
        b = &enemy_bullets[i];
        if (!b->active) continue;
        b->y += 2;
        if (b->y > 152) {
            b->active = 0;
            move_sprite(ENEMY_BULLET_BASE + i, 0, 0);
        } else {
            move_sprite(ENEMY_BULLET_BASE + i, b->x, b->y);
        }
    }
}

/* Spawn one enemy bullet downward from (x,y). Returns 0 if pool full. */
static uint8_t enemy_bullet_spawn(uint8_t x, uint8_t y) {
    uint8_t i;
    for (i = 0; i < ENEMY_BULLET_COUNT; i++) {
        if (!enemy_bullets[i].active) {
            enemy_bullets[i].x      = x;
            enemy_bullets[i].y      = y;
            enemy_bullets[i].active = 1;
            move_sprite(ENEMY_BULLET_BASE + i, x, y);
            return 1;
        }
    }
    return 0;
}

void enemies_update(void) {
    uint8_t i;
    Enemy *e;

    for (i = 0; i < ENEMY_COUNT; i++) {
        e = &enemies[i];
        if (!e->active) continue;

        e->timer++;

        switch (e->pattern) {
        case PATTERN_STRAIGHT:
            e->y += e->speed_y;
            break;

        case PATTERN_ZIGZAG:
            e->y += e->speed_y;
            if (e->dir == 0) {
                if (e->x < 152) e->x++;
                else e->dir = 1;
            } else {
                if (e->x > 8) e->x--;
                else e->dir = 0;
            }
            break;

        case PATTERN_SWOOP:
            if (e->timer < 40)
                e->y += 2;
            else
                e->y += e->speed_y;
            if (e->dir == 0) {
                if (e->x < 140) e->x++;
                else e->dir = 1;
            } else {
                if (e->x > 20) e->x--;
                else e->dir = 0;
            }
            break;

        case PATTERN_DIAGONAL:
            e->y += e->speed_y;
            e->x  = (uint8_t)((int8_t)e->x + e->speed_x);
            if (e->x <= 8 || e->x >= 152)
                e->speed_x = -e->speed_x;
            break;

        case PATTERN_SINE:
            e->y += e->speed_y;
            {
                uint8_t phase = (e->timer >> 3) & 3;
                if (phase < 2) e->x++;
                else           e->x--;
            }
            break;

        case PATTERN_KAMIKAZE:
            /* Hover slowly for 20 frames, then lock onto player and charge */
            if (e->timer == 20) {
                /* Lock direction toward player's current position */
                if (player.x > e->x + 4)       e->speed_x =  2;
                else if (player.x < e->x - 4)  e->speed_x = -2;
                else                            e->speed_x =  0;
                e->speed_y = 3;   /* charge speed */
                e->dir     = 1;   /* 1 = locked and charging */
            }
            e->y += e->speed_y;
            if (e->dir == 1) {
                e->x = (uint8_t)((int8_t)e->x + e->speed_x);
                if (e->x < 8)   e->x = 8;
                if (e->x > 152) e->x = 152;
            }
            break;
        }

        /* Stage 2+: enemies fire downward at player */
        if (game_stage >= 2) {
            if (e->fire_timer > 0) {
                e->fire_timer--;
            } else {
                /* Randomise next shot: 60-80 frames so they don't sync up */
                e->fire_timer = 60 + (erand() & 31);
                enemy_bullet_spawn(e->x + 3, e->y + 8);
            }
        }

        if (e->y > 144) {
            e->active = 0;
            move_sprite(ENEMY_OAM_BASE + i, 0, 0);
            if (enemies_alive > 0) enemies_alive--;
        } else {
            move_sprite(ENEMY_OAM_BASE + i, e->x, e->y);
        }
    }
}
