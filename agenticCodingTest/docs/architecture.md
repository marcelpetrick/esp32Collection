# Tiny Blaster Runner — Architecture

> C4 model documentation for the ESP32 arcade firmware.  
> Diagrams use [Mermaid](https://mermaid.js.org/) syntax, rendered by GitHub, GitLab, and most modern Markdown viewers.

---

## Level 1 — System Context

Who uses the system and what external systems does it talk to?

```mermaid
C4Context
    title System Context — Tiny Blaster Runner

    Person(player, "Player", "Presses two physical buttons\nto jump and shoot")

    System(device, "TENSTAR T-Display ESP32", "Runs the arcade game firmware.\nDisplays 135×240 colour screen.\nStores high score in flash.")

    System_Ext(espidf, "ESP-IDF v5.5", "Espressif IoT Development Framework.\nProvides RTOS, SPI driver, NVS, GPIO.")

    System_Ext(nvs, "NVS Flash Partition", "Non-volatile storage on the 16 MB\nflash chip. Survives power cycles.")

    Rel(player,  device, "Presses buttons", "GPIO interrupt / poll")
    Rel(device,  nvs,    "Reads / writes high score", "NVS API")
    Rel(device,  espidf, "Built on top of", "C library calls")
```

---

## Level 2 — Container Diagram

The firmware is a single binary. Logically it is split into four layers:

```mermaid
C4Container
    title Containers — Firmware Layers

    Person(player, "Player")

    Container_Boundary(fw, "ESP32 Firmware (tdisplay_games.elf)") {
        Container(hal,      "Hardware Abstraction",  "C / ESP-IDF drivers",  "ST7789 display SPI, GPIO button debounce, board pin constants")
        Container(engine,   "Game Engine",           "C",                    "Game loop (30 fps target), graphics primitives, sprite renderer")
        Container(game,     "Game Logic",            "C",                    "Tiny Blaster Runner — all gameplay, physics, spawning, scoring")
        Container(persist,  "Persistence",           "NVS",                  "High-score read/write across power cycles")
    }

    Rel(player,   hal,     "Button presses",         "GPIO")
    Rel(hal,      engine,  "button_event_t stream",  "poll / callback")
    Rel(engine,   game,    "on_input / on_update\non_render callbacks", "function pointers")
    Rel(game,     engine,  "graphics_* API calls",   "fill_rect, draw_sprite…")
    Rel(engine,   hal,     "pixel data",             "SPI DMA → ST7789")
    Rel(game,     persist, "nvs_get_u32 / set_u32",  "NVS namespace \"tbr\"")
```

---

## Level 3 — Component Diagram

Each source file is a component. Arrows show compile-time `#include` / call dependencies.

```mermaid
C4Component
    title Components — main/ source files

    Container_Boundary(main, "main/") {
        Component(app,      "app_main.c",        "entry point",   "Boots hardware, wires callbacks into game_loop_run()")
        Component(gl,       "game_loop.c/.h",    "engine",        "30 fps loop: poll buttons → on_input → on_update → on_render → vsync wait")
        Component(gr,       "graphics.c/.h",     "renderer",      "fill_rect, draw_sprite_scaled, draw_text (5×7 bitmap font)")
        Component(sg,       "shooter_game.c/.h", "game",          "All game state, physics, spawning, collision, rendering dispatch")
        Component(spr,      "shooter_sprites.h", "assets",        "Inline RGB565 pixel-art: robot (3 frames), ground bot, drone, coin (4 frames), heart, rock, crate")
        Component(bi,       "button_input.c/.h", "input",         "GPIO debounce (30 ms), PRESSED / RELEASED / LONG_PRESSED events")
        Component(disp,     "st7789_display.c",  "HAL",           "SPI init, window set, DMA pixel push")
        Component(bc,       "board_config.c/.h", "HAL",           "Pin assignments for TENSTAR T-Display (SPI, GPIO, ADC)")
    }

    Rel(app,  gl,   "game_loop_run()")
    Rel(app,  sg,   "shooter_game_init()")
    Rel(gl,   bi,   "button_input_poll()")
    Rel(gl,   disp, "st7789_display_push_frame()")
    Rel(sg,   gr,   "graphics_fill_rect / draw_sprite_scaled / draw_text")
    Rel(sg,   spr,  "#include shooter_sprites.h")
    Rel(gr,   disp, "st7789_display_push_pixels()")
    Rel(bi,   bc,   "BOARD_BUTTON_LEFT_GPIO / RIGHT_GPIO")
    Rel(disp, bc,   "BOARD_TFT_* pin constants")
```

---

## Level 4 — Code: Game Loop Data Flow

How one 33 ms frame moves through the system:

```mermaid
sequenceDiagram
    participant OS  as FreeRTOS tick
    participant GL  as game_loop.c
    participant BI  as button_input.c
    participant SG  as shooter_game.c
    participant GR  as graphics.c
    participant SPI as ST7789 SPI

    OS  ->> GL  : task wakes (33 ms elapsed)
    GL  ->> BI  : button_input_poll()
    BI  -->> GL : button_event_t[N]
    GL  ->> SG  : on_input(events, N)
    note over SG: handle jump / shoot presses,\nstate transitions (menu→play…)
    GL  ->> SG  : on_update(frame, dt_ms)
    note over SG: physics, scroll, collisions,\nspawning, score, difficulty
    GL  ->> SG  : on_render(frame, dt_ms)
    SG  ->> GR  : graphics_fill_rect (background bands)
    SG  ->> GR  : graphics_draw_sprite_scaled ×N (obstacles, coins, enemies, player)
    SG  ->> GR  : graphics_fill_rect (projectiles, particles)
    SG  ->> GR  : graphics_draw_text (HUD score, floating "+N" text)
    GR  ->> SPI : st7789_display_push_pixels (DMA burst)
    GL  ->> OS  : vTaskDelay(remaining budget)
```

---

## Level 4 — Code: Game State Machine

```mermaid
stateDiagram-v2
    direction LR

    [*] --> SPLASH : power on

    SPLASH --> SPLASH  : state_ms < 2500 ms\n(idle, blinking prompt)
    SPLASH --> MENU    : any button press\n— OR —\n2.5 s timeout

    MENU   --> PLAY      : LEFT button
    MENU   --> MENU      : RIGHT button\n(reserved for future skin select)

    PLAY   --> PLAY      : game running\n(update 30 fps)
    PLAY   --> GAMEOVER  : lives == 0\n→ save hi-score to NVS

    GAMEOVER --> PLAY    : LEFT button (retry)
    GAMEOVER --> MENU    : RIGHT button
```

---

## Level 4 — Code: Key Data Structures

```mermaid
classDiagram
    class shooter_game_t {
        +graphics_t* graphics
        +sg_state_t state
        +sg_player_t player
        +sg_projectile_t projectiles[3]
        +sg_enemy_t enemies[4]
        +sg_coin_t coins[12]
        +sg_obstacle_t obstacles[4]
        +sg_particle_t particles[24]
        +int32_t score
        +int32_t hi_score
        +int lives
        +float scroll_speed
        +uint32_t game_ms
        +uint32_t rng
    }

    class sg_player_t {
        +float x, y
        +float vy
        +bool in_air
        +bool invuln
        +uint32_t invuln_ms
        +bool visible
        +int run_frame
        +uint32_t shoot_cooldown_ms
    }

    class sg_enemy_t {
        +bool active
        +sg_enemy_type_t type
        +float x, y
        +int anim_frame
    }

    class sg_particle_t {
        +bool active
        +sg_part_type_t type
        +float x, y, vx, vy
        +uint32_t life_ms
        +uint16_t color
        +int value
    }

    shooter_game_t "1" *-- "1"  sg_player_t
    shooter_game_t "1" *-- "4"  sg_enemy_t
    shooter_game_t "1" *-- "24" sg_particle_t
```

---

## Object Pool Limits

| Pool | Max active | Native sprite | Drawn size (2×) |
|------|-----------|---------------|-----------------|
| Projectiles | 3 | 8 × 4 px (rect) | 8 × 4 px |
| Enemies | 4 | 12 × 12 px | 24 × 24 px |
| Coins | 12 | 8 × 8 px | 16 × 16 px |
| Obstacles | 4 | 12 × 10–12 px | 24 × 20–24 px |
| Particles | 24 | 3 × 3 px (rect) | 3 × 3 px |
| Player | 1 | 16 × 16 px | 32 × 32 px |

---

## Physics Constants

| Parameter | Value | Notes |
|-----------|-------|-------|
| Frame rate target | 30 fps | ~33 ms budget |
| Scroll speed (start) | 2.0 px / frame | 60 px/s |
| Scroll speed (max) | 4.0 px / frame | reached after ~60 s |
| Difficulty step | +0.10 px/frame | every 30 s |
| Jump velocity | −7.5 px / frame | upward |
| Jump hold boost | −0.04 px/frame | while held, max 250 ms |
| Gravity | +0.5 px / frame² | per update tick |
| Peak height | ~57 px | above ground line |
| Projectile speed | 5 px / frame | rightward |
| Shot cooldown | 250 ms | 4 shots/s max |
| Invulnerability | 2000 ms | 100 ms blink interval |

---

## NVS Layout

Namespace: `"tbr"`

| Key | Type | Description |
|-----|------|-------------|
| `hi` | `uint32_t` | All-time high score |

---

## Build Pipeline

```
build_flash.sh [build | flash | monitor | deploy | all]
```

```mermaid
flowchart LR
    SRC[Source files] --> BUILD["idf.py build\n(Xtensa GCC + CMake)"]
    BUILD --> BIN["tdisplay_games.bin\n~249 KB"]
    BIN --> FLASH["idf.py flash\n460800 baud SPI-UART"]
    FLASH --> DEVICE["ESP32 /dev/ttyACM0"]
    DEVICE --> MON["idf.py monitor\n115200 baud serial log"]
```

Flash size used: **~249 KB** of 2 MB app partition (**12%** occupied, 88% free).

---

## Display Layout (135 × 240 portrait)

```
┌─────────────────────────────────┐  y = 0
│ ♥ ♥ ♥            0 0 0 4 2 │  y = 0–13   HUD bar (14 px)
├─────────────────────────────────┤  y = 14
│                                 │
│           [ dark sky ]          │  y = 14–73   60 px
│                                 │
│           [ mid sky ]           │  y = 74–153  80 px
│  🪙   💠→      🤖  👾          │
│           [ light sky ]         │  y = 154–207 54 px
├═════════════════════════════════╡  y = 208   ground line (3 px bright)
│         [ ground ]              │  y = 208–239
└─────────────────────────────────┘  y = 239
```

Player is fixed at X = 18, jumps up to Y ≈ 151 from ground baseline Y = 208.
