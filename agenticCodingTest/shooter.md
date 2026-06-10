# Game Design Document: Two-Button Side-Scrolling Microcontroller Game

## Working Title

**Tiny Blaster Runner**

Alternative title ideas:

* Pixel Coin Blaster
* Jump & Blast
* Robot Coin Dash
* Pocket Runner
* Mini Hero Runner

---

# 1. High-Level Concept

**Tiny Blaster Runner** is a colorful 2D side-scrolling action runner designed for a small portrait-oriented microcontroller display.

The player controls a small animated character that automatically runs forward while obstacles, enemies, and coins scroll in from the right side of the screen.

The game uses only two physical buttons:

* **Left button:** Jump
* **Right button:** Shoot

The player jumps over obstacles, shoots enemies and breakable objects, collects rotating coins, avoids damage, and tries to achieve the highest possible score.

The game is designed to be simple enough for an 8-year-old to understand quickly, but polished enough to feel like a real arcade game.

---

# 2. Target Hardware

## Display

* Resolution: approximately **120 × 240 pixels**
* Orientation: **portrait mode**
* Color screen
* Small pixel-art sprites
* No audio output assumed

## Input

Two buttons only:

* Left button
* Right button

Each button should primarily use **short press** interactions.

Long press should be avoided as a required gameplay action because holding a button too long may trigger a microcontroller reset or restart behavior.

---

# 3. Display Orientation and Layout

The game is designed for a **portrait screen**.

Assumed display dimensions:

```text
Width:  120 px
Height: 240 px
```

The action scrolls horizontally.

Objects enter from the **right side** of the screen and move toward the player on the **left side**.

## Screen Layout

```text
┌────────────────────────┐
│ ♥♥♥              00000 │
│                        │
│                        │
│        coins           │
│        enemies         │
│                        │
│                        │
│                        │
│                        │
│                        │
│                        │
│                        │
│                        │
│                        │
│ PLAYER →     objects ← │
│ ground line            │
└────────────────────────┘
```

## HUD

Top left:

```text
♥ ♥ ♥
```

Shows remaining player lives.

Top right:

```text
00000
```

Shows current score.

At game start:

```text
Lives: 3
Score: 0
```

---

# 4. Core Game Loop

1. Player starts with 3 lives and score 0.
2. Character automatically runs forward.
3. World scrolls from right to left.
4. Player jumps with the left button.
5. Player shoots with the right button.
6. Coins, enemies, and obstacles appear from the right side.
7. Player collects coins by touching them.
8. Player destroys enemies or breakable obstacles by shooting them.
9. Player loses one life when hit by an enemy or obstacle.
10. After taking damage, the player flashes and becomes temporarily invulnerable.
11. Difficulty gradually increases.
12. Game ends when all lives are lost.
13. Final score and high score are shown.
14. Player can immediately restart.

---

# 5. Controls

## Left Button

### Short Press

The player jumps.

Used for:

* Jumping over obstacles
* Avoiding enemies
* Collecting coins placed in the air

### Long Press

Long press should not be required.

Optional behavior:

* A slightly higher jump may be allowed if the button remains pressed briefly.
* Maximum hold time should be capped.
* The game must not require holding the button for a long duration.

Recommended maximum hold influence:

```text
0 ms – 120 ms: normal jump
120 ms – 250 ms: slightly higher jump
>250 ms: no additional effect
```

This avoids encouraging long holds.

---

## Right Button

### Short Press

The player shoots a projectile to the right.

Used for:

* Destroying enemies
* Destroying breakable obstacles
* Clearing dangerous objects

### Fire Rate Limit

To avoid uncontrolled spamming:

```text
Minimum time between shots: 250 ms
```

This means the player can shoot up to 4 times per second.

---

# 6. Player Character

## Theme Recommendation

The recommended player character is a **small robot explorer**.

This works well because:

* It fits naturally with shooting projectiles.
* It is easy to draw in pixel art.
* It can have colorful animations.
* Kids understand robots quickly.
* Future skins can be added easily.

## Player Sprite

Recommended sprite size:

```text
16 × 16 px
```

Optional larger size:

```text
18 × 18 px
```

The player should stand near the bottom-left area of the screen.

Recommended player position:

```text
X position: 18 px
Ground Y position: 196 px
```

The player stays mostly fixed on the X axis while the world scrolls.

---

# 7. Player Lives and Damage

## Starting Lives

The player starts each run with:

```text
3 lives
```

Displayed as hearts in the top-left corner:

```text
♥ ♥ ♥
```

## Taking Damage

The player takes damage when colliding with:

* Enemy
* Obstacle
* Hazard
* Projectile, if enemies later get projectiles

When damage occurs:

1. Player loses 1 life.
2. Player sprite starts flashing.
3. Player becomes temporarily invulnerable.
4. The object that caused damage is removed or ignored.
5. Gameplay continues if lives remain.

## Invulnerability

After taking damage:

```text
Invulnerability duration: 2 seconds
```

During this time:

* Player cannot take additional damage.
* Player sprite flashes/blinks.
* Collisions with enemies or obstacles do not reduce lives.

## Flashing Animation

Recommended flashing behavior:

```text
Blink interval: 100 ms
Duration: 2000 ms
```

Visual behavior:

* Alternate between normal sprite and bright/white-tinted sprite.
* Optional red flash outline.
* Optional small burst particles.

## Game Over

When lives reach zero:

```text
Lives = 0
```

The game switches to the Game Over screen.

---

# 8. Jump Mechanics

The jump must feel responsive and predictable.

## Basic Jump

Recommended values for a 120 × 240 portrait screen:

```text
Ground Y: 196 px
Jump height: 45 px
Jump duration: 600 ms
Time rising: 300 ms
Time falling: 300 ms
```

At maximum jump height:

```text
Player Y ≈ 151 px
```

## Jump Feel

The jump should be:

* Quick enough to feel responsive
* High enough to collect airborne coins
* Not so high that obstacles become trivial

## Optional Variable Jump

A very short button hold can slightly increase jump height.

Recommended:

```text
Tap jump height: 38 px
Held jump height: 48 px
Maximum hold influence: 250 ms
```

Holding longer than 250 ms should have no extra effect.

## No Double Jump Initially

The initial version should not include double jump.

Reason:

* Keeps controls simple
* Makes level design easier
* Better for first implementation

Double jump can be added later as a power-up.

---

# 9. Shooting Mechanics

## Projectile

When the right button is pressed, the player fires a projectile to the right.

Recommended projectile:

```text
Size: 4 × 4 px
Shape: small glowing energy ball
Color: bright blue, yellow, or green
Speed: 3–5 px per frame
```

## Projectile Behavior

A projectile:

* Moves horizontally to the right
* Disappears when leaving the screen
* Disappears when hitting an enemy
* Disappears when hitting a breakable obstacle

## Shooting Cooldown

```text
Cooldown: 250 ms
```

This prevents excessive projectiles and keeps performance stable.

## Visual Effects

When shooting:

* Small muzzle flash
* Projectile trail
* Brief arm/blaster animation
* Impact burst when it hits something

No sound is available, so visual feedback must make shooting satisfying.

---

# 10. Enemies

Enemies enter from the right side and move left.

Enemies should be colorful, readable, and funny.

## Enemy Examples

### Ground Bot

* Moves along the ground
* Easy to shoot
* Can be jumped over

### Flying Drone

* Appears in the air
* Must be shot or avoided
* Encourages timing

### Bouncing Slime

* Moves in small hops
* Funny animation
* Good for younger players

### Tiny Alien

* Moves faster than normal enemies
* Higher score reward

---

## Enemy Values

Recommended starting enemy parameters:

```text
Ground enemy speed: same as scroll speed
Flying enemy speed: scroll speed + 10%
Enemy sprite size: 12 × 12 px or 16 × 16 px
```

## Enemy Collision

If the player collides with an enemy:

* Player loses 1 life
* Player starts flashing
* Enemy disappears or moves past
* Player becomes invulnerable for 2 seconds

## Enemy Destruction

If a projectile hits an enemy:

* Enemy disappears
* Small explosion animation appears
* Score increases

Recommended score:

```text
Enemy destroyed: +10 points
```

---

# 11. Obstacles

Obstacles also enter from the right and move left.

They create jumping and shooting challenges.

## Obstacle Types

### Low Obstacle

Example:

* Rock
* Crate
* Barrel

Behavior:

* Must be jumped over
* May or may not be shootable

### High Obstacle

Example:

* Tall wall
* Pillar
* Stack of crates

Behavior:

* Cannot always be jumped over
* Often must be shot if breakable
* Used to make shooting meaningful

### Breakable Obstacle

Example:

* Wooden crate
* Ice block
* Energy barrier

Behavior:

* Can be destroyed with one or more shots

Recommended score:

```text
Breakable obstacle destroyed: +5 points
```

### Unbreakable Obstacle

Example:

* Spike block
* Metal wall
* Stone column

Behavior:

* Cannot be destroyed
* Must be avoided by jumping or positioning

---

# 12. Coins

Coins are collectible objects placed in the air or along the path.

## Coin Appearance

Coins should be animated and colorful.

Recommended sprite size:

```text
8 × 8 px
```

## Rotating Coin Animation

The coin should appear to rotate while floating.

Recommended animation:

```text
Frame 1: full coin
Frame 2: narrow coin
Frame 3: thin edge
Frame 4: narrow coin
Frame 5: full coin
```

Animation speed:

```text
Frame change every 100–150 ms
```

## Coin Placement

Coins can be placed:

* Along the ground
* In small arcs
* Above obstacles
* Between enemies
* As reward trails after difficult jumps

## Coin Collection

When player touches a coin:

* Coin disappears
* Score increases
* Small sparkle animation appears
* Optional floating text appears

Recommended score:

```text
Coin collected: +1 point
```

Optional:

```text
Special large coin: +5 points
```

---

# 13. Scoring System

The score starts at:

```text
0
```

## Score Sources

```text
Coin collected:               +1
Enemy destroyed:              +10
Breakable obstacle destroyed: +5
Distance survived:            +1 every second
Mini boss defeated:           +50
```

## HUD Score

The score is shown at the top right.

Example:

```text
00042
```

Recommended formatting:

* Use 5 digits
* Pad with leading zeros

Examples:

```text
00000
00010
00125
01240
```

---

# 14. Difficulty Progression

The game should become harder over time.

## Starting Difficulty

At the beginning:

* Slow scroll speed
* Few enemies
* Easy obstacles
* Coins placed generously

## Scaling

Every 30 seconds:

```text
Scroll speed: +5%
Enemy spawn rate: +10%
Obstacle spawn rate: +5%
Coin patterns: slightly harder
```

## Maximum Difficulty

To keep the game playable:

```text
Maximum scroll speed: 200% of starting speed
```

## Difficulty Phases

### Phase 1: Beginner

Time:

```text
0–30 seconds
```

Features:

* Simple coins
* Low obstacles
* Few enemies

### Phase 2: Normal

Time:

```text
30–90 seconds
```

Features:

* More enemies
* Flying coins
* Breakable obstacles

### Phase 3: Hard

Time:

```text
90–180 seconds
```

Features:

* Faster scroll
* Enemy combinations
* Coins placed above obstacles

### Phase 4: Expert

Time:

```text
180+ seconds
```

Features:

* High speed
* Mixed enemy waves
* Fast decision-making
* Shooting and jumping required together

---

# 15. Visual Feedback Instead of Sound

The hardware is assumed to have no sound output.

Therefore, all feedback must be visual.

## Required Visual Feedback

### Jump

* Squash/stretch animation
* Small dust puff at takeoff
* Small dust puff at landing

### Coin Collection

* Sparkle burst
* Coin disappears
* Floating "+1"

### Shooting

* Muzzle flash
* Projectile glow
* Projectile trail

### Enemy Hit

* Impact flash
* Small explosion
* Floating "+10"

### Player Damage

* Character flashes
* Brief red screen border
* Small star burst
* Optional screen shake

### Game Over

* Player falls or freezes
* Screen darkens slightly
* Game Over text appears

---

# 16. Animation Requirements

## Player Animations

### Running

Recommended:

```text
2–4 frames
```

Loop continuously while on ground.

### Jumping

Recommended:

```text
1–2 frames
```

Different pose than running.

### Shooting

Recommended:

```text
1 frame overlay or arm pose
```

Triggered briefly when shooting.

### Damage

Recommended:

```text
Flashing/blinking animation
```

Duration:

```text
2 seconds
```

---

## Enemy Animations

Each enemy should have simple animation.

Examples:

```text
Ground bot: wheel movement
Drone: spinning propeller
Slime: squash/stretch bounce
Alien: walking feet
```

Recommended:

```text
2–4 frames per enemy
```

---

## Coin Animation

Coins should rotate continuously.

Recommended:

```text
4–5 frames
```

Loop speed:

```text
100–150 ms per frame
```

---

## Explosion Animation

When enemies or obstacles are destroyed:

Recommended:

```text
3–5 frames
Duration: 200–400 ms
```

Visual style:

* Pixel burst
* Smoke puff
* Stars
* Small colorful fragments

---

# 17. Background and World Design

The world should feel alive despite the small screen.

## Parallax Background

Use multiple scrolling layers.

### Layer 1: Far Background

Examples:

* Sky
* Stars
* Gradient
* Distant clouds

Scroll speed:

```text
Very slow
```

### Layer 2: Mid Background

Examples:

* Hills
* Buildings
* Mountains
* Trees

Scroll speed:

```text
Medium slow
```

### Layer 3: Foreground

Examples:

* Ground pattern
* Small rocks
* Grass
* Pipes

Scroll speed:

```text
Same as game world
```

## Why Parallax Matters

Parallax creates a stronger feeling of movement without requiring complex gameplay logic.

---

# 18. Recommended Theme

## Theme: Robot Explorer Adventure

The player is a small robot explorer running through a colorful mechanical world, collecting energy coins and blasting malfunctioning machines.

## Player

Small robot with:

* Bright eyes
* Tiny blaster arm
* Running legs or wheels
* Funny jump pose

## Coins

Energy coins or power crystals.

## Enemies

* Broken drones
* Walking bots
* Bouncing slime machines
* Tiny alien robots

## Obstacles

* Crates
* Metal blocks
* Energy barriers
* Spikes
* Broken pipes

## Visual Tone

* Bright
* Friendly
* Funny
* Non-scary
* Suitable for children

---

# 19. Game States

The game should be organized into clear states.

## State 1: Splash Screen

Shown briefly on startup.

Displays:

```text
Tiny Blaster Runner
```

Optional:

* Animated logo
* Blinking "Press any button"

## State 2: Main Menu

Displays:

```text
TINY BLASTER RUNNER

LEFT: START
RIGHT: SKIN
```

Optional:

```text
BEST: 01240
COINS: 00087
```

## State 3: Gameplay

Main game state.

Contains:

* Player
* Enemies
* Obstacles
* Coins
* Projectiles
* HUD
* Background

## State 4: Game Over

Displays:

```text
GAME OVER

SCORE: 01240
BEST: 01890

LEFT: RETRY
RIGHT: MENU
```

## State 5: Skin Selection

Optional future state.

Allows character skin selection.

---

# 20. Collision Rules

## Player vs Coin

Result:

* Coin collected
* Score +1
* Coin removed
* Sparkle effect

## Player vs Enemy

If not invulnerable:

* Lose 1 life
* Start flashing
* Enemy removed or ignored
* Invulnerability begins

If invulnerable:

* No damage
* Enemy may pass through or disappear

## Player vs Obstacle

If not invulnerable:

* Lose 1 life
* Start flashing
* Obstacle removed or ignored
* Invulnerability begins

If invulnerable:

* No damage

## Projectile vs Enemy

Result:

* Enemy destroyed
* Score +10
* Projectile removed
* Explosion effect

## Projectile vs Breakable Obstacle

Result:

* Obstacle health reduced
* If health reaches zero, obstacle destroyed
* Score +5
* Projectile removed
* Impact effect

## Projectile vs Unbreakable Obstacle

Result:

* Projectile removed
* Small spark effect
* Obstacle remains

---

# 21. Object Spawning

Objects spawn from the right side of the screen.

Spawn X position:

```text
X = 120 px + object width
```

Objects move left each frame.

When object leaves screen:

```text
If X < -object width:
    remove object
```

## Spawn Categories

The game should spawn:

* Coins
* Enemies
* Obstacles
* Decorative objects

## Spawn Timing

Recommended initial spawn timing:

```text
Coin pattern every:      1.5–3.0 seconds
Enemy every:             2.0–4.0 seconds
Obstacle every:          2.5–5.0 seconds
```

As difficulty increases, spawn intervals become shorter.

---

# 22. Coin Patterns

Coins should not appear randomly one by one only.

Use small patterns.

## Pattern 1: Ground Line

Coins placed low.

Purpose:

* Easy reward

## Pattern 2: Jump Arc

Coins form an arc matching the jump path.

Purpose:

* Teaches jumping

## Pattern 3: High Line

Coins placed above obstacles.

Purpose:

* Rewards precise jumping

## Pattern 4: Risk Trail

Coins appear near enemies or hazards.

Purpose:

* Tempts player to take risks

---

# 23. Enemy and Obstacle Patterns

The game should mix simple challenge patterns.

## Pattern 1: Single Rock

Player jumps over it.

## Pattern 2: Enemy on Ground

Player can jump or shoot.

## Pattern 3: Flying Drone

Player should shoot or avoid.

## Pattern 4: Breakable Wall

Player should shoot.

## Pattern 5: Coin Above Rock

Player jumps over rock and collects coin.

## Pattern 6: Enemy After Obstacle

Player jumps, lands, then shoots.

## Pattern 7: Mixed Challenge

Player must shoot and jump close together.

Used only after the player has survived long enough.

---

# 24. Power-Ups

Power-ups are optional but recommended for later versions.

## Shield

Effect:

* Blocks one hit

Visual:

* Glowing bubble around player

## Rapid Fire

Effect:

* Reduces shooting cooldown

Recommended:

```text
Cooldown becomes 120 ms
Duration: 8 seconds
```

## Coin Magnet

Effect:

* Nearby coins move toward player

Duration:

```text
8 seconds
```

## Mega Jump

Effect:

* Jump height increases by 30%

Duration:

```text
8 seconds
```

## Double Score

Effect:

* All score gains doubled

Duration:

```text
10 seconds
```

---

# 25. Mini Boss Concept

Optional for advanced version.

Every certain score interval, a mini boss appears.

Recommended trigger:

```text
Every 1000 points
```

## Example Boss: Giant Drone

Behavior:

* Enters from right
* Moves slowly
* Requires multiple hits
* Avoids making the game too hard for children

## Boss Health

```text
Hits required: 5
```

## Boss Reward

```text
Boss defeated: +50 points
```

## Boss Visuals

* Larger sprite
* Flash when hit
* Explosion when defeated
* Drops coins

---

# 26. Rewards and Long-Term Progression

The game should support repeated play.

## High Score

Store locally:

```text
Best score
```

Displayed on:

* Main menu
* Game over screen

## Total Coins

Optional:

```text
Total coins collected across all runs
```

Used to unlock cosmetic items.

## Unlockable Skins

Optional cosmetic skins:

* Robot
* Pirate robot
* Astronaut robot
* Ninja robot
* Dinosaur robot
* Rainbow robot

Skins should not change gameplay.

This keeps the game fair.

---

# 27. Recommended Initial Balancing Values

## Player

```text
Player sprite: 16 × 16 px
Player X: 18 px
Ground Y: 196 px
Lives: 3
Invulnerability: 2000 ms
Blink interval: 100 ms
```

## Jump

```text
Tap jump height: 38 px
Held jump height: 48 px
Jump duration: 600 ms
Max hold influence: 250 ms
```

## Shooting

```text
Projectile size: 4 × 4 px
Projectile speed: 4 px/frame
Shot cooldown: 250 ms
```

## Coins

```text
Coin sprite: 8 × 8 px
Coin score: +1
Coin animation speed: 100–150 ms/frame
```

## Enemies

```text
Enemy sprite: 12 × 12 px or 16 × 16 px
Enemy destroyed score: +10
```

## Obstacles

```text
Breakable obstacle score: +5
Small obstacle size: 12 × 12 px
Tall obstacle size: 12 × 24 px
```

## Scrolling

```text
Initial scroll speed: 1.5 px/frame
Maximum scroll speed: 3.0 px/frame
```

---

# 28. Performance Considerations

Because this runs on a microcontroller, the implementation should stay simple.

## Recommended Limits

Maximum active objects at once:

```text
Coins:       12
Enemies:      4
Obstacles:    4
Projectiles:  3
Particles:   12
```

## Object Pooling

Use object pools instead of dynamic allocation if possible.

Recommended pools:

* Coin pool
* Enemy pool
* Obstacle pool
* Projectile pool
* Particle pool

## Sprite Sizes

Use small sprites and limited animation frames.

Recommended:

```text
Player:      16 × 16
Enemy:       12 × 12 or 16 × 16
Coin:         8 × 8
Projectile:   4 × 4
Heart:        8 × 8
```

---

# 29. Visual Style Guide

## General Style

* Bright colors
* High contrast
* Clear silhouettes
* Funny, friendly designs
* No scary or violent graphics

## Recommended Colors

Player:

* Blue body
* White eyes
* Yellow highlights

Coins:

* Gold/yellow
* Orange shading

Enemies:

* Purple, green, red, or gray
* Distinct from player

Projectiles:

* Bright cyan, green, or yellow

Damage:

* Red flash
* White blink
* Screen border flash

## Readability Rule

Every important object must be recognizable at a glance.

If an object cannot be recognized on a 120 × 240 display, simplify it.

---

# 30. No-Sound Design Rule

Because the game may not have sound, every important event needs visible feedback.

## Every Action Needs a Visual Response

Jump:

* Dust puff

Shoot:

* Muzzle flash

Coin:

* Sparkle

Hit enemy:

* Explosion

Take damage:

* Flashing player and screen border

Lose final life:

* Big Game Over transition

New high score:

* Celebration animation

---

# 31. Example Gameplay Moment

The player is running near the bottom of the screen.

A low rock appears from the right.

Above the rock, three coins rotate in an arc.

Behind the rock, a flying drone appears.

The player presses the left button to jump over the rock and collect the coins.

While landing, the player presses the right button to shoot the drone.

The projectile hits the drone.

The drone explodes into small particles.

The player receives:

```text
+3 coins
+10 enemy score
+1 distance bonus
```

The score increases and the run continues.

---

# 32. Minimum Viable Version

The first playable version should include only the essential features.

## Required for Version 1

* Portrait display support
* Player running animation
* Left button jump
* Right button shoot
* 3 lives
* Score counter
* Coins with rotation animation
* One enemy type
* One obstacle type
* Player damage flashing
* Invulnerability after damage
* Game over screen
* Restart

## Not Required for Version 1

* Power-ups
* Bosses
* Unlockable skins
* Multiple worlds
* Complex enemy AI
* Permanent coin economy

---

# 33. Version 2 Features

After Version 1 works, add:

* More enemy types
* Breakable obstacles
* Multiple coin patterns
* Parallax background
* High score saving
* More animation polish
* Floating score numbers
* Improved particles

---

# 34. Version 3 Features

Later advanced features:

* Power-ups
* Mini bosses
* Unlockable skins
* Multiple background themes
* Permanent coin counter
* More complex level patterns

---

# 35. Implementation Notes

## Suggested Game Systems

The implementation can be organized into these systems:

```text
Input Manager
Player Controller
World Scroller
Coin Manager
Enemy Manager
Obstacle Manager
Projectile Manager
Collision Manager
Score Manager
HUD Renderer
Particle System
Game State Manager
```

## Game State Flow

```text
Splash Screen
     ↓
Main Menu
     ↓
Gameplay
     ↓
Game Over
     ↓
Retry or Menu
```

## Main Update Loop

Pseudo-logic:

```text
read buttons

if state == MENU:
    handle menu input

if state == GAMEPLAY:
    update player
    update jump physics
    update projectiles
    update enemies
    update obstacles
    update coins
    update particles
    check collisions
    update score
    update difficulty
    render frame

if state == GAME_OVER:
    handle retry/menu input
```

---

# 36. Essential Design Rules

## Rule 1: Two Buttons Only

All gameplay must work with:

```text
Left = Jump
Right = Shoot
```

No required long press.

No complicated combinations.

## Rule 2: Always Readable

The player must immediately understand:

* Where the character is
* What is dangerous
* What can be collected
* What can be shot
* How many lives remain
* What the score is

## Rule 3: No Instant Frustration

The player has 3 lives.

After each hit, the player becomes invulnerable briefly.

This prevents unfair repeated damage.

## Rule 4: Visual Feedback Replaces Sound

Since there is no sound, the game must use:

* Flashing
* Particles
* Screen shake
* Sparkles
* Floating text
* Color changes

## Rule 5: Fun Within 5 Seconds

A new player should understand the game almost immediately:

```text
Left button jumps.
Right button shoots.
Coins are good.
Enemies are bad.
Hearts are lives.
Score goes up.
```

---

# 37. One-Sentence Pitch

**Tiny Blaster Runner is a colorful two-button portrait-mode arcade runner where a cute robot jumps over obstacles, shoots enemies, collects rotating coins, survives with three lives, and chases a high score using clear visual effects instead of sound.**
