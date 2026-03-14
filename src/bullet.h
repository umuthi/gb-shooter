#ifndef BULLET_H
#define BULLET_H

#include <gbdk/platform.h>

#define BULLET_COUNT      8
#define BULLET_OAM_BASE   1      /* OAM slots 1-8 */
#define BULLET_SPEED      4
#define BULLET_TILE       SPR_BULLET

typedef struct {
    uint8_t x;
    uint8_t y;
    uint8_t active;
} Bullet;

extern Bullet bullets[BULLET_COUNT];

void bullets_init(void);
uint8_t bullet_spawn(uint8_t x, uint8_t y);
void bullets_update(void);

#endif
