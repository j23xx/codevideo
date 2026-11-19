// physics.h — hand-written rigid-body simulation of the ball and the ten pins on the hero lane.
// Pins are compound bodies of spheres along their axis; impulses with restitution and Coulomb friction.
#pragma once
#include "layout.h"

// ---- world-time schedule of the shot (seconds of world time; the film remaps video time onto this)
static const double T_REL = 15.60;            // ball crosses the foul line
static const double BALL_V0 = 8.7, BALL_DEC = 0.36;
static const float HAND_Z = 17.6f;
static const double T_HAND = T_REL + (BALL_V0 - std::sqrt(BALL_V0 * BALL_V0 - 2 * BALL_DEC * 17.6)) / BALL_DEC;  // ball reaches z = 17.6 (solves v0 t - d t^2/2 = 17.6); the simulation takes over
static const float PIN_COM = 0.147f;          // centre of mass above the base
static const float CUSH_Z = 19.90f, CUSH_BOTTOM = PIT_Y + 0.19f;
static const float CUSH_GAP_LX = -0.545f;       // the cushion stops short of the -x kickback: the ball exit to the lift

// sweep (rake) board: bottom edge height and z position
static inline float sweepY(double tau) {
    double t = tau - T_HAND;
    float down = smoother(0.75f, 1.05f, (float)t) - smoother(5.95f, 6.35f, (float)t);
    return lerp(0.60f, 0.015f, down);
}
static inline float sweepZ(double tau) {
    double t = tau - T_HAND;
    float back = smoother(1.45f, 2.45f, (float)t) - smoother(2.55f, 3.45f, (float)t);
    return lerp(17.99f, 19.20f, back);
}
// pin table: 0 = up in the machine, 1 = pins standing on the deck
static inline float tableDown(double tau) {
    double t = tau - T_HAND;
    return smoother(3.55f, 4.55f, (float)t) - smoother(4.95f, 5.85f, (float)t);
}
static inline bool newPinsReleased(double tau) { return tau - T_HAND > 4.75; }

struct Body {
    V3 x, v, w;
    Quat q;
    float invM = 0;
    V3 invI;          // body-frame inverse inertia (diagonal)
    bool awake = false, active = true;
    std::vector<V3> sc; // collision spheres, body frame
    std::vector<float> sr;
    float bound = 0;
    bool isBall = false;
};
struct Contact {
    int a, b;         // b = -1: environment
    V3 p, n;          // n points from b toward a
    float depth;
    V3 vs;            // surface velocity of the environment
    float e, mu;
};
struct PhysEvent { double t; int kind; float strength; V3 pos; }; // kind 0 ball-pin, 1 pin-pin, 2 pin-floor/wall, 3 ball-cushion

struct Physics {
    std::vector<Body> B;
    double t0 = T_HAND, dtRec = 1.0 / 1000.0;
    std::vector<std::vector<Pose>> rec; // rec[step][body], pose of body origin (COM)
    std::vector<std::vector<uint8_t>> recActive;
    std::vector<PhysEvent> events;
    double tImpact = -1, tCushion = -1;
    float cx = 0;
    int standing = 0;

    V3 applyInvI(const Body &b, V3 v) const {
        V3 l = qrot(qconj(b.q), v);
        l = V3(l.x * b.invI.x, l.y * b.invI.y, l.z * b.invI.z);
        return qrot(b.q, l);
    }
    V3 velAt(const Body &b, V3 p) const { return b.v + cross(b.w, p - b.x); }

    void init(float laneCx, float entryLx, float slope, float speed) {
        cx = laneCx;
        B.clear();
        static const float sh[10] = {0.040f, 0.075f, 0.114f, 0.150f, 0.185f, 0.215f, 0.250f, 0.285f, 0.320f, 0.3495f};
        static const float sr[10] = {0.040f, 0.053f, 0.0605f, 0.057f, 0.046f, 0.033f, 0.0235f, 0.0255f, 0.0315f, 0.0315f};
        for (int k = 0; k < 10; k++) {
            Body b;
            V3 s = pinSpot(k);
            b.x = V3(cx + s.x, PIN_COM, s.z);
            float m = 1.55f;
            b.invM = 1 / m;
            b.invI = V3(1 / 0.0150f, 1 / 0.0019f, 1 / 0.0150f);
            for (int i = 0; i < 10; i++) { b.sc.push_back(V3(0, sh[i] - PIN_COM, 0)); b.sr.push_back(sr[i]); }
            for (int i = 0; i < 8; i++) { // base ring
                float a = 2 * PI * i / 8;
                b.sc.push_back(V3(0.0245f * std::cos(a), 0.0015f - PIN_COM, 0.0245f * std::sin(a)));
                b.sr.push_back(0.0015f);
            }
            b.bound = 0.26f;
            b.q = qaxis(V3(0, 1, 0), hash1(k * 7.7f + 1.3f) * 6.28f);
            B.push_back(b);
        }
        Body ball;
        ball.isBall = true;
        float m = 7.0f;
        ball.invM = 1 / m;
        float I = 0.4f * m * BALL_R * BALL_R;
        ball.invI = V3(1 / I, 1 / I, 1 / I);
        ball.sc.push_back(V3(0, 0, 0)); ball.sr.push_back(BALL_R);
        ball.bound = BALL_R;
        ball.x = V3(cx + entryLx, BALL_R, HAND_Z);
        V3 dir = norm(V3(slope, 0, 1));
        ball.v = dir * speed;
        ball.w = cross(V3(0, 1, 0), ball.v) / BALL_R + V3(0, 3.0f, 0); // rolling plus a little side spin
        ball.awake = true;
        B.push_back(ball);
    }

    void envContacts(int bi, double tau, std::vector<Contact> &C) {
        Body &b = B[bi];
        const float hw = LANE_W * 0.5f;
        const float kick = PITCH * 0.5f - 0.07f;
        float sy = sweepY(tau), sz = sweepZ(tau);
        float svz = (sweepZ(tau + 0.001) - sweepZ(tau - 0.001)) / 0.002f;
        for (size_t i = 0; i < b.sc.size(); i++) {
            V3 c = b.x + qrot(b.q, b.sc[i]);
            float r = b.sr[i];
            float lx = c.x - cx;
            bool onDeck = std::fabs(lx) <= hw && c.z <= DECK_END;
            // floors
            if (onDeck && c.y > -0.02f) {
                if (c.y - r < 0) C.push_back({bi, -1, V3(c.x, 0, c.z), V3(0, 1, 0), r - c.y, V3(), b.isBall ? 0.05f : 0.62f, b.isBall ? 0.12f : 0.40f});
            } else if (c.z > DECK_END) {
                if (c.y - r < PIT_Y) C.push_back({bi, -1, V3(c.x, PIT_Y, c.z), V3(0, 1, 0), PIT_Y + r - c.y, V3(b.isBall ? -0.9f : 0.0f, 0, 0.55f), 0.12f, 0.9f});
            } else if (!onDeck) {
                float fy = -0.085f;
                if (c.y - r < fy) C.push_back({bi, -1, V3(c.x, fy, c.z), V3(0, 1, 0), fy + r - c.y, V3(), 0.25f, 0.35f});
            }
            // deck side faces and tail (for things below deck level)
            if (c.y < 0 && c.z <= DECK_END + r && std::fabs(lx) < hw + r) {
                float dSide = hw + r - std::fabs(lx), dTail = DECK_END + r - c.z;
                if (std::fabs(lx) > hw - 0.01f && dSide < dTail) C.push_back({bi, -1, c, V3(lx > 0 ? 1.f : -1.f, 0, 0), dSide, V3(), 0.3f, 0.3f});
                else if (c.z > DECK_END - 0.01f) C.push_back({bi, -1, c, V3(0, 0, 1), dTail, V3(), 0.3f, 0.3f});
            }
            // kickbacks
            if (c.z > KICK_Z0 - 0.1f) {
                if (lx + r > kick) C.push_back({bi, -1, c, V3(-1, 0, 0), lx + r - kick, V3(), 0.6f, 0.3f});
                if (lx - r < -kick) C.push_back({bi, -1, c, V3(1, 0, 0), -kick - (lx - r), V3(), 0.6f, 0.3f});
            }
            // pit cushion: stops the ball and anything flying, lets fallen pins pass underneath
            if (c.y + r > CUSH_BOTTOM && c.z < CUSH_Z + 0.05f && lx > CUSH_GAP_LX) {
                float dy = std::max(0.0f, CUSH_BOTTOM - c.y);
                float re = std::sqrt(std::max(r * r - dy * dy, 0.0f));
                if (c.z + re > CUSH_Z) {
                    C.push_back({bi, -1, c, V3(0, 0, -1), c.z + re - CUSH_Z, V3(), 0.03f, b.isBall ? 0.02f : 0.4f});
                    if (b.isBall && tCushion < 0) { tCushion = tau; events.push_back({tau, 3, len(b.v), c}); }
                }
            }
            // back board (hard stop behind the cushion)
            if (!b.isBall && c.z + r > PIT_END + 0.42f) C.push_back({bi, -1, c, V3(0, 0, -1), c.z + r - (PIT_END + 0.42f), V3(), 0.1f, 0.5f});
            // sweep board pushing dead wood into the pit
            if (c.y - r < sy + 0.26f && c.y + r > sy && c.z - r < sz && c.z > sz - 0.12f && std::fabs(lx) < kick)
                C.push_back({bi, -1, c, V3(0, 0, 1), sz - (c.z - r), V3(0, 0, svz), 0.0f, 0.3f});
        }
    }

    void pairContacts(int ai, int bi, std::vector<Contact> &C) {
        Body &a = B[ai], &b = B[bi];
        V3 d = a.x - b.x;
        float bd = a.bound + b.bound;
        if (dot(d, d) > bd * bd) return;
        bool ballPair = a.isBall || b.isBall;
        for (size_t i = 0; i < a.sc.size(); i++) {
            if (a.sr[i] < 0.01f) continue;
            V3 ca = a.x + qrot(a.q, a.sc[i]);
            for (size_t j = 0; j < b.sc.size(); j++) {
                if (b.sr[j] < 0.01f) continue;
                V3 cb = b.x + qrot(b.q, b.sc[j]);
                V3 dd = ca - cb;
                float rr = a.sr[i] + b.sr[j];
                float d2 = dot(dd, dd);
                if (d2 >= rr * rr || d2 < 1e-12f) continue;
                float l = std::sqrt(d2);
                V3 n = dd / l;
                C.push_back({ai, bi, cb + n * (b.sr[j] - (rr - l) * 0.5f), n, rr - l, V3(), ballPair ? 0.78f : 0.76f, ballPair ? 0.25f : 0.34f});
            }
        }
    }

    void solve(std::vector<Contact> &C, double tau) {
        for (int it = 0; it < 6; it++)
            for (auto &c : C) {
                Body &a = B[c.a];
                Body *b = c.b >= 0 ? &B[c.b] : nullptr;
                V3 ra = c.p - a.x, rb = b ? c.p - b->x : V3();
                V3 vr = velAt(a, c.p) - (b ? velAt(*b, c.p) : c.vs);
                float vn = dot(vr, c.n);
                if (vn > 0) continue;
                // wake sleepers on a real hit
                if (!a.awake) a.awake = true;
                if (b && !b->awake) b->awake = true;
                float invA = a.invM + dot(cross(applyInvI(a, cross(ra, c.n)), ra), c.n);
                float invB = b ? b->invM + dot(cross(applyInvI(*b, cross(rb, c.n)), rb), c.n) : 0;
                float e = (vn < -0.35f) ? c.e : 0.0f;
                float jn = -(1 + e) * vn / (invA + invB);
                V3 J = c.n * jn;
                a.v += J * a.invM; a.w += applyInvI(a, cross(ra, J));
                if (b) { b->v -= J * b->invM; b->w -= applyInvI(*b, cross(rb, J)); }
                if (it == 0 && vn < -0.6f) {
                    int kind = (b && (a.isBall || b->isBall)) ? 0 : (b ? 1 : 2);
                    if (kind == 0 && tImpact < 0) tImpact = tau;
                    events.push_back({tau, kind, -vn, c.p});
                }
                // friction
                vr = velAt(a, c.p) - (b ? velAt(*b, c.p) : c.vs);
                V3 vt = vr - c.n * dot(vr, c.n);
                float vtl = len(vt);
                if (vtl > 1e-5f) {
                    V3 tdir = vt / vtl;
                    float invTA = a.invM + dot(cross(applyInvI(a, cross(ra, tdir)), ra), tdir);
                    float invTB = b ? b->invM + dot(cross(applyInvI(*b, cross(rb, tdir)), rb), tdir) : 0;
                    float jt = std::min(vtl / (invTA + invTB), c.mu * jn);
                    V3 Jt = tdir * (-jt);
                    a.v += Jt * a.invM; a.w += applyInvI(a, cross(ra, Jt));
                    if (b) { b->v -= Jt * b->invM; b->w -= applyInvI(*b, cross(rb, Jt)); }
                }
            }
    }

    void run(double duration) {
        const double dt = 1.0 / 4000.0;
        int nsteps = (int)(duration / dt);
        int recEvery = (int)llround(dtRec / dt);
        std::vector<Contact> C;
        Rng shake(77);
        rec.clear(); recActive.clear();
        for (int s = 0; s <= nsteps; s++) {
            double tau = t0 + s * dt;
            if (s % recEvery == 0) {
                std::vector<Pose> fr(B.size());
                std::vector<uint8_t> ac(B.size());
                for (size_t i = 0; i < B.size(); i++) { fr[i].q = B[i].q; fr[i].p = B[i].x; ac[i] = B[i].active; }
                rec.push_back(fr); recActive.push_back(ac);
            }
            for (auto &b : B) {
                if (!(b.awake && b.active)) continue;
                b.v += V3(0, -9.81f, 0) * (float)dt;
                if (b.x.z > DECK_END + 0.05f && b.x.y < PIT_Y + 0.2f) {
                    if (b.isBall) b.v += V3(b.x.x - cx > -0.67f ? -2.2f : 0.0f, 0, 0.6f) * (float)dt;          // pit conveyor carries the ball to its exit
                    else {                                                         // shaker board keeps the pins moving back
                        V3 j(shake.g(), 0.3f * shake.g(), shake.g());
                        b.v += j * (0.9f * std::sqrt((float)dt)) + V3(0, 0, 0.8f) * (float)dt;
                        b.w += V3(shake.g(), shake.g(), shake.g()) * (6.0f * std::sqrt((float)dt));
                    }
                }
            }
            C.clear();
            for (size_t i = 0; i < B.size(); i++) if (B[i].active && B[i].awake) envContacts((int)i, tau, C);
            for (size_t i = 0; i < B.size(); i++)
                for (size_t j = i + 1; j < B.size(); j++)
                    if (B[i].active && B[j].active && (B[i].awake || B[j].awake)) pairContacts((int)i, (int)j, C);
            solve(C, tau);
            for (auto &c : C) { // positional correction
                Body &a = B[c.a];
                Body *b = c.b >= 0 ? &B[c.b] : nullptr;
                if (!a.awake || (b && !b->awake)) continue;
                float corr = std::max(c.depth - 0.0004f, 0.0f) * 0.25f;
                float wa = a.invM, wb = b ? b->invM : 0;
                a.x += c.n * (corr * wa / (wa + wb));
                if (b) b->x -= c.n * (corr * wb / (wa + wb));
            }
            for (auto &b : B) {
                if (!b.awake || !b.active) continue;
                b.x += b.v * (float)dt;
                b.q = qintegrate(b.q, b.w, (float)dt);
                float damp = b.isBall ? 0.05f : 0.35f;      // rolling resistance / air
                b.w *= (1 - damp * (float)dt);
                b.v *= (1 - 0.02f * (float)dt);
                // collected: pins carried under the cushion to the elevator, ball through the ball door
                if (!b.isBall && b.x.z > PIT_END + 0.22f) b.active = false;
                if (b.isBall && b.x.z > 20.64f) b.active = false;
            }
        }
        standing = 0;
        for (int k = 0; k < 10; k++) {
            V3 up = qrot(B[k].q, V3(0, 1, 0));
            if (B[k].active && up.y > 0.9f && B[k].x.z < DECK_END && std::fabs(B[k].x.y - PIN_COM) < 0.02f) standing++;
        }
    }

    bool sample(int body, double tau, Pose &out) const {
        double f = (tau - t0) / dtRec;
        if (f <= 0) { out = rec[0][body]; return recActive[0][body]; }
        int n = (int)rec.size();
        if (f >= n - 1) { out = rec[n - 1][body]; return recActive[n - 1][body]; }
        int i = (int)f;
        float u = (float)(f - i);
        out.p = lerp(rec[i][body].p, rec[i + 1][body].p, u);
        out.q = qnlerp(rec[i][body].q, rec[i + 1][body].q, u);
        return recActive[i][body];
    }
};
static Physics PHYS;

// ball centre on the lane before the hand-over (kinematic skid/hook path); lx relative to lane centre
static Pchip BALL_PATH;
static float BALL_ENTRY_LX = -0.125f, BALL_ENTRY_SLOPE = 0.085f;
static void buildBallPath() {
    BALL_PATH = Pchip();
    BALL_PATH.add(-3.0f, -0.165f); BALL_PATH.add(0, -0.20f); BALL_PATH.add(4, -0.285f); BALL_PATH.add(8, -0.355f); BALL_PATH.add(12, -0.40f);
    BALL_PATH.add(14, -0.385f); BALL_PATH.add(16, -0.275f);
    BALL_PATH.add(HAND_Z, BALL_ENTRY_LX); BALL_PATH.add(HAND_Z + 2, BALL_ENTRY_LX + 2 * BALL_ENTRY_SLOPE);
    BALL_PATH.build();
}
static inline double ballZ(double tau) { double t = tau - T_REL; return 0.0 + BALL_V0 * t - 0.5 * BALL_DEC * t * t; }
static inline double ballSpeed(double tau) { return BALL_V0 - BALL_DEC * (tau - T_REL); }

// orientation of the ball while it is on the kinematic path: integrate spin numerically once
static std::vector<Quat> BALL_SPIN;
static const double BALL_T0 = T_REL - 0.45; // ball enters from behind the camera
static void buildBallSpin() {
    BALL_SPIN.clear();
    Quat q = qaxis(norm(V3(0.3f, 1, 0.2f)), 1.1f);
    double dt = 1.0 / 1000.0;
    for (double t = BALL_T0; t <= T_HAND + dt; t += dt) {
        BALL_SPIN.push_back(q);
        double z = ballZ(t), v = ballSpeed(t);
        float slope = (BALL_PATH.eval((float)z + 0.05f) - BALL_PATH.eval((float)z - 0.05f)) / 0.1f;
        V3 vel = norm(V3(slope, 0, 1)) * (float)v;
        V3 roll = cross(V3(0, 1, 0), vel) / BALL_R;
        // early on the ball skids: it spins about a tilted axis more slowly than it travels
        float k = smooth(4.0f, 15.5f, (float)z);
        V3 skid = norm(V3(0.75f, 0.35f, -0.55f)) * 31.0f;
        V3 w = lerp(skid, roll + V3(0, 3.0f, 0), k);
        q = qintegrate(q, w, (float)dt);
    }
}

static Pose ballPose(double tau, bool *visible = nullptr) {
    Pose p;
    if (visible) *visible = true;
    if (tau < BALL_T0) { if (visible) *visible = false; p.p = V3(0, -50, 0); return p; }
    if (tau < T_HAND) {
        double z = ballZ(tau);
        p.p = V3(PHYS.cx + BALL_PATH.eval((float)z), BALL_R, (float)z);
        double f = (tau - BALL_T0) * 1000.0;
        int i = std::min((int)f, (int)BALL_SPIN.size() - 2);
        p.q = qnlerp(BALL_SPIN[i], BALL_SPIN[i + 1], (float)(f - i));
        return p;
    }
    bool act = PHYS.sample(10, tau, p);
    // continue the spin orientation from the kinematic phase
    p.q = qmul(p.q, BALL_SPIN.back());
    if (!act) { if (visible) *visible = false; p.p = V3(0, -50, 0); }
    return p;
}
// pin pose for rendering (prototype origin is the centre of the base)
static Pose pinPose(int k, double tau, bool *visible = nullptr) {
    Pose p;
    bool act = PHYS.sample(k, tau, p);
    p.p = p.p - qrot(p.q, V3(0, PIN_COM, 0));
    if (visible) *visible = act;
    if (!act) p.p = V3(0, -50, 0);
    return p;
}

static void runPhysics(bool verbose = false) {
    // search a small neighbourhood of entry parameters for the most satisfying strike (deterministic)
    float bestLx = BALL_ENTRY_LX, bestSl = BALL_ENTRY_SLOPE;
    int best = 9999;
    static const float lxs[] = {-0.125f, -0.118f, -0.132f, -0.110f, -0.140f, -0.104f};
    static const float sls[] = {0.085f, 0.095f, 0.075f, 0.105f};
    for (float sl : sls) {
        for (float lx : lxs) {
            PHYS = Physics();
            PHYS.init(laneX(HERO), lx, sl, (float)(BALL_V0 - BALL_DEC * (T_HAND - T_REL)));
            PHYS.run(5.2);
            int left = 0;
            for (int k = 0; k < 10; k++) left += PHYS.B[k].active;
            int score = PHYS.standing * 100 + left * 10 + (PHYS.B[10].active ? 5 : 0);
            if (verbose) fprintf(stderr, "physics try lx=%.3f slope=%.3f -> standing %d, left in pit %d, ball %d\n", lx, sl, PHYS.standing, left, (int)PHYS.B[10].active);
            if (score < best) { best = score; bestLx = lx; bestSl = sl; }
            if (best == 0) break;
        }
        if (best == 0) break;
    }
    BALL_ENTRY_LX = bestLx; BALL_ENTRY_SLOPE = bestSl;
    PHYS = Physics();
    PHYS.init(laneX(HERO), bestLx, bestSl, (float)(BALL_V0 - BALL_DEC * (T_HAND - T_REL)));
    PHYS.run(7.5);
    buildBallPath();
    buildBallSpin();
    if (verbose) fprintf(stderr, "physics: lx=%.3f slope=%.3f standing=%d impact=%.4f cushion=%.4f events=%zu\n", bestLx, bestSl, PHYS.standing, PHYS.tImpact - T_HAND, PHYS.tCushion - T_HAND, PHYS.events.size());
}
