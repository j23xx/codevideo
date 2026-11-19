// mesh.h — procedural modelling toolkit: everything in the film is built from these calls.
#pragma once
#include "base.h"

struct TriC {
    V3 p[3], n[3];
    float uv[3][2];
    int mat;
    float var;
    int flags;
};
enum { F_NOSHADOW = 1 };

struct Ctx {
    std::vector<TriC> *out = nullptr;
    int mat = 0;
    float var = 0;
    int flags = 0;
    Xf xf;
    float u0 = 0, v0 = 0, u1 = 1, v1 = 1; // uv rect used by quads
};
static Ctx G;

struct Push { // scoped transform
    Xf saved;
    explicit Push(const Xf &x) { saved = G.xf; G.xf = G.xf * x; }
    ~Push() { G.xf = saved; }
};
struct PushMat {
    int m; float v; int f;
    explicit PushMat(int mat) { m = G.mat; v = G.var; f = G.flags; G.mat = mat; }
    PushMat(int mat, float var) { m = G.mat; v = G.var; f = G.flags; G.mat = mat; G.var = var; }
    ~PushMat() { G.mat = m; G.var = v; G.flags = f; }
};
static inline void setUV(float u0 = 0, float v0 = 0, float u1 = 1, float v1 = 1) { G.u0 = u0; G.v0 = v0; G.u1 = u1; G.v1 = v1; }

static inline void triN(V3 a, V3 b, V3 c, V3 na, V3 nb, V3 nc, V2 ta = V2(), V2 tb = V2(), V2 tc = V2()) {
    TriC t;
    t.p[0] = G.xf.pt(a); t.p[1] = G.xf.pt(b); t.p[2] = G.xf.pt(c);
    V3 e = cross(t.p[1] - t.p[0], t.p[2] - t.p[0]);
    if (!(dot(e, e) >= 1e-18f) || !std::isfinite(dot(e, e))) return;
    t.n[0] = G.xf.dir(na); t.n[1] = G.xf.dir(nb); t.n[2] = G.xf.dir(nc);
    t.uv[0][0] = ta.x; t.uv[0][1] = ta.y; t.uv[1][0] = tb.x; t.uv[1][1] = tb.y; t.uv[2][0] = tc.x; t.uv[2][1] = tc.y;
    t.mat = G.mat; t.var = G.var; t.flags = G.flags;
    G.out->push_back(t);
}
static inline void tri(V3 a, V3 b, V3 c) {
    V3 n = norm(cross(b - a, c - a));
    triN(a, b, c, n, n, n);
}
// flat quad a,b,c,d (counter-clockwise seen from the front); uv from G rect
static inline void quad(V3 a, V3 b, V3 c, V3 d) {
    V3 n = norm(cross(b - a, d - a));
    V2 ta(G.u0, G.v0), tb(G.u1, G.v0), tc(G.u1, G.v1), td(G.u0, G.v1);
    triN(a, b, c, n, n, n, ta, tb, tc);
    triN(a, c, d, n, n, n, ta, tc, td);
}
static inline void quadN(V3 a, V3 b, V3 c, V3 d, V3 na, V3 nb, V3 nc, V3 nd) {
    triN(a, b, c, na, nb, nc);
    triN(a, c, d, na, nc, nd);
}
// upright rectangle facing `nrm`, centred at c, width w along `right`, height h along up
static inline void panel(V3 c, V3 right, V3 up, float w, float h) {
    right = norm(right) * (w * 0.5f); up = norm(up) * (h * 0.5f);
    quad(c - right - up, c + right - up, c + right + up, c - right + up);
}

static inline void box(V3 mn, V3 mx) {
    V3 a(mn.x, mn.y, mn.z), b(mx.x, mn.y, mn.z), c(mx.x, mx.y, mn.z), d(mn.x, mx.y, mn.z);
    V3 e(mn.x, mn.y, mx.z), f(mx.x, mn.y, mx.z), g(mx.x, mx.y, mx.z), h(mn.x, mx.y, mx.z);
    quad(b, a, d, c); // -z
    quad(e, f, g, h); // +z
    quad(a, e, h, d); // -x
    quad(f, b, c, g); // +x
    quad(d, h, g, c); // +y
    quad(a, b, f, e); // -y
}
static inline void boxC(V3 c, V3 half) { box(c - half, c + half); }

// box with rounded-looking (chamfered, smooth-shaded) edges
static inline void rbox(V3 mn, V3 mx, float r) {
    V3 c = (mn + mx) * 0.5f, h = (mx - mn) * 0.5f;
    r = std::min(r, std::min(h.x, std::min(h.y, h.z)) * 0.98f);
    auto P = [&](int sx, int sy, int sz, int face) {
        V3 q(sx * (h.x - (face == 0 ? 0 : r)), sy * (h.y - (face == 1 ? 0 : r)), sz * (h.z - (face == 2 ? 0 : r)));
        return c + q;
    };
    auto Nn = [&](int s, int face) { V3 n; n[face] = (float)s; return n; };
    // faces
    for (int face = 0; face < 3; face++)
        for (int s = -1; s <= 1; s += 2) {
            int a1 = (face + 1) % 3, a2 = (face + 2) % 3;
            int sg[3];
            V3 q[4];
            int k = 0;
            for (int j = 0; j < 4; j++) {
                int s1 = (j == 0 || j == 3) ? -1 : 1, s2 = (j < 2) ? -1 : 1;
                sg[face] = s; sg[a1] = s1; sg[a2] = s2;
                q[k++] = P(sg[0], sg[1], sg[2], face);
            }
            V3 n = Nn(s, face);
            V2 t0(G.u0, G.v0), t1(G.u1, G.v0), t2(G.u1, G.v1), t3(G.u0, G.v1);
            triN(q[0], q[1], q[2], n, n, n, t0, t1, t2);
            triN(q[0], q[2], q[3], n, n, n, t0, t2, t3);
        }
    // edges: for each axis, 4 edges
    for (int ax = 0; ax < 3; ax++) {
        int a1 = (ax + 1) % 3, a2 = (ax + 2) % 3;
        for (int s1 = -1; s1 <= 1; s1 += 2)
            for (int s2 = -1; s2 <= 1; s2 += 2) {
                int sg[3];
                sg[a1] = s1; sg[a2] = s2;
                sg[ax] = -1; V3 pa1 = P(sg[0], sg[1], sg[2], a1), pa2 = P(sg[0], sg[1], sg[2], a2);
                sg[ax] = 1;  V3 pb1 = P(sg[0], sg[1], sg[2], a1), pb2 = P(sg[0], sg[1], sg[2], a2);
                V3 n1 = Nn(s1, a1), n2 = Nn(s2, a2);
                quadN(pa1, pb1, pb2, pa2, n1, n1, n2, n2);
            }
    }
    // corners
    for (int sx = -1; sx <= 1; sx += 2)
        for (int sy = -1; sy <= 1; sy += 2)
            for (int sz = -1; sz <= 1; sz += 2)
                triN(P(sx, sy, sz, 0), P(sx, sy, sz, 1), P(sx, sy, sz, 2), Nn(sx, 0), Nn(sy, 1), Nn(sz, 2));
}
static inline void rboxC(V3 c, V3 half, float r) { rbox(c - half, c + half, r); }

// cone/cylinder between two points
static inline void cyl(V3 a, V3 b, float ra, float rb, int seg = 16, bool capA = true, bool capB = true) {
    V3 ax = b - a;
    float L = len(ax);
    if (L < 1e-7f) return;
    Xf f = xfAlignY(ax);
    float slope = (ra - rb) / L;
    for (int i = 0; i < seg; i++) {
        float t0 = 2 * PI * i / seg, t1 = 2 * PI * (i + 1) / seg;
        V3 d0 = f.X * std::cos(t0) + f.Z * std::sin(t0), d1 = f.X * std::cos(t1) + f.Z * std::sin(t1);
        V3 n0 = norm(d0 + f.Y * slope), n1 = norm(d1 + f.Y * slope);
        V3 p00 = a + d0 * ra, p01 = a + d1 * ra, p10 = b + d0 * rb, p11 = b + d1 * rb;
        float u0 = (float)i / seg, u1 = (float)(i + 1) / seg;
        triN(p00, p10, p11, n0, n0, n1, V2(u0, 0), V2(u0, 1), V2(u1, 1));
        triN(p00, p11, p01, n0, n1, n1, V2(u0, 0), V2(u1, 1), V2(u1, 0));
        if (capA && ra > 0) triN(a, p00, p01, -f.Y, -f.Y, -f.Y);
        if (capB && rb > 0) triN(b, p11, p10, f.Y, f.Y, f.Y);
    }
}

// surface of revolution around the local Y axis; profile is (radius, y) pairs; creases found by angle
static inline void lathe(const std::vector<V2> &pr, int seg = 32, float a0 = 0, float a1 = 2 * PI) {
    int n = (int)pr.size();
    if (n < 2) return;
    std::vector<V2> sn(n - 1);
    for (int i = 0; i < n - 1; i++) {
        V2 d = pr[i + 1] - pr[i];
        float l = len(d);
        sn[i] = l > 1e-9f ? V2(d.y / l, -d.x / l) : V2(1, 0);
    }
    auto vn = [&](int segi, int end) { // normal at end (0/1) of segment segi
        int other = end == 0 ? segi - 1 : segi + 1;
        if (other < 0 || other >= n - 1) return sn[segi];
        if (dot(sn[segi], sn[other]) < 0.75f) return sn[segi];
        V2 s = sn[segi] + sn[other];
        float l = len(s);
        return V2(s.x / l, s.y / l);
    };
    for (int i = 0; i < n - 1; i++) {
        V2 na = vn(i, 0), nb = vn(i, 1);
        for (int j = 0; j < seg; j++) {
            float t0 = a0 + (a1 - a0) * j / seg, t1 = a0 + (a1 - a0) * (j + 1) / seg;
            float c0 = std::cos(t0), s0 = std::sin(t0), c1 = std::cos(t1), s1 = std::sin(t1);
            V3 p00(pr[i].x * c0, pr[i].y, pr[i].x * s0), p01(pr[i].x * c1, pr[i].y, pr[i].x * s1);
            V3 p10(pr[i + 1].x * c0, pr[i + 1].y, pr[i + 1].x * s0), p11(pr[i + 1].x * c1, pr[i + 1].y, pr[i + 1].x * s1);
            V3 n00(na.x * c0, na.y, na.x * s0), n01(na.x * c1, na.y, na.x * s1), n10(nb.x * c0, nb.y, nb.x * s0), n11(nb.x * c1, nb.y, nb.x * s1);
            float u0 = (float)j / seg, u1 = (float)(j + 1) / seg;
            triN(p00, p10, p11, n00, n10, n11, V2(u0, pr[i].y), V2(u0, pr[i + 1].y), V2(u1, pr[i + 1].y));
            triN(p00, p11, p01, n00, n11, n01, V2(u0, pr[i].y), V2(u1, pr[i + 1].y), V2(u1, pr[i].y));
        }
    }
}

// Catmull-Rom resample of a 2D profile (for smooth lathe outlines)
static inline std::vector<V2> smoothProfile(const std::vector<V2> &k, int sub) {
    std::vector<V2> o;
    int n = (int)k.size();
    for (int i = 0; i < n - 1; i++) {
        V2 p0 = k[std::max(i - 1, 0)], p1 = k[i], p2 = k[i + 1], p3 = k[std::min(i + 2, n - 1)];
        for (int j = 0; j < sub; j++) {
            float t = (float)j / sub, t2 = t * t, t3 = t2 * t;
            V2 q = (p1 * 2.0f + (p2 - p0) * t + (p0 * 2.0f - p1 * 5.0f + p2 * 4.0f - p3) * t2 + (p1 * 3.0f - p0 - p2 * 3.0f + p3) * t3) * 0.5f;
            o.push_back(q);
        }
    }
    o.push_back(k[n - 1]);
    return o;
}
static inline std::vector<V3> smoothPath(const std::vector<V3> &k, int sub) {
    std::vector<V3> o;
    int n = (int)k.size();
    for (int i = 0; i < n - 1; i++) {
        V3 p0 = k[std::max(i - 1, 0)], p1 = k[i], p2 = k[i + 1], p3 = k[std::min(i + 2, n - 1)];
        for (int j = 0; j < sub; j++) {
            float t = (float)j / sub, t2 = t * t, t3 = t2 * t;
            V3 q = (p1 * 2.0f + (p2 - p0) * t + (p0 * 2.0f - p1 * 5.0f + p2 * 4.0f - p3) * t2 + (p1 * 3.0f - p0 - p2 * 3.0f + p3) * t3) * 0.5f;
            o.push_back(q);
        }
    }
    o.push_back(k[n - 1]);
    return o;
}

static inline void sphere(V3 c, float r, int seg = 24, float sy = 1.0f) {
    std::vector<V2> pr;
    int rings = seg / 2;
    for (int i = 0; i <= rings; i++) {
        float a = -PI / 2 + PI * i / rings;
        pr.push_back(V2(std::max(0.0f, r * std::cos(a)), r * sy * std::sin(a)));
    }
    Push p(xfT(c));
    lathe(pr, seg);
}

// tube swept along a polyline (parallel-transport frames)
static inline void tube(const std::vector<V3> &pts, float r, int seg = 8, bool caps = true, float r1 = -1) {
    int n = (int)pts.size();
    if (n < 2) return;
    if (r1 < 0) r1 = r;
    std::vector<V3> T(n), Nn(n), B(n);
    for (int i = 0; i < n; i++) T[i] = norm(pts[std::min(i + 1, n - 1)] - pts[std::max(i - 1, 0)]);
    Xf f = xfAlignY(T[0]);
    Nn[0] = f.X; B[0] = f.Z;
    for (int i = 1; i < n; i++) {
        V3 v = Nn[i - 1] - T[i] * dot(Nn[i - 1], T[i]);
        Nn[i] = norm(v);
        B[i] = cross(T[i], Nn[i]);
    }
    for (int i = 0; i < n - 1; i++) {
        float ra = lerp(r, r1, (float)i / (n - 1)), rb = lerp(r, r1, (float)(i + 1) / (n - 1));
        for (int j = 0; j < seg; j++) {
            float t0 = 2 * PI * j / seg, t1 = 2 * PI * (j + 1) / seg;
            V3 d00 = Nn[i] * std::cos(t0) + B[i] * std::sin(t0), d01 = Nn[i] * std::cos(t1) + B[i] * std::sin(t1);
            V3 d10 = Nn[i + 1] * std::cos(t0) + B[i + 1] * std::sin(t0), d11 = Nn[i + 1] * std::cos(t1) + B[i + 1] * std::sin(t1);
            quadN(pts[i] + d00 * ra, pts[i + 1] + d10 * rb, pts[i + 1] + d11 * rb, pts[i] + d01 * ra, d00, d10, d11, d01);
        }
    }
    if (caps)
        for (int j = 0; j < seg; j++) {
            float t0 = 2 * PI * j / seg, t1 = 2 * PI * (j + 1) / seg;
            V3 d0 = Nn[0] * std::cos(t0) + B[0] * std::sin(t0), d1 = Nn[0] * std::cos(t1) + B[0] * std::sin(t1);
            triN(pts[0], pts[0] + d0 * r, pts[0] + d1 * r, -T[0], -T[0], -T[0]);
            V3 e0 = Nn[n - 1] * std::cos(t0) + B[n - 1] * std::sin(t0), e1 = Nn[n - 1] * std::cos(t1) + B[n - 1] * std::sin(t1);
            triN(pts[n - 1], pts[n - 1] + e1 * r1, pts[n - 1] + e0 * r1, T[n - 1], T[n - 1], T[n - 1]);
        }
}

static inline void torus(V3 c, V3 axis, float R, float r, int segR = 32, int segr = 10, float a0 = 0, float a1 = 2 * PI) {
    Xf f = xfAlignY(axis);
    for (int i = 0; i < segR; i++) {
        float A0 = a0 + (a1 - a0) * i / segR, A1 = a0 + (a1 - a0) * (i + 1) / segR;
        V3 d0 = f.X * std::cos(A0) + f.Z * std::sin(A0), d1 = f.X * std::cos(A1) + f.Z * std::sin(A1);
        for (int j = 0; j < segr; j++) {
            float b0 = 2 * PI * j / segr, b1 = 2 * PI * (j + 1) / segr;
            V3 n00 = d0 * std::cos(b0) + f.Y * std::sin(b0), n01 = d0 * std::cos(b1) + f.Y * std::sin(b1);
            V3 n10 = d1 * std::cos(b0) + f.Y * std::sin(b0), n11 = d1 * std::cos(b1) + f.Y * std::sin(b1);
            quadN(c + d0 * R + n00 * r, c + d1 * R + n10 * r, c + d1 * R + n11 * r, c + d0 * R + n01 * r, n00, n10, n11, n01);
        }
    }
}

// parametric surface with smooth normals; uv in [0,1]^2
static inline void surf(int nu, int nv, const std::function<V3(float, float)> &f) {
    std::vector<V3> P((nu + 1) * (nv + 1)), N((nu + 1) * (nv + 1));
    auto at = [&](int i, int j) -> V3 & { return P[j * (nu + 1) + i]; };
    for (int j = 0; j <= nv; j++)
        for (int i = 0; i <= nu; i++) at(i, j) = f((float)i / nu, (float)j / nv);
    for (int j = 0; j <= nv; j++)
        for (int i = 0; i <= nu; i++) {
            V3 du = at(std::min(i + 1, nu), j) - at(std::max(i - 1, 0), j);
            V3 dv = at(i, std::min(j + 1, nv)) - at(i, std::max(j - 1, 0));
            N[j * (nu + 1) + i] = norm(cross(du, dv));
        }
    for (int j = 0; j < nv; j++)
        for (int i = 0; i < nu; i++) {
            int a = j * (nu + 1) + i, b = a + 1, c = a + nu + 2, d = a + nu + 1;
            V2 ta((float)i / nu, (float)j / nv), tb((float)(i + 1) / nu, (float)j / nv), tc((float)(i + 1) / nu, (float)(j + 1) / nv), td((float)i / nu, (float)(j + 1) / nv);
            triN(P[a], P[b], P[c], N[a], N[b], N[c], ta, tb, tc);
            triN(P[a], P[c], P[d], N[a], N[c], N[d], ta, tc, td);
        }
}

// --- polygon extrusion (outline in the local XZ plane, extruded along Y from y0 to y1)
static inline float polyArea(const std::vector<V2> &p) {
    float a = 0;
    for (size_t i = 0; i < p.size(); i++) { V2 q = p[(i + 1) % p.size()]; a += p[i].x * q.y - q.x * p[i].y; }
    return a * 0.5f;
}
static inline std::vector<int> earclip(const std::vector<V2> &p) {
    std::vector<int> idx, out;
    int n = (int)p.size();
    bool ccw = polyArea(p) > 0;
    for (int i = 0; i < n; i++) idx.push_back(ccw ? i : n - 1 - i);
    auto crs = [](V2 a, V2 b, V2 c) { return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x); };
    int guard = 0;
    while (idx.size() > 3 && guard++ < 100000) {
        bool cut = false;
        int m = (int)idx.size();
        for (int i = 0; i < m; i++) {
            int ia = idx[(i + m - 1) % m], ib = idx[i], ic = idx[(i + 1) % m];
            if (crs(p[ia], p[ib], p[ic]) <= 1e-12f) continue;
            bool ok = true;
            for (int k = 0; k < m && ok; k++) {
                int ik = idx[k];
                if (ik == ia || ik == ib || ik == ic) continue;
                if (crs(p[ia], p[ib], p[ik]) >= 0 && crs(p[ib], p[ic], p[ik]) >= 0 && crs(p[ic], p[ia], p[ik]) >= 0) ok = false;
            }
            if (!ok) continue;
            out.push_back(ia); out.push_back(ib); out.push_back(ic);
            idx.erase(idx.begin() + i);
            cut = true;
            break;
        }
        if (!cut) idx.erase(idx.begin()); // degenerate; drop a vertex
    }
    if (idx.size() == 3) { out.push_back(idx[0]); out.push_back(idx[1]); out.push_back(idx[2]); }
    return out;
}
static inline void extrude(const std::vector<V2> &poly, float y0, float y1, bool smoothSides = false) {
    std::vector<int> t = earclip(poly);
    V3 up(0, 1, 0);
    for (size_t i = 0; i + 2 < t.size(); i += 3) {
        V2 a = poly[t[i]], b = poly[t[i + 1]], c = poly[t[i + 2]];
        triN(V3(a.x, y1, a.y), V3(b.x, y1, b.y), V3(c.x, y1, c.y), up, up, up, a, b, c);
        triN(V3(a.x, y0, a.y), V3(c.x, y0, c.y), V3(b.x, y0, b.y), -up, -up, -up, a, c, b);
    }
    int n = (int)poly.size();
    float sgn = polyArea(poly) > 0 ? 1.f : -1.f;
    auto en = [&](int i) { V2 d = poly[(i + 1) % n] - poly[i]; float l = std::max(len(d), 1e-9f); return V3(d.y / l * sgn, 0, -d.x / l * sgn); };
    for (int i = 0; i < n; i++) {
        V2 a = poly[i], b = poly[(i + 1) % n];
        V3 nn = en(i), na = nn, nb = nn;
        if (smoothSides) {
            V3 pn = en((i + n - 1) % n), xn = en((i + 1) % n);
            if (dot(pn, nn) > 0.6f) na = norm(pn + nn);
            if (dot(xn, nn) > 0.6f) nb = norm(xn + nn);
        }
        quadN(V3(a.x, y0, a.y), V3(b.x, y0, b.y), V3(b.x, y1, b.y), V3(a.x, y1, a.y), na, nb, nb, na);
    }
}
static inline std::vector<V2> circlePoly(float r, int n, float cx = 0, float cz = 0) {
    std::vector<V2> p;
    for (int i = 0; i < n; i++) p.push_back(V2(cx + r * std::cos(2 * PI * i / n), cz + r * std::sin(2 * PI * i / n)));
    return p;
}
static inline std::vector<V2> roundRectPoly(float hx, float hz, float r, int k = 5) {
    std::vector<V2> p;
    for (int c = 0; c < 4; c++) {
        float cx = (c == 0 || c == 3) ? hx - r : -(hx - r), cz = (c < 2) ? hz - r : -(hz - r);
        for (int i = 0; i <= k; i++) {
            float a = PI / 2 * c + PI / 2 * i / k;
            p.push_back(V2(cx + r * std::cos(a), cz + r * std::sin(a)));
        }
    }
    return p;
}
