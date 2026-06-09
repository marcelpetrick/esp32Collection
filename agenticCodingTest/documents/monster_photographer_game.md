# Monster Photographer

A discovery-and-collection game designed specifically for:

* 120×240 color display
* 2 physical buttons
* Short and long presses
* 8-year-old players
* Simple implementation by an AI coding agent

The core loop is:

**Explore → Find Monster → Observe Behavior → Take Photo → Complete Collection Book → Unlock New Areas**

---

# Controls

## Exploration

| Input   | Action            |
| ------- | ----------------- |
| A short | Move left         |
| B short | Move right        |
| A long  | Open Monster Book |
| B long  | Take Photo        |

Long presses are infrequent and intentional.

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

Display layout:

```
+------------------+
|    Sky Area      |
|                  |
|    Monster       |
|                  |
|                  |
|                  |
| Player           |
+------------------+
A=Left  B=Right
```

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

Player holds B.

Camera overlay appears.

```
+------------------+
|      [ ]         |
|                  |
|   Fluffalo       |
|                  |
+------------------+
```

Release after hold duration.

Photo scored.

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

A long press on A opens collection book.

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

Tiny save file.

```json
{
  "stars": 530,
  "photos_taken": 144,
  "species_found": 37,
  "best_photos": {}
}
```

Suitable for microcontrollers.

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
