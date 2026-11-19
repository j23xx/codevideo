// atlas.h — every sign, label and printed graphic is painted by code into one texture,
// using a stroke font defined below (also used to bend neon tubes).
#pragma once
#include "base.h"

static const int ATLAS_N = 4096;
static std::vector<uint8_t> ATLAS((size_t)ATLAS_N * ATLAS_N * 4, 0);

// ---------------------------------------------------------------- stroke font (4 x 6 unit cell)
static const char *glyphDef(char c) {
    switch (c) {
    case 'A': return "0,0 2,6 4,0|0.8,2 3.2,2";
    case 'B': return "0,0 0,6 2.5,6 3.5,5.2 3.5,3.8 2.5,3 0,3|2.5,3 4,2.2 4,0.8 3,0 0,0";
    case 'C': return "4,5 3,6 1,6 0,5 0,1 1,0 3,0 4,1";
    case 'D': return "0,0 0,6 2.5,6 4,4.5 4,1.5 2.5,0 0,0";
    case 'E': return "4,0 0,0 0,6 4,6|0,3 3,3";
    case 'F': return "0,0 0,6 4,6|0,3 3,3";
    case 'G': return "4,5 3,6 1,6 0,5 0,1 1,0 3,0 4,1 4,3 2.2,3";
    case 'H': return "0,0 0,6|4,0 4,6|0,3 4,3";
    case 'I': return "2,0 2,6";
    case 'J': return "0,1 1,0 2.5,0 3.5,1 3.5,6";
    case 'K': return "0,0 0,6|4,6 0,2.5|1.3,3.6 4,0";
    case 'L': return "0,6 0,0 4,0";
    case 'M': return "0,0 0,6 2,2.5 4,6 4,0";
    case 'N': return "0,0 0,6 4,0 4,6";
    case 'O': case '0': return "1,0 3,0 4,1 4,5 3,6 1,6 0,5 0,1 1,0";
    case 'P': return "0,0 0,6 3,6 4,5 4,4 3,3 0,3";
    case 'Q': return "1,0 3,0 4,1 4,5 3,6 1,6 0,5 0,1 1,0|2.5,1.5 4,0";
    case 'R': return "0,0 0,6 3,6 4,5 4,4 3,3 0,3|2,3 4,0";
    case 'S': return "4,5 3,6 1,6 0,5 0,4 1,3 3,3 4,2 4,1 3,0 1,0 0,1";
    case 'T': return "2,0 2,6|0,6 4,6";
    case 'U': return "0,6 0,1 1,0 3,0 4,1 4,6";
    case 'V': return "0,6 2,0 4,6";
    case 'W': return "0,6 1,0 2,3.5 3,0 4,6";
    case 'X': return "0,0 4,6|0,6 4,0";
    case 'Y': return "0,6 2,3 4,6|2,3 2,0";
    case 'Z': return "0,6 4,6 0,0 4,0";
    case '1': return "1,5 2,6 2,0|1,0 3,0";
    case '2': return "0,5 1,6 3,6 4,5 4,3.8 0,0 4,0";
    case '3': return "0,5 1,6 3,6 4,5 4,4 3,3 1.5,3|3,3 4,2 4,1 3,0 1,0 0,1";
    case '4': return "3,0 3,6 0,2 4,2";
    case '5': return "4,6 0,6 0,3.3 3,3.3 4,2.3 4,1 3,0 1,0 0,1";
    case '6': return "3.5,6 1.5,6 0,4.2 0,1 1,0 3,0 4,1 4,2.4 3,3.4 0.2,3.4";
    case '7': return "0,6 4,6 1.5,0";
    case '8': return "1,3 0,4 0,5 1,6 3,6 4,5 4,4 3,3 1,3 0,2 0,1 1,0 3,0 4,1 4,2 3,3";
    case '9': return "0.5,0 2.5,0 4,1.8 4,5 3,6 1,6 0,5 0,3.6 1,2.6 3.8,2.6";
    case '-': return "0.8,3 3.2,3";
    case '.': return "2,0 2,0.15";
    case '!': return "2,2 2,6|2,0 2,0.15";
    case '\'': return "2,4.8 2,6";
    case ':': return "2,1.5 2,1.65|2,4 2,4.15";
    case '&': return "4,0 1,4.5 1,5.3 1.8,6 2.6,5.3 2.6,4.6 0,2 0,1 1,0 2,0 4,2.5";
    case '/': return "0,0 4,6";
    case '*': return "2,1 2,5|0.3,2 3.7,4|0.3,4 3.7,2";
    case '+': return "2,1 2,5|0,3 4,3";
    case '$': return "4,5 3,5.5 1,5.5 0,4.7 0,3.8 1,3 3,3 4,2.2 4,1.3 3,0.5 1,0.5 0,1|2,0 2,6";
    case 'c': return "3.5,3.5 2.8,4 1.6,4 1,3.4 1,1.6 1.6,1 2.8,1 3.5,1.5|2.2,0.3 2.2,4.7"; // cent sign
    default: return "";
    }
}
static inline bool narrowGlyph(char c) { return c == 'I' || c == '.' || c == '!' || c == '\'' || c == ':' || c == '1'; }
static inline float glyphAdvance(char c) { return c == ' ' ? 3.0f : (narrowGlyph(c) ? 3.2f : 5.6f); }
// strokes of a text line in glyph units (origin at left of baseline); returns total width
static float textStrokes(const std::string &s, std::vector<std::vector<V2>> &out) {
    float x = 0;
    for (char ch : s) {
        const char *d = glyphDef(ch);
        float shift = narrowGlyph(ch) ? -1.2f : 0.0f;
        std::vector<V2> cur;
        const char *p = d;
        while (*p) {
            char *e;
            float gx = strtof(p, &e); p = e + 1;
            float gy = strtof(p, &e); p = e;
            cur.push_back(V2(x + gx + shift, gy));
            if (*p == '|') { out.push_back(cur); cur.clear(); p++; }
            else if (*p == ' ') p++;
        }
        if (!cur.empty()) out.push_back(cur);
        x += glyphAdvance(ch);
    }
    return x - 1.6f;
}
static float textWidth(const std::string &s) { float x = 0; for (char ch : s) x += glyphAdvance(ch); return x - 1.6f; }

// ---------------------------------------------------------------- painter
struct Region { int x = 0, y = 0, w = 0, h = 0; };
struct AtlasAlloc { int x = 2, y = 2, rowH = 0; };
static AtlasAlloc AAL;
static Region atlasAlloc(int w, int h) {
    if (AAL.x + w + 2 > ATLAS_N) { AAL.x = 2; AAL.y += AAL.rowH + 4; AAL.rowH = 0; }
    Region r; r.x = AAL.x; r.y = AAL.y; r.w = w; r.h = h;
    AAL.x += w + 4; AAL.rowH = std::max(AAL.rowH, h);
    if (r.y + h > ATLAS_N) { fprintf(stderr, "atlas full!\n"); r.x = r.y = 2; }
    return r;
}
struct Col { float r, g, b; };
static inline Col hexc(int h) { return {((h >> 16) & 255) / 255.0f, ((h >> 8) & 255) / 255.0f, (h & 255) / 255.0f}; }
static inline void aBlend(const Region &rg, int x, int y, Col c, float a) {
    if (x < 0 || y < 0 || x >= rg.w || y >= rg.h || a <= 0) return;
    uint8_t *p = &ATLAS[((size_t)(rg.y + y) * ATLAS_N + rg.x + x) * 4];
    a = sat(a);
    p[0] = (uint8_t)(p[0] + (c.r * 255 - p[0]) * a); p[1] = (uint8_t)(p[1] + (c.g * 255 - p[1]) * a); p[2] = (uint8_t)(p[2] + (c.b * 255 - p[2]) * a);
    p[3] = 255;
}
static void aFill(const Region &rg, Col c) { for (int y = -1; y <= rg.h; y++) for (int x = -1; x <= rg.w; x++) { // bleed 1px
        int xx = rg.x + x, yy = rg.y + y; if (xx < 0 || yy < 0 || xx >= ATLAS_N || yy >= ATLAS_N) continue;
        uint8_t *p = &ATLAS[((size_t)yy * ATLAS_N + xx) * 4]; p[0] = (uint8_t)(c.r * 255); p[1] = (uint8_t)(c.g * 255); p[2] = (uint8_t)(c.b * 255); p[3] = 255; } }
static void aRect(const Region &rg, float x0, float y0, float x1, float y1, Col c, float a = 1) {
    for (int y = (int)std::floor(y0); y <= (int)std::ceil(y1); y++)
        for (int x = (int)std::floor(x0); x <= (int)std::ceil(x1); x++) {
            float cov = sat(std::min(std::min(x + 1 - x0, x1 - x), 1.0f)) * sat(std::min(std::min(y + 1 - y0, y1 - y), 1.0f));
            aBlend(rg, x, y, c, cov * a);
        }
}
static void aLine(const Region &rg, float x0, float y0, float x1, float y1, float w, Col c, float a = 1) {
    float hw = w * 0.5f;
    int bx0 = (int)std::floor(std::min(x0, x1) - hw - 1), bx1 = (int)std::ceil(std::max(x0, x1) + hw + 1);
    int by0 = (int)std::floor(std::min(y0, y1) - hw - 1), by1 = (int)std::ceil(std::max(y0, y1) + hw + 1);
    float dx = x1 - x0, dy = y1 - y0, l2 = std::max(dx * dx + dy * dy, 1e-6f);
    for (int y = by0; y <= by1; y++)
        for (int x = bx0; x <= bx1; x++) {
            float px = x + 0.5f - x0, py = y + 0.5f - y0;
            float t = sat((px * dx + py * dy) / l2);
            float d = std::sqrt((px - dx * t) * (px - dx * t) + (py - dy * t) * (py - dy * t));
            aBlend(rg, x, y, c, sat(hw - d + 0.5f) * a);
        }
}
static void aCircle(const Region &rg, float cx, float cy, float r, Col c, float a = 1, float ring = 0) {
    for (int y = (int)(cy - r - 2); y <= (int)(cy + r + 2); y++)
        for (int x = (int)(cx - r - 2); x <= (int)(cx + r + 2); x++) {
            float d = std::sqrt((x + 0.5f - cx) * (x + 0.5f - cx) + (y + 0.5f - cy) * (y + 0.5f - cy));
            float cov = ring > 0 ? sat(ring * 0.5f - std::fabs(d - r) + 0.5f) : sat(r - d + 0.5f);
            aBlend(rg, x, y, c, cov * a);
        }
}
static void aTri(const Region &rg, V2 p0, V2 p1, V2 p2, Col c, float a = 1) {
    float ar = (p1.x - p0.x) * (p2.y - p0.y) - (p1.y - p0.y) * (p2.x - p0.x);
    if (ar < 0) std::swap(p1, p2);
    int bx0 = (int)std::floor(std::min(p0.x, std::min(p1.x, p2.x))) - 1, bx1 = (int)std::ceil(std::max(p0.x, std::max(p1.x, p2.x))) + 1;
    int by0 = (int)std::floor(std::min(p0.y, std::min(p1.y, p2.y))) - 1, by1 = (int)std::ceil(std::max(p0.y, std::max(p1.y, p2.y))) + 1;
    auto ed = [](V2 a, V2 b, float x, float y) { float dx = b.x - a.x, dy = b.y - a.y; float l = std::max(std::sqrt(dx * dx + dy * dy), 1e-6f); return ((x - a.x) * dy - (y - a.y) * dx) / -l; };
    for (int y = by0; y <= by1; y++)
        for (int x = bx0; x <= bx1; x++) {
            float fx = x + 0.5f, fy = y + 0.5f;
            float d = std::min(ed(p0, p1, fx, fy), std::min(ed(p1, p2, fx, fy), ed(p2, p0, fx, fy)));
            aBlend(rg, x, y, c, sat(d + 0.5f) * a);
        }
}
// text with the stroke font. (x,y) = left / TOP of the cap height, in region pixels (y down). slant: italic shear
static float aText(const Region &rg, const std::string &s, float x, float y, float h, float weight, Col c, float slant = 0, float a = 1) {
    std::vector<std::vector<V2>> st;
    float w = textStrokes(s, st);
    float sc = h / 6.0f;
    for (auto &pl : st)
        for (size_t i = 0; i + 1 < pl.size() || (pl.size() == 1 && i == 0); i++) {
            V2 p = pl[i], q = pl[std::min(i + 1, pl.size() - 1)];
            aLine(rg, x + (p.x + slant * p.y) * sc, y + (6 - p.y) * sc, x + (q.x + slant * q.y) * sc, y + (6 - q.y) * sc, weight, c, a);
        }
    return w * sc;
}
static float aTextC(const Region &rg, const std::string &s, float cx, float y, float h, float weight, Col c, float slant = 0) {
    float w = textWidth(s) * h / 6.0f;
    return aText(rg, s, cx - w * 0.5f - slant * h * 0.5f, y, h, weight, c, slant);
}
static void aStarburst(const Region &rg, float cx, float cy, float r, int n, Col c, float seed = 0, float wid = 0.06f) {
    for (int i = 0; i < n; i++) {
        float a = 2 * PI * i / n + seed;
        float rl = r * ((i & 1) ? 0.55f : 1.0f) * (0.8f + 0.2f * hash1(i * 3.1f + seed));
        V2 d(std::cos(a), std::sin(a)), pd(-d.y, d.x);
        aTri(rg, V2(cx, cy) + pd * (r * wid), V2(cx, cy) - pd * (r * wid), V2(cx, cy) + d * rl, c);
    }
    aCircle(rg, cx, cy, r * 0.10f, c);
}
static void aBoomerang(const Region &rg, float cx, float cy, float s, float ang, Col c) {
    V2 a(std::cos(ang), std::sin(ang)), b(std::cos(ang + 1.9f), std::sin(ang + 1.9f));
    for (int i = 0; i < 12; i++) {
        float t0 = i / 12.0f, t1 = (i + 1) / 12.0f;
        aLine(rg, cx + a.x * s * t0, cy + a.y * s * t0, cx + a.x * s * t1, cy + a.y * s * t1, s * 0.34f * (1 - 0.75f * t0), c);
        aLine(rg, cx + b.x * s * t0, cy + b.y * s * t0, cx + b.x * s * t1, cy + b.y * s * t1, s * 0.34f * (1 - 0.75f * t0), c);
    }
}
// UV rect of a region
static inline void regionUV(const Region &r, float &u0, float &v0, float &u1, float &v1) {
    u0 = (float)r.x / ATLAS_N; u1 = (float)(r.x + r.w) / ATLAS_N;
    v1 = (float)r.y / ATLAS_N; v0 = (float)(r.y + r.h) / ATLAS_N; // quad v runs bottom->top
}
