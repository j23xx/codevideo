// world3.h — the lobby (snack bar, control desk, jukebox, ball racks, sputnik lights) and the exterior
// (facade, neon roof sign, canopy, parking lot).
#pragma once
#include "world2.h"
#include "world4.h"

static const float DOOR_X0 = -4.4f, DOOR_X1 = -2.6f, DOOR_H = 2.2f;
static const float SNACK_X = -7.2f;       // snack counter centre line
static const float LCEIL = 3.3f;          // lobby ceiling (absolute)
static const V3 JUKE_POS(-3.3f, FL, -12.3f);

static void canLight(V3 c, float r, V3 e) { // recessed downlight in the ceiling at c
    rectLight(c + V3(0, -0.012f, 0), V3(r, 0, 0), V3(0, 0, r), e);
    PushMat pm(MT.chromeTrim);
    torus(c + V3(0, -0.012f, 0), V3(0, 1, 0), r * 1.42f, 0.012f, 20, 6);
}

static void stool() {
    // fixed diner stool: bolted floor flange, trumpet column, ribbed chrome seat band, padded sparkle-vinyl cushion
    {
        PushMat pm(MT.chrome);
        lathe(smoothProfile({V2(0, 0.012f), V2(0.150f, 0.012f), V2(0.152f, 0.020f), V2(0.135f, 0.030f), V2(0.080f, 0.050f), V2(0.050f, 0.095f), V2(0.038f, 0.19f), V2(0.034f, 0.40f),
                             V2(0.034f, 0.585f)}, 3), 28);
        lathe({V2(0.0f, 0.0f), V2(0.158f, 0.0f), V2(0.158f, 0.012f), V2(0.0f, 0.012f)}, 28);
        for (int i = 0; i < 4; i++) { float a = i * PI / 2 + 0.6f; cyl(V3(0.125f * std::cos(a), 0.012f, 0.125f * std::sin(a)), V3(0.125f * std::cos(a), 0.024f, 0.125f * std::sin(a)), 0.011f, 0.009f, 6); }
        lathe({V2(0.034f, 0.585f), V2(0.045f, 0.60f), V2(0.060f, 0.612f), V2(0.150f, 0.632f), V2(0.172f, 0.645f)}, 28);
        lathe({V2(0.172f, 0.645f), V2(0.191f, 0.648f), V2(0.193f, 0.659f), V2(0.188f, 0.662f), V2(0.193f, 0.665f), V2(0.193f, 0.676f), V2(0.188f, 0.679f), V2(0.193f, 0.682f),
               V2(0.193f, 0.693f), V2(0.187f, 0.700f)}, 32);
    }
    { PushMat pm(MT.flake);
      lathe(smoothProfile({V2(0.186f, 0.698f), V2(0.197f, 0.712f), V2(0.198f, 0.742f), V2(0.184f, 0.770f), V2(0.13f, 0.788f), V2(0.06f, 0.795f), V2(0.0f, 0.796f)}, 4), 32); }
    { PushMat pm(mat(rgb(0xE8E0CC), 0.4f)); torus(V3(0, 0.748f, 0), V3(0, 1, 0), 0.1975f, 0.0055f, 32, 6); lathe(smoothProfile({V2(0.017f, 0.794f), V2(0.012f, 0.801f), V2(0, 0.803f)}, 2), 10); }
}

// ---- things that stand on the counter and the back bar
static void sugarPourer() {
    { PushMat pm(MT.glass); G.flags |= F_NOSHADOW; lathe({V2(0.0f, 0.0f), V2(0.031f, 0.0f), V2(0.033f, 0.012f), V2(0.031f, 0.095f), V2(0.027f, 0.102f)}, 14); }
    { PushMat pm(MT.white); lathe({V2(0.0f, 0.003f), V2(0.029f, 0.003f), V2(0.029f, 0.066f), V2(0.0f, 0.070f)}, 12); }
    { PushMat pm(MT.chrome); lathe(smoothProfile({V2(0.029f, 0.100f), V2(0.031f, 0.108f), V2(0.018f, 0.128f), V2(0.006f, 0.140f), V2(0, 0.141f)}, 2), 14); }
}
static void shakers() {
    for (int k = 0; k < 2; k++) {
        Push p(xfT(k * 0.045f, 0, k * 0.012f));
        { PushMat pm(k ? MT.steelDark : MT.white); lathe({V2(0, 0.002f), V2(0.015f, 0.002f), V2(0.013f, 0.045f), V2(0, 0.046f)}, 10); }
        { PushMat pm(MT.glass); G.flags |= F_NOSHADOW; lathe({V2(0.017f, 0), V2(0.018f, 0.01f), V2(0.015f, 0.058f)}, 10); }
        { PushMat pm(MT.chrome); lathe({V2(0.016f, 0.056f), V2(0.016f, 0.066f), V2(0.010f, 0.074f), V2(0, 0.075f)}, 10); }
    }
}
static void ashtray() {
    PushMat pm(mat(V3(0.85f, 0.55f, 0.18f), 0.03f).glass(0.85f));
    lathe({V2(0, 0.004f), V2(0.050f, 0.004f), V2(0.062f, 0.022f), V2(0.058f, 0.024f), V2(0.047f, 0.010f), V2(0, 0.010f)}, 16);
    lathe({V2(0, 0.0f), V2(0.052f, 0.0f), V2(0.050f, 0.004f)}, 16);
}
static void cakeStand(int kind) {
    { PushMat pm(MT.plasticWhite);
      lathe(smoothProfile({V2(0, 0), V2(0.07f, 0), V2(0.06f, 0.012f), V2(0.018f, 0.03f), V2(0.016f, 0.085f), V2(0.05f, 0.10f), V2(0.155f, 0.108f), V2(0.160f, 0.114f), V2(0, 0.114f)}, 2), 28); }
    if (kind == 0) {     // layer cake with a wedge already sold
        { PushMat pm(mat(rgb(0xF2DDE0), 0.45f)); lathe(smoothProfile({V2(0, 0.114f), V2(0.105f, 0.114f), V2(0.108f, 0.125f), V2(0.108f, 0.185f), V2(0.098f, 0.197f), V2(0, 0.199f)}, 2), 30, 0.55f, 2 * PI); }
        { PushMat pm(mat(rgb(0x5B3418), 0.7f)); for (float a : {0.0f, 0.55f}) { V3 d(std::cos(a), 0, std::sin(a)); quad(V3(0, 0.116f, 0), d * 0.104f + V3(0, 0.116f, 0), d * 0.104f + V3(0, 0.196f, 0), V3(0, 0.196f, 0)); } }
        { PushMat pm(mat(rgb(0xA01A22), 0.25f)); for (int i = 0; i < 8; i++) { float a = 0.9f + i * 0.72f; sphere(V3(0.08f * std::cos(a), 0.205f, 0.08f * std::sin(a)), 0.011f, 6); } }
    } else {             // doughnuts
        Rng r(kind * 7);
        for (int i = 0; i < 8; i++) {
            float a = i * 0.9f, rad = i < 5 ? 0.095f : 0.04f, y = i < 5 ? 0.132f : 0.166f;
            V3 c(rad * std::cos(a * (i < 5 ? 1.39f : 2.1f)), y, rad * std::sin(a * (i < 5 ? 1.39f : 2.1f)));
            { PushMat pm(mat(rgb(0xC08A45), 0.6f)); torus(c, norm(V3(r.r(-0.15f, 0.15f), 1, r.r(-0.15f, 0.15f))), 0.028f, 0.018f, 14, 8); }
            { PushMat pm(mat(i % 3 ? rgb(0x4A2412) : rgb(0xF0C8CF), 0.3f)); torus(c + V3(0, 0.008f, 0), V3(0, 1, 0), 0.028f, 0.0155f, 14, 6); }
        }
    }
    { PushMat pm(MT.glass); G.flags |= F_NOSHADOW;
      lathe(smoothProfile({V2(0.150f, 0.114f), V2(0.150f, 0.20f), V2(0.135f, 0.26f), V2(0.08f, 0.30f), V2(0.012f, 0.312f)}, 4), 28);
      sphere(V3(0, 0.33f, 0), 0.022f, 10); }
}
static void pieSlicePlate() {
    { PushMat pm(MT.plasticWhite); lathe({V2(0, 0), V2(0.055f, 0), V2(0.088f, 0.010f), V2(0.092f, 0.014f), V2(0.055f, 0.007f), V2(0, 0.007f)}, 22); }
    { PushMat pm(mat(rgb(0xC98A3A), 0.6f)); Push p(xfT(-0.03f, 0.007f, -0.02f)); extrude({V2(0, 0), V2(0.095f, -0.03f), V2(0.095f, 0.035f)}, 0, 0.03f); }
    { PushMat pm(mat(rgb(0x7A1420), 0.35f)); Push p(xfT(-0.03f, 0.012f, -0.02f)); extrude({V2(0.004f, 0.0f), V2(0.088f, -0.026f), V2(0.088f, 0.031f)}, 0, 0.02f); }
    { PushMat pm(MT.chrome); rbox(V3(0.02f, 0.012f, 0.045f), V3(0.13f, 0.016f, 0.056f), 0.002f); }
}
static void strawJar() {
    { PushMat pm(MT.glass); G.flags |= F_NOSHADOW; lathe({V2(0, 0), V2(0.036f, 0), V2(0.038f, 0.01f), V2(0.038f, 0.21f)}, 14); }
    { PushMat pm(MT.chrome); lathe(smoothProfile({V2(0.040f, 0.205f), V2(0.042f, 0.215f), V2(0.03f, 0.235f), V2(0.008f, 0.245f), V2(0.01f, 0.26f), V2(0, 0.262f)}, 2), 14); lathe({V2(0, 0), V2(0.041f, 0), V2(0.041f, 0.008f)}, 14); }
    Rng r(9);
    for (int i = 0; i < 14; i++) { float a = r.r(0, 6.28f), d = r.r(0, 0.026f); PushMat pm(i % 2 ? MT.red : MT.white); cyl(V3(d * std::cos(a), 0.01f, d * std::sin(a)), V3(d * std::cos(a) * 1.15f, 0.20f, d * std::sin(a) * 1.15f), 0.003f, 0.003f, 5); }
}
static void colaGlass() {
    { PushMat pm(MT.glass); G.flags |= F_NOSHADOW; lathe({V2(0, 0), V2(0.026f, 0), V2(0.028f, 0.008f), V2(0.036f, 0.15f)}, 14); }
    { PushMat pm(mat(rgb(0x2A0E06), 0.08f)); lathe({V2(0, 0.006f), V2(0.025f, 0.006f), V2(0.033f, 0.125f), V2(0, 0.127f)}, 14); }
    { PushMat pm(MT.white); cyl(V3(0.0f, 0.02f, 0.01f), V3(0.03f, 0.21f, 0.02f), 0.003f, 0.003f, 6); }
}
static void coffeeUrn() {
    { PushMat pm(MT.chrome);
      lathe(smoothProfile({V2(0, 0.06f), V2(0.135f, 0.06f), V2(0.142f, 0.09f), V2(0.142f, 0.44f), V2(0.135f, 0.47f), V2(0.11f, 0.50f), V2(0.03f, 0.535f), V2(0.0f, 0.54f)}, 2), 28);
      for (int i = 0; i < 3; i++) { float a = i * 2.094f + 0.5f; cyl(V3(0.11f * std::cos(a), 0, 0.11f * std::sin(a)), V3(0.11f * std::cos(a), 0.065f, 0.11f * std::sin(a)), 0.014f, 0.018f, 8); }
      torus(V3(0, 0.20f, 0), V3(0, 1, 0), 0.143f, 0.006f, 28, 6); torus(V3(0, 0.40f, 0), V3(0, 1, 0), 0.143f, 0.006f, 28, 6);
      cyl(V3(0.14f, 0.12f, 0), V3(0.205f, 0.115f, 0), 0.013f, 0.011f, 8); cyl(V3(0.195f, 0.115f, 0), V3(0.195f, 0.085f, 0), 0.009f, 0.007f, 8);
      cyl(V3(0.150f, 0.15f, 0.03f), V3(0.150f, 0.17f, 0.03f), 0.008f, 0.008f, 6); cyl(V3(0.150f, 0.38f, 0.03f), V3(0.150f, 0.40f, 0.03f), 0.008f, 0.008f, 6); }
    { PushMat pm(MT.glass); G.flags |= F_NOSHADOW; cyl(V3(0.150f, 0.17f, 0.03f), V3(0.150f, 0.38f, 0.03f), 0.006f, 0.006f, 6, false, false); }
    { PushMat pm(mat(rgb(0x2A1608), 0.2f)); cyl(V3(0.150f, 0.17f, 0.03f), V3(0.150f, 0.29f, 0.03f), 0.004f, 0.004f, 5); }
    { PushMat pm(MT.black); sphere(V3(0, 0.555f, 0), 0.024f, 10); rbox(V3(0.185f, 0.125f, -0.008f), V3(0.215f, 0.165f, 0.008f), 0.006f); }
}
static void maltMixer() {
    int green = mat(rgb(0x86C2AC), 0.25f).spec(1.3f);
    { PushMat pm(green);
      rbox(V3(-0.19f, 0, -0.11f), V3(0.19f, 0.045f, 0.10f), 0.02f);
      rbox(V3(-0.17f, 0.045f, -0.11f), V3(0.17f, 0.50f, -0.045f), 0.022f);
      rbox(V3(-0.19f, 0.40f, -0.12f), V3(0.19f, 0.52f, 0.075f), 0.04f); }
    for (int k = -1; k <= 1; k++) {
        { PushMat pm(MT.chrome); cyl(V3(k * 0.115f, 0.40f, 0.025f), V3(k * 0.115f, 0.15f, 0.025f), 0.005f, 0.005f, 6);
          Push p(xfT(k * 0.115f, 0.15f, 0.025f)); lathe({V2(0.0f, 0), V2(0.014f, 0.004f), V2(0.0f, 0.008f)}, 8); }
        if (k != 0) { PushMat pm(MT.steel); Push p(xfT(k * 0.115f, 0.048f, 0.025f)); lathe({V2(0, 0), V2(0.036f, 0), V2(0.046f, 0.17f), V2(0.048f, 0.172f)}, 16); }
        { PushMat pm(MT.black); cyl(V3(k * 0.115f, 0.47f, 0.075f), V3(k * 0.115f, 0.47f, 0.09f), 0.012f, 0.012f, 8); }
    }
}
static void juiceBubbler(int liquidMat) {
    { PushMat pm(MT.chrome); rbox(V3(-0.15f, 0, -0.15f), V3(0.15f, 0.16f, 0.15f), 0.02f); rbox(V3(-0.155f, 0.52f, -0.155f), V3(0.155f, 0.56f, 0.155f), 0.015f);
      cyl(V3(0, 0.07f, 0.15f), V3(0, 0.065f, 0.21f), 0.012f, 0.010f, 8); cyl(V3(0, 0.065f, 0.20f), V3(0, 0.03f, 0.20f), 0.008f, 0.006f, 6);
      rbox(V3(-0.07f, 0.0f, 0.15f), V3(0.07f, 0.012f, 0.25f), 0.004f); }
    { PushMat pm(liquidMat); rbox(V3(-0.128f, 0.165f, -0.128f), V3(0.128f, 0.43f, 0.128f), 0.015f); }
    { PushMat pm(MT.glass); G.flags |= F_NOSHADOW; rbox(V3(-0.14f, 0.16f, -0.14f), V3(0.14f, 0.52f, 0.14f), 0.018f); }
    { PushMat pm(MT.black); rbox(V3(-0.01f, 0.085f, 0.185f), V3(0.01f, 0.12f, 0.20f), 0.004f); }
}
static void cashRegister() {
    int body = mat(rgb(0x5E4A2C), 0.3f).metal(0.7f);
    { PushMat pm(body);
      rbox(V3(-0.23f, 0, -0.21f), V3(0.23f, 0.12f, 0.22f), 0.015f);
      rbox(V3(-0.21f, 0.12f, -0.20f), V3(0.21f, 0.30f, 0.06f), 0.03f);
      rbox(V3(-0.21f, 0.28f, -0.20f), V3(0.21f, 0.47f, -0.07f), 0.03f);
      surf(1, 1, [&](float u, float v) { return V3(lerp(-0.20f, 0.20f, u), lerp(0.125f, 0.30f, v), lerp(0.20f, 0.055f, v)); }); }
    { PushMat pm(MT.plasticWhite);
      for (int i = 0; i < 28; i++) { int cx = i % 7, cy = i / 7; float v = (cy + 0.6f) / 4.4f; V3 c(-0.15f + cx * 0.05f, lerp(0.125f, 0.30f, v), lerp(0.20f, 0.055f, v));
          V3 nn = norm(V3(0, 0.145f, 0.175f)); PushMat pm2(cx == 6 ? MT.red : (cy == 0 ? MT.black : MT.plasticWhite)); cyl(c, c + nn * 0.02f, 0.015f, 0.015f, 8); } }
    { PushMat pm(MT.glass); G.flags |= F_NOSHADOW; box(V3(-0.17f, 0.33f, -0.069f), V3(0.17f, 0.44f, -0.064f)); }
    { PushMat pm(MT.white); for (int i = 0; i < 4; i++) box(V3(-0.13f + i * 0.075f, 0.345f, -0.085f), V3(-0.08f + i * 0.075f, 0.425f, -0.08f)); }
    { PushMat pm(MT.chrome); rbox(V3(-0.08f, 0.045f, 0.22f), V3(0.08f, 0.07f, 0.235f), 0.006f); cyl(V3(0.21f, 0.22f, -0.05f), V3(0.27f, 0.22f, -0.05f), 0.01f, 0.01f, 6); cyl(V3(0.26f, 0.22f, -0.05f), V3(0.26f, 0.14f, -0.02f), 0.008f, 0.008f, 6);
      PushMat pm2(MT.black); sphere(V3(0.26f, 0.135f, -0.018f), 0.016f, 8); }
}
static void toaster() {
    { PushMat pm(MT.chrome); rbox(V3(-0.09f, 0.012f, -0.15f), V3(0.09f, 0.19f, 0.15f), 0.035f); }
    { PushMat pm(MT.black); rbox(V3(-0.10f, 0, -0.16f), V3(0.10f, 0.02f, 0.16f), 0.008f); for (int k = -1; k <= 1; k += 2) box(V3(k * 0.035f - 0.013f, 0.185f, -0.11f), V3(k * 0.035f + 0.013f, 0.192f, 0.11f)); rbox(V3(-0.02f, 0.10f, 0.15f), V3(0.02f, 0.12f, 0.175f), 0.006f); }
}

static void pendant(V3 top, float drop, int shadeMat, V3 e) {
    V3 b = top + V3(0, -drop, 0);
    { PushMat pm(MT.black); tube({top, b + V3(0, 0.16f, 0)}, 0.004f, 5); }
    {
        Push p(xfT(b));
        PushMat pm(shadeMat);
        lathe(smoothProfile({V2(0.02f, 0.17f), V2(0.035f, 0.15f), V2(0.07f, 0.09f), V2(0.15f, 0.0f)}, 3), 24);
        PushMat pm2(MT.white);
        lathe(smoothProfile({V2(0.146f, 0.002f), V2(0.066f, 0.088f), V2(0.03f, 0.145f)}, 3), 24);
    }
    sphereLight(b + V3(0, 0.035f, 0), 0.03f, e);
}

static void sputnik(V3 c, uint64_t seed) {
    Rng r(seed);
    { PushMat pm(MT.black); tube({c, V3(c.x, LCEIL, c.z)}, 0.006f, 6); }
    { PushMat pm(MT.brass); sphere(c, 0.085f, 20); }
    int bulb = mat(V3(1, 0.9f, 0.7f), 0.3f).emit(V3(420, 300, 150), 3);
    for (int i = 0; i < 16; i++) {
        // roughly even directions over the sphere
        float z = 1 - 2 * (i + 0.5f) / 16, ph = i * 2.39996f + r.u() * 0.3f;
        float s = std::sqrt(1 - z * z);
        V3 d(s * std::cos(ph), z * 0.9f - 0.1f, s * std::sin(ph));
        d = norm(d);
        float L = r.r(0.32f, 0.55f);
        { PushMat pm(MT.brass); cyl(c + d * 0.08f, c + d * L, 0.006f, 0.004f, 6, false, false); cyl(c + d * (L - 0.03f), c + d * L, 0.012f, 0.012f, 8); }
        PushMat pm(bulb);
        G.flags |= F_NOSHADOW;
        sphere(c + d * (L + 0.022f), 0.024f, 10);
    }
    sphereLight(c, 0.24f, V3(26, 19, 9.5f), false);
}

static void shoePair(float var) {
    for (int s = -1; s <= 1; s += 2) {
        Push p(xfT(s * 0.058f, 0, 0) * xfRY(s * 0.06f));
        { PushMat pm(MT.rubber); rbox(V3(-0.045f, 0, -0.135f), V3(0.045f, 0.014f, 0.135f), 0.006f); rbox(V3(-0.04f, 0.0f, -0.135f), V3(0.04f, 0.028f, -0.06f), 0.005f); }
        { PushMat pm(mat(V3(0.5f, 0.5f, 0.5f), 0.45f).kind(K_PALETTE, 0.0f), var);
          Push q(xfT(0, 0.045f, -0.02f) * xfS(0.044f, 0.036f, 0.115f)); sphere(V3(0, 0, 0), 1, 12); }
        { PushMat pm(mat(V3(0.5f, 0.5f, 0.5f), 0.45f).kind(K_PALETTE, 0.37f), var);
          Push q(xfT(0, 0.04f, 0.075f) * xfS(0.043f, 0.03f, 0.06f)); sphere(V3(0, 0, 0), 1, 10);
          Push q2(xfT(0, 0.0f, -0.0f)); }
        { PushMat pm(MT.black); Push q(xfT(0, 0.072f, -0.05f)); lathe({V2(0.030f, 0), V2(0.034f, 0.012f), V2(0.028f, 0.014f)}, 10); }
    }
}

static void trophy(float h, int figure) {
    { PushMat pm(MT.walnut); rbox(V3(-0.06f, 0, -0.06f), V3(0.06f, 0.05f, 0.06f), 0.006f); }
    PushMat pm(MT.brass);
    Push p(xfT(0, 0.05f, 0));
    lathe(smoothProfile({V2(0.04f, 0), V2(0.015f, 0.02f), V2(0.012f, h * 0.45f), V2(0.03f, h * 0.55f), V2(0.065f, h * 0.8f), V2(0.07f, h)}, 3), 18);
    if (figure) bake(MP.pinLo, xfT(0, h * 0.55f, 0) * xfS(0.45f), 0);
    for (int s = -1; s <= 1; s += 2) torus(V3(s * 0.075f, h * 0.78f, 0), V3(0, 0, 1), 0.03f, 0.005f, 12, 5);
}

static void burgerPlate() {
    { PushMat pm(MT.plasticWhite); lathe({V2(0, 0), V2(0.07f, 0), V2(0.11f, 0.012f), V2(0.115f, 0.016f), V2(0.07f, 0.008f), V2(0, 0.008f)}, 24); }
    { PushMat pm(mat(rgb(0xC98A3A), 0.6f)); Push p(xfT(-0.02f, 0.008f, 0)); lathe(smoothProfile({V2(0, 0), V2(0.05f, 0.0f), V2(0.052f, 0.012f), V2(0, 0.014f)}, 2), 16);
      Push q(xfT(0, 0.03f, 0)); lathe(smoothProfile({V2(0.05f, 0), V2(0.053f, 0.012f), V2(0.035f, 0.035f), V2(0, 0.042f)}, 3), 16); }
    { PushMat pm(mat(rgb(0x4A2412), 0.8f)); Push p(xfT(-0.02f, 0.022f, 0)); lathe({V2(0, 0), V2(0.054f, 0), V2(0.054f, 0.012f), V2(0, 0.012f)}, 16); }
    { PushMat pm(mat(rgb(0x4E9A2E), 0.6f)); Push p(xfT(-0.02f, 0.034f, 0)); lathe({V2(0, 0), V2(0.058f, 0.0f), V2(0.058f, 0.003f), V2(0, 0.003f)}, 9); }
    { PushMat pm(mat(rgb(0xE3B54A), 0.7f)); Rng r(5); for (int i = 0; i < 9; i++) { float a = r.r(-0.6f, 0.6f); V3 c(0.06f + r.r(-0.015f, 0.02f), 0.014f + 0.005f * (i % 3), r.r(-0.04f, 0.04f)); V3 d(std::sin(a) * 0.03f, r.r(-0.004f, 0.006f), std::cos(a) * 0.03f); rbox(c - d - V3(0.004f), c + d + V3(0.004f), 0.002f); } }
}
static void milkshake(int flavour) {
    { PushMat pm(MT.glass); lathe({V2(0.03f, 0), V2(0.034f, 0.01f), V2(0.012f, 0.035f), V2(0.028f, 0.07f), V2(0.042f, 0.19f)}, 18); }
    { PushMat pm(mat(flavour ? rgb(0xF0C8CF) : rgb(0xEADFC8), 0.5f)); lathe({V2(0.011f, 0.036f), V2(0.026f, 0.07f), V2(0.039f, 0.18f), V2(0.02f, 0.2f), V2(0, 0.205f)}, 18); }
    { PushMat pm(MT.red); cyl(V3(0.01f, 0.1f, 0), V3(0.045f, 0.27f, 0.01f), 0.003f, 0.003f, 6); }
}
static void bottle(int m) {
    PushMat pm(m);
    lathe(smoothProfile({V2(0, 0), V2(0.027f, 0), V2(0.028f, 0.09f), V2(0.016f, 0.13f), V2(0.006f, 0.165f), V2(0, 0.17f)}, 3), 14);
}
static void napkinBox() {
    { PushMat pm(MT.chrome); rbox(V3(-0.05f, 0, -0.035f), V3(0.05f, 0.12f, 0.035f), 0.012f); }
    { PushMat pm(MT.white); box(V3(-0.035f, 0.03f, -0.037f), V3(0.035f, 0.09f, 0.037f)); }
}
static void coffeeCup() {
    PushMat pm(MT.plasticWhite);
    lathe({V2(0, 0), V2(0.07f, 0), V2(0.075f, 0.008f), V2(0.03f, 0.008f), V2(0.028f, 0.012f), V2(0.04f, 0.065f), V2(0.036f, 0.065f), V2(0.025f, 0.02f), V2(0, 0.02f)}, 18);
    torus(V3(0.05f, 0.04f, 0), V3(0, 0, 1), 0.018f, 0.004f, 12, 5);
    PushMat pm2(mat(rgb(0x2A1608), 0.15f));
    lathe({V2(0, 0.055f), V2(0.034f, 0.055f)}, 14);
}

static void jukebox() {
    // console jukebox: wood cabinet, chrome trim, lit title rack under a glass canopy, coloured light pilasters
    { PushMat pm(MT.walnut, 77); rbox(V3(-0.46f, 0.0f, -0.30f), V3(0.46f, 1.02f, 0.30f), 0.03f); }
    { PushMat pm(MT.chrome);
      rbox(V3(-0.47f, 0.02f, 0.295f), V3(0.47f, 0.07f, 0.315f), 0.008f);
      rbox(V3(-0.47f, 0.60f, 0.295f), V3(0.47f, 0.64f, 0.32f), 0.008f);
      for (int i = 0; i < 15; i++) { float x = -0.33f + i * 0.047f; cyl(V3(x, 0.09f, 0.31f), V3(x, 0.58f, 0.31f), 0.008f, 0.008f, 6); }
      rbox(V3(-0.48f, 1.0f, -0.31f), V3(0.48f, 1.04f, 0.33f), 0.012f); }
    { PushMat pm(mat(rgb(0x8A6A2A), 0.8f).kind(K_PEGBOARD)); quad(V3(-0.36f, 0.08f, 0.302f), V3(0.36f, 0.08f, 0.302f), V3(0.36f, 0.59f, 0.302f), V3(-0.36f, 0.59f, 0.302f)); }
    static int glowA = -1, glowB = -1;
    if (glowA < 0) { glowA = emitDecalMat(V3(5, 5, 5), 2); glowB = emitDecalMat(V3(0.6f, 0.56f, 0.44f), 2); }
    // colour pilasters either side of the grille
    decal(V3(-0.415f, 0.33f, 0.317f), V3(1, 0, 0), V3(0, 1, 0), 0.07f, 0.5f, SG.juke, glowA);
    decal(V3(0.415f, 0.33f, 0.317f), V3(-1, 0, 0), V3(0, 1, 0), 0.07f, 0.5f, SG.juke, glowA);
    // upper deck: slanted title rack and record mechanism under a glass canopy
    { PushMat pm(MT.steelDark); rbox(V3(-0.44f, 1.04f, -0.28f), V3(0.44f, 1.08f, 0.30f), 0.006f); }
    V3 up = norm(V3(0, 0.75f, -0.55f));
    decal(V3(0, 1.20f, 0.08f), V3(1, 0, 0), up, 0.76f, 0.34f, SG.jukeTitles, glowB);
    decal(V3(0, 1.36f, -0.27f), V3(1, 0, 0), V3(0, 1, 0), 0.84f, 0.5f, SG.juke, glowA);
    { PushMat pm(MT.black); for (int i = 0; i < 14; i++) { float x = -0.3f + i * 0.045f; cyl(V3(x, 1.10f, -0.14f), V3(x + 0.004f, 1.10f, -0.14f), 0.085f, 0.085f, 18); } }  // the record stack
    { PushMat pm(MT.chrome); cyl(V3(-0.36f, 1.10f, -0.14f), V3(0.36f, 1.10f, -0.14f), 0.006f, 0.006f, 6);
      for (int s = -1; s <= 1; s += 2) rbox(V3(s * 0.45f - 0.015f, 1.04f, -0.29f), V3(s * 0.45f + 0.015f, 1.62f, -0.25f), 0.006f);
      rbox(V3(-0.46f, 1.60f, -0.30f), V3(0.46f, 1.64f, -0.24f), 0.008f);
      // selector buttons
      for (int i = 0; i < 16; i++) cyl(V3(-0.3f + i * 0.04f, 1.055f, 0.265f), V3(-0.3f + i * 0.04f, 1.09f, 0.275f), 0.012f, 0.012f, 8); }
    { PushMat pm(MT.glass);
      surf(12, 1, [&](float u, float v) {
          float a = u * PI * 0.5f;
          return V3(lerp(-0.44f, 0.44f, v), 1.08f + std::sin(a) * 0.54f * (1 - 0.0f) * (u < 1 ? 1 : 1) * 0 + lerp(0.0f, 0.54f, std::sin(a)), 0.30f - (1 - std::cos(a)) * 0.56f);
      }); }
    LightDef L; L.type = 2;
    L.pos = G.xf.pt(V3(0, 0.75f, 0.42f)); L.u = G.xf.vec(V3(0.45f, 0, 0)); L.v = G.xf.vec(V3(0, 0.7f, 0)); L.emit = V3(2.4f, 1.3f, 2.2f);
    LIGHTS.push_back(L);
}

static void gumball(int colorMat) {
    { PushMat pm(colorMat); lathe(smoothProfile({V2(0, 0), V2(0.10f, 0), V2(0.11f, 0.03f), V2(0.05f, 0.08f), V2(0.07f, 0.20f), V2(0.085f, 0.24f)}, 3), 20);
      Push p(xfT(0, 0.50f, 0)); lathe({V2(0.06f, 0), V2(0.05f, 0.03f), V2(0.0f, 0.04f)}, 16); }
    { PushMat pm(MT.glass); sphere(V3(0, 0.38f, 0), 0.15f, 22); }
    Rng r(11);
    for (int i = 0; i < 70; i++) {
        V3 d(r.g(), r.g(), r.g()); d = norm(d);
        float rad = 0.125f * std::cbrt(r.u());
        V3 c = V3(0, 0.38f, 0) + d * rad;
        if (c.y > 0.40f) c.y = 0.40f - (c.y - 0.40f) * 0.7f;
        PushMat pm(mat(V3(0.5f, 0.5f, 0.5f), 0.25f).kind(K_PALETTE, 0.11f * (i % 9)), r.u() * 10);
        sphere(c, 0.013f, 6);
    }
    { PushMat pm(MT.chrome); rbox(V3(-0.03f, 0.12f, 0.06f), V3(0.03f, 0.19f, 0.09f), 0.006f); }
    { PushMat pm(MT.steelDark); cyl(V3(0, -0.75f, 0), V3(0, 0, 0), 0.02f, 0.02f, 8); lathe({V2(0, -0.75f), V2(0.16f, -0.75f), V2(0.14f, -0.73f), V2(0.02f, -0.70f)}, 16); }
}

static void car(int bodyMat, float seed) {
    // a long, low early-60s sedan: body lofted from cross-sections, greenhouse, chrome, round lamps, small fins
    const float L = 5.2f, Wd = 1.9f;
    auto section = [&](float u, float a) { // u: 0 rear .. 1 front, a: 0..pi around (left sill over the top to right sill)
        float x = -std::cos(a);
        float sy = std::max(std::sin(a), 0.0f);
        float nose = smooth(0.0f, 0.10f, u) * smooth(1.0f, 0.93f, u);
        float w = Wd * 0.5f * (0.86f + 0.14f * nose);
        float deck = 0.80f + 0.06f * smooth(0.0f, 0.3f, u) - 0.05f * smooth(0.6f, 1.0f, u);
        float sup = std::pow(std::fabs(x), 4.0f);
        float y = 0.30f + (deck - 0.30f) * std::pow(sy, 0.42f);
        float fin = (u < 0.22f) ? 0.09f * smooth(0.22f, 0.0f, u) * smooth(0.72f, 0.95f, std::fabs(x)) : 0;
        return V3(x * w * (0.93f + 0.07f * (1 - sup) * 0 + 0.07f * std::pow(sy, 0.3f)), y + fin, (u - 0.5f) * L);
    };
    { PushMat pm(bodyMat, seed); surf(40, 22, [&](float u, float v) { return section(u, v * PI); });
      // closed ends
      surf(1, 22, [&](float u, float v) { V3 p = section(0, v * PI); return u < 0.5f ? p : V3(p.x * 0.6f, 0.36f + (p.y - 0.36f) * 0.55f, p.z - 0.05f); });
      surf(1, 22, [&](float u, float v) { V3 p = section(1, v * PI); return u < 0.5f ? p : V3(p.x * 0.6f, 0.36f + (p.y - 0.36f) * 0.55f, p.z + 0.05f); }); }
    {   // greenhouse: roof and pillars, with dark glass
        auto roof = [&](float u, float a) {
            float x = -std::cos(a), sy = std::max(std::sin(a), 0.0f);
            float z = lerp(-1.25f, 0.85f, u);
            float rise = std::pow(std::sin(u * PI), 0.35f);
            float inset = 0.10f + 0.16f * (1 - rise);
            return V3(x * (Wd * 0.5f - 0.1f - inset * 0.5f) * (0.9f + 0.1f * std::pow(sy, 0.3f)), 0.82f + 0.52f * rise * std::pow(sy, 0.5f), z + (u - 0.5f) * -0.5f * (1 - rise));
        };
        PushMat pm(mat(V3(0.02f, 0.025f, 0.03f), 0.05f).spec(1.5f));
        surf(18, 16, [&](float u, float v) { return roof(u, v * PI); });
        PushMat pm2(bodyMat, seed);
        surf(8, 16, [&](float u, float v) { return roof(0.32f + u * 0.36f, v * PI) + V3(0, 0.004f, 0); });
    }
    { PushMat pm(MT.chrome);
      rbox(V3(-Wd * 0.5f - 0.02f, 0.30f, L * 0.5f - 0.02f), V3(Wd * 0.5f + 0.02f, 0.42f, L * 0.5f + 0.10f), 0.04f);
      rbox(V3(-Wd * 0.5f - 0.02f, 0.30f, -L * 0.5f - 0.10f), V3(Wd * 0.5f + 0.02f, 0.42f, -L * 0.5f + 0.02f), 0.04f);
      rbox(V3(-0.6f, 0.45f, L * 0.5f + 0.02f), V3(0.6f, 0.62f, L * 0.5f + 0.06f), 0.02f);
      for (int s = -1; s <= 1; s += 2) {
          cyl(V3(s * 0.74f, 0.56f, L * 0.5f + 0.0f), V3(s * 0.74f, 0.56f, L * 0.5f + 0.08f), 0.10f, 0.09f, 18);
          cyl(V3(s * (Wd * 0.5f + 0.002f), 0.60f, -L * 0.5f + 0.2f), V3(s * (Wd * 0.5f + 0.002f), 0.60f, L * 0.5f - 0.3f), 0.012f, 0.012f, 6);
      } }
    { PushMat pm(mat(V3(0.9f, 0.9f, 0.85f), 0.1f).emit(V3(0.5f, 0.5f, 0.45f), 3));
      for (int s = -1; s <= 1; s += 2) cyl(V3(s * 0.74f, 0.56f, L * 0.5f + 0.08f), V3(s * 0.74f, 0.56f, L * 0.5f + 0.085f), 0.085f, 0.085f, 16); }
    { PushMat pm(mat(V3(0.5f, 0.02f, 0.02f), 0.2f).emit(V3(1.6f, 0.05f, 0.03f), 3));
      for (int s = -1; s <= 1; s += 2) { cyl(V3(s * 0.72f, 0.62f, -L * 0.5f - 0.03f), V3(s * 0.72f, 0.62f, -L * 0.5f + 0.02f), 0.07f, 0.07f, 14); } }
    for (int s = -1; s <= 1; s += 2)
        for (int e = -1; e <= 1; e += 2) {
            V3 c(s * (Wd * 0.5f - 0.12f), 0.34f, e * 1.55f - 0.1f);
            { PushMat pm(MT.rubber); cyl(c - V3(0.10f, 0, 0), c + V3(0.10f, 0, 0), 0.34f, 0.34f, 22); }
            { PushMat pm(MT.chrome); cyl(c + V3(s * 0.10f, 0, 0), c + V3(s * 0.115f, 0, 0), 0.19f, 0.15f, 18); }
            { PushMat pm(MT.white); cyl(c + V3(s * 0.1005f, 0, 0), c + V3(s * 0.102f, 0, 0), 0.27f, 0.27f, 22, false, true); }
        }
}

// ---------------------------------------------------------------- lobby
static void buildLobby() {
    float lx0 = -HALL_X, lx1 = HALL_X;
    {   // floors
        PushMat pm(MT.tile);
        quad(V3(lx0, 0, STEP_Z), V3(lx0, 0, APP_Z), V3(lx1, 0, APP_Z), V3(lx1, 0, STEP_Z));
        PushMat pm2(mat(rgb(0xB9B09A), 0.16f).kind(K_TERRAZZO));
        quad(V3(lx0, FL, -11.4f), V3(lx0, FL, STEP_Z), V3(lx1, FL, STEP_Z), V3(lx1, FL, -11.4f));
        quad(V3(lx0, 0, STEP_Z), V3(lx1, 0, STEP_Z), V3(lx1, FL, STEP_Z), V3(lx0, FL, STEP_Z));
        PushMat pm3(MT.carpet);
        quad(V3(lx0, FL, FRONT_Z), V3(lx0, FL, -11.4f), V3(lx1, FL, -11.4f), V3(lx1, FL, FRONT_Z));
        PushMat pm4(MT.brass);
        box(V3(lx0, FL - 0.02f, STEP_Z - 0.003f), V3(lx1, FL + 0.003f, STEP_Z + 0.03f));
        box(V3(lx0, FL, -11.43f), V3(lx1, FL + 0.004f, -11.37f));
    }
    {   // ceiling with a soffit where it drops to the lane ceiling, and two dropped beams
        PushMat pm(MT.ceil);
        quad(V3(lx0, LCEIL, FRONT_Z), V3(lx1, LCEIL, FRONT_Z), V3(lx1, LCEIL, APP_Z - 0.6f), V3(lx0, LCEIL, APP_Z - 0.6f));
        PushMat pm2(MT.wallWood, 9);
        quad(V3(lx0, CEIL_Y, APP_Z - 0.6f), V3(lx1, CEIL_Y, APP_Z - 0.6f), V3(lx1, LCEIL, APP_Z - 0.6f), V3(lx0, LCEIL, APP_Z - 0.6f));
        for (float z : {-13.2f, -19.0f}) { PushMat pm3(MT.walnut, z); rbox(V3(lx0, LCEIL - 0.28f, z - 0.15f), V3(lx1, LCEIL, z + 0.15f), 0.01f); }
    }
    {   // side walls: wood wainscot + painted wall with the mural
        for (int s = -1; s <= 1; s += 2) {
            float x = s * HALL_X;
            { PushMat pm(MT.wallWood, (float)s + 3); quad(V3(x, FL, FRONT_Z), V3(x, FL, STEP_Z), V3(x, FL + 1.1f, STEP_Z), V3(x, FL + 1.1f, FRONT_Z)); }
            { PushMat pm(MT.wallPaint); quad(V3(x, FL + 1.1f, FRONT_Z), V3(x, FL + 1.1f, STEP_Z), V3(x, LCEIL, STEP_Z), V3(x, LCEIL, FRONT_Z)); }
            { PushMat pm(MT.chromeTrim); box(V3(x - 0.02f, FL + 1.1f, FRONT_Z), V3(x + 0.02f, FL + 1.14f, STEP_Z)); }
        }
        // mural along the east wall of the lanes and the west wall beside the lanes
        decal(V3(HALL_X - 0.004f, 2.1f, 8.0f), V3(0, 0, 1), V3(0, 1, 0), 9.0f, 1.45f, SG.mural);
        decal(V3(-HALL_X + 0.004f, 2.1f, 8.0f), V3(0, 0, -1), V3(0, 1, 0), 9.0f, 1.45f, SG.mural);
    }
    // ---------- snack bar along the west wall
    {
        const float z0 = -21.2f, z1 = -13.4f, top = FL + 1.07f;
        const float xe = SNACK_X + 0.40f;                 // customer edge of the top
        const float xp = SNACK_X + 0.20f, xs = SNACK_X - 0.30f, xc = (xp + xs) * 0.5f, rb = (xp - xs) * 0.5f;   // base: panel face, staff face
        {   // laminate top with a half-round end toward the lanes
            PushMat pm(MT.formica, 2);
            box(V3(SNACK_X - 0.36f, top - 0.045f, z0), V3(xe, top, z1));
            cyl(V3(SNACK_X + 0.02f, top - 0.045f, z1), V3(SNACK_X + 0.02f, top, z1), 0.38f, 0.38f, 40);
        }
        {   // aluminium bull-nose edging, two bead lines
            PushMat pm(MT.chromeTrim);
            for (float dy : {-0.0225f}) {
                cyl(V3(xe, top + dy, z0), V3(xe, top + dy, z1), 0.0235f, 0.0235f, 12);
                cyl(V3(SNACK_X - 0.36f, top + dy, z0), V3(SNACK_X - 0.36f, top + dy, z1), 0.0235f, 0.0235f, 12);
                torus(V3(SNACK_X + 0.02f, top + dy, z1), V3(0, 1, 0), 0.38f, 0.0235f, 48, 10);
                cyl(V3(SNACK_X - 0.36f, top + dy, z0), V3(xe, top + dy, z0), 0.0235f, 0.0235f, 12);
            }
        }
        {   // base: tuck-and-roll vinyl between chrome bands, stainless kick, foot rail on cast brackets
            { PushMat pm(MT.steelDark); box(V3(xs, FL, z0 + 0.03f), V3(xp - 0.03f, top - 0.045f, z1)); cyl(V3(xc, FL, z1), V3(xc, top - 0.045f, z1), rb - 0.03f, rb - 0.03f, 24); }
            int roll = mat(rgb(0x2A9D95), 0.32f).kind(K_VINYL), rollB = mat(rgb(0xE6DFC8), 0.32f).kind(K_VINYL);
            float y0 = FL + 0.17f, y1 = top - 0.10f;
            int np = (int)((z1 - z0 - 0.06f) / 0.058f);
            for (int i = 0; i < np; i++) {
                float z = z0 + 0.06f + (i + 0.5f) * (z1 - z0 - 0.06f) / np;
                PushMat pm((i / 6) % 4 == 3 ? rollB : roll);
                cyl(V3(xp - 0.012f, y0, z), V3(xp - 0.012f, y1, z), 0.031f, 0.031f, 10, false, false);
            }
            for (int i = 0; i <= 13; i++) {
                float a = -PI / 2 + PI * (i / 13.0f);
                PushMat pm((i / 4) % 3 == 1 ? rollB : roll);
                cyl(V3(xc + (rb - 0.012f) * std::cos(a), y0, z1 + (rb - 0.012f) * std::sin(a)), V3(xc + (rb - 0.012f) * std::cos(a), y1, z1 + (rb - 0.012f) * std::sin(a)), 0.031f, 0.031f, 10, false, false);
            }
            PushMat pm(MT.chromeTrim);
            for (float y : {y0 - 0.02f, y1 + 0.02f}) {
                box(V3(xp - 0.01f, y - 0.022f, z0 + 0.03f), V3(xp + 0.024f, y + 0.022f, z1));
                torus(V3(xc, y, z1), V3(0, 1, 0), rb + 0.006f, 0.02f, 32, 6, -PI / 2, PI / 2);
            }
            { PushMat pm2(MT.steel); box(V3(xs, FL, z0 + 0.03f), V3(xp - 0.005f, FL + 0.13f, z1)); cyl(V3(xc, FL, z1), V3(xc, FL + 0.13f, z1), rb - 0.005f, rb - 0.005f, 24); }
            float xr = xp + 0.17f, yr = FL + 0.21f;
            cyl(V3(xr, yr, z0 + 0.15f), V3(xr, yr, z1 - 0.05f), 0.021f, 0.021f, 12);
            sphere(V3(xr, yr, z0 + 0.15f), 0.026f, 10); sphere(V3(xr, yr, z1 - 0.05f), 0.026f, 10);
            for (float z = z0 + 0.45f; z < z1 - 0.2f; z += 1.2f) {
                tube(smoothPath({V3(xp + 0.01f, yr + 0.07f, z), V3(xp + 0.10f, yr + 0.05f, z), V3(xr, yr - 0.03f, z)}, 4), 0.012f, 8);
                torus(V3(xr, yr, z), V3(0, 0, 1), 0.027f, 0.008f, 12, 6);
                cyl(V3(xp + 0.005f, yr + 0.07f, z), V3(xp + 0.02f, yr + 0.07f, z), 0.03f, 0.03f, 12);
            }
        }
        // rubber duck-board behind the counter
        { PushMat pm(MT.rubber); box(V3(-HALL_X + 0.66f, FL, z0), V3(xs - 0.02f, FL + 0.012f, z1 + 0.3f)); }
        for (int i = 0; i < 9; i++) { Push p(xfT(SNACK_X + 0.78f, FL, z0 + 0.6f + i * 0.82f)); stool(); }
        // place settings and counter clutter
        for (int i = 0; i < 9; i++) {
            float z = z0 + 0.6f + i * 0.82f;
            Push p(xfT(SNACK_X, top, z));
            if (i % 2 == 0) {
                { Push q(xfT(-0.20f, 0, 0.40f)); napkinBox(); }
                { Push q(xfT(-0.19f, 0, 0.27f)); bottle(MT.red); }
                { Push q(xfT(-0.12f, 0, 0.29f)); bottle(MT.mustard); }
                { Push q(xfT(-0.07f, 0, 0.42f)); sugarPourer(); }
                { Push q(xfT(-0.02f, 0, 0.33f)); shakers(); }
            }
            if (i == 1 || i == 6) { Push q(xfT(0.14f, 0, 0.0f) * xfRY(i * 1.0f)); burgerPlate(); }
            if (i == 1 || i == 4 || i == 6) { Push q(xfT(0.0f, 0, -0.20f)); milkshake(i & 1); }
            if (i == 3 || i == 7) { Push q(xfT(0.16f, 0, 0.05f) * xfRY(i * 2.0f)); coffeeCup(); }
            if (i == 3) { Push q(xfT(0.13f, 0, -0.22f) * xfRY(0.6f)); pieSlicePlate(); }
            if (i == 7) { Push q(xfT(0.02f, 0, -0.18f)); colaGlass(); }
            if (i == 2) { Push q(xfT(-0.12f, 0, -0.38f)); cakeStand(0); }
            if (i == 5) { Push q(xfT(-0.14f, 0, -0.40f)); cakeStand(1); }
            if (i == 8) { Push q(xfT(-0.16f, 0, 0.0f)); strawJar(); }
            if (i == 0) { PushMat pm(MT.white); Push q(xfT(0.15f, 0, -0.1f) * xfRY(0.3f)); box(V3(-0.04f, 0, -0.065f), V3(0.04f, 0.006f, 0.065f)); }
        }
        // ---- back bar: stainless counter on cabinets, tiled splash-back, glass shelf of glassware
        const float bx0 = -HALL_X + 0.02f, bx1 = -HALL_X + 0.64f, by = FL + 0.95f, bx = -HALL_X + 0.33f;
        {
            { PushMat pm(MT.steel); rbox(V3(bx0, FL + 0.10f, z0), V3(bx1 - 0.02f, by - 0.03f, z1), 0.004f); rbox(V3(bx0, by - 0.035f, z0 - 0.01f), V3(bx1, by, z1 + 0.01f), 0.008f); }
            { PushMat pm(MT.black); box(V3(bx0, FL, z0 + 0.02f), V3(bx1 - 0.07f, FL + 0.10f, z1 - 0.02f));
              for (float z = z0 + 0.65f; z < z1 - 0.3f; z += 0.65f) box(V3(bx1 - 0.021f, FL + 0.13f, z - 0.003f), V3(bx1 - 0.0185f, by - 0.06f, z + 0.003f)); }
            { PushMat pm(MT.chrome); for (float z = z0 + 0.65f; z < z1 + 0.3f; z += 0.65f) cyl(V3(bx1 - 0.005f, by - 0.30f, z - 0.09f), V3(bx1 - 0.005f, by - 0.12f, z - 0.09f), 0.007f, 0.007f, 6); }
            { PushMat pm(mat(rgb(0xE9E5D8), 0.12f).kind(K_TILE, 0.108f, 0.17f, 0.60f, 0.57f)); quad(V3(-HALL_X + 0.008f, by, z1 + 0.2f), V3(-HALL_X + 0.008f, by, z0 - 0.2f), V3(-HALL_X + 0.008f, 2.0f, z0 - 0.2f), V3(-HALL_X + 0.008f, 2.0f, z1 + 0.2f)); }
            // shelf
            float sy = 1.66f;
            { PushMat pm(MT.steel); rbox(V3(bx0, sy - 0.02f, z0 + 0.3f), V3(bx0 + 0.26f, sy, z1 - 0.3f), 0.004f);
              for (float z = z0 + 0.5f; z < z1 - 0.3f; z += 1.35f) { tube({V3(bx0, sy - 0.20f, z), V3(bx0 + 0.24f, sy - 0.02f, z)}, 0.008f, 6); } }
            Rng r(31);
            for (float z = z0 + 0.45f; z < z1 - 0.45f; z += 0.085f) {
                float seg = std::fmod(z - z0, 2.2f);
                Push p(xfT(bx0 + 0.13f + r.r(-0.01f, 0.01f), sy, z));
                if (seg < 0.9f) { PushMat pm(MT.glass); G.flags |= F_NOSHADOW; lathe({V2(0, 0.004f), V2(0.026f, 0.004f), V2(0.034f, 0.12f)}, 10); { Push q(xfT(-0.075f, 0, 0.04f)); lathe({V2(0, 0.004f), V2(0.026f, 0.004f), V2(0.034f, 0.12f)}, 10); } }
                else if (seg < 1.45f) { PushMat pm(MT.glass); G.flags |= F_NOSHADOW; lathe(smoothProfile({V2(0, 0), V2(0.032f, 0), V2(0.008f, 0.02f), V2(0.008f, 0.06f), V2(0.03f, 0.10f), V2(0.045f, 0.16f)}, 2), 10); }
                else if (seg < 1.8f) { PushMat pm(MT.steel); lathe({V2(0, 0), V2(0.036f, 0), V2(0.046f, 0.17f), V2(0.048f, 0.172f)}, 12); }
                else if (std::fmod(z - z0, 0.17f) < 0.085f) { PushMat pm(MT.plasticWhite); for (int k = 0; k < 7; k++) { Push q(xfT(0, k * 0.011f, 0)); lathe({V2(0, 0), V2(0.07f, 0), V2(0.105f, 0.012f), V2(0.07f, 0.006f), V2(0, 0.006f)}, 16); } }
            }
        }
        // equipment along the back bar
        { Push p(xfT(bx + 0.02f, by, -20.75f) * xfRY(PI / 2)); cashRegister(); }
        for (int k = 0; k < 2; k++) { Push p(xfT(bx - 0.04f, by, -19.85f + k * 0.42f)); coffeeUrn(); }
        { PushMat pm(MT.plasticWhite); for (int k = 0; k < 6; k++) { Push p(xfT(bx + 0.16f, by, -19.25f + (k % 3) * 0.085f + 0.0f)); Push q(xfT((k / 3) * -0.09f, 0, 0)); lathe({V2(0.03f, 0), V2(0.04f, 0.065f), V2(0.037f, 0.065f), V2(0.027f, 0.006f), V2(0, 0.006f)}, 12); } }
        { Push p(xfT(bx, by, -18.45f) * xfRY(PI / 2)); juiceBubbler(mat(rgb(0xF08A14), 0.25f)); }
        { Push p(xfT(bx, by, -18.02f) * xfRY(PI / 2)); juiceBubbler(mat(rgb(0x8C1436), 0.25f)); }
        { Push p(xfT(bx - 0.06f, by, -17.25f) * xfRY(PI / 2)); maltMixer(); }
        {   // soda fountain: draught arms over a drip tray
            Push p(xfT(bx, by, -16.35f));
            PushMat pm(MT.chrome);
            rbox(V3(-0.14f, 0, -0.34f), V3(0.10f, 0.20f, 0.34f), 0.03f);
            { PushMat pm2(MT.steelDark); box(V3(0.10f, 0.0f, -0.30f), V3(0.24f, 0.02f, 0.30f)); }
            for (int k = 0; k < 4; k++) { float z = -0.24f + k * 0.16f; tube(smoothPath({V3(0.02f, 0.20f, z), V3(0.04f, 0.40f, z), V3(0.15f, 0.44f, z), V3(0.18f, 0.36f, z)}, 5), 0.012f, 8);
                PushMat pm2(k == 1 ? MT.red : (k == 2 ? MT.cream : MT.black)); cyl(V3(0.05f, 0.44f, z), V3(0.02f, 0.56f, z), 0.011f, 0.017f, 8); }
        }
        {   // pie case
            Push p(xfT(bx, by, -15.25f));
            { PushMat pm(MT.glass); G.flags |= F_NOSHADOW; box(V3(-0.2f, 0.02f, -0.35f), V3(0.2f, 0.42f, 0.35f)); }
            { PushMat pm(MT.chrome); rbox(V3(-0.21f, 0, -0.36f), V3(0.21f, 0.025f, 0.36f), 0.005f); rbox(V3(-0.21f, 0.42f, -0.36f), V3(0.21f, 0.445f, 0.36f), 0.005f); box(V3(-0.2f, 0.215f, -0.35f), V3(0.2f, 0.222f, 0.35f));
              for (int c = 0; c < 4; c++) { float x = (c & 1) ? 0.2f : -0.2f, z = (c & 2) ? 0.35f : -0.35f; cyl(V3(x, 0.02f, z), V3(x, 0.42f, z), 0.008f, 0.008f, 6); } }
            for (int k = 0; k < 4; k++) { Push q(xfT(0, k < 2 ? 0.03f : 0.225f, (k & 1) ? 0.16f : -0.16f));
                { PushMat pm(MT.steel); lathe({V2(0, 0), V2(0.125f, 0), V2(0.145f, 0.028f), V2(0.148f, 0.03f)}, 20); }
                PushMat pm(mat(k == 1 ? rgb(0xE8D9A0) : (k == 2 ? rgb(0x8A2A1A) : rgb(0xB8762E)), 0.6f)); lathe(smoothProfile({V2(0, 0.004f), V2(0.12f, 0.004f), V2(0.142f, 0.03f), V2(0.125f, 0.042f), V2(0.05f, 0.056f), V2(0, 0.058f)}, 2), 20, 0, k == 0 ? 2 * PI * 0.8f : 2 * PI); }
        }
        { Push p(xfT(bx, by, -14.35f)); toaster(); }
        { PushMat pm(MT.glass); G.flags |= F_NOSHADOW; for (int k = 0; k < 10; k++) { Push p(xfT(bx - 0.12f + (k % 3) * 0.085f, by + (k == 9 ? 0.13f : 0), -13.95f + (k / 3) * 0.085f)); lathe({V2(0, 0.004f), V2(0.028f, 0.004f), V2(0.036f, 0.13f)}, 12); } }
        // menu board, cola sign, clock on the wall above
        static int menuGlow = emitDecalMat(V3(0.75f, 0.70f, 0.52f), 2);
        { PushMat pm(MT.chromeTrim); box(V3(-HALL_X + 0.0f, 2.03f, -19.25f), V3(-HALL_X + 0.05f, 3.05f, -16.95f)); }
        decal(V3(-HALL_X + 0.055f, 2.54f, -18.1f), V3(0, 0, -1), V3(0, 1, 0), 2.2f, 0.96f, SG.menu, menuGlow);
        decal(V3(-HALL_X + 0.012f, 2.48f, -15.6f), V3(0, 0, -1), V3(0, 1, 0), 0.8f, 0.8f, SG.cola);
        {
            V3 c(-HALL_X + 0.03f, 2.5f, -14.2f);
            { PushMat pm(MT.chromeTrim); Push p(xfT(c) * xfRZ(-PI / 2)); lathe({V2(0.0f, -0.03f), V2(0.24f, -0.03f), V2(0.25f, 0.0f), V2(0.22f, 0.012f)}, 30); }
            decal(c + V3(0.012f, 0, 0), V3(0, 0, -1), V3(0, 1, 0), 0.44f, 0.44f, SG.clockFace);
            PushMat pm(MT.black);
            tube({c + V3(0.02f, 0, 0), c + V3(0.02f, 0.09f, -0.08f)}, 0.006f, 5);     // hour hand
            tube({c + V3(0.024f, 0, 0), c + V3(0.024f, 0.165f, 0.06f)}, 0.004f, 5);   // minute hand
        }
        // dropped fascia with the neon sign, pendant lamps over the counter
        { PushMat pm(MT.walnut, 14); rbox(V3(SNACK_X - 0.1f, 2.50f, z0 - 0.2f), V3(SNACK_X + 0.1f, LCEIL, z1 + 0.2f), 0.01f); }
        neonText("SNACK BAR", V3(SNACK_X + 0.10f, 2.68f, -15.4f), V3(0, 0, -1), V3(0, 1, 0), 0.36f, V3(60, 9, 22), 0.011f, 1.0f);
        neonPath({V3(SNACK_X + 0.135f, 2.60f, z0), V3(SNACK_X + 0.135f, 2.60f, z1)}, V3(8, 40, 38), 0.009f);
        for (int k = 0; k < 4; k++) pendant(V3(SNACK_X + 0.05f, 2.50f, z0 + 1.0f + k * 1.95f), 0.62f, (k & 1) ? MT.orange : MT.turq, V3(330, 230, 110));
        // a strip light under the shelf washes the back bar
        rectLight(V3(bx0 + 0.14f, 1.632f, (z0 + z1) * 0.5f), V3(0.02f, 0, 0), V3(0, 0, (z1 - z0) * 0.5f - 0.5f), V3(26, 23, 17), false, false);
    }
    // ---------- control desk and shoe wall on the east side
    {
        float z0 = -21.6f, z1 = -16.4f, cxn = 2.7f, top = FL + 1.05f;
        { PushMat pm(MT.formica, 5); rbox(V3(cxn - 0.36f, top - 0.04f, z0), V3(cxn + 0.36f, top, z1), 0.012f); }
        { PushMat pm(MT.walnut, 20); box(V3(cxn - 0.28f, FL, z0 + 0.04f), V3(cxn + 0.30f, top - 0.04f, z1 - 0.04f)); }
        { PushMat pm(MT.orange); for (float z = z0 + 0.3f; z < z1 - 0.3f; z += 0.9f) rbox(V3(cxn - 0.30f, FL + 0.2f, z), V3(cxn - 0.28f, top - 0.14f, z + 0.6f), 0.004f); }
        { PushMat pm(MT.chromeTrim); rbox(V3(cxn - 0.375f, top - 0.05f, z0 - 0.01f), V3(cxn - 0.345f, top + 0.004f, z1 + 0.01f), 0.006f); }
        {   // cash register
            Push p(xfT(cxn + 0.05f, top, -17.2f) * xfRY(-PI / 2));
            PushMat pm(mat(rgb(0x6E5A3A), 0.35f).metal(0.6f));
            rbox(V3(-0.2f, 0, -0.2f), V3(0.2f, 0.16f, 0.2f), 0.02f);
            rbox(V3(-0.2f, 0.16f, -0.2f), V3(0.2f, 0.42f, 0.02f), 0.03f);
            PushMat pm2(MT.plasticWhite);
            for (int i = 0; i < 20; i++) cyl(V3(-0.15f + (i % 5) * 0.075f, 0.16f, 0.05f + (i / 5) * 0.04f), V3(-0.15f + (i % 5) * 0.075f, 0.185f + 0.01f * (i / 5), 0.05f + (i / 5) * 0.04f), 0.014f, 0.014f, 8);
            PushMat pm3(MT.glass); box(V3(-0.15f, 0.30f, 0.021f), V3(0.15f, 0.38f, 0.024f));
        }
        { Push p(xfT(cxn - 0.1f, top, -18.3f)); PushMat pm(MT.chrome); lathe(smoothProfile({V2(0.05f, 0), V2(0.05f, 0.012f), V2(0.035f, 0.035f), V2(0, 0.045f)}, 3), 18); PushMat pm2(MT.black); cyl(V3(0, 0.045f, 0), V3(0, 0.06f, 0), 0.008f, 0.008f, 6); }
        { Push p(xfT(cxn - 0.1f, top, -19.3f)); shoePair(3.3f); }
        { Push p(xfT(cxn + 0.1f, top, -20.3f)); PushMat pm(MT.white); for (int i = 0; i < 12; i++) box(V3(-0.11f, i * 0.004f, -0.14f + 0.002f * (i % 3)), V3(0.11f, i * 0.004f + 0.003f, 0.14f)); }
        {   // PA microphone on a gooseneck
            Push p(xfT(cxn + 0.15f, top, -18.9f));
            PushMat pm(MT.chrome);
            lathe({V2(0, 0), V2(0.06f, 0), V2(0.05f, 0.02f), V2(0.01f, 0.03f)}, 14);
            tube(smoothPath({V3(0, 0.03f, 0), V3(0, 0.2f, 0), V3(-0.06f, 0.3f, 0), V3(-0.14f, 0.32f, 0)}, 5), 0.006f, 6);
            PushMat pm2(MT.steelDark); cyl(V3(-0.14f, 0.32f, 0), V3(-0.22f, 0.33f, 0), 0.022f, 0.026f, 12);
        }
        // shoe cubbies on the wall behind
        float wx = 4.6f;
        { PushMat pm(MT.walnut, 22); box(V3(wx, FL, z0 - 0.2f), V3(wx + 0.34f, FL + 2.0f, z1 + 0.2f)); }
        {
            Rng r(17);
            int cols = 15, rows = 5;
            float cw = (z1 - z0 + 0.3f) / cols, ch = 0.30f;
            for (int ry = 0; ry < rows; ry++)
                for (int c = 0; c < cols; c++) {
                    float zc = z0 - 0.15f + (c + 0.5f) * cw, yc = FL + 0.35f + ry * ch;
                    { PushMat pm(MT.black); box(V3(wx - 0.004f, yc + 0.012f, zc - cw * 0.5f + 0.015f), V3(wx + 0.002f, yc + ch - 0.012f, zc + cw * 0.5f - 0.015f)); }
                    { PushMat pm(MT.cream); box(V3(wx - 0.26f, yc - 0.006f, zc - cw * 0.5f), V3(wx, yc + 0.006f, zc + cw * 0.5f)); }
                    if (r.u() < 0.72f) { Push p(xfT(wx - 0.13f, yc + 0.006f, zc) * xfRY(-PI / 2 + r.r(-0.15f, 0.15f))); shoePair(r.u() * 20); }
                }
            PushMat pm(MT.cream);
            for (int c = 0; c <= cols; c++) { float zc = z0 - 0.15f + c * cw; box(V3(wx - 0.26f, FL + 0.34f, zc - 0.006f), V3(wx, FL + 0.35f + rows * ch, zc + 0.006f)); }
            box(V3(wx - 0.26f, FL + 0.35f + rows * ch - 0.006f, z0 - 0.15f), V3(wx, FL + 0.35f + rows * ch + 0.006f, z1 + 0.15f));
        }
        // hanging sign above the counter (two-sided), rates board, league board
        {
            V3 c(cxn, 2.55f, -19.0f);
            { PushMat pm(MT.chromeTrim); rbox(V3(cxn - 0.02f, 2.25f, -20.1f), V3(cxn + 0.02f, 2.85f, -17.9f), 0.006f);
              cyl(V3(cxn, 2.85f, -19.9f), V3(cxn, LCEIL, -19.9f), 0.008f, 0.008f, 6); cyl(V3(cxn, 2.85f, -18.1f), V3(cxn, LCEIL, -18.1f), 0.008f, 0.008f, 6); }
            decal(c + V3(-0.022f, 0, 0), V3(0, 0, 1), V3(0, 1, 0), 2.1f, 0.56f, SG.shoeRental);
            decal(c + V3(0.022f, 0, 0), V3(0, 0, -1), V3(0, 1, 0), 2.1f, 0.56f, SG.shoeRental);
        }
        decal(V3(wx - 0.006f, FL + 2.25f, -20.2f), V3(0, 0, 1), V3(0, 1, 0), 0.9f, 0.675f, SG.rates);
        decal(V3(wx - 0.006f, FL + 2.25f, -17.8f), V3(0, 0, 1), V3(0, 1, 0), 0.9f, 0.675f, SG.league);
        pendant(V3(cxn, LCEIL, -17.4f), 1.25f, MT.mustard, V3(300, 210, 100));
        pendant(V3(cxn, LCEIL, -20.6f), 1.25f, MT.mustard, V3(300, 210, 100));
    }
    // ---------- jukebox, vending, trophy case, gumballs
    { Push p(xfT(JUKE_POS) * xfRY(PI + 0.75f)); jukebox(); }
    {   // cigarette machine and a cola cooler on the east wall
        Push p(xfT(HALL_X - 0.34f, FL, -13.3f) * xfRY(-PI / 2));
        { PushMat pm(mat(rgb(0x7A1010), 0.35f)); rbox(V3(-0.45f, 0, -0.3f), V3(0.45f, 1.7f, 0.3f), 0.03f); }
        decal(V3(0, 1.25f, 0.302f), V3(1, 0, 0), V3(0, 1, 0), 0.55f, 0.75f, SG.cigs);
        { PushMat pm(MT.chrome); for (int i = 0; i < 8; i++) cyl(V3(-0.35f + i * 0.1f, 0.72f, 0.30f), V3(-0.35f + i * 0.1f, 0.72f, 0.36f), 0.018f, 0.018f, 8); rbox(V3(-0.4f, 0.25f, 0.29f), V3(0.4f, 0.4f, 0.32f), 0.01f); }
    }
    {
        Push p(xfT(HALL_X - 0.4f, FL, -11.9f) * xfRY(-PI / 2));
        { PushMat pm(MT.red); rbox(V3(-0.5f, 0, -0.33f), V3(0.5f, 0.95f, 0.33f), 0.04f); }
        { PushMat pm(MT.chrome); rbox(V3(-0.5f, 0.95f, -0.33f), V3(0.5f, 0.99f, 0.33f), 0.01f); }
        decal(V3(0, 0.5f, 0.332f), V3(1, 0, 0), V3(0, 1, 0), 0.7f, 0.7f, SG.cola);
    }
    {   // trophy case on the east wall near the entrance
        Push p(xfT(HALL_X - 0.22f, FL, -23.2f) * xfRY(-PI / 2));
        { PushMat pm(MT.walnut, 30); rbox(V3(-1.1f, 0, -0.2f), V3(1.1f, 0.9f, 0.2f), 0.01f); rbox(V3(-1.1f, 1.95f, -0.2f), V3(1.1f, 2.02f, 0.2f), 0.01f); box(V3(-1.1f, 0.9f, -0.2f), V3(1.1f, 1.95f, -0.17f)); }
        { PushMat pm(MT.glass); box(V3(-1.08f, 0.9f, -0.16f), V3(1.08f, 1.95f, 0.19f)); box(V3(-1.06f, 1.42f, -0.16f), V3(1.06f, 1.428f, 0.17f)); }
        Rng r(23);
        for (int i = 0; i < 9; i++) { Push q(xfT(-0.9f + (i % 5) * 0.45f + (i >= 5 ? 0.2f : 0), i < 5 ? 0.905f : 1.43f, 0.0f) * xfRY(r.u())); trophy(r.r(0.16f, 0.30f), i % 2); }
        decal(V3(0, 0.6f, 0.203f), V3(1, 0, 0), V3(0, 1, 0), 0.6f, 0.15f, SG.trophyPlate);
        rectLight(V3(0, 1.94f, 0.02f), V3(0.9f, 0, 0), V3(0, 0, -0.03f), V3(22, 19, 13), false, false);
    }
    for (int k = 0; k < 2; k++) { Push p(xfT(-2.25f + k * 0.42f, FL + 0.75f, FRONT_Z + 0.45f)); gumball(k ? MT.red : MT.turq); }
    // pennants and posters
    for (int i = 0; i < 3; i++) decal(V3(HALL_X - 0.006f, 2.55f, -19.6f + i * 1.5f), V3(0, 0, 1), V3(0.1f * (i - 1), 1, 0), 0.8f, 0.33f, SG.pennant[i]);
    decal(V3(HALL_X - 0.006f, 1.95f, -14.9f), V3(0, 0, 1), V3(0, 1, 0), 0.62f, 0.88f, SG.posterA);
    decal(V3(-HALL_X + 0.006f, 1.95f, -12.2f), V3(0, 0, -1), V3(0, 1, 0), 0.62f, 0.88f, SG.posterB);
    decal(V3(-HALL_X + 0.006f, 1.9f, -22.6f), V3(0, 0, -1), V3(0, 1, 0), 0.66f, 0.66f, SG.hours);
    decal(V3(-HALL_X + 0.006f, 1.95f, -9.8f), V3(0, 0, -1), V3(0, 1, 0), 0.9f, 0.35f, SG.noStreet);
    // ---------- house-ball racks and railing along the step
    for (int side = 0; side < 2; side++) {
        float x0 = side ? 1.9f : -7.6f, x1 = side ? 7.6f : -1.9f;
        float z = -9.15f;
        { PushMat pm(MT.walnut, 40 + side);
          for (float x : {x0, (x0 + x1) * 0.5f, x1}) rbox(V3(x - 0.03f, FL, z - 0.22f), V3(x + 0.03f, FL + 1.05f, z + 0.22f), 0.008f); }
        for (int tier = 0; tier < 2; tier++) {
            float y = FL + 0.42f + tier * 0.46f;
            { PushMat pm(MT.chromeTrim); for (int s = -1; s <= 1; s += 2) cyl(V3(x0, y, z + s * 0.075f), V3(x1, y, z + s * 0.075f), 0.012f, 0.012f, 8); }
            int n = (int)((x1 - x0 - 0.2f) / 0.232f);
            for (int i = 0; i < n; i++) {
                if (hash1(i * 3.3f + tier * 17.0f + side * 5.0f) < 0.16f) continue;
                float x = x0 + 0.16f + i * 0.232f;
                bake(P_BALL, xfT(x, y + BALL_R * 0.72f, z) * xfRX(i * 1.7f + tier) * xfRY(i * 0.9f), (float)(int)(hash1(i * 7.7f + tier * 3.0f + side) * 6.99f));
            }
        }
    }
    {   // chrome railing along the step, open at the lane-pair entrances
        PushMat pm(MT.chromeTrim);
        for (int p = 0; p <= NLANES / 2; p++) {
            float a = p == 0 ? -HALL_X + 0.1f : (laneX(2 * p - 2) + laneX(2 * p - 1)) * 0.5f + 0.8f;
            float b = p == NLANES / 2 ? HALL_X - 0.1f : (laneX(2 * p) + laneX(2 * p + 1)) * 0.5f - 0.8f;
            if (b - a < 0.3f) continue;
            cyl(V3(a, FL + 0.9f, STEP_Z - 0.12f), V3(b, FL + 0.9f, STEP_Z - 0.12f), 0.02f, 0.02f, 10);
            cyl(V3(a, FL + 0.5f, STEP_Z - 0.12f), V3(b, FL + 0.5f, STEP_Z - 0.12f), 0.012f, 0.012f, 8);
            for (float x : {a, b}) { cyl(V3(x, FL, STEP_Z - 0.12f), V3(x, FL + 0.9f, STEP_Z - 0.12f), 0.02f, 0.02f, 10); sphere(V3(x, FL + 0.9f, STEP_Z - 0.12f), 0.03f, 10); }
        }
    }
    // ---------- lighting
    for (float z : {-22.6f, -16.1f, -10.2f})
        for (float x : {-5.0f, -0.6f, 3.6f, 7.4f}) canLight(V3(x, LCEIL, z), 0.11f, V3(46, 33, 17));
    for (float x : {-7.0f, -3.5f, 0.0f, 3.5f, 7.0f}) canLight(V3(x, CEIL_Y, -6.9f), 0.10f, V3(42, 31, 17));
    sputnik(V3(-3.6f, 2.62f, -20.6f), 5);
    sputnik(V3(-0.6f, 2.62f, -14.6f), 8);
    // exit sign over the front door (inside)
    static int exitGlow2 = emitDecalMat(V3(9, 1.2f, 0.8f), 2);
    decal(V3((DOOR_X0 + DOOR_X1) * 0.5f, FL + DOOR_H + 0.32f, FRONT_Z + 0.03f), V3(1, 0, 0), V3(0, 1, 0), 0.44f, 0.19f, SG.exitSign, exitGlow2);
}

// ---------------------------------------------------------------- front wall, doors, exterior
static void buildExterior() {
    const float zi = FRONT_Z, ze = FRONT_Z - 0.3f; // inside / outside faces of the front wall
    const float sillY = FL + 0.55f, headY = FL + 2.5f, roofY = 5.2f;
    const float EX = HALL_X + 0.3f;
    int alu = mat(V3(0.75f, 0.76f, 0.77f), 0.3f).metal().kind(K_CHROME);
    int enamel = mat(rgb(0x2C9C97), 0.22f).kind(K_TILE, 0.9f, 0.14f, 0.57f, 0.55f);
    int brick = mat(rgb(0x8A4A32), 0.9f).kind(K_BRICK);
    auto wallSlab = [&](float x0, float x1, float y0, float y1, int mOut, int mIn) {
        { PushMat pm(mOut); quad(V3(x1, y0, ze), V3(x0, y0, ze), V3(x0, y1, ze), V3(x1, y1, ze)); }
        { PushMat pm(mIn); quad(V3(x0, y0, zi), V3(x1, y0, zi), V3(x1, y1, zi), V3(x0, y1, zi)); }
        { PushMat pm(mIn); quad(V3(x0, y0, ze), V3(x0, y0, zi), V3(x0, y1, zi), V3(x0, y1, ze)); quad(V3(x1, y0, zi), V3(x1, y0, ze), V3(x1, y1, ze), V3(x1, y1, zi));
          quad(V3(x0, y0, ze), V3(x1, y0, ze), V3(x1, y0, zi), V3(x0, y0, zi)); quad(V3(x0, y1, zi), V3(x1, y1, zi), V3(x1, y1, ze), V3(x0, y1, ze)); }
    };
    // window bays: [x0,x1] ranges that are glazed between sillY and headY
    const float bays[2][2] = {{-9.0f, DOOR_X0 - 0.35f}, {DOOR_X1 + 0.35f, 5.6f}};
    // base wall under the windows (brick outside)
    wallSlab(-EX, DOOR_X0, FL - 0.3f, sillY, brick, MT.wallWood);
    wallSlab(DOOR_X1, EX, FL - 0.3f, sillY, brick, MT.wallWood);
    // piers
    wallSlab(-EX, bays[0][0], sillY, headY, brick, MT.wallPaint);
    wallSlab(bays[0][1], DOOR_X0, sillY, headY, enamel, MT.wallPaint);
    wallSlab(DOOR_X1, bays[1][0], sillY, headY, enamel, MT.wallPaint);
    wallSlab(bays[1][1], EX, sillY, headY, brick, MT.wallPaint);
    // band above the windows up to the parapet (porcelain enamel panels), and transom over the door
    wallSlab(-EX, EX, headY, roofY, enamel, MT.wallPaint);
    wallSlab(DOOR_X0, DOOR_X1, FL + DOOR_H, headY, enamel, MT.wallPaint);
    for (int b = 0; b < 2; b++) {   // glazing and mullions
        float x0 = bays[b][0], x1 = bays[b][1];
        { PushMat pm(MT.glass); G.flags |= F_NOSHADOW; quad(V3(x1, sillY, ze + 0.12f), V3(x0, sillY, ze + 0.12f), V3(x0, headY, ze + 0.12f), V3(x1, headY, ze + 0.12f)); }
        PushMat pm(alu);
        int n = (int)std::ceil((x1 - x0) / 1.5f);
        for (int i = 0; i <= n; i++) { float x = lerp(x0, x1, (float)i / n); rbox(V3(x - 0.025f, sillY, ze + 0.06f), V3(x + 0.025f, headY, ze + 0.18f), 0.005f); }
        rbox(V3(x0, sillY - 0.02f, ze + 0.04f), V3(x1, sillY + 0.03f, ze + 0.2f), 0.005f);
        rbox(V3(x0, headY - 0.03f, ze + 0.04f), V3(x1, headY + 0.02f, ze + 0.2f), 0.005f);
    }
    {   // door frame and the two glass leaves, propped open inward
        PushMat pm(alu);
        rbox(V3(DOOR_X0 - 0.05f, FL, ze + 0.05f), V3(DOOR_X0, FL + DOOR_H, zi - 0.02f), 0.006f);
        rbox(V3(DOOR_X1, FL, ze + 0.05f), V3(DOOR_X1 + 0.05f, FL + DOOR_H, zi - 0.02f), 0.006f);
        rbox(V3(DOOR_X0 - 0.05f, FL + DOOR_H, ze + 0.05f), V3(DOOR_X1 + 0.05f, FL + DOOR_H + 0.05f, zi - 0.02f), 0.006f);
        for (int s = 0; s < 2; s++) {
            float hx = s ? DOOR_X1 - 0.03f : DOOR_X0 + 0.03f;
            Push p(xfT(hx, FL, zi + 0.02f));
            // leaf lies along +z from the hinge
            { PushMat pm2(alu);
              rbox(V3(-0.02f, 0.02f, 0), V3(0.02f, DOOR_H - 0.03f, 0.05f), 0.005f); rbox(V3(-0.02f, 0.02f, 0.83f), V3(0.02f, DOOR_H - 0.03f, 0.88f), 0.005f);
              rbox(V3(-0.02f, 0.02f, 0), V3(0.02f, 0.14f, 0.88f), 0.005f); rbox(V3(-0.02f, DOOR_H - 0.09f, 0), V3(0.02f, DOOR_H - 0.03f, 0.88f), 0.005f);
              float sx = s ? -1.f : 1.f;
              cyl(V3(sx * 0.05f, 1.05f, 0.08f), V3(sx * 0.05f, 1.05f, 0.80f), 0.014f, 0.014f, 8);
              cyl(V3(0, 1.05f, 0.1f), V3(sx * 0.05f, 1.05f, 0.1f), 0.008f, 0.008f, 6); cyl(V3(0, 1.05f, 0.78f), V3(sx * 0.05f, 1.05f, 0.78f), 0.008f, 0.008f, 6); }
            { PushMat pm2(MT.glass); G.flags |= F_NOSHADOW; quad(V3(0, 0.14f, 0.05f), V3(0, 0.14f, 0.83f), V3(0, DOOR_H - 0.09f, 0.83f), V3(0, DOOR_H - 0.09f, 0.05f)); }
            decal(V3(s ? -0.004f : 0.004f, 1.3f, 0.44f), V3(0, 0, s ? 1.f : -1.f), V3(0, 1, 0), 0.16f, 0.08f, SG.doorPush);
        }
        // door mat
        PushMat pm3(MT.rubber);
        rbox(V3(DOOR_X0 + 0.1f, FL, zi + 0.1f), V3(DOOR_X1 - 0.1f, FL + 0.012f, zi + 1.1f), 0.004f);
    }
    // threshold between the wall faces
    { PushMat pm(alu); box(V3(DOOR_X0, FL - 0.01f, ze), V3(DOOR_X1, FL + 0.004f, zi)); }
    // ---------- outside: sidewalk, curb, lot
    {
        PushMat pm(mat(rgb(0x9A968C), 0.85f).kind(K_SIDEWALK));
        quad(V3(-40, FL - 0.002f, ze - 3.2f), V3(-40, FL - 0.002f, ze), V3(40, FL - 0.002f, ze), V3(40, FL - 0.002f, ze - 3.2f));
        quad(V3(-40, 0.0f, ze - 3.2f), V3(-40, FL - 0.002f, ze - 3.2f), V3(40, FL - 0.002f, ze - 3.2f), V3(40, 0.0f, ze - 3.2f));
        PushMat pm2(mat(rgb(0x4E4D4C), 0.75f).kind(K_ASPHALT));
        quad(V3(-60, 0.0f, -75), V3(-60, 0.0f, ze - 3.2f), V3(60, 0.0f, ze - 3.2f), V3(60, 0.0f, -75));
        // the street beyond the lot, and a strip of kerb between them
        quad(V3(-400, -0.01f, -100), V3(-400, -0.01f, -78), V3(400, -0.01f, -78), V3(400, -0.01f, -100));
        PushMat pm3(MT.concrete);
        box(V3(-60, -0.02f, -78), V3(60, 0.14f, -75));
    }
    buildBackdrop();
    {   // side and far walls of the building seen from outside, roof slab
        PushMat pm(brick);
        quad(V3(-EX, -0.3f, ze), V3(-EX, -0.3f, BACK_Z), V3(-EX, roofY, BACK_Z), V3(-EX, roofY, ze));
        quad(V3(EX, -0.3f, BACK_Z), V3(EX, -0.3f, ze), V3(EX, roofY, ze), V3(EX, roofY, BACK_Z));
        PushMat pm2(MT.black);
        quad(V3(-EX, roofY, ze), V3(-EX, roofY, BACK_Z), V3(EX, roofY, BACK_Z), V3(EX, roofY, ze));
        // lobby roof underside is the ceiling; close the gap between lobby ceiling and roof on the hall side is not visible
    }
    {   // entrance canopy with downlights and a neon stripe
        float cx0 = DOOR_X0 - 1.9f, cx1 = DOOR_X1 + 1.9f, cz = ze - 2.9f, cy = FL + 2.62f;
        { PushMat pm(MT.cream); rbox(V3(cx0, cy, cz), V3(cx1, cy + 0.26f, ze), 0.02f); }
        { PushMat pm(alu); rbox(V3(cx0 - 0.01f, cy + 0.08f, cz - 0.012f), V3(cx1 + 0.01f, cy + 0.18f, cz), 0.004f); }
        for (int s = 0; s < 2; s++) { PushMat pm(alu); float x = s ? cx1 - 0.25f : cx0 + 0.25f; cyl(V3(x, FL, cz + 0.3f), V3(x, cy, cz + 0.3f), 0.045f, 0.045f, 14); }
        for (int i = 0; i < 3; i++) canLight(V3(lerp(cx0 + 0.9f, cx1 - 0.9f, i / 2.0f), cy, ze - 1.45f), 0.12f, V3(50, 38, 22));
        neonPath({V3(cx0, cy + 0.13f, cz - 0.03f), V3(cx1, cy + 0.13f, cz - 0.03f)}, V3(8, 46, 42), 0.012f);
        neonPath({V3(cx0 - 0.03f, cy + 0.13f, cz), V3(cx0 - 0.03f, cy + 0.13f, ze)}, V3(8, 46, 42), 0.012f);
        neonPath({V3(cx1 + 0.03f, cy + 0.13f, cz), V3(cx1 + 0.03f, cy + 0.13f, ze)}, V3(8, 46, 42), 0.012f);
        LightDef L; L.type = 2; L.pos = V3((cx0 + cx1) * 0.5f, cy + 0.13f, cz - 0.08f); L.u = V3((cx1 - cx0) * 0.5f, 0, 0); L.v = V3(0, 0.06f, 0); L.emit = V3(1.6f, 9.0f, 8.2f); LIGHTS.push_back(L);
        static int openGlow = emitDecalMat(V3(10, 2.0f, 1.0f), 2);
        decal(V3(DOOR_X1 + 1.2f, FL + 1.9f, ze + 0.10f), V3(-1, 0, 0), V3(0, 1, 0), 0.6f, 0.2f, SG.openSign, openGlow);
    }
    // ---------- roof sign: STARLITE in pink-red neon, LANES in turquoise, a star and a pin
    {
        float sy = roofY + 0.75f, zs = ze - 0.25f;
        { PushMat pm(MT.steelDark);
          for (float x = -7.5f; x <= 5.6f; x += 1.45f) { cyl(V3(x, roofY, zs + 0.4f), V3(x, sy + 2.0f, zs + 0.12f), 0.03f, 0.03f, 6); cyl(V3(x, roofY, zs + 1.6f), V3(x, sy + 1.6f, zs + 0.14f), 0.02f, 0.02f, 6); }
          for (float y : {sy - 0.15f, sy + 0.85f, sy + 1.85f}) cyl(V3(-7.6f, y, zs + 0.1f), V3(5.7f, y, zs + 0.1f), 0.02f, 0.02f, 6);
          // dark backing letters panel
          PushMat pm2(mat(rgb(0x14202A), 0.6f));
          box(V3(-7.7f, sy - 0.3f, zs + 0.02f), V3(5.8f, sy + 2.05f, zs + 0.06f)); }
        int A_FLICK = addAnim([](double t) { double p = std::fmod(t * 0.37 + 0.2, 1.0); return (p > 0.93 && std::fmod(t * 23.0, 1.0) < 0.5) ? 0.25f : 1.0f; });
        float h1 = 1.25f;
        float w1 = textWidth("STARLITE") * h1 / 6;
        // viewer faces +z: text runs from +x (viewer's left) toward -x
        neonText("STARLITE", V3(5.2f, sy + 0.55f, zs), V3(-1, 0, 0), V3(0, 1, 0), h1, V3(95, 10, 26), 0.022f, 1.0f);
        neonText("LANES", V3(5.2f - w1 + 3.4f, sy - 0.18f, zs), V3(-1, 0, 0), V3(0, 1, 0), 0.6f, V3(10, 70, 64), 0.016f, 1.0f, A_FLICK);
        // eight-point star to the right of the name
        {
            V3 c(5.2f - w1 - 0.9f, sy + 1.25f, zs - 0.035f);
            std::vector<V3> st;
            for (int i = 0; i <= 16; i++) { float a = i * PI / 8; float r = (i & 1) ? 0.22f : ((i % 4 == 0) ? 0.85f : 0.5f); st.push_back(c + V3(std::sin(a) * r, std::cos(a) * r, 0)); }
            neonPath(st, V3(90, 62, 12), 0.016f);
            LightDef L; L.type = 2; L.pos = c + V3(0, 0, -0.05f); L.u = V3(0.6f, 0, 0); L.v = V3(0, 0.6f, 0); L.emit = V3(7, 4.8f, 1.0f); LIGHTS.push_back(L);
        }
        {   // neon bowling pin outline at the left end
            V3 c(5.2f + 0.75f, sy - 0.1f, zs - 0.035f);
            std::vector<V2> pr = pinProfile();
            std::vector<V3> pts;
            float sc = 5.2f;
            for (size_t i = 2; i < pr.size(); i += 2) pts.push_back(c + V3(-pr[i].x * sc, pr[i].y * sc, 0));
            for (size_t i = pr.size() - 1; i >= 2; i -= 2) { pts.push_back(c + V3(pr[i].x * sc, pr[i].y * sc, 0)); if (i < 4) break; }
            pts.push_back(pts[0]);
            neonPath(pts, V3(70, 66, 58), 0.016f);
            neonPath({c + V3(-0.12f, 1.40f, 0), c + V3(0.12f, 1.40f, 0)}, V3(90, 8, 8), 0.016f);
            neonPath({c + V3(-0.14f, 1.52f, 0), c + V3(0.14f, 1.52f, 0)}, V3(90, 8, 8), 0.016f);
            LightDef L; L.type = 2; L.pos = c + V3(0, 1.0f, -0.05f); L.u = V3(0.3f, 0, 0); L.v = V3(0, 1.0f, 0); L.emit = V3(5, 4.6f, 4.0f); LIGHTS.push_back(L);
        }
    }
    // ---------- parking-lot lamps (mercury vapour), marquee by the road
    for (int k = 0; k < 2; k++) {
        V3 b(k ? 11.5f : -13.0f, 0, k ? -39.0f : -35.5f);
        { PushMat pm(MT.steelDark); cyl(b, b + V3(0, 6.2f, 0), 0.07f, 0.045f, 10); tube(smoothPath({b + V3(0, 6.2f, 0), b + V3(0, 6.6f, 0.2f), b + V3(0, 6.6f, 1.1f)}, 5), 0.03f, 8);
          Push p(xfT(b + V3(0, 6.52f, 1.25f))); lathe({V2(0.05f, 0.12f), V2(0.22f, 0.02f), V2(0.24f, 0.0f)}, 16); }
        { PushMat pm(mat(V3(0.75f, 0.78f, 0.74f), 0.25f)); sphere(b + V3(0, 6.45f, 1.25f), 0.09f, 10); }   // not lit yet at this hour
    }
    {
        Push p(xfT(13.5f, 0, -31.5f) * xfRY(0.5f));
        { PushMat pm(MT.steelDark); cyl(V3(-0.9f, 0, 0), V3(-0.9f, 2.2f, 0), 0.05f, 0.05f, 8); cyl(V3(0.9f, 0, 0), V3(0.9f, 2.2f, 0), 0.05f, 0.05f, 8); box(V3(-1.25f, 2.2f, -0.1f), V3(1.25f, 3.3f, 0.1f)); }
        static int mq = emitDecalMat(V3(6.5f, 6.2f, 5.2f), 2);
        decal(V3(0, 2.75f, -0.102f), V3(-1, 0, 0), V3(0, 1, 0), 2.4f, 1.0f, SG.marquee, mq);
    }
    // ---------- parked cars: nose-in to the kerb either side of the walkway, and a second row across the aisle
    {
        struct Park { float x, z, yaw; CarStyle st; };
        const V3 ivory = rgb(0xE8E0CC), black(0.012f, 0.012f, 0.014f);
        std::vector<Park> cars = {
            {-6.85f, -31.25f, 0.02f, {rgb(0x7FC4BD), ivory, rgb(0x9FD0C8), 5.30f, 0.05f, false, 0.045f}},      // turquoise / ivory hardtop
            {-9.55f, -31.45f, -0.03f, {rgb(0x7A1414), rgb(0x7A1414), rgb(0x8E1B1B), 5.38f, 0.10f, false, 0.045f}},  // maroon, fins
            {-14.95f, -31.35f, 0.03f, {rgb(0xD9D2BC), rgb(0xD9D2BC), rgb(0x9B2A20), 5.20f, 0.0f, true, 0.0f}},      // cream wagon
            {1.25f, -31.30f, -0.02f, {rgb(0x1E2F4F), ivory, rgb(0x5A6E8C), 5.30f, 0.03f, false, 0.045f}},      // navy / ivory
            {6.65f, -31.50f, 0.04f, {rgb(0xC9A227), rgb(0xC9A227), rgb(0x2A2620), 5.15f, 0.0f, false, 0.0f}},        // mustard
            {9.35f, -31.20f, 0.0f, {black, black, rgb(0x7A1414), 5.40f, 0.12f, false, 0.045f}},                     // black, tall fins
            {-12.25f, -44.90f, PI + 0.03f, {rgb(0x8FA88E), ivory, rgb(0x8FA88E), 5.30f, 0.04f, false, 0.045f}},     // sage
            {3.95f, -44.70f, PI - 0.02f, {rgb(0xB23A26), rgb(0xB23A26), rgb(0xD9D2BC), 5.25f, 0.0f, true, 0.045f}},  // red wagon
            {9.35f, -44.95f, PI + 0.02f, {rgb(0x6C7F9C), rgb(0x6C7F9C), rgb(0x47566E), 5.30f, 0.07f, false, 0.0f}},
        };
        for (auto &c : cars) { Push p(xfT(c.x, 0, c.z) * xfRY(c.yaw)); car2(c.st); }
        // concrete wheel stops at the head of each stall
        PushMat pm(MT.concrete);
        for (int k = -6; k <= 5; k++) {
            float x = 2.7f * k + 1.25f;
            if (x > -5.6f && x < 0.0f) continue;
            rbox(V3(x - 0.85f, 0, -29.05f), V3(x + 0.85f, 0.11f, -28.85f), 0.03f);
        }
    }
    // planter boxes with clipped shrubs under the windows
    for (int b = 0; b < 2; b++) {
        float x0 = bays[b][0] + 0.2f, x1 = bays[b][1] - 0.2f;
        { PushMat pm(brick); rbox(V3(x0, FL, ze - 0.55f), V3(x1, FL + 0.42f, ze - 0.02f), 0.01f); }
        { PushMat pm(mat(rgb(0x2A2018), 0.95f)); box(V3(x0 + 0.04f, FL + 0.40f, ze - 0.51f), V3(x1 - 0.04f, FL + 0.415f, ze - 0.06f)); }   // soil
        PushMat pm(mat(rgb(0x2C4A22), 0.7f).kind(K_LEAF));
        Rng r(60 + b);
        int k = 0;
        for (float x = x0 + 0.34f; x < x1 - 0.25f; x += 0.60f) { Push p(xfT(x, FL + 0.62f, ze - 0.29f) * xfRY(r.u() * 6)); bushBall(0.31f, 0.25f + 0.07f * r.u(), 0.25f, 70 + b * 40 + k++); }
    }
}

// ---------------------------------------------------------------- assemble the world
static void buildWorld() {
    G.out = &STATIC_TRIS;
    makeMaterials();
    makeMachineMaterials();
    M_DECAL = mat(V3(1, 1, 1), 0.5f).kind(K_ATLAS, 1.0f);
    paintSigns();
    makePrototypes();
    makeMachineProtos();
    runPhysics(false);
    buildHall();
    buildMaskingArt();
    for (int i = 0; i < NLANES; i++) buildMachine(i);
    buildBallReturns();
    buildSettees();
    buildBackOfHouse();
    buildLobby();
    buildExterior();
    {   // second pin-deck tube behind the sweep's plane on the hero lane: it carries the deck while the
        // dropping (or rising) rake board passes through the main tube's beam, which otherwise reads as a 3-frame blackout
        int a = addAnim([](double t) { float sy = sweepY(t); return 0.74f * smooth(0.60f, 0.47f, sy) * smooth(0.22f, 0.40f, sy); });
        V3 ln = norm(V3(0, -0.75f, 0.66f));
        rectLight(V3(laneX(HERO), MASK_Y - 0.075f, MASK_Z + 0.46f), V3(0.62f, 0, 0), cross(ln, V3(1, 0, 0)) * 0.035f, V3(52, 50, 44), false, false, a);
    }
    addHeroMachineDynamics();
}
