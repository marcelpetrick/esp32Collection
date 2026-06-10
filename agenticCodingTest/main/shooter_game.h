#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "button_input.h"
#include "esp_err.h"
#include "graphics.h"

// ---------------------------------------------------------------
// Pool sizes
// ---------------------------------------------------------------
#define SG_MAX_PROJECTILES  3
#define SG_MAX_ENEMIES      4
#define SG_MAX_COINS        12
#define SG_MAX_OBSTACLES    4
#define SG_MAX_PARTICLES    24

// ---------------------------------------------------------------
// Game states
// ---------------------------------------------------------------
typedef enum {
    SG_STATE_SPLASH = 0,
    SG_STATE_MENU,
    SG_STATE_PLAY,
    SG_STATE_GAMEOVER,
} sg_state_t;

// ---------------------------------------------------------------
// Player
// ---------------------------------------------------------------
typedef struct {
    float    x;              // screen X (left edge of sprite)
    float    y;              // screen Y (top edge of sprite), float for sub-pixel
    float    vy;             // vertical velocity px/frame (negative = upward)
    bool     in_air;

    bool     invuln;
    uint32_t invuln_ms;      // remaining invulnerability ms
    bool     visible;        // false during blink
    uint32_t blink_ms;       // time within current blink phase

    int      run_frame;      // 0 or 1
    uint32_t run_timer_ms;

    uint32_t shoot_cooldown_ms;  // remaining cooldown

    bool     left_held;
    uint32_t left_hold_ms;   // how long left is held this press

    bool     shooting_anim;  // brief arm animation
    uint32_t shoot_anim_ms;
} sg_player_t;

// ---------------------------------------------------------------
// Projectile
// ---------------------------------------------------------------
typedef struct {
    bool  active;
    float x, y;
} sg_projectile_t;

// ---------------------------------------------------------------
// Enemy
// ---------------------------------------------------------------
typedef enum {
    SG_ENEMY_GROUND_BOT = 0,
    SG_ENEMY_FLYING_DRONE,
} sg_enemy_type_t;

typedef struct {
    bool           active;
    sg_enemy_type_t type;
    float          x, y;
    int            anim_frame;  // 0 or 1
    uint32_t       anim_ms;
} sg_enemy_t;

// ---------------------------------------------------------------
// Coin
// ---------------------------------------------------------------
typedef struct {
    bool     active;
    float    x, y;
    int      anim_frame;  // 0-3 rotation
    uint32_t anim_ms;
} sg_coin_t;

// ---------------------------------------------------------------
// Obstacle
// ---------------------------------------------------------------
typedef enum {
    SG_OBS_ROCK = 0,
    SG_OBS_CRATE,        // breakable
} sg_obs_type_t;

typedef struct {
    bool          active;
    sg_obs_type_t type;
    int           health;  // 1 = one shot, crate starts at 1
    float         x, y;
} sg_obstacle_t;

// ---------------------------------------------------------------
// Particle
// ---------------------------------------------------------------
typedef enum {
    SG_PART_EXPLOSION = 0,
    SG_PART_SPARKLE,
    SG_PART_DUST,
    SG_PART_SCORE_TEXT,  // floating "+N" text
} sg_part_type_t;

typedef struct {
    bool         active;
    sg_part_type_t type;
    float        x, y;
    float        vx, vy;
    uint32_t     life_ms;
    uint32_t     max_life_ms;
    uint16_t     color;
    int          value;   // for SG_PART_SCORE_TEXT: the value to display
} sg_particle_t;

// ---------------------------------------------------------------
// Main game struct
// ---------------------------------------------------------------
typedef struct {
    graphics_t *graphics;
    sg_state_t  state;
    uint32_t    state_ms;     // time in current state

    // gameplay
    sg_player_t     player;
    sg_projectile_t projectiles[SG_MAX_PROJECTILES];
    sg_enemy_t      enemies[SG_MAX_ENEMIES];
    sg_coin_t       coins[SG_MAX_COINS];
    sg_obstacle_t   obstacles[SG_MAX_OBSTACLES];
    sg_particle_t   particles[SG_MAX_PARTICLES];

    int32_t  score;
    int32_t  hi_score;
    int      lives;

    // difficulty / timing
    float    scroll_speed;     // px per frame
    uint32_t game_ms;          // total play time ms
    uint32_t last_coin_ms;
    uint32_t last_enemy_ms;
    uint32_t last_obstacle_ms;
    uint32_t last_dist_score_ms;

    // RNG
    uint32_t rng;
} shooter_game_t;

// ---------------------------------------------------------------
// API
// ---------------------------------------------------------------
esp_err_t shooter_game_init(shooter_game_t *game, graphics_t *graphics);
esp_err_t shooter_game_handle_input(shooter_game_t *game,
                                    const button_event_t *events,
                                    size_t event_count);
esp_err_t shooter_game_update(shooter_game_t *game, uint32_t frame, uint32_t dt_ms);
esp_err_t shooter_game_render(shooter_game_t *game);
