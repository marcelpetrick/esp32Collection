#include "monster_game.h"

#include <stdint.h>
#include <string.h>

#include "board_config.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "monster_sprites.h"
#include "nvs.h"

static const char *TAG = "monster_game";

// ---------------------------------------------------------------
// Display layout constants (all Y values are screen pixels)
// ---------------------------------------------------------------
#define DISP_W            BOARD_DISPLAY_WIDTH
#define DISP_H            BOARD_DISPLAY_HEIGHT

#define SPRITE_SCALE      2u   // all sprites drawn at 2x pixel scale

#define ZONE_SKY_END      155
#define ZONE_GRASS_END    185
#define ZONE_GROUND_END   228
#define ZONE_HUD_START    228

#define PLAYER_SCREEN_X   60   // fixed horizontal center of player on display
#define PLAYER_SCREEN_Y   196  // top of player sprite; bottom at 196+32=228=HUD
#define MONSTER_FEET_Y    185  // Y where monster feet touch the grass line

#define PLAYER_WALK_PX_S  60   // pixels per second
#define MONSTER_WALK_PX_S 15

#define PHOTO_HOLD_MIN_MS 300u // min hold time to register as photo
#define DISCOVERY_MS      2200u
#define RESULT_MS         1800u

// ---------------------------------------------------------------
// Colors
// ---------------------------------------------------------------
#define C_SKY_DARK  0x4B5Eu
#define C_SKY       0x65BEu
#define C_GRASS     0x3D05u
#define C_GROUND    0x7A83u
#define C_HUD_BG    0x0000u
#define C_WHITE     0xFFFFu
#define C_BLACK     0x0000u
#define C_YELLOW    0xFFE0u
#define C_ORANGE    0xFC00u
#define C_RED       0xF800u
#define C_TEXT      0xFFFFu
#define C_TITLE_BG  0x2965u

// ---------------------------------------------------------------
// Monster metadata table
// ---------------------------------------------------------------
typedef struct {
    const char       *name;
    const uint16_t   *sprite;
    uint16_t          w;
    uint16_t          h;
    uint8_t           rarity;   // 0=common 1=rare 2=legendary
    int32_t           spawn_x;
} monster_info_t;

static const monster_info_t MONSTER_INFO[MONSTER_SPECIES_COUNT] = {
    {
        .name = "FLUFFALO",
        .sprite = FLUF_SPRITE,
        .w = FLUF_SPRITE_W,
        .h = FLUF_SPRITE_H,
        .rarity = 0,
        .spawn_x = 120,
    },
    {
        .name = "TWIGLET",
        .sprite = TWIG_SPRITE,
        .w = TWIG_SPRITE_W,
        .h = TWIG_SPRITE_H,
        .rarity = 1,
        .spawn_x = 310,
    },
    {
        .name = "MOSS MOUSE",
        .sprite = MMOU_SPRITE,
        .w = MMOU_SPRITE_W,
        .h = MMOU_SPRITE_H,
        .rarity = 2,
        .spawn_x = 490,
    },
};

// ---------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------
static uint32_t rng_next(uint32_t *state)
{
    *state = (*state * 1664525u) + 1013904223u;
    return *state;
}

static int32_t clamp32(int32_t v, int32_t lo, int32_t hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static int32_t abs32(int32_t v) { return v < 0 ? -v : v; }

static uint64_t now_ms(void)
{
    return (uint64_t)(esp_timer_get_time() / 1000);
}

static uint16_t bg_color_at_y(int32_t y)
{
    if (y < 25)            return C_SKY_DARK;
    if (y < ZONE_SKY_END)  return C_SKY;
    if (y < ZONE_GRASS_END)return C_GRASS;
    return C_GROUND;
}

// ---------------------------------------------------------------
// NVS save / load
// ---------------------------------------------------------------
static void save_load(save_data_t *save)
{
    nvs_handle_t h;
    if (nvs_open("mpg", NVS_READONLY, &h) != ESP_OK) {
        return;
    }
    nvs_get_u32(h, "stars",  &save->stars);
    nvs_get_u32(h, "taken",  &save->photos_taken);
    nvs_get_u32(h, "found",  &save->species_found);
    nvs_get_u32(h, "best0",  &save->best_photos[0]);
    nvs_get_u32(h, "best1",  &save->best_photos[1]);
    nvs_get_u32(h, "best2",  &save->best_photos[2]);
    uint8_t disc = 0;
    nvs_get_u8(h, "disc", &disc);
    for (int i = 0; i < MONSTER_SPECIES_COUNT; i++) {
        save->discovered[i] = (disc >> i) & 1u;
    }
    nvs_close(h);
    ESP_LOGI(TAG, "Save loaded: stars=%lu taken=%lu found=%lu",
             (unsigned long)save->stars,
             (unsigned long)save->photos_taken,
             (unsigned long)save->species_found);
}

static void save_commit(const save_data_t *save)
{
    nvs_handle_t h;
    if (nvs_open("mpg", NVS_READWRITE, &h) != ESP_OK) {
        return;
    }
    nvs_set_u32(h, "stars",  save->stars);
    nvs_set_u32(h, "taken",  save->photos_taken);
    nvs_set_u32(h, "found",  save->species_found);
    nvs_set_u32(h, "best0",  save->best_photos[0]);
    nvs_set_u32(h, "best1",  save->best_photos[1]);
    nvs_set_u32(h, "best2",  save->best_photos[2]);
    uint8_t disc = 0;
    for (int i = 0; i < MONSTER_SPECIES_COUNT; i++) {
        if (save->discovered[i]) disc |= (uint8_t)(1u << i);
    }
    nvs_set_u8(h, "disc", disc);
    nvs_commit(h);
    nvs_close(h);
}

// ---------------------------------------------------------------
// Background rendering
// ---------------------------------------------------------------
static esp_err_t draw_background(monster_game_t *game)
{
    graphics_t *g = game->graphics;
    ESP_RETURN_ON_ERROR(graphics_fill_rect(g, 0, 0, DISP_W, 25, C_SKY_DARK), TAG, "bg");
    ESP_RETURN_ON_ERROR(graphics_fill_rect(g, 0, 25, DISP_W, ZONE_SKY_END - 25, C_SKY), TAG, "bg");
    ESP_RETURN_ON_ERROR(graphics_fill_rect(g, 0, ZONE_SKY_END, DISP_W, ZONE_GRASS_END - ZONE_SKY_END, C_GRASS), TAG, "bg");
    ESP_RETURN_ON_ERROR(graphics_fill_rect(g, 0, ZONE_GRASS_END, DISP_W, ZONE_HUD_START - ZONE_GRASS_END, C_GROUND), TAG, "bg");
    ESP_RETURN_ON_ERROR(graphics_fill_rect(g, 0, ZONE_HUD_START, DISP_W, DISP_H - ZONE_HUD_START, C_HUD_BG), TAG, "bg");
    ESP_RETURN_ON_ERROR(graphics_draw_text(g, 2,  ZONE_HUD_START + 1, "L:BUCH", C_TEXT, 1), TAG, "hud");
    ESP_RETURN_ON_ERROR(graphics_draw_text(g, 72, ZONE_HUD_START + 1, "R:FOTO", C_TEXT, 1), TAG, "hud");
    return ESP_OK;
}

// ---------------------------------------------------------------
// Title / tutorial screen (German instructions)
// ---------------------------------------------------------------
static esp_err_t draw_title(monster_game_t *game)
{
    graphics_t *g = game->graphics;
    ESP_RETURN_ON_ERROR(graphics_fill_rect(g, 0, 0, DISP_W, DISP_H, C_TITLE_BG), TAG, "title bg");

    // Header
    ESP_RETURN_ON_ERROR(graphics_fill_rect(g, 0, 0, DISP_W, 18, C_SKY), TAG, "title header");
    ESP_RETURN_ON_ERROR(graphics_draw_text(g, 4, 5, "MONSTER FOTO", C_WHITE, 1), TAG, "title hdr");

    int32_t y = 26;
    // Short intro line by line (5x7 font at scale 1 = 6px per char wide, 8px tall with gap)
    ESP_RETURN_ON_ERROR(graphics_draw_text(g, 4, y,      "DU BIST DER",    C_YELLOW, 1), TAG, "t");
    ESP_RETURN_ON_ERROR(graphics_draw_text(g, 4, y + 10, "MONSTER-FOTO-",  C_YELLOW, 1), TAG, "t");
    ESP_RETURN_ON_ERROR(graphics_draw_text(g, 4, y + 20, "GRAF",           C_YELLOW, 1), TAG, "t");

    y += 36;
    ESP_RETURN_ON_ERROR(graphics_draw_text(g, 4, y,      "LAUFEN:",        C_WHITE, 1), TAG, "t");
    ESP_RETURN_ON_ERROR(graphics_draw_text(g, 4, y + 10, "L/R KURZ HALTEN", C_SKY, 1), TAG, "t");

    y += 26;
    ESP_RETURN_ON_ERROR(graphics_draw_text(g, 4, y,      "FOTO:",          C_WHITE, 1), TAG, "t");
    ESP_RETURN_ON_ERROR(graphics_draw_text(g, 4, y + 10, "R LANG HALTEN",  C_ORANGE, 1), TAG, "t");
    ESP_RETURN_ON_ERROR(graphics_draw_text(g, 4, y + 20, "DANN LOSLASSEN", C_ORANGE, 1), TAG, "t");

    y += 36;
    ESP_RETURN_ON_ERROR(graphics_draw_text(g, 4, y,      "MONSTER-BUCH:",  C_WHITE, 1), TAG, "t");
    ESP_RETURN_ON_ERROR(graphics_draw_text(g, 4, y + 10, "L LANG HALTEN",  C_SKY, 1), TAG, "t");

    y += 26;
    ESP_RETURN_ON_ERROR(graphics_draw_text(g, 4, y,      "ZIEL: ALLE 3",   C_WHITE, 1), TAG, "t");
    ESP_RETURN_ON_ERROR(graphics_draw_text(g, 4, y + 10, "MONSTER FINDEN", C_WHITE, 1), TAG, "t");

    y += 28;
    ESP_RETURN_ON_ERROR(graphics_fill_rect(g, 0, y - 2, DISP_W, 1, C_SKY), TAG, "div");
    ESP_RETURN_ON_ERROR(graphics_draw_text(g, 4, y + 4, "TASTE DRUECKEN", C_YELLOW, 1), TAG, "t");
    ESP_RETURN_ON_ERROR(graphics_draw_text(g, 4, y + 16, "ZUM STARTEN!", C_YELLOW, 1), TAG, "t");

    return ESP_OK;
}

// ---------------------------------------------------------------
// Monster sprite drawing
// ---------------------------------------------------------------
static int32_t monster_screen_x(const monster_game_t *game, int idx)
{
    return MONSTER_INFO[idx].spawn_x
           + (game->monsters[idx].world_x - MONSTER_INFO[idx].spawn_x)
           - game->camera_x;
}

static int32_t monster_screen_y(int idx)
{
    return MONSTER_FEET_Y - (int32_t)(MONSTER_INFO[idx].h * SPRITE_SCALE);
}

static esp_err_t clear_sprite_area(graphics_t *g,
                                   int32_t sx, int32_t sy,
                                   uint16_t w, uint16_t h)
{
    int32_t x0 = clamp32(sx, 0, DISP_W);
    int32_t x1 = clamp32(sx + (int32_t)w, 0, DISP_W);
    if (x0 >= x1) return ESP_OK;

    // clear row by row using the correct background color
    for (uint16_t row = 0; row < h; row++) {
        int32_t py = sy + (int32_t)row;
        if (py < 0 || py >= DISP_H) continue;
        uint16_t bg = bg_color_at_y(py);
        ESP_RETURN_ON_ERROR(
            graphics_fill_rect(g, (uint16_t)x0, (uint16_t)py, (uint16_t)(x1 - x0), 1, bg),
            TAG, "clear sprite");
    }
    return ESP_OK;
}

// Draw monster FSM indicator above the sprite (sleeping / playing)
static esp_err_t draw_state_indicator(monster_game_t *game, int idx, int32_t sx, int32_t sy)
{
    graphics_t *g = game->graphics;
    monster_state_t state = game->monsters[idx].state;
    if (state != MSTATE_SLEEPING && state != MSTATE_PLAYING) return ESP_OK;

    int32_t ix = sx + (int32_t)(MONSTER_INFO[idx].w * SPRITE_SCALE) / 2 - 3;
    int32_t iy = sy - 10;
    if (iy < 0 || ix < 0 || ix >= DISP_W) return ESP_OK;

    uint16_t clr = (state == MSTATE_SLEEPING) ? C_WHITE : C_YELLOW;
    const char *label = (state == MSTATE_SLEEPING) ? "Z" : "!";
    // clear background first
    ESP_RETURN_ON_ERROR(
        graphics_fill_rect(g, (uint16_t)ix, (uint16_t)iy, 6, 8,
                           bg_color_at_y(iy)),
        TAG, "ind clear");
    return graphics_draw_text(g, (uint16_t)ix, (uint16_t)iy, label, clr, 1);
}

static esp_err_t draw_monster(monster_game_t *game, int idx)
{
    graphics_t *g = game->graphics;
    int32_t sx = monster_screen_x(game, idx);
    int32_t sy = monster_screen_y(idx);
    const monster_info_t *info = &MONSTER_INFO[idx];

    if (sx + (int32_t)(info->w * SPRITE_SCALE) < 0 || sx >= DISP_W) {
        return ESP_OK;
    }

    ESP_RETURN_ON_ERROR(
        graphics_draw_sprite_scaled(g, sx, sy, info->w, info->h,
                                    info->sprite, SPRITE_KEY,
                                    DISP_W, DISP_H, SPRITE_SCALE),
        TAG, "draw monster sprite");

    return draw_state_indicator(game, idx, sx, sy);
}

// ---------------------------------------------------------------
// Photo scoring
// ---------------------------------------------------------------
static int32_t score_photo(const monster_game_t *game, int32_t *out_idx)
{
    int32_t best_score = -1;
    int32_t best_idx   = -1;

    for (int i = 0; i < MONSTER_SPECIES_COUNT; i++) {
        int32_t sx = monster_screen_x(game, i);
        int32_t sw = (int32_t)(MONSTER_INFO[i].w * SPRITE_SCALE);
        if (sx + sw < 0 || sx >= DISP_W) continue;

        int32_t center = sx + sw / 2;
        int32_t screen_mid = DISP_W / 2;
        int32_t dx = abs32(center - screen_mid);
        int32_t dist_score = clamp32(50 - dx * 50 / (DISP_W / 2), 0, 50);

        int32_t center_score = (sx >= 8 && sx + sw <= DISP_W - 8) ? 30 : 10;
        int32_t rarity_bonus = (int32_t)game->monsters[i].rarity * 20;
        int32_t total = dist_score + center_score + rarity_bonus;

        if (total > best_score) {
            best_score = total;
            best_idx   = i;
        }
    }

    *out_idx = best_idx;
    return (best_idx >= 0) ? best_score : 0;
}

// ---------------------------------------------------------------
// Camera overlay
// ---------------------------------------------------------------
static esp_err_t draw_camera_overlay(monster_game_t *game)
{
    graphics_t *g = game->graphics;
    // viewfinder frame
    ESP_RETURN_ON_ERROR(graphics_draw_rect(g, 8, 20, DISP_W - 16, ZONE_HUD_START - 28, C_WHITE), TAG, "cam frame");
    // corner brackets (thicker feel)
    ESP_RETURN_ON_ERROR(graphics_draw_rect(g, 7, 19, DISP_W - 14, ZONE_HUD_START - 26, C_WHITE), TAG, "cam frame2");
    // center crosshair
    uint16_t cx = DISP_W / 2;
    uint16_t cy = (ZONE_SKY_END + ZONE_GRASS_END) / 2;
    ESP_RETURN_ON_ERROR(graphics_fill_rect(g, cx - 6, cy, 4, 1, C_WHITE), TAG, "ch h1");
    ESP_RETURN_ON_ERROR(graphics_fill_rect(g, cx + 2, cy, 4, 1, C_WHITE), TAG, "ch h2");
    ESP_RETURN_ON_ERROR(graphics_fill_rect(g, cx, cy - 6, 1, 4, C_WHITE), TAG, "ch v1");
    ESP_RETURN_ON_ERROR(graphics_fill_rect(g, cx, cy + 2, 1, 4, C_WHITE), TAG, "ch v2");
    // label
    ESP_RETURN_ON_ERROR(
        graphics_draw_text(g, (uint16_t)(DISP_W / 2 - 15), 24, "KAMERA", C_WHITE, 1),
        TAG, "cam label");
    return ESP_OK;
}

// ---------------------------------------------------------------
// Discovery animation
// ---------------------------------------------------------------
static esp_err_t draw_discovery(monster_game_t *game)
{
    graphics_t *g = game->graphics;
    uint32_t t = game->anim_timer_ms;
    const char *name = MONSTER_INFO[game->photo_species].name;

    if (t < 200u) {
        return graphics_fill_rect(g, 0, 0, DISP_W, DISP_H, C_WHITE);
    }
    if (t < 350u) {
        return graphics_fill_rect(g, 0, 0, DISP_W, DISP_H, C_BLACK);
    }
    if (t < 500u) {
        return graphics_fill_rect(g, 0, 0, DISP_W, DISP_H, C_WHITE);
    }

    // Steady discovery card
    if (game->needs_full_redraw) {
        ESP_RETURN_ON_ERROR(graphics_fill_rect(g, 0, 0, DISP_W, DISP_H, C_BLACK), TAG, "disc bg");

        // Star burst header
        ESP_RETURN_ON_ERROR(graphics_fill_rect(g, 0, 0, DISP_W, 22, C_YELLOW), TAG, "disc hdr");
        ESP_RETURN_ON_ERROR(graphics_draw_text(g, 10, 7, "NEU ENTDECKT", C_BLACK, 1), TAG, "disc hdr t");

        ESP_RETURN_ON_ERROR(graphics_draw_text(g, 10, 40,  "NEUER FREUND", C_YELLOW, 2), TAG, "disc nf");
        ESP_RETURN_ON_ERROR(graphics_draw_text(g, 10, 62,  "GEFUNDEN", C_YELLOW, 2), TAG, "disc f");

        ESP_RETURN_ON_ERROR(graphics_fill_rect(g, 8, 90, DISP_W - 16, 2, C_WHITE), TAG, "disc div");

        ESP_RETURN_ON_ERROR(graphics_draw_text(g, 12, 100, name, C_WHITE, 2), TAG, "disc name");

        // Stars awarded
        uint32_t stars_earned = (game->monsters[game->photo_species].rarity == 0) ? 10u :
                                (game->monsters[game->photo_species].rarity == 1) ? 25u : 50u;

        char star_buf[16] = {0};
        star_buf[0] = '+';
        uint32_t sv = stars_earned;
        int si = 1;
        if (sv == 0) { star_buf[si++] = '0'; }
        else {
            char rev[8]; int ri = 0;
            while (sv > 0) { rev[ri++] = (char)('0' + sv % 10); sv /= 10; }
            while (ri > 0) star_buf[si++] = rev[--ri];
        }
        star_buf[si++] = ' ';
        star_buf[si++] = 'S';
        star_buf[si++] = 'T';
        star_buf[si++] = 'E';
        star_buf[si++] = 'R';
        star_buf[si++] = 'N';
        star_buf[si++] = 'E';
        star_buf[si] = '\0';
        ESP_RETURN_ON_ERROR(graphics_draw_text(g, 12, 140, star_buf, C_YELLOW, 1), TAG, "disc stars");

        // Draw the monster sprite large (centered)
        const monster_info_t *info = &MONSTER_INFO[game->photo_species];
        int32_t ssx = (DISP_W - (int32_t)(info->w * SPRITE_SCALE)) / 2;
        int32_t ssy = 165;
        ESP_RETURN_ON_ERROR(
            graphics_draw_sprite_scaled(g, ssx, ssy, info->w, info->h,
                                        info->sprite, SPRITE_KEY, DISP_W, DISP_H, SPRITE_SCALE),
            TAG, "disc sprite");

        ESP_RETURN_ON_ERROR(
            graphics_fill_rect(g, 0, ZONE_HUD_START, DISP_W, DISP_H - ZONE_HUD_START, C_HUD_BG),
            TAG, "disc hud");
        ESP_RETURN_ON_ERROR(
            graphics_draw_text(g, 20, ZONE_HUD_START + 1, "TASTE DRUECKEN", C_TEXT, 1),
            TAG, "disc prompt");
    }
    return ESP_OK;
}

// ---------------------------------------------------------------
// Photo result screen
// ---------------------------------------------------------------
static esp_err_t draw_result(monster_game_t *game)
{
    graphics_t *g = game->graphics;

    if (!game->needs_full_redraw) return ESP_OK;

    ESP_RETURN_ON_ERROR(graphics_fill_rect(g, 0, 0, DISP_W, DISP_H, C_BLACK), TAG, "res bg");

    const char *label;
    uint16_t    label_col;
    if (game->photo_score >= 80) {
        label = "SUPER!"; label_col = C_YELLOW;
    } else if (game->photo_score >= 50) {
        label = "TOLL!"; label_col = C_SKY;
    } else {
        label = "GUT!"; label_col = C_TEXT;
    }

    ESP_RETURN_ON_ERROR(graphics_fill_rect(g, 0, 0, DISP_W, 22, C_SKY), TAG, "res hdr bg");
    ESP_RETURN_ON_ERROR(graphics_draw_text(g, 4, 7, "FOTO FERTIG", C_WHITE, 1), TAG, "res hdr");

    ESP_RETURN_ON_ERROR(graphics_draw_text(g, 12, 40, label, label_col, 2), TAG, "res label");

    // Score number
    char sc_buf[8] = {0};
    int32_t sv = game->photo_score; int si = 0;
    if (sv == 0) { sc_buf[si++] = '0'; }
    else {
        char rev[8]; int ri = 0;
        while (sv > 0) { rev[ri++] = (char)('0' + sv % 10); sv /= 10; }
        while (ri > 0) sc_buf[si++] = rev[--ri];
    }
    sc_buf[si] = '\0';
    ESP_RETURN_ON_ERROR(graphics_draw_text(g, 12, 70, sc_buf, C_WHITE, 2), TAG, "res score");
    ESP_RETURN_ON_ERROR(graphics_draw_text(g, 12 + (si * 12) + 4, 74, "PTS", C_TEXT, 1), TAG, "res pts");

    // Monster name
    if (game->photo_species >= 0 && game->photo_species < MONSTER_SPECIES_COUNT) {
        const monster_info_t *info = &MONSTER_INFO[game->photo_species];
        ESP_RETURN_ON_ERROR(graphics_draw_text(g, 12, 100, info->name, C_YELLOW, 1), TAG, "res name");

        // Draw monster sprite
        int32_t ssx = (DISP_W - (int32_t)(info->w * SPRITE_SCALE)) / 2;
        ESP_RETURN_ON_ERROR(
            graphics_draw_sprite_scaled(g, ssx, 130, info->w, info->h,
                                        info->sprite, SPRITE_KEY, DISP_W, DISP_H, SPRITE_SCALE),
            TAG, "res sprite");
    }

    // Show updated total stars
    uint32_t st = game->save.stars;
    char st_buf[16] = {'S','T','E','R','N','E',':',' ',0};
    uint32_t sv2 = st; int si2 = 8;
    if (sv2 == 0) { st_buf[si2++] = '0'; }
    else {
        char rev2[8]; int ri2 = 0;
        while (sv2 > 0) { rev2[ri2++] = (char)('0' + sv2 % 10); sv2 /= 10; }
        while (ri2 > 0) st_buf[si2++] = rev2[--ri2];
    }
    st_buf[si2] = '\0';
    ESP_RETURN_ON_ERROR(graphics_draw_text(g, 4, 185, st_buf, C_YELLOW, 1), TAG, "res stars");

    ESP_RETURN_ON_ERROR(
        graphics_fill_rect(g, 0, ZONE_HUD_START, DISP_W, DISP_H - ZONE_HUD_START, C_HUD_BG),
        TAG, "res hud");
    ESP_RETURN_ON_ERROR(
        graphics_draw_text(g, 20, ZONE_HUD_START + 1, "TASTE DRUECKEN", C_TEXT, 1),
        TAG, "res prompt");

    return ESP_OK;
}

// ---------------------------------------------------------------
// Monster Book screen
// ---------------------------------------------------------------
static esp_err_t draw_book(monster_game_t *game)
{
    graphics_t *g = game->graphics;
    if (!game->needs_full_redraw) return ESP_OK;

    ESP_RETURN_ON_ERROR(graphics_fill_rect(g, 0, 0, DISP_W, DISP_H, C_BLACK), TAG, "book bg");
    ESP_RETURN_ON_ERROR(graphics_fill_rect(g, 0, 0, DISP_W, 20, C_SKY), TAG, "book hdr");
    ESP_RETURN_ON_ERROR(graphics_draw_text(g, 4, 6, "MONSTER-BUCH", C_WHITE, 1), TAG, "book hdr t");
    ESP_RETURN_ON_ERROR(
        graphics_draw_text(g, 4, 24,
                           "STERNE GESAMT", C_YELLOW, 1),
        TAG, "book st lbl");

    char st_buf[8] = {0}; int si = 0;
    uint32_t sv = game->save.stars;
    if (sv == 0) { st_buf[si++] = '0'; }
    else { char rv[8]; int ri = 0; while (sv) { rv[ri++]=(char)('0'+sv%10); sv/=10; } while(ri>0) st_buf[si++]=rv[--ri]; }
    st_buf[si] = '\0';
    ESP_RETURN_ON_ERROR(graphics_draw_text(g, 92, 24, st_buf, C_YELLOW, 1), TAG, "book stars");

    int32_t y = 38;
    for (int i = 0; i < MONSTER_SPECIES_COUNT; i++) {
        uint16_t row_bg = (i == game->book_cursor) ? C_SKY_DARK : C_BLACK;
        ESP_RETURN_ON_ERROR(graphics_fill_rect(g, 0, y, DISP_W, 56, row_bg), TAG, "book row");

        if (game->save.discovered[i]) {
            // Name + checkmark
            ESP_RETURN_ON_ERROR(graphics_draw_text(g, 4, y + 2, MONSTER_INFO[i].name, C_WHITE, 1), TAG, "book nm");
            ESP_RETURN_ON_ERROR(graphics_draw_text(g, 4, y + 12, "ENTDECKT!", C_YELLOW, 1), TAG, "book disc");

            // Best photo score
            char sc[12] = {'B','E','S','T',':',' ',0}; int si2=6;
            uint32_t bp = game->save.best_photos[i];
            if (bp==0){sc[si2++]='0';}else{char rv2[8];int ri2=0;while(bp){rv2[ri2++]=(char)('0'+bp%10);bp/=10;}while(ri2>0)sc[si2++]=rv2[--ri2];}
            sc[si2++]='P'; sc[si2++]='T'; sc[si2++]='S'; sc[si2]='\0';
            ESP_RETURN_ON_ERROR(graphics_draw_text(g, 4, y + 22, sc, C_SKY, 1), TAG, "book best");

            // Rarity
            const char *rar = (MONSTER_INFO[i].rarity==0)?"HAEUFIG":
                              (MONSTER_INFO[i].rarity==1)?"SELTEN":"LEGENDAER";
            uint16_t rc = (MONSTER_INFO[i].rarity==0)?C_TEXT:
                          (MONSTER_INFO[i].rarity==1)?C_ORANGE:C_RED;
            ESP_RETURN_ON_ERROR(graphics_draw_text(g, 4, y + 32, rar, rc, 1), TAG, "book rar");

            // Draw mini sprite on right
            const monster_info_t *info = &MONSTER_INFO[i];
            int32_t ssx = DISP_W - (int32_t)(info->w * SPRITE_SCALE) - 4;
            int32_t ssy = y + (56 - (int32_t)(info->h * SPRITE_SCALE)) / 2;
            ESP_RETURN_ON_ERROR(
                graphics_draw_sprite_scaled(g, ssx, ssy, info->w, info->h,
                                            info->sprite, SPRITE_KEY, DISP_W, DISP_H, SPRITE_SCALE),
                TAG, "book sprite");
        } else {
            ESP_RETURN_ON_ERROR(graphics_draw_text(g, 4, y + 2,  "????",        C_TEXT, 1), TAG, "book unk");
            ESP_RETURN_ON_ERROR(graphics_draw_text(g, 4, y + 14, "NOCH NICHT",  C_TEXT, 1), TAG, "book unk2");
            ESP_RETURN_ON_ERROR(graphics_draw_text(g, 4, y + 24, "ENTDECKT",    C_TEXT, 1), TAG, "book unk3");
        }
        y += 58;
    }

    ESP_RETURN_ON_ERROR(
        graphics_fill_rect(g, 0, ZONE_HUD_START, DISP_W, DISP_H - ZONE_HUD_START, C_HUD_BG),
        TAG, "book hud");
    ESP_RETURN_ON_ERROR(
        graphics_draw_text(g, 2, ZONE_HUD_START + 1, "L:WEITER L-LANG:SCHLIES", C_TEXT, 1),
        TAG, "book hud t");

    return ESP_OK;
}

// ---------------------------------------------------------------
// Explore mode rendering (dirty rect)
// ---------------------------------------------------------------
static esp_err_t render_explore(monster_game_t *game)
{
    graphics_t *g = game->graphics;

    if (game->needs_full_redraw) {
        ESP_RETURN_ON_ERROR(draw_background(game), TAG, "bg draw");
        // draw all monsters
        for (int i = 0; i < MONSTER_SPECIES_COUNT; i++) {
            int32_t sx = monster_screen_x(game, i);
            int32_t sy = monster_screen_y(i);
            game->prev_monster_sx[i] = sx;
            game->prev_monster_sy[i] = sy;
            ESP_RETURN_ON_ERROR(draw_monster(game, i), TAG, "init monster");
        }
        // draw player
        int32_t psx = PLAYER_SCREEN_X - (int32_t)(PLAYER_SPRITE_W * SPRITE_SCALE / 2);
        game->prev_player_sx = psx;
        ESP_RETURN_ON_ERROR(
            graphics_draw_sprite_scaled(g, psx, PLAYER_SCREEN_Y,
                                        PLAYER_SPRITE_W, PLAYER_SPRITE_H,
                                        PLAYER_SPRITE, SPRITE_KEY, DISP_W, DISP_H, SPRITE_SCALE),
            TAG, "init player");
        game->needs_full_redraw = false;
        return ESP_OK;
    }

    // Dirty rect: clear old, draw new
    int32_t new_psx = PLAYER_SCREEN_X - (int32_t)(PLAYER_SPRITE_W * SPRITE_SCALE / 2);
    if (new_psx != game->prev_player_sx) {
        ESP_RETURN_ON_ERROR(
            clear_sprite_area(g, game->prev_player_sx, PLAYER_SCREEN_Y,
                              PLAYER_SPRITE_W * SPRITE_SCALE, PLAYER_SPRITE_H * SPRITE_SCALE),
            TAG, "clear player");
        ESP_RETURN_ON_ERROR(
            graphics_draw_sprite_scaled(g, new_psx, PLAYER_SCREEN_Y,
                                        PLAYER_SPRITE_W, PLAYER_SPRITE_H,
                                        PLAYER_SPRITE, SPRITE_KEY, DISP_W, DISP_H, SPRITE_SCALE),
            TAG, "draw player");
        game->prev_player_sx = new_psx;
    }

    for (int i = 0; i < MONSTER_SPECIES_COUNT; i++) {
        int32_t sx = monster_screen_x(game, i);
        int32_t sy = monster_screen_y(i);
        if (sx == game->prev_monster_sx[i]) continue;

        // clear indicator area too
        int32_t old_sx = game->prev_monster_sx[i];
        int32_t old_sy = game->prev_monster_sy[i];
        const monster_info_t *info = &MONSTER_INFO[i];

        // clear old including potential indicator above
        int32_t clr_y = old_sy - 12;
        uint16_t clr_h = (uint16_t)(info->h * SPRITE_SCALE) + 12u;
        ESP_RETURN_ON_ERROR(clear_sprite_area(g, old_sx, clr_y, (uint16_t)(info->w * SPRITE_SCALE), clr_h), TAG, "clear m");

        // draw at new position
        ESP_RETURN_ON_ERROR(draw_monster(game, i), TAG, "draw m");

        game->prev_monster_sx[i] = sx;
        game->prev_monster_sy[i] = sy;
    }

    return ESP_OK;
}

// ---------------------------------------------------------------
// Monster FSM update
// ---------------------------------------------------------------
static void update_monster_fsm(monster_t *m, uint32_t dt_ms, uint32_t *rng)
{
    m->state_timer_ms += dt_ms;

    if (m->state_timer_ms >= m->state_duration_ms) {
        m->state_timer_ms = 0;
        m->state = (monster_state_t)(rng_next(rng) % MSTATE_COUNT);
        m->state_duration_ms = 1000u + (rng_next(rng) % 4000u);

        if (m->state == MSTATE_MOVING) {
            int32_t offset = (int32_t)(rng_next(rng) % 101u) - 50;
            m->target_x = m->spawn_x + offset;
        }
    }

    if (m->state == MSTATE_MOVING) {
        int32_t dx = m->target_x - m->world_x;
        int32_t step = (MONSTER_WALK_PX_S * (int32_t)dt_ms) / 1000;
        if (step == 0) step = 1;
        if (abs32(dx) <= step) {
            m->world_x = m->target_x;
            m->state = MSTATE_IDLE;
        } else if (dx > 0) {
            m->world_x += step;
        } else {
            m->world_x -= step;
        }
    }
}

// ---------------------------------------------------------------
// Public API
// ---------------------------------------------------------------
esp_err_t monster_game_init(monster_game_t *game, graphics_t *graphics)
{
    ESP_RETURN_ON_FALSE(game != NULL, ESP_ERR_INVALID_ARG, TAG, "game required");
    ESP_RETURN_ON_FALSE(graphics != NULL, ESP_ERR_INVALID_ARG, TAG, "graphics required");

    memset(game, 0, sizeof(*game));
    game->graphics = graphics;
    game->rng = 0xDEADBEEFu;
    game->mode = GMODE_TITLE;
    game->player_world_x = 300;
    game->camera_x = 300 - PLAYER_SCREEN_X;
    game->book_cursor = 0;
    game->needs_full_redraw = true;
    game->photo_species = -1;

    for (int i = 0; i < MONSTER_SPECIES_COUNT; i++) {
        game->monsters[i].species = (monster_species_t)i;
        game->monsters[i].state   = MSTATE_IDLE;
        game->monsters[i].world_x = MONSTER_INFO[i].spawn_x;
        game->monsters[i].spawn_x = MONSTER_INFO[i].spawn_x;
        game->monsters[i].target_x = MONSTER_INFO[i].spawn_x;
        game->monsters[i].rarity  = MONSTER_INFO[i].rarity;
        game->monsters[i].state_duration_ms = 1000u + (rng_next(&game->rng) % 3000u);
        game->prev_monster_sx[i] = -999;
        game->prev_monster_sy[i] = monster_screen_y(i);
    }
    game->prev_player_sx = -999;

    // Load save data
    save_load(&game->save);

    ESP_LOGI(TAG, "Monster game initialized");
    return ESP_OK;
}

esp_err_t monster_game_handle_input(monster_game_t *game,
                                    const button_event_t *events,
                                    size_t event_count)
{
    for (size_t i = 0; i < event_count; i++) {
        const button_event_t *ev = &events[i];

        if (game->mode == GMODE_TITLE) {
            // Any press starts the game
            if (ev->type == BUTTON_EVENT_PRESSED) {
                game->mode = GMODE_EXPLORE;
                game->needs_full_redraw = true;
            }
            continue;
        }

        if (game->mode == GMODE_EXPLORE) {
            if (ev->type == BUTTON_EVENT_PRESSED) {
                if (ev->id == BUTTON_ID_LEFT)  game->left_walk  = true;
                if (ev->id == BUTTON_ID_RIGHT) game->right_walk = true;
            }
            if (ev->type == BUTTON_EVENT_RELEASED) {
                if (ev->id == BUTTON_ID_LEFT)  game->left_walk  = false;
                if (ev->id == BUTTON_ID_RIGHT) game->right_walk = false;
            }
            if (ev->type == BUTTON_EVENT_LONG_PRESSED) {
                if (ev->id == BUTTON_ID_LEFT) {
                    game->left_walk = false;
                    game->mode = GMODE_BOOK;
                    game->needs_full_redraw = true;
                }
                if (ev->id == BUTTON_ID_RIGHT) {
                    game->right_walk = false;
                    game->camera_enter_ms = ev->timestamp_ms;
                    game->mode = GMODE_CAMERA;
                    game->needs_full_redraw = true;
                }
            }
            continue;
        }

        if (game->mode == GMODE_CAMERA) {
            if (ev->type == BUTTON_EVENT_PRESSED && ev->id == BUTTON_ID_LEFT) {
                // Cancel camera
                game->mode = GMODE_EXPLORE;
                game->needs_full_redraw = true;
            }
            if (ev->type == BUTTON_EVENT_RELEASED && ev->id == BUTTON_ID_RIGHT) {
                uint64_t held = ev->timestamp_ms - game->camera_enter_ms;
                if (held >= PHOTO_HOLD_MIN_MS) {
                    int32_t sidx;
                    game->photo_score   = score_photo(game, &sidx);
                    game->photo_species = sidx;

                    game->save.photos_taken++;

                    if (sidx >= 0) {
                        if (game->photo_score > (int32_t)game->save.best_photos[sidx]) {
                            game->save.best_photos[sidx] = (uint32_t)game->photo_score;
                        }
                        if (!game->save.discovered[sidx]) {
                            game->save.discovered[sidx] = true;
                            game->save.species_found++;
                            game->photo_new = true;
                            uint32_t bonus = (MONSTER_INFO[sidx].rarity == 0) ? 10u :
                                             (MONSTER_INFO[sidx].rarity == 1) ? 25u : 50u;
                            game->save.stars += bonus;
                        } else {
                            game->photo_new = false;
                            if (game->photo_score >= 50) game->save.stars += 5u;
                        }
                    }
                    save_commit(&game->save);

                    if (game->photo_new) {
                        game->anim_timer_ms = 0;
                        game->mode = GMODE_DISCOVERY;
                    } else {
                        game->anim_timer_ms = 0;
                        game->mode = GMODE_RESULT;
                    }
                    game->needs_full_redraw = true;
                } else {
                    game->mode = GMODE_EXPLORE;
                    game->needs_full_redraw = true;
                }
            }
            continue;
        }

        if (game->mode == GMODE_DISCOVERY || game->mode == GMODE_RESULT) {
            if (ev->type == BUTTON_EVENT_PRESSED) {
                game->mode = GMODE_EXPLORE;
                game->needs_full_redraw = true;
            }
            continue;
        }

        if (game->mode == GMODE_BOOK) {
            if (ev->type == BUTTON_EVENT_PRESSED) {
                if (ev->id == BUTTON_ID_LEFT) {
                    game->book_cursor = (game->book_cursor + 1) % MONSTER_SPECIES_COUNT;
                    game->needs_full_redraw = true;
                }
                if (ev->id == BUTTON_ID_RIGHT) {
                    game->book_cursor = (game->book_cursor + MONSTER_SPECIES_COUNT - 1) % MONSTER_SPECIES_COUNT;
                    game->needs_full_redraw = true;
                }
            }
            if (ev->type == BUTTON_EVENT_LONG_PRESSED && ev->id == BUTTON_ID_LEFT) {
                game->mode = GMODE_EXPLORE;
                game->needs_full_redraw = true;
            }
            continue;
        }
    }

    return ESP_OK;
}

esp_err_t monster_game_update(monster_game_t *game, uint32_t frame, uint32_t dt_ms)
{
    game->frame = frame;

    if (game->mode == GMODE_EXPLORE || game->mode == GMODE_CAMERA) {
        // Player movement
        int32_t step = (PLAYER_WALK_PX_S * (int32_t)dt_ms) / 1000;
        if (step == 0) step = 1;

        if (game->left_walk && !game->right_walk) {
            game->player_world_x -= step;
        } else if (game->right_walk && !game->left_walk) {
            game->player_world_x += step;
        }
        game->player_world_x = clamp32(game->player_world_x, 0, WORLD_WIDTH);

        // Camera follows player
        int32_t target_cam = game->player_world_x - PLAYER_SCREEN_X;
        game->camera_x = clamp32(target_cam, 0, WORLD_WIDTH - DISP_W);

        // Monster FSM
        for (int i = 0; i < MONSTER_SPECIES_COUNT; i++) {
            update_monster_fsm(&game->monsters[i], dt_ms, &game->rng);
        }
    }

    if (game->mode == GMODE_DISCOVERY) {
        uint32_t prev = game->anim_timer_ms;
        game->anim_timer_ms += dt_ms;
        // Trigger card draw as soon as flash phases end
        if (prev < 500u && game->anim_timer_ms >= 500u) {
            game->needs_full_redraw = true;
        }
        if (game->anim_timer_ms >= DISCOVERY_MS) {
            game->anim_timer_ms = 0;
            game->mode = GMODE_RESULT;
            game->needs_full_redraw = true;
        }
        return ESP_OK;
    }

    if (game->mode == GMODE_RESULT) {
        game->anim_timer_ms += dt_ms;
        if (game->anim_timer_ms >= RESULT_MS) {
            game->mode = GMODE_EXPLORE;
            game->needs_full_redraw = true;
        }
    }

    return ESP_OK;
}

esp_err_t monster_game_render(monster_game_t *game)
{
    switch (game->mode) {
    case GMODE_TITLE:
        if (game->needs_full_redraw) {
            ESP_RETURN_ON_ERROR(draw_title(game), TAG, "title render");
            game->needs_full_redraw = false;
        }
        return ESP_OK;

    case GMODE_EXPLORE:
        return render_explore(game);

    case GMODE_CAMERA:
        ESP_RETURN_ON_ERROR(render_explore(game), TAG, "camera bg");
        if (game->needs_full_redraw) {
            ESP_RETURN_ON_ERROR(draw_camera_overlay(game), TAG, "camera overlay");
            game->needs_full_redraw = false;
        }
        return ESP_OK;

    case GMODE_DISCOVERY:
        return draw_discovery(game);

    case GMODE_RESULT:
        if (game->needs_full_redraw) {
            ESP_RETURN_ON_ERROR(draw_result(game), TAG, "result render");
            game->needs_full_redraw = false;
        }
        return ESP_OK;

    case GMODE_BOOK:
        return draw_book(game);

    default:
        return ESP_OK;
    }
}
