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

/* ---- Fire patterns: boss 1 (3 patterns, cycles 0-2) ---- */
static void boss_fire_1(void) {
    uint8_t cx = boss.x + 4;
    uint8_t by = boss.y + 16;

    switch (boss.fire_pat) {
    case 0:
        boss_bullet_spawn(cx, by, 0);
        break;
    case 1:
        boss_bullet_spawn(cx,     by,  0);
        boss_bullet_spawn(cx - 8, by, -1);
        boss_bullet_spawn(cx + 8, by,  1);
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

/* ---- Fire patterns: boss 2 (5 patterns, cycles 0-4) ---- */
static void boss_fire_2(void) {
    uint8_t cx = boss.x + 4;
    uint8_t by = boss.y + 16;

    switch (boss.fire_pat) {
    case 0: /* 5-way fan spread */
        boss_bullet_spawn(cx - 12, by, -2);
        boss_bullet_spawn(cx - 6,  by, -1);
        boss_bullet_spawn(cx,      by,  0);
        boss_bullet_spawn(cx + 6,  by,  1);
        boss_bullet_spawn(cx + 12, by,  2);
        break;
    case 1: /* aimed triple with ±1 spread */
        {
            int8_t dx = 0;
            if (player.x < cx - 4)      dx = -1;
            else if (player.x > cx + 4) dx =  1;
            boss_bullet_spawn(cx,     by, dx);
            boss_bullet_spawn(cx - 6, by, (int8_t)(dx - 1));
            boss_bullet_spawn(cx + 6, by, (int8_t)(dx + 1));
        }
        break;
    case 2: /* side burst — fire outward from boss edges */
        boss_bullet_spawn(boss.x,      by,  1);
        boss_bullet_spawn(boss.x + 8,  by,  2);
        boss_bullet_spawn(boss.x + 8,  by, -1);
        boss_bullet_spawn(boss.x + 16, by, -2);
        break;
    case 3: /* double aimed chase shot */
        {
            int8_t dx = 0;
            if (player.x < cx - 4)      dx = -2;
            else if (player.x > cx + 4) dx =  2;
            boss_bullet_spawn(cx - 5, by, dx);
            boss_bullet_spawn(cx + 5, by, dx);
        }
        break;
    case 4: /* sweeping barrage across full width */
        boss_bullet_spawn(cx - 12, by, -1);
        boss_bullet_spawn(cx - 4,  by,  0);
        boss_bullet_spawn(cx + 4,  by,  0);
        boss_bullet_spawn(cx + 12, by,  1);
        break;
    }
    boss.fire_pat++;
    if (boss.fire_pat > 4) boss.fire_pat = 0;
}

/* ---- Fire patterns: boss 3 / final boss (6 patterns, cycles 0-5) ---- */
static void boss_fire_3(void) {
    uint8_t cx = boss.x + 4;
    uint8_t by = boss.y + 16;
    int8_t  dx;

    switch (boss.fire_pat) {
    case 0: /* V-fan: 6 bullets, no center gap — intimidating opening */
        boss_bullet_spawn(cx - 14, by, -3);
        boss_bullet_spawn(cx - 8,  by, -2);
        boss_bullet_spawn(cx - 3,  by, -1);
        boss_bullet_spawn(cx + 3,  by,  1);
        boss_bullet_spawn(cx + 8,  by,  2);
        boss_bullet_spawn(cx + 14, by,  3);
        break;

    case 1: /* Aimed 5-burst: tight homing cluster aimed at player */
        dx = 0;
        if (player.x < cx - 4)      dx = -1;
        else if (player.x > cx + 4) dx =  1;
        boss_bullet_spawn(cx,     by, (int8_t)(dx - 2));
        boss_bullet_spawn(cx,     by, (int8_t)(dx - 1));
        boss_bullet_spawn(cx,     by, dx);
        boss_bullet_spawn(cx,     by, (int8_t)(dx + 1));
        boss_bullet_spawn(cx,     by, (int8_t)(dx + 2));
        break;

    case 2: /* Rapid double aimed: two stacked bullets straight at player */
        dx = 0;
        if (player.x < cx - 4)      dx = -2;
        else if (player.x > cx + 4) dx =  2;
        boss_bullet_spawn(cx - 4, by,                        dx);
        boss_bullet_spawn(cx + 4, (uint8_t)(by + 5),         dx);
        break;

    case 3: /* Outward burst: bullets fly away from center */
        boss_bullet_spawn(boss.x,      by, -3);
        boss_bullet_spawn(boss.x + 4,  by, -2);
        boss_bullet_spawn(boss.x + 8,  by,  0);
        boss_bullet_spawn(boss.x + 12, by,  0);
        boss_bullet_spawn(boss.x + 16, by,  2);
        boss_bullet_spawn(boss.x + 20, by,  3);
        break;

    case 4: /* Converging: bullets angle toward center lane */
        boss_bullet_spawn(cx - 14, by,  2);
        boss_bullet_spawn(cx - 8,  by,  1);
        boss_bullet_spawn(cx,      by,  0);
        boss_bullet_spawn(cx + 8,  by, -1);
        boss_bullet_spawn(cx + 14, by, -2);
        break;

    case 5: /* Wall of death: 7 columns across the full screen width */
        boss_bullet_spawn(16,  by, 0);
        boss_bullet_spawn(32,  by, 0);
        boss_bullet_spawn(56,  by, 0);
        boss_bullet_spawn(80,  by, 0);
        boss_bullet_spawn(104, by, 0);
        boss_bullet_spawn(128, by, 0);
        boss_bullet_spawn(144, by, 0);
        break;
    }
    boss.fire_pat++;
    if (boss.fire_pat > 5) boss.fire_pat = 0;
}

/* ---- Public API ---- */
void boss_init(uint8_t bnum) {
    uint8_t i;

    boss.x           = 72;
    boss.y           = BOSS_Y_HOME;
    boss.active      = 1;
    boss.hp          = BOSS_HP_MAX;
    boss.timer       = 0;
    boss.dir         = 1;
    boss.phase       = 0;
    boss.fire_timer  = 90;
    boss.fire_pat    = 0;
    boss.dive_off    = 0;
    boss.dive_up     = 0;
    boss.dying       = 0;
    boss.death_timer = 0;
    boss.boss_num    = bnum;
    boss.phase2      = 0;

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
    if (boss.hp == 0 || boss.dying) return;
    if (damage >= boss.hp)
        boss.hp = 0;
    else
        boss.hp -= damage;
    boss_draw_health();
}

void boss_update(void) {
    uint8_t i;
    uint8_t fire_rate;
    uint8_t shift;
    BossBullet *b;

    if (!boss.active) return;

    /* ---- Death animation ---- */
    if (boss.dying) {
        if      (boss.death_timer > 60) shift = 3;
        else if (boss.death_timer > 30) shift = 2;
        else if (boss.death_timer > 10) shift = 1;
        else                            shift = 0;

        if ((boss.death_timer >> shift) & 1U) {
            move_sprite(BOSS_OAM_BASE + 0, boss.x,     boss.y);
            move_sprite(BOSS_OAM_BASE + 1, boss.x + 8, boss.y);
            move_sprite(BOSS_OAM_BASE + 2, boss.x,     boss.y + 8);
            move_sprite(BOSS_OAM_BASE + 3, boss.x + 8, boss.y + 8);
        } else {
            move_sprite(BOSS_OAM_BASE + 0, 0, 0);
            move_sprite(BOSS_OAM_BASE + 1, 0, 0);
            move_sprite(BOSS_OAM_BASE + 2, 0, 0);
            move_sprite(BOSS_OAM_BASE + 3, 0, 0);
        }

        if (boss.death_timer > 0) {
            boss.death_timer--;
        } else {
            boss_hide();
            boss.dying = 0;
        }
        return;
    }

    /* ---- Detect death ---- */
    if (boss.hp == 0) {
        /* Final boss first death: replenish HP and enter phase 2 */
        if (boss.boss_num == 3 && !boss.phase2) {
            boss.phase2 = 1;
            boss.hp     = BOSS_HP_MAX;
            boss_draw_health();
            /* Reset movement to sweep so charge pattern restarts cleanly */
            boss.phase    = 0;
            boss.timer    = 0;
            boss.dir      = 1;
            boss.fire_pat = 0;
            boss.fire_timer = 30; /* short pause before resuming fire */
            return;
        }
        /* Normal death — start flashing animation */
        boss.dying       = 1;
        boss.death_timer = 90;
        for (i = 0; i < BOSS_BULLET_COUNT; i++) {
            boss_bullets[i].active = 0;
            move_sprite(BOSS_BULLET_BASE + i, 0, 0);
        }
        return;
    }

    boss.timer++;

    /* ---- Movement ---- */
    if (boss.phase == 0) {
        /* Sweep left-right */
        if (boss.dir == 0) {
            if (boss.x > 10) boss.x--;
            else boss.dir = 1;
        } else {
            if (boss.x < 130) boss.x++;
            else boss.dir = 0;
        }

        /* Final boss sweeps faster and switches to charge sooner */
        {
            uint8_t sweep_limit = (boss.boss_num == 3) ? 100 : 210;
            if (boss.timer >= sweep_limit) {
                boss.timer    = 0;
                boss.phase    = 1;
                boss.dive_off = 0;
                boss.dive_up  = 0;
                /* Final boss: lock charge direction toward player */
                if (boss.boss_num == 3) {
                    boss.dir = (player.x > boss.x + 8) ? 1 : 0;
                }
            }
        }
    } else {
        /* Phase 1: gentle dive (boss 1/2) or charge dive (boss 3) */
        uint8_t max_dive  = (boss.boss_num == 3) ? 120 : 30;
        uint8_t dive_spd  = (boss.boss_num == 3) ? 3   : 1;

        if (boss.dive_up == 0) {
            if (boss.dive_off < max_dive) boss.dive_off += dive_spd;
            else boss.dive_up = 1;
        } else {
            if (boss.dive_off >= dive_spd) boss.dive_off -= dive_spd;
            else {
                boss.dive_off = 0;
                boss.phase    = 0;
                boss.timer    = 0;
                boss.y        = BOSS_Y_HOME;
                boss.dir      = 1; /* reset to sweep right after charge */
            }
        }
        boss.y = BOSS_Y_HOME + boss.dive_off;

        /* Final boss also moves horizontally during charge (dir=0 left, dir=1 right) */
        if (boss.boss_num == 3) {
            if (boss.dir == 0) {
                if (boss.x > 10) boss.x -= 2;
            } else {
                if (boss.x < 130) boss.x += 2;
            }
        }
    }

    /* Position 4 boss sprites */
    move_sprite(BOSS_OAM_BASE + 0, boss.x,     boss.y);
    move_sprite(BOSS_OAM_BASE + 1, boss.x + 8, boss.y);
    move_sprite(BOSS_OAM_BASE + 2, boss.x,     boss.y + 8);
    move_sprite(BOSS_OAM_BASE + 3, boss.x + 8, boss.y + 8);

    /* ---- Firing ---- */
    if (boss.boss_num == 3)
        fire_rate = (boss.hp <= 10) ? 25 : 40;
    else if (boss.boss_num == 2)
        fire_rate = (boss.hp <= 10) ? 30 : 50;
    else
        fire_rate = (boss.hp <= 7)  ? 40 : 70;

    if (boss.fire_timer > 0) boss.fire_timer--;
    else {
        if      (boss.boss_num == 3) boss_fire_3();
        else if (boss.boss_num == 2) boss_fire_2();
        else                         boss_fire_1();
        boss.fire_timer = fire_rate;
    }

    /* ---- Boss bullets ---- */
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
