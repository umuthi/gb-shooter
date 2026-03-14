#include "boss.h"
#include "tiles.h"
#include "player.h"
#include <gbdk/platform.h>

Boss       boss;
BossBullet boss_bullets[BOSS_BULLET_COUNT];

/* ---- Health bar ---- */
void boss_draw_health(void) {
    uint8_t buf[20];
    uint8_t i;
    for (i = 0; i < 20; i++)
        buf[i] = (i < boss.hp) ? TILE_BAR : TILE_BLANK;
    set_bkg_tiles(0, 0, 20, 1, buf);
}

/* ---- Bullet helpers ---- */
static void boss_bullet_spawn(uint8_t x, uint8_t y, int8_t dx) {
    uint8_t i;
    BossBullet *b;
    for (i = 0; i < BOSS_BULLET_COUNT; i++) {
        b = &boss_bullets[i];
        if (!b->active) {
            b->x      = x;
            b->y      = y;
            b->dx     = dx;
            b->active = 1;
            move_sprite(BOSS_BULLET_BASE + i, x, y);
            return;
        }
    }
}

static void boss_fire(void) {
    uint8_t cx = boss.x + 4;
    uint8_t by = boss.y + 16;

    switch (boss.fire_pat) {
    case 0:
        boss_bullet_spawn(cx, by, 0);
        break;
    case 1:
        boss_bullet_spawn(cx,      by,  0);
        boss_bullet_spawn(cx -  8, by, -1);
        boss_bullet_spawn(cx +  8, by,  1);
        break;
    case 2:
        {
            int8_t dx = 0;
            if (player.x < cx)      dx = -1;
            else if (player.x > cx) dx =  1;
            boss_bullet_spawn(cx, by, dx);
        }
        break;
    }

    boss.fire_pat++;
    if (boss.fire_pat > 2) boss.fire_pat = 0;
}

/* ---- Public API ---- */
void boss_init(void) {
    uint8_t i;

    boss.x          = 72;
    boss.y          = BOSS_Y_HOME;
    boss.active     = 1;
    boss.hp         = BOSS_HP_MAX;
    boss.timer      = 0;
    boss.dir        = 1;
    boss.phase      = 0;
    boss.fire_timer = 90;   /* first shot after 1.5s */
    boss.fire_pat   = 0;
    boss.dive_off   = 0;
    boss.dive_up    = 0;

    set_sprite_tile(BOSS_OAM_BASE + 0, SPR_BOSS_TL);
    set_sprite_tile(BOSS_OAM_BASE + 1, SPR_BOSS_TR);
    set_sprite_tile(BOSS_OAM_BASE + 2, SPR_BOSS_BL);
    set_sprite_tile(BOSS_OAM_BASE + 3, SPR_BOSS_BR);
    set_sprite_prop(BOSS_OAM_BASE + 0, 0x10U);
    set_sprite_prop(BOSS_OAM_BASE + 1, 0x10U);
    set_sprite_prop(BOSS_OAM_BASE + 2, 0x10U);
    set_sprite_prop(BOSS_OAM_BASE + 3, 0x10U);

    for (i = 0; i < BOSS_BULLET_COUNT; i++) {
        boss_bullets[i].active = 0;
        set_sprite_tile(BOSS_BULLET_BASE + i, SPR_BULLET);
        set_sprite_prop(BOSS_BULLET_BASE + i, 0x10U);
        move_sprite(BOSS_BULLET_BASE + i, 0, 0);
    }

    boss_draw_health();
}

void boss_hide(void) {
    uint8_t i;
    boss.active = 0;
    for (i = 0; i < 4; i++)
        move_sprite(BOSS_OAM_BASE + i, 0, 0);
    for (i = 0; i < BOSS_BULLET_COUNT; i++) {
        boss_bullets[i].active = 0;
        move_sprite(BOSS_BULLET_BASE + i, 0, 0);
    }
}

void boss_hit(uint8_t damage) {
    if (boss.hp <= damage + 1)
        boss.hp = (damage >= boss.hp) ? 0 : boss.hp - damage;
    else
        boss.hp -= damage;
    boss_draw_health();
}

void boss_update(void) {
    uint8_t i;
    uint8_t fire_rate;
    BossBullet *b;

    if (!boss.active) return;

    boss.timer++;

    /* --- Movement --- */
    if (boss.phase == 0) {
        /* Sweep left-right */
        if (boss.dir == 0) {
            if (boss.x > 10) boss.x--;
            else boss.dir = 1;
        } else {
            if (boss.x < 130) boss.x++;
            else boss.dir = 0;
        }
        /* Switch to dive after 210 frames */
        if (boss.timer >= 210) {
            boss.timer   = 0;
            boss.phase   = 1;
            boss.dive_off = 0;
            boss.dive_up  = 0;
        }
    } else {
        /* Dive: descend 30px then rise back */
        if (boss.dive_up == 0) {
            if (boss.dive_off < 30) boss.dive_off++;
            else boss.dive_up = 1;
        } else {
            if (boss.dive_off > 0) boss.dive_off--;
            else {
                boss.phase = 0;
                boss.timer = 0;
                boss.y     = BOSS_Y_HOME;
            }
        }
        boss.y = BOSS_Y_HOME + boss.dive_off;
    }

    /* Position 4 boss sprites */
    move_sprite(BOSS_OAM_BASE + 0, boss.x,     boss.y);
    move_sprite(BOSS_OAM_BASE + 1, boss.x + 8, boss.y);
    move_sprite(BOSS_OAM_BASE + 2, boss.x,     boss.y + 8);
    move_sprite(BOSS_OAM_BASE + 3, boss.x + 8, boss.y + 8);

    /* --- Firing --- */
    fire_rate = (boss.hp <= 7) ? 40 : 70;
    if (boss.fire_timer > 0) boss.fire_timer--;
    else {
        boss_fire();
        boss.fire_timer = fire_rate;
    }

    /* --- Boss bullets --- */
    for (i = 0; i < BOSS_BULLET_COUNT; i++) {
        b = &boss_bullets[i];
        if (!b->active) continue;
        b->y += 2;
        b->x  = (uint8_t)((int8_t)b->x + b->dx);
        if (b->y > 152) {
            b->active = 0;
            move_sprite(BOSS_BULLET_BASE + i, 0, 0);
        } else {
            move_sprite(BOSS_BULLET_BASE + i, b->x, b->y);
        }
    }
}
