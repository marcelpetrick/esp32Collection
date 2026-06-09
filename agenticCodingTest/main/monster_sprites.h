#pragma once
#include <stdint.h>

// Transparent key: magenta (does not appear in any sprite palette)
#define SPRITE_KEY 0xF81Fu

// --- sprite dimensions ---
#define PLAYER_SPRITE_W  10u
#define PLAYER_SPRITE_H  16u
#define FLUF_SPRITE_W    14u
#define FLUF_SPRITE_H    10u
#define TWIG_SPRITE_W     8u
#define TWIG_SPRITE_H    22u
#define MMOU_SPRITE_W    14u
#define MMOU_SPRITE_H    11u

// --- color palette ---
// Player
#define _K   0xF81Fu  // transparent
#define _HAT 0x7A22u  // dark brown hat
#define _SKN 0xFEF7u  // peach skin
#define _EYE 0x0000u  // black eye dot
#define _SHT 0x1C9Fu  // bright blue shirt
#define _CAM 0xA514u  // camera body gray
#define _CDK 0x3A8Cu  // camera lens dark
#define _PNT 0x298Au  // dark navy pants
#define _SHO 0x5141u  // dark brown shoes

// Fluffalo (blue blob, common)
#define _FM  0x2B3Bu  // medium blue body
#define _FL  0x651Fu  // light blue edge
#define _EW  0xFFFFu  // white of eye
#define _EP  0x00AFu  // dark blue pupil
#define _PK  0xFCB6u  // pink rosy cheek
#define _SM  0x094Cu  // dark smile corner

// Twiglet (green twig creature, rare)
#define _TL  0x3E47u  // bright leaf green
#define _TG  0x13C2u  // dark green body
#define _TB  0x7A83u  // trunk brown
#define _TY  0xFF82u  // glowing yellow eye
#define _TP  0x01E0u  // green pupil in yellow

// Moss Mouse (mossy mouse, legendary)
#define _MB  0xB367u  // mouse brown body
#define _ME  0xFD16u  // pink ear inner
#define _MO  0x5647u  // bright moss green
#define _MN  0xFC74u  // pink nose
#define _MW  0xFFFFu  // white eye
#define _MP  0x0000u  // black pupil

// =============================================================
// PLAYER SPRITE  10 x 16
// A cheerful photographer kid with a blue shirt and camera
// =============================================================
static const uint16_t PLAYER_SPRITE[PLAYER_SPRITE_W * PLAYER_SPRITE_H] = {
    _K,   _K,   _K,   _HAT, _HAT, _HAT, _HAT, _K,   _K,   _K,   // r0  hat top
    _K,   _K,   _HAT, _HAT, _HAT, _HAT, _HAT, _HAT, _K,   _K,   // r1  hat brim
    _K,   _K,   _SKN, _SKN, _SKN, _SKN, _SKN, _SKN, _K,   _K,   // r2  forehead
    _K,   _K,   _SKN, _EYE, _SKN, _SKN, _EYE, _SKN, _K,   _K,   // r3  eyes
    _K,   _K,   _SKN, _SKN, _SKN, _SKN, _SKN, _SKN, _K,   _K,   // r4  cheeks/chin
    _K,   _SHT, _SHT, _SHT, _SHT, _SHT, _SHT, _SHT, _K,   _K,   // r5  shirt top
    _K,   _SHT, _SHT, _CAM, _CAM, _CAM, _SHT, _SHT, _K,   _K,   // r6  camera outline
    _K,   _SHT, _SHT, _CAM, _CDK, _CAM, _SHT, _SHT, _K,   _K,   // r7  camera lens
    _K,   _SHT, _SHT, _CAM, _CAM, _CAM, _SHT, _SHT, _K,   _K,   // r8  camera bottom
    _K,   _K,   _SHT, _SHT, _SHT, _SHT, _SHT, _K,   _K,   _K,   // r9  waist
    _K,   _K,   _PNT, _PNT, _K,   _K,   _PNT, _PNT, _K,   _K,   // r10 pants
    _K,   _K,   _PNT, _PNT, _K,   _K,   _PNT, _PNT, _K,   _K,   // r11
    _K,   _K,   _PNT, _PNT, _K,   _K,   _PNT, _PNT, _K,   _K,   // r12
    _K,   _K,   _PNT, _PNT, _K,   _K,   _PNT, _PNT, _K,   _K,   // r13
    _K,   _K,   _SHO, _SHO, _K,   _K,   _SHO, _SHO, _K,   _K,   // r14 shoes
    _K,   _SHO, _SHO, _SHO, _K,   _K,   _SHO, _SHO, _SHO, _K,   // r15 wider sole
};

// =============================================================
// FLUFFALO SPRITE  14 x 10
// A fluffy round blue blob with big round eyes, pink cheeks,
// and a happy smile. Very round, very bouncy.
// =============================================================
static const uint16_t FLUF_SPRITE[FLUF_SPRITE_W * FLUF_SPRITE_H] = {
    _K,  _K,  _FL, _FM, _FM, _FM, _FM, _FM, _FM, _FL, _K,  _K,  _K,  _K,  // r0 top
    _K,  _FL, _FM, _FM, _FM, _FM, _FM, _FM, _FM, _FM, _FL, _K,  _K,  _K,  // r1
    _FL, _FM, _FM, _EW, _EW, _FM, _FM, _FM, _EW, _EW, _FM, _FM, _FL, _K,  // r2 eyes outer
    _FL, _FM, _FM, _EW, _EP, _EW, _FM, _EW, _EP, _EW, _FM, _FM, _FL, _K,  // r3 pupils
    _FL, _FM, _FM, _EW, _EW, _FM, _FM, _FM, _EW, _EW, _FM, _FM, _FL, _K,  // r4 eyes outer
    _FL, _FM, _FM, _FM, _FM, _PK, _FM, _PK, _FM, _FM, _FM, _FM, _FL, _K,  // r5 pink cheeks
    _FL, _FM, _FM, _SM, _FM, _FM, _FM, _FM, _FM, _SM, _FM, _FM, _FL, _K,  // r6 smile corners
    _FL, _FM, _FM, _FM, _FM, _FM, _FM, _FM, _FM, _FM, _FM, _FM, _FL, _K,  // r7 body
    _K,  _FL, _FM, _FM, _FM, _FM, _FM, _FM, _FM, _FM, _FL, _K,  _K,  _K,  // r8 bottom
    _K,  _K,  _FL, _FL, _FM, _FM, _FM, _FM, _FL, _FL, _K,  _K,  _K,  _K,  // r9 tiny nubs
};

// =============================================================
// TWIGLET SPRITE  8 x 22
// A tall thin green twig creature with glowing yellow eyes,
// leaf arms, and forked branch feet. Rare and mysterious.
// =============================================================
static const uint16_t TWIG_SPRITE[TWIG_SPRITE_W * TWIG_SPRITE_H] = {
    _K,  _K,  _TL, _K,  _TL, _K,  _K,  _K,  // r0  leaf tips
    _K,  _TL, _TG, _TL, _TG, _TL, _K,  _K,  // r1  leaves
    _K,  _K,  _TL, _K,  _TL, _K,  _K,  _K,  // r2  leaf lower
    _K,  _K,  _TG, _TG, _K,  _K,  _K,  _K,  // r3  head top narrow
    _K,  _TG, _TG, _TG, _TG, _K,  _K,  _K,  // r4  head upper
    _TG, _TY, _TY, _TG, _TY, _TY, _TG, _K,  // r5  glowing eyes
    _TG, _TY, _TP, _TG, _TP, _TY, _TG, _K,  // r6  eye pupils
    _TG, _TY, _TY, _TG, _TY, _TY, _TG, _K,  // r7  eyes lower
    _K,  _TG, _TG, _TG, _TG, _K,  _K,  _K,  // r8  lower face
    _K,  _K,  _TB, _TB, _K,  _K,  _K,  _K,  // r9  neck
    _K,  _K,  _TB, _TB, _K,  _K,  _K,  _K,  // r10 trunk
    _K,  _K,  _TB, _TB, _K,  _K,  _K,  _K,  // r11 trunk
    _K,  _TL, _TB, _TB, _TL, _K,  _K,  _K,  // r12 leaf arms narrow
    _TL, _TL, _TB, _TB, _TL, _TL, _K,  _K,  // r13 leaf arms wide
    _K,  _TL, _TB, _TB, _TL, _K,  _K,  _K,  // r14 leaf arms narrow
    _K,  _K,  _TB, _TB, _K,  _K,  _K,  _K,  // r15 trunk lower
    _K,  _K,  _TB, _TB, _K,  _K,  _K,  _K,  // r16
    _K,  _K,  _TB, _TB, _K,  _K,  _K,  _K,  // r17
    _K,  _K,  _TB, _TB, _K,  _K,  _K,  _K,  // r18
    _K,  _TB, _TB, _K,  _TB, _TB, _K,  _K,  // r19 forked legs
    _K,  _TB, _TB, _K,  _TB, _TB, _K,  _K,  // r20
    _TB, _TB, _TB, _K,  _TB, _TB, _TB, _K,  // r21 wide root feet
};

// =============================================================
// MOSS MOUSE SPRITE  14 x 11
// A chubby little brown mouse with big round ears, large
// curious eyes, and patches of bright green moss growing all
// over its body. Legendary and magical.
// =============================================================
static const uint16_t MMOU_SPRITE[MMOU_SPRITE_W * MMOU_SPRITE_H] = {
    _K,  _MB, _MB, _K,  _K,  _K,  _K,  _K,  _K,  _K,  _MB, _MB, _K,  _K,  // r0  ears
    _MB, _MB, _ME, _MB, _K,  _K,  _K,  _K,  _K,  _MB, _ME, _MB, _MB, _K,  // r1  ear inner
    _K,  _MB, _MB, _K,  _K,  _K,  _K,  _K,  _K,  _K,  _MB, _MB, _K,  _K,  // r2  ear lower
    _K,  _MB, _MB, _MB, _MB, _MB, _MB, _MB, _MB, _MB, _MB, _MB, _K,  _K,  // r3  body top
    _K,  _MB, _MW, _MW, _MB, _MO, _MB, _MB, _MO, _MB, _MW, _MW, _MB, _K,  // r4  eyes + moss
    _K,  _MB, _MW, _MP, _MB, _MO, _MO, _MO, _MO, _MB, _MP, _MW, _MB, _K,  // r5  pupils + moss
    _K,  _MB, _MB, _MB, _MO, _MO, _MO, _MO, _MO, _MO, _MB, _MB, _MB, _K,  // r6  big moss patch
    _K,  _MB, _MB, _MO, _MO, _MB, _MN, _MB, _MO, _MO, _MB, _MB, _MB, _K,  // r7  snout/nose
    _K,  _K,  _MB, _MB, _MB, _MB, _MB, _MB, _MB, _MB, _MB, _MB, _K,  _K,  // r8  bottom body
    _K,  _K,  _K,  _MB, _MB, _MB, _MB, _MB, _MB, _MB, _MB, _K,  _K,  _K,  // r9  little belly
    _K,  _K,  _K,  _MB, _K,  _K,  _MB, _MB, _K,  _K,  _MB, _K,  _K,  _K,  // r10 tiny paws
};

// Clean up local palette macros to avoid leaking into other files
#undef _K
#undef _HAT
#undef _SKN
#undef _EYE
#undef _SHT
#undef _CAM
#undef _CDK
#undef _PNT
#undef _SHO
#undef _FM
#undef _FL
#undef _EW
#undef _EP
#undef _PK
#undef _SM
#undef _TL
#undef _TG
#undef _TB
#undef _TY
#undef _TP
#undef _MB
#undef _ME
#undef _MO
#undef _MN
#undef _MW
#undef _MP
