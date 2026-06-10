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
