# Tiny Blaster Runner — Ideas Backlog

---

## #0 — Commander Keen-style player sprite

**What:** Replace the current robot sprite with a character styled after Commander Keen
from *Commander Keen 4: Secret of the Oracle* (id Software, 1991). Same native 16×16 px
resolution (rendered at 2× = 32×32 on screen).

**Why it fits:** The side-scrolling platformer genre, the chunky pixel art scale, and the
bright primary-colour palette all map naturally onto Keen's look — helmet, red shirt,
green pants, pogo stick energy. The existing three-frame animation set (run0, run1, jump)
maps directly to Keen's idle/walk and jump poses.

**Feasibility:** Yes. The current sprite slot is 16×16 RGB565 with a magenta transparency
key (`0xF81F`). Three frames are needed (run0, run1, jump). An authentic Keen 4 likeness
cannot be distributed (copyrighted), but a clearly inspired/homage sprite in the same
chunky style is straightforward to hand-author as hex RGB565 arrays in
`main/shooter_sprites.h`. No code changes required — only new pixel data.

**Effort:** Medium. Requires careful hand-pixelling of ~3 × 256 colour values, or
conversion from a reference image via a small Python script (image → RGB565 hex array).

---

## #1 — Double jump

**What:** While the player is airborne after a first jump, pressing the jump button once
more launches a second upward impulse from the current mid-air position. The second jump
is consumed immediately; a third press in the same airborne phase has no effect. The
ability resets the moment the player lands.

**Design details:**
- Add `bool double_jump_used` to `sg_player_t`.
- On jump button press: if `in_air && !double_jump_used` → apply the same initial upward
  velocity as a ground jump, set `double_jump_used = true`.
- On landing: clear both `in_air` and `double_jump_used`.
- Spawn a small particle burst at the player's current position on the second jump to give
  visual feedback.

**Effort:** Small — ~15 lines of logic in `shooter_game_update`, one new field in the
struct.

---

## #2 — Local build / test / flash pipeline

### Current state

| Step | Tool | Status |
|------|------|--------|
| Build | `build_flash.sh build` | Works. Calls `idf.py build` via ESP-IDF 5.5.4. |
| Flash | `build_flash.sh flash` / `deploy` | Works. 460800 baud via esptool. |
| Monitor | `build_flash.sh monitor` | Works. 115200 baud serial. |
| Unit tests | — | **Missing entirely.** No host-side tests, no Unity test component. |
| On-device smoke test | `display_smoke_test.c` | **Dead code.** File exists but is not registered in `main/CMakeLists.txt` and is never called. |
| End-of-run summary | — | **Missing.** `build_flash.sh` prints no final summary line — no binary size, no timing, no PASS/FAIL. |

### What a complete pipeline would look like

1. **`build_flash.sh` summary line** — after every `build` or `deploy` run, print one
   line: binary size, partition free %, and elapsed wall-clock seconds.  
   Example: `✓ build complete — 246 KB / 2048 KB (88% free) in 4.2 s`

2. **Wire the smoke test** — add `display_smoke_test.c` to `CMakeLists.txt`, add an
   opt-in call in `app_main` gated on a compile flag (`CONFIG_RUN_SMOKE_TEST`), so a
   normal flash runs the game but `idf.py -DCONFIG_RUN_SMOKE_TEST=y build flash` runs
   the colour-fill / pattern test first and logs PASS/FAIL.

3. **Host-side unit tests for pure logic** — game logic functions (`rng_range`,
   `aabb`, difficulty scaling, spawn spacing) have no hardware dependency and can be
   tested on the host with a minimal Unity harness under `test/`. Run with
   `idf.py -C test build` (ESP-IDF native host-target support).

4. **One-command CI script** — a `ci.sh` (or Makefile target) that runs:
   build → size check → host unit tests → (optionally) flash + smoke test.
   Exits non-zero on any failure so it can gate commits or be run pre-push.

**Effort:**  
- Summary line: trivial (5 lines of bash).  
- Wire smoke test: small (CMakeLists + Kconfig entry + one `#ifdef` in app_main).  
- Host unit tests: medium (Unity component, ~3–5 test files for the pure-logic layer).  
- CI script: small once the above exist.
