// render.metal — the whole renderer: a path tracer in one Metal compute kernel with a hand-written
// BVH traversal (no Metal ray-tracing API), procedural materials, plus the denoise/resolve kernels.
#include <metal_stdlib>
using namespace metal;

struct Node { float4 mn; float4 mx; };
struct Tri { float4 v0; float4 e1; float4 e2; };            // v0.w = material (int bits), e1.w = flags (int bits), e2.w = var
struct TriS { float4 n0; float4 n1; float4 n2; float4 uv01; float4 uv2; };
struct Mat { float4 base; float4 emit; float4 prm; float4 prm2; }; // base: rgb,rough | emit: rgb,mode | prm: metal,spec,kind,p0 | prm2: trans,p1,p2,p3
struct Light { float4 pos; float4 u; float4 v; float4 nrm; float4 emit; }; // pos.w type | u.w area | v.w twoSided | nrm.w power | emit.w radius
struct Inst { float4 q0; float4 p0; float4 q1; float4 p1; float4 mn; float4 mx; }; // p0.w = root (int bits), p1.w = var
struct Uni {
    float4 camP0, camQ0, camP1, camQ1;
    float4 lens;   // k (stereographic), aperture radius, focus distance, aspect (h/w)
    float4 misc;   // world time, haze sigma, clamp, cap on directly seen emission (0: none)
    float4 haze;   // rgb haze ambient, unused
    float4 sky;    // sky scale, sun on/off
    int4 dims;     // w, h, sampleBase, samples this pass
    int4 counts;   // nLights, nInst, maxBounce, frame seed
};

struct Hit { float t; int tri; float u; float v; int inst; };

// ------------------------------------------------------------------ rng / hashing / noise
static inline uint pcg(thread uint &s) {
    s = s * 747796405u + 2891336453u;
    uint w = ((s >> ((s >> 28u) + 4u)) ^ s) * 277803737u;
    return (w >> 22u) ^ w;
}
static inline float rnd(thread uint &s) { return float(pcg(s) >> 8) * (1.0f / 16777216.0f); }
static inline uint hashu(uint x) { x ^= x >> 16; x *= 0x7feb352du; x ^= x >> 15; x *= 0x846ca68bu; x ^= x >> 16; return x; }

static inline float hash11(float p) { p = fract(p * 0.1031f); p *= p + 33.33f; p *= p + p; return fract(p); }
static inline float hash21(float2 p) { float3 p3 = fract(float3(p.xyx) * 0.1031f); p3 += dot(p3, p3.yzx + 33.33f); return fract((p3.x + p3.y) * p3.z); }
static inline float hash31(float3 p3) { p3 = fract(p3 * 0.1031f); p3 += dot(p3, p3.zyx + 31.32f); return fract((p3.x + p3.y) * p3.z); }
static inline float2 hash22(float2 p) { float3 p3 = fract(float3(p.xyx) * float3(0.1031f, 0.1030f, 0.0973f)); p3 += dot(p3, p3.yzx + 33.33f); return fract((p3.xx + p3.yz) * p3.zy); }
static inline float3 hash33(float3 p3) { p3 = fract(p3 * float3(0.1031f, 0.1030f, 0.0973f)); p3 += dot(p3, p3.yxz + 33.33f); return fract((p3.xxy + p3.yxx) * p3.zyx); }

static float vnoise2(float2 p) {
    float2 i = floor(p), f = fract(p);
    f = f * f * (3.0f - 2.0f * f);
    float a = hash21(i), b = hash21(i + float2(1, 0)), c = hash21(i + float2(0, 1)), d = hash21(i + float2(1, 1));
    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}
static float vnoise3(float3 p) {
    float3 i = floor(p), f = fract(p);
    f = f * f * (3.0f - 2.0f * f);
    float a = mix(hash31(i), hash31(i + float3(1, 0, 0)), f.x);
    float b = mix(hash31(i + float3(0, 1, 0)), hash31(i + float3(1, 1, 0)), f.x);
    float c = mix(hash31(i + float3(0, 0, 1)), hash31(i + float3(1, 0, 1)), f.x);
    float d = mix(hash31(i + float3(0, 1, 1)), hash31(i + float3(1, 1, 1)), f.x);
    return mix(mix(a, b, f.y), mix(c, d, f.y), f.z);
}
static float fbm2(float2 p, int oct) {
    float s = 0, a = 0.5f;
    for (int i = 0; i < oct; i++) { s += a * vnoise2(p); p = p * 2.03f + float2(17.1f, 5.3f); a *= 0.5f; }
    return s;
}
static float fbm3(float3 p, int oct) {
    float s = 0, a = 0.5f;
    for (int i = 0; i < oct; i++) { s += a * vnoise3(p); p = p * 2.03f + float3(17.1f, 5.3f, 9.7f); a *= 0.5f; }
    return s;
}
static inline float lum(float3 c) { return dot(c, float3(0.2126f, 0.7152f, 0.0722f)); }
static inline float3 qrot(float4 q, float3 v) { float3 t = 2.0f * cross(q.xyz, v); return v + q.w * t + cross(q.xyz, t); }
static inline float sdSeg(float2 p, float2 a, float2 b) { float2 pa = p - a, ba = b - a; float h = saturate(dot(pa, ba) / dot(ba, ba)); return length(pa - ba * h); }

// ------------------------------------------------------------------ intersection
static inline bool slab(float3 mn, float3 mx, float3 ro, float3 inv, float tmax, thread float &tn) {
    float3 t0 = (mn - ro) * inv, t1 = (mx - ro) * inv;
    float3 a = min(t0, t1), b = max(t0, t1);
    tn = max(max(a.x, a.y), max(a.z, 0.0f));
    float tf = min(min(b.x, b.y), min(b.z, tmax));
    return tn <= tf;
}

static void traverse(const device Node *nodes, const device Tri *tris, int root, float3 ro, float3 rd, thread Hit &h, bool any, int inst) {
    float3 inv = 1.0f / rd;
    int stack[40];
    int sp = 0;
    int ni = root;
    {
        float tn;
        if (!slab(nodes[root].mn.xyz, nodes[root].mx.xyz, ro, inv, h.t, tn)) return;
    }
    while (true) {
        float4 nmn = nodes[ni].mn, nmx = nodes[ni].mx;
        int cnt = as_type<int>(nmx.w), lf = as_type<int>(nmn.w);
        if (cnt > 0) {
            for (int i = lf; i < lf + cnt; i++) {
                float4 v0 = tris[i].v0, e1 = tris[i].e1, e2 = tris[i].e2;
                if (any && (as_type<int>(e1.w) & 1)) continue;
                float3 pv = cross(rd, e2.xyz);
                float det = dot(e1.xyz, pv);
                if (fabs(det) < 1e-14f) continue;
                float id = 1.0f / det;
                float3 tv = ro - v0.xyz;
                float u = dot(tv, pv) * id;
                if (u < 0.0f || u > 1.0f) continue;
                float3 qv = cross(tv, e1.xyz);
                float v = dot(rd, qv) * id;
                if (v < 0.0f || u + v > 1.0f) continue;
                float t = dot(e2.xyz, qv) * id;
                if (t > 0.0f && t < h.t) {
                    h.t = t; h.tri = i; h.u = u; h.v = v; h.inst = inst;
                    if (any) return;
                }
            }
            if (sp == 0) break;
            ni = stack[--sp];
        } else {
            float ta, tb;
            bool ha = slab(nodes[lf].mn.xyz, nodes[lf].mx.xyz, ro, inv, h.t, ta);
            bool hb = slab(nodes[lf + 1].mn.xyz, nodes[lf + 1].mx.xyz, ro, inv, h.t, tb);
            if (ha && hb) {
                if (ta <= tb) { stack[sp++] = lf + 1; ni = lf; }
                else { stack[sp++] = lf; ni = lf + 1; }
            } else if (ha) ni = lf;
            else if (hb) ni = lf + 1;
            else {
                if (sp == 0) break;
                ni = stack[--sp];
            }
        }
    }
}

static void sceneHit(const device Node *nodes, const device Tri *tris, const device Inst *insts, const device Node *inodes, int nInst,
                     float3 ro, float3 rd, float tm, thread Hit &h, bool any) {
    rd = select(rd, float3(1e-9f), fabs(rd) < float3(1e-9f));
    traverse(nodes, tris, 0, ro, rd, h, any, -1);
    if (nInst == 0 || (any && h.tri >= 0)) return;
    // second level: a small BVH over the animated instances, rebuilt on the CPU every frame
    float3 inv = 1.0f / rd;
    float tn;
    if (!slab(inodes[0].mn.xyz, inodes[0].mx.xyz, ro, inv, h.t, tn)) return;
    int stack[24];
    int sp = 0, ni = 0;
    while (true) {
        int cnt = as_type<int>(inodes[ni].mx.w), lf = as_type<int>(inodes[ni].mn.w);
        if (cnt > 0) {
            int i = lf;
            float4 q = normalize(mix(insts[i].q0, insts[i].q1, tm));
            float3 p = mix(insts[i].p0.xyz, insts[i].p1.xyz, tm);
            float4 qc = float4(-q.xyz, q.w);
            float3 o = qrot(qc, ro - p), d = qrot(qc, rd);
            d = select(d, float3(1e-9f), fabs(d) < float3(1e-9f));
            traverse(nodes, tris, as_type<int>(insts[i].p0.w), o, d, h, any, i);
            if (any && h.tri >= 0) return;
            if (sp == 0) break;
            ni = stack[--sp];
        } else {
            float ta, tb;
            bool ha = slab(inodes[lf].mn.xyz, inodes[lf].mx.xyz, ro, inv, h.t, ta);
            bool hb = slab(inodes[lf + 1].mn.xyz, inodes[lf + 1].mx.xyz, ro, inv, h.t, tb);
            if (ha && hb) {
                if (ta <= tb) { stack[sp++] = lf + 1; ni = lf; }
                else { stack[sp++] = lf; ni = lf + 1; }
            } else if (ha) ni = lf;
            else if (hb) ni = lf + 1;
            else {
                if (sp == 0) break;
                ni = stack[--sp];
            }
        }
    }
}

// ------------------------------------------------------------------ procedural materials
struct Surf {
    float3 alb;   // diffuse albedo (already scaled by 1-metal)
    float3 f0;
    float rough;
    float3 emit;
    int emode;
    float trans;
    float3 n;
};

#define K_PLAIN 0
#define K_LANE 1
#define K_APPROACH 2
#define K_CARPET 3
#define K_TILE 4
#define K_CEIL 5
#define K_WOOD 6
#define K_BRICK 7
#define K_ASPHALT 8
#define K_PIN 9
#define K_BALL 10
#define K_ATLAS 11
#define K_METAL 12
#define K_CONCRETE 14
#define K_FORMICA 15
#define K_VINYL 16
#define K_TERRAZZO 17
#define K_STUCCO 18
#define K_BELT 20
#define K_PALETTE 21
#define K_CHROME 22
#define K_GUTTER 23
#define K_BLOCK 24
#define K_SIDEWALK 25
#define K_FLAKE 26
#define K_CURTAIN 27
#define K_PEGBOARD 28
#define K_LEAF 29

constant float LANE_W = 1.0668f;
constant float BOARD_W = 1.0668f / 39.0f;
constant float PINZ = 18.288f;

// wood grain colour for a point on a board; uv.x across the board (m), uv.y along it (m)
static float3 woodGrain(float2 uv, float seed, float3 light, float3 dark, float figure) {
    float2 q = float2(uv.x * 40.0f, uv.y * 1.2f) + seed * 13.7f;
    float warp = fbm2(q, 3);
    float ring = fract(uv.x * 28.0f + warp * 3.5f * figure + seed * 5.0f);
    ring = smoothstep(0.0f, 0.75f, ring) * smoothstep(1.0f, 0.82f, ring);
    float pores = vnoise2(float2(uv.x * 1400.0f, uv.y * 22.0f) + seed * 3.1f);
    float streak = fbm2(float2(uv.x * 220.0f, uv.y * 3.0f) + seed, 3);
    float t = saturate(0.20f + 0.45f * ring * (0.5f + 0.5f * warp) + 0.30f * (streak - 0.5f) + 0.16f * (pores - 0.5f));
    return mix(light, dark, t);
}

// lane surface: returns colour; rough and bump height via refs. lx = across from lane centre, z = distance from foul line
static float3 laneTex(float lx, float z, float laneSeed, thread float &rough, thread float &hgt) {
    float bx = (lx + LANE_W * 0.5f) / BOARD_W;
    float bi = floor(bx), bf = fract(bx);
    float bseed = hash21(float2(bi, laneSeed * 7.0f + 3.0f));
    // boards are random lengths end-to-end
    float off = bseed * 3.7f;
    float seg = floor((z + off) / 2.9f);
    float segf = fract((z + off) / 2.9f) * 2.9f;
    float sseed = hash21(float2(bi + seg * 31.0f, laneSeed * 11.0f + seg));
    bool maple = (z < 4.6f + 0.25f * step(0.5f, fract(bi * 0.5f)) * 2.0f - 0.25f) || (z > 17.3f + 0.25f * step(0.5f, fract(bi * 0.5f)) * 2.0f - 0.25f);
    float3 lightC = maple ? float3(0.80f, 0.60f, 0.36f) : float3(0.76f, 0.50f, 0.25f);
    float3 darkC = maple ? float3(0.60f, 0.40f, 0.20f) : float3(0.50f, 0.27f, 0.11f);
    float3 c = woodGrain(float2(bf * BOARD_W + bi * 0.37f, z), sseed * 50.0f + bi, lightC, darkC, maple ? 0.6f : 1.3f);
    c *= 0.86f + 0.26f * sseed;                         // board to board tone
    c = mix(c, c * float3(1.06f, 0.96f, 0.82f), bseed * 0.5f);
    float seam = smoothstep(0.0f, 0.035f, bf) * smoothstep(1.0f, 0.965f, bf);
    float endj = smoothstep(0.0f, 0.0018f, segf) * smoothstep(2.9f, 2.8982f, segf);
    c *= mix(0.55f, 1.0f, seam * endj);
    hgt = (seam * endj) * 0.00012f;
    // markings: arrows (darts) and dots, inlaid dark wood
    float3 mark = float3(0.16f, 0.07f, 0.035f);
    float m = 0.0f;
    for (int k = 1; k <= 7; k++) {
        float ax = -LANE_W * 0.5f + (k * 5.0f - 0.5f) * BOARD_W;
        float az = 3.70f + (3.0f - fabs(float(k) - 4.0f)) * 0.335f;
        float dz = z - az, dx = fabs(lx - ax);
        if (dz > 0.0f && dz < 0.155f) m = max(m, smoothstep(0.0015f, 0.0f, dx - 0.016f * (1.0f - dz / 0.155f)));
    }
    {   // guide dots 2.1 m past the foul line
        float dz = z - 2.13f;
        const float db[5] = {3.0f, 5.0f, 8.0f, 11.0f, 14.0f};
        for (int k = 0; k < 5; k++) {
            float ax = -LANE_W * 0.5f + (db[k] - 0.5f) * BOARD_W;
            m = max(m, smoothstep(0.0012f, 0.0f, length(float2(fabs(lx) - fabs(ax), dz)) - 0.0085f));
        }
    }
    c = mix(c, mark, m);
    // pin spots on the deck
    if (z > 18.0f) {
        float sp = 0.0f;
        for (int r = 0; r < 4; r++)
            for (int k = 0; k <= r; k++) {
                float2 pp = float2((float(k) - float(r) * 0.5f) * 0.3048f, PINZ + float(r) * 0.26396f);
                float d = length(float2(lx, z) - pp);
                sp = max(sp, smoothstep(0.0015f, 0.0f, fabs(d - 0.029f) - 0.0022f));
                sp = max(sp, 0.35f * smoothstep(0.034f, 0.0f, d) * vnoise2(float2(lx, z) * 300.0f)); // scuffed finish under each pin
            }
        c = mix(c, float3(0.07f, 0.06f, 0.055f), sp);
    }
    // finish: lacquer + oil. oil is heavier in the middle and for the first ~12 m; dry and dusty at the back
    float oil = smoothstep(13.5f, 9.0f, z) * (0.55f + 0.45f * smoothstep(0.52f, 0.1f, fabs(lx)));
    float track = fbm2(float2(lx * 55.0f, z * 0.35f) + laneSeed * 3.0f, 3);     // long ball tracks
    float smudge = fbm2(float2(lx * 9.0f, z * 2.5f) + laneSeed, 4);
    rough = 0.20f - 0.10f * oil + 0.10f * (track - 0.5f) + 0.10f * (smudge - 0.45f);
    rough = clamp(rough, 0.06f, 0.45f);
    c *= 0.94f + 0.10f * (track - 0.5f);
    // dark ball-impact scuffs near the head pin and the rubbed track
    if (z > 17.6f) c *= 1.0f - 0.25f * smoothstep(0.35f, 0.75f, fbm2(float2(lx * 30.0f, z * 9.0f), 3));
    return c;
}

static float3 approachTex(float lx, float z, float seed, thread float &rough, thread float &hgt) {
    float bx = (lx + 40.0f) / BOARD_W;
    float bi = floor(bx), bf = fract(bx);
    float bseed = hash21(float2(bi, seed + 9.0f));
    float off = bseed * 3.7f;
    float seg = floor((z + 30.0f + off) / 2.4f);
    float segf = fract((z + 30.0f + off) / 2.4f) * 2.4f;
    float sseed = hash21(float2(bi + seg * 17.0f, seg));
    float3 c = woodGrain(float2(bf * BOARD_W + bi * 0.37f, z), sseed * 50.0f + bi, float3(0.80f, 0.61f, 0.37f), float3(0.60f, 0.40f, 0.21f), 0.6f);
    c *= 0.86f + 0.26f * sseed;
    float seam = smoothstep(0.0f, 0.035f, bf) * smoothstep(1.0f, 0.965f, bf);
    float endj = smoothstep(0.0f, 0.0018f, segf) * smoothstep(2.4f, 2.3982f, segf);
    c *= mix(0.55f, 1.0f, seam * endj);
    hgt = seam * endj * 0.00012f;
    float sm = fbm2(float2(lx * 6.0f, z * 6.0f), 4);
    rough = clamp(0.24f + 0.25f * (sm - 0.45f), 0.12f, 0.5f);
    c *= 0.9f + 0.2f * sm;                              // shoe scuffing
    return c;
}

static float starburst(float2 p, float r, float n, float seed) {
    float a = atan2(p.y, p.x), d = length(p);
    float k = a * n / (2.0f * M_PI_F);
    float ki = floor(k + 0.5f);
    float rl = r * (0.55f + 0.45f * hash11(ki + seed * 13.0f + (fmod(fabs(ki), 2.0f) < 0.5f ? 0.0f : 7.0f)));
    if (fmod(fabs(ki), 2.0f) > 0.5f) rl *= 0.62f;
    float ang = (k - ki) * (2.0f * M_PI_F) / n;
    float w = d * fabs(sin(ang));
    float wid = 0.012f * r / 0.2f * saturate(1.0f - d / rl) + 0.0015f;
    float ray = (d < rl) ? smoothstep(wid, wid * 0.5f, w) : 0.0f;
    return max(ray, smoothstep(r * 0.11f, r * 0.09f, d));
}

static float3 carpetTex(float2 p, float3 base, thread float &hgt) {
    // 60s lobby carpet: deep field with scattered atomic starbursts, boomerangs and dots
    float3 c = base;
    float2 cell = floor(p / 0.75f);
    float pile = hash21(floor(p * 420.0f)) ;
    float loop = vnoise2(p * 160.0f);
    hgt = loop * 0.0015f;
    for (int j = -1; j <= 1; j++)
        for (int i = -1; i <= 1; i++) {
            float2 cc = cell + float2(i, j);
            float2 h = hash22(cc * 1.7f + 3.0f);
            float kind = hash21(cc + 41.0f);
            float2 o = (cc + 0.25f + 0.5f * h) * 0.75f;
            float2 q = p - o;
            float ang = h.x * 6.28f;
            float2 qr = float2(q.x * cos(ang) - q.y * sin(ang), q.x * sin(ang) + q.y * cos(ang));
            if (kind < 0.4f) {
                float s = starburst(qr, 0.20f + 0.12f * h.y, 12.0f, h.x * 9.0f);
                c = mix(c, kind < 0.2f ? float3(0.85f, 0.62f, 0.18f) : float3(0.80f, 0.78f, 0.66f), s);
            } else if (kind < 0.75f) {
                // boomerang: two capsule arms
                float d = min(sdSeg(qr, float2(0, 0), float2(0.16f, 0.07f)), sdSeg(qr, float2(0, 0), float2(-0.07f, 0.16f)));
                float wdt = 0.035f * (1.0f - 0.6f * saturate(length(qr) / 0.18f));
                float s = smoothstep(wdt + 0.004f, wdt, d);
                c = mix(c, kind < 0.58f ? float3(0.10f, 0.42f, 0.42f) : float3(0.78f, 0.30f, 0.12f), s);
            } else {
                float d = length(q);
                float s = smoothstep(0.045f, 0.040f, d) * smoothstep(0.022f, 0.027f, d);
                c = mix(c, float3(0.80f, 0.78f, 0.66f), s);
            }
        }
    c *= 0.72f + 0.38f * pile * (0.6f + 0.4f * loop);
    return c;
}

static float2 wallUV(float3 P, float3 n) {
    float3 a = fabs(n);
    if (a.y > a.x && a.y > a.z) return P.xz;
    if (a.x > a.z) return float2(P.z, P.y);
    return float2(P.x, P.y);
}

static void evalMat(const device Mat *mats, int mi, float3 P, float3 Ng, float2 uv, float tvar, float ivar, float tWorld, float lod,
                    texture2d<float> atlas, bool detail, thread Surf &s, thread float3 &bumpG) {
    float4 base = mats[mi].base, em = mats[mi].emit, prm = mats[mi].prm, prm2 = mats[mi].prm2;
    float3 col = base.rgb;
    float rough = base.a, metal = prm.x, spec = prm.y;
    int kind = int(prm.z + 0.5f);
    float p0 = prm.w, p1 = prm2.y, p2 = prm2.z;
    float3 emit = em.rgb;
    float hgt = 0.0f;
    bumpG = float3(0.0f);
    constexpr sampler smp(filter::linear, mip_filter::linear, address::clamp_to_edge);
    if (!detail) {
        // indirect bounces only need the average colour of a patterned surface
        bool cheap = true;
        switch (kind) {
        case K_PIN: case K_BALL: case K_ATLAS: case K_PALETTE: case K_PLAIN: case K_CHROME: case K_FLAKE: case K_BELT: cheap = false; break;
        case K_LANE: col = float3(0.66f, 0.46f, 0.25f); rough = 0.14f; break;
        case K_APPROACH: col = float3(0.66f, 0.48f, 0.27f); break;
        case K_TILE: col = 0.45f * (base.rgb + float3(prm2.y, prm2.z, prm2.w)); break;
        case K_WOOD: col = base.rgb * 0.70f; break;
        case K_CEIL: col = base.rgb * float3(0.84f, 0.82f, 0.74f); rough = 1.0f; break;
        case K_CARPET: rough = 1.0f; col = base.rgb * 0.95f + float3(0.02f, 0.02f, 0.015f); break;
        case K_ASPHALT: col = base.rgb * 0.8f; rough = 0.6f; break;
        default: col = base.rgb * 0.88f; break;
        }
        if (cheap) {
            s.rough = clamp(rough, 0.02f, 1.0f);
            s.alb = saturate(col) * (1.0f - metal);
            s.f0 = mix(float3(0.04f * spec), saturate(col), metal);
            s.emit = emit; s.emode = int(em.a + 0.5f); s.trans = prm2.x;
            return;
        }
    }
    switch (kind) {
    case K_LANE: {
        col = laneTex(P.x - tvar, P.z, tvar, rough, hgt);
        if (detail) {   // board seams only
            float bf0 = fract((P.x - tvar + LANE_W * 0.5f) / BOARD_W), bf1 = fract((P.x - tvar + 0.0006f + LANE_W * 0.5f) / BOARD_W);
            float s0 = smoothstep(0.0f, 0.035f, bf0) * smoothstep(1.0f, 0.965f, bf0), s1 = smoothstep(0.0f, 0.035f, bf1) * smoothstep(1.0f, 0.965f, bf1);
            bumpG = float3((s1 - s0) * 0.00012f / 0.0006f, 0.0f, 0.0f);
        }
    } break;
    case K_APPROACH: {
        col = approachTex(P.x, P.z, 1.0f, rough, hgt);
        // foul line and approach dots
        float fl = smoothstep(0.012f, 0.010f, fabs(P.z + 0.012f));
        float lx = fmod(P.x - tvar + 100.0f * 1.73f + 0.865f, 1.73f) - 0.865f;
        if (fabs(lx) < LANE_W * 0.5f) {
            col = mix(col, float3(0.03f), fl);
            float m = 0.0f;
            for (int k = 1; k <= 7; k++) {
                float ax = -LANE_W * 0.5f + (k * 5.0f - 0.5f) * BOARD_W;
                m = max(m, smoothstep(0.0012f, 0.0f, length(float2(lx - ax, P.z + 0.07f)) - 0.008f));
                m = max(m, smoothstep(0.0012f, 0.0f, length(float2(lx - ax, P.z + 3.66f)) - 0.008f));
                m = max(m, smoothstep(0.0012f, 0.0f, length(float2(lx - ax, P.z + 4.57f)) - 0.008f));
            }
            col = mix(col, float3(0.14f, 0.06f, 0.03f), m);
        }
    } break;
    case K_CARPET: {
        col = carpetTex(P.xz, base.rgb, hgt);
        rough = 1.0f;
    } break;
    case K_TILE: {
        float sz = p0 > 0.0f ? p0 : 0.3048f;
        float2 q = wallUV(P, Ng) / sz;
        float2 ci = floor(q), cf = fract(q);
        float chk = fmod(ci.x + ci.y + 1000.0f, 2.0f);
        float3 c2 = float3(prm2.y, prm2.z, prm2.w);
        col = chk < 0.5f ? base.rgb : c2;
        float sp = fbm2(q * 9.0f + ci * 3.1f, 4);
        col *= 0.82f + 0.36f * sp;
        col = mix(col, col * 0.5f, smoothstep(0.62f, 0.7f, vnoise2(q * 60.0f)) * 0.4f);   // marbled flecks
        float seam = smoothstep(0.0f, 0.012f, cf.x) * smoothstep(1.0f, 0.988f, cf.x) * smoothstep(0.0f, 0.012f, cf.y) * smoothstep(1.0f, 0.988f, cf.y);
        col *= mix(0.45f, 1.0f, seam);
        float wear = fbm2(wallUV(P, Ng) * 1.7f, 4);
        rough = clamp(base.a + 0.22f * (wear - 0.45f) + 0.15f * (1.0f - seam), 0.05f, 0.7f);
        hgt = seam * 0.0002f;
        if (detail) {
            float2 qa = q + float2(0.003f, 0), qb = q + float2(0, 0.003f);
            float2 fa = fract(qa), fb = fract(qb);
            float sa = smoothstep(0.0f, 0.012f, fa.x) * smoothstep(1.0f, 0.988f, fa.x) * smoothstep(0.0f, 0.012f, fa.y) * smoothstep(1.0f, 0.988f, fa.y);
            float sb = smoothstep(0.0f, 0.012f, fb.x) * smoothstep(1.0f, 0.988f, fb.x) * smoothstep(0.0f, 0.012f, fb.y) * smoothstep(1.0f, 0.988f, fb.y);
            float3 a = fabs(Ng);
            float2 g = float2(sa - seam, sb - seam) * 0.0002f / (0.003f * sz);
            if (a.y > a.x && a.y > a.z) bumpG = float3(g.x, 0, g.y);
        }
    } break;
    case K_CEIL: {
        float sz = 0.6096f;
        float2 q = P.xz / sz;
        float2 ci = floor(q), cf = fract(q);
        float tone = hash21(ci);
        col = base.rgb * (0.9f + 0.12f * tone);
        // fissured acoustic tile: worm holes and pin holes
        float fz = fbm2(q * float2(34.0f, 11.0f) + ci * 7.0f, 3);
        float holes = smoothstep(0.66f, 0.72f, fz);
        float2 ph = fract(q * 24.0f) - 0.5f;
        float pin = smoothstep(0.16f, 0.10f, length(ph)) * step(0.45f, hash21(floor(q * 24.0f)));
        col *= 1.0f - 0.55f * max(holes, pin * 0.7f);
        float seam = smoothstep(0.0f, 0.02f, cf.x) * smoothstep(1.0f, 0.98f, cf.x) * smoothstep(0.0f, 0.02f, cf.y) * smoothstep(1.0f, 0.98f, cf.y);
        col *= mix(0.5f, 1.0f, seam);
        col *= 0.93f + 0.14f * fbm2(P.xz * 0.6f, 3);       // nicotine staining
        col *= float3(1.0f, 0.97f, 0.88f);
        rough = 1.0f;
    } break;
    case K_WOOD: {
        // p0: plank width; p1: axis mode (0: along wall vertical, 1: along x, 2: along z); p2: figure
        float pw = p0 > 0.0f ? p0 : 0.14f;
        float2 q;
        if (p1 < 0.5f) { q = wallUV(P, Ng); }
        else if (p1 < 1.5f) { float3 a = fabs(Ng); q = (a.y > a.z) ? float2(P.z, P.x) : float2(P.y, P.x); }
        else { float3 a = fabs(Ng); q = (a.y > a.x) ? float2(P.x, P.z) : float2(P.y, P.z); }
        float bi = floor(q.x / pw), bf = fract(q.x / pw);
        float bs = hash21(float2(bi, tvar * 3.0f + 5.0f));
        float3 dk = base.rgb * float3(0.42f, 0.36f, 0.30f);
        col = woodGrain(float2(bf * pw + bs * 3.0f, q.y + bs * 9.0f), bs * 40.0f + tvar, base.rgb, dk, p2 > 0.0f ? p2 : 1.0f);
        col *= 0.85f + 0.3f * bs;
        float seam = smoothstep(0.0f, 0.02f, bf) * smoothstep(1.0f, 0.98f, bf);
        col *= mix(0.35f, 1.0f, seam);
        rough = clamp(base.a + 0.12f * (fbm2(q * 5.0f, 3) - 0.5f), 0.08f, 1.0f);
    } break;
    case K_BRICK: {
        float2 q = wallUV(P, Ng);
        float row = floor(q.y / 0.075f);
        float bx = q.x / 0.215f + 0.5f * fmod(row + 1000.0f, 2.0f);
        float bi = floor(bx);
        float2 f = float2(fract(bx), fract(q.y / 0.075f));
        float bs = hash21(float2(bi, row));
        float m = smoothstep(0.0f, 0.03f, f.x) * smoothstep(1.0f, 0.97f, f.x) * smoothstep(0.0f, 0.10f, f.y) * smoothstep(1.0f, 0.90f, f.y);
        float n = fbm2(q * 60.0f, 4);
        float3 bc = base.rgb * (0.65f + 0.6f * bs) * (0.8f + 0.4f * n);
        bc = mix(bc, bc * float3(0.6f, 0.55f, 0.6f), step(0.85f, bs));
        col = mix(float3(0.42f, 0.40f, 0.37f) * (0.8f + 0.4f * n), bc, m);
        rough = 0.95f;
        hgt = m * 0.004f + n * 0.001f;
    } break;
    case K_ASPHALT: {
        // a dry, sun-baked lot: aggregate, resurfaced patches, cracks, oil under the engines, worn paint
        float n = fbm2(P.xz * 0.9f, 5);
        float g = hash21(floor(P.xz * 260.0f));
        float agg = vnoise2(P.xz * 140.0f);
        col = base.rgb * (0.70f + 0.50f * n) * (0.78f + 0.30f * agg + 0.24f * g);
        {
            float2 q = (P.xz + float2(3.0f, 7.0f)) / float2(9.0f, 6.5f);
            float2 pf = fract(q);
            float ph = hash21(floor(q));
            if (ph > 0.78f) col *= mix(1.0f, 0.70f, step(0.18f, pf.x) * step(pf.x, 0.74f) * step(0.22f, pf.y) * step(pf.y, 0.83f));
        }
        float cr = smoothstep(0.0035f, 0.0f, fabs(fbm2(P.xz * 0.55f + 3.0f, 6) - 0.5f)) * smoothstep(0.42f, 0.62f, fbm2(P.xz * 0.13f + 9.0f, 3));
        col *= 1.0f - 0.45f * cr;
        bool rowA = P.z > -34.2f && P.z < -28.7f, rowB = P.z > -47.7f && P.z < -42.2f;
        bool walk = rowA && P.x > -5.5f && P.x < -0.1f;
        float sx = fabs(fmod(P.x + 101.35f, 2.7f) - 1.35f);
        float stripe = smoothstep(0.06f, 0.05f, sx) * ((rowA || rowB) && fabs(P.x) < 22.0f ? 1.0f : 0.0f);
        if (walk) {
            float edge = smoothstep(0.06f, 0.05f, min(fabs(P.x + 5.5f), fabs(P.x + 0.1f)));
            float hatch = smoothstep(0.075f, 0.065f, fabs(fract((P.x + P.z) * 0.62f) - 0.5f) * 1.14f) ;
            stripe = max(edge, hatch * step(-5.44f, P.x) * step(P.x, -0.16f));
        }
        float wornP = smoothstep(0.25f, 0.55f, fbm2(P.xz * 2.3f + 11.0f, 4)) * (0.75f + 0.25f * g);
        col = mix(col, float3(0.74f, 0.72f, 0.62f) * (0.65f + 0.35f * n), stripe * wornP * 0.9f);
        float stx = fmod(P.x + 100.0f, 2.7f) - 1.35f;
        float stz = rowA ? P.z + 30.1f : (rowB ? P.z + 45.9f : 9.0f);
        float stain = smoothstep(0.75f, 0.1f, length(float2(stx, stz * 0.65f))) * smoothstep(0.3f, 0.7f, fbm2(P.xz * 1.7f + 5.0f, 4)) * (walk ? 0.0f : 1.0f);
        col *= 1.0f - 0.55f * stain;
        rough = clamp(0.84f - 0.35f * stain - 0.12f * (n - 0.5f), 0.4f, 0.95f);
        hgt = agg * 0.002f;
        if (detail) {
            float a1 = vnoise2((P.xz + float2(0.004f, 0)) * 140.0f), a2 = vnoise2((P.xz + float2(0, 0.004f)) * 140.0f);
            bumpG = float3(a1 - agg, 0, a2 - agg) * (0.002f / 0.004f);
        }
    } break;
    case K_PIN: {
        float y = P.y;
        float a = atan2(P.z, P.x);
        col = float3(0.93f, 0.91f, 0.84f);
        // two red neck stripes
        float st = smoothstep(0.0015f, 0.0f, fabs(y - 0.263f) - 0.0085f) + smoothstep(0.0015f, 0.0f, fabs(y - 0.288f) - 0.0055f);
        col = mix(col, float3(0.62f, 0.035f, 0.03f), saturate(st));
        // crown mark on the belly
        float2 lg = float2(a * 0.055f, y - 0.175f);
        if (fabs(lg.x) < 0.02f && fabs(lg.y) < 0.02f) {
            float cr = smoothstep(0.001f, 0.0f, fabs(lg.y + 0.008f) - 0.0025f) * step(fabs(lg.x), 0.014f);
            float tooth = fabs(fract(lg.x / 0.0093f + 0.5f) - 0.5f) * 0.0093f;
            cr = max(cr, step(fabs(lg.x), 0.014f) * step(-0.006f, lg.y) * step(lg.y, 0.006f - tooth * 1.6f));
            col = mix(col, float3(0.62f, 0.035f, 0.03f), cr);
        }
        // scuffs: ball marks round the belly, grey smears, yellowing
        float sc = fbm3(P * float3(60.0f, 25.0f, 60.0f) + ivar * 31.0f, 4);
        float belly = smoothstep(0.03f, 0.09f, y) * smoothstep(0.20f, 0.12f, y);
        float scuff = smoothstep(0.60f, 0.74f, sc) * (0.12f + 0.88f * belly);
        col = mix(col, float3(0.30f, 0.29f, 0.28f), scuff * 0.38f);
        col *= mix(float3(1.0f), float3(0.96f, 0.90f, 0.74f), fbm3(P * 14.0f + ivar * 7.0f, 3) * 0.7f);
        rough = clamp(0.13f + 0.4f * scuff + 0.10f * (sc - 0.5f), 0.06f, 0.8f);
        if (y < 0.004f) { col = float3(0.75f, 0.72f, 0.62f); rough = 0.6f; } // nylon base ring
    } break;
    case K_BALL: {
        // marbled ball; ivar picks the colourway
        float id = floor(ivar + 0.5f);
        float3 c1, c2;
        if (id < 0.5f) { c1 = float3(0.012f, 0.012f, 0.014f); c2 = float3(0.035f, 0.035f, 0.04f); }
        else if (id < 1.5f) { c1 = float3(0.02f, 0.05f, 0.30f); c2 = float3(0.25f, 0.45f, 0.80f); }
        else if (id < 2.5f) { c1 = float3(0.35f, 0.02f, 0.03f); c2 = float3(0.85f, 0.35f, 0.15f); }
        else if (id < 3.5f) { c1 = float3(0.02f, 0.22f, 0.12f); c2 = float3(0.35f, 0.65f, 0.40f); }
        else if (id < 4.5f) { c1 = float3(0.45f, 0.28f, 0.03f); c2 = float3(0.90f, 0.78f, 0.35f); }
        else if (id < 5.5f) { c1 = float3(0.20f, 0.03f, 0.28f); c2 = float3(0.70f, 0.45f, 0.80f); }
        else { c1 = float3(0.05f, 0.25f, 0.30f); c2 = float3(0.55f, 0.85f, 0.85f); }
        float3 q = P * 9.0f + ivar * 3.3f;
        float3 w = float3(fbm3(q, 3), fbm3(q + 5.2f, 3), fbm3(q + 9.1f, 3));
        float m = fbm3(q + 4.0f * w, 4);
        m = smoothstep(0.32f, 0.68f, m);
        float vein = smoothstep(0.035f, 0.0f, fabs(fbm3(q * 0.7f + 2.5f * w, 3) - 0.5f));
        col = mix(c1, c2, m * 0.85f);
        col = mix(col, c2 * 1.2f + 0.05f, vein * 0.5f);
        rough = 0.07f;
        // finger holes: thumb and two fingers around the grip direction
        float3 d = normalize(P);
        const float3 gz = normalize(float3(0.25f, 0.55f, 0.80f));
        float3 gx = normalize(cross(float3(0, 1, 0), gz)), gy = cross(gz, gx);
        float3 h0 = normalize(gz - gy * 0.33f), h1 = normalize(gz + gy * 0.27f + gx * 0.16f), h2 = normalize(gz + gy * 0.27f - gx * 0.16f);
        float e0 = acos(saturate(dot(d, h0))) * 0.108f, e1 = acos(saturate(dot(d, h1))) * 0.108f, e2 = acos(saturate(dot(d, h2))) * 0.108f;
        float hole = max(smoothstep(0.0165f, 0.015f, e0), max(smoothstep(0.0125f, 0.0115f, e1), smoothstep(0.0125f, 0.0115f, e2)));
        float rim = max(smoothstep(0.003f, 0.0f, fabs(e0 - 0.0165f)), max(smoothstep(0.003f, 0.0f, fabs(e1 - 0.0125f)), smoothstep(0.003f, 0.0f, fabs(e2 - 0.0125f))));
        col = mix(col, float3(0.004f), hole);
        rough = mix(rough, 0.9f, hole);
        col = mix(col, col * 1.5f + 0.02f, rim * (1.0f - hole) * 0.5f);
        spec = mix(spec, 0.0f, hole);
        // fine scratches & dust
        rough += 0.06f * smoothstep(0.5f, 0.8f, fbm3(P * 70.0f, 3));
    } break;
    case K_ATLAS: {
        float4 t = atlas.sample(smp, uv, level(lod));
        col = t.rgb * base.rgb;
        emit = em.rgb * t.rgb;
        // print wear
        if (p0 > 0.0f) col *= 0.9f + 0.2f * fbm2(uv * 900.0f, 3);
    } break;
    case K_METAL: {
        // painted machine steel: grease, chips to bare metal on noise peaks
        float n = fbm3(P * 6.0f + tvar, 4);
        float n2 = fbm3(P * 45.0f, 3);
        float chip = smoothstep(0.71f, 0.75f, n2 * 0.55f + n * 0.5f + 0.1f * fbm3(P * 140.0f, 2)) * p0;
        float grease = smoothstep(0.50f, 0.80f, fbm3(P * float3(7.0f, 2.5f, 7.0f) + 3.0f, 4));
        col = base.rgb * (0.8f + 0.4f * n);
        col = mix(col, float3(0.05f, 0.045f, 0.035f), grease * 0.45f * p0);
        rough = clamp(base.a + 0.25f * (n - 0.5f) - 0.25f * grease * p0, 0.12f, 0.9f);
        metal = mix(metal, 1.0f, chip);
        col = mix(col, float3(0.55f, 0.53f, 0.50f), chip);
        rough = mix(rough, 0.35f, chip);
    } break;
    case K_CONCRETE: {
        float2 q = wallUV(P, Ng);
        float n = fbm2(q * 2.2f, 5), g = fbm2(q * 40.0f, 3);
        col = base.rgb * (0.7f + 0.45f * n) * (0.9f + 0.2f * g);
        float st = smoothstep(0.55f, 0.75f, fbm2(q * 0.9f + 11.0f, 4));
        col *= mix(1.0f, 0.55f, st);                    // oil stains
        rough = mix(0.85f, 0.3f, st * 0.8f);
        float cr = smoothstep(0.012f, 0.0f, fabs(fbm2(q * 0.7f + 3.0f, 4) - 0.5f));
        col *= 1.0f - 0.5f * cr * step(fabs(Ng.y), 0.5f) * 0.0f - 0.35f * cr;
    } break;
    case K_BLOCK: {
        // painted concrete block wall
        float2 q = wallUV(P, Ng);
        float row = floor(q.y / 0.2f);
        float bx = q.x / 0.4f + 0.5f * fmod(row + 100.0f, 2.0f);
        float2 f = float2(fract(bx), fract(q.y / 0.2f));
        float m = smoothstep(0.0f, 0.02f, f.x) * smoothstep(1.0f, 0.98f, f.x) * smoothstep(0.0f, 0.04f, f.y) * smoothstep(1.0f, 0.96f, f.y);
        float n = fbm2(q * 50.0f, 3);
        col = base.rgb * (0.85f + 0.3f * n) * mix(0.7f, 1.0f, m);
        col *= 0.8f + 0.3f * fbm2(q * 0.8f, 4);
        col *= mix(0.6f, 1.0f, smoothstep(0.0f, 0.5f, P.y));   // grime near the floor
        rough = 0.8f;
        hgt = m * 0.002f + n * 0.0008f;
    } break;
    case K_FORMICA: {
        // boomerang laminate
        float2 q = wallUV(P, Ng);
        float2 cell = floor(q / 0.09f);
        col = base.rgb;
        for (int j = -1; j <= 1; j++)
            for (int i = -1; i <= 1; i++) {
                float2 cc = cell + float2(i, j);
                float2 h = hash22(cc + 0.5f);
                float2 o = (cc + 0.2f + 0.6f * h) * 0.09f;
                float ang = h.x * 12.0f;
                float2 d = q - o;
                float2 qr = float2(d.x * cos(ang) - d.y * sin(ang), d.x * sin(ang) + d.y * cos(ang));
                float dd = min(sdSeg(qr, float2(0, 0), float2(0.022f, 0.008f)), sdSeg(qr, float2(0, 0), float2(-0.008f, 0.022f)));
                float s = smoothstep(0.0032f, 0.0024f, dd);
                float k = hash21(cc + 9.0f);
                col = mix(col, k < 0.5f ? float3(prm2.y, prm2.z, prm2.w) : float3(0.75f, 0.74f, 0.70f), s * 0.85f);
            }
        rough = clamp(base.a + 0.2f * (fbm2(q * 7.0f, 3) - 0.5f), 0.05f, 0.6f);
    } break;
    case K_VINYL: {
        float n = fbm3(P * 30.0f, 3);
        col = base.rgb * (0.88f + 0.24f * n);
        rough = clamp(base.a + 0.2f * (n - 0.5f), 0.1f, 0.9f);
    } break;
    case K_FLAKE: {
        // metal-flake / glitter vinyl
        float3 cellp = floor(P * 900.0f);
        float f = hash31(cellp);
        col = base.rgb * (0.8f + 0.5f * f);
        rough = f > 0.9f ? 0.08f : base.a;
        metal = f > 0.9f ? 1.0f : metal;
    } break;
    case K_TERRAZZO: {
        float2 q = P.xz;
        float3 c = base.rgb;
        for (int o = 0; o < 2; o++) {
            float sc = o == 0 ? 55.0f : 120.0f;
            float2 cell = floor(q * sc), f = fract(q * sc);
            float md = 9.0f; float id = 0.0f;
            for (int j = -1; j <= 1; j++)
                for (int i = -1; i <= 1; i++) {
                    float2 g = float2(i, j);
                    float2 h = hash22(cell + g);
                    float d = length(g + h - f);
                    if (d < md) { md = d; id = hash21(cell + g + 3.0f); }
                }
            float chip = smoothstep(0.42f, 0.36f, md) * step(0.35f, id);
            float3 cc = id < 0.55f ? float3(0.70f, 0.66f, 0.58f) : (id < 0.75f ? float3(0.25f, 0.22f, 0.20f) : (id < 0.9f ? float3(0.55f, 0.30f, 0.22f) : float3(0.80f, 0.74f, 0.50f)));
            c = mix(c, cc, chip);
        }
        col = c * (0.85f + 0.25f * fbm2(q * 1.5f, 4));
        // brass divider strips every 1.5 m
        float2 st = fabs(fract(q / 1.5f) - 0.5f) * 1.5f;
        float strip = smoothstep(0.004f, 0.003f, min(st.x, st.y));
        col = mix(col, float3(0.65f, 0.5f, 0.22f), strip);
        metal = strip;
        rough = clamp(base.a + 0.25f * (fbm2(q * 2.3f + 5.0f, 4) - 0.45f), 0.04f, 0.6f);
    } break;
    case K_STUCCO: {
        float2 q = wallUV(P, Ng);
        float n = fbm2(q * 1.3f, 4), g = fbm2(q * 90.0f, 2);
        col = base.rgb * (0.86f + 0.22f * n) * (0.95f + 0.1f * g);
        col *= mix(0.75f, 1.0f, smoothstep(0.0f, 0.35f, P.y));
        hgt = g * 0.0006f;
    } break;
    case K_SIDEWALK: {
        float2 q = P.xz;
        float n = fbm2(q * 3.0f, 5), g = vnoise2(q * 220.0f);
        col = base.rgb * (0.78f + 0.32f * n) * (0.9f + 0.2f * g);
        float2 f = fract(q / 1.5f);
        float seam = smoothstep(0.0f, 0.008f, f.x) * smoothstep(1.0f, 0.992f, f.x) * smoothstep(0.0f, 0.008f, f.y) * smoothstep(1.0f, 0.992f, f.y);
        col *= mix(0.45f, 1.0f, seam);
        float stn = smoothstep(0.55f, 0.75f, fbm2(q * 0.8f + 2.0f, 4));
        col *= mix(1.0f, 0.78f, stn);
        float gum = smoothstep(0.035f, 0.025f, length(fract(q * 1.3f) - 0.5f - 0.3f * (hash22(floor(q * 1.3f)) - 0.5f))) * step(0.72f, hash21(floor(q * 1.3f) + 5.0f));
        col *= 1.0f - 0.45f * gum;
        rough = 0.85f;
    } break;
    case K_LEAF: {
        float n = fbm3(P * 34.0f, 3), m = fbm3(P * 7.0f + 3.0f, 3);
        col = base.rgb * (0.45f + 1.0f * n) * mix(float3(1.0f), float3(1.25f, 1.15f, 0.6f), smoothstep(0.5f, 0.8f, m) * 0.6f);
        rough = 0.55f;
    } break;
    case K_BELT: {
        // moving belt: p0 = speed (m/s); p1 = 0: runs along world z (pit carpet), 1: along uv.x (conveyor), 2: chain / V-belt links
        float run = (p1 > 0.5f) ? uv.x : P.z;
        float zz = run - tWorld * p0;
        if (p1 > 1.5f) {
            float link = fract(zz / 0.028f);
            float m = smoothstep(0.12f, 0.3f, link) * smoothstep(0.88f, 0.7f, link);
            col = base.rgb * (0.35f + 1.1f * m);
            rough = mix(0.7f, base.a, m);
            break;
        }
        float across = (p1 > 0.5f) ? uv.y * 0.17f : P.x;
        float rib = smoothstep(0.35f, 0.5f, fabs(fract(zz / 0.05f) - 0.5f));
        float n = fbm2(float2(across * 30.0f, zz * 30.0f), 3);
        col = base.rgb * (0.55f + 0.45f * rib) * (0.75f + 0.5f * n);
        if (p1 < 0.5f) col *= 0.75f + 0.5f * smoothstep(0.35f, 0.65f, fbm2(float2(across * 3.0f, zz * 2.2f), 3));   // worn, patchy pile
        float lint = smoothstep(0.72f, 0.85f, vnoise2(float2(across * 420.0f, zz * 420.0f)));
        col = mix(col, float3(0.32f, 0.31f, 0.28f), lint * 0.35f);
        if (p1 > 0.5f) col *= 0.55f + 0.45f * smoothstep(0.0f, 0.12f, fabs(fract(zz / 0.24f) - 0.5f));   // cleats
        rough = 0.9f;
    } break;
    case K_PALETTE: {
        // colour picked per object from a 60s palette via tvar
        float k = fract(tvar * 7.31f + p0) * 8.0f;
        const float3 pal[8] = {float3(0.75f, 0.08f, 0.05f), float3(0.05f, 0.12f, 0.45f), float3(0.78f, 0.74f, 0.62f), float3(0.03f, 0.03f, 0.035f),
                               float3(0.05f, 0.40f, 0.38f), float3(0.85f, 0.45f, 0.05f), float3(0.45f, 0.25f, 0.10f), float3(0.80f, 0.65f, 0.12f)};
        col = pal[int(k) & 7];
        col *= 0.85f + 0.3f * fbm3(P * 40.0f, 2);
    } break;
    case K_CHROME: {
        float n = fbm3(P * 25.0f, 3);
        rough = clamp(base.a + 0.10f * smoothstep(0.5f, 0.8f, n), 0.02f, 0.6f);
        col = base.rgb * (0.92f + 0.08f * n);
    } break;
    case K_GUTTER: {
        float n = fbm2(float2(P.x * 40.0f, P.z * 0.6f), 4);
        float sc = fbm2(float2(P.x * 14.0f, P.z * 3.0f) + 4.0f, 4);
        col = base.rgb * (0.75f + 0.4f * n);
        col = mix(col, float3(0.02f), smoothstep(0.55f, 0.8f, sc) * 0.5f);     // black ball rubber marks
        rough = clamp(base.a + 0.25f * (sc - 0.5f), 0.1f, 0.8f);
    } break;
    case K_CURTAIN: {
        float fold = sin(P.x * 55.0f + 2.0f * sin(P.y * 3.0f)) * 0.5f + 0.5f;
        col = base.rgb * (0.55f + 0.5f * fold) * (0.8f + 0.3f * fbm2(P.xy * 20.0f, 3));
        rough = 0.9f;
    } break;
    case K_PEGBOARD: {
        float2 q = wallUV(P, Ng) / 0.0254f;
        float h = smoothstep(0.16f, 0.12f, length(fract(q) - 0.5f));
        col = base.rgb * (0.85f + 0.2f * fbm2(q * 0.3f, 3));
        col = mix(col, float3(0.02f), h);
        rough = 0.8f;
    } break;
    default: break;
    }
    s.rough = clamp(rough, 0.02f, 1.0f);
    s.alb = saturate(col) * (1.0f - metal);
    s.f0 = mix(float3(0.04f * spec), saturate(col), metal);
    s.emit = emit;
    s.emode = int(em.a + 0.5f);
    s.trans = prm2.x;
}

// ------------------------------------------------------------------ BSDF helpers
static inline void onb(float3 n, thread float3 &t, thread float3 &b) {
    float s = n.z >= 0.0f ? 1.0f : -1.0f;
    float a = -1.0f / (s + n.z), c = n.x * n.y * a;
    t = float3(1.0f + s * n.x * n.x * a, s * c, -s * n.x);
    b = float3(c, s + n.y * n.y * a, -n.y);
}
static inline float3 fresnel(float3 f0, float c) { float m = 1.0f - saturate(c); float m2 = m * m; return f0 + (1.0f - f0) * (m2 * m2 * m); }
static inline float ggxD(float nh, float a) { float a2 = a * a; float d = nh * nh * (a2 - 1.0f) + 1.0f; return a2 / (M_PI_F * d * d + 1e-12f); }
static inline float smithL(float nx, float a) { float a2 = a * a; return (-1.0f + sqrt(1.0f + a2 * (1.0f - nx * nx) / max(nx * nx, 1e-8f))) * 0.5f; }
static inline float smithG1(float nx, float a) { return 1.0f / (1.0f + smithL(nx, a)); }
static inline float smithG2(float nv, float nl, float a) { return 1.0f / (1.0f + smithL(nv, a) + smithL(nl, a)); }

// GGX visible-normal sampling (Heitz 2018), tangent space with z = normal
static float3 sampleVNDF(float3 Ve, float a, float u1, float u2) {
    float3 Vh = normalize(float3(a * Ve.x, a * Ve.y, Ve.z));
    float lensq = Vh.x * Vh.x + Vh.y * Vh.y;
    float3 T1 = lensq > 1e-10f ? float3(-Vh.y, Vh.x, 0.0f) / sqrt(lensq) : float3(1, 0, 0);
    float3 T2 = cross(Vh, T1);
    float r = sqrt(u1), phi = 2.0f * M_PI_F * u2;
    float t1 = r * cos(phi), t2 = r * sin(phi);
    float sv = 0.5f * (1.0f + Vh.z);
    t2 = (1.0f - sv) * sqrt(max(0.0f, 1.0f - t1 * t1)) + sv * t2;
    float3 Nh = t1 * T1 + t2 * T2 + sqrt(max(0.0f, 1.0f - t1 * t1 - t2 * t2)) * Vh;
    return normalize(float3(a * Nh.x, a * Nh.y, max(0.0f, Nh.z)));
}

// golden hour: the sun sits low behind the building's west corner. Direction to the sun, irradiance on a
// surface facing it, and the cosine of its angular radius (drawn a little larger than life, 0.75 degrees).
constant float3 SUN_DIR = float3(-0.6130f, 0.0958f, 0.7843f);
constant float3 SUN_E = float3(8.0f, 4.6f, 1.9f);
constant float SUN_COS = 0.999914f;
constant float SUN_OMEGA = 5.4e-4f;

static inline bool outdoors(float3 P) {
    return !(P.x > -9.74f && P.x < 9.74f && P.z > -25.29f && P.z < 24.5f && P.y < 5.19f);
}

constant float SPEC_DELTA = 0.0011f; // lobes sharper than this are treated as mirrors (no light sampling)

// ---- light grid: each cell lists the lights that matter there (built on the CPU)
constant float3 LG_MIN = float3(-45.0f, -1.0f, -60.0f);
constant float3 LG_CELL = float3(3.0f, 2.5f, 3.0f);
constant int LG_NX = 30, LG_NY = 4, LG_NZ = 29;
static inline int lightCell(float3 P) {
    float3 f = (P - LG_MIN) / LG_CELL;
    int ix = clamp(int(f.x), 0, LG_NX - 1), iy = clamp(int(f.y), 0, LG_NY - 1), iz = clamp(int(f.z), 0, LG_NZ - 1);
    return (iz * LG_NY + iy) * LG_NX + ix;
}

// selection weight of light i for a shading point (must be identical wherever it is used).
// Includes the glossy lobe, so a small lamp sitting in a reflection is picked often enough to resolve cleanly.
static inline float lightW(const device Light *lights, int i, float3 P, float3 N, float3 V, float a, float ks) {
    float3 d = lights[i].pos.xyz - P;
    float d2 = dot(d, d);
    float3 dn = d * rsqrt(max(d2, 1e-8f));
    float cn = max(dot(N, dn), 0.0f);
    float area = lights[i].u.w;
    float cl = 1.0f;
    float ltype = lights[i].pos.w;
    if (ltype != 1.0f) {
        cl = dot(lights[i].nrm.xyz, -dn);
        cl = (lights[i].v.w > 0.5f ? fabs(cl) : max(cl, 0.0f)) + 0.05f;
    }
    float sp = 0.0f;
    if (ks > 0.0f && ltype != 2.0f) {
        float3 H = normalize(V + dn);
        float ae = sqrt(a * a + 0.06f * area / (d2 + area));      // lobe widened by the lamp's apparent size
        sp = ks * min(ggxD(saturate(dot(N, H)), ae), 300.0f) * 0.25f * cn;
    }
    return lights[i].nrm.w * cl * (cn + 0.08f + sp) / (d2 + area);
}

// direct lighting with MIS against BSDF sampling: returns the diffuse part WITHOUT albedo (Ld) and the specular part (Ls)
static void nee(const device Node *nodes, const device Tri *tris, const device Inst *insts, const device Node *inodes, const device Light *lights,
                const device int2 *lgCells, const device int *lgIdx, constant Uni &U,
                float3 P, float3 Ng, float3 N, float3 V, thread Surf &s, float pS, float tm, thread uint &rng,
                thread float3 &Ld, thread float3 &Ls, thread float &wsumOut) {
    Ld = float3(0.0f); Ls = float3(0.0f);
    if (U.sky.y > 0.5f && outdoors(P)) {
        // the sun: always sampled outdoors, one shadow ray into its disc
        float cT = 1.0f - rnd(rng) * (1.0f - SUN_COS), sT = sqrt(max(0.0f, 1.0f - cT * cT)), ph = 2.0f * M_PI_F * rnd(rng);
        float3 st, sb;
        onb(SUN_DIR, st, sb);
        float3 wi = normalize(st * (sT * cos(ph)) + sb * (sT * sin(ph)) + SUN_DIR * cT);
        float nl = dot(N, wi);
        if (nl > 1e-5f && dot(Ng, wi) > 0.0f) {
            Hit sh; sh.t = 1e30f; sh.tri = -1; sh.inst = -1;
            sceneHit(nodes, tris, insts, inodes, U.counts.y, P + Ng * 2e-4f, wi, tm, sh, true);
            if (sh.tri < 0) {
                float nv = max(dot(N, V), 1e-4f);
                float3 Fd = fresnel(s.f0, nv);
                float a = max(s.rough * s.rough, 0.004f);
                Ld = SUN_E * U.sky.x * (nl / M_PI_F) * (1.0f - max(Fd.x, max(Fd.y, Fd.z)));
                if (s.rough * s.rough >= SPEC_DELTA) {
                    float3 H = normalize(V + wi);
                    float nh = saturate(dot(N, H)), vh = saturate(dot(V, H));
                    Ls = SUN_E * U.sky.x * fresnel(s.f0, vh) * (min(ggxD(nh, a), 400.0f) * smithG2(nv, nl, a) / (4.0f * nv));
                }
            }
        }
    }
    int2 cell = lgCells[lightCell(P)];
    float wsum = 0.0f, wsel = 0.0f;
    int sel = -1;
    for (int k = 0; k < cell.y; k++) {
        int i = lgIdx[cell.x + k];
        float w = lightW(lights, i, P, N, V, s.rough * s.rough, pS);
        wsum += w;
        if (rnd(rng) * wsum < w) { sel = i; wsel = w; }
    }
    wsumOut = wsum;
    if (sel < 0 || wsum <= 0.0f) return;
    float psel = wsel / wsum;
    float3 lp, ln;
    float area = lights[sel].u.w;
    float u1 = rnd(rng), u2 = rnd(rng);
    bool two = false;
    float ltype = lights[sel].pos.w;
    if (ltype != 1.0f) {
        lp = lights[sel].pos.xyz + lights[sel].u.xyz * (2.0f * u1 - 1.0f) + lights[sel].v.xyz * (2.0f * u2 - 1.0f);
        ln = lights[sel].nrm.xyz;
        two = lights[sel].v.w > 0.5f;
    } else {
        float z = 1.0f - 2.0f * u1, r = sqrt(max(0.0f, 1.0f - z * z)), ph = 2.0f * M_PI_F * u2;
        ln = float3(r * cos(ph), z, r * sin(ph));
        lp = lights[sel].pos.xyz + ln * lights[sel].emit.w;
    }
    float3 wi = lp - P;
    float dist2 = dot(wi, wi), dist = sqrt(dist2);
    wi /= dist;
    float cl = dot(ln, -wi);
    if (two) cl = fabs(cl);
    float nl = dot(N, wi);
    if (cl <= 1e-5f || nl <= 1e-5f || dot(Ng, wi) <= 0.0f) return;
    if (lum(lights[sel].emit.rgb) <= 0.0f) return;
    Hit sh; sh.t = dist * 0.999f - 2e-4f; sh.tri = -1; sh.inst = -1;
    sceneHit(nodes, tris, insts, inodes, U.counts.y, P + Ng * 2e-4f, wi, tm, sh, true);
    if (sh.tri >= 0) return;
    float pL = psel * dist2 / (cl * area);        // solid-angle pdf of this light sample
    float3 Li = lights[sel].emit.rgb / pL;
    float nv = max(dot(N, V), 1e-4f);
    float3 Fd = fresnel(s.f0, nv);
    float a = s.rough * s.rough;
    bool proxy = ltype == 2.0f;                   // stand-in light for emitters that are too thin to sample (neon): diffuse only
    bool delta = a < SPEC_DELTA;
    float pB = (1.0f - pS) * nl / M_PI_F;
    float3 fs = float3(0.0f);
    if (!delta && !proxy) {
        float3 H = normalize(V + wi);
        float nh = saturate(dot(N, H)), vh = saturate(dot(V, H));
        float D = ggxD(nh, a);
        fs = fresnel(s.f0, vh) * (D * smithG2(nv, nl, a) / (4.0f * nv));   // includes the cosine
        pB += pS * smithG1(nv, a) * D / (4.0f * nv);
    }
    float wm = proxy ? 1.0f : (pL * pL) / (pL * pL + pB * pB);
    Ld += Li * wm * (nl / M_PI_F) * (1.0f - max(Fd.x, max(Fd.y, Fd.z)));
    Ls += Li * wm * fs;
}

static float3 skyColor(float3 d, constant Uni &U, bool disc) {
    float h = max(d.y, 0.0f);
    float mu = dot(d, SUN_DIR);
    float2 dh = d.xz * rsqrt(max(dot(d.xz, d.xz), 1e-8f));
    float az = saturate(dot(dh, normalize(SUN_DIR.xz)) * 0.5f + 0.5f);     // 1 toward the sun, 0 opposite
    float azs = az * az * az;
    float3 zen = float3(0.060f, 0.135f, 0.36f);
    float3 hor = mix(float3(0.62f, 0.44f, 0.42f), float3(2.7f, 1.22f, 0.34f), azs);          // rose haze -> deep gold
    float3 mid = mix(float3(0.24f, 0.33f, 0.55f), float3(0.90f, 0.62f, 0.40f), azs);
    float3 c = mix(hor, mid, smoothstep(0.0f, 0.22f, h));
    c = mix(c, zen, smoothstep(0.10f, 0.70f, h));
    float m = max(mu, 0.0f);
    float m2 = m * m, m8 = m2 * m2 * m2 * m2, m64 = m8 * m8 * m8 * m8 * m8 * m8 * m8 * m8;
    c += float3(1.5f, 0.74f, 0.20f) * m8 * (1.0f - 0.6f * h) + float3(7.0f, 3.9f, 1.3f) * m64 * m64;
    if (d.y > 0.0f) {
        // high streaky cloud, lit from underneath on the sun's side
        float2 q = d.xz / (d.y + 0.14f);
        float2 qs = float2(q.x * 0.80f + q.y * 0.60f, -q.x * 0.60f + q.y * 0.80f) * float2(0.22f, 0.75f);
        float n = fbm2(qs + float2(3.7f, 1.3f), 5) + 0.25f * fbm2(q * 1.9f + 9.0f, 3);
        float dens = smoothstep(0.60f, 0.86f, n) * smoothstep(0.0f, 0.05f, d.y);
        float thin = 1.0f - smoothstep(0.64f, 0.98f, n);
        float3 lit = mix(float3(0.78f, 0.50f, 0.50f), float3(3.2f, 1.45f, 0.50f), azs) * (0.55f + 0.75f * thin);
        float3 shade = mix(float3(0.22f, 0.22f, 0.32f), float3(0.62f, 0.36f, 0.30f), azs);
        float3 cc = mix(shade, lit, saturate(0.35f + 0.65f * thin + 0.5f * (1.0f - h)));
        c = mix(c, cc, dens * 0.85f);
    } else {
        c = mix(hor * 0.55f, float3(0.05f, 0.045f, 0.04f), smoothstep(0.0f, -0.12f, d.y));
    }
    if (disc && mu > SUN_COS) c += SUN_E / SUN_OMEGA;
    return c * U.sky.x;
}

// ------------------------------------------------------------------ the path tracing kernel
kernel void trace(uint2 gid [[thread_position_in_grid]],
                  constant Uni &U [[buffer(0)]],
                  const device Node *nodes [[buffer(1)]],
                  const device Tri *tris [[buffer(2)]],
                  const device TriS *trs [[buffer(3)]],
                  const device Mat *mats [[buffer(4)]],
                  const device Light *lights [[buffer(5)]],
                  const device Inst *insts [[buffer(6)]],
                  device float4 *accD [[buffer(7)]],
                  device float4 *accS [[buffer(8)]],
                  device float4 *accA [[buffer(9)]],
                  device float4 *accN [[buffer(10)]],
                  device float4 *accP [[buffer(11)]],
                  const device Node *inodes [[buffer(12)]],
                  const device int2 *lgCells [[buffer(13)]],
                  const device int *lgIdx [[buffer(14)]],
                  device float4 *accX [[buffer(15)]],
                  device float4 *accE [[buffer(16)]],
                  texture2d<float> atlas [[texture(0)]]) {
    int W = U.dims.x, Hh = U.dims.y;
    if ((int)gid.x >= W || (int)gid.y >= Hh) return;
    int pix = (int)gid.y * W + (int)gid.x;
    int nInst = U.counts.y;
    float4 sD = float4(0.0f), sS = float4(0.0f), sA = float4(0.0f), sN = float4(0.0f), sP = float4(0.0f), sX = float4(0.0f), sE = float4(0.0f);
    float pixAngle = 2.0f * atan(U.lens.x) * 2.0f / float(W); // approx radians per pixel

    for (int si = 0; si < U.dims.w; si++) {
        int sidx = U.dims.z + si;
        uint rng = hashu(uint(pix) * 9781u + hashu(uint(U.counts.w) * 6271u + uint(sidx) * 26699u + 17u));
        pcg(rng);
        // R2 sequence with a per-pixel shift for pixel jitter and shutter time
        uint ph = hashu(uint(pix) + uint(U.counts.w) * 1013u);
        float2 jit = fract(float2(float(ph & 0xffffu), float(ph >> 16)) / 65536.0f + float(sidx) * float2(0.7548776662f, 0.5698402910f));
        float tm = fract(float(hashu(ph) & 0xffffu) / 65536.0f + float(sidx) * 0.6180339887f);

        // --- camera ray (stereographic fisheye, thin lens, shutter time)
        float2 ndc = float2((float(gid.x) + jit.x) / float(W) * 2.0f - 1.0f, (1.0f - (float(gid.y) + jit.y) / float(Hh) * 2.0f) * U.lens.w);
        float r = length(ndc);
        float th = 2.0f * atan(r * U.lens.x);
        float3 dl = r > 1e-6f ? float3(sin(th) * ndc.x / r, sin(th) * ndc.y / r, -cos(th)) : float3(0, 0, -1);
        float4 cq = normalize(mix(U.camQ0, U.camQ1, tm));
        float3 ro = mix(U.camP0.xyz, U.camP1.xyz, tm);
        float3 rd = qrot(cq, dl);
        if (U.lens.y > 0.0f) {
            float a = 2.0f * M_PI_F * rnd(rng), rr = sqrt(rnd(rng)) * U.lens.y;
            float3 lo = qrot(cq, float3(cos(a) * rr, sin(a) * rr, 0.0f));
            float3 fp = ro + rd * U.lens.z;
            ro += lo;
            rd = normalize(fp - ro);
        }

        float3 thr = float3(1.0f);
        float3 LD = float3(0.0f), LS = float3(0.0f), LE = float3(0.0f);   // LE: emitters and sky seen directly (never denoised)
        float aovW = 1.0f, fId = 0.0f, specHd = 0.0f, specCnt = 0.0f;
        bool probe = false;
        float prevPdf = -1.0f;   // solid-angle pdf of the direction that led here; < 0: camera / mirror (no MIS)
        float3 prevP = ro, prevN = float3(0, 1, 0), prevV = float3(0, 1, 0);
        float prevA = 1.0f, prevKs = 0.0f;
        float prevWsum = 1.0f;
        bool prevSpec = true;    // arrived via camera, glass or a specular lobe
        bool camChain = true;    // still on the camera/glass/mirror chain (full texture detail, haze)
        int comp = 0;            // 0: specular component, 1: diffuse (demodulated) component
        bool aov = false;
        bool firstDone = false;
        int bounce = 0;
        float3 fAlb = float3(0.0f), fN = float3(0.0f), fP = float3(0.0f);
        float fRough = 1.0f;
        float pathLen = 0.0f;

        for (int iter = 0; iter < 14 && bounce <= U.counts.z; iter++) {
            Hit h; h.t = 1e30f; h.tri = -1; h.inst = -1;
            sceneHit(nodes, tris, insts, inodes, nInst, ro, rd, tm, h, false);
            if (probe) { probe = false; specHd = h.tri < 0 ? 120.0f : min(h.t, 120.0f); specCnt = 1.0f; }   // where the mirror image lies
            if (camChain && U.misc.y > 0.0f) {      // haze (cigarette smoke): only inside the building
                float3 rs = select(rd, float3(1e-9f), fabs(rd) < float3(1e-9f));
                float3 ta = (float3(-9.45f, -0.3f, -25.0f) - ro) / rs, tb = (float3(9.45f, 3.4f, 24.4f) - ro) / rs;
                float3 t0 = min(ta, tb), t1 = max(ta, tb);
                float tt = max(min(min(t1.x, t1.y), min(t1.z, min(h.t, 80.0f))) - max(max(t0.x, t0.y), max(t0.z, 0.0f)), 0.0f);
                float T = exp(-U.misc.y * tt);
                float3 hz = thr * U.haze.rgb * (1.0f - T);
                if (comp == 1) LD += hz; else LS += hz;
                thr *= T;
                // outdoors: warm evening air between the lens and anything far away
                float to = h.tri < 0 ? 0.0f : max(h.t - tt - 25.0f, 0.0f);
                if (to > 0.0f && U.sky.y > 0.5f) {
                    float To = exp(-to * 0.0016f);
                    float3 ah = skyColor(normalize(float3(rd.x, 0.02f, rd.z)), U, false) * 0.8f;
                    float3 as = thr * ah * (1.0f - To);
                    if (comp == 1) LD += as; else LS += as;
                    thr *= To;
                }
            }
            if (h.tri < 0) {
                float3 sk = skyColor(rd, U, prevPdf < 0.0f) * thr;
                if (!camChain) sk = min(sk, float3(U.misc.z));
                if (!firstDone && comp == 0) { float mx = max(sk.x, max(sk.y, sk.z)); LE += (U.misc.w > 0.0f && mx > U.misc.w) ? sk * (U.misc.w / mx) : sk; }
                else if (comp == 1) LD += sk; else LS += sk;
                if (!aov) { aov = true; fP = ro + rd * 200.0f; }
                break;
            }
            float3 P = ro + rd * h.t;
            pathLen += h.t;
            float4 tv0 = tris[h.tri].v0, te1 = tris[h.tri].e1, te2 = tris[h.tri].e2;
            int mi = as_type<int>(tv0.w);
            float tvar = te2.w;
            TriS ts = trs[h.tri];
            float w0 = 1.0f - h.u - h.v;
            float3 Ng = normalize(cross(te1.xyz, te2.xyz));
            float3 Ns = normalize(ts.n0.xyz * w0 + ts.n1.xyz * h.u + ts.n2.xyz * h.v);
            float2 uv = ts.uv01.xy * w0 + ts.uv01.zw * h.u + ts.uv2.xy * h.v;
            float3 Pt = P; // texture-space position
            float ivar = 0.0f;
            float4 iq = float4(0, 0, 0, 1);
            if (h.inst >= 0) {
                iq = normalize(mix(insts[h.inst].q0, insts[h.inst].q1, tm));
                float3 ip = mix(insts[h.inst].p0.xyz, insts[h.inst].p1.xyz, tm);
                Pt = qrot(float4(-iq.xyz, iq.w), P - ip);
                ivar = insts[h.inst].p1.w;
            }
            float lod = 0.0f;                        // texture lod for atlas decals
            {
                float du = length(ts.uv01.zw - ts.uv01.xy) / max(length(te1.xyz), 1e-6f);
                float dv = length(ts.uv2.xy - ts.uv01.xy) / max(length(te2.xyz), 1e-6f);
                float foot = pathLen * pixAngle / max(fabs(dot(rd, h.inst >= 0 ? qrot(iq, Ng) : Ng)), 0.15f);
                lod = clamp(log2(max(foot * max(du, dv) * 4096.0f, 1e-6f)), 0.0f, 9.0f);
                if (!camChain) lod = max(lod, 2.0f);
            }
            Surf s;
            float3 bumpG;
            evalMat(mats, mi, Pt, Ng, uv, tvar, ivar + tvar, U.misc.x, lod, atlas, camChain, s, bumpG);
            if (h.inst >= 0) { Ng = qrot(iq, Ng); Ns = qrot(iq, Ns); bumpG = qrot(iq, bumpG); }
            float3 NgRaw = Ng;
            if (dot(Ng, rd) > 0.0f) Ng = -Ng;
            if (dot(Ns, Ng) < 0.0f) Ns = -Ns;
            if (camChain && dot(bumpG, bumpG) > 0.0f) Ns = normalize(Ns - (bumpG - Ns * dot(bumpG, Ns)));
            float3 V = -rd;
            if (dot(Ns, V) < 0.02f) Ns = normalize(Ns + V * (0.02f - dot(Ns, V)));

            // --- thin glass: reflect or pass straight through
            if (s.trans > 0.0f) {
                float c = fabs(dot(Ns, V));
                float F = 0.04f + 0.96f * pow(1.0f - c, 5.0f);
                F = 2.0f * F / (1.0f + F);          // both faces
                if (lum(s.emit) > 0.0f && prevSpec) {
                    float3 e = s.emit * thr;
                    if (!firstDone && comp == 0) { float mx = max(e.x, max(e.y, e.z)); LE += (U.misc.w > 0.0f && mx > U.misc.w) ? e * (U.misc.w / mx) : e; }
                    else if (comp == 1) LD += e; else LS += e;
                }
                if (rnd(rng) < F) {
                    rd = reflect(rd, Ns);
                    ro = P + Ng * 2e-4f;
                    prevPdf = -1.0f; prevSpec = true;
                    // a reflection off glass barely counts toward the pixel's guide buffers: they describe what is seen through it
                    if (!aov) { aov = true; fAlb = float3(0.0f); fN = Ns; fP = P; fRough = 0.0f; aovW = 0.001f; }
                    firstDone = true;
                } else {
                    thr *= mix(float3(1.0f), s.alb, s.trans);
                    ro = P - Ng * 2e-4f;
                }
                continue;
            }

            // --- emission
            if (lum(s.emit) > 0.0f) {
                bool add = true;
                float w = 1.0f;
                if (s.emode == 1) {
                    int li = int(mats[mi].prm2.w + 0.5f);
                    bool isRect = lights[li].pos.w != 1.0f;
                    if (isRect && lights[li].v.w < 0.5f && dot(lights[li].nrm.xyz, rd) > 0.0f) add = false;   // back of a one-sided light
                    if (add && prevPdf > 0.0f) {
                        // was this light a candidate at the previous vertex? if not, BSDF sampling carries it alone
                        int2 pc = lgCells[lightCell(prevP)];
                        bool listed = false;
                        for (int k = 0; k < pc.y; k++) if (lgIdx[pc.x + k] == li) { listed = true; break; }
                        if (listed) {
                            float cl = max(fabs(dot(NgRaw, rd)), 1e-4f);
                            float pl = lightW(lights, li, prevP, prevN, prevV, prevA, prevKs) / max(prevWsum, 1e-20f) * (h.t * h.t) / (cl * lights[li].u.w);
                            w = prevPdf * prevPdf / (prevPdf * prevPdf + pl * pl);
                        }
                    }
                } else if (s.emode == 3) add = prevSpec;
                if (add) {
                    float3 e = s.emit * thr * w;
                    if (!camChain) e = min(e, float3(prevSpec ? 3.5f : U.misc.z));   // small bright emitters seen in rough reflections: tame the sparkle
                    if (!firstDone && comp == 0) { float mx = max(e.x, max(e.y, e.z)); LE += (U.misc.w > 0.0f && mx > U.misc.w) ? e * (U.misc.w / mx) : e; }
                    else if (comp == 1) LD += e; else LS += e;
                }
            }
            if (!aov) {
                aov = true;
                fAlb = s.alb; fN = Ns; fP = P; fRough = s.rough;
                fId = h.inst >= 0 ? insts[h.inst].mn.w + 1.0f : 0.0f;
            }
            if (bounce >= U.counts.z) break;

            // --- lobe probabilities
            float nv = max(dot(Ns, V), 1e-4f);
            float3 Fv = fresnel(s.f0, nv);
            float fmax = max(Fv.x, max(Fv.y, Fv.z));
            float wS = lum(Fv), wD = lum(s.alb) * (1.0f - fmax);
            float pS = (wS + wD) > 0.0f ? wS / (wS + wD) : 0.0f;
            if (wD > 0.0f && wS > 0.0f) pS = clamp(pS, 0.1f, 0.9f);
            float a = s.rough * s.rough;
            bool delta = a < SPEC_DELTA;

            // --- direct light
            float3 Ld, Ls;
            float wsum;
            nee(nodes, tris, insts, inodes, lights, lgCells, lgIdx, U, P, Ng, Ns, V, s, pS, tm, rng, Ld, Ls, wsum);
            if (bounce > 0) { Ld = min(Ld, float3(U.misc.z * 4.0f)); Ls = min(Ls, float3(U.misc.z * 4.0f)); }
            bool first = !firstDone;
            firstDone = true;
            if (first) {
                LD += thr * Ld;                 // albedo re-applied after denoising
                LS += thr * Ls;
            } else {
                float3 c = thr * (Ld * s.alb + Ls);
                if (comp == 1) LD += c; else LS += c;
            }

            // --- sample the next direction
            bool pickS = rnd(rng) < pS;
            float3 T, B;
            onb(Ns, T, B);
            float3 wi;
            float nl;
            if (pickS) {
                float3 Vl = float3(dot(V, T), dot(V, B), dot(V, Ns));
                float3 Hl = sampleVNDF(Vl, a, rnd(rng), rnd(rng));
                float3 Hw = normalize(T * Hl.x + B * Hl.y + Ns * Hl.z);
                wi = reflect(-V, Hw);
                nl = dot(Ns, wi);
                if (nl <= 0.0f || dot(Ng, wi) <= 0.0f) break;
                float3 F = fresnel(s.f0, saturate(dot(V, Hw)));
                thr *= F * (smithG2(nv, nl, a) / max(smithG1(nv, a), 1e-5f)) / pS;
                if (first) probe = true;
                if (a > 0.0025f) camChain = false;      // only near-mirrors keep full detail / haze
            } else {
                if (wD <= 0.0f) break;
                float u1 = rnd(rng), u2 = rnd(rng);
                float rr = sqrt(u1), ph2 = 2.0f * M_PI_F * u2;
                wi = normalize(T * (rr * cos(ph2)) + B * (rr * sin(ph2)) + Ns * sqrt(max(0.0f, 1.0f - u1)));
                nl = dot(Ns, wi);
                if (nl <= 0.0f || dot(Ng, wi) <= 0.0f) break;
                float3 wgt = float3((1.0f - fmax) / (1.0f - pS));
                if (first) comp = 1; else wgt *= s.alb;
                thr *= wgt;
                camChain = false;
            }
            if (pickS && delta) prevPdf = -1.0f;
            else {
                float pdf = (1.0f - pS) * nl / M_PI_F;
                if (!delta) {
                    float3 H = normalize(V + wi);
                    pdf += pS * smithG1(nv, a) * ggxD(saturate(dot(Ns, H)), a) / (4.0f * nv);
                }
                prevPdf = max(pdf, 1e-8f);
            }
            prevSpec = pickS;
            prevP = P; prevN = Ns; prevWsum = wsum; prevV = V; prevA = a; prevKs = pS;
            rd = wi;
            ro = P + Ng * 2e-4f;
            bounce++;
            if (bounce >= 2) {
                float q = clamp(max(thr.x, max(thr.y, thr.z)), 0.05f, 0.95f);
                if (rnd(rng) > q) break;
                thr /= q;
            }
        }
        // bit test: survives fast-math
        if (((as_type<uint>(LD.x) & 0x7f800000u) == 0x7f800000u) || ((as_type<uint>(LD.y) & 0x7f800000u) == 0x7f800000u) || ((as_type<uint>(LD.z) & 0x7f800000u) == 0x7f800000u)) LD = float3(0.0f);
        if (((as_type<uint>(LS.x) & 0x7f800000u) == 0x7f800000u) || ((as_type<uint>(LS.y) & 0x7f800000u) == 0x7f800000u) || ((as_type<uint>(LS.z) & 0x7f800000u) == 0x7f800000u)) LS = float3(0.0f);
        float lD = lum(LD), lS = lum(LS);
        sD += float4(LD, lD * lD);
        sS += float4(LS, lS * lS);
        if (((as_type<uint>(LE.x) & 0x7f800000u) == 0x7f800000u) || ((as_type<uint>(LE.y) & 0x7f800000u) == 0x7f800000u) || ((as_type<uint>(LE.z) & 0x7f800000u) == 0x7f800000u)) LE = float3(0.0f);
        sA += float4(fAlb, fRough) * aovW;
        sN += float4(fN, 0.0f) * aovW;
        sP += float4(fP, 1.0f) * aovW;
        sX += float4(specHd, specCnt, fId * aovW, 1.0f);
        sE += float4(LE, 0.0f);
    }
    accD[pix] += sD; accS[pix] += sS; accA[pix] += sA; accN[pix] += sN; accP[pix] += sP; accX[pix] += sX; accE[pix] += sE;
}

// ------------------------------------------------------------------ resolve, temporal accumulation, a-trous denoise
// Guide buffers: accA = albedo + roughness, accN = normal, accP = position + guide weight (all weighted sums),
// accX = (mirror-image distance sum, its count, moving-object id sum, sample count), accE = directly seen emission.
kernel void resolve(uint2 gid [[thread_position_in_grid]],
                    constant Uni &U [[buffer(0)]],
                    const device float4 *accD [[buffer(1)]], const device float4 *accS [[buffer(2)]], const device float4 *accX [[buffer(3)]],
                    device float4 *oD [[buffer(4)]], device float4 *oS [[buffer(5)]],
                    const device float4 *accE [[buffer(6)]], device float4 *oE [[buffer(7)]]) {
    int W = U.dims.x, Hh = U.dims.y;
    if ((int)gid.x >= W || (int)gid.y >= Hh) return;
    int pix = (int)gid.y * W + (int)gid.x;
    float n = max(accX[pix].w, 1.0f);
    float4 d = accD[pix] / n, s = accS[pix] / n;
    float vd = max(d.w - lum(d.rgb) * lum(d.rgb), 0.0f) / n;
    float vs = max(s.w - lum(s.rgb) * lum(s.rgb), 0.0f) / n;
    oD[pix] = float4(d.rgb, vd);
    oS[pix] = float4(s.rgb, vs);
    oE[pix] = float4(accE[pix].rgb / n, 0.0f);
}

// ---- temporal accumulation: every pixel finds where its surface point was in the previous frame and blends the
// previous frame's accumulated light into this one, so each frame effectively carries several frames' samples.
// Diffuse light follows the surface; glossy reflections follow their mirror image; anything that no longer matches
// (a different surface, a moved shadow) is rejected or clipped to what this frame's neighbourhood allows.
struct TUni {
    float4 prevP, prevQ, curP, curQ;   // mid-shutter camera of the previous and the current frame
    float4 prm;                        // x: history usable, y/z/w: minimum blend weight of the new frame (diffuse / specular / emission)
    float4 prm2;                       // x/y: clip width in standard deviations (diffuse / specular), z/w: debug
    float4 prm3;                       // x: weight of the new frame in the final anti-shimmer blend (1: off)
};
struct DynPose { float4 q; float4 p; };
struct HistTap { float w[4]; int q[4]; float wsum; };

static bool projectCam(float3 X, float3 camP, float4 camQ, constant Uni &U, thread float2 &px) {
    float3 dl = qrot(float4(-camQ.xyz, camQ.w), X - camP);
    float l = length(dl);
    if (l < 1e-6f) return false;
    dl /= l;
    float c = -dl.z;
    if (c < -0.6f) return false;
    float sth = length(dl.xy);
    float r = (sth / (1.0f + c)) / U.lens.x;          // tan(theta / 2) / k
    float2 ndc = sth > 1e-7f ? dl.xy * (r / sth) : float2(0.0f);
    px = float2((ndc.x * 0.5f + 0.5f) * float(U.dims.x), (1.0f - ndc.y / U.lens.w) * 0.5f * float(U.dims.y));
    return true;
}

static void histTaps(float2 px, float3 Pref, float3 Nref, float idRef, float roughRef, bool useRough, float tol, int W, int Hh,
                     const device float4 *pP, const device float4 *pN, const device float4 *pA, const device float4 *pX, thread HistTap &t) {
    float2 f = px - 0.5f;
    int x0 = int(floor(f.x)), y0 = int(floor(f.y));
    float fx = f.x - float(x0), fy = f.y - float(y0);
    t.wsum = 0.0f;
    for (int k = 0; k < 4; k++) {
        int xx = x0 + (k & 1), yy = y0 + (k >> 1);
        float bw = ((k & 1) ? fx : 1.0f - fx) * ((k >> 1) ? fy : 1.0f - fy);
        t.q[k] = 0; t.w[k] = 0.0f;
        if (xx < 0 || yy < 0 || xx >= W || yy >= Hh) continue;
        int q = yy * W + xx;
        float nq = pP[q].w;
        if (nq <= 0.0f) continue;
        float3 Nq = pN[q].xyz / nq;
        float nlq = length(Nq);
        if (nlq < 0.3f) continue;
        if (dot(Nq / nlq, Nref) < 0.9f) continue;
        if (fabs(dot(Nref, pP[q].xyz / nq - Pref)) > tol) continue;
        if (fabs(pX[q].z / nq - idRef) > 0.01f) continue;
        if (useRough && fabs(pA[q].w / nq - roughRef) > 0.12f) continue;
        t.q[k] = q; t.w[k] = bw; t.wsum += bw;
    }
}
static inline float4 histFetch(const device float4 *b, thread HistTap &t) {
    return (b[t.q[0]] * t.w[0] + b[t.q[1]] * t.w[1] + b[t.q[2]] * t.w[2] + b[t.q[3]] * t.w[3]) / max(t.wsum, 1e-6f);
}

kernel void temporal(uint2 gid [[thread_position_in_grid]],
                     constant Uni &U [[buffer(0)]], constant TUni &T [[buffer(1)]],
                     const device float4 *cD [[buffer(2)]], const device float4 *cS [[buffer(3)]], const device float4 *cE [[buffer(4)]],
                     const device float4 *accA [[buffer(5)]], const device float4 *accN [[buffer(6)]], const device float4 *accP [[buffer(7)]], const device float4 *accX [[buffer(8)]],
                     const device float4 *pA [[buffer(9)]], const device float4 *pN [[buffer(10)]], const device float4 *pP [[buffer(11)]], const device float4 *pX [[buffer(12)]],
                     const device float4 *hD [[buffer(13)]], const device float4 *hS [[buffer(14)]], const device float4 *hE [[buffer(15)]], const device float4 *hM [[buffer(16)]],
                     device float4 *oD [[buffer(17)]], device float4 *oS [[buffer(18)]], device float4 *oE [[buffer(19)]], device float4 *oM [[buffer(20)]],
                     const device DynPose *dynC [[buffer(21)]], const device DynPose *dynP [[buffer(22)]]) {
    int W = U.dims.x, Hh = U.dims.y;
    if ((int)gid.x >= W || (int)gid.y >= Hh) return;
    int x = gid.x, y = gid.y, pix = y * W + x;
    float4 d = cD[pix], s = cS[pix];
    float3 e = cE[pix].rgb;
    // despeckle: a pixel may not be much brighter than its second-brightest neighbour
    {
        float d1 = 0.0f, d2 = 0.0f, s1 = 0.0f, s2 = 0.0f, dm = 0.0f, sm = 0.0f;
        for (int j = -1; j <= 1; j++)
            for (int i = -1; i <= 1; i++) {
                if (i == 0 && j == 0) continue;
                int q = clamp(y + j, 0, Hh - 1) * W + clamp(x + i, 0, W - 1);
                float ld = lum(cD[q].rgb), ls = lum(cS[q].rgb);
                if (ld > d1) { d2 = d1; d1 = ld; } else if (ld > d2) d2 = ld;
                if (ls > s1) { s2 = s1; s1 = ls; } else if (ls > s2) s2 = ls;
                dm += ld * 0.125f; sm += ls * 0.125f;
            }
        float limD = max(2.0f * d2, 3.0f * dm) + 1e-3f, limS = max(2.0f * s2, 3.0f * sm) + 1e-3f;
        float ld = lum(d.rgb), ls = lum(s.rgb);
        // (only where the pixel's own samples disagree wildly: a steady bright detail is left alone)
        if (ld > limD && sqrt(d.w) > 0.4f * ld) { float k = limD / ld; d.rgb *= k; d.w *= k * k; }
        if (ls > limS && sqrt(s.w) > 0.4f * ls) { float k = limS / ls; s.rgb *= k; s.w *= k * k; }
    }
    float n = accP[pix].w;
    float3 Nc = n > 0.0f ? accN[pix].xyz / n : float3(0.0f);
    float nl = length(Nc);
    if (T.prm.x < 0.5f || nl < 0.3f) {
        oD[pix] = d; oS[pix] = s; oE[pix] = float4(e, 0.0f); oM[pix] = float4(nl < 0.3f ? 0.0f : 1.0f);
        return;
    }
    Nc /= nl;
    float3 Pc = accP[pix].xyz / n;
    {   // the guide position is an average over the pixel; slide it along its plane to the pixel centre so that a
        // camera that barely moves reprojects exactly onto pixel centres (no blur from resampling)
        float2 ndc = float2((float(x) + 0.5f) / float(W) * 2.0f - 1.0f, (1.0f - (float(y) + 0.5f) / float(Hh) * 2.0f) * U.lens.w);
        float r = length(ndc), th = 2.0f * atan(r * U.lens.x);
        float3 dl = r > 1e-6f ? float3(sin(th) * ndc.x / r, sin(th) * ndc.y / r, -cos(th)) : float3(0, 0, -1);
        float3 rdc = qrot(T.curQ, dl);
        float dn = dot(rdc, Nc);
        if (fabs(dn) > 0.02f) {
            float t = dot(Pc - T.curP.xyz, Nc) / dn;
            float3 Pm = T.curP.xyz + rdc * t;
            if (t > 0.0f && length(Pm - Pc) < 0.02f * t + 0.01f) Pc = Pm;
        }
    }
    float rough = accA[pix].w / n;
    float4 X = accX[pix];
    float depth = length(Pc - T.curP.xyz);
    float tol = 0.006f * depth + 0.003f;

    // statistics of this frame's light around the pixel (same surface only)
    float3 m1D = float3(0.0f), m2D = float3(0.0f), m1S = float3(0.0f), m2S = float3(0.0f);
    float cnt = 0.0f;
    for (int j = -3; j <= 3; j++)
        for (int i = -3; i <= 3; i++) {
            int xx = x + i, yy = y + j;
            if (xx < 0 || yy < 0 || xx >= W || yy >= Hh) continue;
            int q = yy * W + xx;
            float nq = accP[q].w;
            if (nq <= 0.0f) continue;
            if (dot(accN[q].xyz / nq, Nc) < 0.8f) continue;
            if (fabs(dot(Nc, accP[q].xyz / nq - Pc)) > 3.0f * tol) continue;
            float3 qd = cD[q].rgb, qs = cS[q].rgb;
            m1D += qd; m2D += qd * qd; m1S += qs; m2S += qs * qs; cnt += 1.0f;
        }
    cnt = max(cnt, 1.0f);
    m1D /= cnt; m1S /= cnt;
    float3 sdD = sqrt(max(m2D / cnt - m1D * m1D, float3(0.0f))), sdS = sqrt(max(m2S / cnt - m1S * m1S, float3(0.0f)));

    // where was this surface point one frame ago?
    float idc = X.z / n;
    int id = int(idc + 0.5f);
    bool idOk = fabs(idc - float(id)) < 0.01f;
    float3 Pp = Pc, Np = Nc;
    if (id > 0 && idOk) {
        DynPose c = dynC[id - 1], p = dynP[id - 1];
        float4 cc = float4(-c.q.xyz, c.q.w);
        Pp = p.p.xyz + qrot(p.q, qrot(cc, Pc - c.p.xyz));
        Np = qrot(p.q, qrot(cc, Nc));
    }
    HistTap ts; ts.wsum = 0.0f;
    float2 px;
    bool okS = idOk && projectCam(Pp, T.prevP.xyz, T.prevQ, U, px);
    if (okS) histTaps(px, Pp, Np, float(id), rough, false, tol, W, Hh, pP, pN, pA, pX, ts);
    okS = okS && ts.wsum > 0.05f;

    float4 M = float4(1.0f, 1.0f, 1.0f, 0.0f);
    float4 outD = d, outS = s;
    float3 outE = e;
    if (okS) {
        float4 hm = histFetch(hM, ts);
        // diffuse
        float4 h = histFetch(hD, ts);
        float3 lo = m1D - T.prm2.x * sdD - 0.02f * m1D - 1e-4f, hi = m1D + T.prm2.x * sdD + 0.02f * m1D + 1e-4f;
        h.rgb = clamp(h.rgb, lo, hi);
        float len = min(hm.x + 1.0f, 1.0f / T.prm.y), a = 1.0f / len;
        outD = float4(mix(h.rgb, d.rgb, a), a * a * d.w + (1.0f - a) * (1.0f - a) * h.w);
        M.x = len;
        // emission: static surfaces glowing, no clipping needed
        float3 he = histFetch(hE, ts).rgb;
        {
            float3 lo = e, hi = e;
            for (int j = -1; j <= 1; j++)
                for (int i = -1; i <= 1; i++) {
                    float3 q = cE[clamp(y + j, 0, Hh - 1) * W + clamp(x + i, 0, W - 1)].rgb;
                    lo = min(lo, q); hi = max(hi, q);
                }
            he = clamp(he, lo, hi);
        }
        float lenE = min(hm.z + 1.0f, 1.0f / T.prm.w);
        outE = mix(he, e, 1.0f / lenE);
        M.z = lenE;
    }
    // specular: prefer the motion of the mirror image on smooth flat surfaces
    {
        bool got = false;
        float4 h = float4(0.0f);
        float hlen = 0.0f, amin = T.prm.z;
        if (id == 0 && X.y > 0.5f && rough < 0.3f) {
            float hd = X.x / X.y;
            float3 Xv = Pc + (Pc - T.curP.xyz) / max(depth, 1e-5f) * hd;
            float2 pv;
            HistTap tv; tv.wsum = 0.0f;
            if (projectCam(Xv, T.prevP.xyz, T.prevQ, U, pv)) {
                histTaps(pv, Pc, Nc, 0.0f, rough, true, tol, W, Hh, pP, pN, pA, pX, tv);
                if (tv.wsum > 0.3f) { h = histFetch(hS, tv); hlen = histFetch(hM, tv).y; got = true; }
            }
        }
        if (!got && okS) {
            h = histFetch(hS, ts); hlen = histFetch(hM, ts).y; got = true;
            if (rough < 0.3f) amin = max(amin, 0.3f);
        }
        if (got) {
            float3 lo = m1S - T.prm2.y * sdS - 0.02f * m1S - 1e-4f, hi = m1S + T.prm2.y * sdS + 0.02f * m1S + 1e-4f;
            h.rgb = clamp(h.rgb, lo, hi);
            float len = min(hlen + 1.0f, 1.0f / amin), a = 1.0f / len;
            outS = float4(mix(h.rgb, s.rgb, a), a * a * s.w + (1.0f - a) * (1.0f - a) * h.w);
            M.y = len;
        }
    }
    if (T.prm2.z > 0.5f) {   // debug: reprojection error of the current camera, and history validity
        float2 pc; projectCam(Pc, T.curP.xyz, T.curQ, U, pc);
        float2 er = fabs(pc - (float2(gid) + 0.5f));
        outD = float4(0.0f); outS = float4(0.0f); outE = float3(er.x, er.y, okS ? 0.05f : 0.0f) * T.prm2.w;
    }
    oD[pix] = outD; oS[pix] = outS; oE[pix] = float4(outE, 0.0f); oM[pix] = M;
}

kernel void atrous(uint2 gid [[thread_position_in_grid]],
                   constant Uni &U [[buffer(0)]],
                   const device float4 *iD [[buffer(1)]], const device float4 *iS [[buffer(2)]],
                   const device float4 *accA [[buffer(3)]], const device float4 *accN [[buffer(4)]], const device float4 *accP [[buffer(5)]],
                   device float4 *oD [[buffer(6)]], device float4 *oS [[buffer(7)]],
                   constant float4 &prm [[buffer(8)]]) {
    int W = U.dims.x, Hh = U.dims.y;
    if ((int)gid.x >= W || (int)gid.y >= Hh) return;
    int x = gid.x, y = gid.y, pix = y * W + x;
    int step = int(prm.x);
    float n = max(accP[pix].w, 1e-6f);
    float3 Pc = accP[pix].xyz / n;
    float3 Nc = accN[pix].xyz / n;
    float nl = length(Nc);
    float4 Ac = accA[pix] / n;
    float4 cD = iD[pix], cS = iS[pix];
    if (nl < 0.3f) { oD[pix] = cD; oS[pix] = cS; return; }   // sky / heavily mixed pixel
    Nc /= nl;
    float depth = length(Pc - U.camP0.xyz);
    // 3x3 gaussian prefilter of the variance
    float vD = 0.0f, vS = 0.0f;
    {
        const float g[3] = {0.25f, 0.5f, 0.25f};
        for (int j = -1; j <= 1; j++)
            for (int i = -1; i <= 1; i++) {
                int xx = clamp(x + i, 0, W - 1), yy = clamp(y + j, 0, Hh - 1);
                float w = g[i + 1] * g[j + 1];
                vD += iD[yy * W + xx].w * w; vS += iS[yy * W + xx].w * w;
            }
    }
    float sgD = prm.y * sqrt(max(vD, 0.0f)) + 1e-4f;
    float sgS = prm.z * sqrt(max(vS, 0.0f)) + 1e-4f;
    float lDc = lum(cD.rgb), lSc = lum(cS.rgb);
    const float k[3] = {0.375f, 0.25f, 0.0625f};
    float4 aD = float4(0.0f), aS = float4(0.0f);
    float wDs = 0.0f, wSs = 0.0f;
    // a second, relaxed accumulation rescues silhouette pixels that find no strict neighbours
    float4 rD = float4(0.0f), rS = float4(0.0f);
    float wDr = 0.0f, wSr = 0.0f;
    float sp = 0.004f * depth * float(step) + 0.002f;
    for (int j = -2; j <= 2; j++)
        for (int i = -2; i <= 2; i++) {
            int xx = x + i * step, yy = y + j * step;
            if (xx < 0 || yy < 0 || xx >= W || yy >= Hh) continue;
            int q = yy * W + xx;
            float nq = max(accP[q].w, 1e-6f);
            float3 Nq = accN[q].xyz / nq;
            float3 Pq = accP[q].xyz / nq;
            float4 Aq = accA[q] / nq;
            float kw = k[abs(i)] * k[abs(j)];
            float nd = saturate(dot(Nc, Nq));
            float wn = pow(nd, 48.0f);
            float pd = fabs(dot(Nc, Pq - Pc));
            float wp = exp(-pd / sp);
            float wr = exp(-fabs(Aq.w - Ac.w) * 12.0f);
            float4 qD = iD[q], qS = iS[q];
            float lwD = exp(-fabs(lum(qD.rgb) - lDc) / sgD), lwS = exp(-fabs(lum(qS.rgb) - lSc) / sgS);
            float wd = kw * wn * wp * lwD;
            float ws = kw * wn * wp * wr * lwS * exp(-length(Aq.rgb - Ac.rgb) * 6.0f);
            float wrel = kw * (0.15f + 0.85f * nd * nd) * exp(-pd / (sp * 6.0f + 0.02f)) * exp(-length(Pq - Pc) / (0.03f * depth + 0.01f));
            float wdr = wrel * sqrt(lwD), wsr = wrel * wr * sqrt(lwS);
            if (i == 0 && j == 0) { wd = kw; ws = kw; wdr = kw; wsr = kw; }
            aD += float4(qD.rgb * wd, qD.w * wd * wd); wDs += wd;
            aS += float4(qS.rgb * ws, qS.w * ws * ws); wSs += ws;
            rD += float4(qD.rgb * wdr, qD.w * wdr * wdr); wDr += wdr;
            rS += float4(qS.rgb * wsr, qS.w * wsr * wsr); wSr += wsr;
        }
    float4 oDv = float4(aD.rgb / wDs, aD.w / (wDs * wDs)), oSv = float4(aS.rgb / wSs, aS.w / (wSs * wSs));
    float4 oDr = float4(rD.rgb / wDr, rD.w / (wDr * wDr)), oSr = float4(rS.rgb / wSr, rS.w / (wSr * wSr));
    // strict weight total of ~0.14 means "only myself": blend toward the relaxed estimate there
    float fD = smoothstep(0.45f, 0.16f, wDs), fS = smoothstep(0.45f, 0.16f, wSs);
    if (step > 4) { fD = 0.0f; fS = 0.0f; }
    oD[pix] = mix(oDv, oDr, fD);
    oS[pix] = mix(oSv, oSr, fS * 0.7f);
}

kernel void compose(uint2 gid [[thread_position_in_grid]],
                    constant Uni &U [[buffer(0)]],
                    const device float4 *iD [[buffer(1)]], const device float4 *iS [[buffer(2)]],
                    const device float4 *accA [[buffer(3)]], const device float4 *accP [[buffer(4)]],
                    device float4 *out [[buffer(5)]],
                    const device float4 *iE [[buffer(6)]], const device float4 *accX [[buffer(7)]]) {
    int W = U.dims.x, Hh = U.dims.y;
    if ((int)gid.x >= W || (int)gid.y >= Hh) return;
    int pix = (int)gid.y * W + (int)gid.x;
    float n = max(accP[pix].w, 1e-6f);
    float3 alb = accA[pix].rgb / n;
    float3 c = iD[pix].rgb * alb + iS[pix].rgb + iE[pix].rgb;
    out[pix] = float4(max(c, float3(0.0f)), accX[pix].w);
}

// ---- last pass: blend each finished frame with the previous finished frame, re-projected, and clamped to what the
// current frame shows around the pixel. Thin things (palm fronds, wires, far railings) cover a different fraction of
// a pixel in every frame; this steadies them without letting anything trail.
kernel void taa(uint2 gid [[thread_position_in_grid]],
                constant Uni &U [[buffer(0)]], constant TUni &T [[buffer(1)]],
                const device float4 *cur [[buffer(2)]], const device float4 *hist [[buffer(3)]], device float4 *out [[buffer(4)]],
                const device float4 *accN [[buffer(5)]], const device float4 *accP [[buffer(6)]], const device float4 *accX [[buffer(7)]],
                const device DynPose *dynC [[buffer(8)]], const device DynPose *dynP [[buffer(9)]]) {
    int W = U.dims.x, Hh = U.dims.y;
    if ((int)gid.x >= W || (int)gid.y >= Hh) return;
    int x = gid.x, y = gid.y, pix = y * W + x;
    float4 c = cur[pix];
    if (T.prm.x < 0.5f || T.prm2.z > 0.5f || T.prm3.x >= 0.999f) { out[pix] = c; return; }
    float2 ndc = float2((float(x) + 0.5f) / float(W) * 2.0f - 1.0f, (1.0f - (float(y) + 0.5f) / float(Hh) * 2.0f) * U.lens.w);
    float r = length(ndc), th = 2.0f * atan(r * U.lens.x);
    float3 dl = r > 1e-6f ? float3(sin(th) * ndc.x / r, sin(th) * ndc.y / r, -cos(th)) : float3(0, 0, -1);
    float3 rdc = qrot(T.curQ, dl);
    float n = accP[pix].w;
    float3 Nc = n > 0.0f ? accN[pix].xyz / n : float3(0.0f);
    float nl = length(Nc);
    float3 Pc = T.curP.xyz + rdc * 1.0e5f;                 // sky: direction only
    if (nl > 0.05f) {
        float3 Pa = accP[pix].xyz / n;
        float t = dot(Pa - T.curP.xyz, rdc);              // depth of the guide point along the pixel's own ray
        Pc = T.curP.xyz + rdc * clamp(t, 0.02f, 1.0e5f);
        float idc = accX[pix].z / n;
        int id = int(idc + 0.5f);
        if (id > 0 && fabs(idc - float(id)) < 0.01f) {
            DynPose a = dynC[id - 1], b = dynP[id - 1];
            Pc = b.p.xyz + qrot(b.q, qrot(float4(-a.q.xyz, a.q.w), Pc - a.p.xyz));
        }
    }
    float2 px;
    if (!projectCam(Pc, T.prevP.xyz, T.prevQ, U, px) || px.x < 0.0f || px.y < 0.0f || px.x > float(W) || px.y > float(Hh)) { out[pix] = c; return; }
    float2 f = px - 0.5f;
    int x0 = clamp(int(floor(f.x)), 0, W - 2), y0 = clamp(int(floor(f.y)), 0, Hh - 2);
    float fx = saturate(f.x - float(x0)), fy = saturate(f.y - float(y0));
    float3 h = mix(mix(hist[y0 * W + x0].rgb, hist[y0 * W + x0 + 1].rgb, fx), mix(hist[(y0 + 1) * W + x0].rgb, hist[(y0 + 1) * W + x0 + 1].rgb, fx), fy);
    float3 lo = c.rgb, hi = c.rgb;
    for (int j = -1; j <= 1; j++)
        for (int i = -1; i <= 1; i++) {
            float3 q = cur[clamp(y + j, 0, Hh - 1) * W + clamp(x + i, 0, W - 1)].rgb;
            lo = min(lo, q); hi = max(hi, q);
        }
    h = clamp(h, lo, hi);
    float a = T.prm3.x;
    float wc = a / (1.0f + lum(c.rgb)), wh = (1.0f - a) / (1.0f + lum(h));    // weigh in a compressed range so highlights do not dominate
    out[pix] = float4((c.rgb * wc + h * wh) / (wc + wh), c.w);
}
