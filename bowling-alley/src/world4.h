// world4.h — things outside the building: early-60s cars, palms, utility poles and the far backdrop.
#pragma once
#include "world2.h"

struct CarStyle {
    V3 paint, roof;      // body colour, roof colour (two-tone when different)
    V3 trimIn;           // upholstery
    float L;             // overall length
    float fin;           // height of the tail fins
    bool wagon;
    float wall;          // whitewall width (0: blackwall)
};

// one wheel at the origin, axle along x, outer face toward s * x
static void carWheel(float s, float wall) {
    const float hw = 0.095f;
    Push p(xfRZ(-s * PI / 2));
    { PushMat pm(MT.rubber);
      lathe({V2(0.0f, -hw), V2(0.29f, -hw), V2(0.322f, -hw + 0.018f), V2(0.335f, -hw + 0.05f), V2(0.335f, hw - 0.05f), V2(0.322f, hw - 0.018f),
             V2(0.295f, hw), V2(0.20f, hw - 0.004f)}, 28); }
    if (wall > 0) { PushMat pm(mat(V3(0.80f, 0.79f, 0.74f), 0.55f)); lathe({V2(0.262f - wall, hw + 0.0015f), V2(0.262f, hw + 0.0015f)}, 28); }
    { PushMat pm(MT.chrome);
      lathe(smoothProfile({V2(0.0f, hw + 0.040f), V2(0.045f, hw + 0.038f), V2(0.075f, hw + 0.022f), V2(0.135f, hw + 0.020f), V2(0.172f, hw + 0.006f),
                           V2(0.192f, hw - 0.006f), V2(0.203f, hw - 0.012f)}, 3), 28); }
}

static void car2(const CarStyle &cs) {
    const float L = cs.L, hl = L * 0.5f, W = 0.985f;
    const float zf = 1.60f * L / 5.3f, zr = -1.46f * L / 5.3f;      // axles
    const float WY = 0.335f, RAF = 0.42f, RAR = 0.405f;
    const float zc = zf - 0.70f;                                    // base of the windscreen
    const float zrw = cs.wagon ? -hl + 0.16f : zr + 0.02f;          // base of the rear window
    const float ybelt = 0.915f, yroof = 1.365f;
    const float wb = W - 0.075f, wr = W - 0.25f;
    const float zwt = zc - 0.43f, zrt = cs.wagon ? zrw + 0.22f : zrw + 0.46f;   // top edges of the two screens
    int paint = mat(cs.paint, 0.13f).spec(1.6f);
    int paintRoof = mat(cs.roof, 0.13f).spec(1.6f);
    int trim = mat(cs.trimIn, 0.5f).kind(K_VINYL);
    int glass = mat(V3(0.34f, 0.42f, 0.40f), 0.02f).glass(0.9f);
    int lens = mat(V3(0.78f, 0.78f, 0.74f), 0.06f).spec(2.2f);
    int redLens = mat(V3(0.42f, 0.015f, 0.012f), 0.10f).spec(2.0f).emit(V3(0.35f, 0.01f, 0.008f), 3);

    auto halfW = [&](float z) { return W * (1.0f - 0.055f * smooth(hl - 1.3f, hl, std::fabs(z))); };
    auto lowY = [&](float z) {
        float y = 0.235f + 0.05f * smooth(hl - 0.55f, hl, std::fabs(z));
        float df = z - zf, dr = z - zr;
        if (std::fabs(df) < RAF) y = std::max(y, WY + std::sqrt(RAF * RAF - df * df));
        if (std::fabs(dr) < RAR) y = std::max(y, WY + std::sqrt(RAR * RAR - dr * dr) - 0.03f);
        return y;
    };
    // half cross-section from the centre line over the shoulder down to the sill: (x, y)
    auto halfProf = [&](float z) {
        float w = halfW(z);
        bool cabin = z < zc - 0.02f && z > zrw + 0.02f;
        float fend = 0.885f + 0.03f * smooth(hl, zc, z) - 0.015f * smooth(zrw, -hl, z);
        float fin = cs.fin * smooth(zr + 0.5f, -hl + 0.2f, z);
        float yc = z > 0 ? 0.905f + 0.03f * smooth(hl, zc, z) : 0.895f - 0.03f * smooth(zrw, -hl, z);
        float sag = 0.03f * smooth(hl - 0.25f, hl, std::fabs(z));          // the ends roll over a little
        if (cabin) yc = 0.60f;
        std::vector<V2> k = {V2(0, yc - sag), V2(0.50f * w, (cabin ? 0.60f : yc - 0.004f) - sag), V2(0.84f * w, (cabin ? 0.60f : fend + 0.012f + fin * 0.4f) - sag),
                             V2(0.955f * w, fend - 0.022f + fin - sag), V2(0.995f * w, fend - 0.13f + fin * 0.5f), V2(w, 0.64f), V2(w - 0.012f, 0.47f), V2(w - 0.04f, 0.235f)};
        std::vector<V2> p = smoothProfile(k, 4);
        if (cabin) for (auto &q : p) if (q.x < 0.86f * w) q.y = std::min(q.y, 0.60f);
        float yl = lowY(z);
        for (auto &q : p) if (q.y < yl && q.x > 0.8f * w) q.y = yl;
        return p;
    };
    auto sect = [&](float z, float v) {     // v: 0 = left sill, 0.5 = centre line, 1 = right sill
        std::vector<V2> p = halfProf(z);
        float t = std::fabs(v * 2 - 1) * (p.size() - 1);
        int i = std::min((int)t, (int)p.size() - 2);
        V2 q = p[i] + (p[i + 1] - p[i]) * (t - i);
        return V3(v < 0.5f ? -q.x : q.x, q.y, z);
    };
    {   // body shell
        PushMat pm(paint);
        int nu = (int)(L / 0.035f);
        surf(nu, 56, [&](float u, float v) { return sect(lerp(-hl, hl, u), v); });
        // flat nose and tail panels
        for (int e = -1; e <= 1; e += 2)
            surf(1, 28, [&](float u, float v) { V3 a = sect(e * hl, v * 0.5f), b = sect(e * hl, 1 - v * 0.5f); return lerp(a, b, u); });
    }
    {   // underside, wheel housings
        PushMat pm(MT.black);
        box(V3(-W + 0.13f, 0.19f, -hl + 0.2f), V3(W - 0.13f, 0.58f, hl - 0.2f));
    }
    for (int s = -1; s <= 1; s += 2) {
        { Push p(xfT(s * (W - 0.135f), WY, zf)); carWheel((float)s, cs.wall); }
        { Push p(xfT(s * (W - 0.135f), WY, zr)); carWheel((float)s, cs.wall); }
    }
    // ---------------- greenhouse
    auto roofPt = [&](float u, float v) {      // u along the car, v across
        float x = (v * 2 - 1), z = lerp(zrt, zwt, u), sz = u * 2 - 1;
        float x4 = x * x * x * x, s6 = sz * sz * sz * sz * sz * sz;
        return V3(x * wr * (1.0f - 0.03f * s6), yroof + 0.055f * (1 - x4) - 0.018f * s6, z + 0.05f * sz * (1 - x * x));
    };
    {
        PushMat pm(paintRoof);
        surf(16, 14, roofPt);
        {   // rolled edge all round the roof panel
            std::vector<V3> e;
            for (int i = 0; i <= 12; i++) e.push_back(roofPt(i / 12.0f, 0) + V3(0, -0.012f, 0));
            for (int i = 1; i <= 8; i++) e.push_back(roofPt(1, i / 8.0f) + V3(0, -0.012f, 0));
            for (int i = 11; i >= 0; i--) e.push_back(roofPt(i / 12.0f, 1) + V3(0, -0.012f, 0));
            for (int i = 7; i >= 0; i--) e.push_back(roofPt(0, i / 8.0f) + V3(0, -0.012f, 0));
            tube(e, 0.024f, 8, false);
        }
        // pillars
        for (int s = -1; s <= 1; s += 2) {
            tube({V3(s * wb, ybelt, zc + 0.01f), V3(s * wr, yroof, zwt)}, 0.026f, 8);                                   // A
            if (!cs.wagon) {
                surf(1, 4, [&](float u, float v) {                                                                      // wide C
                    V3 top = V3(s * (wr + 0.004f), yroof, lerp(zrt, zrt + 0.30f, v)), bot = V3(s * (wb + 0.004f), ybelt, lerp(zrw + 0.03f, zrw + 0.46f, v));
                    return lerp(bot, top, u);
                });
                tube({V3(s * wb, ybelt, zc - 1.03f), V3(s * wr, yroof, zc - 1.06f)}, 0.017f, 6);                        // B
            } else {
                tube({V3(s * wb, ybelt, zrw + 0.02f), V3(s * wr, yroof, zrt)}, 0.028f, 8);
                tube({V3(s * wb, ybelt, zc - 1.03f), V3(s * wr, yroof, zc - 1.06f)}, 0.017f, 6);
                tube({V3(s * wb, ybelt, zr + 0.25f), V3(s * wr, yroof, zr + 0.22f)}, 0.020f, 6);
            }
        }
        PushMat pm2(mat(V3(0.72f, 0.70f, 0.62f), 0.9f));       // headliner
        surf(4, 4, [&](float u, float v) { return roofPt(0.04f + u * 0.92f, 0.04f + v * 0.92f) - V3(0, 0.03f, 0); });
    }
    {
        PushMat pm(glass);
        G.flags |= F_NOSHADOW;
        surf(1, 8, [&](float u, float v) { float x = v * 2 - 1; V3 b(x * wb, ybelt, zc + 0.09f * (1 - x * x)), t(x * wr, yroof, zwt + 0.07f * (1 - x * x)); return lerp(b, t, u); });
        surf(1, 8, [&](float u, float v) { float x = v * 2 - 1; V3 b(x * wb, ybelt, zrw - 0.08f * (1 - x * x)), t(x * wr, yroof, zrt - 0.06f * (1 - x * x)); return lerp(b, t, u); });
        for (int s = -1; s <= 1; s += 2) {
            float zb = cs.wagon ? zrw + 0.03f : zrw + 0.46f, zt = cs.wagon ? zrt : zrt + 0.30f;
            quad(V3(s * wb, ybelt, zb), V3(s * wb, ybelt, zc), V3(s * wr, yroof, zwt), V3(s * wr, yroof, zt));
        }
    }
    {   // bright trim round the glass, drip rails, belt moulding, wipers
        PushMat pm(MT.chrome);
        for (int s = -1; s <= 1; s += 2) {
            std::vector<V3> rail;
            for (int i = 0; i <= 10; i++) rail.push_back(roofPt(i / 10.0f, s > 0 ? 1.0f : 0.0f) + V3(s * 0.006f, -0.004f, 0));
            tube(rail, 0.011f, 6);
            tube({V3(s * (wb + 0.012f), ybelt + 0.004f, zrw), V3(s * (wb + 0.012f), ybelt + 0.004f, zc)}, 0.009f, 6);
        }
        std::vector<V3> fr, rr;
        for (int i = 0; i <= 10; i++) { float x = i / 5.0f - 1; fr.push_back(V3(x * wb, ybelt + 0.004f, zc + 0.012f + 0.09f * (1 - x * x))); rr.push_back(V3(x * wb, ybelt + 0.004f, zrw - 0.012f - 0.08f * (1 - x * x))); }
        tube(fr, 0.010f, 6); tube(rr, 0.010f, 6);
        PushMat pm2(MT.black);
        for (int s = -1; s <= 1; s += 2) tube({V3(s * 0.22f - 0.18f, ybelt + 0.012f, zc + 0.085f), V3(s * 0.22f + 0.2f, ybelt + 0.05f, zc + 0.04f)}, 0.005f, 5);
    }
    {   // interior: floor, seats, dash, wheel
        { PushMat pm(MT.black); box(V3(-wb + 0.02f, 0.59f, zrw + 0.06f), V3(wb - 0.02f, 0.63f, zc - 0.05f)); }
        PushMat pm(trim);
        for (int s = -1; s <= 1; s += 2) box(V3(s * (wb - 0.035f) - 0.012f, 0.62f, zrw + 0.10f), V3(s * (wb - 0.035f) + 0.012f, ybelt - 0.012f, zc - 0.06f));
        float z1 = zc - 0.62f;                                    // front bench
        rbox(V3(-wb + 0.07f, 0.63f, z1 - 0.50f), V3(wb - 0.07f, 0.80f, z1), 0.04f);
        { Push p(xfT(0, 0.74f, z1 - 0.52f) * xfRX(-0.22f)); rbox(V3(-wb + 0.07f, 0, -0.07f), V3(wb - 0.07f, 0.43f, 0.07f), 0.045f); }
        float z2 = (cs.wagon ? zr + 0.55f : zrw + 0.66f);         // rear bench
        rbox(V3(-wb + 0.07f, 0.63f, z2 - 0.48f), V3(wb - 0.07f, 0.79f, z2), 0.04f);
        { Push p(xfT(0, 0.73f, z2 - 0.50f) * xfRX(-0.25f)); rbox(V3(-wb + 0.07f, 0, -0.07f), V3(wb - 0.07f, 0.41f, 0.07f), 0.045f); }
        { PushMat pm2(mat(cs.paint * 0.6f, 0.3f)); rbox(V3(-wb + 0.03f, 0.74f, zc - 0.30f), V3(wb - 0.03f, 0.91f, zc - 0.02f), 0.04f); }
        { PushMat pm2(MT.chrome); cyl(V3(-wb + 0.1f, 0.86f, zc - 0.305f), V3(wb - 0.1f, 0.86f, zc - 0.305f), 0.008f, 0.008f, 6); }
        {   // steering wheel on the left (+x) side
            V3 hub(0.40f, 0.93f, zc - 0.50f), ax = norm(V3(0, 0.42f, -1));
            { PushMat pm2(mat(V3(0.75f, 0.73f, 0.66f), 0.3f)); torus(hub, ax, 0.195f, 0.012f, 26, 6); cyl(hub - ax * 0.04f, hub, 0.035f, 0.03f, 10); }
            PushMat pm2(MT.chrome);
            cyl(hub, hub - ax * 0.42f, 0.016f, 0.02f, 8);
            V3 sx = norm(cross(ax, V3(0, 1, 0)));
            tube({hub - sx * 0.19f, hub - ax * 0.03f, hub + sx * 0.19f}, 0.006f, 5);
        }
    }
    // ---------------- brightwork, lamps
    auto bumper = [&](float e) {
        float zb = e * (hl + 0.085f);
        PushMat pm(MT.chrome);
        for (float y : {0.335f, 0.405f}) {
            float r = y < 0.37f ? 0.050f : 0.058f;
            tube(smoothPath({V3(-W - 0.02f, y, e * (hl - 0.52f)), V3(-W - 0.035f, y, e * (hl - 0.12f)), V3(-W + 0.12f, y, zb), V3(0, y, zb + e * 0.012f), V3(W - 0.12f, y, zb),
                             V3(W + 0.035f, y, e * (hl - 0.12f)), V3(W + 0.02f, y, e * (hl - 0.52f))}, 6), r, 10);
        }
        for (int s = -1; s <= 1; s += 2) { Push p(xfT(s * 0.33f, 0.30f, zb + e * 0.03f)); rbox(V3(-0.035f, 0, -0.04f), V3(0.035f, 0.21f, 0.04f), 0.025f); }
        // number plate
        PushMat pm2(mat(e > 0 ? V3(0.80f, 0.62f, 0.10f) : V3(0.06f, 0.07f, 0.20f), 0.4f));
        box(V3(-0.155f, 0.30f, zb + e * 0.052f), V3(0.155f, 0.455f, zb + e * 0.062f));
        PushMat pm3(paint);
        box(V3(-W + 0.10f, 0.215f, e * (hl - 0.30f)), V3(W - 0.10f, 0.30f, e * (hl + 0.02f)));     // valance
    };
    bumper(1); bumper(-1);
    {   // grille and four headlamps
        float z = hl;
        { PushMat pm(MT.black); box(V3(-W + 0.10f, 0.47f, z - 0.01f), V3(W - 0.10f, 0.76f, z + 0.012f)); }
        PushMat pm(MT.chrome);
        for (int i = 0; i < 8; i++) { float y = 0.492f + i * 0.036f; box(V3(-0.52f, y - 0.006f, z + 0.012f), V3(0.52f, y + 0.006f, z + 0.03f)); }
        for (int i = -6; i <= 6; i++) box(V3(i * 0.08f - 0.004f, 0.48f, z + 0.010f), V3(i * 0.08f + 0.004f, 0.755f, z + 0.024f));
        rbox(V3(-W + 0.08f, 0.455f, z + 0.0f), V3(W - 0.08f, 0.478f, z + 0.045f), 0.008f);
        rbox(V3(-W + 0.08f, 0.752f, z + 0.0f), V3(W - 0.08f, 0.775f, z + 0.05f), 0.008f);
        for (int s = -1; s <= 1; s += 2)
            for (int k = 0; k < 2; k++) {
                Push p(xfT(s * (0.62f + k * 0.185f), 0.615f, z + 0.012f) * xfRX(PI / 2));
                { PushMat pm2(MT.chrome); lathe({V2(0.093f, 0), V2(0.097f, 0.030f), V2(0.088f, 0.040f), V2(0.080f, 0.030f)}, 20); }
                PushMat pm2(lens); lathe(smoothProfile({V2(0.080f, 0.026f), V2(0.062f, 0.040f), V2(0.03f, 0.048f), V2(0, 0.050f)}, 3), 20);
            }
    }
    {   // tail lamps, boot lock, exhaust
        float z = -hl;
        for (int s = -1; s <= 1; s += 2)
            for (int k = 0; k < (cs.wagon ? 1 : 3); k++) {
                Push p(xfT(s * (0.50f + k * 0.165f), 0.70f, z - 0.004f) * xfRX(-PI / 2));
                { PushMat pm2(MT.chrome); lathe({V2(0.074f, 0), V2(0.078f, 0.022f), V2(0.068f, 0.030f), V2(0.062f, 0.022f)}, 18); }
                PushMat pm2(k == 1 && !cs.wagon ? lens : redLens); lathe(smoothProfile({V2(0.062f, 0.020f), V2(0.045f, 0.040f), V2(0, 0.052f)}, 3), 18);
            }
        PushMat pm(MT.chrome);
        rbox(V3(-W + 0.12f, 0.585f, z - 0.02f), V3(W - 0.12f, 0.605f, z + 0.005f), 0.006f);
        cyl(V3(-0.55f, 0.24f, z + 0.3f), V3(-0.55f, 0.235f, z - 0.06f), 0.028f, 0.028f, 8);
    }
    {   // side spear, sill moulding, door handles, mirror, door shut lines, aerial
        for (int s = -1; s <= 1; s += 2) {
            { PushMat pm(MT.chrome);
              std::vector<V3> sp, sl;
              for (int i = 0; i <= 24; i++) { float z = lerp(-hl + 0.25f, hl - 0.18f, i / 24.0f); sp.push_back(V3(s * (halfW(z) + 0.003f), 0.645f + 0.02f * smooth(zr, -hl, z) * (cs.fin > 0 ? 3 : 0), z)); }
              tube(sp, 0.011f, 6);
              for (int i = 0; i <= 10; i++) { float z = lerp(zr + RAR + 0.03f, zf - RAF - 0.03f, i / 10.0f); sl.push_back(V3(s * (halfW(z) - 0.028f), 0.262f, z)); }
              tube(sl, 0.016f, 6);
              for (float z : {zc - 0.95f, (cs.wagon ? zr + 0.36f : zrw + 0.58f)}) { Push p(xfT(s * (W + 0.006f), 0.80f, z)); rbox(V3(-0.012f, -0.012f, -0.07f), V3(0.012f, 0.012f, 0.07f), 0.01f); }
            }
            PushMat pm(mat(V3(0.01f, 0.01f, 0.01f), 0.6f));
            for (float z : {zc + 0.03f, zc - 1.04f, (cs.wagon ? zr + 0.24f : zrw + 0.50f)}) {
                std::vector<V2> p = halfProf(z);
                std::vector<V3> line;
                for (auto &q : p) if (q.y > 0.27f && q.y < 0.905f && q.x > 0.9f * W * 0.9f) line.push_back(V3(s * (q.x + 0.0005f), q.y, z));
                if (line.size() >= 2) tube(line, 0.0035f, 4, false);
            }
        }
        PushMat pm(MT.chrome);
        { V3 m(W + 0.07f, 0.99f, zc - 0.22f); cyl(V3(W - 0.07f, 0.92f, zc - 0.2f), m, 0.008f, 0.008f, 5); Push p(xfT(m) * xfRX(PI / 2)); lathe(smoothProfile({V2(0, -0.02f), V2(0.05f, -0.01f), V2(0.058f, 0.02f), V2(0, 0.022f)}, 2), 14); }
        tube({V3(-W + 0.14f, 0.90f, zc + 0.35f), V3(-W + 0.16f, 1.75f, zc + 0.12f)}, 0.003f, 4);
        // hood ornament / badge
        rbox(V3(-0.012f, 0.925f, hl - 0.34f), V3(0.012f, 0.955f, hl - 0.10f), 0.008f);
    }
}

// ---------------------------------------------------------------- palms
// lod >= 1: how far away the palm stands, as a multiple of 30 m. Far palms get fewer, broader leaflets so that no
// leaflet is thinner than a pixel (slivers smaller than a pixel sparkle from frame to frame).
static void palm(float h, float lean, uint64_t seed, float lod = 1.0f) {
    Rng r(seed);
    float la = r.r(0, 2 * PI);
    V3 ld(std::cos(la), 0, std::sin(la));
    std::vector<V3> k;
    for (int i = 0; i <= 5; i++) { float t = i / 5.0f; k.push_back(ld * (lean * h * t * t) + V3(0, t * h, 0)); }
    std::vector<V3> path = smoothPath(k, 4);
    {
        PushMat pm(mat(rgb(0x6a5a49), 0.9f).kind(K_VINYL));
        int n = (int)path.size();
        surf(n * 3, 9, [&](float u, float v) {          // trunk with leaf-scar rings, flared at the foot
            float f = u * (n - 1); int i = std::min((int)f, n - 2);
            V3 c = lerp(path[i], path[i + 1], f - i);
            float rad = (0.21f - 0.07f * u + 0.10f * std::exp(-u * h * 2.2f)) * (1.0f + 0.05f * std::sin(u * h * 26.0f));
            float a = v * 2 * PI;
            return c + V3(std::cos(a), 0, std::sin(a)) * rad;
        });
    }
    V3 top = path.back();
    int green = mat(rgb(0x3a5a22), 0.5f).kind(K_VINYL), dead = mat(rgb(0x7a6238), 0.8f).kind(K_VINYL);
    { PushMat pm(dead); sphere(top + V3(0, -0.25f, 0), 0.34f, 10, 1.5f); }
    int nf = 30;
    for (int i = 0; i < nf + 10; i++) {
        bool old = i >= nf;
        float a = i * 2.39996f + r.r(-0.2f, 0.2f);
        float el = old ? r.r(-1.35f, -0.9f) : lerp(1.25f, -0.55f, std::pow((i + 0.5f) / nf, 0.8f)) + r.r(-0.1f, 0.1f);
        float len = old ? r.r(1.6f, 2.3f) : r.r(2.7f, 3.5f);
        float droop = old ? 0.15f : lerp(0.25f, 0.75f, (float)i / nf);
        V3 dh(std::cos(a), 0, std::sin(a));
        auto P = [&](float s) { return top + dh * (s * len * std::cos(el)) + V3(0, s * len * std::sin(el) - droop * s * s * len, 0); };
        PushMat pm(old ? dead : green);
        std::vector<V3> ra;
        for (int j = 0; j <= 7; j++) ra.push_back(P(j / 7.0f));
        tube(ra, 0.028f, 5, false, 0.006f);
        int nl = std::max(5, (int)((old ? 14 : 30) / lod));
        V3 side = cross(dh, V3(0, 1, 0));
        for (int j = 1; j <= nl; j++) {
            float s = (j + 0.3f) / (nl + 1.5f);
            V3 b = P(s), tg = norm(P(s + 0.02f) - P(s - 0.02f));
            float ll = (0.16f + 0.62f * std::pow(std::sin(PI * std::pow(s, 0.8f)), 0.7f)) * (old ? 0.7f : 1.0f);
            for (int sd = -1; sd <= 1; sd += 2) {
                V3 d = norm(side * (sd * 0.80f) + V3(0, old ? -0.9f : -0.42f - 0.3f * s, 0) + tg * 0.35f + V3(r.r(-0.08f, 0.08f), r.r(-0.08f, 0.08f), r.r(-0.08f, 0.08f)));
                V3 tip = b + d * ll, w = tg * (0.022f * lod);
                tri(b - w, b + w, tip);
            }
        }
    }
}

static void bushBall(float rx, float ry, float rz, uint64_t seed) {
    Rng r(seed);
    float p1 = r.r(0, 6), p2 = r.r(0, 6), p3 = r.r(0, 6);
    surf(22, 14, [&](float u, float v) {
        float a = u * 2 * PI, b = (v - 0.5f) * PI;
        V3 d(std::cos(a) * std::cos(b), std::sin(b), std::sin(a) * std::cos(b));
        float n = 0.06f * std::sin(d.x * 9 + p1) * std::sin(d.y * 8 + p2) + 0.05f * std::sin(d.z * 13 + p3) * std::sin(d.x * 11 + p2) + 0.03f * std::sin(d.y * 23 + p1) * std::sin(d.z * 19 + p3);
        float k = 1.0f + n;
        return V3(d.x * rx * k, d.y * ry * k, d.z * rz * k);
    });
}

static void utilityPole(V3 b, float yaw) {
    Push p(xfT(b) * xfRY(yaw));
    PushMat pm(mat(rgb(0x3a2e24), 0.9f));
    cyl(V3(0, 0, 0), V3(0, 9.5f, 0), 0.15f, 0.10f, 8);
    box(V3(-1.2f, 8.7f, -0.05f), V3(1.2f, 8.85f, 0.05f));
    box(V3(-0.8f, 7.9f, -0.05f), V3(0.8f, 8.02f, 0.05f));
    PushMat pm2(mat(V3(0.3f, 0.35f, 0.33f), 0.2f));
    for (float x : {-1.1f, -0.5f, 0.5f, 1.1f}) cyl(V3(x, 8.85f, 0), V3(x, 9.0f, 0), 0.035f, 0.03f, 6);
}
static void wires(V3 a, V3 b, float yaw) {     // four sagging conductors between two pole tops
    PushMat pm(MT.black);
    V3 side(std::cos(yaw), 0, -std::sin(yaw));
    for (float x : {-1.1f, -0.5f, 0.5f, 1.1f}) {
        std::vector<V3> p;
        for (int i = 0; i <= 12; i++) { float t = i / 12.0f; p.push_back(lerp(a, b, t) + side * x + V3(0, 9.0f - 1.3f * 4 * t * (1 - t), 0)); }
        tube(p, 0.03f, 4, false);
    }
}

// everything beyond the lot: a street's worth of low buildings, palms, poles, far hills
static void buildBackdrop() {
    Rng r(4242);
    {   // ground out to the hills
        PushMat pm(mat(rgb(0x3b382f), 0.95f));
        // (laid round the building's footprint, never under it: the pits and gutters inside go below this level)
        const float bx = HALL_X + 0.3f, bz0 = FRONT_Z - 0.3f, bz1 = BACK_Z;
        quad(V3(-900, -0.02f, -900), V3(-900, -0.02f, bz0), V3(900, -0.02f, bz0), V3(900, -0.02f, -900));
        quad(V3(-900, -0.02f, bz1), V3(-900, -0.02f, 900), V3(900, -0.02f, 900), V3(900, -0.02f, bz1));
        quad(V3(-900, -0.02f, bz0), V3(-900, -0.02f, bz1), V3(-bx, -0.02f, bz1), V3(-bx, -0.02f, bz0));
        quad(V3(bx, -0.02f, bz0), V3(bx, -0.02f, bz1), V3(900, -0.02f, bz1), V3(900, -0.02f, bz0));
    }
    {   // hills on the horizon
        PushMat pm(mat(V3(0.050f, 0.055f, 0.065f), 1.0f));
        int n = 96;
        auto hgt = [&](float a) { return 26.0f + 16.0f * std::sin(a * 3.0f + 1.0f) + 9.0f * std::sin(a * 7.0f + 2.0f) + 5.0f * std::sin(a * 17.0f) + 3.0f * std::sin(a * 31.0f + 4.0f); };
        for (int i = 0; i < n; i++) {
            float a0 = i * 2 * PI / n, a1 = (i + 1) * 2 * PI / n;
            V3 d0(std::cos(a0), 0, std::sin(a0)), d1(std::cos(a1), 0, std::sin(a1));
            quad(d0 * 640.0f, d1 * 640.0f, d1 * 720.0f + V3(0, hgt(a1), 0), d0 * 720.0f + V3(0, hgt(a0), 0));
            quad(d0 * 720.0f + V3(0, hgt(a0), 0), d1 * 720.0f + V3(0, hgt(a1), 0), d1 * 800.0f, d0 * 800.0f);
        }
    }
    // low commercial buildings round about (flat roofs, parapets, a few awnings)
    const int cols[6] = {0xB9A88C, 0x9C8F80, 0xC7B299, 0x8B8F8C, 0xA78E78, 0xD2C4A8};
    auto block = [&](float x, float z, float w, float d, float h, int ci) {
        PushMat pm(mat(rgb(cols[ci % 6]), 0.85f).kind(K_STUCCO));
        box(V3(x - w, 0, z - d), V3(x + w, h, z + d));
        box(V3(x - w - 0.15f, h, z - d - 0.15f), V3(x + w + 0.15f, h + 0.35f, z + d + 0.15f));
        PushMat pm2(mat(V3(0.03f, 0.035f, 0.04f), 0.08f).spec(1.5f));   // dark shopfront glazing
        box(V3(x - w * 0.8f, 0.6f, z - d - 0.04f), V3(x + w * 0.8f, 2.6f, z - d + 0.02f));
        box(V3(x - w * 0.8f, 0.6f, z + d - 0.02f), V3(x + w * 0.8f, 2.6f, z + d + 0.04f));
    };
    for (int i = 0; i < 30; i++) {
        float x = -260 + i * 18.0f + r.r(-3, 3), z = r.r(92, 150), w = r.r(5, 10), h = r.r(3.6f, 8.5f), d = r.r(5, 10);
        block(x, z, w, d, h, i);
    }
    for (int i = 0; i < 16; i++) {       // across the street, behind the camera's starting point: seen in reflections
        float x = -150 + i * 20.0f + r.r(-3, 3), z = -r.r(105, 135), w = r.r(5, 9), h = r.r(3.6f, 7.5f), d = r.r(5, 9);
        block(x, z, w, d, h, i + 3);
    }
    for (int s = -1; s <= 1; s += 2)
        for (int i = 0; i < 9; i++) { float z = -70 + i * 26.0f + r.r(-4, 4), x = s * r.r(60, 95), w = r.r(5, 9), h = r.r(3.6f, 7.0f), d = r.r(6, 11); block(x, z, w, d, h, i + (s > 0 ? 1 : 4)); }
    // palms: a row along the street behind the building, a loose line down both sides of the lot, a few far ones
    struct PP { float x, z, h; };
    std::vector<PP> pp;
    for (int i = 0; i < 13; i++) pp.push_back({-78.0f + i * 13.0f + r.r(-2, 2), r.r(34, 40), r.r(12.5f, 17.0f)});
    for (int i = 0; i < 7; i++) { pp.push_back({-24.0f + r.r(-1.5f, 1.5f), -62.0f + i * 15.5f, r.r(10.5f, 14.5f)}); pp.push_back({25.0f + r.r(-1.5f, 1.5f), -58.0f + i * 15.5f, r.r(10.5f, 14.5f)}); }
    for (int i = 0; i < 12; i++) pp.push_back({r.r(-170, 170), r.r(70, 135), r.r(11, 16)});
    for (int i = 0; i < 9; i++) pp.push_back({-95.0f + i * 23.0f + r.r(-3, 3), -r.r(84, 92), r.r(11, 15)});
    pp.push_back({-13.2f, -27.2f, 8.2f}); pp.push_back({13.6f, -27.4f, 7.4f});     // by the building's corners
    int id = 0;
    for (auto &q : pp) {
        float d = std::sqrt(q.x * q.x + (q.z + 38.0f) * (q.z + 38.0f));      // from the middle of the lot
        Push p(xfT(q.x, 0, q.z));
        palm(q.h, r.r(0.01f, 0.07f), 900 + id++, clampf(d / 45.0f, 1.0f, 2.2f));
    }
    // utility poles and wires along the far side street (x = -44) and the street behind
    {
        std::vector<V3> pa;
        for (int i = 0; i < 9; i++) pa.push_back(V3(-44.0f + r.r(-0.3f, 0.3f), 0, -95.0f + i * 34.0f));
        for (size_t i = 0; i < pa.size(); i++) { utilityPole(pa[i], 0); if (i + 1 < pa.size()) wires(pa[i], pa[i + 1], 0); }
        std::vector<V3> pb;
        for (int i = 0; i < 9; i++) pb.push_back(V3(-150.0f + i * 38.0f, 0, 58.0f + r.r(-0.3f, 0.3f)));
        for (size_t i = 0; i < pb.size(); i++) utilityPole(pb[i], PI / 2);
    }
}
