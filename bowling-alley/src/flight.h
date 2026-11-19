// flight.h — the drone: flight path, attitude (banking from real accelerations), and the remapping of
// video time onto world time that produces the bullet-time passage.
#pragma once
#include "physics.h"
#include "atlas.h"

struct Cam { V3 p; Quat q; float fov = 114; float aperture = 0.0009f; float focus = 2.5f; };
static const double FILM_LEN = 42.0;
static float HAZE_SIGMA = 0.010f;
static V3 HAZE_COLOR(0.115f, 0.095f, 0.065f);

static Cam lookCam(V3 eye, V3 at, V3 up, float roll) {
    V3 f = norm(at - eye), r = norm(cross(f, up)), u = cross(r, f);
    float c = std::cos(roll), s = std::sin(roll);
    V3 r2 = r * c + u * s, u2 = u * c - r * s;
    Cam cm; cm.p = eye; cm.q = qbasis(r2, u2, -f);
    return cm;
}

// ---------------------------------------------------------------- time remap
struct RateKey { double tv; double rate; };
static const RateKey RATE[] = {{0, 1}, {17.70, 1}, {17.86, 0.12}, {19.3, 0.12}, {19.6, 0.02}, {21.0, 0.02}, {21.6, 0.25}, {22.8, 1.0}, {1e9, 1.0}};
static std::vector<double> TAU_TAB; // world time at 1 ms steps of video time
static double rateAt(double tv) {
    int n = sizeof(RATE) / sizeof(RATE[0]);
    for (int i = 0; i + 1 < n; i++)
        if (tv < RATE[i + 1].tv) {
            double u = (tv - RATE[i].tv) / (RATE[i + 1].tv - RATE[i].tv);
            u = u * u * (3 - 2 * u);
            // interpolate in log space so the ramps feel even
            return std::exp(std::log(RATE[i].rate) + (std::log(RATE[i + 1].rate) - std::log(RATE[i].rate)) * u);
        }
    return 1;
}
static double worldTime(double tv) {
    if (tv <= 0) return tv;
    double f = tv * 1000.0;
    int i = (int)f;
    if (i >= (int)TAU_TAB.size() - 1) return TAU_TAB.back() + (tv - (TAU_TAB.size() - 1) / 1000.0);
    return TAU_TAB[i] + (TAU_TAB[i + 1] - TAU_TAB[i]) * (f - i);
}
static double videoTimeOf(double tau) { // inverse (monotone) by bisection
    double a = 0, b = FILM_LEN + 5;
    for (int i = 0; i < 50; i++) { double m = (a + b) * 0.5; if (worldTime(m) < tau) a = m; else b = m; }
    return (a + b) * 0.5;
}

// ---------------------------------------------------------------- path
struct FKey { float t; V3 p; V3 look; };
static std::vector<FKey> FK;
static const int PATH_HZ = 240;
static std::vector<V3> PATH_P, PATH_L;
static std::vector<float> PATH_ROLL, PATH_PITCH;

static V3 hermite(const std::vector<FKey> &k, float t, bool look) {
    int n = (int)k.size();
    if (t <= k[0].t) return look ? k[0].look : k[0].p;
    if (t >= k[n - 1].t) return look ? k[n - 1].look : k[n - 1].p;
    int i = 0;
    while (i + 1 < n && k[i + 1].t <= t) i++;
    auto P = [&](int j) { j = std::min(std::max(j, 0), n - 1); return look ? k[j].look : k[j].p; };
    auto T = [&](int j) { j = std::min(std::max(j, 0), n - 1); return k[j].t; };
    auto vel = [&](int j) {
        if (j <= 0) return (P(1) - P(0)) / (T(1) - T(0));
        if (j >= n - 1) return (P(n - 1) - P(n - 2)) / (T(n - 1) - T(n - 2));
        return (P(j + 1) - P(j - 1)) / (T(j + 1) - T(j - 1));
    };
    float h = T(i + 1) - T(i), s = (t - T(i)) / h;
    float h00 = (1 + 2 * s) * (1 - s) * (1 - s), h10 = s * (1 - s) * (1 - s), h01 = s * s * (3 - 2 * s), h11 = s * s * (s - 1);
    return P(i) * h00 + vel(i) * (h10 * h) + P(i + 1) * h01 + vel(i + 1) * (h11 * h);
}

static void buildFlight() {
    // time table
    TAU_TAB.clear();
    double tau = 0;
    int n = (int)((FILM_LEN + 2) * 1000);
    for (int i = 0; i <= n; i++) { TAU_TAB.push_back(tau); tau += rateAt((i + 0.5) / 1000.0) * 0.001; }

    const float cx = laneX(HERO);
    auto K = [&](float t, V3 p, V3 look) { FK.push_back({t, p, look}); };
    FK.clear();
    // --- outside: one long glide down over the lot to the open door
    // (before the chase the camera simply looks where it is going: see the look-ahead below; these look points
    //  only matter for the hand-over into the chase)
    K(0.0f, V3(-1.9f, 5.6f, -49.0f), V3(-2.4f, 5.2f, -25.3f));
    K(1.6f, V3(-2.5f, 4.6f, -42.0f), V3(-2.9f, 4.0f, -25.3f));
    K(3.0f, V3(-3.1f, 2.95f, -34.5f), V3(-3.5f, 1.9f, -25.0f));
    K(4.1f, V3(-3.45f, 1.88f, -28.6f), V3(-3.6f, 1.45f, -20.0f));
    K(4.9f, V3(-3.55f, 1.56f, -25.1f), V3(-4.6f, 1.35f, -18.5f));
    // --- lobby: a single wide arc along the snack bar, round the jukebox and out over the ball racks
    K(5.7f, V3(-4.1f, 1.55f, -22.6f), V3(-6.6f, 1.30f, -18.0f));
    K(6.6f, V3(-5.3f, 1.53f, -20.4f), V3(-6.95f, 1.28f, -16.0f));
    K(7.5f, V3(-6.15f, 1.50f, -18.2f), V3(-7.05f, 1.26f, -14.5f));
    K(8.4f, V3(-6.45f, 1.48f, -15.9f), V3(-7.0f, 1.28f, -12.4f));
    K(9.3f, V3(-6.0f, 1.48f, -13.9f), V3(-3.6f, 1.25f, -12.4f));
    K(10.2f, V3(-4.95f, 1.50f, -12.3f), V3(-3.3f, 1.2f, -12.2f));
    K(11.0f, V3(-3.9f, 1.56f, -10.9f), V3(-1.6f, 1.05f, -9.2f));
    K(11.8f, V3(-2.6f, 1.62f, -9.9f), V3(-0.3f, 0.95f, -7.4f));
    K(12.6f, V3(-1.4f, 1.58f, -8.8f), V3(0.2f, 0.85f, -5.6f));
    K(13.4f, V3(-0.5f, 1.50f, -7.3f), V3(0.35f, 0.75f, -3.6f));
    K(14.1f, V3(0.02f, 1.38f, -5.9f), V3(0.5f, 0.55f, -1.6f));
    K(14.8f, V3(0.25f, 1.05f, -4.3f), V3(0.75f, 0.30f, 1.0f));
    K(15.25f, V3(0.52f, 0.74f, -2.7f), V3(cx - 0.1f, 0.12f, 3.5f));
    // --- the ball comes through from behind; chase it down the lane
    float tr = (float)T_REL;
    auto ballAt = [&](float t) { double z = ballZ(t); return V3(cx + BALL_PATH.eval((float)z), BALL_R, (float)z); };
    K(tr, V3(cx - 0.05f, 0.47f, -1.25f), ballAt(tr) + V3(0, 0, 3.2f));
    K(tr + 0.4f, V3(cx - 0.04f, 0.31f, 1.55f), ballAt(tr + 0.4f) + V3(0, 0, 3.0f));
    K(tr + 0.9f, V3(cx - 0.10f, 0.27f, 5.6f), ballAt(tr + 0.9f) + V3(0, 0, 2.8f));
    K(tr + 1.4f, V3(cx - 0.20f, 0.25f, 10.1f), ballAt(tr + 1.4f) + V3(0, 0, 2.6f));
    K(tr + 1.8f, V3(cx - 0.28f, 0.27f, 13.75f), ballAt(tr + 1.8f) + V3(0.1f, 0, 2.6f));
    K(17.66f, V3(cx - 0.36f, 0.31f, 15.75f), V3(cx + 0.0f, 0.16f, 18.3f));
    // --- bullet time: slide in under the masking unit and arc round the exploding rack
    K(17.86f, V3(cx - 0.42f, 0.33f, 16.55f), V3(cx, 0.18f, 18.35f));
    K(18.5f, V3(cx - 0.56f, 0.36f, 17.35f), V3(cx + 0.02f, 0.18f, 18.45f));
    K(19.3f, V3(cx - 0.66f, 0.42f, 18.05f), V3(cx + 0.05f, 0.17f, 18.6f));
    K(20.1f, V3(cx - 0.64f, 0.48f, 18.75f), V3(cx + 0.08f, 0.16f, 18.65f));
    K(20.9f, V3(cx - 0.42f, 0.52f, 19.30f), V3(cx + 0.05f, 0.14f, 18.7f));
    K(21.6f, V3(cx - 0.05f, 0.54f, 19.52f), V3(cx + 0.0f, 0.08f, 18.8f));
    K(22.4f, V3(cx + 0.30f, 0.53f, 19.52f), V3(cx - 0.05f, 0.0f, 19.3f));
    // --- time runs again: the ball slams the cushion, the sweep clears the deck into the pit below us
    K(23.2f, V3(cx + 0.42f, 0.52f, 19.50f), V3(cx - 0.25f, -0.05f, 19.55f));
    K(24.0f, V3(cx + 0.45f, 0.56f, 19.52f), V3(cx - 0.05f, 0.05f, 19.0f));
    K(24.8f, V3(cx + 0.45f, 0.78f, 19.58f), V3(cx, 0.10f, 18.75f));
    // --- up to watch the table bring the new rack down
    K(25.6f, V3(cx + 0.45f, 1.02f, 19.62f), V3(cx, 0.25f, 18.7f));
    K(26.6f, V3(cx + 0.40f, 0.98f, 19.56f), V3(cx, 0.18f, 18.7f));
    // --- a look along the row of machines, round to the elevator wheel, then along the belt to the turret
    K(27.3f, V3(cx + 0.40f, 1.10f, 19.55f), V3(cx + 3.0f, 0.95f, 19.2f));
    K(28.0f, V3(cx + 0.38f, 1.25f, 19.53f), V3(cx + 1.6f, 0.90f, 20.6f));
    K(28.7f, V3(cx + 0.36f, 1.45f, 19.50f), V3(cx + 0.42f, 0.85f, 20.25f));
    K(29.5f, V3(cx + 0.38f, 1.62f, 19.46f), V3(cx + 0.10f, 1.00f, 20.15f));
    K(30.3f, V3(cx + 0.42f, 1.72f, 19.36f), V3(cx + 0.02f, 1.40f, 19.2f));
    K(31.0f, V3(cx + 0.42f, 1.74f, 19.22f), V3(cx + 0.02f, 1.55f, 18.85f));
    K(31.7f, V3(cx + 0.40f, 1.50f, 19.38f), V3(cx, 1.20f, 18.7f));
    // --- drop behind the table and leave over the fresh rack, out under the masking unit
    K(32.4f, V3(cx + 0.22f, 0.80f, 19.50f), V3(cx, 0.35f, 18.3f));
    K(33.0f, V3(cx + 0.06f, 0.52f, 19.30f), V3(cx, 0.36f, 17.0f));
    K(33.6f, V3(cx, 0.48f, 18.45f), V3(cx, 0.36f, 15.0f));
    K(34.1f, V3(cx, 0.45f, 17.35f), V3(cx, 0.42f, 12.0f));
    // --- back up the lane toward the lobby, rising; then turn to face the lanes for the last look
    K(34.9f, V3(cx + 0.02f, 0.62f, 14.6f), V3(cx - 0.1f, 0.7f, 6.0f));
    K(36.0f, V3(cx + 0.0f, 0.95f, 9.6f), V3(cx - 0.3f, 1.1f, 0.0f));
    K(37.1f, V3(cx - 0.15f, 1.45f, 4.6f), V3(cx - 0.8f, 1.5f, -6.0f));
    K(38.1f, V3(cx - 0.45f, 1.85f, 0.6f), V3(-3.5f, 1.6f, -6.5f));
    K(39.0f, V3(cx - 0.75f, 2.05f, -1.9f), V3(-5.5f, 1.4f, 0.5f));
    K(39.8f, V3(cx - 0.85f, 2.15f, -3.3f), V3(-3.0f, 1.0f, 8.0f));
    K(40.6f, V3(cx - 0.86f, 2.20f, -4.0f), V3(0.0f, 0.7f, 14.0f));
    K(42.5f, V3(cx - 0.86f, 2.26f, -4.4f), V3(0.2f, 0.6f, 17.0f));

    // sample, then low-pass (the drone has mass); roll from lateral acceleration
    int N = (int)((FILM_LEN + 1) * PATH_HZ);
    std::vector<V3> rp(N), rl(N);
    for (int i = 0; i < N; i++) { float t = (float)i / PATH_HZ; rp[i] = hermite(FK, t, false); rl[i] = hermite(FK, t, true); }
    auto gauss = [&](const std::vector<V3> &in, float sigma) {
        int r = (int)(sigma * PATH_HZ * 3);
        std::vector<V3> out(in.size());
        for (int i = 0; i < (int)in.size(); i++) {
            V3 s; float w = 0;
            for (int j = -r; j <= r; j++) {
                int k = std::min(std::max(i + j, 0), (int)in.size() - 1);
                float g = std::exp(-0.5f * (j / (sigma * PATH_HZ)) * (j / (sigma * PATH_HZ)));
                s += in[k] * g; w += g;
            }
            out[i] = s / w;
        }
        return out;
    };
    // the first half is flown like a cinewhoop on a calm day: a heavily smoothed line, eyes on where it is going.
    // From the chase on the path is tighter and the look points are placed by hand.
    {
        std::vector<V3> pTight = gauss(rp, 0.11f), pCalm = gauss(rp, 0.42f), lTight = gauss(rl, 0.16f);
        PATH_P.resize(N); PATH_L.resize(N);
        for (int i = 0; i < N; i++) { float t = (float)i / PATH_HZ; PATH_P[i] = lerp(pCalm[i], pTight[i], smooth(14.3f, 15.5f, t)); }
        std::vector<V3> ahead(N);
        for (int i = 0; i < N; i++) {
            int j = std::min(i + (int)(0.95f * PATH_HZ), N - 1);
            V3 d = PATH_P[j] - PATH_P[i];
            float dl = len(d);
            // keep a steady look distance and a slight downward tilt
            ahead[i] = PATH_P[i] + (dl > 1e-4f ? d * (4.0f / dl) : V3(0, 0, 4.0f)) + V3(0, -0.42f, 0);
        }
        ahead = gauss(ahead, 0.35f);
        for (int i = 0; i < N; i++) { float t = (float)i / PATH_HZ; PATH_L[i] = lerp(ahead[i], lTight[i], smooth(14.0f, 15.3f, t)); }
    }
    PATH_ROLL.assign(N, 0); PATH_PITCH.assign(N, 0);
    std::vector<V3> rr(N);
    for (int i = 1; i < N - 1; i++) {
        V3 acc = (PATH_P[i + 1] - PATH_P[i] * 2.0f + PATH_P[i - 1]) * (float)(PATH_HZ * PATH_HZ);
        V3 f = norm(PATH_L[i] - PATH_P[i]);
        V3 right = norm(cross(f, V3(0, 1, 0)));
        V3 fwd = norm(V3(f.x, 0, f.z));
        float alat = clampf(dot(acc, right), -14.0f, 14.0f), afwd = clampf(dot(acc, fwd), -12.0f, 12.0f);
        float tt = (float)i / PATH_HZ, calm = 1.0f - smooth(14.5f, 16.0f, tt);
        rr[i] = V3(-std::atan2(alat, 9.81f) * lerp(0.60f, 0.22f, calm), -std::atan2(afwd, 9.81f) * lerp(0.22f, 0.08f, calm), 0);
    }
    rr = gauss(rr, 0.16f);
    for (int i = 0; i < N; i++) { PATH_ROLL[i] = rr[i].x; PATH_PITCH[i] = rr[i].y; }
}

static Cam camAt(double tv) {
    double f = std::max(0.0, tv) * PATH_HZ;
    int i = std::min((int)f, (int)PATH_P.size() - 2);
    float u = (float)(f - i);
    V3 p = lerp(PATH_P[i], PATH_P[i + 1], u), l = lerp(PATH_L[i], PATH_L[i + 1], u);
    float roll = lerp(PATH_ROLL[i], PATH_ROLL[i + 1], u), pitch = lerp(PATH_PITCH[i], PATH_PITCH[i + 1], u);
    float t = (float)tv;
    // small attitude noise: slow drift plus a touch of prop-wash buzz
    float nr = 0.0030f * vnoise1(t * 1.1f, 1) + 0.0008f * vnoise1(t * 9.0f, 2);
    float np = 0.0026f * vnoise1(t * 0.9f, 3) + 0.0007f * vnoise1(t * 8.0f, 4);
    float ny = 0.0026f * vnoise1(t * 0.8f, 5) + 0.0007f * vnoise1(t * 7.0f, 6);
    V3 fw = norm(l - p);
    V3 right = norm(cross(fw, V3(0, 1, 0))), up = cross(right, fw);
    fw = norm(fw + up * (pitch + np) + right * ny);
    Cam c = lookCam(p, p + fw, V3(0, 1, 0), roll + nr);
    c.p = p + V3(0.002f * vnoise1(t * 1.7f, 7), 0.002f * vnoise1(t * 1.9f, 8), 0.002f * vnoise1(t * 1.5f, 9));
    return c;
}
static void dumpFlight(const char *path) {
    FILE *f = fopen(path, "w");
    if (!f) return;
    for (int i = 0; i <= (int)(FILM_LEN * 30); i++) {
        Cam c = camAt(i / 30.0);
        V3 fw = qrot(c.q, V3(0, 0, -1)), rt = qrot(c.q, V3(1, 0, 0));
        fprintf(f, "%d %.4f %.4f %.4f %.3f %.3f %.3f\n", i, c.p.x, c.p.y, c.p.z, std::atan2(fw.x, fw.z) * 57.2958f, std::asin(fw.y) * 57.2958f, std::asin(rt.y) * 57.2958f);
    }
    fclose(f);
}
static float fadeAt(double tv) { return smooth(0.0f, 0.7f, (float)tv) * smooth((float)FILM_LEN, (float)FILM_LEN - 0.9f, (float)tv); }

// timeline of sound-making events for the audio synthesis
static void writeTimeline(const char *path) {
    FILE *f = fopen(path, "w");
    if (!f) return;
    fprintf(f, "{\n \"film_len\": %.3f,\n \"t_rel\": %.4f,\n \"t_hand\": %.4f,\n", FILM_LEN, videoTimeOf(T_REL), videoTimeOf(T_HAND));
    fprintf(f, " \"t_impact\": %.4f,\n \"t_cushion\": %.4f,\n", videoTimeOf(PHYS.tImpact), videoTimeOf(PHYS.tCushion));
    fprintf(f, " \"rate\": [");
    for (int i = 0; i <= (int)(FILM_LEN * 100); i++) fprintf(f, "%s%.4f", i ? "," : "", rateAt(i / 100.0));
    fprintf(f, "],\n \"cam\": [");
    for (int i = 0; i <= (int)(FILM_LEN * 50); i++) {
        double tv = i / 50.0;
        Cam c = camAt(tv), c2 = camAt(tv + 0.02);
        V3 v = (c2.p - c.p) / 0.02f;
        bool bv; Pose bp = ballPose(worldTime(tv), &bv);
        fprintf(f, "%s[%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%d]", i ? "," : "", c.p.x, c.p.y, c.p.z, len(v), bp.p.x, bp.p.y, bp.p.z, (int)bv);
    }
    fprintf(f, "],\n \"events\": [");
    bool first = true;
    for (auto &e : PHYS.events) {
        double tv = videoTimeOf(e.t);
        fprintf(f, "%s[%.4f,%d,%.3f,%.3f,%.3f,%.3f,%.4f]", first ? "" : ",", tv, e.kind, e.strength, e.pos.x, e.pos.y, e.pos.z, rateAt(tv));
        first = false;
    }
    fprintf(f, "],\n \"machine\": {\"sweep_down\": %.3f, \"sweep_run\": %.3f, \"sweep_back\": %.3f, \"table_down\": %.3f, \"table_set\": %.3f, \"table_up\": %.3f, \"sweep_up\": %.3f},\n",
            videoTimeOf(T_HAND + 0.75), videoTimeOf(T_HAND + 1.45), videoTimeOf(T_HAND + 2.55), videoTimeOf(T_HAND + 3.55), videoTimeOf(T_HAND + 4.6), videoTimeOf(T_HAND + 4.95), videoTimeOf(T_HAND + 5.95));
    fprintf(f, " \"hero_x\": %.3f\n}\n", laneX(HERO));
    fclose(f);
}
