#pragma once
#include <stdint.h>

// Transparent key: magenta (not used in any sprite)
#define SHOOT_SPR_KEY 0xF81Fu

// Sprite native dimensions (drawn at 2x scale on screen)
#define ROBOT_SPR_W   16u
#define ROBOT_SPR_H   16u
#define EBOT_SPR_W    12u
#define EBOT_SPR_H    12u
#define DRONE_SPR_W   12u
#define DRONE_SPR_H   12u
#define COIN_SPR_W     8u
#define COIN_SPR_H     8u
#define HEART_SPR_W    8u
#define HEART_SPR_H    8u
#define ROCK_SPR_W    12u
#define ROCK_SPR_H    10u
#define CRATE_SPR_W   12u
#define CRATE_SPR_H   12u

// ---------------------------------------------------------------
// Local palette macros
// ---------------------------------------------------------------
#define _K   0xF81Fu  // transparent

// --- Robot player ---
#define _RB  0x235Fu  // steel blue body
#define _RL  0x4E9Fu  // lighter blue
#define _RY  0xFFE0u  // yellow chest / highlight
#define _RW  0xFFFFu  // white eyes
#define _RD  0x0000u  // black pupils
#define _RG  0x8430u  // gray joint
#define _RF  0x0010u  // dark blue feet/wheels

// --- Ground bot enemy ---
#define _EB  0x600Fu  // dark purple body
#define _EL  0xA01Fu  // lighter purple
#define _EE  0xF800u  // red eye glow
#define _EW  0xFFFFu  // white wheel
#define _EC  0x8410u  // gray chest panel
#define _ED  0x2104u  // dark wheel hub
#define _EO  0xFD20u  // orange indicator

// --- Flying drone ---
#define _DB  0x4208u  // dark gray body
#define _DL  0x8410u  // light gray
#define _DG  0x07E0u  // green propeller
#define _DE  0xF800u  // red sensor
#define _DY  0xFFE0u  // yellow thrust

// --- Coin ---
#define _CG  0xFEA0u  // gold
#define _CO  0xD360u  // dark gold / shadow
#define _CW  0xFFFFu  // white shine

// --- Heart ---
#define _HR  0xF800u  // red
#define _HD  0xA000u  // dark red shadow

// --- Rock ---
#define _RK  0x8430u  // medium gray
#define _RH  0xC638u  // highlight
#define _RS  0x4208u  // shadow

// --- Crate ---
#define _CB  0xA285u  // wood brown
#define _CL  0xD4A7u  // light wood
#define _CS  0x6180u  // dark wood shadow
#define _CX  0x8163u  // cross stripe

// =============================================================
// ROBOT PLAYER  16 x 16  — 2 run frames + 1 jump frame
// The player is a small blue robot with a yellow chest panel,
// white square eyes, and a blaster arm extending to the right.
// =============================================================
// Frame 0: run (left leg forward)
static const uint16_t ROBOT_RUN0[ROBOT_SPR_W * ROBOT_SPR_H] = {
    _K,  _K,  _K,  _K,  _K,  _K,  _K,  _RY, _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  // r0  antenna tip
    _K,  _K,  _K,  _RB, _RB, _RB, _RB, _RY, _RB, _RB, _RB, _K,  _K,  _K,  _K,  _K,  // r1  head top
    _K,  _K,  _RB, _RB, _RW, _RB, _RB, _RB, _RB, _RW, _RB, _RB, _K,  _K,  _K,  _K,  // r2  eye outer
    _K,  _K,  _RB, _RB, _RD, _RW, _RB, _RB, _RW, _RD, _RB, _RB, _K,  _K,  _K,  _K,  // r3  pupils
    _K,  _K,  _RB, _RB, _RW, _RB, _RB, _RB, _RB, _RW, _RB, _RB, _K,  _K,  _K,  _K,  // r4  eye lower
    _K,  _K,  _K,  _RB, _RB, _RB, _RB, _RB, _RB, _RB, _RB, _K,  _K,  _K,  _K,  _K,  // r5  jaw
    _K,  _RG, _RB, _RB, _RB, _RY, _RY, _RY, _RB, _RB, _RB, _RB, _RY, _RY, _K,  _K,  // r6  body + blaster arm
    _K,  _RG, _RB, _RB, _RB, _RY, _RY, _RY, _RB, _RB, _RB, _RB, _RY, _RY, _K,  _K,  // r7  body
    _K,  _K,  _RB, _RB, _RB, _RB, _RB, _RB, _RB, _RB, _RB, _RB, _K,  _K,  _K,  _K,  // r8  body lower
    _K,  _K,  _RG, _RB, _RB, _RB, _RB, _RB, _RB, _RB, _RG, _K,  _K,  _K,  _K,  _K,  // r9  hip joint
    _K,  _K,  _RB, _RF, _K,  _K,  _K,  _K,  _RB, _RF, _K,  _K,  _K,  _K,  _K,  _K,  // r10 upper legs
    _K,  _K,  _RB, _RF, _K,  _K,  _K,  _K,  _K,  _RF, _K,  _K,  _K,  _K,  _K,  _K,  // r11 legs mid (L fwd)
    _K,  _K,  _RF, _RF, _K,  _K,  _K,  _K,  _K,  _RF, _K,  _K,  _K,  _K,  _K,  _K,  // r12 lower left leg
    _K,  _RF, _RF, _RF, _RF, _K,  _K,  _K,  _K,  _RF, _K,  _K,  _K,  _K,  _K,  _K,  // r13 left foot forward
    _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _RF, _RF, _K,  _K,  _K,  _K,  _K,  _K,  // r14 right foot
    _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  // r15
};

// Frame 1: run (right leg forward)
static const uint16_t ROBOT_RUN1[ROBOT_SPR_W * ROBOT_SPR_H] = {
    _K,  _K,  _K,  _K,  _K,  _K,  _K,  _RY, _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  // r0
    _K,  _K,  _K,  _RB, _RB, _RB, _RB, _RY, _RB, _RB, _RB, _K,  _K,  _K,  _K,  _K,  // r1
    _K,  _K,  _RB, _RB, _RW, _RB, _RB, _RB, _RB, _RW, _RB, _RB, _K,  _K,  _K,  _K,  // r2
    _K,  _K,  _RB, _RB, _RD, _RW, _RB, _RB, _RW, _RD, _RB, _RB, _K,  _K,  _K,  _K,  // r3
    _K,  _K,  _RB, _RB, _RW, _RB, _RB, _RB, _RB, _RW, _RB, _RB, _K,  _K,  _K,  _K,  // r4
    _K,  _K,  _K,  _RB, _RB, _RB, _RB, _RB, _RB, _RB, _RB, _K,  _K,  _K,  _K,  _K,  // r5
    _K,  _RG, _RB, _RB, _RB, _RY, _RY, _RY, _RB, _RB, _RB, _RB, _RY, _RY, _K,  _K,  // r6
    _K,  _RG, _RB, _RB, _RB, _RY, _RY, _RY, _RB, _RB, _RB, _RB, _RY, _RY, _K,  _K,  // r7
    _K,  _K,  _RB, _RB, _RB, _RB, _RB, _RB, _RB, _RB, _RB, _RB, _K,  _K,  _K,  _K,  // r8
    _K,  _K,  _RG, _RB, _RB, _RB, _RB, _RB, _RB, _RB, _RG, _K,  _K,  _K,  _K,  _K,  // r9
    _K,  _K,  _RF, _RB, _K,  _K,  _K,  _K,  _RB, _RF, _K,  _K,  _K,  _K,  _K,  _K,  // r10
    _K,  _K,  _RF, _K,  _K,  _K,  _K,  _K,  _RB, _RF, _K,  _K,  _K,  _K,  _K,  _K,  // r11 (R fwd)
    _K,  _K,  _RF, _K,  _K,  _K,  _K,  _K,  _RF, _RF, _K,  _K,  _K,  _K,  _K,  _K,  // r12
    _K,  _K,  _RF, _K,  _K,  _K,  _K,  _RF, _RF, _RF, _RF, _K,  _K,  _K,  _K,  _K,  // r13 right foot fwd
    _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  // r14
    _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  // r15
};

// Frame 2: jump (legs tucked)
static const uint16_t ROBOT_JUMP[ROBOT_SPR_W * ROBOT_SPR_H] = {
    _K,  _K,  _K,  _K,  _K,  _K,  _K,  _RY, _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  // r0
    _K,  _K,  _K,  _RB, _RB, _RB, _RB, _RY, _RB, _RB, _RB, _K,  _K,  _K,  _K,  _K,  // r1
    _K,  _K,  _RB, _RB, _RW, _RB, _RB, _RB, _RB, _RW, _RB, _RB, _K,  _K,  _K,  _K,  // r2
    _K,  _K,  _RB, _RB, _RD, _RW, _RB, _RB, _RW, _RD, _RB, _RB, _K,  _K,  _K,  _K,  // r3
    _K,  _K,  _RB, _RB, _RW, _RB, _RB, _RB, _RB, _RW, _RB, _RB, _K,  _K,  _K,  _K,  // r4
    _K,  _K,  _K,  _RB, _RB, _RB, _RB, _RB, _RB, _RB, _RB, _K,  _K,  _K,  _K,  _K,  // r5
    _K,  _RG, _RB, _RB, _RB, _RY, _RY, _RY, _RB, _RB, _RB, _RB, _RY, _RY, _K,  _K,  // r6
    _K,  _RG, _RB, _RB, _RB, _RY, _RY, _RY, _RB, _RB, _RB, _RB, _RY, _RY, _K,  _K,  // r7
    _K,  _K,  _RB, _RB, _RB, _RB, _RB, _RB, _RB, _RB, _RB, _RB, _K,  _K,  _K,  _K,  // r8
    _K,  _K,  _RG, _RB, _RB, _RB, _RB, _RB, _RB, _RB, _RG, _K,  _K,  _K,  _K,  _K,  // r9 hip
    _K,  _RF, _RF, _RB, _K,  _K,  _K,  _K,  _RB, _RF, _RF, _K,  _K,  _K,  _K,  _K,  // r10 tucked legs
    _RF, _RF, _RF, _RF, _K,  _K,  _K,  _K,  _RF, _RF, _RF, _RF, _K,  _K,  _K,  _K,  // r11 feet out
    _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  // r12
    _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  // r13
    _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  // r14
    _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  // r15
};

// =============================================================
// GROUND BOT ENEMY  12 x 12  — 2 frames (wheel spin)
// A mean purple robot with red eyes and rolling wheels.
// =============================================================
// Frame 0
static const uint16_t EBOT_F0[EBOT_SPR_W * EBOT_SPR_H] = {
    _K,  _K,  _EB, _EB, _EB, _EB, _EB, _EB, _K,  _K,  _K,  _K,  // r0 head
    _K,  _EB, _EB, _EE, _EE, _EB, _EB, _EE, _EE, _EB, _K,  _K,  // r1 eyes
    _K,  _EB, _EL, _EE, _EE, _EB, _EB, _EE, _EE, _EL, _K,  _K,  // r2 eye shine
    _K,  _K,  _EB, _EB, _EB, _EB, _EB, _EB, _EB, _K,  _K,  _K,  // r3 jaw
    _K,  _EC, _EB, _EB, _EO, _EO, _EO, _EB, _EB, _EC, _K,  _K,  // r4 body top
    _K,  _EC, _EB, _EB, _EO, _EB, _EO, _EB, _EB, _EC, _K,  _K,  // r5 chest
    _K,  _K,  _EB, _EB, _EB, _EB, _EB, _EB, _EB, _K,  _K,  _K,  // r6 body lower
    _K,  _K,  _EW, _EB, _K,  _K,  _K,  _EB, _EW, _K,  _K,  _K,  // r7 axle
    _K,  _EW, _EW, _EW, _K,  _K,  _K,  _EW, _EW, _EW, _K,  _K,  // r8 wheels
    _K,  _EW, _ED, _EW, _K,  _K,  _K,  _EW, _ED, _EW, _K,  _K,  // r9 hub
    _K,  _EW, _EW, _EW, _K,  _K,  _K,  _EW, _EW, _EW, _K,  _K,  // r10
    _K,  _K,  _EW, _K,  _K,  _K,  _K,  _K,  _EW, _K,  _K,  _K,  // r11
};

// Frame 1 (wheels rotated: hub shifted)
static const uint16_t EBOT_F1[EBOT_SPR_W * EBOT_SPR_H] = {
    _K,  _K,  _EB, _EB, _EB, _EB, _EB, _EB, _K,  _K,  _K,  _K,  // r0
    _K,  _EB, _EB, _EE, _EE, _EB, _EB, _EE, _EE, _EB, _K,  _K,  // r1
    _K,  _EB, _EL, _EE, _EE, _EB, _EB, _EE, _EE, _EL, _K,  _K,  // r2
    _K,  _K,  _EB, _EB, _EB, _EB, _EB, _EB, _EB, _K,  _K,  _K,  // r3
    _K,  _EC, _EB, _EB, _EO, _EO, _EO, _EB, _EB, _EC, _K,  _K,  // r4
    _K,  _EC, _EB, _EB, _EO, _EB, _EO, _EB, _EB, _EC, _K,  _K,  // r5
    _K,  _K,  _EB, _EB, _EB, _EB, _EB, _EB, _EB, _K,  _K,  _K,  // r6
    _K,  _K,  _EW, _EB, _K,  _K,  _K,  _EB, _EW, _K,  _K,  _K,  // r7
    _K,  _EW, _EW, _EW, _K,  _K,  _K,  _EW, _EW, _EW, _K,  _K,  // r8
    _K,  _ED, _EW, _EW, _K,  _K,  _K,  _EW, _EW, _ED, _K,  _K,  // r9 hub shifted
    _K,  _EW, _EW, _EW, _K,  _K,  _K,  _EW, _EW, _EW, _K,  _K,  // r10
    _K,  _K,  _EW, _K,  _K,  _K,  _K,  _K,  _EW, _K,  _K,  _K,  // r11
};

// =============================================================
// FLYING DRONE ENEMY  12 x 12  — 2 frames (propeller spin)
// A mean gray drone with spinning green propellers and a red eye.
// =============================================================
// Frame 0
static const uint16_t DRONE_F0[DRONE_SPR_W * DRONE_SPR_H] = {
    _DG, _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _DG, _K,  _K,  // r0 propeller tip L
    _DG, _DG, _K,  _K,  _K,  _K,  _K,  _K,  _DG, _DG, _K,  _K,  // r1 propeller
    _K,  _DG, _DB, _DB, _DB, _DB, _DB, _DB, _DG, _K,  _K,  _K,  // r2 body top + hub
    _K,  _K,  _DB, _DE, _DB, _DB, _DB, _DB, _DB, _K,  _K,  _K,  // r3 red eye
    _K,  _K,  _DB, _DB, _DL, _DL, _DL, _DL, _DB, _K,  _K,  _K,  // r4 fuselage
    _K,  _K,  _DB, _DB, _DB, _DY, _DB, _DB, _DB, _K,  _K,  _K,  // r5 thruster
    _K,  _K,  _K,  _DB, _DY, _DY, _DY, _DB, _K,  _K,  _K,  _K,  // r6 nozzle
    _K,  _K,  _K,  _K,  _DY, _DY, _DY, _K,  _K,  _K,  _K,  _K,  // r7 thrust flame
    _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  // r8-11 empty
    _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,
    _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,
    _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,
};

// Frame 1 (propellers rotated)
static const uint16_t DRONE_F1[DRONE_SPR_W * DRONE_SPR_H] = {
    _K,  _DG, _K,  _K,  _K,  _K,  _K,  _K,  _DG, _K,  _K,  _K,  // r0
    _K,  _DG, _DG, _K,  _K,  _K,  _K,  _DG, _DG, _K,  _K,  _K,  // r1
    _K,  _K,  _DB, _DB, _DB, _DB, _DB, _DB, _DB, _K,  _K,  _K,  // r2
    _K,  _K,  _DB, _DE, _DB, _DB, _DB, _DB, _DB, _K,  _K,  _K,  // r3
    _K,  _K,  _DB, _DB, _DL, _DL, _DL, _DL, _DB, _K,  _K,  _K,  // r4
    _K,  _K,  _DB, _DB, _DB, _DY, _DB, _DB, _DB, _K,  _K,  _K,  // r5
    _K,  _K,  _K,  _DB, _DY, _DY, _DY, _DB, _K,  _K,  _K,  _K,  // r6
    _K,  _K,  _K,  _K,  _DY, _DY, _DY, _K,  _K,  _K,  _K,  _K,  // r7
    _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,
    _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,
    _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,
    _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,
};

// =============================================================
// COIN  8 x 8  — 4 rotation frames
// A spinning gold coin: full → narrow → edge → narrow → full
// =============================================================
// Frame 0: full face
static const uint16_t COIN_F0[COIN_SPR_W * COIN_SPR_H] = {
    _K,  _CG, _CG, _CG, _CG, _CG, _K,  _K,  // r0
    _CG, _CG, _CO, _CO, _CO, _CG, _CG, _K,  // r1
    _CG, _CO, _CG, _CW, _CG, _CO, _CG, _K,  // r2 shine
    _CG, _CO, _CG, _CG, _CG, _CO, _CG, _K,  // r3
    _CG, _CO, _CG, _CW, _CG, _CO, _CG, _K,  // r4
    _CG, _CG, _CO, _CO, _CO, _CG, _CG, _K,  // r5
    _K,  _CG, _CG, _CG, _CG, _CG, _K,  _K,  // r6
    _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  // r7
};

// Frame 1: slightly narrow
static const uint16_t COIN_F1[COIN_SPR_W * COIN_SPR_H] = {
    _K,  _K,  _CG, _CG, _CG, _K,  _K,  _K,  // r0
    _K,  _CG, _CO, _CG, _CO, _CG, _K,  _K,  // r1
    _K,  _CG, _CG, _CW, _CG, _CG, _K,  _K,  // r2
    _K,  _CG, _CO, _CG, _CO, _CG, _K,  _K,  // r3
    _K,  _CG, _CG, _CW, _CG, _CG, _K,  _K,  // r4
    _K,  _CG, _CO, _CG, _CO, _CG, _K,  _K,  // r5
    _K,  _K,  _CG, _CG, _CG, _K,  _K,  _K,  // r6
    _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  // r7
};

// Frame 2: thin edge
static const uint16_t COIN_F2[COIN_SPR_W * COIN_SPR_H] = {
    _K,  _K,  _K,  _CG, _K,  _K,  _K,  _K,  // r0
    _K,  _K,  _K,  _CO, _K,  _K,  _K,  _K,  // r1
    _K,  _K,  _K,  _CW, _K,  _K,  _K,  _K,  // r2
    _K,  _K,  _K,  _CO, _K,  _K,  _K,  _K,  // r3
    _K,  _K,  _K,  _CW, _K,  _K,  _K,  _K,  // r4
    _K,  _K,  _K,  _CO, _K,  _K,  _K,  _K,  // r5
    _K,  _K,  _K,  _CG, _K,  _K,  _K,  _K,  // r6
    _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  // r7
};

// Frame 3: narrow again (same as F1 reused)
static const uint16_t COIN_F3[COIN_SPR_W * COIN_SPR_H] = {
    _K,  _K,  _CG, _CG, _CG, _K,  _K,  _K,
    _K,  _CG, _CO, _CG, _CO, _CG, _K,  _K,
    _K,  _CG, _CG, _CW, _CG, _CG, _K,  _K,
    _K,  _CG, _CO, _CG, _CO, _CG, _K,  _K,
    _K,  _CG, _CG, _CW, _CG, _CG, _K,  _K,
    _K,  _CG, _CO, _CG, _CO, _CG, _K,  _K,
    _K,  _K,  _CG, _CG, _CG, _K,  _K,  _K,
    _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,
};

// =============================================================
// HEART  8 x 8  (HUD lives)
// =============================================================
static const uint16_t HEART_SPR[HEART_SPR_W * HEART_SPR_H] = {
    _K,  _HR, _HR, _K,  _K,  _HR, _HR, _K,  // r0
    _HR, _HR, _HR, _HR, _HR, _HR, _HR, _K,  // r1
    _HR, _HR, _HR, _HR, _HR, _HR, _HR, _K,  // r2
    _HR, _HR, _HR, _HR, _HR, _HR, _HR, _K,  // r3
    _K,  _HR, _HR, _HR, _HR, _HR, _K,  _K,  // r4
    _K,  _K,  _HR, _HR, _HR, _K,  _K,  _K,  // r5
    _K,  _K,  _K,  _HR, _K,  _K,  _K,  _K,  // r6
    _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  // r7
};

// =============================================================
// ROCK OBSTACLE  12 x 10  (unbreakable)
// A rough gray boulder that must be jumped over.
// =============================================================
static const uint16_t ROCK_SPR[ROCK_SPR_W * ROCK_SPR_H] = {
    _K,  _K,  _K,  _RH, _RK, _RK, _RK, _RK, _K,  _K,  _K,  _K,  // r0 top
    _K,  _K,  _RH, _RK, _RK, _RK, _RK, _RK, _RK, _K,  _K,  _K,  // r1
    _K,  _RK, _RK, _RK, _RS, _RK, _RK, _RK, _RK, _RK, _K,  _K,  // r2 crack
    _K,  _RK, _RK, _RS, _RK, _RK, _RK, _RS, _RK, _RK, _K,  _K,  // r3
    _RK, _RK, _RK, _RK, _RK, _RK, _RK, _RK, _RK, _RK, _RK, _K,  // r4 wide base
    _RK, _RK, _RS, _RK, _RK, _RK, _RK, _RS, _RK, _RK, _RK, _K,  // r5
    _RK, _RK, _RK, _RK, _RK, _RK, _RK, _RK, _RK, _RK, _RK, _K,  // r6
    _RS, _RK, _RK, _RK, _RK, _RK, _RK, _RK, _RK, _RS, _K,  _K,  // r7 shadow
    _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  // r8
    _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  _K,  // r9
};

// =============================================================
// CRATE OBSTACLE  12 x 12  (breakable, 1 hit)
// A wooden crate with cross-nail pattern.
// =============================================================
static const uint16_t CRATE_SPR[CRATE_SPR_W * CRATE_SPR_H] = {
    _CB, _CB, _CB, _CB, _CB, _CB, _CB, _CB, _CB, _CB, _CB, _CB,  // r0 top edge
    _CB, _CL, _CL, _CX, _CL, _CB, _CB, _CL, _CX, _CL, _CL, _CB,  // r1 cross top
    _CB, _CL, _CX, _CL, _CL, _CB, _CB, _CL, _CL, _CX, _CL, _CB,  // r2
    _CB, _CX, _CL, _CL, _CL, _CB, _CB, _CL, _CL, _CL, _CX, _CB,  // r3
    _CB, _CL, _CL, _CL, _CL, _CB, _CB, _CL, _CL, _CL, _CL, _CB,  // r4
    _CB, _CB, _CB, _CB, _CB, _CB, _CB, _CB, _CB, _CB, _CB, _CB,  // r5 center bar
    _CB, _CB, _CB, _CB, _CB, _CB, _CB, _CB, _CB, _CB, _CB, _CB,  // r6
    _CB, _CL, _CL, _CX, _CL, _CB, _CB, _CL, _CX, _CL, _CL, _CB,  // r7
    _CB, _CL, _CX, _CL, _CL, _CB, _CB, _CL, _CL, _CX, _CL, _CB,  // r8
    _CB, _CX, _CL, _CL, _CL, _CB, _CB, _CL, _CL, _CL, _CX, _CB,  // r9
    _CB, _CL, _CL, _CL, _CL, _CB, _CB, _CL, _CL, _CL, _CL, _CB,  // r10
    _CS, _CS, _CS, _CS, _CS, _CS, _CS, _CS, _CS, _CS, _CS, _CS,  // r11 bottom shadow
};

// Clean up local palette macros
#undef _K
#undef _RB
#undef _RL
#undef _RY
#undef _RW
#undef _RD
#undef _RG
#undef _RF
#undef _EB
#undef _EL
#undef _EE
#undef _EW
#undef _EC
#undef _ED
#undef _EO
#undef _DB
#undef _DL
#undef _DG
#undef _DE
#undef _DY
#undef _CG
#undef _CO
#undef _CW
#undef _HR
#undef _HD
#undef _RK
#undef _RH
#undef _RS
#undef _CB
#undef _CL
#undef _CS
#undef _CX
