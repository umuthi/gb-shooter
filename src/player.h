#ifndef PLAYER_H
#define PLAYER_H

#include <gbdk/platform.h>

#define PLAYER_OAM_SLOT   0
#define PLAYER_SPEED      2
#define PLAYER_LEFT_MIN   8
#define PLAYER_RIGHT_MAX  152
#define PLAYER_TOP_MIN    16
#define PLAYER_BOTTOM_MAX 128
#define PLAYER_START_X    80
#define PLAYER_START_Y    120
#define PLAYER_LIVES_MAX  5
#define PLAYER_LIVES_INIT 3
#define PLAYER_INV_FRAMES 60   /* 1 second of invincibility */

/* Difficulty levels — selected at start of each run */
#define DIFFICULTY_EASY   0
#define DIFFICULTY_NORMAL 1
#define DIFFICULTY_HARD   2
extern uint8_t difficulty;

typedef struct {
    uint8_t x;
    uint8_t y;
    uint8_t lives;
    uint8_t inv_timer;   /* invincibility countdown */
    uint8_t shoot_cooldown;
    uint8_t alive;
    uint8_t power_level;
    uint8_t bombs;
    uint8_t dev_mode;    /* 1 = developer invincibility (no damage taken) */
} Player;

extern Player player;

void player_init(void);
void player_update(uint8_t joy);
void player_hit(void);
void player_draw(void);

#endif
