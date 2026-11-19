// layout.h — the fixed dimensions of the building, shared by modelling, physics and the flight path.
// Units are metres. x runs across the lanes, y is up, z runs from the foul line (0) toward the pins.
#pragma once
#include "base.h"

static const int NLANES = 10;
static const int HERO = 5;                 // the lane the ball is bowled on
static const float PITCH = 1.73f;          // lane centre to lane centre
static const float LANE_W = 1.0668f;       // 42 in
static const float BOARD_W = LANE_W / 39;
static const float GUT_W = 0.235f;         // 9 1/4 in
static const float DIV_W = PITCH - LANE_W - 2 * GUT_W;
static const float PINZ = 18.288f;         // head pin, 60 ft from the foul line
static const float PIN_ROW = 0.26396f;     // row spacing (12 in triangle)
static const float PIN_SP = 0.3048f;
static const float DECK_END = 19.17f;      // back edge of the pin deck
static const float PIT_Y = -0.10f;         // pit floor
static const float PIT_END = 20.02f;       // pit back board
static const float KICK_Z0 = 17.42f;       // kickbacks / flat gutters start
static const float MASK_Z = 17.62f;        // front face of the masking units
static const float MASK_Y = 0.66f;         // bottom edge of the masking units
static const float BACK_Z = 24.4f;         // rear wall of the building
static const float APP_Z = -4.88f;         // back edge of the approach
static const float STEP_Z = -8.3f;         // step up to the concourse
static const float FL = 0.16f;             // concourse / lobby floor height
static const float FRONT_Z = -25.0f;       // inside face of the front wall
static const float HALL_X = 9.45f;         // half width of the building interior
static const float CEIL_Y = 3.05f;         // ceiling over the lanes
static const float LOBBY_CEIL = 3.25f;
static const float PIN_H = 0.381f;
static const float BALL_R = 0.1085f;

static inline float laneX(int i) { return (i - (NLANES - 1) * 0.5f) * PITCH; }
// pin k = 0..9 (pin number k+1) position relative to lane centre
static inline V3 pinSpot(int k) {
    static const int row[10] = {0, 1, 1, 2, 2, 2, 3, 3, 3, 3};
    static const int col[10] = {0, 0, 1, 0, 1, 2, 0, 1, 2, 3};
    return V3((col[k] - row[k] * 0.5f) * PIN_SP, 0, PINZ + row[k] * PIN_ROW);
}
