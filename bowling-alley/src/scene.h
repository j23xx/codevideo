// scene.h — registries for materials, lights, instanced prototypes and animated objects.
#pragma once
#include "bvh.h"

// material kinds (must match render.metal)
enum {
    K_PLAIN = 0, K_LANE = 1, K_APPROACH = 2, K_CARPET = 3, K_TILE = 4, K_CEIL = 5, K_WOOD = 6, K_BRICK = 7, K_ASPHALT = 8, K_PIN = 9,
    K_BALL = 10, K_ATLAS = 11, K_METAL = 12, K_CONCRETE = 14, K_FORMICA = 15, K_VINYL = 16, K_TERRAZZO = 17, K_STUCCO = 18,
    K_BELT = 20, K_PALETTE = 21, K_CHROME = 22, K_GUTTER = 23, K_BLOCK = 24, K_SIDEWALK = 25, K_FLAKE = 26, K_CURTAIN = 27, K_PEGBOARD = 28, K_LEAF = 29
};

struct MatDef {
    float base[4] = {0.5f, 0.5f, 0.5f, 0.5f}; // rgb, roughness
    float emit[4] = {0, 0, 0, 0};             // rgb, mode (1: also a sampled light, 2: plain emitter)
    float prm[4] = {0, 1, 0, 0};              // metal, spec, kind, p0
    float prm2[4] = {0, 0, 0, 0};             // transmission, p1, p2, p3
};
static std::vector<MatDef> MATS;
static std::vector<V3> MAT_EMIT0;  // base emission (before animation)
static std::vector<int> MAT_ANIM;  // animation channel per material (0 = none)

struct MB {
    int id;
    MB &metal(float m = 1) { MATS[id].prm[0] = m; return *this; }
    MB &spec(float s) { MATS[id].prm[1] = s; return *this; }
    MB &kind(int k, float p0 = 0, float p1 = 0, float p2 = 0, float p3 = 0) {
        MATS[id].prm[2] = (float)k; MATS[id].prm[3] = p0; MATS[id].prm2[1] = p1; MATS[id].prm2[2] = p2; MATS[id].prm2[3] = p3;
        return *this;
    }
    MB &emit(V3 e, int mode = 2, int anim = 0) {
        MATS[id].emit[0] = e.x; MATS[id].emit[1] = e.y; MATS[id].emit[2] = e.z; MATS[id].emit[3] = (float)mode;
        MAT_EMIT0[id] = e; MAT_ANIM[id] = anim;
        return *this;
    }
    MB &glass(float t = 0.9f) { MATS[id].prm2[0] = t; return *this; }
    operator int() const { return id; }
};
static MB mat(V3 base, float rough) {
    MatDef m;
    m.base[0] = base.x; m.base[1] = base.y; m.base[2] = base.z; m.base[3] = rough;
    MATS.push_back(m); MAT_EMIT0.push_back(V3()); MAT_ANIM.push_back(0);
    return MB{(int)MATS.size() - 1};
}
static inline V3 rgb(int hex) { // sRGB hex -> linear
    auto f = [](int c) { float v = c / 255.0f; return v <= 0.04045f ? v / 12.92f : std::pow((v + 0.055f) / 1.055f, 2.4f); };
    return V3(f((hex >> 16) & 255), f((hex >> 8) & 255), f(hex & 255));
}

struct LightDef {
    int type = 0; // 0 rect, 1 sphere, 2 rect proxy (diffuse-only stand-in for thin emitters)
    V3 pos, u, v, emit;
    bool two = false;
    float radius = 0;
    int anim = 0;
};
static std::vector<LightDef> LIGHTS;

static std::vector<TriC> STATIC_TRIS;

// rectangular area light; optionally with a visible emitting panel
static int rectLight(V3 c, V3 u, V3 v, V3 emit, bool two = false, bool visible = true, int anim = 0) {
    LightDef L;
    L.pos = G.xf.pt(c); L.u = G.xf.vec(u); L.v = G.xf.vec(v); L.emit = emit; L.two = two; L.anim = anim;
    LIGHTS.push_back(L);
    if (visible) {
        int m = mat(V3(0, 0, 0), 1).emit(emit, 1, anim);
        MATS[m].prm2[3] = (float)(LIGHTS.size() - 1);
        PushMat pm(m);
        G.flags |= F_NOSHADOW;
        quad(c - u - v, c + u - v, c + u + v, c - u + v);
    }
    return (int)LIGHTS.size() - 1;
}
static int sphereLight(V3 c, float r, V3 emit, bool visible = true, int anim = 0) {
    LightDef L;
    L.type = 1; L.pos = G.xf.pt(c); L.radius = r; L.emit = emit; L.anim = anim;
    LIGHTS.push_back(L);
    if (visible) {
        int m = mat(V3(0, 0, 0), 1).emit(emit, 1, anim);
        MATS[m].prm2[3] = (float)(LIGHTS.size() - 1);
        PushMat pm(m);
        G.flags |= F_NOSHADOW;
        sphere(c, r, 16);
    }
    return (int)LIGHTS.size() - 1;
}

// ---- prototypes (meshes with their own BVH, placed by animated instances)
struct Proto {
    std::vector<TriC> tris;
    int first = 0, count = 0, root = 0;
    V3 mn, mx;
};
static std::vector<Proto> PROTOS;
struct ProtoScope {
    std::vector<TriC> *savedOut; Xf savedXf; int id;
    ProtoScope() {
        PROTOS.push_back(Proto());
        id = (int)PROTOS.size() - 1;
        savedOut = G.out; savedXf = G.xf;
        G.out = &PROTOS[id].tris; G.xf = Xf();
    }
    ~ProtoScope() { G.out = savedOut; G.xf = savedXf; }
};
// NOTE: PROTOS must not grow while a ProtoScope is open.

struct Dyn {
    int proto;
    float var;
    std::function<Pose(double)> pose; // world time -> pose
};
static std::vector<Dyn> DYNS;
static inline void addDyn(int proto, float var, std::function<Pose(double)> f) { DYNS.push_back({proto, var, f}); }

// animation channels for emitters: value multiplies base emission
static std::vector<std::function<float(double)>> ANIMS = {[](double) { return 1.0f; }};
static inline int addAnim(std::function<float(double)> f) { ANIMS.push_back(f); return (int)ANIMS.size() - 1; }
