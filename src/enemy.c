#include "enemy.h"
#include "tiles.h"
#include <gbdk/platform.h>

Enemy enemies[ENEMY_COUNT];
uint8_t wave_num;
uint8_t enemies_alive;

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
    wave_num = 0;
    enemies_alive = 0;
    for (i = 0; i < ENEMY_COUNT; i++) {
        enemies[i].active = 0;
        move_sprite(ENEMY_OAM_BASE + i, 0, 0);
        set_sprite_tile(ENEMY_OAM_BASE + i, SPR_ENEMY_A);
        set_sprite_prop(ENEMY_OAM_BASE + i, 0x10U);
    }
}

void enemies_spawn_wave(void) {
    uint8_t i;
    const WaveConfig *w = &waves[wave_num % WAVE_COUNT];
    uint8_t count = w->count;
    uint8_t pat = w->pattern;
    uint8_t sy = w->speed_y;
    Enemy *e;

    if (count > ENEMY_COUNT) count = ENEMY_COUNT;

    enemies_alive = 0;
    for (i = 0; i < ENEMY_COUNT; i++) {
        enemies[i].active = 0;
        move_sprite(ENEMY_OAM_BASE + i, 0, 0);
    }

    for (i = 0; i < count; i++) {
        e = &enemies[i];
        e->active = 1;
        e->x = 20 + (i * (140 / count));
        e->y = 10 + (i * 4);
        e->pattern = pat;
        e->speed_y = sy;
        e->timer = i << 3;
        e->dir = i & 1;
        e->speed_x = (pat == PATTERN_DIAGONAL) ? ((i & 1) ? 1 : -1) : 0;
        e->tile = (i & 1) ? SPR_ENEMY_B : SPR_ENEMY_A;
        set_sprite_tile(ENEMY_OAM_BASE + i, e->tile);
        move_sprite(ENEMY_OAM_BASE + i, e->x, e->y);
        enemies_alive++;
    }

    wave_num++;
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
            e->x = (uint8_t)((int8_t)e->x + e->speed_x);
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
