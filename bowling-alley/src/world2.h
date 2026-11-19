// world2.h — signage painted into the atlas, and the furnishings of the lane hall:
// masking-unit art with pin indicators, ball returns, settees, score tables, overhead score screens, back-of-house props.
#pragma once
#include "atlas.h"
#include "machine.h"

struct Signs {
    Region mask[NLANES], score[3], sheet, menu, shoeRental, snackBar, noStreet, league, calendar, clockFace, exitSign, nameplate, danger,
        juke, jukeTitles, pennant[3], posterA, posterB, cigs, openSign, laneNum[NLANES], hours, gum, phone, trophyPlate, cola, mural, marquee, doorPush, rates;
};
static Signs SG;
static int M_DECAL = -1, M_DECAL_GLOSS = -1;
static std::vector<int> EMIT_DECALS;

// a textured rectangle: centre, right vector, up vector, width, height
static void decal(V3 c, V3 right, V3 up, float w, float h, const Region &r, int m = -1) {
    PushMat pm(m < 0 ? M_DECAL : m);
    float u0, v0, u1, v1;
    regionUV(r, u0, v0, u1, v1);
    setUV(u0, v0, u1, v1);
    panel(c, right, up, w, h);
    setUV();
}
static int emitDecalMat(V3 e, int mode = 2, int anim = 0) { return mat(V3(1, 1, 1), 0.4f).kind(K_ATLAS).emit(e, mode, anim); }

static void paintSigns() {
    Col cream = hexc(0xEDE4C8), turq = hexc(0x2FA39B), orange = hexc(0xE0641C), gold = hexc(0xD9A21B), red = hexc(0xB5201A), ink = hexc(0x1B1A18),
        white = hexc(0xF6F2E6), navy = hexc(0x14284A), brown = hexc(0x4A2C18), pink = hexc(0xE86A8A);
    // --- masking unit panels
    for (int i = 0; i < NLANES; i++) {
        Region r = SG.mask[i] = atlasAlloc(768, 704);
        aFill(r, cream);
        aRect(r, 0, 250, 768, 420, turq);
        aRect(r, 0, 244, 768, 250, gold); aRect(r, 0, 420, 768, 426, gold);
        // diamond harlequin band at the bottom
        for (int k = -1; k < 14; k++) {
            float x = k * 64.0f + (i % 2) * 32.0f;
            aTri(r, V2(x, 640), V2(x + 32, 600), V2(x + 64, 640), (k & 1) ? orange : turq);
            aTri(r, V2(x, 640), V2(x + 32, 680), V2(x + 64, 640), (k & 1) ? orange : turq);
        }
        aStarburst(r, 120, 150, 105, 16, gold, i * 0.7f);
        aStarburst(r, 660, 520, 60, 12, orange, i * 1.3f, 0.07f);
        aStarburst(r, 610, 90, 38, 12, turq, i * 0.4f, 0.07f);
        aBoomerang(r, 250, 530, 50, 0.4f + i, orange);
        aBoomerang(r, 480, 110, 40, 2.3f + i, turq);
        // pin indicator housing (dark inverted triangle) in the centre band
        aTri(r, V2(384 - 150, 262), V2(384 + 150, 262), V2(384, 410), ink);
        // lane number disc
        aCircle(r, 640, 335, 62, cream); aCircle(r, 640, 335, 62, orange, 1, 8);
        std::string num = std::to_string(i + 1);
        aTextC(r, num, 640, 335 - 38, 76, 13, ink);
        aCircle(r, 128, 335, 40, cream); aCircle(r, 128, 335, 40, gold, 1, 6);
        aTextC(r, num, 128, 335 - 22, 44, 8, ink);
    }
    // --- overhead score screens (projected score sheets), dark background, light lines
    static const char *names[6] = {"BETTY", "FRANK", "DOT", "HANK", "MARGE", "LOU"};
    for (int v = 0; v < 3; v++) {
        Region r = SG.score[v] = atlasAlloc(640, 300);
        aFill(r, hexc(0xE9E3C4));
        Rng rg(40 + v);
        for (int row = 0; row < 4; row++) {
            float y = 30 + row * 66;
            aLine(r, 10, y, 630, y, 2, ink);
            aLine(r, 10, y + 66, 630, y + 66, 2, ink);
            aText(r, names[(v * 2 + row) % 6], 16, y + 22, 22, 3.2f, navy, 0.15f);
            for (int f = 0; f <= 10; f++) {
                float x = 130 + f * 50;
                aLine(r, x, y, x, y + 66, 2, ink);
                if (f < 10) { aLine(r, x + 25, y, x + 25, y + 24, 1.5f, ink); aLine(r, x + 25, y + 24, x + 50, y + 24, 1.5f, ink); }
            }
            int tot = 0;
            int upto = 3 + rg.i(6);
            for (int f = 0; f < upto; f++) {
                float x = 130 + f * 50;
                int a = rg.i(10), b = rg.i(11 - a);
                if (rg.u() < 0.22f) { aText(r, "X", x + 31, y + 5, 15, 2.6f, red); tot += 20 + rg.i(8); }
                else {
                    aText(r, std::to_string(a), x + 7, y + 5, 15, 2.4f, navy, 0.2f);
                    if (a + b == 10) aText(r, "/", x + 31, y + 5, 15, 2.4f, navy); else aText(r, std::to_string(b), x + 31, y + 5, 15, 2.4f, navy, 0.2f);
                    tot += a + b + (a + b == 10 ? 6 : 0);
                }
                aTextC(r, std::to_string(tot), x + 25, y + 34, 22, 3.0f, navy, 0.15f);
            }
        }
        aText(r, "LANE", 16, 6, 14, 2.2f, ink);
    }
    {   // paper score sheet on the scorer's table
        Region r = SG.sheet = atlasAlloc(512, 256);
        aFill(r, hexc(0xF1ECD6));
        for (int row = 0; row < 5; row++) {
            float y = 30 + row * 42;
            aLine(r, 6, y, 506, y, 1.5f, hexc(0x3A6A8A));
            for (int f = 0; f <= 10; f++) aLine(r, 90 + f * 41.0f, y, 90 + f * 41.0f, y + 42, 1.2f, hexc(0x3A6A8A));
            if (row < 3) for (int f = 0; f < 4 + row; f++) aText(r, std::to_string((f * 7 + row * 3) % 10), 100 + f * 41.0f, y + 12, 18, 2.2f, hexc(0x2A2A30), 0.25f);
        }
        aText(r, "STARLITE LANES", 150, 6, 16, 2.4f, red);
    }
    {   // snack bar menu board (back-lit)
        Region r = SG.menu = atlasAlloc(1024, 448);
        aFill(r, hexc(0xF4EBCB));
        aRect(r, 0, 0, 1024, 70, red);
        aTextC(r, "SNACK BAR", 512, 12, 46, 8, white);
        static const char *items[10] = {"HAMBURGER", "CHEESEBURGER", "HOT DOG", "FRENCH FRIES", "GRILLED CHEESE", "COFFEE", "MALTS & SHAKES", "COLA", "PIE", "ICE CREAM"};
        static const char *price[10] = {".35", ".40", ".25", ".15", ".30", ".10", ".30", ".10", ".20", ".15"};
        for (int i = 0; i < 10; i++) {
            float x = 40 + (i / 5) * 500.0f, y = 100 + (i % 5) * 66.0f;
            aText(r, items[i], x, y, 34, 5, ink);
            aText(r, price[i], x + 380, y, 34, 5, red);
            for (float dx = textWidth(items[i]) * 34 / 6 + 14; dx < 370; dx += 14) aCircle(r, x + dx, y + 31, 2, ink);
        }
        aStarburst(r, 975, 400, 34, 12, orange);
    }
    auto simpleSign = [&](Region &rg, int w, int h, Col bg, Col fg, std::vector<std::string> lines, float hgt, float wt, Col border, float slant = 0) {
        rg = atlasAlloc(w, h);
        aFill(rg, bg);
        aRect(rg, 4, 4, w - 4, 10, border); aRect(rg, 4, h - 10, w - 4, h - 4, border); aRect(rg, 4, 4, 10, h - 4, border); aRect(rg, w - 10, 4, w - 4, h - 4, border);
        float total = lines.size() * hgt * 1.45f;
        for (size_t i = 0; i < lines.size(); i++) aTextC(rg, lines[i], w * 0.5f, (h - total) * 0.5f + i * hgt * 1.45f + hgt * 0.2f, hgt, wt, fg, slant);
    };
    simpleSign(SG.shoeRental, 640, 256, turq, white, {"SHOE RENTAL", "25c"}, 62, 10, gold);
    simpleSign(SG.snackBar, 512, 128, red, white, {"SNACK BAR"}, 60, 10, gold);
    simpleSign(SG.noStreet, 512, 200, cream, ink, {"NO STREET SHOES", "ON THE LANES"}, 38, 6, red);
    simpleSign(SG.league, 512, 384, hexc(0x2E5A3A), white, {"LEAGUE NIGHT", "TUES & THURS", "7:30", "SIGN UP AT DESK"}, 40, 5.5f, gold, 0.1f);
    simpleSign(SG.exitSign, 256, 112, hexc(0x101010), hexc(0xFF3020), {"EXIT"}, 66, 12, hexc(0x101010));
    simpleSign(SG.nameplate, 384, 160, hexc(0x1E1E1E), hexc(0xD8C07A), {"KINGPIN K-2", "AUTOMATIC PINSETTER"}, 28, 4, hexc(0xD8C07A));
    simpleSign(SG.danger, 384, 160, hexc(0xE0B000), ink, {"DANGER", "KEEP HANDS CLEAR"}, 36, 6, ink);
    simpleSign(SG.openSign, 384, 128, hexc(0x101010), hexc(0xFF5030), {"OPEN"}, 80, 14, hexc(0x101010));
    simpleSign(SG.hours, 384, 384, cream, navy, {"OPEN BOWLING", "DAILY 10 - 2", "35c A LINE", "AIR CONDITIONED"}, 34, 5, turq);
    simpleSign(SG.cigs, 256, 384, hexc(0x7A1010), white, {"CIGARETTES", "35c"}, 30, 5, gold);
    simpleSign(SG.gum, 128, 128, red, white, {"1c"}, 60, 10, white);
    simpleSign(SG.phone, 256, 128, navy, white, {"TELEPHONE"}, 30, 5, white);
    simpleSign(SG.trophyPlate, 256, 64, hexc(0xC9A24A), ink, {"CHAMPIONS 1963"}, 22, 3, ink);
    simpleSign(SG.cola, 384, 384, red, white, {"DRINK", "COLA", "ICE COLD"}, 60, 10, white, 0.2f);
    simpleSign(SG.doorPush, 128, 64, hexc(0x202020), white, {"PUSH"}, 26, 4.5f, white);
    simpleSign(SG.rates, 512, 384, hexc(0x1D3B4F), white, {"RATES", "DAYS 30c", "NIGHTS 40c", "SHOES 25c"}, 44, 6.5f, gold);
    for (int i = 0; i < NLANES; i++) {
        Region r = SG.laneNum[i] = atlasAlloc(128, 128);
        aFill(r, hexc(0x15130F));
        aCircle(r, 64, 64, 56, hexc(0xF2E6C8));
        aTextC(r, std::to_string(i + 1), 64, 30, 68, 11, hexc(0x15130F));
    }
    {   // wall calendar
        Region r = SG.calendar = atlasAlloc(320, 448);
        aFill(r, white);
        aRect(r, 0, 0, 320, 200, hexc(0x2A6E8C));
        aStarburst(r, 230, 80, 55, 14, gold);
        aBoomerang(r, 80, 120, 50, 0.5f, orange);
        aTextC(r, "JUNE 1964", 160, 215, 30, 4.5f, red);
        for (int d = 1; d <= 30; d++) {
            int c = (d) % 7, rr = (d) / 7;
            aTextC(r, std::to_string(d), 34 + c * 42.0f, 265 + rr * 36.0f, 18, 2.5f, c == 0 ? red : ink);
        }
        aCircle(r, 34 + 6 * 42.0f, 275 + 1 * 36.0f, 16, red, 1, 2.5f);
    }
    {   // clock face
        Region r = SG.clockFace = atlasAlloc(256, 256);
        aFill(r, hexc(0x1A1A1A));
        aCircle(r, 128, 128, 120, hexc(0xF4EFDF));
        for (int h = 0; h < 12; h++) {
            float a = h * PI / 6;
            aLine(r, 128 + std::sin(a) * 96, 128 - std::cos(a) * 96, 128 + std::sin(a) * 112, 128 - std::cos(a) * 112, h % 3 == 0 ? 9 : 4, ink);
        }
        aTextC(r, "12", 128, 34, 26, 4, ink); aTextC(r, "6", 128, 196, 26, 4, ink); aTextC(r, "3", 208, 115, 26, 4, ink); aTextC(r, "9", 48, 115, 26, 4, ink);
    }
    {   // jukebox front glow panel and title strips
        Region r = SG.juke = atlasAlloc(256, 256);
        for (int y = 0; y < 256; y++)
            for (int x = 0; x < 256; x++) {
                float t = x / 255.0f, s = y / 255.0f;
                Col c = {0.5f + 0.5f * std::sin(t * 6.3f + 0.5f), 0.45f + 0.45f * std::sin(t * 6.3f + 2.6f + s), 0.5f + 0.5f * std::sin(t * 6.3f + 4.7f)};
                aBlend(r, x, y, c, 1);
            }
        for (int i = 0; i < 9; i++) aLine(r, 14 + i * 28.5f, 10, 14 + i * 28.5f, 246, 5, hexc(0xF0E8D0), 0.55f);
        Region q = SG.jukeTitles = atlasAlloc(384, 192);
        aFill(q, hexc(0xF5F0DC));
        static const char *songs[8] = {"SURF CITY NIGHTS", "MY BABY BOWLS", "TWISTIN' PINS", "LONELY LANE", "TEN IN A ROW", "GUTTER BALL BLUES", "STARLITE STOMP", "SPARE ME"};
        for (int i = 0; i < 16; i++) {
            float x = 8 + (i / 8) * 190.0f, y = 6 + (i % 8) * 23.0f;
            aRect(q, x, y, x + 180, y + 19, (i & 1) ? hexc(0xF7D9D9) : hexc(0xD9E8F7));
            aText(q, std::string(1, (char)('A' + i / 8)) + std::to_string(i % 8 + 1), x + 3, y + 4, 11, 1.8f, red);
            aText(q, songs[(i * 3) % 8], x + 34, y + 5, 9, 1.4f, ink);
        }
    }
    for (int i = 0; i < 3; i++) {
        Region r = SG.pennant[i] = atlasAlloc(384, 160);
        Col bg = i == 0 ? navy : (i == 1 ? red : hexc(0x1E6E3A));
        aFill(r, bg);
        aRect(r, 0, 0, 26, 160, gold);
        static const char *txt[3] = {"STARLITE", "CHAMPS '63", "STRIKERS"};
        aText(r, txt[i], 44, 56, 44 - i * 4, 7, white, 0.15f);
    }
    {   // travel-style poster
        Region r = SG.posterA = atlasAlloc(384, 544);
        aFill(r, hexc(0xF2C230));
        aRect(r, 0, 380, 384, 544, turq);
        aStarburst(r, 192, 190, 150, 20, white);
        aCircle(r, 192, 190, 70, ink);
        aCircle(r, 170, 165, 9, hexc(0xF2C230)); aCircle(r, 200, 160, 9, hexc(0xF2C230)); aCircle(r, 186, 192, 11, hexc(0xF2C230));
        aTextC(r, "BOWL", 192, 400, 70, 13, white);
        aTextC(r, "FOR FUN & HEALTH", 192, 495, 22, 3.6f, ink);
        Region q = SG.posterB = atlasAlloc(384, 544);
        aFill(q, hexc(0xE9DFC3));
        aRect(q, 20, 20, 364, 524, red);
        aTextC(q, "FAMILY", 192, 60, 58, 10, white);
        aTextC(q, "NIGHT", 192, 150, 58, 10, white);
        aStarburst(q, 192, 320, 80, 14, gold);
        aTextC(q, "EVERY FRIDAY", 192, 440, 30, 5, white);
    }
    {   // long wall mural: atomic starbursts, boomerangs, pins
        Region r = SG.mural = atlasAlloc(2048, 384);
        aFill(r, hexc(0xD9CFB4));
        Rng rg(9);
        Col cols[4] = {turq, orange, gold, hexc(0x7A2E1E)};
        for (int i = 0; i < 26; i++) {
            float x = 60 + i * 76.0f + rg.r(-20, 20), y = rg.r(60, 320);
            int k = rg.i(3);
            Col c = cols[rg.i(4)];
            if (k == 0) aStarburst(r, x, y, rg.r(30, 70), 12 + 4 * rg.i(2), c, rg.u() * 6);
            else if (k == 1) aBoomerang(r, x, y, rg.r(40, 70), rg.u() * 6, c);
            else { aCircle(r, x, y, rg.r(14, 30), c, 1, 5); aCircle(r, x, y, 5, c); }
        }
        for (int i = 0; i < 4; i++) aLine(r, 0, 60 + i * 90.0f + 30 * std::sin(i * 2.0f), 2048, 100 + i * 80.0f, 2.5f, hexc(0x7A2E1E), 0.5f);
    }
    {   // roadside marquee panel
        Region r = SG.marquee = atlasAlloc(768, 320);
        aFill(r, hexc(0xF4F0E0));
        for (int i = 0; i < 4; i++) aLine(r, 10, 20 + i * 76.0f, 758, 20 + i * 76.0f, 2, hexc(0xB8B4A4));
        aTextC(r, "OPEN BOWLING NITELY", 384, 34, 44, 7.5f, ink);
        aTextC(r, "COCKTAILS  SNACK BAR", 384, 110, 44, 7.5f, red);
        aTextC(r, "24 AUTOMATIC LANES", 384, 186, 44, 7.5f, ink);
        aTextC(r, "LEAGUES FORMING NOW", 384, 262, 44, 7.5f, red);
    }
}

// neon tubes bent along the stroke font; adds a diffuse-only stand-in light for the glow it throws
static float neonText(const std::string &s, V3 origin, V3 right, V3 up, float h, V3 color, float tubeR = 0.011f, float power = 1.0f, int anim = 0, bool light = true) {
    std::vector<std::vector<V2>> st;
    float wU = textStrokes(s, st);
    float sc = h / 6.0f;
    V3 nrm = norm(cross(right, up));
    int m = mat(V3(0.9f, 0.9f, 0.9f), 0.3f).emit(color, 3, anim);
    float total = 0;
    {
        PushMat pm(m);
        G.flags |= F_NOSHADOW;
        for (auto &pl : st) {
            std::vector<V3> pts;
            for (auto &p : pl) pts.push_back(origin + right * (p.x * sc) + up * (p.y * sc) + nrm * 0.035f);
            if (pts.size() == 1) pts.push_back(pts[0] + up * 0.01f);
            for (size_t i = 0; i + 1 < pts.size(); i++) total += len(pts[i + 1] - pts[i]);
            tube(pts, tubeR, 8, true);
        }
    }
    {   // standoffs
        PushMat pm(MT.black);
        for (auto &pl : st) { V3 p = origin + right * (pl[0].x * sc) + up * (pl[0].y * sc); cyl(p, p + nrm * 0.035f, 0.006f, 0.006f, 6, false, false); }
    }
    float w = wU * sc;
    if (light) {
        LightDef L;
        L.type = 2;
        V3 c = origin + right * (w * 0.5f) + up * (h * 0.5f) + nrm * 0.06f;
        L.pos = G.xf.pt(c); L.u = G.xf.vec(right * (w * 0.5f)); L.v = G.xf.vec(up * (h * 0.5f));
        float areaTube = total * 2 * tubeR * 2.2f, areaRect = std::max(w * h, 1e-4f);
        L.emit = color * (areaTube / areaRect) * power;
        L.anim = anim;
        LIGHTS.push_back(L);
    }
    return w;
}
// generic neon outline from a polyline (closed or open)
static void neonPath(const std::vector<V3> &pts, V3 color, float tubeR = 0.011f, int anim = 0) {
    int m = mat(V3(0.9f, 0.9f, 0.9f), 0.3f).emit(color, 3, anim);
    PushMat pm(m);
    G.flags |= F_NOSHADOW;
    tube(pts, tubeR, 8, true);
}

static int A_PINLAMPS = 0, A_STRIKE = 0;

static void buildMaskingArt() {
    A_PINLAMPS = addAnim([](double t) { double d = t - T_HAND; return (d < 0.45 || d > 4.95) ? 1.0f : 0.02f; });
    A_STRIKE = addAnim([](double t) { double d = t - T_HAND; return (d > 0.9 && d < 4.9) ? (std::fmod(d, 0.5) < 0.3 ? 1.0f : 0.05f) : 0.0f; });
    int lampOn = mat(V3(0.9f, 0.8f, 0.5f), 0.3f).emit(V3(26, 19, 8), 3);
    int lampHero = mat(V3(0.9f, 0.8f, 0.5f), 0.3f).emit(V3(26, 19, 8), 3, A_PINLAMPS);
    int lampStrike = mat(V3(0.9f, 0.2f, 0.1f), 0.3f).emit(V3(40, 5, 2), 3, A_STRIKE);
    int lampOff = mat(V3(0.25f, 0.06f, 0.04f), 0.3f);
    for (int i = 0; i < NLANES; i++) {
        float cx = laneX(i);
        float pw = PITCH - 0.02f, ph = 2.2f - MASK_Y;
        V3 c(cx, MASK_Y + ph * 0.5f, MASK_Z - 0.002f);
        decal(c, V3(-1, 0, 0), V3(0, 1, 0), pw, ph, SG.mask[i]);   // faces -z (toward the bowlers); right = -x
        // pin indicator lamps: bowler's view, head pin at the bottom
        auto px2w = [&](float px, float py) { return V3(cx + (0.5f - px / 768.0f) * pw, MASK_Y + ph * (1 - py / 704.0f), MASK_Z - 0.004f); };
        for (int k = 0; k < 10; k++) {
            V3 s = pinSpot(k);
            int row = (int)std::lround((s.z - PINZ) / PIN_ROW);
            float px = 384 - s.x / PIN_SP * 62.0f, py = 388 - row * 34.0f - 10;
            PushMat pm(i == HERO ? lampHero : lampOn);
            V3 p = px2w(px, py);
            cyl(p, p + V3(0, 0, -0.012f), 0.021f, 0.016f, 12, false, true);
        }
        {   // strike lamp ("X") to the left of the triangle
            PushMat pm(i == HERO ? lampStrike : lampOff);
            V3 p = px2w(384, 232);
            p.y = MASK_Y + ph * (1 - 222.0f / 704.0f);
            cyl(p + V3(0.30f, -0.1f, 0), p + V3(0.30f, -0.1f, -0.012f), 0.03f, 0.024f, 14, false, true);
        }
        // chrome trim around the panel
        PushMat pm(MT.chromeTrim);
        rbox(V3(cx - PITCH * 0.5f, MASK_Y - 0.012f, MASK_Z - 0.015f), V3(cx + PITCH * 0.5f, MASK_Y + 0.012f, MASK_Z + 0.02f), 0.004f);
        rbox(V3(cx - PITCH * 0.5f - 0.008f, MASK_Y, MASK_Z - 0.012f), V3(cx - PITCH * 0.5f + 0.008f, 2.2f, MASK_Z + 0.01f), 0.003f);
    }
    {
        PushMat pm(MT.chromeTrim);
        rbox(V3(-HALL_X, 2.19f, MASK_Z - 0.015f), V3(HALL_X, 2.215f, MASK_Z + 0.02f), 0.004f);
    }
}

// ---------------------------------------------------------------- ball return + rack between each lane pair
static int P_HOOD = -1;
static void buildBallReturns() {
    for (int p = 0; p < NLANES / 2; p++) {
        float x = (laneX(2 * p) + laneX(2 * p + 1)) * 0.5f;
        Push base(xfT(x, 0, 0));
        {   // streamlined hood where the ball surfaces
            PushMat pm(MT.turq);
            surf(18, 14, [&](float u, float v) {
                float a = u * PI;                        // across: 0..pi
                float t = v;                             // along z (-3.0 front ... -3.9 back)
                float w = 0.19f * (0.75f + 0.25f * std::sin(t * PI));
                float hgt = 0.50f * std::pow(std::sin(t * PI * 0.92f + 0.12f), 0.6f) + 0.02f;
                return V3(-std::cos(a) * w, std::sin(a) * hgt, lerp(-2.95f, -3.95f, t));
            });
            PushMat pm2(MT.chromeTrim);
            rbox(V3(-0.2f, 0.0f, -3.97f), V3(0.2f, 0.035f, -2.93f), 0.01f);
            torus(V3(0, 0.26f, -3.955f), V3(0, 0, 1), 0.125f, 0.012f, 24, 8);
            PushMat pm3(MT.black);
            cyl(V3(0, 0.26f, -3.94f), V3(0, 0.26f, -3.95f), 0.12f, 0.12f, 20);
        }
        {   // rails carrying the balls back to the rack
            PushMat pm(MT.chromeTrim);
            for (int s = -1; s <= 1; s += 2) {
                tube(smoothPath({V3(s * 0.07f, 0.20f, -3.95f), V3(s * 0.07f, 0.40f, -4.25f), V3(s * 0.07f, 0.62f, -4.6f), V3(s * 0.07f, 0.62f, -5.55f)}, 8), 0.012f, 8);
                cyl(V3(s * 0.07f, 0, -4.55f), V3(s * 0.07f, 0.62f, -4.6f), 0.014f, 0.014f, 8);
                cyl(V3(s * 0.07f, 0, -5.5f), V3(s * 0.07f, 0.62f, -5.5f), 0.014f, 0.014f, 8);
            }
            cyl(V3(-0.07f, 0.62f, -5.56f), V3(0.07f, 0.62f, -5.56f), 0.03f, 0.03f, 12);
            PushMat pm2(MT.walnut, (float)p);
            rbox(V3(-0.16f, 0.0f, -5.62f), V3(0.16f, 0.05f, -4.5f), 0.01f);
        }
        // house balls waiting on the rack
        int nb = 3 + (p * 7) % 3;
        for (int b = 0; b < nb; b++) {
            float z = -5.43f + b * 0.225f;
            bake(P_BALL, xfT(0, 0.62f + BALL_R * 0.86f, z) * xfRX(b * 1.3f + p) * xfRY(b * 2.1f), (float)((b * 3 + p * 2) % 7));
        }
        {   // lane-number plates on the hood sides
            decal(V3(-0.165f, 0.2f, -3.45f), V3(0, 0, -1), V3(-0.35f, 1, 0), 0.11f, 0.11f, SG.laneNum[2 * p]);
            decal(V3(0.165f, 0.2f, -3.45f), V3(0, 0, 1), V3(0.35f, 1, 0), 0.11f, 0.11f, SG.laneNum[2 * p + 1]);
        }
    }
}

// ---------------------------------------------------------------- settee area: benches, scorer's tables, overhead score screens
static void shellSeat(float w) {
    // fibreglass bench: seat and back as one curved shell, on chrome legs
    surf(10, 12, [&](float u, float v) {
        float x = (u - 0.5f) * w;
        // profile: back top (v=0) down to seat front (v=1)
        float t = v;
        float y, z;
        if (t < 0.5f) { float s = t / 0.5f; y = lerp(0.84f, 0.44f, s * s * (3 - 2 * s) * 0.9f + s * 0.1f); z = lerp(-0.30f, -0.19f, s) - 0.05f * std::sin(s * PI) * 0; }
        else { float s = (t - 0.5f) / 0.5f; y = 0.44f - 0.03f * std::sin(s * PI) + 0.02f * s; z = lerp(-0.19f, 0.25f, s); }
        float curl = 0.03f * std::pow(std::fabs(u - 0.5f) * 2, 3.0f);
        return V3(x, y + curl, z - (t < 0.5f ? curl * 0.6f : 0.0f));
    });
}
static void bench(float w, int matSeat) {
    { PushMat pm(matSeat); shellSeat(w); }
    PushMat pm(MT.chrome);
    for (int s = -1; s <= 1; s += 2) {
        float x = s * (w * 0.5f - 0.18f);
        tube(smoothPath({V3(x, 0.0f, -0.28f), V3(x, 0.40f, -0.20f), V3(x, 0.42f, 0.16f), V3(x, 0.0f, 0.24f)}, 6), 0.012f, 8);
    }
}
static void scorerTable(int variant) {
    {   // pedestal console with a slanted, lit writing surface
        PushMat pm(MT.turq);
        rbox(V3(-0.42f, 0.0f, -0.26f), V3(0.42f, 0.70f, 0.26f), 0.03f);
    }
    {
        PushMat pm(MT.formica);
        // slanted top
        quad(V3(-0.46f, 0.70f, 0.30f), V3(0.46f, 0.70f, 0.30f), V3(0.46f, 0.86f, -0.30f), V3(-0.46f, 0.86f, -0.30f));
        quad(V3(-0.46f, 0.70f, 0.30f), V3(-0.46f, 0.70f, -0.30f), V3(-0.46f, 0.86f, -0.30f), V3(-0.46f, 0.70f, 0.30f));
        quad(V3(0.46f, 0.70f, 0.30f), V3(0.46f, 0.86f, -0.30f), V3(0.46f, 0.70f, -0.30f), V3(0.46f, 0.70f, 0.30f));
        quad(V3(-0.46f, 0.70f, -0.30f), V3(0.46f, 0.70f, -0.30f), V3(0.46f, 0.86f, -0.30f), V3(-0.46f, 0.86f, -0.30f));
    }
    V3 up = norm(V3(0, 0.16f, -0.60f)), nrm = norm(cross(V3(1, 0, 0), up));
    if (nrm.y < 0) nrm = -nrm;
    V3 c = V3(0, 0.78f, 0.0f) + nrm * 0.003f;
    static int sheetGlow = -1;
    if (sheetGlow < 0) sheetGlow = emitDecalMat(V3(0.38f, 0.35f, 0.27f), 2);
    decal(c, V3(1, 0, 0), up, 0.62f, 0.31f, SG.sheet, sheetGlow);
    {   // pencil and ashtray
        PushMat pm(MT.mustard);
        V3 a = c + V3(0.2f, 0, 0) + up * -0.2f + nrm * 0.006f;
        cyl(a, a + V3(0.14f, 0, 0.03f), 0.0035f, 0.0035f, 6);
        PushMat pm2(MT.glass);
        Push p(xfT(-0.36f, 0.86f, -0.22f));
        lathe({V2(0, 0), V2(0.05f, 0), V2(0.06f, 0.025f), V2(0.05f, 0.025f), V2(0.045f, 0.008f), V2(0, 0.008f)}, 14);
    }
    {   // projector head on a gooseneck: throws the sheet onto the overhead screen
        PushMat pm(MT.chrome);
        tube(smoothPath({V3(0.36f, 0.84f, -0.24f), V3(0.40f, 1.15f, -0.26f), V3(0.30f, 1.28f, -0.20f)}, 6), 0.012f, 8);
        PushMat pm2(MT.steelDark);
        rbox(V3(0.20f, 1.24f, -0.26f), V3(0.36f, 1.34f, -0.12f), 0.015f);
    }
    (void)variant;
}

static void buildSettees() {
    static int screenGlow = -1;
    if (screenGlow < 0) screenGlow = emitDecalMat(V3(0.42f, 0.40f, 0.30f), 2);
    for (int p = 0; p < NLANES / 2; p++) {
        float x = (laneX(2 * p) + laneX(2 * p + 1)) * 0.5f;
        // scorer's table faces the lanes
        { Push b(xfT(x, 0, -6.15f) * xfRY(PI)); scorerTable(p); }
        // U of benches behind it
        int seatM = (p & 1) ? MT.orange : MT.turq;
        { Push b(xfT(x, 0, -7.75f)); bench(2.5f, seatM); }              // back bench, facing the lanes
        { Push b(xfT(x - 1.45f, 0, -6.9f) * xfRY(PI / 2)); bench(1.5f, seatM); }
        { Push b(xfT(x + 1.45f, 0, -6.9f) * xfRY(-PI / 2)); bench(1.5f, seatM); }
        // scorer's swivel chair
        {
            Push b(xfT(x + 0.05f, 0, -6.78f) * xfRY(0.3f));
            { PushMat pm((p & 1) ? MT.turq : MT.orange); Push s(xfS(1.0f)); shellSeat(0.48f); }
            PushMat pm(MT.chrome);
            cyl(V3(0, 0, 0), V3(0, 0.42f, 0), 0.02f, 0.02f, 10);
            lathe({V2(0, 0), V2(0.22f, 0), V2(0.2f, 0.02f), V2(0.03f, 0.05f), V2(0, 0.05f)}, 20);
        }
        // overhead score screens for the two lanes, angled down toward the seats
        for (int s = 0; s < 2; s++) {
            float sx = x + (s ? 0.86f : -0.86f);
            V3 c(sx, 2.38f, -4.55f);
            V3 up = norm(V3(0, 1, -0.28f)), right(1, 0, 0);
            V3 nrm = norm(cross(V3(-1, 0, 0), up)); // faces -z and slightly down
            {
                PushMat pm(MT.steelDark);
                V3 h = right * 0.56f, v = up * 0.29f, d = nrm * -0.10f;
                // housing behind the screen
                quad(c - h - v + d, c + h - v + d, c + h + v + d, c - h + v + d);
                quad(c - h + v, c + h + v, c + h + v + d, c - h + v + d);
                quad(c - h - v, c - h - v + d, c + h - v + d, c + h - v);
                quad(c - h - v, c - h + v, c - h + v + d, c - h - v + d);
                quad(c + h - v, c + h - v + d, c + h + v + d, c + h + v);
                PushMat pm2(MT.chromeTrim);
                cyl(c + up * 0.29f + V3(-0.3f, 0, 0), V3(sx - 0.3f, CEIL_Y, -4.5f), 0.01f, 0.01f, 6);
                cyl(c + up * 0.29f + V3(0.3f, 0, 0), V3(sx + 0.3f, CEIL_Y, -4.5f), 0.01f, 0.01f, 6);
            }
            decal(c + nrm * 0.004f, V3(-1, 0, 0), up, 1.06f, 0.52f, SG.score[(p * 2 + s) % 3], screenGlow);
            // lane number tile above the screen
            decal(c + up * 0.37f + nrm * 0.004f, V3(-1, 0, 0), up, 0.16f, 0.16f, SG.laneNum[2 * p + s]);
        }
    }
    // low planter wall along the step up to the concourse, with gaps for stairs at each pair
    for (int p = 0; p <= NLANES / 2; p++) {
        float x0 = (p == 0) ? -HALL_X : (laneX(2 * p - 1) + laneX(2 * p)) * 0.5f - 0.65f;
        float x1 = (p == NLANES / 2) ? HALL_X : (laneX(2 * p - 1) + laneX(2 * p)) * 0.5f + 0.65f;
        if (p == 0) x1 = laneX(0) - 0.2f - 0.65f + 0.865f - 0.865f + (laneX(0) + laneX(1)) * 0.0f, x1 = (laneX(0) + laneX(1)) * 0.5f - 2.2f;
        if (p == NLANES / 2) x0 = (laneX(NLANES - 2) + laneX(NLANES - 1)) * 0.5f + 2.2f;
        (void)x0; (void)x1;
    }
}

// ---------------------------------------------------------------- back of house: service aisle behind the machines
static void buildBackOfHouse() {
    // work lights: bare bulbs in cages hanging over every other machine, plus two fluorescent strips over the aisle
    for (int i = 0; i < NLANES; i += 2) {
        float x = laneX(i) + 0.865f;
        V3 b(x, 2.32f, 20.25f);
        sphereLight(b, 0.032f, V3(1500, 980, 460));
        PushMat pm(MT.black);
        cyl(b + V3(0, 0.03f, 0), b + V3(0, 0.09f, 0), 0.018f, 0.018f, 8);
        tube({b + V3(0, 0.09f, 0), V3(x, CEIL_Y, 20.25f)}, 0.004f, 5);
        PushMat pm2(MT.steel);
        for (int k = 0; k < 6; k++) {
            float a = k * PI / 3;
            V3 d(std::cos(a) * 0.055f, 0, std::sin(a) * 0.055f);
            tube(smoothPath({b + V3(d.x * 0.4f, 0.06f, d.z * 0.4f), b + d + V3(0, 0.0f, 0), b + V3(d.x * 0.3f, -0.075f, d.z * 0.3f)}, 4), 0.0018f, 4);
        }
    }
    for (int p = 0; p < NLANES / 2; p++) {   // fluorescent strips over the machines
        float x = (laneX(2 * p) + laneX(2 * p + 1)) * 0.5f;
        rectLight(V3(x, CEIL_Y - 0.06f, 19.5f), V3(0.6f, 0, 0), V3(0, 0, 0.05f), V3(34, 35, 31));
        PushMat pm(MT.white);
        box(V3(x - 0.66f, CEIL_Y - 0.055f, 19.40f), V3(x + 0.66f, CEIL_Y, 19.60f));
    }
    for (int k = 0; k < 2; k++) {
        float x = k ? 4.6f : -4.6f;
        rectLight(V3(x, CEIL_Y - 0.06f, 22.9f), V3(0.6f, 0, 0), V3(0, 0, 0.05f), V3(26, 27, 24));
        PushMat pm(MT.white);
        box(V3(x - 0.66f, CEIL_Y - 0.055f, 22.80f), V3(x + 0.66f, CEIL_Y, 23.0f));
    }
    // workbench against the rear wall with pegboard, vise, spare pins, radio, thermos, oil can
    {
        Push b(xfT(1.6f, -0.2f, BACK_Z - 0.38f));
        { PushMat pm(MT.darkWood, 31); rbox(V3(-1.2f, 0.84f, -0.36f), V3(1.2f, 0.90f, 0.36f), 0.008f); }
        { PushMat pm(MM.green, 60); for (int s = -1; s <= 1; s += 2) for (int t = -1; t <= 1; t += 2) rbox(V3(s * 1.12f - 0.025f, 0, t * 0.30f - 0.025f), V3(s * 1.12f + 0.025f, 0.84f, t * 0.30f + 0.025f), 0.004f);
          rbox(V3(-1.14f, 0.25f, -0.30f), V3(1.14f, 0.28f, 0.30f), 0.004f); }
        { PushMat pm(mat(rgb(0xb89a6a), 0.8f).kind(K_PEGBOARD)); quad(V3(-1.2f, 0.95f, 0.365f), V3(1.2f, 0.95f, 0.365f), V3(1.2f, 2.0f, 0.365f), V3(-1.2f, 2.0f, 0.365f)); }
        // tools on the pegboard
        { PushMat pm(MT.steel);
          for (int i = 0; i < 6; i++) { float x = -1.0f + i * 0.17f; cyl(V3(x, 1.85f, 0.35f), V3(x, 1.85f - 0.16f - 0.03f * i, 0.35f), 0.008f, 0.008f, 8); torus(V3(x, 1.87f, 0.35f), V3(0, 0, 1), 0.016f + 0.002f * i, 0.005f, 12, 5); }
          PushMat pm2(MM.redPaint);
          for (int i = 0; i < 4; i++) { float x = 0.2f + i * 0.13f; cyl(V3(x, 1.80f, 0.35f), V3(x, 1.68f, 0.35f), 0.014f, 0.012f, 8); PushMat pm3(MT.steel); cyl(V3(x, 1.68f, 0.35f), V3(x, 1.52f - 0.02f * i, 0.35f), 0.004f, 0.003f, 6); }
        }
        // spare pins lined up, one lying down
        for (int i = 0; i < 5; i++) bake(P_PIN, xfT(-1.0f + i * 0.15f, 0.90f, 0.18f) * xfRY(i * 1.1f), 800.0f + i);
        bake(P_PIN, xfT(-0.2f, 0.90f + 0.06f, -0.1f) * xfRY(0.6f) * xfRX(PI / 2), 806.0f);
        {   // radio with a glowing dial
            PushMat pm(MT.cream);
            rbox(V3(0.35f, 0.90f, 0.05f), V3(0.72f, 1.12f, 0.28f), 0.02f);
            PushMat pm2(mat(V3(0.9f, 0.7f, 0.3f), 0.3f).emit(V3(7, 4.5f, 1.5f), 3));
            quad(V3(0.40f, 1.02f, 0.048f), V3(0.40f, 1.08f, 0.048f), V3(0.67f, 1.08f, 0.048f), V3(0.67f, 1.02f, 0.048f));
            PushMat pm3(MT.brass);
            cyl(V3(0.42f, 0.96f, 0.05f), V3(0.42f, 0.96f, 0.03f), 0.018f, 0.016f, 12);
            cyl(V3(0.65f, 0.96f, 0.05f), V3(0.65f, 0.96f, 0.03f), 0.018f, 0.016f, 12);
            tube({V3(0.70f, 1.12f, 0.2f), V3(0.9f, 1.55f, 0.25f)}, 0.002f, 4);
        }
        {   // thermos, oil can, coffee mug, rag
            PushMat pm(MM.redPaint);
            Push p(xfT(0.95f, 0.90f, 0.12f));
            lathe({V2(0, 0), V2(0.045f, 0), V2(0.045f, 0.22f), V2(0.03f, 0.25f), V2(0.03f, 0.29f), V2(0, 0.29f)}, 16);
            PushMat pm2(MT.chrome);
            lathe({V2(0.031f, 0.25f), V2(0.036f, 0.26f), V2(0.036f, 0.32f), V2(0, 0.33f)}, 16);
        }
        {
            PushMat pm(MT.brass);
            Push p(xfT(-0.55f, 0.90f, -0.15f));
            lathe({V2(0, 0), V2(0.05f, 0), V2(0.05f, 0.05f), V2(0.012f, 0.09f), V2(0.006f, 0.20f), V2(0, 0.20f)}, 14);
        }
        {   // bench vise
            PushMat pm(MM.green, 61);
            rbox(V3(-1.15f, 0.90f, -0.32f), V3(-0.95f, 1.0f, -0.12f), 0.01f);
            rbox(V3(-1.12f, 1.0f, -0.36f), V3(-0.98f, 1.10f, -0.26f), 0.008f);
            rbox(V3(-1.12f, 1.0f, -0.22f), V3(-0.98f, 1.10f, -0.14f), 0.008f);
            PushMat pm2(MT.steel);
            cyl(V3(-1.05f, 1.03f, -0.42f), V3(-1.05f, 1.03f, -0.12f), 0.012f, 0.012f, 8);
            cyl(V3(-1.12f, 1.03f, -0.42f), V3(-0.98f, 1.03f, -0.42f), 0.005f, 0.005f, 6);
        }
        // calendar and danger sign above the bench
        decal(V3(1.55f, 1.55f, 0.36f), V3(-1, 0, 0), V3(0, 1, 0), 0.34f, 0.48f, SG.calendar);
    }
    // crates of pins and a rack of spare balls along the wall
    for (int k = 0; k < 3; k++) {
        Push b(xfT(-3.2f - k * 1.05f, -0.2f, BACK_Z - 0.4f));
        { PushMat pm(MT.darkWood, 40 + k); rbox(V3(-0.42f, 0, -0.3f), V3(0.42f, 0.36f, 0.3f), 0.006f); }
        { PushMat pm(MT.black); box(V3(-0.38f, 0.355f, -0.26f), V3(0.38f, 0.362f, 0.26f)); }
        for (int i = 0; i < 6; i++) bake(MP.pinLo, xfT(-0.3f + (i % 3) * 0.3f, 0.30f, -0.12f + (i / 3) * 0.24f) * xfRZ(0.25f * hash1(i + k * 9.0f) - 0.12f) * xfRY(i), 820.0f + i + k * 10);
    }
    // nameplates and danger placards on the hero machine and its neighbours
    for (int i = HERO - 1; i <= HERO + 1; i++) {
        float cx = laneX(i);
        decal(V3(cx + 0.3f, 0.95f, WH_Z + 0.192f), V3(-1, 0, 0), V3(0, 1, 0), 0.34f, 0.14f, SG.nameplate);
        decal(V3(cx - 0.1f, 0.6f, WH_Z + 0.192f), V3(-1, 0, 0), V3(0, 1, 0), 0.30f, 0.125f, SG.danger);
        decal(V3(cx + 0.695f, 1.04f, 20.398f), V3(1, 0, 0), V3(0, 1, 0), 0.15f, 0.0625f, SG.danger);
    }
    // conduit and a long pipe along the rear wall
    {
        PushMat pm(MT.steel);
        cyl(V3(-HALL_X + 0.2f, 2.5f, BACK_Z - 0.05f), V3(HALL_X - 0.2f, 2.5f, BACK_Z - 0.05f), 0.02f, 0.02f, 10);
        cyl(V3(-HALL_X + 0.2f, 2.62f, BACK_Z - 0.04f), V3(HALL_X - 0.2f, 2.62f, BACK_Z - 0.04f), 0.012f, 0.012f, 8);
        for (int i = 0; i < 9; i++) rbox(V3(-8.0f + i * 2.0f - 0.03f, 2.46f, BACK_Z - 0.09f), V3(-8.0f + i * 2.0f + 0.03f, 2.66f, BACK_Z), 0.004f);
    }
    // fire extinguisher and exit sign at the back door
    {
        PushMat pm(MM.redPaint);
        Push p(xfT(6.2f, 0.75f, BACK_Z - 0.12f));
        lathe({V2(0, 0), V2(0.07f, 0), V2(0.07f, 0.42f), V2(0.03f, 0.50f), V2(0.02f, 0.56f), V2(0, 0.56f)}, 16);
        static int exitGlow = emitDecalMat(V3(9, 1.2f, 0.8f), 2);
        decal(V3(1.6f, 2.15f, -0.02f + 0.1f), V3(-1, 0, 0), V3(0, 1, 0), 0.40f, 0.175f, SG.exitSign, exitGlow);
        PushMat pm2(MT.steelDark);
        box(V3(1.0f, -0.95f, 0.10f), V3(2.2f, 1.25f, 0.13f));   // back door
    }
}
