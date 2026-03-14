#ifndef ENEMY_H
#define ENEMY_H

#include <gbdk/platform.h>

#define ENEMY_COUNT       8
#define ENEMY_OAM_BASE    9      /* OAM slots 9-16 */

/* Movement pattern types */
#define PATTERN_STRAIGHT  0
#define PATTERN_ZIGZAG    1
#define PATTERN_SWOOP     2
#define PATTERN_DIAGONAL  3
#define PATTERN_SINE      4

typedef struct {
    uint8_t x;
    uint8_t y;
    uint8_t active;
    uint8_t pattern;
    uint8_t timer;       /* used for pattern phases */
    uint8_t dir;         /* horizontal direction for zigzag */
    int8_t  speed_x;     /* signed horizontal component */
    uint8_t speed_y;
    uint8_t tile;        /* which sprite tile */
} Enemy;

extern Enemy enemies[ENEMY_COUNT];
extern uint8_t wave_num;
extern uint8_t enemies_alive;

void enemies_init(void);
void enemies_spawn_wave(void);
void enemies_update(void);

#endif
