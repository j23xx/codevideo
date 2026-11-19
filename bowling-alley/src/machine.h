// machine.h — the automatic pinsetter behind each lane: frame, sweep, pin table, pit cushion, pin elevator wheel,
// distributor belt, turret and drive. The hero lane's machine is fully animated; the others stand idle.
#pragma once
#include "physics.h"
#include "world.h"

static const float WH_Y = 0.58f, WH_Z = 20.41f;          // wheel centre (ring spans z 20.36..20.46)
static const float WH_RIN = 0.60f, WH_ROUT = 0.68f, WH_RPIN = 0.525f;
static const float WH_W = 1.4f;                          // rad/s
static const float SCOOP_Z0 = 19.98f, SCOOP_Z1 = 20.36f;
static const V3 BELT_A(0, 0.84f, 20.12f), BELT_B(0, 1.78f, 18.97f); // belt surface, lane-centre relative x
static const float BELT_V = 0.75f;
static const float TUR_Y = 1.34f, TUR_Z = 18.62f, TUR_R = 0.30f;    // turret: pin base height, centre z, pocket circle
static const float TABLE_UP = 0.70f;                     // pin base height when the table is up
static const int TUR_BAKED = 1;                          // pins already in the turret at the start

struct MMats { int green, yellow, redPaint, belt, chain, cage, lampRed, lampGreen, bulb, galv, cushion, nameplate; };
static MMats MM;
struct MProtos { int wheel, turret, table, sweep, carriage, cushion, cam, pulleyBig, pulleySmall, pinLo; };
static MProtos MP;

static void makeMachineMaterials() {
    MM.green = mat(rgb(0x62786c), 0.42f).kind(K_METAL, 1.0f);
    MM.yellow = mat(rgb(0xd9a400), 0.45f).kind(K_METAL, 0.8f);
    MM.redPaint = mat(rgb(0xa51c14), 0.4f).kind(K_METAL, 0.8f);
    MM.belt = mat(rgb(0x23211f), 0.85f).kind(K_BELT, BELT_V, 1.0f);
    MM.chain = mat(rgb(0x3a3936), 0.4f).metal(0.8f).kind(K_BELT, 0.9f, 2.0f);
    MM.cage = mat(V3(0.62f, 0.62f, 0.63f), 0.3f).metal().kind(K_CHROME);
    MM.lampRed = mat(V3(0.3f, 0.02f, 0.02f), 0.3f).emit(V3(14, 0.5f, 0.3f), 3);
    MM.lampGreen = mat(V3(0.02f, 0.3f, 0.05f), 0.3f).emit(V3(0.6f, 10, 1.2f), 3);
    MM.galv = mat(V3(0.60f, 0.61f, 0.60f), 0.5f).metal(0.9f).kind(K_METAL, 0.6f);
    MM.cushion = mat(V3(0.025f, 0.024f, 0.022f), 0.7f).kind(K_GUTTER);
    MM.nameplate = mat(V3(1, 1, 1), 0.35f).kind(K_ATLAS, 1.0f);
}

static void bolt(V3 p, V3 n, float r = 0.009f) {
    PushMat pm(MT.steel);
    cyl(p, p + norm(n) * 0.007f, r, r, 6, false, true);
}

static void makeMachineProtos() {
    {   // low-res pin for pins seen only inside machines
        ProtoScope ps; MP.pinLo = ps.id;
        PushMat pm(MT.pin);
        std::vector<V2> pr = pinProfile(), lo;
        for (size_t i = 0; i < pr.size(); i += 3) lo.push_back(pr[i]);
        lo.push_back(pr.back());
        lathe(lo, 20);
    }
    {   // pin elevator wheel: origin at centre, axis along z, scoops reach toward -z
        ProtoScope ps; MP.wheel = ps.id;
        {
            PushMat pm(MM.green, 3);
            Push p(xfRX(PI / 2)); // lathe axis Y -> Z
            lathe({V2(WH_RIN, -0.05f), V2(WH_ROUT, -0.05f), V2(WH_ROUT, 0.05f), V2(WH_RIN, 0.05f), V2(WH_RIN, -0.05f)}, 64);
            lathe({V2(0.0f, 0.02f), V2(0.07f, 0.02f), V2(0.07f, 0.12f), V2(0.0f, 0.12f)}, 20); // hub (behind the ring)
        }
        for (int i = 0; i < 4; i++) { // spokes on the back face
            PushMat pm(MM.green, 4);
            Push p(xfRZ(PI / 4 + i * PI / 2));
            rbox(V3(0.05f, -0.03f, 0.055f), V3(WH_RIN + 0.03f, 0.03f, 0.085f), 0.006f);
            bolt(V3(WH_RIN - 0.02f, 0, 0.085f), V3(0, 0, 1));
            bolt(V3(0.1f, 0, 0.085f), V3(0, 0, 1));
        }
        for (int i = 0; i < 8; i++) { // scoops: troughs open toward the wheel centre
            Push p(xfRZ(i * PI / 4));
            PushMat pm(MM.galv, (float)i);
            float zl = SCOOP_Z0 - WH_Z, zr = SCOOP_Z1 - WH_Z + 0.01f;
            surf(10, 1, [&](float u, float v) {
                float a = PI * 0.5f + (u - 0.5f) * PI * 1.25f; // arc centred on the outward (+x after rotation... local +y) direction
                // trough centre at radius WH_RPIN along local +x; open side faces -x (the centre)
                float ang = (u - 0.5f) * PI * 1.2f;
                V3 c(WH_RPIN, 0, 0);
                V3 o(std::cos(ang) * 0.078f, std::sin(ang) * 0.078f, 0);
                (void)a;
                return c + o + V3(0, 0, lerp(zl, zr, v));
            });
            // end lip at the front so pins cannot slide out
            surf(8, 1, [&](float u, float v) {
                float ang = (u - 0.5f) * PI * 1.2f;
                float rr = 0.078f * (1 - v);
                return V3(WH_RPIN + std::cos(ang) * rr, std::sin(ang) * rr, zl);
            });
            PushMat pm2(MT.steel);
            rbox(V3(WH_RIN - 0.01f, -0.02f, zr - 0.03f), V3(WH_RIN + 0.05f, 0.02f, zr + 0.005f), 0.004f);
        }
    }
    {   // turret: origin at centre, pin-base level; rotates about y
        ProtoScope ps; MP.turret = ps.id;
        {
            PushMat pm(MM.green, 9);
            cyl(V3(0, -0.05f, 0), V3(0, 0.62f, 0), 0.03f, 0.03f, 12);
            cyl(V3(0, 0.44f, 0), V3(0, 0.47f, 0), TUR_R + 0.02f, TUR_R + 0.02f, 40);
            cyl(V3(0, -0.02f, 0), V3(0, 0.0f, 0), 0.12f, 0.12f, 20);
        }
        for (int i = 0; i < 10; i++) {
            float a = i * 2 * PI / 10;
            V3 c(std::sin(a) * TUR_R, 0, std::cos(a) * TUR_R); // pocket 0 faces +z (the loading position)
            PushMat pm(MM.cage);
            torus(c + V3(0, 0.10f, 0), V3(0, 1, 0), 0.071f, 0.005f, 20, 6);
            torus(c + V3(0, 0.33f, 0), V3(0, 1, 0), 0.071f, 0.005f, 20, 6);
            torus(c + V3(0, 0.0f, 0), V3(0, 1, 0), 0.035f, 0.005f, 14, 6);
            for (int k = 0; k < 4; k++) {
                float b = a + PI / 4 + k * PI / 2;
                V3 d(std::sin(b) * 0.071f, 0, std::cos(b) * 0.071f);
                tube({c + V3(d.x * 0.5f, 0, d.z * 0.5f), c + d + V3(0, 0.10f, 0), c + d + V3(0, 0.445f, 0)}, 0.004f, 6);
            }
            if (i >= 10 - TUR_BAKED) bake(MP.pinLo, xfT(c) * xfRY(i * 1.7f), 100.0f + i);
        }
    }
    {   // pin table: origin at lane centre, pin-base level, z = world z
        ProtoScope ps; MP.table = ps.id;
        for (int k = 0; k < 10; k++) {
            V3 s = pinSpot(k);
            PushMat pm(MM.cage);
            torus(V3(s.x, 0.125f, s.z), V3(0, 1, 0), 0.068f, 0.007f, 24, 6);
            torus(V3(s.x, 0.30f, s.z), V3(0, 1, 0), 0.034f, 0.006f, 16, 6);
            for (int j = 0; j < 3; j++) {
                float a = j * 2 * PI / 3 + k;
                tube({V3(s.x + 0.068f * std::cos(a), 0.125f, s.z + 0.068f * std::sin(a)), V3(s.x + 0.034f * std::cos(a), 0.30f, s.z + 0.034f * std::sin(a)), V3(s.x + 0.034f * std::cos(a), 0.44f, s.z + 0.034f * std::sin(a))}, 0.004f, 6);
            }
        }
        {
            PushMat pm(MM.green, 12);
            // triangular deck plate above the pin heads, with ribs
            V3 a = pinSpot(0), b = pinSpot(6), c = pinSpot(9);
            std::vector<V2> poly = {V2(a.x - 0.14f, a.z - 0.16f), V2(a.x + 0.14f, a.z - 0.16f), V2(c.x + 0.16f, c.z - 0.02f), V2(c.x + 0.16f, c.z + 0.13f), V2(b.x - 0.16f, b.z + 0.13f), V2(b.x - 0.16f, b.z - 0.02f)};
            extrude(poly, 0.44f, 0.465f);
            rbox(V3(-0.03f, 0.465f, a.z - 0.1f), V3(0.03f, 0.52f, c.z + 0.1f), 0.006f);
            rbox(V3(b.x - 0.1f, 0.465f, c.z - 0.03f), V3(c.x + 0.1f, 0.52f, c.z + 0.03f), 0.006f);
            // yoke up to the lift rods
            rbox(V3(-0.62f, 0.50f, 18.66f), V3(0.62f, 0.56f, 18.72f), 0.006f);
            for (int s = -1; s <= 1; s += 2) {
                rbox(V3(s * 0.62f - 0.035f, 0.44f, 18.64f), V3(s * 0.62f + 0.035f, 0.62f, 18.74f), 0.008f); // slider blocks on the guide rods
                bolt(V3(s * 0.62f, 0.56f, 18.64f), V3(0, 0, -1));
            }
        }
        {
            PushMat pm(MM.yellow);
            V3 a = pinSpot(0);
            rbox(V3(-0.13f, 0.40f, a.z - 0.175f), V3(0.13f, 0.47f, a.z - 0.16f), 0.004f);
        }
    }
    {   // sweep board + arms: origin at lane centre, bottom edge of the board, pin-side face at z = 0
        ProtoScope ps; MP.sweep = ps.id;
        {
            PushMat pm(MT.darkWood, 5);
            rbox(V3(-0.70f, 0.0f, -0.03f), V3(0.70f, 0.26f, 0.0f), 0.006f);
        }
        {
            PushMat pm(MT.rubber);
            rbox(V3(-0.70f, -0.012f, -0.034f), V3(0.70f, 0.012f, 0.004f), 0.004f);
        }
        {
            PushMat pm(MM.yellow);
            rbox(V3(-0.70f, 0.20f, -0.036f), V3(0.70f, 0.26f, -0.03f), 0.002f);
        }
        for (int s = -1; s <= 1; s += 2) {
            PushMat pm(MT.steel);
            rbox(V3(s * 0.74f - 0.012f, 0.05f, -0.05f), V3(s * 0.74f + 0.012f, 1.02f, -0.015f), 0.004f);
            rbox(V3(s * 0.70f - 0.05f, 0.08f, -0.045f), V3(s * 0.70f + 0.05f, 0.14f, -0.03f), 0.004f);
            bolt(V3(s * 0.70f, 0.11f, -0.045f), V3(0, 0, -1));
        }
    }
    {   // sweep carriage: rides the rails; origin at lane centre, z = sweep z
        ProtoScope ps; MP.carriage = ps.id;
        for (int s = -1; s <= 1; s += 2) {
            PushMat pm(MM.green, 20);
            rbox(V3(s * 0.74f - 0.04f, 0.70f, -0.12f), V3(s * 0.74f + 0.04f, 0.80f, 0.05f), 0.008f);
            PushMat pm2(MT.steel);
            for (int k = 0; k < 2; k++) {
                V3 c(s * 0.74f, 0.74f, -0.09f + k * 0.11f);
                cyl(c + V3(-0.05f * s, 0, 0), c + V3(-0.06f * s - 0.0f, 0, 0), 0.03f, 0.03f, 14);
            }
        }
        PushMat pm(MM.green, 21);
        rbox(V3(-0.74f, 0.80f, -0.06f), V3(0.74f, 0.84f, -0.02f), 0.006f);
    }
    {   // pit cushion: origin at the hinge line (top), hangs down along -y; face toward -z
        ProtoScope ps; MP.cushion = ps.id;
        float x0 = CUSH_GAP_LX, x1 = PITCH * 0.5f - 0.075f;
        float hgt = 0.66f - CUSH_BOTTOM;
        {
            PushMat pm(MM.cushion, 3);
            rbox(V3(x0, -hgt, 0), V3(x1, -0.02f, 0.045f), 0.01f);
        }
        {
            PushMat pm(MT.steel);
            cyl(V3(x0 - 0.02f, 0, 0.02f), V3(x1 + 0.02f, 0, 0.02f), 0.012f, 0.012f, 10);
            for (int i = 0; i < 6; i++) bolt(V3(lerp(x0 + 0.08f, x1 - 0.08f, i / 5.0f), -0.06f, 0), V3(0, 0, -1), 0.012f);
        }
    }
    {   // camshaft with three eccentric cams: origin on the shaft axis (x), spins about x
        ProtoScope ps; MP.cam = ps.id;
        PushMat pm(MT.steel);
        cyl(V3(-0.5f, 0, 0), V3(0.5f, 0, 0), 0.016f, 0.016f, 12);
        for (int i = 0; i < 3; i++) {
            float x = -0.3f + i * 0.3f, a = i * 2.1f;
            V3 o(0, std::cos(a) * 0.03f, std::sin(a) * 0.03f);
            PushMat pm2(i == 1 ? MM.redPaint : MM.galv);
            cyl(V3(x - 0.015f, 0, 0) + o, V3(x + 0.015f, 0, 0) + o, 0.07f, 0.07f, 28);
            bolt(V3(x + 0.015f, 0, 0) + o * 2.0f, V3(1, 0, 0), 0.01f);
        }
    }
    auto pulley = [&](float R, int holes) {
        PushMat pm(MM.green, 30 + R * 10);
        Push p(xfRZ(PI / 2)); // lathe axis Y -> -X ... (spins about x)
        lathe({V2(0.02f, -0.02f), V2(R, -0.02f), V2(R, -0.012f), V2(R - 0.012f, 0.0f), V2(R, 0.012f), V2(R, 0.02f), V2(0.02f, 0.02f), V2(0.02f, -0.02f)}, 36);
        PushMat pm2(MT.black);
        for (int i = 0; i < holes; i++) {
            float a = i * 2 * PI / holes;
            V3 c(R * 0.58f * std::cos(a), 0, R * 0.58f * std::sin(a));
            cyl(c + V3(0, -0.0205f, 0), c + V3(0, 0.0205f, 0), R * 0.19f, R * 0.19f, 12);
        }
        PushMat pm3(MT.steel);
        cyl(V3(0, -0.05f, 0), V3(0, 0.05f, 0), 0.02f, 0.02f, 10);
        boxC(V3(0.02f, 0.03f, 0), V3(0.006f, 0.022f, 0.006f)); // key
    };
    { ProtoScope ps; MP.pulleyBig = ps.id; pulley(0.17f, 5); }
    { ProtoScope ps; MP.pulleySmall = ps.id; pulley(0.05f, 0); }
}

// ---------------------------------------------------------------- animation of the hero machine
struct CircPin { double tBottom; int scoop; int pocket; double tArrive; };
static std::vector<CircPin> CIRC;
static const double WHEEL_HALF = (178.0 * PI / 180.0) / WH_W;
static const double DROP_T = 0.28, TIP_T = 0.50;
static const float BELT_LEN = std::sqrt((BELT_A.z - BELT_B.z) * (BELT_A.z - BELT_B.z) + (BELT_B.y - BELT_A.y) * (BELT_B.y - BELT_A.y));
static inline double wheelAngle(double tau) { return WH_W * (tau - T_HAND); }

static void planCirculation() {
    CIRC.clear();
    std::vector<double> want = {T_HAND - 2.75, T_HAND - 0.95};
    std::vector<double> col;
    for (int k = 0; k < 10; k++) {
        for (size_t s = 0; s < PHYS.rec.size(); s++)
            if (!PHYS.recActive[s][k]) { col.push_back(PHYS.t0 + s * PHYS.dtRec + 0.25); break; }
    }
    std::sort(col.begin(), col.end());
    if (col.size() > 7) col.resize(7);
    // pins queue behind the cushion: the wheel takes one every other scoop
    for (double c : col) want.push_back(std::max(c, want.back() + 1.1));
    std::vector<double> used;
    int pocket = TUR_BAKED + 1;
    for (double w : want) {
        // next scoop passing the bottom (-90 deg) after time w that is not taken
        double bestT = 1e30; int bestS = 0;
        for (int s = 0; s < 8; s++) {
            // angle(t) = wheelAngle(t) + s*45deg = -90deg + 360n
            double base = (-PI / 2 - s * PI / 4) / WH_W + T_HAND;
            double period = 2 * PI / WH_W;
            double n = std::ceil((w - base) / period);
            for (int extra = 0; extra < 6; extra++) {
                double t = base + (n + extra) * period;
                bool taken = false;
                for (double u : used) if (std::fabs(u - t) < 0.05) taken = true;
                if (!taken) { if (t < bestT) { bestT = t; bestS = s; } break; }
            }
        }
        used.push_back(bestT);
        CircPin c; c.tBottom = bestT; c.scoop = bestS; c.pocket = pocket++ % 10;
        c.tArrive = bestT + WHEEL_HALF + DROP_T + BELT_LEN / BELT_V + TIP_T;
        CIRC.push_back(c);
    }
}
// turret angle: indexes one pocket (36 deg) after each arrival
static float turretAngle(double tau) {
    float a = 0;
    for (auto &c : CIRC) a += smoother(0.12f, 0.50f, (float)(tau - c.tArrive));
    return -a * (2 * PI / 10);
}
static int arrivalsBefore(const CircPin &c) { int n = 0; for (auto &o : CIRC) if (o.tArrive < c.tArrive - 1e-6) n++; return n; }

static Pose circPinPose(int j, double tau, float cx) {
    const CircPin &c = CIRC[j];
    Pose P;
    P.p = V3(0, -50, 0);
    double t = tau - c.tBottom;
    if (t < 0) return P;
    Quat lieZ = qaxis(V3(1, 0, 0), PI / 2);          // pin axis (base->head) along +z
    V3 wc(cx, WH_Y, WH_Z);
    float zc = (SCOOP_Z0 + SCOOP_Z1) * 0.5f - 0.19f;  // pin base z while in a scoop
    if (t < WHEEL_HALF) {
        float th = -PI / 2 + WH_W * (float)t;
        // the pin settles toward the low side of its trough as the wheel turns
        float rr = WH_RPIN + 0.016f;
        V3 pos = wc + V3(std::cos(th) * rr, std::sin(th) * rr, 0);
        float sag = 0.014f * std::cos(th);            // lean against the guard on the way up
        pos.y -= std::fabs(sag);
        P.p = V3(pos.x, pos.y, zc);
        P.q = qmul(qaxis(V3(0, 0, 1), th * 1.7f + j), lieZ);
        return P;
    }
    t -= WHEEL_HALF;
    V3 A(cx + BELT_A.x, BELT_A.y, BELT_A.z), B(cx + BELT_B.x, BELT_B.y, BELT_B.z);
    V3 d = norm(B - A);
    V3 nrm = norm(cross(V3(1, 0, 0), d) * -1.0f);
    if (nrm.y < 0) nrm = -nrm;
    Quat lieBelt = qbasis(V3(1, 0, 0), -d, cross(V3(1, 0, 0), -d)); // pin +Y -> -d (base leads)
    float lead = 0.30f;                               // base position along the belt at t=0
    V3 onBelt0 = A + d * lead + nrm * 0.058f;
    if (t < DROP_T) {
        float u = (float)(t / DROP_T);
        float th = -PI / 2 + WH_W * (float)WHEEL_HALF;
        V3 p0 = V3(wc.x + std::cos(th) * WH_RPIN, wc.y + std::sin(th) * WH_RPIN, zc);
        V3 p1 = onBelt0;
        V3 pos = lerp(p0, p1, smooth(0, 1, u));
        pos.y = lerp(p0.y, p1.y, u * u);
        P.p = pos;
        P.q = qnlerp(qmul(qaxis(V3(0, 0, 1), th * 1.7f + j), lieZ), lieBelt, smooth(0, 1, u));
        // base leads up the belt, so the pin turns end for end as it drops: handled by the slerp above
        return P;
    }
    t -= DROP_T;
    double tb = (BELT_LEN - lead) / BELT_V;
    if (t < tb) {
        P.p = onBelt0 + d * (float)(BELT_V * t);
        P.q = lieBelt;
        return P;
    }
    t -= tb;
    // tip over the head pulley and drop base-first into the loading pocket
    V3 head = B + nrm * 0.058f;
    V3 pocketRear(cx, TUR_Y, TUR_Z + TUR_R);
    if (t < TIP_T) {
        float u = (float)(t / TIP_T);
        V3 pos = lerp(head, pocketRear, smooth(0, 1, u));
        pos.y = lerp(head.y, pocketRear.y, u * u) + 0.05f * std::sin(PI * std::min(1.0f, u * 2));
        P.p = pos;
        P.q = qnlerp(lieBelt, Quat(), smooth(0.0f, 0.75f, u));
        return P;
    }
    // riding in the turret
    int before = arrivalsBefore(c);
    float a = turretAngle(tau) + before * (2 * PI / 10); // this pin's pocket was at the rear when it arrived
    float bounce = 0.012f * std::exp(-(float)(t - TIP_T) * 9.0f) * std::fabs(std::sin((float)(t - TIP_T) * 30.0f));
    P.p = V3(cx + std::sin(a) * TUR_R, TUR_Y + bounce, TUR_Z + std::cos(a) * TUR_R);
    P.q = qaxis(V3(0, 1, 0), a + j);
    return P;
}

// ---------------------------------------------------------------- static structure for one lane
static void buildMachine(int li) {
    float cx = laneX(li);
    bool hero = li == HERO;
    Push base(xfT(cx, 0, 0));
    const float px = 0.865f - 0.035f; // posts stand on the kickbacks
    // frame
    {
        PushMat pm(MM.green, (float)li);
        static const float pz[3] = {17.98f, 19.34f, 20.62f};
        for (int s = -1; s <= 1; s += 2) {
            for (int k = 0; k < 3; k++) {
                rbox(V3(s * px - 0.025f, k == 2 ? -0.2f : 0.62f, pz[k] - 0.025f), V3(s * px + 0.025f, 1.98f, pz[k] + 0.025f), 0.005f);
                for (int b = 0; b < 2; b++) bolt(V3(s * (px - 0.025f), 0.72f + b * 1.18f, pz[k]), V3(-s, 0, 0));
            }
            rbox(V3(s * px - 0.025f, 1.93f, 17.955f), V3(s * px + 0.025f, 1.98f, 20.645f), 0.005f);   // top rail
            rbox(V3(s * px - 0.02f, 1.15f, 17.98f), V3(s * px + 0.02f, 1.19f, 20.62f), 0.004f);       // mid rail
            // diagonal braces at the back
            tube({V3(s * px, 1.17f, 19.36f), V3(s * px, 1.93f, 20.60f)}, 0.012f, 6);
        }
        for (int k = 0; k < 3; k++) rbox(V3(-px, 1.93f, pz[k] - 0.025f), V3(px, 1.98f, pz[k] + 0.025f), 0.005f); // cross members
        rbox(V3(-px, 1.15f, 20.60f), V3(px, 1.19f, 20.64f), 0.004f);
        rbox(V3(-px, 0.40f, 20.60f), V3(px, 0.44f, 20.64f), 0.004f);
    }
    // sweep rails along both kickbacks
    for (int s = -1; s <= 1; s += 2) {
        PushMat pm(MT.steel);
        cyl(V3(s * 0.74f, 0.74f, 17.86f), V3(s * 0.74f, 0.74f, 19.42f), 0.014f, 0.014f, 10);
        for (int k = 0; k < 3; k++) {
            PushMat pm2(MM.green, 2);
            float xa = s * 0.73f, xb = s * 0.84f;
            rbox(V3(std::min(xa, xb), 0.62f, 17.9f + k * 0.74f), V3(std::max(xa, xb), 0.73f, 17.94f + k * 0.74f), 0.003f);
        }
    }
    // table guide rods and lift links
    for (int s = -1; s <= 1; s += 2) {
        PushMat pm(MT.chrome);
        cyl(V3(s * 0.62f, 0.72f, 18.69f), V3(s * 0.62f, 1.95f, 18.69f), 0.014f, 0.014f, 12);
        PushMat pm2(MM.green, 6);
        rbox(V3(s * 0.62f - 0.03f, 0.66f, 18.66f), V3(s * px, 0.72f, 18.72f), 0.004f);
    }
    // turret support and index gear
    {
        PushMat pm(MM.green, 7);
        rbox(V3(-px, 1.93f, TUR_Z - 0.03f), V3(px, 1.98f, TUR_Z + 0.03f), 0.006f);
        cyl(V3(0, 1.90f, TUR_Z), V3(0, 2.02f, TUR_Z), 0.05f, 0.05f, 16);
        PushMat pm2(MT.brass);
        { Push pp(xfT(0, 0, TUR_Z)); lathe({V2(0.0f, 1.985f), V2(0.11f, 1.985f), V2(0.11f, 2.005f), V2(0.0f, 2.005f)}, 30); }
    }
    // distributor belt (static bed; the belt surface animates in the shader)
    {
        V3 A = BELT_A, B = BELT_B;
        V3 d = norm(B - A), n = norm(cross(d, V3(1, 0, 0)));
        if (n.y < 0) n = -n;
        float L = len(B - A);
        {
            PushMat pm(MM.belt);
            setUV(0, 0, L, 1);
            V3 r(0.085f, 0, 0);
            quad(A - r - d * 0.03f, B - r + d * 0.03f, B + r + d * 0.03f, A + r - d * 0.03f); // u along the belt
            V3 dn = n * -0.06f;
            quad(A + r + dn, B + r + dn, B - r + dn, A - r + dn);
            setUV();
        }
        {
            PushMat pm(MM.green, 8);
            for (int s = -1; s <= 1; s += 2) {
                V3 r(s * 0.10f, 0, 0);
                // side rails of the conveyor
                V3 a0 = A + r - n * 0.07f, b0 = B + r - n * 0.07f, a1 = A + r + n * 0.05f, b1 = B + r + n * 0.05f;
                V3 t(s * 0.008f, 0, 0);
                quad(a0 - t, b0 - t, b1 - t, a1 - t); quad(a0 + t, a1 + t, b1 + t, b0 + t);
                quad(a1 - t, b1 - t, b1 + t, a1 + t);
                tube({A + r * 1.0f - n * 0.03f, V3(s * px, 1.17f, A.z - 0.3f)}, 0.01f, 6);
                tube({B + r * 1.0f - n * 0.03f, V3(s * px, 1.93f, B.z + 0.25f)}, 0.01f, 6);
            }
            PushMat pm2(MT.steel);
            cyl(A + V3(-0.11f, 0, 0) - n * 0.03f, A + V3(0.11f, 0, 0) - n * 0.03f, 0.03f, 0.03f, 16);
            cyl(B + V3(-0.11f, 0, 0) - n * 0.03f, B + V3(0.11f, 0, 0) - n * 0.03f, 0.03f, 0.03f, 16);
        }
    }
    // guard band inside the wheel (keeps pins in their scoops on the way up) and wheel shroud
    {
        PushMat pm(MM.galv, 40);
        surf(28, 1, [&](float u, float v) {
            float th = lerp(-0.95f, 1.22f, u);
            float r = WH_RPIN - 0.085f;
            return V3(std::cos(th) * r, WH_Y + std::sin(th) * r, lerp(SCOOP_Z0 + 0.02f, SCOOP_Z1 - 0.02f, v));
        });
        // back board behind the wheel
        PushMat pm2(MM.green, 41);
        box(V3(-0.53f, -0.2f, WH_Z + 0.16f), V3(0.80f, 1.15f, WH_Z + 0.19f));
        // pit ramp into the wheel
        PushMat pm3(MT.steelDark);
        quad(V3(-0.53f, PIT_Y, CUSH_Z + 0.05f), V3(-0.53f, PIT_Y + 0.07f, SCOOP_Z0 + 0.3f), V3(0.795f, PIT_Y + 0.07f, SCOOP_Z0 + 0.3f), V3(0.795f, PIT_Y, CUSH_Z + 0.05f));
        quad(V3(-0.53f, PIT_Y, CUSH_Z + 0.05f), V3(-0.53f, PIT_Y, SCOOP_Z0 + 0.3f), V3(-0.53f, PIT_Y + 0.07f, SCOOP_Z0 + 0.3f), V3(-0.53f, PIT_Y, CUSH_Z + 0.05f));
    }
    // ball lift tower behind the wheel, at the ball exit corner
    {
        PushMat pm(MM.green, 44);
        rbox(V3(-0.795f, PIT_Y, 20.50f), V3(-0.53f, 1.05f, 20.82f), 0.012f);
        PushMat pm2(MT.black);
        box(V3(-0.775f, PIT_Y + 0.005f, 20.492f), V3(-0.55f, 0.20f, 20.51f));
        PushMat pm3(MM.yellow);
        rbox(V3(-0.795f, 0.20f, 20.488f), V3(-0.53f, 0.26f, 20.502f), 0.003f);
        PushMat pm4(MT.steel);
        tube({V3(-0.545f, PIT_Y + 0.02f, CUSH_Z), V3(-0.545f, PIT_Y + 0.02f, 20.5f)}, 0.012f, 6); // ball guide rail
    }
    // drive: motor, gearbox, belts, chain — on a shelf at the top rear, -x side
    {
        float sx = -0.45f, sy = 1.26f, sz = 19.95f;
        {
            PushMat pm(MM.green, 50);
            rbox(V3(-px, sy - 0.03f, 19.50f), V3(-0.12f, sy, 20.50f), 0.006f);          // shelf
            rbox(V3(sx - 0.14f, sy, sz + 0.12f), V3(sx + 0.14f, sy + 0.30f, sz + 0.42f), 0.02f); // gearbox
            for (int i = 0; i < 4; i++) bolt(V3(sx - 0.1f + 0.2f * (i & 1), sy + 0.30f, sz + 0.18f + 0.18f * (i >> 1)), V3(0, 1, 0), 0.012f);
        }
        {   // motor: finned cylinder along x
            PushMat pm(MT.steelDark);
            V3 m0(sx - 0.17f, sy + 0.11f, sz - 0.22f), m1(sx + 0.13f, sy + 0.11f, sz - 0.22f);
            cyl(m0, m1, 0.095f, 0.095f, 24);
            for (int i = 0; i < 9; i++) {
                float x = lerp(m0.x + 0.03f, m1.x - 0.03f, i / 8.0f);
                cyl(V3(x, m0.y, m0.z), V3(x + 0.008f, m0.y, m0.z), 0.108f, 0.108f, 24);
            }
            rbox(V3(sx - 0.12f, sy, sz - 0.30f), V3(sx + 0.08f, sy + 0.03f, sz - 0.14f), 0.004f);
            PushMat pm2(MM.nameplate);
            // (nameplate decal added by the signage pass)
        }
        {   // V-belt between the motor pulley and the big gearbox pulley
            PushMat pm(MM.chain);
            V3 a(sx + 0.19f, sy + 0.11f, sz - 0.22f), b(sx + 0.19f, sy + 0.16f, sz + 0.27f);
            float ra = 0.05f, rb = 0.17f;
            float L = len(b - a);
            V3 d = (b - a) / L, n(0, d.z, -d.y);
            setUV(0, 0, L, 1);
            for (int s = -1; s <= 1; s += 2) {
                V3 p0 = a + n * (ra * s), p1 = b + n * (rb * s);
                V3 w(0.012f, 0, 0);
                quad(p0 - w, p1 - w, p1 + w, p0 + w);
                quad(p0 + w + n * (-0.008f * s), p1 + w + n * (-0.008f * s), p1 - w + n * (-0.008f * s), p0 - w + n * (-0.008f * s));
            }
            setUV();
        }
        {   // chain from the gearbox down to the wheel hub
            PushMat pm(MM.chain);
            V3 a(sx - 0.05f, sy + 0.15f, WH_Z + 0.11f), b(0.0f, WH_Y, WH_Z + 0.11f);
            float L = len(b - a);
            V3 d = (b - a) / L, n(d.y, -d.x, 0);
            setUV(0, 0, L, 1);
            for (int s = -1; s <= 1; s += 2) {
                V3 p0 = a + n * (0.05f * s), p1 = b + n * (0.065f * s);
                V3 w(0, 0, 0.009f);
                quad(p0 - w, p1 - w, p1 + w, p0 + w);
            }
            setUV();
            PushMat pm2(MT.steel);
            cyl(a - V3(0, 0, 0.012f), a + V3(0, 0, 0.012f), 0.05f, 0.05f, 16);
            cyl(b - V3(0, 0, 0.012f), b + V3(0, 0, 0.012f), 0.065f, 0.065f, 18);
            cyl(a - V3(0, 0, 0.012f), V3(a.x, a.y, sz + 0.40f), 0.014f, 0.014f, 8);
        }
        {   // control box with pilot lamps, conduit
            PushMat pm(MT.steelDark);
            rbox(V3(px - 0.20f, 1.30f, 20.40f), V3(px - 0.03f, 1.78f, 20.60f), 0.008f);
            PushMat pm2(MT.steel);
            tube(smoothPath({V3(px - 0.11f, 1.78f, 20.50f), V3(px - 0.11f, 1.90f, 20.50f), V3(px - 0.2f, 1.96f, 20.45f), V3(-0.3f, 1.96f, 20.3f), V3(sx, 1.70f, sz - 0.22f), V3(sx, sy + 0.2f, sz - 0.22f)}, 6), 0.011f, 8);
            tube(smoothPath({V3(px - 0.06f, 1.30f, 20.5f), V3(px - 0.06f, 0.9f, 20.55f), V3(px - 0.02f, 0.2f, 20.66f), V3(px - 0.02f, -0.18f, 20.75f)}, 6), 0.009f, 8);
            { PushMat l(hero ? MM.lampGreen : MM.lampRed); sphere(V3(px - 0.15f, 1.70f, 20.395f), 0.014f, 10); }
            { PushMat l(MM.lampRed); sphere(V3(px - 0.08f, 1.70f, 20.395f), 0.014f, 10); }
            PushMat pm3(MT.black);
            cyl(V3(px - 0.115f, 1.55f, 20.40f), V3(px - 0.115f, 1.55f, 20.375f), 0.022f, 0.016f, 12);
        }
    }
    // camshaft bearings on the -x side frame
    for (int k = 0; k < 2; k++) {
        PushMat pm(MM.green, 55);
        float z = 18.88f + k * 0.94f;
        rbox(V3(-px, 1.56f, z - 0.03f), V3(-0.58f, 1.68f, z + 0.03f), 0.006f);
    }
    if (!hero) {
        // idle machine: everything parked, a full rack waiting in the table
        bake(MP.wheel, xfT(0, WH_Y, WH_Z) * xfRZ(li * 0.37f));
        bake(MP.turret, xfT(0, TUR_Y, TUR_Z) * xfRY(li * 0.6f));
        bake(MP.table, xfT(0, TABLE_UP, 0));
        bake(MP.sweep, xfT(0, 0.60f, 17.99f));
        bake(MP.carriage, xfT(0, 0, 17.99f));
        bake(MP.cushion, xfT(0, 0.66f, CUSH_Z));
        bake(MP.cam, xfT(-0.62f, 1.62f, 19.35f) * xfRY(PI / 2) * xfRX(li * 1.1f));
        bake(MP.pulleyBig, xfT(-0.45f + 0.19f, 1.26f + 0.16f, 19.95f + 0.27f));
        bake(MP.pulleySmall, xfT(-0.45f + 0.19f, 1.26f + 0.11f, 19.95f - 0.22f));
        for (int k = 0; k < 10; k++) { V3 s = pinSpot(k); bake(MP.pinLo, xfT(s.x, TABLE_UP, s.z) * xfRY(k * 1.3f + li), li * 10.0f + k + 300); }
    }
}

static void addHeroMachineDynamics() {
    float cx = laneX(HERO);
    planCirculation();

    addDyn(MP.wheel, 0, [cx](double t) { Pose p; p.p = V3(cx, WH_Y, WH_Z); p.q = qaxis(V3(0, 0, 1), (float)wheelAngle(t)); return p; });
    addDyn(MP.turret, 0, [cx](double t) { Pose p; p.p = V3(cx, TUR_Y, TUR_Z); p.q = qaxis(V3(0, 1, 0), turretAngle(t)); return p; });
    addDyn(MP.table, 0, [cx](double t) { Pose p; p.p = V3(cx, TABLE_UP * (1 - tableDown(t)), 0); return p; });
    addDyn(MP.sweep, 0, [cx](double t) { Pose p; p.p = V3(cx, sweepY(t), sweepZ(t)); return p; });
    addDyn(MP.carriage, 0, [cx](double t) { Pose p; p.p = V3(cx, 0, sweepZ(t)); return p; });
    addDyn(MP.cushion, 0, [cx](double t) {
        Pose p; p.p = V3(cx, 0.66f, CUSH_Z);
        float dt = (float)(t - PHYS.tCushion);
        float a = (PHYS.tCushion > 0 && dt > 0) ? 0.30f * std::exp(-2.6f * dt) * std::sin(8.5f * dt) : 0.0f;
        for (auto &e : PHYS.events) // flying pins make it shiver
            if (e.kind == 2 && e.pos.z > CUSH_Z - 0.2f && t > e.t && t - e.t < 1.0) a += 0.02f * std::min(e.strength, 4.0f) * std::exp(-5.0f * (float)(t - e.t)) * std::sin(14.0f * (float)(t - e.t));
        p.q = qaxis(V3(1, 0, 0), -a);
        return p;
    });
    addDyn(MP.cam, 0, [cx](double t) { Pose p; p.p = V3(cx - 0.62f, 1.62f, 19.35f); p.q = qmul(qaxis(V3(0, 1, 0), PI / 2), qaxis(V3(1, 0, 0), (float)(1.05 * (t - T_HAND)))); return p; });
    addDyn(MP.pulleyBig, 0, [cx](double t) { Pose p; p.p = V3(cx - 0.45f + 0.19f, 1.26f + 0.16f, 19.95f + 0.27f); p.q = qaxis(V3(1, 0, 0), (float)(5.3 * t)); return p; });
    addDyn(MP.pulleySmall, 0, [cx](double t) { Pose p; p.p = V3(cx - 0.45f + 0.19f, 1.26f + 0.11f, 19.95f - 0.22f); p.q = qaxis(V3(1, 0, 0), (float)(18.0 * t)); return p; });
    // the ten pins being bowled at, and the ball
    for (int k = 0; k < 10; k++) addDyn(P_PIN, 500.0f + k, [k](double t) { return pinPose(k, t); });
    addDyn(P_BALL, 2, [](double t) { return ballPose(t); });
    // the next rack, riding the table until it is set
    for (int k = 0; k < 10; k++)
        addDyn(P_PIN, 600.0f + k, [k, cx](double t) {
            Pose p; V3 s = pinSpot(k);
            float y = newPinsReleased(t) ? 0.0f : TABLE_UP * (1 - tableDown(t));
            p.p = V3(cx + s.x, y, s.z);
            p.q = qaxis(V3(0, 1, 0), k * 2.1f);
            return p;
        });
    // pins circulating through the elevator, belt and turret
    for (size_t j = 0; j < CIRC.size(); j++) addDyn(P_PIN, 700.0f + j, [j, cx](double t) { return circPinPose((int)j, t, cx); });
}

