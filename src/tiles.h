#ifndef TILES_H
#define TILES_H

#include <gbdk/platform.h>

/* BG / Window tile indices */
#define TILE_BLANK        0   /* all color-0 pixels → black space background */
#define TILE_STAR_SM      1
#define TILE_STAR_LG      2
#define TILE_DIGIT_0      3   /* digits 3-12 */
#define TILE_HEART        13  /* used as WIN tile in HUD strip */
#define TILE_BLACK        14  /* all color-3 pixels → solid black (overlay bg) */
#define TILE_DASH         15  /* horizontal divider line */

/* Letter tiles */
#define TILE_LETTER_A     16
#define TILE_LETTER_E     17
#define TILE_LETTER_G     18
#define TILE_LETTER_H     19
#define TILE_LETTER_M     20
#define TILE_LETTER_O     21
#define TILE_LETTER_P     22
#define TILE_LETTER_R     23
#define TILE_LETTER_S     24
#define TILE_LETTER_T     25
#define TILE_LETTER_V     26
#define TILE_LETTER_B     27  /* for HUD bomb display */
#define TILE_LETTER_U     28
#define TILE_LETTER_D     29
#define TILE_BAR          30  /* solid block for boss health bar */

/* Dialogue letter tiles — added for cutscene system */
#define TILE_LETTER_C     31
#define TILE_LETTER_F     32
#define TILE_LETTER_I     33
#define TILE_LETTER_K     34
#define TILE_LETTER_L     35
#define TILE_LETTER_N     36
#define TILE_LETTER_W     37
#define TILE_LETTER_Y     38
#define TILE_PERIOD       39
#define TILE_EXCLAIM      40
#define TILE_LETTER_X     41
#define TILE_LETTER_Z     42

/* Sprite tile indices (loaded into sprite VRAM separately) */
#define SPR_PLAYER        0
#define SPR_BULLET        1
#define SPR_ENEMY_A       2
#define SPR_ENEMY_B       3
#define SPR_EXPLOSION_0   4
#define SPR_EXPLOSION_1   5
#define SPR_HEART         6   /* heart icon — used as BG/WIN tile in HUD */
#define SPR_PICKUP_POWER  7
#define SPR_PICKUP_BOMB   8
#define SPR_BOSS_TL       9   /* boss 2x2 metasprite — top-left */
#define SPR_BOSS_TR       10
#define SPR_BOSS_BL       11
#define SPR_BOSS_BR       12

extern const uint8_t bg_tiles[];
extern const uint8_t sprite_tiles[];

#define BG_TILES_COUNT    43
#define SPR_TILES_COUNT   13

void tiles_load(void);

#endif
