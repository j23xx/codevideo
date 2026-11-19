// world.h — the lane hall: lanes, gutters, pin decks, masking units, shell and lighting.
#pragma once
#include "layout.h"
#include "scene.h"

struct Mats {
    int lane, approach, gutter, flatGutter, tailPlank, divider, chromeTrim, kick, pitBelt, pin, ball, ceil, wallPaint, wallWood, rubber, steel, steelDark, machineGreen,
        chrome, black, white, concrete, block, carpet, tile, cream, turq, orange, mustard, red, walnut, formica, brass, glass, vinylRed, vinylTurq,
        maskPanel, flake, plasticWhite, darkWood;
};
static Mats MT;
static int P_PIN = -1, P_BALL = -1;

static void makeMaterials() {
    MT.lane = mat(V3(0.7f, 0.5f, 0.3f), 0.15f).kind(K_LANE).spec(1.0f);
    MT.approach = mat(V3(0.7f, 0.5f, 0.3f), 0.25f).kind(K_APPROACH).spec(1.0f);
    MT.gutter = mat(rgb(0x5a5d5c), 0.38f).kind(K_GUTTER);
    MT.divider = mat(rgb(0x3b2416), 0.3f).kind(K_WOOD, 0.2f, 2, 0.8f);
    MT.chromeTrim = mat(V3(0.86f, 0.86f, 0.88f), 0.14f).metal().kind(K_CHROME);
    MT.kick = mat(rgb(0x3a1d14), 0.45f).kind(K_GUTTER);
    MT.pitBelt = mat(rgb(0x3d4f42), 0.9f).kind(K_BELT, 0.55f);
    MT.flatGutter = mat(rgb(0xa8a294), 0.42f).kind(K_GUTTER);
    MT.tailPlank = mat(rgb(0xd8d0b8), 0.5f).kind(K_GUTTER);
    MT.pin = mat(V3(0.9f, 0.9f, 0.85f), 0.18f).kind(K_PIN).spec(1.1f);
    MT.ball = mat(V3(0.02f, 0.02f, 0.02f), 0.07f).kind(K_BALL).spec(1.2f);
    MT.ceil = mat(rgb(0xe6e0d2), 1.0f).kind(K_CEIL);
    MT.wallPaint = mat(rgb(0xd9cfb4), 0.85f).kind(K_STUCCO);
    MT.wallWood = mat(rgb(0x6e4526), 0.35f).kind(K_WOOD, 0.13f, 0, 1.0f);
    MT.darkWood = mat(rgb(0x4a2c18), 0.3f).kind(K_WOOD, 0.3f, 1, 0.9f);
    MT.rubber = mat(V3(0.02f, 0.02f, 0.02f), 0.75f);
    MT.steel = mat(V3(0.55f, 0.55f, 0.56f), 0.4f).metal().kind(K_CHROME);
    MT.steelDark = mat(rgb(0x2a2c2e), 0.5f).kind(K_METAL, 1.0f);
    MT.machineGreen = mat(rgb(0x5f7a6a), 0.45f).kind(K_METAL, 1.0f);
    MT.chrome = mat(V3(0.9f, 0.9f, 0.92f), 0.06f).metal().kind(K_CHROME);
    MT.black = mat(V3(0.015f, 0.015f, 0.015f), 0.6f);
    MT.white = mat(V3(0.85f, 0.85f, 0.82f), 0.5f);
    MT.plasticWhite = mat(V3(0.85f, 0.84f, 0.78f), 0.25f);
    MT.concrete = mat(rgb(0x8a8780), 0.8f).kind(K_CONCRETE);
    MT.block = mat(rgb(0xb9b6a8), 0.8f).kind(K_BLOCK);
    MT.carpet = mat(rgb(0x5a1118), 1.0f).kind(K_CARPET).spec(0.0f);
    MT.tile = mat(rgb(0xd8cdb0), 0.22f).kind(K_TILE, 0.2286f, 0.10f, 0.33f, 0.32f);
    MT.cream = mat(rgb(0xe9dfc3), 0.4f);
    MT.turq = mat(rgb(0x2fa39b), 0.35f);
    MT.orange = mat(rgb(0xe0641c), 0.35f);
    MT.mustard = mat(rgb(0xd49a17), 0.4f);
    MT.red = mat(rgb(0xb5201a), 0.35f);
    MT.walnut = mat(rgb(0x5c381f), 0.3f).kind(K_WOOD, 0.5f, 1, 1.2f);
    MT.formica = mat(rgb(0xe3dcc6), 0.18f).kind(K_FORMICA, 0, 0.18f, 0.62f, 0.60f);
    MT.brass = mat(rgb(0xc9a24a), 0.25f).metal();
    MT.glass = mat(V3(0.92f, 0.96f, 0.95f), 0.02f).glass(0.9f);
    MT.vinylRed = mat(rgb(0xa5181a), 0.35f).kind(K_VINYL);
    MT.vinylTurq = mat(rgb(0x1f8f8a), 0.35f).kind(K_VINYL);
    MT.maskPanel = mat(rgb(0xe9e2cc), 0.4f);
    MT.flake = mat(rgb(0xc03020), 0.3f).kind(K_FLAKE);
}

// ------------------------------------------------------------------ prototypes
static std::vector<V2> pinProfile() {
    // USBC pin outline: (height in, diameter in)
    static const float k[][2] = {{0, 2.031f}, {0.75f, 2.828f}, {2.25f, 3.906f}, {3.375f, 4.510f}, {4.5f, 4.766f}, {5.875f, 4.563f},
                                 {7.25f, 3.703f}, {8.625f, 2.472f}, {9.375f, 1.965f}, {10.0f, 1.797f}, {10.875f, 1.870f}, {11.75f, 2.094f},
                                 {12.625f, 2.406f}, {13.5f, 2.547f}};
    std::vector<V2> key;
    key.push_back(V2(0.0f, 0.0f));
    key.push_back(V2(0.9f * INCH, 0.0f));
    key.push_back(V2(1.0155f * INCH, 0.004f));
    for (int i = 1; i < 14; i++) key.push_back(V2(k[i][1] * 0.5f * INCH, k[i][0] * INCH));
    // domed top: radius 1.273 in centred at 13.727 in
    for (int i = 1; i <= 8; i++) {
        float a = PI / 2 * i / 8;
        key.push_back(V2(1.2735f * INCH * std::cos(a), (13.5f + 0.22f + 1.28f * std::sin(a)) * INCH));
    }
    std::vector<V2> pr;
    pr.push_back(key[0]); pr.push_back(key[1]);
    std::vector<V2> body(key.begin() + 2, key.end());
    std::vector<V2> sm = smoothProfile(body, 4);
    pr.insert(pr.end(), sm.begin(), sm.end());
    pr.back().x = 0;
    return pr;
}
static float pinRadiusAt(float y) { // for physics & placement
    static std::vector<V2> pr = pinProfile();
    for (size_t i = 2; i + 1 < pr.size(); i++)
        if (y >= pr[i].y && y <= pr[i + 1].y) {
            float t = (y - pr[i].y) / std::max(pr[i + 1].y - pr[i].y, 1e-6f);
            return lerp(pr[i].x, pr[i + 1].x, t);
        }
    return 0;
}

static void makePrototypes() {
    PROTOS.reserve(512);
    {
        ProtoScope ps; P_PIN = ps.id;
        PushMat pm(MT.pin);
        lathe(pinProfile(), 40);
    }
    {
        ProtoScope ps; P_BALL = ps.id;
        PushMat pm(MT.ball);
        sphere(V3(0, 0, 0), BALL_R, 56);
    }
}

// a static copy of a prototype baked into the world (for things that never move)
static void bake(int proto, const Xf &x, float var = 0) {
    for (TriC t : PROTOS[proto].tris) {
        for (int k = 0; k < 3; k++) { t.p[k] = (G.xf * x).pt(t.p[k]); t.n[k] = (G.xf * x).dir(t.n[k]); }
        if (var != 0) t.var = var;
        G.out->push_back(t);
    }
}

// ------------------------------------------------------------------ the lanes
static void buildLane(int li) {
    float cx = laneX(li);
    float hw = LANE_W * 0.5f;
    {   // lane bed
        PushMat pm(MT.lane, cx);
        quad(V3(cx - hw, 0, 0), V3(cx - hw, 0, DECK_END), V3(cx + hw, 0, DECK_END), V3(cx + hw, 0, 0));
        // tail plank face down into the pit
        PushMat pmt(MT.tailPlank, (float)li);
        quad(V3(cx - hw, 0, DECK_END), V3(cx - hw, PIT_Y - 0.05f, DECK_END), V3(cx + hw, PIT_Y - 0.05f, DECK_END), V3(cx + hw, 0, DECK_END));
    }
    for (int s = -1; s <= 1; s += 2) {
        float gx = cx + s * (hw + GUT_W * 0.5f);
        PushMat pm(MT.gutter, (float)li);
        float d = 0.047f, ghw = GUT_W * 0.5f;
        float R = (ghw * ghw + d * d) / (2 * d);
        // round gutter from the foul line to the pin deck
        surf(10, 1, [&](float u, float v) {
            float x = (u * 2 - 1) * ghw;
            float y = -(std::sqrt(R * R - x * x) - (R - d));
            return V3(gx + x, y, v * KICK_Z0);
        });
        // transition down into the flat gutter beside the deck
        float fy = -0.085f;
        float xin = cx + s * hw, xout = cx + s * (hw + GUT_W + (DIV_W - 0.14f) * 0.5f);
        surf(10, 4, [&](float u, float v) {
            float x = (u * 2 - 1) * ghw;
            float y0 = -(std::sqrt(R * R - x * x) - (R - d));
            float t = smooth(0, 1, v);
            float xa = gx + x, xb = lerp(xin, xout, u);
            if (s < 0) xb = lerp(xout, xin, u);
            return V3(lerp(xa, xb, t), lerp(y0, fy, t), KICK_Z0 + v * 0.25f);
        });
        float x0 = std::min(xin, xout), x1 = std::max(xin, xout);
        PushMat pmf(MT.flatGutter, (float)li);
        quad(V3(x0, fy, KICK_Z0 + 0.25f), V3(x0, fy, DECK_END), V3(x1, fy, DECK_END), V3(x1, fy, KICK_Z0 + 0.25f));
        // lane edge down to the flat gutter
        {
            PushMat pm2(MT.tailPlank, (float)li + 0.5f);
            quad(V3(xin, 0, KICK_Z0), V3(xin, fy, KICK_Z0), V3(xin, fy, DECK_END), V3(xin, 0, DECK_END));
        }
        quad(V3(x0, fy, DECK_END), V3(x0, PIT_Y - 0.05f, DECK_END), V3(x1, PIT_Y - 0.05f, DECK_END), V3(x1, fy, DECK_END));
    }
    {   // pit floor: the moving carpet
        PushMat pm(MT.pitBelt);
        float x0 = cx - PITCH * 0.5f + 0.07f, x1 = cx + PITCH * 0.5f - 0.07f;
        quad(V3(x0, PIT_Y, DECK_END), V3(x0, PIT_Y, 20.85f), V3(x1, PIT_Y, 20.85f), V3(x1, PIT_Y, DECK_END));
    }
    for (int s = -1; s <= 1; s += 2) {   // soft fill from the kickbacks (no visible fitting), so the pit reads as a pit
        float x = cx + s * (PITCH * 0.5f - 0.07f - 0.012f);
        rectLight(V3(x, 0.42f, 19.55f), V3(0, 0, 0.26f), V3(s * 0.006f, -s * 0.014f, 0) * -1.0f, V3(36, 33, 25), false, false);
    }
}

static void buildDividers() {
    for (int i = 0; i <= NLANES; i++) {
        float dx = laneX(0) - PITCH * 0.5f + i * PITCH;
        {   // capping between gutters
            PushMat pm(MT.divider, (float)i);
            rbox(V3(dx - DIV_W * 0.5f, -0.06f, 0.0f), V3(dx + DIV_W * 0.5f, 0.028f, KICK_Z0 + 0.02f), 0.012f);
        }
        {
            PushMat pm(MT.chromeTrim);
            rbox(V3(dx - 0.02f, 0.027f, 0.01f), V3(dx + 0.02f, 0.033f, KICK_Z0), 0.002f);
        }
        {   // kickback (side board of the pin deck and pit)
            PushMat pm(MT.kick, (float)i + 20);
            rbox(V3(dx - 0.07f, -0.2f, KICK_Z0), V3(dx + 0.07f, 0.62f, 20.66f), 0.008f);
        }
    }
}

static void buildHall() {
    // approach: one continuous floor of boards
    {
        PushMat pm(MT.approach, laneX(HERO));
        quad(V3(-HALL_X, 0, APP_Z), V3(-HALL_X, 0, 0), V3(HALL_X, 0, 0), V3(HALL_X, 0, APP_Z));
    }
    for (int i = 0; i < NLANES; i++) buildLane(i);
    buildDividers();
    float lx0 = laneX(0) - PITCH * 0.5f, lx1 = laneX(NLANES - 1) + PITCH * 0.5f;
    {   // side strips between the outer lanes and the walls
        PushMat pm(MT.carpet);
        quad(V3(-HALL_X, 0.03f, 0), V3(-HALL_X, 0.03f, MASK_Z), V3(lx0 - 0.07f, 0.03f, MASK_Z), V3(lx0 - 0.07f, 0.03f, 0));
        quad(V3(lx1 + 0.07f, 0.03f, 0), V3(lx1 + 0.07f, 0.03f, MASK_Z), V3(HALL_X, 0.03f, MASK_Z), V3(HALL_X, 0.03f, 0));
        quad(V3(-HALL_X, 0, 0), V3(-HALL_X, 0.03f, 0), V3(lx0, 0.03f, 0), V3(lx0, 0, 0));
        quad(V3(lx1, 0, 0), V3(lx1, 0.03f, 0), V3(HALL_X, 0.03f, 0), V3(HALL_X, 0, 0));
    }
    {   // ceiling over the lanes
        PushMat pm(MT.ceil);
        quad(V3(-HALL_X, CEIL_Y, APP_Z - 0.6f), V3(HALL_X, CEIL_Y, APP_Z - 0.6f), V3(HALL_X, CEIL_Y, BACK_Z), V3(-HALL_X, CEIL_Y, BACK_Z));
    }
    {   // side walls: wood wainscot, painted above
        for (int s = -1; s <= 1; s += 2) {
            float x = s * HALL_X;
            {
                PushMat pm(MT.wallWood, (float)s);
                quad(V3(x, 0, STEP_Z), V3(x, 0, MASK_Z + 0.3f), V3(x, 1.25f, MASK_Z + 0.3f), V3(x, 1.25f, STEP_Z));
            }
            {
                PushMat pm(MT.chromeTrim);
                box(V3(x - 0.02f, 1.25f, STEP_Z), V3(x + 0.02f, 1.29f, MASK_Z + 0.3f));
            }
            {
                PushMat pm(MT.wallPaint);
                quad(V3(x, 1.25f, STEP_Z), V3(x, 1.25f, MASK_Z + 0.3f), V3(x, 3.3f, MASK_Z + 0.3f), V3(x, 3.3f, STEP_Z));
            }
            {
                PushMat pm(MT.block);
                quad(V3(x, -0.2f, MASK_Z + 0.3f), V3(x, -0.2f, BACK_Z), V3(x, CEIL_Y, BACK_Z), V3(x, CEIL_Y, MASK_Z + 0.3f));
            }
        }
        PushMat pm(MT.block);
        quad(V3(-HALL_X, -0.2f, BACK_Z), V3(HALL_X, -0.2f, BACK_Z), V3(HALL_X, CEIL_Y, BACK_Z), V3(-HALL_X, CEIL_Y, BACK_Z));
    }
    {   // service-aisle floor behind the machines, and the slab under the pits
        PushMat pm(MT.concrete);
        quad(V3(-HALL_X, -0.2f, MASK_Z), V3(-HALL_X, -0.2f, BACK_Z), V3(HALL_X, -0.2f, BACK_Z), V3(HALL_X, -0.2f, MASK_Z));
    }
    // masking units: one panel per lane with a light hood along the bottom
    for (int i = 0; i < NLANES; i++) {
        float cx = laneX(i);
        {
            PushMat pm(MT.maskPanel, (float)i);
            rbox(V3(cx - PITCH * 0.5f + 0.01f, MASK_Y, MASK_Z), V3(cx + PITCH * 0.5f - 0.01f, 2.2f, MASK_Z + 0.06f), 0.01f);
        }
        {   // hood housing the pin-deck light
            PushMat pm(MT.steelDark);
            box(V3(cx - PITCH * 0.5f + 0.01f, MASK_Y, MASK_Z + 0.06f), V3(cx + PITCH * 0.5f - 0.01f, MASK_Y + 0.02f, MASK_Z + 0.42f));
            box(V3(cx - PITCH * 0.5f + 0.01f, MASK_Y - 0.07f, MASK_Z + 0.40f), V3(cx + PITCH * 0.5f - 0.01f, MASK_Y + 0.02f, MASK_Z + 0.42f));
        }
        // fluorescent tube aimed at the pins
        V3 lc(cx, MASK_Y - 0.035f, MASK_Z + 0.2f);
        V3 ln = norm(V3(0, -0.75f, 0.66f));
        V3 lu(0.62f, 0, 0), lv = cross(ln, V3(1, 0, 0)) * 0.035f;
        rectLight(lc, lu, lv, V3(52, 50, 44));
    }
    {   // soffit above the masking units up to the ceiling
        PushMat pm(MT.wallWood, 7);
        quad(V3(-HALL_X, 2.2f, MASK_Z), V3(HALL_X, 2.2f, MASK_Z), V3(HALL_X, CEIL_Y, MASK_Z), V3(-HALL_X, CEIL_Y, MASK_Z));
    }
    // lane lighting: rows of recessed troffers, dimmer toward the pins
    for (int r = 0; r < 4; r++) {
        float z = -2.4f + r * 5.4f;
        float e = r == 0 ? 15.0f : 6.0f;
        for (int p = 0; p < NLANES / 2; p++) {
            float x = (laneX(2 * p) + laneX(2 * p + 1)) * 0.5f;
            rectLight(V3(x, CEIL_Y - 0.004f, z), V3(0.6f, 0, 0), V3(0, 0, 0.15f), V3(e, e * 0.93f, e * 0.78f));
        }
    }
    // pins on every lane except the hero lane (those are simulated)
    for (int i = 0; i < NLANES; i++) {
        if (i == HERO) continue;
        for (int k = 0; k < 10; k++) {
            V3 p = pinSpot(k);
            bake(P_PIN, xfT(laneX(i) + p.x, 0, p.z) * xfRY(hash1(i * 31.0f + k) * 6.28f), i * 10.0f + k + 1);
        }
    }
}


