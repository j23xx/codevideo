// main.mm — host: builds the world, uploads it to Metal, drives the path-tracing kernel frame by frame,
// denoises, applies the lens/film look and writes PNG frames.
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#include <sys/stat.h>
#include <chrono>

#include "world3.h"
#include "flight.h"
#include "post.h"

struct GTri { float v0[3]; int32_t mat; float e1[3]; int32_t flags; float e2[3]; float var; };
struct GTriS { float n0[4], n1[4], n2[4], uv01[4], uv2[4]; };
struct GLight { float pos[4], u[4], v[4], nrm[4], emit[4]; };
struct GInst { float q0[4]; float p0[3]; int32_t root; float q1[4]; float p1[3]; float var; float mn[4], mx[4]; };
struct GUni {
    float camP0[4], camQ0[4], camP1[4], camQ1[4], lens[4], misc[4], haze[4], sky[4];
    int32_t dims[4], counts[4];
};

static double nowSec() { return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
static void set4(float *d, V3 v, float w) { d[0] = v.x; d[1] = v.y; d[2] = v.z; d[3] = w; }
static void setQ(float *d, Quat q) { d[0] = q.x; d[1] = q.y; d[2] = q.z; d[3] = q.w; }

int main(int argc, char **argv) {
    @autoreleasepool {
        int W = 960, H = 540, spp = 16, from = 0, to = -1, step = 1, maxBounce = 4, sppPass = 2;
        double still = -1, fps = 30;
        const char *outDir = "work/frames";
        const char *meterPath = nullptr, *expoPath = nullptr, *timelinePath = nullptr, *shaderPath = "src/render.metal";
        bool haveEye = false, denoise = true, raw = false, temporal = true;
        float alphaD = 0.1f, alphaS = 0.12f, clipG = 1.25f, sigD = 3.0f, sigS = 2.6f;
        int preroll = 8;
        float dbg = 0, taaA = 0.4f;
        V3 eye, at; float fovOverride = -1; double tauOverride = -1e9;
        float evBias = 0;
        for (int i = 1; i < argc; i++) {
            std::string a = argv[i];
            auto nx = [&]() { return i + 1 < argc ? argv[++i] : "0"; };
            if (a == "--w") W = atoi(nx());
            else if (a == "--h") H = atoi(nx());
            else if (a == "--spp") spp = atoi(nx());
            else if (a == "--pass") sppPass = atoi(nx());
            else if (a == "--from") from = atoi(nx());
            else if (a == "--to") to = atoi(nx());
            else if (a == "--step") step = atoi(nx());
            else if (a == "--t") still = atof(nx());
            else if (a == "--out") outDir = nx();
            else if (a == "--bounce") maxBounce = atoi(nx());
            else if (a == "--meter") meterPath = nx();
            else if (a == "--expo") expoPath = nx();
            else if (a == "--timeline") timelinePath = nx();
            else if (a == "--shader") shaderPath = nx();
            else if (a == "--nodenoise") denoise = false;
            else if (a == "--raw") raw = true;
            else if (a == "--notemporal") temporal = false;
            else if (a == "--alphad") alphaD = atof(nx());
            else if (a == "--alphas") alphaS = atof(nx());
            else if (a == "--clip") clipG = atof(nx());
            else if (a == "--sigd") sigD = atof(nx());
            else if (a == "--sigs") sigS = atof(nx());
            else if (a == "--preroll") preroll = atoi(nx());
            else if (a == "--dbg") dbg = atof(nx());
            else if (a == "--taa") taaA = atof(nx());
            else if (a == "--ev") evBias = atof(nx());
            else if (a == "--fov") fovOverride = atof(nx());
            else if (a == "--tau") tauOverride = atof(nx());
            else if (a == "--eye") { sscanf(nx(), "%f,%f,%f", &eye.x, &eye.y, &eye.z); haveEye = true; }
            else if (a == "--at") { sscanf(nx(), "%f,%f,%f", &at.x, &at.y, &at.z); }
        }
        double t0 = nowSec();
        buildWorld();
        buildFlight();
        if (timelinePath) { writeTimeline(timelinePath); return 0; }
        if (getenv("DUMP_FLIGHT")) { dumpFlight(getenv("DUMP_FLIGHT")); return 0; }

        // ---- flatten geometry: static world first (root 0), then each prototype with its own BVH
        // pre-split very large triangles so that their bounding boxes do not spoil the BVH
        std::vector<TriC> tris;
        {
            float maxEdge = getenv("SPLIT") ? atof(getenv("SPLIT")) : 1.5f;
            std::vector<TriC> work = STATIC_TRIS;
            tris.reserve(work.size() * 2);
            while (!work.empty()) {
                TriC t = work.back(); work.pop_back();
                float l[3]; int k = 0;
                for (int i = 0; i < 3; i++) { l[i] = len(t.p[(i + 1) % 3] - t.p[i]); if (l[i] > l[k]) k = i; }
                // far from the building nothing needs fine pieces: let the limit grow with distance
                V3 cen = (t.p[0] + t.p[1] + t.p[2]) * (1.0f / 3.0f);
                float far = std::max({std::fabs(cen.x) - 30.0f, std::fabs(cen.z) - 50.0f, 0.0f});
                if (!(l[k] > maxEdge * (1.0f + far * 0.15f))) { tris.push_back(t); continue; }
                int a = k, b = (k + 1) % 3, c = (k + 2) % 3;
                V3 pm = (t.p[a] + t.p[b]) * 0.5f, nm = norm(t.n[a] + t.n[b]);
                float um = (t.uv[a][0] + t.uv[b][0]) * 0.5f, vm = (t.uv[a][1] + t.uv[b][1]) * 0.5f;
                TriC t1 = t, t2 = t;
                t1.p[b] = pm; t1.n[b] = nm; t1.uv[b][0] = um; t1.uv[b][1] = vm;   // a, m, c
                t2.p[a] = pm; t2.n[a] = nm; t2.uv[a][0] = um; t2.uv[a][1] = vm;   // m, b, c
                (void)c;
                work.push_back(t1); work.push_back(t2);
            }
        }
        std::vector<BNode> nodes;
        int nStatic = (int)tris.size();
        {
            BvhBuilder b(tris, nodes);
            b.build(0, nStatic);
        }
        for (auto &p : PROTOS) {
            p.first = (int)tris.size(); p.count = (int)p.tris.size();
            tris.insert(tris.end(), p.tris.begin(), p.tris.end());
            p.mn = V3(1e30f); p.mx = V3(-1e30f);
            for (auto &t : p.tris) for (int k = 0; k < 3; k++) { p.mn = vmin(p.mn, t.p[k]); p.mx = vmax(p.mx, t.p[k]); }
            if (p.count == 0) { p.root = 0; continue; }
            BvhBuilder b(tris, nodes);
            p.root = b.build(p.first, p.count);
        }
        fprintf(stderr, "world: %d static tris, %d total, %zu nodes, %zu mats, %zu lights, %zu protos, %zu dyn  (%.2fs)\n", nStatic, (int)tris.size(),
                nodes.size(), MATS.size(), LIGHTS.size(), PROTOS.size(), DYNS.size(), nowSec() - t0);

        id<MTLDevice> dev = MTLCreateSystemDefaultDevice();
        id<MTLCommandQueue> queue = [dev newCommandQueue];
        NSError *err = nil;
        NSString *src = [NSString stringWithContentsOfFile:[NSString stringWithUTF8String:shaderPath] encoding:NSUTF8StringEncoding error:&err];
        if (!src) { fprintf(stderr, "cannot read shader %s\n", shaderPath); return 1; }
        MTLCompileOptions *opt = [MTLCompileOptions new];
        opt.fastMathEnabled = YES;
        id<MTLLibrary> lib = [dev newLibraryWithSource:src options:opt error:&err];
        if (!lib) { fprintf(stderr, "shader compile failed:\n%s\n", [[err description] UTF8String]); return 1; }
        auto pipe = [&](const char *name) {
            NSError *e = nil;
            id<MTLComputePipelineState> p = [dev newComputePipelineStateWithFunction:[lib newFunctionWithName:[NSString stringWithUTF8String:name]] error:&e];
            if (!p) { fprintf(stderr, "pipeline %s failed: %s\n", name, [[e description] UTF8String]); exit(1); }
            return p;
        };
        id<MTLComputePipelineState> psoTrace = pipe("trace"), psoResolve = pipe("resolve"), psoAtrous = pipe("atrous"), psoCompose = pipe("compose"), psoTemporal = pipe("temporal"), psoTaa = pipe("taa");

        auto mkbuf = [&](size_t bytes) { return [dev newBufferWithLength:std::max<size_t>(bytes, 16) options:MTLResourceStorageModeShared]; };
        // geometry buffers
        id<MTLBuffer> bNodes = mkbuf(nodes.size() * sizeof(BNode));
        memcpy([bNodes contents], nodes.data(), nodes.size() * sizeof(BNode));
        id<MTLBuffer> bTris = mkbuf(tris.size() * sizeof(GTri)), bTriS = mkbuf(tris.size() * sizeof(GTriS));
        {
            GTri *gt = (GTri *)[bTris contents];
            GTriS *gs = (GTriS *)[bTriS contents];
            for (size_t i = 0; i < tris.size(); i++) {
                const TriC &t = tris[i];
                V3 e1 = t.p[1] - t.p[0], e2 = t.p[2] - t.p[0];
                gt[i].v0[0] = t.p[0].x; gt[i].v0[1] = t.p[0].y; gt[i].v0[2] = t.p[0].z; gt[i].mat = t.mat;
                gt[i].e1[0] = e1.x; gt[i].e1[1] = e1.y; gt[i].e1[2] = e1.z; gt[i].flags = t.flags;
                gt[i].e2[0] = e2.x; gt[i].e2[1] = e2.y; gt[i].e2[2] = e2.z; gt[i].var = t.var;
                set4(gs[i].n0, t.n[0], 0); set4(gs[i].n1, t.n[1], 0); set4(gs[i].n2, t.n[2], 0);
                gs[i].uv01[0] = t.uv[0][0]; gs[i].uv01[1] = t.uv[0][1]; gs[i].uv01[2] = t.uv[1][0]; gs[i].uv01[3] = t.uv[1][1];
                gs[i].uv2[0] = t.uv[2][0]; gs[i].uv2[1] = t.uv[2][1]; gs[i].uv2[2] = 0; gs[i].uv2[3] = 0;
            }
        }
        id<MTLBuffer> bMats = mkbuf(MATS.size() * sizeof(MatDef));
        id<MTLBuffer> bLights = mkbuf(LIGHTS.size() * sizeof(GLight));
        // light grid: per cell, the lights that contribute at least ~1% there (unoccluded estimate)
        const int LGX = 30, LGY = 4, LGZ = 29;
        const V3 LGMIN(-45.0f, -1.0f, -60.0f), LGCELL(3.0f, 2.5f, 3.0f);
        std::vector<int32_t> lgCells(LGX * LGY * LGZ * 2), lgIdx;
        {
            size_t maxList = 0, total = 0;
            for (int iz = 0; iz < LGZ; iz++) for (int iy = 0; iy < LGY; iy++) for (int ix = 0; ix < LGX; ix++) {
                std::vector<std::pair<float, int>> w(LIGHTS.size());
                float sum = 0;
                for (size_t li = 0; li < LIGHTS.size(); li++) {
                    const LightDef &L = LIGHTS[li];
                    float lumE = 0.2126f * L.emit.x + 0.7152f * L.emit.y + 0.0722f * L.emit.z;
                    V3 n = L.type != 1 ? cross(L.u, L.v) : V3(0, 1, 0);
                    float area = L.type != 1 ? 4 * len(n) : 4 * PI * L.radius * L.radius;
                    n = norm(n);
                    float best = 0;
                    for (int c = 0; c < 27; c++) {
                        V3 p = LGMIN + V3((ix + (c % 3) * 0.5f) * LGCELL.x, (iy + ((c / 3) % 3) * 0.5f) * LGCELL.y, (iz + (c / 9) * 0.5f) * LGCELL.z);
                        V3 d = L.pos - p; float d2 = dot(d, d);
                        float cl = 1;
                        if (L.type != 1) { cl = dot(n, d * (-1.0f / std::sqrt(std::max(d2, 1e-8f)))); cl = (L.two ? std::fabs(cl) : std::max(cl, 0.0f)) + 0.05f; }
                        best = std::max(best, lumE * area * cl / (d2 + area + 0.3f));
                    }
                    w[li] = {best, (int)li}; sum += best;
                }
                std::sort(w.begin(), w.end(), [](auto &a, auto &b) { return a.first > b.first; });
                int cell = (iz * LGY + iy) * LGX + ix;
                lgCells[cell * 2] = (int)lgIdx.size();
                int cnt = 0;
                for (auto &e : w) { if (cnt >= 44 || (cnt >= 4 && e.first < 0.0015f * sum) || e.first <= 0) break; lgIdx.push_back(e.second); cnt++; }
                lgCells[cell * 2 + 1] = cnt;
                maxList = std::max(maxList, (size_t)cnt); total += cnt;
            }
            fprintf(stderr, "light grid: avg %.1f lights per cell, max %zu\n", (double)total / (LGX * LGY * LGZ), maxList);
        }
        id<MTLBuffer> bLgCells = mkbuf(lgCells.size() * 4), bLgIdx = mkbuf(std::max<size_t>(lgIdx.size(), 4) * 4);
        memcpy([bLgCells contents], lgCells.data(), lgCells.size() * 4);
        memcpy([bLgIdx contents], lgIdx.data(), lgIdx.size() * 4);
        id<MTLBuffer> bInst = mkbuf(std::max<size_t>(DYNS.size(), 1) * sizeof(GInst));
        id<MTLBuffer> bINodes = mkbuf(std::max<size_t>(DYNS.size(), 1) * 2 * sizeof(BNode));
        size_t npx = (size_t)W * H;
        id<MTLBuffer> accD = mkbuf(npx * 16), accS = mkbuf(npx * 16), accE = mkbuf(npx * 16);
        // guide buffers and accumulated history exist twice: this frame's and the previous frame's
        id<MTLBuffer> accA[2], accN[2], accP[2], accX[2], hD[2], hS[2], hE[2], hM[2];
        for (int k = 0; k < 2; k++) {
            accA[k] = mkbuf(npx * 16); accN[k] = mkbuf(npx * 16); accP[k] = mkbuf(npx * 16); accX[k] = mkbuf(npx * 16);
            hD[k] = mkbuf(npx * 16); hS[k] = mkbuf(npx * 16); hE[k] = mkbuf(npx * 16); hM[k] = mkbuf(npx * 16);
        }
        id<MTLBuffer> rD = mkbuf(npx * 16), rS = mkbuf(npx * 16), rE = mkbuf(npx * 16);
        id<MTLBuffer> d0 = mkbuf(npx * 16), d1 = mkbuf(npx * 16), s0 = mkbuf(npx * 16), s1 = mkbuf(npx * 16), hdr = mkbuf(npx * 16);
        id<MTLBuffer> fin[2] = {mkbuf(npx * 16), mkbuf(npx * 16)};     // finished frames: this one and the previous one
        struct GPose { float q[4], p[4]; };
        id<MTLBuffer> bDyn[2] = {mkbuf(std::max<size_t>(DYNS.size(), 1) * sizeof(GPose)), mkbuf(std::max<size_t>(DYNS.size(), 1) * sizeof(GPose))};

        // atlas texture (painted by code in atlas.h)
        MTLTextureDescriptor *td = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm_sRGB width:ATLAS_N height:ATLAS_N mipmapped:YES];
        td.usage = MTLTextureUsageShaderRead;
        id<MTLTexture> atlas = [dev newTextureWithDescriptor:td];
        [atlas replaceRegion:MTLRegionMake2D(0, 0, ATLAS_N, ATLAS_N) mipmapLevel:0 withBytes:ATLAS.data() bytesPerRow:ATLAS_N * 4];
        {
            id<MTLCommandBuffer> cb = [queue commandBuffer];
            id<MTLBlitCommandEncoder> bl = [cb blitCommandEncoder];
            [bl generateMipmapsForTexture:atlas];
            [bl endEncoding];
            [cb commit];
            [cb waitUntilCompleted];
        }

        // exposure curve (from the metering pre-pass), one value per frame
        std::vector<float> expo;
        if (expoPath) {
            FILE *f = fopen(expoPath, "r");
            if (f) { float v; while (fscanf(f, "%f", &v) == 1) expo.push_back(v); fclose(f); }
        }
        FILE *meterF = meterPath ? fopen(meterPath, "w") : nullptr;
        mkdir("work", 0755);
        mkdir(outDir, 0755);

        std::vector<int> frames;
        if (still >= 0 || haveEye) frames.push_back(still >= 0 ? (int)llround(still * fps) : 0);
        else { if (to < 0) to = (int)llround(FILM_LEN * fps) - 1; for (int f = from; f <= to; f += step) frames.push_back(f); }

        std::vector<uint8_t> rgb8;
        struct GTUni { float prevP[4], prevQ[4], curP[4], curQ[4], prm[4], prm2[4], prm3[4]; };
        int histFrame = -1000000, cur = 0;     // frame whose accumulated light sits in set 1 - cur
        Cam histCam;
        float lastExpo = 3.0f;
        bool temporalOn = temporal && frames.size() > 1 && step == 1 && !meterF;

        auto renderFrame = [&](int fr, bool write) {
            char path[512];
            snprintf(path, sizeof path, "%s/f_%05d.png", outDir, fr);
            double tf = nowSec();
            double tv = fr / fps;                       // video time at frame centre
            double sh = 0.5 / fps;                      // 180 degree shutter
            double tvA = tv - sh * 0.5, tvB = tv + sh * 0.5;
            Cam cA = camAt(tvA), cB = camAt(tvB), cM = camAt(tv);
            double tauA = worldTime(tvA), tauB = worldTime(tvB);
            if (tauOverride > -1e8) { tauA = tauOverride; tauB = tauOverride + sh; }
            if (haveEye) {
                cA = lookCam(eye, at, V3(0, 1, 0), 0);
                cB = cA; cM = cA;
            }
            if (fovOverride > 0) cA.fov = cB.fov = fovOverride;
            int prv = 1 - cur;
            bool useHist = temporalOn && histFrame == fr - 1;

            // animated emitters
            {
                MatDef *gm = (MatDef *)[bMats contents];
                memcpy(gm, MATS.data(), MATS.size() * sizeof(MatDef));
                for (size_t i = 0; i < MATS.size(); i++)
                    if (MAT_ANIM[i]) { float a = ANIMS[MAT_ANIM[i]](tauA); gm[i].emit[0] = MAT_EMIT0[i].x * a; gm[i].emit[1] = MAT_EMIT0[i].y * a; gm[i].emit[2] = MAT_EMIT0[i].z * a; }
                GLight *gl = (GLight *)[bLights contents];
                for (size_t i = 0; i < LIGHTS.size(); i++) {
                    const LightDef &L = LIGHTS[i];
                    float a = L.anim ? ANIMS[L.anim](tauA) : 1.0f;
                    V3 e = L.emit * a;
                    float lumE = 0.2126f * e.x + 0.7152f * e.y + 0.0722f * e.z;
                    if (L.type != 1) {
                        V3 n = cross(L.u, L.v);
                        float area = 4 * len(n);
                        set4(gl[i].pos, L.pos, (float)L.type); set4(gl[i].u, L.u, area); set4(gl[i].v, L.v, L.two ? 1.f : 0.f);
                        set4(gl[i].nrm, norm(n), lumE * area); set4(gl[i].emit, e, 0);
                    } else {
                        float area = 4 * PI * L.radius * L.radius;
                        set4(gl[i].pos, L.pos, 1); set4(gl[i].u, V3(), area); set4(gl[i].v, V3(), 0);
                        set4(gl[i].nrm, V3(0, 1, 0), lumE * area); set4(gl[i].emit, e, L.radius);
                    }
                }
            }
            // animated instances at shutter open / close, and a small BVH over them
            int nVis = 0;
            {
                GInst *gi = (GInst *)[bInst contents];
                GPose *gp = (GPose *)[bDyn[cur] contents];
                std::vector<V3> bmn, bmx;
                for (size_t i = 0; i < DYNS.size(); i++) {
                    Pose a = DYNS[i].pose(tauA), b = DYNS[i].pose(tauB);
                    {   // mid-shutter pose, for following the object from frame to frame
                        Pose m = DYNS[i].pose((tauA + tauB) * 0.5);
                        setQ(gp[i].q, m.q); set4(gp[i].p, m.p, 0);
                    }
                    const Proto &p = PROTOS[DYNS[i].proto];
                    if (p.count == 0 || (a.p.y < -40 && b.p.y < -40)) continue;   // hidden
                    float d = a.q.x * b.q.x + a.q.y * b.q.y + a.q.z * b.q.z + a.q.w * b.q.w;
                    if (d < 0) { b.q.x = -b.q.x; b.q.y = -b.q.y; b.q.z = -b.q.z; b.q.w = -b.q.w; }
                    GInst &g = gi[nVis];
                    setQ(g.q0, a.q); setQ(g.q1, b.q);
                    g.p0[0] = a.p.x; g.p0[1] = a.p.y; g.p0[2] = a.p.z; g.root = p.root;
                    g.p1[0] = b.p.x; g.p1[1] = b.p.y; g.p1[2] = b.p.z; g.var = DYNS[i].var;
                    V3 mn(1e30f), mx(-1e30f);
                    for (int c = 0; c < 8; c++) {
                        V3 q((c & 1) ? p.mx.x : p.mn.x, (c & 2) ? p.mx.y : p.mn.y, (c & 4) ? p.mx.z : p.mn.z);
                        V3 wa = a.p + qrot(a.q, q), wb = b.p + qrot(b.q, q);
                        mn = vmin(mn, vmin(wa, wb)); mx = vmax(mx, vmax(wa, wb));
                    }
                    V3 pad = (mx - mn) * 0.02f + V3(0.002f);
                    mn = mn - pad; mx = mx + pad;
                    set4(g.mn, mn, (float)i); set4(g.mx, mx, 0);      // mn.w: which moving object this is
                    bmn.push_back(mn); bmx.push_back(mx);
                    nVis++;
                }
                BNode *in = (BNode *)[bINodes contents];
                std::vector<int> idx(nVis);
                for (int i = 0; i < nVis; i++) idx[i] = i;
                int nn = 1;
                std::function<void(int, int, int)> rec = [&](int ni, int first, int count) {
                    V3 mn(1e30f), mx(-1e30f);
                    for (int i = first; i < first + count; i++) { mn = vmin(mn, bmn[idx[i]]); mx = vmax(mx, bmx[idx[i]]); }
                    for (int k = 0; k < 3; k++) { in[ni].mn[k] = mn[k]; in[ni].mx[k] = mx[k]; }
                    if (count == 1) { in[ni].leftFirst = idx[first]; in[ni].count = 1; return; }
                    V3 e = mx - mn;
                    int ax = e.x > e.y ? (e.x > e.z ? 0 : 2) : (e.y > e.z ? 1 : 2);
                    std::sort(idx.begin() + first, idx.begin() + first + count, [&](int a, int b) { return bmn[a][ax] + bmx[a][ax] < bmn[b][ax] + bmx[b][ax]; });
                    int l = nn; nn += 2;
                    in[ni].leftFirst = l; in[ni].count = 0;
                    rec(l, first, count / 2);
                    rec(l + 1, first + count / 2, count - count / 2);
                };
                if (nVis > 0) rec(0, 0, nVis);
            }
            float exNow = fr < (int)expo.size() ? expo[fr] : lastExpo;
            GUni u;
            memset(&u, 0, sizeof u);
            set4(u.camP0, cA.p, 0); setQ(u.camQ0, cA.q);
            set4(u.camP1, cB.p, 0);
            {
                Quat qb = cB.q;
                float d = cA.q.x * qb.x + cA.q.y * qb.y + cA.q.z * qb.z + cA.q.w * qb.w;
                if (d < 0) { qb.x = -qb.x; qb.y = -qb.y; qb.z = -qb.z; qb.w = -qb.w; }
                setQ(u.camQ1, qb);
            }
            u.lens[0] = std::tan(cA.fov * PI / 180 / 4); u.lens[1] = cA.aperture; u.lens[2] = cA.focus; u.lens[3] = (float)H / W;
            u.misc[0] = (float)tauA; u.misc[1] = HAZE_SIGMA; u.misc[2] = 12.0f;
            u.misc[3] = 60.0f / std::max(exNow, 0.05f);   // directly seen lamps: no brighter than 60x display white
            set4(u.haze, HAZE_COLOR, 0);
            u.sky[0] = 1.0f; u.sky[1] = 1.0f;
            u.dims[0] = W; u.dims[1] = H;
            u.counts[0] = (int)LIGHTS.size(); u.counts[1] = nVis; u.counts[2] = maxBounce; u.counts[3] = fr;
            GTUni tu;
            memset(&tu, 0, sizeof tu);
            set4(tu.prevP, histCam.p, 0); setQ(tu.prevQ, histCam.q);
            set4(tu.curP, cM.p, 0); setQ(tu.curQ, cM.q);
            tu.prm[0] = useHist ? 1.0f : 0.0f; tu.prm[1] = alphaD; tu.prm[2] = alphaS; tu.prm[3] = 0.2f;
            tu.prm2[0] = clipG; tu.prm2[1] = clipG; tu.prm2[2] = dbg > 0 ? 1.0f : 0.0f; tu.prm2[3] = dbg;
            tu.prm3[0] = taaA;

            bool ok = false;
            for (int attempt = 0; attempt < 3 && !ok; attempt++) {
                memset([accD contents], 0, npx * 16); memset([accS contents], 0, npx * 16); memset([accE contents], 0, npx * 16);
                memset([accA[cur] contents], 0, npx * 16); memset([accN[cur] contents], 0, npx * 16);
                memset([accP[cur] contents], 0, npx * 16); memset([accX[cur] contents], 0, npx * 16);
                std::vector<id<MTLCommandBuffer>> cbs;
                for (int s = 0; s < spp; s += sppPass) {
                    u.dims[2] = s; u.dims[3] = std::min(sppPass, spp - s);
                    id<MTLCommandBuffer> cb = [queue commandBuffer];
                    id<MTLComputeCommandEncoder> enc = [cb computeCommandEncoder];
                    [enc setComputePipelineState:psoTrace];
                    [enc setBytes:&u length:sizeof u atIndex:0];
                    [enc setBuffer:bNodes offset:0 atIndex:1];
                    [enc setBuffer:bTris offset:0 atIndex:2];
                    [enc setBuffer:bTriS offset:0 atIndex:3];
                    [enc setBuffer:bMats offset:0 atIndex:4];
                    [enc setBuffer:bLights offset:0 atIndex:5];
                    [enc setBuffer:bInst offset:0 atIndex:6];
                    [enc setBuffer:accD offset:0 atIndex:7];
                    [enc setBuffer:accS offset:0 atIndex:8];
                    [enc setBuffer:accA[cur] offset:0 atIndex:9];
                    [enc setBuffer:accN[cur] offset:0 atIndex:10];
                    [enc setBuffer:accP[cur] offset:0 atIndex:11];
                    [enc setBuffer:bINodes offset:0 atIndex:12];
                    [enc setBuffer:bLgCells offset:0 atIndex:13];
                    [enc setBuffer:bLgIdx offset:0 atIndex:14];
                    [enc setBuffer:accX[cur] offset:0 atIndex:15];
                    [enc setBuffer:accE offset:0 atIndex:16];
                    [enc setTexture:atlas atIndex:0];
                    [enc dispatchThreads:MTLSizeMake(W, H, 1) threadsPerThreadgroup:MTLSizeMake(8, 8, 1)];
                    [enc endEncoding];
                    [cb commit];
                    cbs.push_back(cb);
                    if (cbs.size() >= 4) { [cbs.front() waitUntilCompleted]; if (cbs.front().status != MTLCommandBufferStatusCompleted) ok = false; cbs.erase(cbs.begin()); }
                }
                // resolve -> temporal accumulation -> spatial denoise -> compose
                id<MTLCommandBuffer> cb = [queue commandBuffer];
                id<MTLComputeCommandEncoder> enc = [cb computeCommandEncoder];
                MTLSize grid = MTLSizeMake(W, H, 1), tg = MTLSizeMake(8, 8, 1);
                [enc setComputePipelineState:psoResolve];
                [enc setBytes:&u length:sizeof u atIndex:0];
                [enc setBuffer:accD offset:0 atIndex:1]; [enc setBuffer:accS offset:0 atIndex:2]; [enc setBuffer:accX[cur] offset:0 atIndex:3];
                [enc setBuffer:rD offset:0 atIndex:4]; [enc setBuffer:rS offset:0 atIndex:5];
                [enc setBuffer:accE offset:0 atIndex:6]; [enc setBuffer:rE offset:0 atIndex:7];
                [enc dispatchThreads:grid threadsPerThreadgroup:tg];
                [enc setComputePipelineState:psoTemporal];
                [enc setBytes:&u length:sizeof u atIndex:0];
                [enc setBytes:&tu length:sizeof tu atIndex:1];
                [enc setBuffer:rD offset:0 atIndex:2]; [enc setBuffer:rS offset:0 atIndex:3]; [enc setBuffer:rE offset:0 atIndex:4];
                [enc setBuffer:accA[cur] offset:0 atIndex:5]; [enc setBuffer:accN[cur] offset:0 atIndex:6]; [enc setBuffer:accP[cur] offset:0 atIndex:7]; [enc setBuffer:accX[cur] offset:0 atIndex:8];
                [enc setBuffer:accA[prv] offset:0 atIndex:9]; [enc setBuffer:accN[prv] offset:0 atIndex:10]; [enc setBuffer:accP[prv] offset:0 atIndex:11]; [enc setBuffer:accX[prv] offset:0 atIndex:12];
                [enc setBuffer:hD[prv] offset:0 atIndex:13]; [enc setBuffer:hS[prv] offset:0 atIndex:14]; [enc setBuffer:hE[prv] offset:0 atIndex:15]; [enc setBuffer:hM[prv] offset:0 atIndex:16];
                [enc setBuffer:hD[cur] offset:0 atIndex:17]; [enc setBuffer:hS[cur] offset:0 atIndex:18]; [enc setBuffer:hE[cur] offset:0 atIndex:19]; [enc setBuffer:hM[cur] offset:0 atIndex:20];
                [enc setBuffer:bDyn[cur] offset:0 atIndex:21]; [enc setBuffer:bDyn[prv] offset:0 atIndex:22];
                [enc dispatchThreads:grid threadsPerThreadgroup:tg];
                id<MTLBuffer> di = hD[cur], dO = d0, si = hS[cur], so = s0;
                if (denoise) {
                    [enc setComputePipelineState:psoAtrous];
                    for (int it = 0; it < 5; it++) {
                        float prm[4] = {(float)(1 << it), sigD, sigS, 0};
                        [enc setBytes:&u length:sizeof u atIndex:0];
                        [enc setBuffer:di offset:0 atIndex:1]; [enc setBuffer:si offset:0 atIndex:2];
                        [enc setBuffer:accA[cur] offset:0 atIndex:3]; [enc setBuffer:accN[cur] offset:0 atIndex:4]; [enc setBuffer:accP[cur] offset:0 atIndex:5];
                        [enc setBuffer:dO offset:0 atIndex:6]; [enc setBuffer:so offset:0 atIndex:7];
                        [enc setBytes:prm length:16 atIndex:8];
                        [enc dispatchThreads:grid threadsPerThreadgroup:tg];
                        if (it == 0) { di = d0; dO = d1; si = s0; so = s1; }
                        else { std::swap(di, dO); std::swap(si, so); }
                    }
                }
                [enc setComputePipelineState:psoCompose];
                [enc setBytes:&u length:sizeof u atIndex:0];
                [enc setBuffer:di offset:0 atIndex:1]; [enc setBuffer:si offset:0 atIndex:2];
                [enc setBuffer:accA[cur] offset:0 atIndex:3]; [enc setBuffer:accP[cur] offset:0 atIndex:4];
                [enc setBuffer:hdr offset:0 atIndex:5];
                [enc setBuffer:hE[cur] offset:0 atIndex:6]; [enc setBuffer:accX[cur] offset:0 atIndex:7];
                [enc dispatchThreads:grid threadsPerThreadgroup:tg];
                [enc setComputePipelineState:psoTaa];
                [enc setBytes:&u length:sizeof u atIndex:0];
                [enc setBytes:&tu length:sizeof tu atIndex:1];
                [enc setBuffer:hdr offset:0 atIndex:2]; [enc setBuffer:fin[prv] offset:0 atIndex:3]; [enc setBuffer:fin[cur] offset:0 atIndex:4];
                [enc setBuffer:accN[cur] offset:0 atIndex:5]; [enc setBuffer:accP[cur] offset:0 atIndex:6]; [enc setBuffer:accX[cur] offset:0 atIndex:7];
                [enc setBuffer:bDyn[cur] offset:0 atIndex:8]; [enc setBuffer:bDyn[prv] offset:0 atIndex:9];
                [enc dispatchThreads:grid threadsPerThreadgroup:tg];
                [enc endEncoding];
                [cb commit];
                [cb waitUntilCompleted];
                ok = (cb.status == MTLCommandBufferStatusCompleted);
                for (auto c : cbs) if (c.status != MTLCommandBufferStatusCompleted) ok = false;
                // sample-count guard: every pixel must have received all samples
                if (ok) {
                    const float *h = (const float *)[fin[cur] contents];
                    for (size_t i = 0; i < npx; i += 97) if (h[i * 4 + 3] != (float)spp) { ok = false; break; }
                }
                if (!ok) fprintf(stderr, "frame %d: bad GPU pass (attempt %d), retrying\n", fr, attempt);
            }
            const float *h = (const float *)[fin[cur] contents];
            histFrame = ok ? fr : -1000000; histCam = cM; cur = prv;
            // centre-weighted log-average luminance for metering
            double ls = 0, lw = 0;
            for (int y = 0; y < H; y += 2)
                for (int x = 0; x < W; x += 2) {
                    const float *p = h + ((size_t)y * W + x) * 4;
                    float L = 0.2126f * p[0] + 0.7152f * p[1] + 0.0722f * p[2];
                    float nx = (x + 0.5f) / W * 2 - 1, ny = (y + 0.5f) / H * 2 - 1;
                    float w = std::exp(-1.5f * (nx * nx + ny * ny));
                    ls += std::log(std::min(std::max(L, 1e-4f), 30.0f) + 0.01f) * w; lw += w;
                }
            float meter = (float)std::exp(ls / lw);
            if (meterF && write) { fprintf(meterF, "%d %.6f\n", fr, meter); fflush(meterF); }
            PostParams pp;
            pp.exposure = (fr < (int)expo.size() ? expo[fr] : 0.18f / std::max(meter, 0.003f)) * std::pow(2.0f, evBias);
            lastExpo = pp.exposure;
            pp.seed = fr;
            pp.fade = haveEye ? 1.0f : fadeAt(tv);
            if (raw) { pp.bloom = 0; pp.grain = 0; pp.ca = 0; pp.vignette = 0; }
            if (!meterF && write) {
                postProcess(h, W, H, pp, rgb8);
                writePNG(path, rgb8.data(), W, H);
            }
            fprintf(stderr, "frame %5d%s t=%.3f tau=%.3f  %.2fs  meter=%.4f expo=%.3f\n", fr, write ? " " : "*", tv, tauA, nowSec() - tf, meter, pp.exposure);
        };

        for (int fr : frames) {
            char path[512];
            snprintf(path, sizeof path, "%s/f_%05d.png", outDir, fr);
            if (!meterF && frames.size() > 1) { struct stat st; if (stat(path, &st) == 0 && st.st_size > 1000) continue; }
            // frames lean on the ones before them: when starting cold, render (but do not save) a short run-up
            if (temporalOn && histFrame != fr - 1)
                for (int p = std::max(0, fr - preroll); p < fr; p++) renderFrame(p, false);
            renderFrame(fr, true);
        }
        if (meterF) fclose(meterF);
    }
    return 0;
}
