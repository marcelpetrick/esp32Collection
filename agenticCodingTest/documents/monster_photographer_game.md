# Monster Photographer

A discovery-and-collection game designed specifically for:

* 135×240 ST7789 color display (TENSTAR T-Display ESP32 clone)
* 2 physical buttons: left (GPIO0) and right (GPIO35)
* Short and long presses (long = held ≥ 400 ms)
* 8-year-old players
* Simple implementation by an AI coding agent

The core loop is:

**Explore → Find Monster → Observe Behavior → Take Photo → Complete Collection Book → Unlock New Areas**

---

# Controls

## Exploration

| Input         | Action                    |
| ------------- | ------------------------- |
| Left short    | Move left                 |
| Right short   | Move right                |
| Left long     | Open Monster Book         |
| Right long    | Enter camera / take photo |

"Left" = GPIO0 button, "Right" = GPIO35 button.
Long press = held ≥ 400 ms (fires once mid-hold, not on release).
Short press = walk (continuous while held, stops on release).

---

# Game World

The world consists of horizontal "biomes".

Example:

```
Forest
↓
Crystal Cave
↓
Beach
↓
Volcano
↓
Cloud Kingdom
```

Each biome is a single scrolling scene.

Display layout (135 wide × 240 tall, portrait):

```
+------------------+  y=0
|    Sky / clouds  |
|                  |
|    Monster       |  y≈145
|                  |
|                  |
+~~~~~~~~~~~~~~~~~~+  y=170 grass line
|    Ground        |  y=170-210
| [Player]         |  y=192  (sprite 10×16 px)
+==================+  y=228
|  L:BOOK  R:PHOTO |  y=228-240 HUD
+------------------+  y=240
```

SPI display clock is 20 MHz (ESP-IDF rejected 40 MHz on this GPIO-matrix route).

---

# Main Character

Small photographer kid.

Can only:

* walk left
* walk right
* take photos

No combat.

No health.

No dying.

This removes frustration.

---

# Monster System

Every biome contains monsters.

Each monster has:

```json
{
  "name": "Fluffalo",
  "rarity": 2,
  "color": "blue",
  "size": "large",
  "behavior": "hops"
}
```

Example monsters:

### Forest

* Fluffalo
* Twiglet
* Moss Mouse

### Cave

* Crystal Bat
* Glow Worm
* Rock Hopper

### Volcano

* Lava Puff
* Fire Gecko
* Magma Snail

---

# Monster Behaviors

Simple finite-state machine.

States:

```
idle
moving
sleeping
playing
eating
```

Every few seconds:

```python
change_state()
```

This creates life with little code.

---

# Photographing

Player holds Right button (≥ 400 ms long press).

Camera overlay appears immediately on long-press event.

```
+------------------+
|  [  VIEWFINDER ] |
|  |            |  |
|  |  Fluffalo  |  |
|  |            |  |
|  +------------+  |
+------------------+
```

Right button release = photo taken and scored.
Left button press while in camera mode = cancel and return to exploring.

---

# Photo Quality

Simple score.

```
quality =
  distance_score +
  center_score +
  rarity_bonus
```

Example:

```
Amazing Photo!
87 Points
```

Kids understand this immediately.

---

# Discovery Mechanic

The first photo of a species unlocks:

```
NEW MONSTER!
FLUFFALO
```

Animation:

* flash
* star particles
* monster card

Very rewarding.

---

# Monster Book

A long press on the Left button opens collection book. Left/Right short press navigates entries. Left long press closes book.

```
MONSTER BOOK

1. Fluffalo ✓
2. Twiglet ✓
3. ????
4. ????
```

Selecting a monster shows:

```
FLUFFALO

Likes:
Blue Flowers

Found:
Forest

Best Photo:
92
```

Kids love filling collections.

---

# Rare Variants

Every monster has:

```python
normal = 90%
rare = 9%
legendary = 1%
```

Example:

### Normal

Green Fluffalo

### Rare

Golden Fluffalo

### Legendary

Rainbow Fluffalo

Book shows:

```
Seen: No
```

until found.

Creates long-term goals.

---

# Time-of-Day System

Every few minutes:

```
Morning
Day
Evening
Night
```

Different monsters appear.

Example:

| Time  | Monster   |
| ----- | --------- |
| Day   | Fluffalo  |
| Night | Moon Puff |

Easy implementation.

Huge replay value.

---

# Quest System

Simple NPC animals.

Example:

```
Squirrel Reporter:

Can you photograph
a sleeping Glow Worm?
```

Reward:

```
+50 Stars
```

Stars unlock biomes.

---

# Biome Unlocks

Need stars.

```
Forest      Free
Beach       100
Cave        250
Volcano     500
Clouds      1000
```

Provides progression.

---

# Dynamic Events

Occasionally:

```
METEOR SHOWER!
```

or

```
MONSTER FESTIVAL!
```

Special monsters spawn.

Makes the world feel alive.

---

# Procedural Monster Generator

For endless content.

Monster generated from:

```python
body
eyes
color
ears
behavior
special_effect
```

Example output:

```
Sparklepuff
Purple
3 Eyes
Hops
Leaves Stars
```

Children love discovering weird combinations.

---

# Save Data

Stored in ESP-IDF NVS (non-volatile storage), namespace `"mpg"`.

| NVS key  | Type | Meaning                         |
| -------- | ---- | ------------------------------- |
| `stars`  | u32  | total stars earned              |
| `taken`  | u32  | total photos taken              |
| `found`  | u32  | species discovered count        |
| `best0`  | u32  | best photo score, species 0     |
| `best1`  | u32  | best photo score, species 1     |
| `best2`  | u32  | best photo score, species 2     |
| `disc`   | u8   | bitmask of discovered species   |

Loaded at boot, saved after each photo. Survives power cycles.
NVS is at flash offset `0x9000`, 20 KB, already partitioned.

---

# The Secret Ingredient

The game should never say:

> "You lost."

Instead it should constantly say:

> "You discovered something."

Examples:

* New species
* Better photo
* Rare color
* New biome
* Fun monster fact
* Hidden event

That turns the game from a score-chasing arcade game into a curiosity machine, which tends to work extremely well for 8-year-olds.
