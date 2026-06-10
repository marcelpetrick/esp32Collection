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

---

## #3 — Unit testing (Unity / host-target build)

**What:** Add a `test/` component that compiles and runs pure-logic code on the host
(Linux) using ESP-IDF's native host-target support and the bundled Unity framework.
No hardware required; runs in CI or locally in seconds.

**What to test first:**
- `rng_range` — boundary behaviour, hi==lo edge case, no signed overflow
- `aabb` — overlap/touch/gap correctness for all axis combinations
- Difficulty scaler — score thresholds produce expected speed/spawn-rate values
- Spawn spacing — consecutive spawn positions never overlap

**Implementation sketch:**
```
test/
  CMakeLists.txt          # idf_component_register + unity
  main/
    CMakeLists.txt
    test_rng.c
    test_aabb.c
    test_difficulty.c
```
Build and run: `idf.py -C test --target linux build && ./build/test.elf`  
Pure-logic files (`shooter_game.c` game-math sections) compile for Linux without
any ESP-IDF HAL dependency; HAL calls are either absent in the tested units or
stubbed with a thin shim header.

**Pipeline hook:** `ci.sh` runs host tests before flashing. Non-zero exit blocks the flash.

**Effort:** Medium — Unity is already in ESP-IDF; the main work is identifying the
pure-logic boundary and writing the first ~20 test cases.

---

## #4 — Coverage report (gcov / lcov)

**What:** After the host-target unit tests pass, generate a line/branch coverage HTML
report showing which parts of the game logic are exercised by the test suite.

**How it works:**
- Compile the host test build with `-fprofile-arcs -ftest-coverage` (add to
  `test/CMakeLists.txt` via `target_compile_options` and `target_link_options`).
- Run the test binary — `.gcda` / `.gcno` files are emitted alongside object files.
- `lcov --capture --directory build/ --output-file coverage.info`
- `lcov --remove coverage.info '*/esp-idf/*' --output-file coverage_filtered.info`
- `genhtml coverage_filtered.info --output-directory docs/coverage/`

**Pipeline hook:** add a `coverage` target to `build_flash.sh` (or `ci.sh`):
```
build_flash.sh coverage   →  builds host tests with gcov flags,
                              runs them, generates docs/coverage/index.html
```
Print a one-line summary at the end: `Coverage: 74.3% lines, 61.8% branches`.

**Prerequisites:** `lcov` and `genhtml` must be installed on the dev machine
(`pacman -S lcov` on Manjaro). `gcov` ships with GCC.

**Effort:** Small once #3 exists — mostly CMake flags and three shell commands.
Does not work for the on-device build (Xtensa GCC produces `.gcda` files on the
target filesystem, which this board doesn't expose; host-only is the practical path).

---

## #5 — Doxygen documentation generation

**What:** Auto-generate HTML API docs from source comments using Doxygen, covering
the public headers (`st7789_display.h`, `graphics.h`, `game_loop.h`, `shooter_game.h`,
`button_input.h`). Output goes to `docs/html/`.

**Doxyfile key settings:**
```
PROJECT_NAME     = "Tiny Blaster Runner"
INPUT            = main/
FILE_PATTERNS    = *.h *.c
RECURSIVE        = NO
EXTRACT_ALL      = YES
GENERATE_HTML    = YES
HTML_OUTPUT      = ../docs/html
GENERATE_LATEX   = NO
HAVE_DOT         = YES          # optional: call-graph diagrams via Graphviz
CALL_GRAPH       = YES
CALLER_GRAPH     = YES
```
A `Doxyfile` in the project root is the single source of truth; committed to the repo.

**Pipeline hook:** add a `docs` target to `build_flash.sh`:
```
build_flash.sh docs   →  runs doxygen, prints "Docs generated: docs/html/index.html"
```
`ci.sh` can run `docs` after tests and fail if Doxygen exits non-zero (catches
malformed comment syntax).

**Prerequisites:** `doxygen` (+ optionally `graphviz` for call graphs) —
`pacman -S doxygen graphviz` on Manjaro.

**Effort:** Small — write the `Doxyfile`, add the shell target, add minimal `/** */`
doc-comments to the six public headers. No existing comments need to change.
