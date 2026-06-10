# Code Review — Tiny Blaster Runner Firmware

> Date: 2026-06-10  
> Scope: all source files under `main/`  
> Ranked by severity (worst first).

---

## Issue 1 — Font data duplicated across two translation units

**Files:** `main/graphics.c` (lines 13–53) and `main/shooter_game.c` (lines 102–141)

**Problem:**  
`SFB_DIGIT_FONT[10][5]` and `SFB_LETTER_FONT[26][5]` in `shooter_game.c` are byte-for-byte copies of `DIGIT_FONT` and `LETTER_FONT` in `graphics.c`. Both are `static`, so the linker keeps two separate copies in flash. Worse, any glyph edit in one file silently leaves the other stale — there is no compiler or build warning.

**Fix:**  
Extract both tables into a new `main/font_5x7.h` header that both files include. Mark them `static const` inline arrays in the header, or move them to a new `font_5x7.c` with external linkage.

---

## Issue 2 — `game_loop_run` accepts `st7789_display_t *display` and ignores it

**File:** `main/game_loop.c` (line 39), `main/game_loop.h` (line 22)

**Problem:**  
The function signature is `game_loop_run(st7789_display_t *display, button_input_t *buttons, const game_loop_config_t *config)` but the implementation never reads `display`. The caller (`app_main.c` line 88) must pass a pointer that does nothing. A reader studying the signature naturally assumes the loop needs the display for something (vsync? double-buffer swap?) — it doesn't.

**Fix:**  
Remove the `display` parameter from both the declaration and the call site.

---

## Issue 3 — `graphics_draw_sprite[_scaled]` require caller to supply screen dimensions the function already has

**File:** `main/graphics.h` (lines 42–60), `main/graphics.c` (lines 167–300)

**Problem:**  
Both sprite functions take explicit `uint16_t screen_width, uint16_t screen_height` for clipping, but `graphics_t` already holds `display` which holds `board` which exposes `display_width`/`display_height`. Every call site must pass `DISP_W, DISP_H` redundantly. This creates two sources of truth for the same value and requires any future game to know the display size at the call site.

**Fix:**  
Remove the two size parameters; derive them from `graphics->display->board->display_width` and `graphics->display->board->display_height` inside the function.

---

## Issue 4 — Internal pool-size constants exposed in the public header

**File:** `main/shooter_game.h` (lines 12–17)

**Problem:**  
`SG_MAX_PROJECTILES`, `SG_MAX_ENEMIES`, `SG_MAX_COINS`, `SG_MAX_OBSTACLES`, and `SG_MAX_PARTICLES` are implementation details — they size arrays inside `shooter_game_t`. Placing them in the public header means any translation unit that includes `shooter_game.h` can read (and inadvertently depend on) them. Increasing `SG_MAX_COINS` from 12 to 16 becomes a visible API change rather than a private one.

**Fix:**  
Move all five `#define`s to `shooter_game.c`. In `shooter_game.h` keep only the types and the four public functions.

---

## Issue 5 — `graphics_t` is a zero-value wrapper that provides no abstraction

**File:** `main/graphics.h`, `main/graphics.c`

**Problem:**  
`graphics_t` holds exactly one field (`st7789_display_t *display`) and every function in `graphics.c` is a thin pass-through to a `st7789_display_*` function. The struct provides no added state, no testability seam, no capability beyond the raw display pointer. With the framebuffer approach now in use, `shooter_game.c` only calls `graphics_draw_bitmap` — the rest of the graphics API is dead code. The layer adds import complexity and call overhead without benefit.

**Fix:**  
Either: (a) delete `graphics_t` and have the game call `st7789_display_push_pixels` directly; or (b) elevate `graphics_t` to own the framebuffer, making it a real abstraction. Option (b) is the cleaner long-term design.

---

## Issue 6 — `game_loop_run` has no graceful termination path

**File:** `main/game_loop.c` (line 61 — `while (true)`)

**Problem:**  
The loop runs forever. There is no stop flag, no "request exit" API, and no way for a callback to signal that the loop should end. A fatal `esp_err_t` propagates as a hard exit from `app_main`, which triggers a panic / watchdog reset rather than a clean shutdown. Future requirements — OTA, deep sleep, switching games — cannot be implemented without restructuring the entire entry point.

**Fix:**  
Add `volatile bool *stop_requested` to `game_loop_config_t`. Check it once per frame after render. When true, return `ESP_OK`. Callers that don't need graceful shutdown set it to `NULL` (loop continues as before).

---

## Issue 7 — Player `x` coordinate stored as `float` but is an immutable constant

**File:** `main/shooter_game.h` (line 33), `main/shooter_game.c` (line 233, `PLAYER_FIXED_X`)

**Problem:**  
`sg_player_t.x` is `float`, implying it changes. It is initialised to `PLAYER_FIXED_X = 18` and never modified — the world scrolls past a fixed-position player. The `x` field occupies space in the struct, its value is assigned during `reset_play`, and it is read in collision / render code. None of this is necessary. A reader inspecting the struct expects horizontal movement that doesn't exist.

**Fix:**  
Remove `x` from `sg_player_t`. Use `PLAYER_FIXED_X` directly in collision and render code.

---

## Issue 8 — Magic color literal `0x4208u` has no name

**File:** `main/shooter_game.c`, `draw_hud` function

**Problem:**  
The empty-heart outline is drawn with `sfb_outline(hx, 3, HEART_SPR_W, HEART_SPR_H, 0x4208u)`. The value `0x4208` (RGB565 for a dark gray, roughly `#212121`) is not explained and does not appear in the color constant block at the top of the file. It will not be found by anyone searching for `0x42` and it cannot be reused without copy-pasting the magic number.

**Fix:**  
Add `#define C_DARK_GRAY 0x4208u` to the color constants section and use it in `draw_hud`.

---

## Issue 9 — `rng_range` uses inclusive upper bound without any documentation

**File:** `main/shooter_game.c` (lines 203–207)

**Problem:**  
`rng_range(s, lo, hi)` returns a value in **[lo, hi]** inclusive (`lo + rng_next % (hi-lo+1)`). The C ecosystem convention (`rand() % n`, uniform distributions) uses *exclusive* upper bounds. Every call site like `rng_range(&g->rng, 2, 4)` returning 2, 3, or 4 (not just 2 or 3) is a latent off-by-one the next developer will "fix" incorrectly. Additionally, if `hi - lo == INT32_MAX`, the expression `(hi - lo + 1)` wraps to 0, causing undefined behaviour.

**Fix:**  
Add a function-level comment stating the inclusive contract. Add a guard: `if (hi <= lo) return lo;` is already present but the edge case of `hi - lo == INT32_MAX` is not handled. Either document the input restriction or widen the arithmetic to `uint32_t` before the `+1`.

---

## Issue 10 — `st7789_display_draw_pixel` silently issues 3+ SPI transactions per pixel

**File:** `main/st7789_display.c` (line 272–274), `main/st7789_display.h` (line 15)

**Problem:**  
`st7789_display_draw_pixel` is implemented as `st7789_display_draw_rect(x, y, 1, 1, color)`, which calls `set_window` (CASET + RASET + RAMWR = 3 SPI transactions, ~45 µs at 40 MHz) plus a 2-byte DMA transfer. For a single pixel this is unavoidable, but the function is in the public API with no performance caveat. Any developer who reaches for it to draw a line or scatter plot will unknowingly issue hundreds of full SPI setup sequences instead of one streaming write, easily making their render 10–100× slower.

**Fix:**  
Add a one-line doc comment to both the header declaration and the implementation: `/* O(1) SPI transactions but high fixed overhead; use push_pixels for multi-pixel writes */`. This costs zero code change and eliminates the trap for future contributors.

---

## Summary table

| # | Issue | File(s) | Severity |
|---|-------|---------|----------|
| 1 | Font data duplicated | graphics.c, shooter_game.c | High — silent divergence |
| 2 | Dead `display` param in `game_loop_run` | game_loop.c/h | Medium — misleading API |
| 3 | Redundant screen dims in sprite API | graphics.c/h | Medium — overcoupled interface |
| 4 | Internal pool sizes in public header | shooter_game.h | Medium — leaks implementation |
| 5 | `graphics_t` zero-value wrapper | graphics.c/h | Medium — dead abstraction |
| 6 | No `game_loop_run` exit mechanism | game_loop.c | Medium — architectural limit |
| 7 | Immutable `player.x` stored as `float` | shooter_game.h/c | Low — misleading struct |
| 8 | Magic color `0x4208u` | shooter_game.c | Low — readability |
| 9 | `rng_range` inclusive semantics undocumented | shooter_game.c | Low — latent off-by-one |
| 10 | `draw_pixel` performance trap | st7789_display.c/h | Low — API hazard |
