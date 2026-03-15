#ifndef COLLISION_H
#define COLLISION_H

#include <gbdk/platform.h>

/* AABB with 6x6 effective hitbox (1px inset from 8x8 sprite) */
uint8_t aabb_hit(uint8_t ax, uint8_t ay, uint8_t bx, uint8_t by);

void collision_check(void);
void collision_check_boss(void);
/* Used during final-boss phase 2: enemies + enemy bullets vs player,
   player bullets vs enemies.  Call alongside collision_check_boss(). */
void collision_check_enemies(void);

#endif
