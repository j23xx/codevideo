// post.h — lens and film look applied to the linear HDR frame on the CPU, plus a tiny PNG writer.
#pragma once
#include "base.h"
#include <dispatch/dispatch.h>
#include <zlib.h>

static inline void parallelRows(int n, void (^blk)(size_t)) {
    dispatch_apply((size_t)n, dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), blk);
}

struct Img {
    int w = 0, h = 0;
    std::vector<float> d; // rgb
    void init(int W, int H) { w = W; h = H; d.assign((size_t)W * H * 3, 0); }
    float *at(int x, int y) { return &d[((size_t)y * w + x) * 3]; }
    const float *at(int x, int y) const { return &d[((size_t)y * w + x) * 3]; }
};

static void downsample(const Img &a, Img &b) {
    b.init(std::max(1, a.w / 2), std::max(1, a.h / 2));
    const Img *pa = &a; Img *pb = &b;
    parallelRows(b.h, ^(size_t y) {
        for (int x = 0; x < pb->w; x++) {
            int x0 = std::min(2 * x, pa->w - 1), x1 = std::min(2 * x + 1, pa->w - 1), y0 = std::min(2 * (int)y, pa->h - 1), y1 = std::min(2 * (int)y + 1, pa->h - 1);
            for (int c = 0; c < 3; c++) pb->at(x, (int)y)[c] = 0.25f * (pa->at(x0, y0)[c] + pa->at(x1, y0)[c] + pa->at(x0, y1)[c] + pa->at(x1, y1)[c]);
        }
    });
}
static void blur(Img &a, int radius) {
    Img t; t.init(a.w, a.h);
    std::vector<float> k(radius + 1);
    float sum = 0, sg = radius * 0.5f + 0.3f;
    for (int i = 0; i <= radius; i++) { k[i] = std::exp(-0.5f * i * i / (sg * sg)); sum += (i ? 2 : 1) * k[i]; }
    for (auto &v : k) v /= sum;
    Img *pa = &a, *pt = &t; const float *kk = k.data();
    parallelRows(a.h, ^(size_t y) {
        for (int x = 0; x < pa->w; x++) {
            float s[3] = {0, 0, 0};
            for (int i = -radius; i <= radius; i++) {
                const float *p = pa->at(std::min(std::max(x + i, 0), pa->w - 1), (int)y);
                float w = kk[std::abs(i)];
                s[0] += p[0] * w; s[1] += p[1] * w; s[2] += p[2] * w;
            }
            float *o = pt->at(x, (int)y); o[0] = s[0]; o[1] = s[1]; o[2] = s[2];
        }
    });
    parallelRows(a.h, ^(size_t y) {
        for (int x = 0; x < pa->w; x++) {
            float s[3] = {0, 0, 0};
            for (int i = -radius; i <= radius; i++) {
                const float *p = pt->at(x, std::min(std::max((int)y + i, 0), pa->h - 1));
                float w = kk[std::abs(i)];
                s[0] += p[0] * w; s[1] += p[1] * w; s[2] += p[2] * w;
            }
            float *o = pa->at(x, (int)y); o[0] = s[0]; o[1] = s[1]; o[2] = s[2];
        }
    });
}
static inline void bilin(const Img &a, float fx, float fy, float *o) {
    fx = clampf(fx, 0, a.w - 1.001f); fy = clampf(fy, 0, a.h - 1.001f);
    int x = (int)fx, y = (int)fy;
    float u = fx - x, v = fy - y;
    const float *p00 = a.at(x, y), *p10 = a.at(std::min(x + 1, a.w - 1), y), *p01 = a.at(x, std::min(y + 1, a.h - 1)), *p11 = a.at(std::min(x + 1, a.w - 1), std::min(y + 1, a.h - 1));
    for (int c = 0; c < 3; c++) o[c] = (p00[c] * (1 - u) + p10[c] * u) * (1 - v) + (p01[c] * (1 - u) + p11[c] * u) * v;
}
static inline float tonemap1(float x) { // filmic curve (ACES-like fit), x >= 0
    x *= 0.6f;
    return sat((x * (2.51f * x + 0.03f)) / (x * (2.43f * x + 0.59f) + 0.14f));
}
static inline float oetf(float v) { return v <= 0.0031308f ? 12.92f * v : 1.055f * std::pow(v, 1 / 2.4f) - 0.055f; }

struct PostParams {
    float exposure = 1.0f;
    float bloom = 0.07f;
    float vignette = 0.32f;
    float ca = 0.0016f;
    float grain = 0.006f;
    float fade = 1.0f; // global fade multiplier
    int seed = 0;
};

// hdr: w*h float4 (rgb, count). out: w*h*3 bytes
static void postProcess(const float *hdr, int W, int H, const PostParams &pp, std::vector<uint8_t> &out) {
    Img base; base.init(W, H);
    Img *pb = &base;
    parallelRows(H, ^(size_t y) {
        for (int x = 0; x < W; x++) {
            const float *p = hdr + ((size_t)y * W + x) * 4;
            float *o = pb->at(x, (int)y);
            for (int c = 0; c < 3; c++) { float v = p[c] * pp.exposure; o[c] = (v == v) ? std::min(v, 400.0f) : 0; }
        }
    });
    // bloom pyramid
    const int NL = 6;
    Img lv[NL];
    downsample(base, lv[0]);
    for (int i = 1; i < NL; i++) downsample(lv[i - 1], lv[i]);
    for (int i = 0; i < NL; i++) blur(lv[i], 3);
    static const float wl[NL] = {0.30f, 0.24f, 0.18f, 0.13f, 0.09f, 0.06f};
    out.resize((size_t)W * H * 3);
    uint8_t *po = out.data();
    Img *plv = lv;
    float asp = (float)H / W;
    parallelRows(H, ^(size_t yy) {
        int y = (int)yy;
        uint32_t rs = (uint32_t)(y * 9781 + pp.seed * 6271 + 1);
        auto rnd = [&]() { rs = rs * 747796405u + 2891336453u; uint32_t w = ((rs >> ((rs >> 28u) + 4u)) ^ rs) * 277803737u; w = (w >> 22u) ^ w; return (w >> 8) * (1.0f / 16777216.0f); };
        for (int x = 0; x < W; x++) {
            float nx = (x + 0.5f) / W * 2 - 1, ny = ((y + 0.5f) / H * 2 - 1) * asp;
            float r2 = nx * nx + ny * ny;
            // lateral chromatic aberration: sample R and B at slightly scaled radii
            float c[3], t[3];
            float sr = 1 + pp.ca * r2, sb = 1 - pp.ca * r2;
            bilin(*pb, ((nx * sr) * 0.5f + 0.5f) * W - 0.5f, ((ny * sr / asp) * 0.5f + 0.5f) * H - 0.5f, t); c[0] = t[0];
            c[1] = pb->at(x, y)[1];
            bilin(*pb, ((nx * sb) * 0.5f + 0.5f) * W - 0.5f, ((ny * sb / asp) * 0.5f + 0.5f) * H - 0.5f, t); c[2] = t[2];
            float bl[3] = {0, 0, 0};
            for (int l = 0; l < NL; l++) {
                float s = 1.0f / (2 << l);
                bilin(plv[l], (x + 0.5f) * s - 0.5f, (y + 0.5f) * s - 0.5f, t);
                bl[0] += t[0] * wl[l]; bl[1] += t[1] * wl[l]; bl[2] += t[2] * wl[l];
            }
            float vig = 1.0f - pp.vignette * smooth(0.25f, 1.45f, r2) - 0.08f * r2;
            float g = (rnd() + rnd() + rnd() + rnd() - 2.0f) * 1.7f; // ~gaussian
            float gc[3] = {rnd() - 0.5f, rnd() - 0.5f, rnd() - 0.5f};
            float v3[3];
            for (int k = 0; k < 3; k++) v3[k] = (c[k] * (1 - pp.bloom) + bl[k] * pp.bloom) * vig * pp.fade;
            // gentle saturation lift before the curve (action-camera colour)
            float L = 0.2126f * v3[0] + 0.7152f * v3[1] + 0.0722f * v3[2];
            for (int k = 0; k < 3; k++) {
                float v = std::max(0.0f, L + (v3[k] - L) * 1.08f);
                v = oetf(tonemap1(v));
                float gr = pp.grain * (g + 0.5f * gc[k]) * (0.35f + 1.3f * std::sqrt(std::max(v * (1 - v), 0.0f)));
                v = v + gr + (rnd() - 0.5f) / 255.0f;
                po[((size_t)y * W + x) * 3 + k] = (uint8_t)clampf(v * 255.0f + 0.5f, 0, 255);
            }
        }
    });
}

static bool writePNG(const char *path, const uint8_t *rgb, int w, int h) {
    std::vector<uint8_t> raw((size_t)(w * 3 + 1) * h);
    for (int y = 0; y < h; y++) {
        raw[(size_t)y * (w * 3 + 1)] = 0;
        memcpy(&raw[(size_t)y * (w * 3 + 1) + 1], rgb + (size_t)y * w * 3, (size_t)w * 3);
    }
    uLongf clen = compressBound((uLong)raw.size());
    std::vector<uint8_t> comp(clen);
    if (compress2(comp.data(), &clen, raw.data(), (uLong)raw.size(), 1) != Z_OK) return false;
    FILE *f = fopen(path, "wb");
    if (!f) return false;
    static const uint8_t sig[8] = {137, 80, 78, 71, 13, 10, 26, 10};
    fwrite(sig, 1, 8, f);
    auto chunk = [&](const char *type, const uint8_t *data, uint32_t len) {
        uint8_t b[4] = {(uint8_t)(len >> 24), (uint8_t)(len >> 16), (uint8_t)(len >> 8), (uint8_t)len};
        fwrite(b, 1, 4, f);
        fwrite(type, 1, 4, f);
        if (len) fwrite(data, 1, len, f);
        uLong crc = crc32(0, (const Bytef *)type, 4);
        if (len) crc = crc32(crc, data, len);
        uint8_t cb[4] = {(uint8_t)(crc >> 24), (uint8_t)(crc >> 16), (uint8_t)(crc >> 8), (uint8_t)crc};
        fwrite(cb, 1, 4, f);
    };
    uint8_t ih[13] = {(uint8_t)(w >> 24), (uint8_t)(w >> 16), (uint8_t)(w >> 8), (uint8_t)w, (uint8_t)(h >> 24), (uint8_t)(h >> 16), (uint8_t)(h >> 8), (uint8_t)h, 8, 2, 0, 0, 0};
    chunk("IHDR", ih, 13);
    chunk("IDAT", comp.data(), (uint32_t)clen);
    chunk("IEND", nullptr, 0);
    fclose(f);
    return true;
}
