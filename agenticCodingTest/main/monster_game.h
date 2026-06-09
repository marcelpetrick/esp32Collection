#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "button_input.h"
#include "esp_err.h"
#include "graphics.h"

#define MONSTER_SPECIES_COUNT 3
#define WORLD_WIDTH           600

typedef enum {
    SPECIES_FLUFFALO = 0,
    SPECIES_TWIGLET,
    SPECIES_MOSS_MOUSE,
} monster_species_t;

typedef enum {
    MSTATE_IDLE = 0,
    MSTATE_MOVING,
    MSTATE_SLEEPING,
    MSTATE_PLAYING,
    MSTATE_EATING,
    MSTATE_COUNT,
} monster_state_t;

typedef enum {
    GMODE_TITLE = 0,
    GMODE_EXPLORE,
    GMODE_CAMERA,
    GMODE_BOOK,
    GMODE_DISCOVERY,
    GMODE_RESULT,
} game_mode_t;

typedef struct {
    monster_species_t species;
    monster_state_t   state;
    int32_t           world_x;
    int32_t           spawn_x;
    int32_t           target_x;
    uint32_t          state_timer_ms;
    uint32_t          state_duration_ms;
    uint8_t           rarity;
} monster_t;

typedef struct {
    uint32_t stars;
    uint32_t photos_taken;
    uint32_t species_found;
    uint32_t best_photos[MONSTER_SPECIES_COUNT];
    bool     discovered[MONSTER_SPECIES_COUNT];
} save_data_t;

typedef struct {
    graphics_t *graphics;
    save_data_t save;

    game_mode_t mode;
    uint32_t    rng;
    uint32_t    frame;

    int32_t  player_world_x;
    int32_t  camera_x;
    bool     left_walk;
    bool     right_walk;

    monster_t monsters[MONSTER_SPECIES_COUNT];

    int32_t prev_player_sx;
    int32_t prev_monster_sx[MONSTER_SPECIES_COUNT];
    int32_t prev_monster_sy[MONSTER_SPECIES_COUNT];

    uint64_t camera_enter_ms;
    int32_t  photo_score;
    int32_t  photo_species;
    bool     photo_new;

    int32_t  book_cursor;

    uint32_t anim_timer_ms;
    bool     needs_full_redraw;
} monster_game_t;

esp_err_t monster_game_init(monster_game_t *game, graphics_t *graphics);
esp_err_t monster_game_handle_input(monster_game_t *game,
                                    const button_event_t *events,
                                    size_t event_count);
esp_err_t monster_game_update(monster_game_t *game, uint32_t frame, uint32_t dt_ms);
esp_err_t monster_game_render(monster_game_t *game);
