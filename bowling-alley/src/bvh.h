// bvh.h — binned-SAH bounding volume hierarchy built on the CPU, traversed by hand in the Metal kernel.
#pragma once
#include "mesh.h"

struct BNode {
    float mn[3];
    int32_t leftFirst; // inner: index of left child (right = left+1); leaf: first triangle
    float mx[3];
    int32_t count; // 0 for inner nodes
};

struct BvhBuilder {
    std::vector<TriC> &tris;
    std::vector<BNode> &nodes;
    std::vector<V3> cen, bmn, bmx;
    std::vector<uint32_t> idx;
    int base = 0;
    BvhBuilder(std::vector<TriC> &t, std::vector<BNode> &n) : tris(t), nodes(n) {}

    void bounds(int first, int count, V3 &mn, V3 &mx) {
        mn = V3(1e30f); mx = V3(-1e30f);
        for (int i = first; i < first + count; i++) { mn = vmin(mn, bmn[idx[i]]); mx = vmax(mx, bmx[idx[i]]); }
    }
    static float area(V3 mn, V3 mx) { V3 e = mx - mn; return e.x * e.y + e.y * e.z + e.z * e.x; }

    void split(int ni, int first, int count, int depth) {
        V3 mn, mx;
        bounds(first, count, mn, mx);
        BNode &nd = nodes[ni];
        for (int k = 0; k < 3; k++) { nd.mn[k] = mn[k]; nd.mx[k] = mx[k]; }
        if (count <= 4 || depth > 48) { nd.leftFirst = base + first; nd.count = count; return; }
        V3 cmn(1e30f), cmx(-1e30f);
        for (int i = first; i < first + count; i++) { cmn = vmin(cmn, cen[idx[i]]); cmx = vmax(cmx, cen[idx[i]]); }
        const int NB = 16;
        float best = 1e30f; int bax = -1, bsp = 0;
        for (int ax = 0; ax < 3; ax++) {
            float e = cmx[ax] - cmn[ax];
            if (e < 1e-7f) continue;
            V3 bn[NB], bx[NB]; int bc[NB];
            for (int b = 0; b < NB; b++) { bn[b] = V3(1e30f); bx[b] = V3(-1e30f); bc[b] = 0; }
            float sc = NB / e;
            for (int i = first; i < first + count; i++) {
                int b = std::min(NB - 1, (int)((cen[idx[i]][ax] - cmn[ax]) * sc));
                bc[b]++; bn[b] = vmin(bn[b], bmn[idx[i]]); bx[b] = vmax(bx[b], bmx[idx[i]]);
            }
            float la[NB], ra[NB]; int lc[NB], rc[NB];
            V3 an(1e30f), ax_(-1e30f); int c = 0;
            for (int b = 0; b < NB - 1; b++) { an = vmin(an, bn[b]); ax_ = vmax(ax_, bx[b]); c += bc[b]; la[b] = c ? area(an, ax_) : 0; lc[b] = c; }
            an = V3(1e30f); ax_ = V3(-1e30f); c = 0;
            for (int b = NB - 1; b > 0; b--) { an = vmin(an, bn[b]); ax_ = vmax(ax_, bx[b]); c += bc[b]; ra[b - 1] = c ? area(an, ax_) : 0; rc[b - 1] = c; }
            for (int b = 0; b < NB - 1; b++) {
                if (!lc[b] || !rc[b]) continue;
                float cost = la[b] * lc[b] + ra[b] * rc[b];
                if (cost < best) { best = cost; bax = ax; bsp = b; }
            }
        }
        int mid;
        if (bax < 0) {
            if (count <= 12) { nd.leftFirst = base + first; nd.count = count; return; }
            mid = first + count / 2;
        } else {
            float leafCost = area(mn, mx) * count;
            if (best >= leafCost && count <= 8) { nd.leftFirst = base + first; nd.count = count; return; }
            float sc = NB / (cmx[bax] - cmn[bax]);
            auto it = std::partition(idx.begin() + first, idx.begin() + first + count, [&](uint32_t t) {
                return std::min(NB - 1, (int)((cen[t][bax] - cmn[bax]) * sc)) <= bsp;
            });
            mid = int(it - idx.begin());
            if (mid == first || mid == first + count) mid = first + count / 2;
        }
        int l = (int)nodes.size();
        nodes.push_back(BNode()); nodes.push_back(BNode());
        nodes[ni].leftFirst = l; nodes[ni].count = 0;
        split(l, first, mid - first, depth + 1);
        split(l + 1, mid, first + count - mid, depth + 1);
    }

    // builds a BVH over tris[first..first+count) (reordering that range); returns root node index
    int build(int first, int count) {
        base = first;
        cen.resize(count); bmn.resize(count); bmx.resize(count); idx.resize(count);
        for (int i = 0; i < count; i++) {
            const TriC &t = tris[first + i];
            bmn[i] = vmin(t.p[0], vmin(t.p[1], t.p[2]));
            bmx[i] = vmax(t.p[0], vmax(t.p[1], t.p[2]));
            cen[i] = (bmn[i] + bmx[i]) * 0.5f;
            idx[i] = i;
        }
        int root = (int)nodes.size();
        nodes.push_back(BNode());
        if (count == 0) {
            BNode &n = nodes[root];
            n.mn[0] = n.mn[1] = n.mn[2] = 1e30f; n.mx[0] = n.mx[1] = n.mx[2] = -1e30f; n.leftFirst = first; n.count = 0;
            // an empty inner node would loop; make it a harmless leaf with a degenerate box
            n.count = 0; n.leftFirst = root; // never traversed: box test fails
            return root;
        }
        split(root, 0, count, 0);
        std::vector<TriC> tmp(count);
        for (int i = 0; i < count; i++) tmp[i] = tris[first + idx[i]];
        std::copy(tmp.begin(), tmp.end(), tris.begin() + first);
        return root;
    }
};
