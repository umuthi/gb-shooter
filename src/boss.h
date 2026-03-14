#ifndef BOSS_H
#define BOSS_H

#include <gbdk/platform.h>

#define BOSS_OAM_BASE      21   /* OAM slots 21-24 (2x2 sprite) */
#define BOSS_BULLET_BASE   25   /* OAM slots 25-28              */
#define BOSS_BULLET_COUNT  4
#define BOSS_HP_MAX        20
#define BOSS_Y_HOME        20   /* resting y position           */

typedef struct {
    uint8_t x, y;
    uint8_t active;
    uint8_t hp;
    uint8_t timer;
    uint8_t dir;        /* 0=moving left, 1=moving right */
    uint8_t phase;      /* 0=sweep, 1=dive               */
    uint8_t fire_timer;
    uint8_t fire_pat;   /* cycles 0-2                    */
    uint8_t dive_off;   /* current dive offset (0-30)    */
    uint8_t dive_up;    /* 0=descending, 1=ascending     */
} Boss;

typedef struct {
    uint8_t  x, y;
    int8_t   dx;
    uint8_t  active;
} BossBullet;

extern Boss       boss;
extern BossBullet boss_bullets[BOSS_BULLET_COUNT];

void boss_init(void);
void boss_update(void);
void boss_hit(uint8_t damage);
void boss_draw_health(void);
void boss_hide(void);

#endif
