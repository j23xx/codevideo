// base.h — small math toolkit shared by the whole film.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <string>
#include <vector>

static const float PI = 3.14159265358979f;
static const float INCH = 0.0254f;

struct V2 {
    float x = 0, y = 0;
    V2() {}
    V2(float a, float b) : x(a), y(b) {}
};
static inline V2 operator+(V2 a, V2 b) { return {a.x + b.x, a.y + b.y}; }
static inline V2 operator-(V2 a, V2 b) { return {a.x - b.x, a.y - b.y}; }
static inline V2 operator*(V2 a, float s) { return {a.x * s, a.y * s}; }
static inline float dot(V2 a, V2 b) { return a.x * b.x + a.y * b.y; }
static inline float len(V2 a) { return std::sqrt(dot(a, a)); }

struct V3 {
    float x = 0, y = 0, z = 0;
    V3() {}
    V3(float a, float b, float c) : x(a), y(b), z(c) {}
    explicit V3(float a) : x(a), y(a), z(a) {}
    float operator[](int i) const { return (&x)[i]; }
    float &operator[](int i) { return (&x)[i]; }
};
static inline V3 operator+(V3 a, V3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
static inline V3 operator-(V3 a, V3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
static inline V3 operator-(V3 a) { return {-a.x, -a.y, -a.z}; }
static inline V3 operator*(V3 a, float s) { return {a.x * s, a.y * s, a.z * s}; }
static inline V3 operator*(float s, V3 a) { return {a.x * s, a.y * s, a.z * s}; }
static inline V3 operator*(V3 a, V3 b) { return {a.x * b.x, a.y * b.y, a.z * b.z}; }
static inline V3 operator/(V3 a, float s) { return {a.x / s, a.y / s, a.z / s}; }
static inline V3 &operator+=(V3 &a, V3 b) { a = a + b; return a; }
static inline V3 &operator-=(V3 &a, V3 b) { a = a - b; return a; }
static inline V3 &operator*=(V3 &a, float s) { a = a * s; return a; }
static inline float dot(V3 a, V3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
static inline V3 cross(V3 a, V3 b) { return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x}; }
static inline float len(V3 a) { return std::sqrt(dot(a, a)); }
static inline V3 norm(V3 a) { float l = len(a); return l > 1e-20f ? a / l : V3(0, 1, 0); }
static inline V3 lerp(V3 a, V3 b, float t) { return a + (b - a) * t; }
static inline float lerp(float a, float b, float t) { return a + (b - a) * t; }
static inline V3 vmin(V3 a, V3 b) { return {std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z)}; }
static inline V3 vmax(V3 a, V3 b) { return {std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z)}; }
static inline float clampf(float x, float a, float b) { return x < a ? a : (x > b ? b : x); }
static inline float sat(float x) { return clampf(x, 0, 1); }
static inline float smooth(float a, float b, float x) { float t = sat((x - a) / (b - a)); return t * t * (3 - 2 * t); }
static inline float smoother(float a, float b, float x) { float t = sat((x - a) / (b - a)); return t * t * t * (t * (6 * t - 15) + 10); }

// ---------------------------------------------------------------- quaternions
struct Quat {
    float x = 0, y = 0, z = 0, w = 1;
};
static inline Quat qaxis(V3 ax, float ang) {
    ax = norm(ax);
    float s = std::sin(ang * 0.5f);
    return {ax.x * s, ax.y * s, ax.z * s, std::cos(ang * 0.5f)};
}
static inline Quat qmul(Quat a, Quat b) {
    return {a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
            a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
            a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
            a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z};
}
static inline Quat qconj(Quat q) { return {-q.x, -q.y, -q.z, q.w}; }
static inline Quat qnorm(Quat q) {
    float l = std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
    if (l < 1e-20f) return Quat();
    return {q.x / l, q.y / l, q.z / l, q.w / l};
}
static inline V3 qrot(Quat q, V3 v) {
    V3 u(q.x, q.y, q.z);
    V3 t = cross(u, v) * 2.0f;
    return v + t * q.w + cross(u, t);
}
static inline Quat qnlerp(Quat a, Quat b, float t) {
    float d = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    float s = d < 0 ? -1.f : 1.f;
    return qnorm({a.x + (b.x * s - a.x) * t, a.y + (b.y * s - a.y) * t, a.z + (b.z * s - a.z) * t, a.w + (b.w * s - a.w) * t});
}
// quaternion from an orthonormal basis (columns X,Y,Z)
static inline Quat qbasis(V3 X, V3 Y, V3 Z) {
    float m00 = X.x, m10 = X.y, m20 = X.z, m01 = Y.x, m11 = Y.y, m21 = Y.z, m02 = Z.x, m12 = Z.y, m22 = Z.z;
    float tr = m00 + m11 + m22;
    Quat q;
    if (tr > 0) {
        float s = std::sqrt(tr + 1.0f) * 2;
        q.w = 0.25f * s; q.x = (m21 - m12) / s; q.y = (m02 - m20) / s; q.z = (m10 - m01) / s;
    } else if (m00 > m11 && m00 > m22) {
        float s = std::sqrt(1.0f + m00 - m11 - m22) * 2;
        q.w = (m21 - m12) / s; q.x = 0.25f * s; q.y = (m01 + m10) / s; q.z = (m02 + m20) / s;
    } else if (m11 > m22) {
        float s = std::sqrt(1.0f + m11 - m00 - m22) * 2;
        q.w = (m02 - m20) / s; q.x = (m01 + m10) / s; q.y = 0.25f * s; q.z = (m12 + m21) / s;
    } else {
        float s = std::sqrt(1.0f + m22 - m00 - m11) * 2;
        q.w = (m10 - m01) / s; q.x = (m02 + m20) / s; q.y = (m12 + m21) / s; q.z = 0.25f * s;
    }
    return qnorm(q);
}
// integrate angular velocity (world frame) into orientation
static inline Quat qintegrate(Quat q, V3 w, float dt) {
    Quat dq = {w.x * dt * 0.5f, w.y * dt * 0.5f, w.z * dt * 0.5f, 0};
    Quat r = qmul(dq, q);
    return qnorm({q.x + r.x, q.y + r.y, q.z + r.z, q.w + r.w});
}

struct Pose { Quat q; V3 p; };

// ---------------------------------------------------------------- rigid/uniform-scale transform used while modelling
struct Xf {
    V3 X{1, 0, 0}, Y{0, 1, 0}, Z{0, 0, 1}, T;
    V3 pt(V3 p) const { return T + X * p.x + Y * p.y + Z * p.z; }
    V3 vec(V3 d) const { return X * d.x + Y * d.y + Z * d.z; }
    V3 dir(V3 d) const { return norm(vec(d)); }
};
static inline Xf operator*(const Xf &a, const Xf &b) { // apply b first, then a
    Xf r;
    r.X = a.vec(b.X); r.Y = a.vec(b.Y); r.Z = a.vec(b.Z); r.T = a.pt(b.T);
    return r;
}
static inline Xf xfT(V3 t) { Xf r; r.T = t; return r; }
static inline Xf xfT(float x, float y, float z) { return xfT(V3(x, y, z)); }
static inline Xf xfS(float s) { Xf r; r.X = V3(s, 0, 0); r.Y = V3(0, s, 0); r.Z = V3(0, 0, s); return r; }
static inline Xf xfS(float sx, float sy, float sz) { Xf r; r.X = V3(sx, 0, 0); r.Y = V3(0, sy, 0); r.Z = V3(0, 0, sz); return r; }
static inline Xf xfRY(float a) { Xf r; float c = std::cos(a), s = std::sin(a); r.X = V3(c, 0, -s); r.Z = V3(s, 0, c); return r; }
static inline Xf xfRX(float a) { Xf r; float c = std::cos(a), s = std::sin(a); r.Y = V3(0, c, s); r.Z = V3(0, -s, c); return r; }
static inline Xf xfRZ(float a) { Xf r; float c = std::cos(a), s = std::sin(a); r.X = V3(c, s, 0); r.Y = V3(-s, c, 0); return r; }
static inline Xf xfQ(Quat q, V3 t) { Xf r; r.X = qrot(q, V3(1, 0, 0)); r.Y = qrot(q, V3(0, 1, 0)); r.Z = qrot(q, V3(0, 0, 1)); r.T = t; return r; }
// frame whose Y axis points along `up`
static inline Xf xfAlignY(V3 up) {
    Xf r; r.Y = norm(up);
    V3 a = std::fabs(r.Y.y) < 0.95f ? V3(0, 1, 0) : V3(1, 0, 0);
    r.X = norm(cross(a, r.Y)); r.Z = cross(r.X, r.Y);
    return r;
}

// ---------------------------------------------------------------- randomness
struct Rng {
    uint64_t s;
    explicit Rng(uint64_t seed = 1) : s(seed * 0x9E3779B97F4A7C15ull + 0x632BE59BD9B4E019ull) { u(); u(); }
    uint32_t next() {
        s = s * 6364136223846793005ull + 1442695040888963407ull;
        uint32_t x = (uint32_t)(((s >> 18u) ^ s) >> 27u), r = (uint32_t)(s >> 59u);
        return (x >> r) | (x << ((32 - r) & 31));
    }
    float u() { return (next() >> 8) * (1.0f / 16777216.0f); }
    float r(float a, float b) { return a + (b - a) * u(); }
    int i(int n) { return (int)(u() * n) % n; }
    float g() { float a = std::max(u(), 1e-7f), b = u(); return std::sqrt(-2 * std::log(a)) * std::cos(2 * PI * b); }
};
static inline float hash1(float n) { float s = std::sin(n * 127.1f + 311.7f) * 43758.5453f; return s - std::floor(s); }

// 1D smooth value noise (for hand-held style drift)
static inline float vnoise1(float x, float seed) {
    float i = std::floor(x), f = x - i;
    float u = f * f * (3 - 2 * f);
    return lerp(hash1(i + seed * 57.0f), hash1(i + 1 + seed * 57.0f), u) * 2 - 1;
}

// monotone cubic (PCHIP) interpolation through (t,y) keys
struct Pchip {
    std::vector<float> t, y, m;
    void add(float tt, float yy) { t.push_back(tt); y.push_back(yy); }
    void build() {
        int n = (int)t.size();
        m.assign(n, 0);
        if (n < 2) return;
        std::vector<float> d(n - 1), h(n - 1);
        for (int i = 0; i < n - 1; i++) { h[i] = t[i + 1] - t[i]; d[i] = (y[i + 1] - y[i]) / h[i]; }
        m[0] = d[0]; m[n - 1] = d[n - 2];
        for (int i = 1; i < n - 1; i++) {
            if (d[i - 1] * d[i] <= 0) m[i] = 0;
            else {
                float w1 = 2 * h[i] + h[i - 1], w2 = h[i] + 2 * h[i - 1];
                m[i] = (w1 + w2) / (w1 / d[i - 1] + w2 / d[i]);
            }
        }
    }
    float eval(float x) const {
        int n = (int)t.size();
        if (x <= t[0]) return y[0];
        if (x >= t[n - 1]) return y[n - 1];
        int i = int(std::upper_bound(t.begin(), t.end(), x) - t.begin()) - 1;
        float h = t[i + 1] - t[i], s = (x - t[i]) / h;
        float h00 = (1 + 2 * s) * (1 - s) * (1 - s), h10 = s * (1 - s) * (1 - s), h01 = s * s * (3 - 2 * s), h11 = s * s * (s - 1);
        return h00 * y[i] + h10 * h * m[i] + h01 * y[i + 1] + h11 * h * m[i + 1];
    }
};
