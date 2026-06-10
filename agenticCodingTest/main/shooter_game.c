#include "shooter_game.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "board_config.h"
#include "esp_check.h"
#include "esp_log.h"
#include "nvs.h"
#include "shooter_sprites.h"

static const char *TAG = "shooter_game";

// ---------------------------------------------------------------
// Layout
// ---------------------------------------------------------------
#define DISP_W          BOARD_DISPLAY_WIDTH
#define DISP_H          BOARD_DISPLAY_HEIGHT
#define HUD_H           14
#define GROUND_Y        208
#define SPR_SCALE       2u

#define ROBOT_DRAW_W    (ROBOT_SPR_W * SPR_SCALE)
#define ROBOT_DRAW_H    (ROBOT_SPR_H * SPR_SCALE)
#define EBOT_DRAW_W     (EBOT_SPR_W  * SPR_SCALE)
#define EBOT_DRAW_H     (EBOT_SPR_H  * SPR_SCALE)
#define DRONE_DRAW_W    (DRONE_SPR_W * SPR_SCALE)
#define DRONE_DRAW_H    (DRONE_SPR_H * SPR_SCALE)
#define COIN_DRAW_W     (COIN_SPR_W  * SPR_SCALE)
#define COIN_DRAW_H     (COIN_SPR_H  * SPR_SCALE)
#define ROCK_DRAW_W     (ROCK_SPR_W  * SPR_SCALE)
#define ROCK_DRAW_H     (ROCK_SPR_H  * SPR_SCALE)
#define CRATE_DRAW_W    (CRATE_SPR_W * SPR_SCALE)
#define CRATE_DRAW_H    (CRATE_SPR_H * SPR_SCALE)

#define PLAYER_FIXED_X  18
#define PLAYER_GND_Y    (GROUND_Y - ROBOT_DRAW_H)

#define SPAWN_X         (DISP_W + 4)

// Physics
#define JUMP_VY         (-7.5f)
#define GRAVITY         (0.5f)

// Shooting
#define PROJ_W          8
#define PROJ_H          4
#define PROJ_SPEED      5.0f
#define SHOOT_COOLDOWN  250u

// Animation intervals (ms)
#define RUN_ANIM_MS     150u
#define COIN_ANIM_MS    120u
#define ENEMY_ANIM_MS   160u

// Invulnerability
#define INVULN_MS       2000u
#define BLINK_MS        100u

// Difficulty
#define DIFF_INTERVAL_MS    30000u
#define SCROLL_SPEED_START  2.0f
#define SCROLL_SPEED_MAX    4.0f
#define SCROLL_SPEED_STEP   0.10f

// Score
#define SCORE_COIN          1
#define SCORE_ENEMY         10
#define SCORE_BREAKABLE     5
#define DIST_SCORE_MS       1000u

// Spawn base intervals (ms)
#define SPAWN_COIN_BASE     2000u
#define SPAWN_ENEMY_BASE    3000u
#define SPAWN_OBS_BASE      3500u

// ---------------------------------------------------------------
// Colors (RGB565)
// ---------------------------------------------------------------
#define C_SKY1      0x0210u
#define C_SKY2      0x0435u
#define C_SKY3      0x0859u
#define C_GROUND    0x2C05u
#define C_GROUNDTOP 0x2E03u
#define C_HUD_BG    0x0000u
#define C_WHITE     0xFFFFu
#define C_YELLOW    0xFFE0u
#define C_RED       0xF800u
#define C_CYAN      0x07FFu
#define C_ORANGE    0xFD20u
#define C_GOLD      0xFEA0u
#define C_GRAY      0x8410u
#define C_TITLE_BG  0x0820u
#define C_PROJ_TRAIL 0x035Fu

// 8-direction vectors (pre-computed, no math.h needed)
static const float DIR_X[8] = { 1.0f, 0.71f, 0.0f,-0.71f,-1.0f,-0.71f, 0.0f, 0.71f};
static const float DIR_Y[8] = { 0.0f, 0.71f, 1.0f, 0.71f, 0.0f,-0.71f,-1.0f,-0.71f};

// ---------------------------------------------------------------
// NVS
// ---------------------------------------------------------------
static void nvs_load_hi(int32_t *hi)
{
    *hi = 0;
    nvs_handle_t h;
    if (nvs_open("tbr", NVS_READONLY, &h) != ESP_OK) return;
    uint32_t v = 0;
    nvs_get_u32(h, "hi", &v);
    *hi = (int32_t)v;
    nvs_close(h);
}

static void nvs_save_hi(int32_t hi)
{
    nvs_handle_t h;
    if (nvs_open("tbr", NVS_READWRITE, &h) != ESP_OK) return;
    nvs_set_u32(h, "hi", (uint32_t)hi);
    nvs_commit(h);
    nvs_close(h);
}

// ---------------------------------------------------------------
// RNG
// ---------------------------------------------------------------
static uint32_t rng_next(uint32_t *s)
{
    *s = (*s * 1664525u) + 1013904223u;
    return *s;
}

static int32_t rng_range(uint32_t *s, int32_t lo, int32_t hi)
{
    if (hi <= lo) return lo;
    return lo + (int32_t)(rng_next(s) % (uint32_t)(hi - lo + 1));
}

// ---------------------------------------------------------------
// Particle helpers
// ---------------------------------------------------------------
static sg_particle_t *alloc_particle(shooter_game_t *g)
{
    for (int i = 0; i < SG_MAX_PARTICLES; i++) {
        if (!g->particles[i].active) return &g->particles[i];
    }
    return NULL;
}

static void spawn_explosion(shooter_game_t *g, float cx, float cy)
{
    static const uint16_t cols[4] = {C_ORANGE, C_YELLOW, C_RED, C_WHITE};
    for (int i = 0; i < 8; i++) {
        sg_particle_t *p = alloc_particle(g);
        if (!p) break;
        float spd = 1.5f + (float)(rng_next(&g->rng) % 200) / 100.0f;
        p->active       = true;
        p->type         = SG_PART_EXPLOSION;
        p->x            = cx;
        p->y            = cy;
        p->vx           = DIR_X[i] * spd;
        p->vy           = DIR_Y[i] * spd - 0.5f;
        p->life_ms      = 300u + (rng_next(&g->rng) % 200u);
        p->max_life_ms  = p->life_ms;
        p->color        = cols[i % 4];
        p->value        = 0;
    }
}

static void spawn_sparkle(shooter_game_t *g, float cx, float cy)
{
    for (int i = 0; i < 5; i++) {
        sg_particle_t *p = alloc_particle(g);
        if (!p) break;
        p->active       = true;
        p->type         = SG_PART_SPARKLE;
        p->x            = cx + (float)rng_range(&g->rng, -6, 6);
        p->y            = cy + (float)rng_range(&g->rng, -4, 4);
        p->vx           = (float)rng_range(&g->rng, -200, 200) / 100.0f;
        p->vy           = -1.5f - (float)(rng_next(&g->rng) % 150u) / 100.0f;
        p->life_ms      = 250u + (rng_next(&g->rng) % 150u);
        p->max_life_ms  = p->life_ms;
        p->color        = C_GOLD;
        p->value        = 0;
    }
}

static void spawn_dust(shooter_game_t *g, float cx, float cy)
{
    for (int i = 0; i < 4; i++) {
        sg_particle_t *p = alloc_particle(g);
        if (!p) break;
        p->active       = true;
        p->type         = SG_PART_DUST;
        p->x            = cx + (float)rng_range(&g->rng, -8, 8);
        p->y            = cy;
        p->vx           = (float)rng_range(&g->rng, -150, 150) / 100.0f;
        p->vy           = -0.5f - (float)(rng_next(&g->rng) % 100u) / 100.0f;
        p->life_ms      = 200u + (rng_next(&g->rng) % 100u);
        p->max_life_ms  = p->life_ms;
        p->color        = C_GRAY;
        p->value        = 0;
    }
}

static void spawn_score_text(shooter_game_t *g, float cx, float cy, int value)
{
    sg_particle_t *p = alloc_particle(g);
    if (!p) return;
    p->active       = true;
    p->type         = SG_PART_SCORE_TEXT;
    p->x            = cx;
    p->y            = cy;
    p->vx           = 0.0f;
    p->vy           = -0.8f;
    p->life_ms      = 600u;
    p->max_life_ms  = 600u;
    p->color        = C_YELLOW;
    p->value        = value;
}

// ---------------------------------------------------------------
// Reset gameplay state
// ---------------------------------------------------------------
static void reset_play(shooter_game_t *g)
{
    memset(g->projectiles, 0, sizeof(g->projectiles));
    memset(g->enemies,     0, sizeof(g->enemies));
    memset(g->coins,       0, sizeof(g->coins));
    memset(g->obstacles,   0, sizeof(g->obstacles));
    memset(g->particles,   0, sizeof(g->particles));

    g->player = (sg_player_t){
        .x                  = PLAYER_FIXED_X,
        .y                  = (float)PLAYER_GND_Y,
        .vy                 = 0.0f,
        .in_air             = false,
        .invuln             = false,
        .invuln_ms          = 0,
        .visible            = true,
        .blink_ms           = 0,
        .run_frame          = 0,
        .run_timer_ms       = 0,
        .shoot_cooldown_ms  = 0,
        .left_held          = false,
        .left_hold_ms       = 0,
        .shooting_anim      = false,
        .shoot_anim_ms      = 0,
    };

    g->score               = 0;
    g->lives               = 3;
    g->scroll_speed        = SCROLL_SPEED_START;
    g->game_ms             = 0;
    g->last_coin_ms        = 500u;
    g->last_enemy_ms       = 1500u;
    g->last_obstacle_ms    = 2000u;
    g->last_dist_score_ms  = 0;
}

// ---------------------------------------------------------------
// AABB collision
// ---------------------------------------------------------------
static bool aabb(float ax, float ay, float aw, float ah,
                 float bx, float by, float bw, float bh)
{
    return ax < bx + bw && ax + aw > bx &&
           ay < by + bh && ay + ah > by;
}

// ---------------------------------------------------------------
// Spawn helpers
// ---------------------------------------------------------------
static void try_spawn_coin(shooter_game_t *g)
{
    int   count = rng_range(&g->rng, 2, 4);
    float y_pos;
    int   pattern = (int)(rng_next(&g->rng) % 3u);
    switch (pattern) {
    case 0:  y_pos = (float)(GROUND_Y - COIN_DRAW_H - 2);  break;
    case 1:  y_pos = (float)(PLAYER_GND_Y - 22);           break;
    default: y_pos = (float)(PLAYER_GND_Y - 44);           break;
    }
    int spawned = 0;
    for (int i = 0; i < SG_MAX_COINS && spawned < count; i++) {
        if (!g->coins[i].active) {
            g->coins[i] = (sg_coin_t){
                .active     = true,
                .x          = (float)(SPAWN_X + spawned * (COIN_DRAW_W + 4)),
                .y          = y_pos,
                .anim_frame = 0,
                .anim_ms    = 0,
            };
            spawned++;
        }
    }
}

static void try_spawn_enemy(shooter_game_t *g)
{
    sg_enemy_type_t type = (rng_next(&g->rng) & 1u) ? SG_ENEMY_FLYING_DRONE
                                                      : SG_ENEMY_GROUND_BOT;
    float y;
    if (type == SG_ENEMY_GROUND_BOT) {
        y = (float)(GROUND_Y - EBOT_DRAW_H);
    } else {
        y = (float)rng_range(&g->rng, HUD_H + 30, 170);
    }
    for (int i = 0; i < SG_MAX_ENEMIES; i++) {
        if (!g->enemies[i].active) {
            g->enemies[i] = (sg_enemy_t){
                .active     = true,
                .type       = type,
                .x          = (float)SPAWN_X,
                .y          = y,
                .anim_frame = 0,
                .anim_ms    = 0,
            };
            return;
        }
    }
}

static void try_spawn_obstacle(shooter_game_t *g)
{
    sg_obs_type_t type = (rng_next(&g->rng) & 1u) ? SG_OBS_CRATE : SG_OBS_ROCK;
    float y = (type == SG_OBS_ROCK)
              ? (float)(GROUND_Y - ROCK_DRAW_H)
              : (float)(GROUND_Y - CRATE_DRAW_H);
    for (int i = 0; i < SG_MAX_OBSTACLES; i++) {
        if (!g->obstacles[i].active) {
            g->obstacles[i] = (sg_obstacle_t){
                .active  = true,
                .type    = type,
                .health  = 1,
                .x       = (float)SPAWN_X,
                .y       = y,
            };
            return;
        }
    }
}

static void player_take_damage(shooter_game_t *g)
{
    if (g->player.invuln) return;
    g->lives--;
    g->player.invuln    = true;
    g->player.invuln_ms = INVULN_MS;
    g->player.blink_ms  = 0;
    g->player.visible   = true;
}

// ---------------------------------------------------------------
// Init
// ---------------------------------------------------------------
esp_err_t shooter_game_init(shooter_game_t *game, graphics_t *graphics)
{
    memset(game, 0, sizeof(*game));
    game->graphics = graphics;
    game->state    = SG_STATE_SPLASH;
    game->state_ms = 0;
    game->rng      = 0xDEADBEEFu;

    nvs_load_hi(&game->hi_score);
    reset_play(game);

    ESP_LOGI(TAG, "Tiny Blaster Runner init, hi=%ld", (long)game->hi_score);
    return ESP_OK;
}

// ---------------------------------------------------------------
// Input
// ---------------------------------------------------------------
esp_err_t shooter_game_handle_input(shooter_game_t *game,
                                    const button_event_t *events,
                                    size_t event_count)
{
    for (size_t i = 0; i < event_count; i++) {
        const button_event_t *ev = &events[i];

        if (game->state == SG_STATE_SPLASH) {
            if (ev->type == BUTTON_EVENT_PRESSED) {
                game->state    = SG_STATE_MENU;
                game->state_ms = 0;
            }
            continue;
        }

        if (game->state == SG_STATE_MENU) {
            if (ev->type == BUTTON_EVENT_PRESSED && ev->id == BUTTON_ID_LEFT) {
                reset_play(game);
                game->state    = SG_STATE_PLAY;
                game->state_ms = 0;
            }
            continue;
        }

        if (game->state == SG_STATE_PLAY) {
            sg_player_t *p = &game->player;

            if (ev->type == BUTTON_EVENT_PRESSED && ev->id == BUTTON_ID_LEFT) {
                if (!p->in_air) {
                    p->vy           = JUMP_VY;
                    p->in_air       = true;
                    p->left_held    = true;
                    p->left_hold_ms = 0;
                    spawn_dust(game, p->x + ROBOT_DRAW_W / 2.0f, (float)GROUND_Y);
                }
            }
            if (ev->type == BUTTON_EVENT_RELEASED && ev->id == BUTTON_ID_LEFT) {
                p->left_held = false;
            }

            if (ev->type == BUTTON_EVENT_PRESSED && ev->id == BUTTON_ID_RIGHT) {
                if (p->shoot_cooldown_ms == 0) {
                    for (int pi = 0; pi < SG_MAX_PROJECTILES; pi++) {
                        if (!game->projectiles[pi].active) {
                            game->projectiles[pi] = (sg_projectile_t){
                                .active = true,
                                .x      = p->x + ROBOT_DRAW_W,
                                .y      = p->y + (float)ROBOT_DRAW_H / 2.0f - (float)PROJ_H / 2.0f,
                            };
                            break;
                        }
                    }
                    p->shoot_cooldown_ms = SHOOT_COOLDOWN;
                    p->shooting_anim     = true;
                    p->shoot_anim_ms     = 80u;
                }
            }
            continue;
        }

        if (game->state == SG_STATE_GAMEOVER) {
            if (ev->type == BUTTON_EVENT_PRESSED) {
                if (ev->id == BUTTON_ID_LEFT) {
                    reset_play(game);
                    game->state    = SG_STATE_PLAY;
                    game->state_ms = 0;
                } else {
                    game->state    = SG_STATE_MENU;
                    game->state_ms = 0;
                }
            }
            continue;
        }
    }
    return ESP_OK;
}

// ---------------------------------------------------------------
// Update
// ---------------------------------------------------------------
esp_err_t shooter_game_update(shooter_game_t *game, uint32_t frame, uint32_t dt_ms)
{
    (void)frame;
    game->state_ms += dt_ms;

    if (game->state == SG_STATE_SPLASH) {
        if (game->state_ms >= 2500u) {
            game->state    = SG_STATE_MENU;
            game->state_ms = 0;
        }
        return ESP_OK;
    }

    if (game->state != SG_STATE_PLAY) return ESP_OK;

    game->game_ms += dt_ms;
    sg_player_t *p = &game->player;

    // Variable jump boost while holding
    if (p->in_air && p->left_held) {
        p->left_hold_ms += dt_ms;
        if (p->left_hold_ms < 250u && p->vy < 0.0f) {
            p->vy -= 0.04f;
            if (p->vy < -9.0f) p->vy = -9.0f;
        }
    }

    // Jump physics
    p->vy += GRAVITY;
    p->y  += p->vy;
    if (p->y >= (float)PLAYER_GND_Y) {
        if (p->in_air) spawn_dust(game, p->x + ROBOT_DRAW_W / 2.0f, (float)GROUND_Y);
        p->y      = (float)PLAYER_GND_Y;
        p->vy     = 0.0f;
        p->in_air = false;
    }

    // Run animation
    if (!p->in_air) {
        p->run_timer_ms += dt_ms;
        if (p->run_timer_ms >= RUN_ANIM_MS) {
            p->run_timer_ms = 0;
            p->run_frame    = 1 - p->run_frame;
        }
    }

    // Shoot cooldown + anim
    if (p->shoot_cooldown_ms > dt_ms) p->shoot_cooldown_ms -= dt_ms;
    else                               p->shoot_cooldown_ms  = 0;
    if (p->shoot_anim_ms > dt_ms) {
        p->shoot_anim_ms -= dt_ms;
    } else {
        p->shoot_anim_ms  = 0;
        p->shooting_anim  = false;
    }

    // Invulnerability blink
    if (p->invuln) {
        if (p->invuln_ms > dt_ms) p->invuln_ms -= dt_ms;
        else                       p->invuln_ms  = 0;
        p->blink_ms += dt_ms;
        if (p->blink_ms >= BLINK_MS) {
            p->blink_ms = 0;
            p->visible  = !p->visible;
        }
        if (p->invuln_ms == 0) {
            p->invuln  = false;
            p->visible = true;
        }
    }

    // Projectiles
    for (int i = 0; i < SG_MAX_PROJECTILES; i++) {
        if (!game->projectiles[i].active) continue;
        game->projectiles[i].x += PROJ_SPEED;
        if (game->projectiles[i].x > (float)DISP_W) game->projectiles[i].active = false;
    }

    // Enemies
    for (int i = 0; i < SG_MAX_ENEMIES; i++) {
        sg_enemy_t *e = &game->enemies[i];
        if (!e->active) continue;
        float spd = game->scroll_speed * ((e->type == SG_ENEMY_FLYING_DRONE) ? 1.15f : 1.0f);
        e->x -= spd;
        e->anim_ms += dt_ms;
        if (e->anim_ms >= ENEMY_ANIM_MS) {
            e->anim_ms   = 0;
            e->anim_frame = 1 - e->anim_frame;
        }
        if (e->x + (float)EBOT_DRAW_W < 0.0f) e->active = false;
    }

    // Coins
    for (int i = 0; i < SG_MAX_COINS; i++) {
        sg_coin_t *c = &game->coins[i];
        if (!c->active) continue;
        c->x -= game->scroll_speed;
        c->anim_ms += dt_ms;
        if (c->anim_ms >= COIN_ANIM_MS) {
            c->anim_ms    = 0;
            c->anim_frame = (c->anim_frame + 1) % 4;
        }
        if (c->x + (float)COIN_DRAW_W < 0.0f) c->active = false;
    }

    // Obstacles
    for (int i = 0; i < SG_MAX_OBSTACLES; i++) {
        sg_obstacle_t *o = &game->obstacles[i];
        if (!o->active) continue;
        o->x -= game->scroll_speed;
        if (o->x + (float)CRATE_DRAW_W < 0.0f) o->active = false;
    }

    // Particles
    for (int i = 0; i < SG_MAX_PARTICLES; i++) {
        sg_particle_t *pt = &game->particles[i];
        if (!pt->active) continue;
        pt->x += pt->vx;
        pt->y += pt->vy;
        if (pt->type != SG_PART_SPARKLE && pt->type != SG_PART_SCORE_TEXT) pt->vy += 0.1f;
        if (pt->life_ms > dt_ms) pt->life_ms -= dt_ms;
        else                      pt->active = false;
    }

    // Collisions: projectile vs enemy
    for (int pi = 0; pi < SG_MAX_PROJECTILES; pi++) {
        if (!game->projectiles[pi].active) continue;
        for (int ei = 0; ei < SG_MAX_ENEMIES; ei++) {
            sg_enemy_t *e = &game->enemies[ei];
            if (!e->active) continue;
            float ew = (float)EBOT_DRAW_W;
            float eh = (float)EBOT_DRAW_H;
            if (aabb(game->projectiles[pi].x, game->projectiles[pi].y, PROJ_W, PROJ_H,
                     e->x, e->y, ew, eh)) {
                game->projectiles[pi].active = false;
                e->active = false;
                game->score += SCORE_ENEMY;
                spawn_explosion(game, e->x + ew / 2.0f, e->y + eh / 2.0f);
                spawn_score_text(game, e->x, e->y - 10.0f, SCORE_ENEMY);
            }
        }
    }

    // Collisions: projectile vs obstacle
    for (int pi = 0; pi < SG_MAX_PROJECTILES; pi++) {
        if (!game->projectiles[pi].active) continue;
        for (int oi = 0; oi < SG_MAX_OBSTACLES; oi++) {
            sg_obstacle_t *o = &game->obstacles[oi];
            if (!o->active) continue;
            float ow = (o->type == SG_OBS_ROCK) ? (float)ROCK_DRAW_W  : (float)CRATE_DRAW_W;
            float oh = (o->type == SG_OBS_ROCK) ? (float)ROCK_DRAW_H  : (float)CRATE_DRAW_H;
            if (aabb(game->projectiles[pi].x, game->projectiles[pi].y, PROJ_W, PROJ_H,
                     o->x, o->y, ow, oh)) {
                game->projectiles[pi].active = false;
                if (o->type == SG_OBS_CRATE) {
                    o->health--;
                    if (o->health <= 0) {
                        o->active = false;
                        game->score += SCORE_BREAKABLE;
                        spawn_explosion(game, o->x + ow / 2.0f, o->y + oh / 2.0f);
                        spawn_score_text(game, o->x, o->y - 10.0f, SCORE_BREAKABLE);
                    }
                }
                // rock: projectile removed, obstacle stays
            }
        }
    }

    // Collisions: player vs enemy
    for (int ei = 0; ei < SG_MAX_ENEMIES; ei++) {
        sg_enemy_t *e = &game->enemies[ei];
        if (!e->active) continue;
        float ew = (float)EBOT_DRAW_W;
        float eh = (float)EBOT_DRAW_H;
        if (aabb(p->x + 4, p->y + 4, (float)ROBOT_DRAW_W - 8, (float)ROBOT_DRAW_H - 8,
                 e->x + 2, e->y + 2, ew - 4, eh - 4)) {
            e->active = false;
            player_take_damage(game);
        }
    }

    // Collisions: player vs obstacle
    for (int oi = 0; oi < SG_MAX_OBSTACLES; oi++) {
        sg_obstacle_t *o = &game->obstacles[oi];
        if (!o->active) continue;
        float ow = (o->type == SG_OBS_ROCK) ? (float)ROCK_DRAW_W  : (float)CRATE_DRAW_W;
        float oh = (o->type == SG_OBS_ROCK) ? (float)ROCK_DRAW_H  : (float)CRATE_DRAW_H;
        if (aabb(p->x + 4, p->y + 4, (float)ROBOT_DRAW_W - 8, (float)ROBOT_DRAW_H - 8,
                 o->x + 2, o->y, ow - 4, oh)) {
            o->active = false;
            player_take_damage(game);
        }
    }

    // Collisions: player vs coin
    for (int ci = 0; ci < SG_MAX_COINS; ci++) {
        sg_coin_t *c = &game->coins[ci];
        if (!c->active) continue;
        if (aabb(p->x + 2, p->y + 2, (float)ROBOT_DRAW_W - 4, (float)ROBOT_DRAW_H - 4,
                 c->x, c->y, (float)COIN_DRAW_W, (float)COIN_DRAW_H)) {
            c->active = false;
            game->score += SCORE_COIN;
            spawn_sparkle(game, c->x + COIN_DRAW_W / 2.0f, c->y + COIN_DRAW_H / 2.0f);
        }
    }

    // Distance score
    if (game->game_ms - game->last_dist_score_ms >= DIST_SCORE_MS) {
        game->score++;
        game->last_dist_score_ms = game->game_ms;
    }

    // Difficulty scaling
    {
        uint32_t phase     = game->game_ms / DIFF_INTERVAL_MS;
        float    new_speed = SCROLL_SPEED_START + (float)phase * SCROLL_SPEED_STEP;
        if (new_speed > SCROLL_SPEED_MAX) new_speed = SCROLL_SPEED_MAX;
        game->scroll_speed = new_speed;
    }

    // Spawn logic
    {
        float    diff     = game->scroll_speed / SCROLL_SPEED_START;
        uint32_t c_int    = (uint32_t)((float)SPAWN_COIN_BASE  / diff);
        uint32_t e_int    = (uint32_t)((float)SPAWN_ENEMY_BASE / diff);
        uint32_t o_int    = (uint32_t)((float)SPAWN_OBS_BASE   / diff);
        if (c_int < 800u)  c_int = 800u;
        if (e_int < 1200u) e_int = 1200u;
        if (o_int < 1500u) o_int = 1500u;

        if (game->game_ms - game->last_coin_ms >= c_int) {
            try_spawn_coin(game);
            game->last_coin_ms = game->game_ms;
        }
        if (game->game_ms - game->last_enemy_ms >= e_int) {
            try_spawn_enemy(game);
            game->last_enemy_ms = game->game_ms;
        }
        if (game->game_ms - game->last_obstacle_ms >= o_int) {
            if ((rng_next(&game->rng) % 10u) < 7u) try_spawn_obstacle(game);
            game->last_obstacle_ms = game->game_ms;
        }
    }

    // Game over
    if (game->lives <= 0) {
        if (game->score > game->hi_score) {
            game->hi_score = game->score;
            nvs_save_hi(game->hi_score);
        }
        game->state    = SG_STATE_GAMEOVER;
        game->state_ms = 0;
    }

    return ESP_OK;
}

// ---------------------------------------------------------------
// Render helpers
// ---------------------------------------------------------------
static void draw_background(shooter_game_t *g)
{
    graphics_t *gr = g->graphics;
    // Sky bands
    graphics_fill_rect(gr, 0, HUD_H,          DISP_W, 60,                  C_SKY1);
    graphics_fill_rect(gr, 0, HUD_H + 60,     DISP_W, 80,                  C_SKY2);
    graphics_fill_rect(gr, 0, HUD_H + 140,    DISP_W, GROUND_Y - HUD_H - 140, C_SKY3);
    // Ground
    graphics_fill_rect(gr, 0, GROUND_Y,       DISP_W, DISP_H - GROUND_Y,  C_GROUND);
    graphics_fill_rect(gr, 0, GROUND_Y,       DISP_W, 3,                   C_GROUNDTOP);
}

static void draw_hud(shooter_game_t *g)
{
    graphics_t *gr = g->graphics;
    graphics_fill_rect(gr, 0, 0, DISP_W, HUD_H, C_HUD_BG);

    // Hearts
    for (int i = 0; i < 3; i++) {
        if (i < g->lives) {
            graphics_draw_sprite(gr, 1 + i * (HEART_SPR_W + 2), 3,
                                 HEART_SPR_W, HEART_SPR_H,
                                 HEART_SPR, SHOOT_SPR_KEY, DISP_W, DISP_H);
        } else {
            graphics_draw_rect(gr,
                               (uint16_t)(1 + i * (HEART_SPR_W + 2)), 3,
                               HEART_SPR_W, HEART_SPR_H, 0x4208u);
        }
    }

    // Score right-aligned (5 digits, scale 1)
    char buf[8];
    snprintf(buf, sizeof(buf), "%05ld", (long)g->score);
    uint16_t score_x = (uint16_t)(DISP_W - 5 * 6 - 1);
    graphics_draw_text(gr, score_x, 3, buf, C_WHITE, 1);
}

static const uint16_t *coin_spr(int frame)
{
    switch (frame) {
    case 0:  return COIN_F0;
    case 1:  return COIN_F1;
    case 2:  return COIN_F2;
    default: return COIN_F3;
    }
}

static void draw_gameplay(shooter_game_t *g)
{
    graphics_t *gr = g->graphics;

    draw_background(g);

    // Obstacles
    for (int i = 0; i < SG_MAX_OBSTACLES; i++) {
        sg_obstacle_t *o = &g->obstacles[i];
        if (!o->active) continue;
        if (o->type == SG_OBS_ROCK) {
            graphics_draw_sprite_scaled(gr, (int32_t)o->x, (int32_t)o->y,
                                        ROCK_SPR_W, ROCK_SPR_H, ROCK_SPR,
                                        SHOOT_SPR_KEY, DISP_W, DISP_H, (uint8_t)SPR_SCALE);
        } else {
            graphics_draw_sprite_scaled(gr, (int32_t)o->x, (int32_t)o->y,
                                        CRATE_SPR_W, CRATE_SPR_H, CRATE_SPR,
                                        SHOOT_SPR_KEY, DISP_W, DISP_H, (uint8_t)SPR_SCALE);
        }
    }

    // Coins
    for (int i = 0; i < SG_MAX_COINS; i++) {
        sg_coin_t *c = &g->coins[i];
        if (!c->active) continue;
        graphics_draw_sprite_scaled(gr, (int32_t)c->x, (int32_t)c->y,
                                    COIN_SPR_W, COIN_SPR_H, coin_spr(c->anim_frame),
                                    SHOOT_SPR_KEY, DISP_W, DISP_H, (uint8_t)SPR_SCALE);
    }

    // Enemies
    for (int i = 0; i < SG_MAX_ENEMIES; i++) {
        sg_enemy_t *e = &g->enemies[i];
        if (!e->active) continue;
        if (e->type == SG_ENEMY_GROUND_BOT) {
            const uint16_t *spr = (e->anim_frame == 0) ? EBOT_F0 : EBOT_F1;
            graphics_draw_sprite_scaled(gr, (int32_t)e->x, (int32_t)e->y,
                                        EBOT_SPR_W, EBOT_SPR_H, spr,
                                        SHOOT_SPR_KEY, DISP_W, DISP_H, (uint8_t)SPR_SCALE);
        } else {
            const uint16_t *spr = (e->anim_frame == 0) ? DRONE_F0 : DRONE_F1;
            graphics_draw_sprite_scaled(gr, (int32_t)e->x, (int32_t)e->y,
                                        DRONE_SPR_W, DRONE_SPR_H, spr,
                                        SHOOT_SPR_KEY, DISP_W, DISP_H, (uint8_t)SPR_SCALE);
        }
    }

    // Particles (dust / explosion / sparkle)
    for (int i = 0; i < SG_MAX_PARTICLES; i++) {
        sg_particle_t *pt = &g->particles[i];
        if (!pt->active || pt->type == SG_PART_SCORE_TEXT) continue;
        int px = (int)pt->x, py = (int)pt->y;
        if (px < 0 || px >= DISP_W || py < HUD_H || py >= DISP_H) continue;
        graphics_fill_rect(gr, (uint16_t)px, (uint16_t)py, 3, 3, pt->color);
    }

    // Player
    if (g->player.visible) {
        const uint16_t *spr = g->player.in_air
                               ? ROBOT_JUMP
                               : (g->player.run_frame == 0 ? ROBOT_RUN0 : ROBOT_RUN1);
        graphics_draw_sprite_scaled(gr, (int32_t)g->player.x, (int32_t)g->player.y,
                                    ROBOT_SPR_W, ROBOT_SPR_H, spr,
                                    SHOOT_SPR_KEY, DISP_W, DISP_H, (uint8_t)SPR_SCALE);
    }

    // Projectiles
    for (int i = 0; i < SG_MAX_PROJECTILES; i++) {
        if (!g->projectiles[i].active) continue;
        int px = (int)g->projectiles[i].x;
        int py = (int)g->projectiles[i].y;
        if (px < 0 || px + PROJ_W > DISP_W) continue;
        if (py < HUD_H || py + PROJ_H > DISP_H) continue;
        graphics_fill_rect(gr, (uint16_t)px,     (uint16_t)py,     (uint16_t)PROJ_W, (uint16_t)PROJ_H, C_CYAN);
        if (px > 3)
            graphics_fill_rect(gr, (uint16_t)(px - 3), (uint16_t)(py + 1), 3, (uint16_t)(PROJ_H - 2), C_PROJ_TRAIL);
    }

    // Score text particles (floating "+N")
    for (int i = 0; i < SG_MAX_PARTICLES; i++) {
        sg_particle_t *pt = &g->particles[i];
        if (!pt->active || pt->type != SG_PART_SCORE_TEXT) continue;
        int px = (int)pt->x, py = (int)pt->y;
        if (px < 0 || px > DISP_W - 20 || py < HUD_H || py >= DISP_H) continue;
        char buf[8];
        snprintf(buf, sizeof(buf), "+%d", pt->value);
        graphics_draw_text(gr, (uint16_t)px, (uint16_t)py, buf, pt->color, 1);
    }

    // HUD on top
    draw_hud(g);

    // Red border on fresh damage
    if (g->player.invuln && g->player.invuln_ms > (INVULN_MS - 300u)) {
        graphics_draw_rect(gr, 0, HUD_H, DISP_W, DISP_H - HUD_H, C_RED);
    }
}

static void draw_stars(graphics_t *gr)
{
    uint32_t r = 0xABCD1234u;
    for (int i = 0; i < 35; i++) {
        r = r * 1664525u + 1013904223u;
        uint16_t sx = (uint16_t)(r % (uint32_t)DISP_W);
        r = r * 1664525u + 1013904223u;
        uint16_t sy = (uint16_t)(r % 120u) + 8u;
        graphics_fill_rect(gr, sx, sy, 1, 1, C_WHITE);
    }
}

static void draw_splash(shooter_game_t *g)
{
    graphics_t *gr = g->graphics;
    graphics_fill_rect(gr, 0, 0, DISP_W, DISP_H, C_TITLE_BG);
    draw_stars(gr);

    graphics_draw_text(gr, (DISP_W - 4*12)/2,  60, "TINY",    C_YELLOW, 2);
    graphics_draw_text(gr, (DISP_W - 7*12)/2,  82, "BLASTER", C_WHITE,  2);
    graphics_draw_text(gr, (DISP_W - 6*12)/2, 104, "RUNNER",  C_CYAN,   2);

    graphics_draw_sprite_scaled(gr, (DISP_W - (int32_t)ROBOT_DRAW_W) / 2, 135,
                                ROBOT_SPR_W, ROBOT_SPR_H, ROBOT_RUN0,
                                SHOOT_SPR_KEY, DISP_W, DISP_H, (uint8_t)SPR_SCALE);

    if ((g->state_ms / 500u) % 2u == 0u) {
        graphics_draw_text(gr, (DISP_W - 14*6)/2, 195, "PRESS A BUTTON", C_WHITE, 1);
    }
}

static void draw_menu(shooter_game_t *g)
{
    graphics_t *gr = g->graphics;
    graphics_fill_rect(gr, 0, 0, DISP_W, DISP_H, C_TITLE_BG);
    draw_stars(gr);

    graphics_draw_text(gr, (DISP_W - 4*12)/2,  25, "TINY",    C_YELLOW, 2);
    graphics_draw_text(gr, (DISP_W - 7*12)/2,  47, "BLASTER", C_WHITE,  2);
    graphics_draw_text(gr, (DISP_W - 6*12)/2,  69, "RUNNER",  C_CYAN,   2);

    graphics_draw_sprite_scaled(gr, (DISP_W - (int32_t)ROBOT_DRAW_W) / 2, 98,
                                ROBOT_SPR_W, ROBOT_SPR_H, ROBOT_RUN1,
                                SHOOT_SPR_KEY, DISP_W, DISP_H, (uint8_t)SPR_SCALE);

    graphics_draw_text(gr, 10, 150, "LEFT  START", C_YELLOW, 1);
    graphics_draw_text(gr, 10, 163, "RIGHT MENU",  C_GRAY,   1);

    char buf[20];
    snprintf(buf, sizeof(buf), "BEST  %05ld", (long)g->hi_score);
    graphics_draw_text(gr, 10, 180, buf, C_WHITE, 1);
}

static void draw_gameover(shooter_game_t *g)
{
    graphics_t *gr = g->graphics;
    graphics_fill_rect(gr, 0, 0, DISP_W, DISP_H, 0x2000u);

    graphics_draw_text(gr, (DISP_W - 9*12)/2, 55, "GAME OVER", C_RED, 2);

    char buf[20];
    snprintf(buf, sizeof(buf), "SCORE %05ld", (long)g->score);
    graphics_draw_text(gr, (DISP_W - 11*6)/2, 106, buf, C_WHITE, 1);

    snprintf(buf, sizeof(buf), "BEST  %05ld", (long)g->hi_score);
    graphics_draw_text(gr, (DISP_W - 11*6)/2, 119, buf, C_YELLOW, 1);

    if (g->score > 0 && g->score >= g->hi_score && (g->state_ms / 400u) % 2u == 0u) {
        graphics_draw_text(gr, (DISP_W - 12*6)/2, 137, "NEW HI SCORE", C_CYAN, 1);
    }

    graphics_draw_text(gr, 10, 182, "LEFT  RETRY", C_WHITE, 1);
    graphics_draw_text(gr, 10, 195, "RIGHT MENU",  C_GRAY,  1);
}

// ---------------------------------------------------------------
// Render
// ---------------------------------------------------------------
esp_err_t shooter_game_render(shooter_game_t *game)
{
    switch (game->state) {
    case SG_STATE_SPLASH:   draw_splash(game);   break;
    case SG_STATE_MENU:     draw_menu(game);     break;
    case SG_STATE_PLAY:     draw_gameplay(game); break;
    case SG_STATE_GAMEOVER: draw_gameover(game); break;
    }
    return ESP_OK;
}
