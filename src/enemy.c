#include "enemy.h"
#include "tiles.h"
#include <gbdk/platform.h>

Enemy enemies[ENEMY_COUNT];
uint8_t wave_num;
uint8_t enemies_alive;

/* Wave configs: {count, pattern, speed_y, spread} */
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
        set_sprite_prop(ENEMY_OAM_BASE + i, 0x10U); /* OBP1 → shade 3 (black) */
    }
}

void enemies_spawn_wave(void) {
    uint8_t i;
    const WaveConfig *w = &waves[wave_num % WAVE_COUNT];
    uint8_t count = w->count;
    if (count > ENEMY_COUNT) count = ENEMY_COUNT;

    enemies_alive = 0;
    for (i = 0; i < ENEMY_COUNT; i++) {
        enemies[i].active = 0;
        move_sprite(ENEMY_OAM_BASE + i, 0, 0);
    }

    for (i = 0; i < count; i++) {
        enemies[i].active = 1;
        /* Space evenly across the top */
        enemies[i].x = 20 + (i * (140 / count));
        enemies[i].y = 10 + (i * 4);
        enemies[i].pattern = w->pattern;
        enemies[i].speed_y = w->speed_y;
        enemies[i].timer = i * 8; /* stagger pattern phase */
        enemies[i].dir = (i & 1) ? 1 : 0;
        enemies[i].speed_x = (w->pattern == PATTERN_DIAGONAL) ? ((i & 1) ? 1 : -1) : 0;
        enemies[i].tile = (i & 1) ? SPR_ENEMY_B : SPR_ENEMY_A;
        set_sprite_tile(ENEMY_OAM_BASE + i, enemies[i].tile);
        move_sprite(ENEMY_OAM_BASE + i, enemies[i].x, enemies[i].y);
        enemies_alive++;
    }

    wave_num++;
}

void enemies_update(void) {
    uint8_t i;
    for (i = 0; i < ENEMY_COUNT; i++) {
        if (!enemies[i].active) continue;

        enemies[i].timer++;

        switch (enemies[i].pattern) {
        case PATTERN_STRAIGHT:
            enemies[i].y += enemies[i].speed_y;
            break;

        case PATTERN_ZIGZAG:
            enemies[i].y += enemies[i].speed_y;
            if (enemies[i].dir == 0) {
                if (enemies[i].x < 152) enemies[i].x++;
                else enemies[i].dir = 1;
            } else {
                if (enemies[i].x > 8) enemies[i].x--;
                else enemies[i].dir = 0;
            }
            break;

        case PATTERN_SWOOP:
            /* Swoop: move down fast for first 40 frames, then slow */
            if (enemies[i].timer < 40)
                enemies[i].y += 2;
            else
                enemies[i].y += enemies[i].speed_y;
            /* Also drift horizontally */
            if (enemies[i].dir == 0) {
                if (enemies[i].x < 140) enemies[i].x++;
                else enemies[i].dir = 1;
            } else {
                if (enemies[i].x > 20) enemies[i].x--;
                else enemies[i].dir = 0;
            }
            break;

        case PATTERN_DIAGONAL:
            enemies[i].y += enemies[i].speed_y;
            enemies[i].x = (uint8_t)((int8_t)enemies[i].x + enemies[i].speed_x);
            /* Bounce off walls */
            if (enemies[i].x <= 8 || enemies[i].x >= 152)
                enemies[i].speed_x = -enemies[i].speed_x;
            break;

        case PATTERN_SINE:
            /* Approximate sine with counter: shift every 16 frames */
            enemies[i].y += enemies[i].speed_y;
            {
                uint8_t phase = (enemies[i].timer >> 3) & 3;
                if (phase == 0)      enemies[i].x++;
                else if (phase == 1) enemies[i].x++;
                else if (phase == 2) enemies[i].x--;
                else                 enemies[i].x--;
            }
            break;
        }

        /* Check if enemy has left the screen at the bottom */
        if (enemies[i].y > 144) {
            enemies[i].active = 0;
            move_sprite(ENEMY_OAM_BASE + i, 0, 0);
            if (enemies_alive > 0) enemies_alive--;
        } else {
            move_sprite(ENEMY_OAM_BASE + i, enemies[i].x, enemies[i].y);
        }
    }
}
