#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <cmath>
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <array>
#include <cstdlib>
#include <exception>
#include <random>
#include <fstream>   

static const std::string ASSET_ROOT = R"(C:/Users/Administrator/Documents/OOP project/Tekken2D/x64/Debug/assets)";
static constexpr float STADIUM_W = 2200.f;
static constexpr float STADIUM_H = 1300.f;
static constexpr float FIELD_X = 180.f;
static constexpr float FIELD_Y = 160.f;
static constexpr float FIELD_W = 1840.f;
static constexpr float FIELD_H = 980.f;
static constexpr float PLAYER_R = 18.f;
static constexpr float BALL_R = 10.f;
static constexpr float GOAL_H = 250.f;
static constexpr int WIN_GOALS = 3;

namespace U {
    inline float len(sf::Vector2f v) { return std::sqrt(v.x * v.x + v.y * v.y); }
    inline float dist(sf::Vector2f a, sf::Vector2f b) { return len(a - b); }
    inline sf::Vector2f norm(sf::Vector2f v) { const float l = len(v); return l < 0.01f ? sf::Vector2f(0.f, 0.f) : sf::Vector2f(v.x / l, v.y / l); }
    inline float clamp(float v, float a, float b) { return std::max(a, std::min(v, b)); }
    inline sf::Vector2f clamp(sf::Vector2f p, sf::FloatRect r, float m) {
        p.x = clamp(p.x, r.left + m, r.left + r.width - m);
        p.y = clamp(p.y, r.top + m, r.top + r.height - m);
        return p;
    }
    inline float f(unsigned int v) { return static_cast<float>(v); }
    inline float f(int v) { return static_cast<float>(v); }
    inline float f(std::size_t v) { return static_cast<float>(v); }
}

namespace FifaIntro {
    //Constant for scene
    constexpr float W = 960.f;
    constexpr float H = 540.f;
    constexpr float PI = 3.14159265358979f;

    // Math calculations
    static inline float lerp(float a, float b, float t) { return a + (b - a) * t; }
    static inline float clamp(float x, float a, float b) { return std::max(a, std::min(b, x)); }
    static inline float easeIn3(float t) { return t * t * t; }
    static inline float easeOut3(float t) { float u = 1.f - t; return 1.f - u * u * u; }
    static inline float smoothstep(float t) { t = clamp(t, 0.f, 1.f); return t * t * (3.f - 2.f * t); }

    static std::mt19937 rng(42);
    static float frand() { return std::uniform_real_distribution<float>(0.f, 1.f)(rng); }
    static float frandRange(float a, float b) { return lerp(a, b, frand()); }

    // Color design
    static sf::Color lerpColor(sf::Color a, sf::Color b, float t) {
        return sf::Color(
            (sf::Uint8)lerp(a.r, b.r, t),
            (sf::Uint8)lerp(a.g, b.g, t),
            (sf::Uint8)lerp(a.b, b.b, t),
            (sf::Uint8)lerp(a.a, b.a, t)
        );
    }
    static sf::Color withAlpha(sf::Color c, sf::Uint8 a) { c.a = a; return c; }

    //  Particle System part

    struct Particle {
        sf::Vector2f pos{ 0.f, 0.f }, vel{ 0.f, 0.f };
        float radius = 0.f;
        float life = 0.f;
        float decay = 0.f;
        float gravity = 0.f;
        sf::Color col = sf::Color::White;
        float glow = 0.f; 
    };

    static std::vector<Particle> PARTS;

    struct ParticleOpts {
        std::vector<sf::Color> cols = { sf::Color::White };
        float maxS = 4.f, minS = .5f;
        float spr = 1.f;
        float vx = 0.f, vy = 0.f;
        float dec = .025f;
        float maxR = 4.f, minR = 1.f;
        float gv = 0.f;
        float glo = 0.f;
    };

    static void addP(int n, float x, float y, const ParticleOpts& o = {}) {
        PARTS.reserve(PARTS.size() + n);
        for (int i = 0; i < n; ++i) {
            float a = frand() * PI * 2.f;
            float sp = frandRange(o.minS, o.maxS);
            Particle p;
            p.pos = { x, y };
            p.vel = { std::cos(a) * sp * o.spr + o.vx,
                        std::sin(a) * sp * o.spr + o.vy };
            p.radius = frandRange(o.minR, o.maxR);
            p.life = 1.f;
            p.decay = frandRange(0.008f, o.dec);
            p.col = o.cols[int(frand() * o.cols.size()) % o.cols.size()];
            p.gravity = o.gv;
            p.glow = o.glo;
            PARTS.push_back(p);
        }
    }

    static void tickP() {
        for (auto& p : PARTS) {
            p.pos += p.vel;
            p.vel.y += p.gravity;
            p.life -= p.decay;
        }
        PARTS.erase(std::remove_if(PARTS.begin(), PARTS.end(),
            [](const Particle& p) { return p.life <= 0.f; }), PARTS.end());
    }

    static void drawP(sf::RenderTarget& rt) {
        for (auto& p : PARTS) {
            float r = std::max(.5f, p.radius * p.life);
            // Circle glow
            if (p.glow > 0.f) {
                sf::CircleShape glo(r + p.glow * .5f);
                glo.setOrigin(r + p.glow * .5f, r + p.glow * .5f);
                glo.setPosition(p.pos);
                sf::Color gc = withAlpha(p.col, sf::Uint8(p.life * 80));
                glo.setFillColor(gc);
                rt.draw(glo);
            }
            sf::CircleShape c(r);
            c.setOrigin(r, r);
            c.setPosition(p.pos);
            c.setFillColor(withAlpha(p.col, sf::Uint8(p.life * 230)));
            rt.draw(c);
        }
    }

    //  Shockwaves part
 
    struct Shockwave {
        sf::Vector2f pos;
        sf::Color col;
        float r, maxR, life, speed;
    };

    static std::vector<Shockwave> SHOCKS;

    static void addSW(float x, float y, sf::Color col, float maxR, float spd) {
        SHOCKS.push_back({ {x,y}, col, 0.f, maxR, 1.f, spd });
    }

    static void tickSW() {
        for (auto& s : SHOCKS) {
            s.r += s.speed;
            s.life = 1.f - s.r / s.maxR;
        }
        SHOCKS.erase(std::remove_if(SHOCKS.begin(), SHOCKS.end(),
            [](const Shockwave& s) { return s.life <= 0.f; }), SHOCKS.end());
    }

    static void drawSW(sf::RenderTarget& rt) {
        for (auto& s : SHOCKS) {
            int segs = 64;
            sf::VertexArray ring(sf::LinesStrip, segs + 1);
            for (int i = 0; i <= segs; ++i) {
                float a = (float)i / segs * PI * 2.f;
                ring[i].position = { s.pos.x + std::cos(a) * s.r,
                                      s.pos.y + std::sin(a) * s.r };
                ring[i].color = withAlpha(s.col, sf::Uint8(s.life * 200));
            }
            rt.draw(ring);
        }
    }

    //  Cracks part

    struct Crack {
        std::vector<sf::Vector2f> pts;
        float maxAlpha = 0.f;
    };

    static std::vector<Crack> CRACKS;

    static void spawnCracks(float ox, float oy) {
        CRACKS.clear();
        for (int i = 0; i < 14; ++i) {
            float baseA = (float)i / 14.f * PI * 2.f + frand() * .4f;
            float len = frandRange(70.f, 190.f);
            Crack c;
            c.maxAlpha = frandRange(.25f, .80f);
            c.pts.push_back({ ox, oy });
            float cx = ox, cy = oy;
            for (int j = 0; j < 5; ++j) {
                float j2 = (frand() - .5f) * 40.f;
                cx += std::cos(baseA + .02f * j2) * (len / 5.f);
                cy += std::sin(baseA + .02f * j2) * (len / 5.f);
                c.pts.push_back({ cx + j2 * .5f, cy + j2 * .5f });
            }
            CRACKS.push_back(c);
        }
    }

    static void drawCracks(sf::RenderTarget& rt, float prog) {
        for (auto& c : CRACKS) {
            int n = (int)std::ceil(prog * (c.pts.size() - 1));
            if (n <= 0) continue;
            sf::VertexArray line(sf::LinesStrip, n + 1);
            for (int i = 0; i <= std::min(n, (int)c.pts.size() - 1); ++i) {
                line[i].position = c.pts[i];
                line[i].color = sf::Color(80, 170, 255, sf::Uint8(c.maxAlpha * 200));
            }
            rt.draw(line);
        }
    }

    //  Stars background part

    struct Star { float x, y, r, ph, spd; };
    static std::vector<Star> STARS;

    static void initStars() {
        STARS.clear();
        for (int i = 0; i < 380; ++i)
            STARS.push_back({ frand() * W, frand() * H,
            frandRange(.2f,2.f),
            frand() * PI * 2.f,
            frandRange(.5f,2.f) });
    }

    static void drawStars(sf::RenderTarget& rt, float t, float alpha) {
        for (auto& s : STARS) {
            float tw = .45f + .55f * std::sin(s.ph + t * s.spd * 1.8f);
            sf::CircleShape c(s.r);
            c.setOrigin(s.r, s.r);
            c.setPosition(s.x, s.y);
            c.setFillColor(sf::Color(255, 255, 255, sf::Uint8(alpha * tw * 200)));
            rt.draw(c);
        }
    }

    // ══════════════════════════════════════════════════════════
    //  FOOTBALL BALL DRAWING
    //  Drawn via sf::VertexArray + sf::CircleShape
    // ══════════════════════════════════════════════════════════
    static void drawBall(sf::RenderTarget& rt, float x, float y,
        float r, float angle,
        sf::Color glowCol = { 0,0,0,0 }, float glowSz = 0.f,
        float alpha = 1.f)
    {
        if (r <= 0.f) return;

        // Outer glow
        if (glowSz > 0.f && glowCol.a > 0) {
            float gr = r + glowSz;
            sf::CircleShape glo(gr);
            glo.setOrigin(gr, gr);
            glo.setPosition(x, y);
            glo.setFillColor(withAlpha(glowCol, sf::Uint8(alpha * 80)));
            rt.draw(glo);
        }

        // Base circle (gradient simulated as solid + shading rings)
        sf::CircleShape ball(r);
        ball.setOrigin(r, r);
        ball.setPosition(x, y);
        ball.setFillColor(sf::Color(220, 220, 220, sf::Uint8(alpha * 255)));
        ball.setOutlineThickness(0.8f);
        ball.setOutlineColor(sf::Color(80, 80, 80, sf::Uint8(alpha * 200)));
        rt.draw(ball);

        // Dark edge shading
        sf::CircleShape edge(r);
        edge.setOrigin(r, r);
        edge.setPosition(x, y);
        edge.setFillColor(sf::Color(0, 0, 0, 0));
        edge.setOutlineThickness(-r * .18f);
        edge.setOutlineColor(sf::Color(0, 0, 0, sf::Uint8(alpha * 100)));
        rt.draw(edge);

        // Pentagon patches (8 patches approximated as filled pentagons)
        struct PatchDef { float cx, cy, sa; };
        constexpr PatchDef patches[] = {
            { 0,-14,-90}, {-13,7,-18}, {13,7,-18},
            {-20,-1,54}, {20,-1,54}, {0,20,18},
            {-12,-14,126}, {12,-14,126}
        };
        float s = r / 24.f;
        for (auto& pd : patches) {
            sf::ConvexShape pent;
            pent.setPointCount(5);
            for (int i = 0; i < 5; ++i) {
                float a = (pd.sa + i * 72.f) * PI / 180.f + angle;
                pent.setPoint(i, { pd.cx * s + 9.5f * s * std::cos(a),
                                    pd.cy * s + 9.5f * s * std::sin(a) });
            }
            pent.setPosition(x, y);
            pent.setFillColor(sf::Color(12, 12, 12, sf::Uint8(alpha * 240)));
            pent.setOutlineColor(sf::Color(50, 50, 50, sf::Uint8(alpha * 160)));
            pent.setOutlineThickness(0.4f);
            rt.draw(pent);
        }

        // Specular highlight (top-left white circle)
        sf::CircleShape spec(r * .35f);
        spec.setOrigin(r * .35f, r * .35f);
        spec.setPosition(x - r * .28f, y - r * .28f);
        spec.setFillColor(sf::Color(255, 255, 255, sf::Uint8(alpha * 120)));
        rt.draw(spec);
    }

    // ══════════════════════════════════════════════════════════
    //  ELECTRIC ARCS  (drawn as jagged line strips)
    // ══════════════════════════════════════════════════════════
    static void drawArcs(sf::RenderTarget& rt, float x, float y, float r, float T,
        sf::Color col)
    {
        for (int a = 0; a < 7; ++a) {
            float ba = (float)a / 7.f * PI * 2.f + T * 1.6f;
            float ex = x + std::cos(ba + std::sin(T * 3.f + a) * .4f) * (r + 22.f + std::sin(T * 5.f + a) * 18.f);
            float ey = y + std::sin(ba + std::sin(T * 3.f + a) * .4f) * (r + 22.f + std::sin(T * 5.f + a) * 18.f);
            float sx = x + std::cos(ba) * r, sy = y + std::sin(ba) * r;

            sf::VertexArray arc(sf::LinesStrip, 7);
            arc[0].position = { sx, sy };
            arc[0].color = withAlpha(col, 150);
            for (int seg = 1; seg <= 6; ++seg) {
                float f = (float)seg / 6.f;
                float px = lerp(sx, ex, f) + std::sin(T * 12.f + seg * 4.2f + a) * 14.f * (1.f - f);
                float py = lerp(sy, ey, f) + std::cos(T * 9.f + seg * 3.f + a) * 14.f * (1.f - f);
                arc[seg].position = { px, py };
                arc[seg].color = withAlpha(col, sf::Uint8(150 * (1.f - f * .5f)));
            }
            rt.draw(arc);
        }
    }

    // ══════════════════════════════════════════════════════════
    //  DRAW GRADIENT RECT (horizontal)
    // ══════════════════════════════════════════════════════════
    static void drawGradRect(sf::RenderTarget& rt,
        float x, float y, float w, float h,
        sf::Color top, sf::Color bot)
    {
        sf::VertexArray q(sf::Quads, 4);
        q[0] = { {x,   y  }, top };
        q[1] = { {x + w, y  }, top };
        q[2] = { {x + w, y + h}, bot };
        q[3] = { {x,   y + h}, bot };
        rt.draw(q);
    }

    // ══════════════════════════════════════════════════════════
    //  CLOUD STRIP
    // ══════════════════════════════════════════════════════════
    static void drawClouds(sf::RenderTarget& rt, float spd, float alpha, float st)
    {
        for (int i = 0; i < 10; ++i) {
            float off = std::fmod(st * spd * 180.f + i * H / 10.f, H + 120.f) - 60.f;
            float x = (i % 2 == 0) ? W * .1f + i * 55.f : W * .55f + i * 35.f;
            float a2 = alpha * (.25f + .3f * std::sin((float)i));
            int   aa = (int)(a2 * 200);

            for (int bi = 0; bi < 4; ++bi) {
                float bs[] = { 70.f,52.f,58.f,40.f };
                sf::CircleShape c(bs[bi]);
                c.setOrigin(bs[bi], bs[bi]);
                c.setPosition(x + bi * 28.f, off - bi * 10.f);
                c.setFillColor(sf::Color(200, 220, 255, sf::Uint8(aa)));
                rt.draw(c);
            }
        }
    }

    // ══════════════════════════════════════════════════════════
    //  STADIUM BACKGROUND
    // ══════════════════════════════════════════════════════════
    static void drawStadium(sf::RenderTarget& rt, float bright, float shake)
    {
        sf::RenderStates rs;
        sf::Transform tr;
        if (shake > 0.f) {
            tr.translate((frand() - .5f) * shake, (frand() - .5f) * shake);
            rs.transform = tr;
        }

        auto B = [&](sf::Uint8 v) { return sf::Uint8(v * bright); };
        auto bC = [&](int r2, int g2, int b2, float a) { return sf::Color(B(r2), B(g2), B(b2), sf::Uint8(a * 255)); };

        // Sky gradient
        drawGradRect(rt, 0, 0, W, H,
            bC(2, 5, 18, 1.f), bC(8, 28, 14, 1.f));

        // Back stand
        sf::RectangleShape bs({ W - 80.f, H * .5f });
        bs.setPosition(40, 28); bs.setFillColor(bC(18, 26, 42, .9f)); rt.draw(bs, rs);
        for (int i = 0; i < 10; ++i) {
            sf::RectangleShape row({ W - 90.f,14.f });
            row.setPosition(45, 38 + i * 20);
            row.setFillColor(sf::Color(255, 255, 255, B(6)));
            rt.draw(row, rs);
        }

        // Side stands (left)
        sf::ConvexShape left; left.setPointCount(4);
        left.setPoint(0, { 0,48 }); left.setPoint(1, { 115,78 });
        left.setPoint(2, { 115,H * .78f }); left.setPoint(3, { 0,H * .85f });
        left.setFillColor(bC(15, 22, 38, .9f)); rt.draw(left, rs);
        sf::ConvexShape right; right.setPointCount(4);
        right.setPoint(0, { W,48 }); right.setPoint(1, { W - 115,78 });
        right.setPoint(2, { W - 115,H * .78f }); right.setPoint(3, { W,H * .85f });
        right.setFillColor(bC(15, 22, 38, .9f)); rt.draw(right, rs);

        // Floodlights
        for (int fi = 0; fi < 2; ++fi) {
            float fx = (fi == 0) ? 22.f : W - 22.f;
            float fy = 16.f;
            // Glow cone (radial gradient simulated)
            sf::VertexArray cone(sf::TriangleFan, 18);
            cone[0] = { {fx,fy}, sf::Color(255,250,200, sf::Uint8(120 * bright)) };
            for (int ci = 0; ci < 17; ++ci) {
                float a = (float)ci / 16.f * PI * .4f - PI * .2f;
                cone[ci + 1].position = { fx + std::sin(a) * 320.f, fy + std::cos(a) * 220.f };
                cone[ci + 1].color = sf::Color(255, 250, 200, 0);
            }
            rt.draw(cone, rs);

            // Pole
            sf::RectangleShape pole({ 6.f,84.f }); pole.setPosition(fx - 3, fy);
            pole.setFillColor(bC(38, 52, 72, 1.f)); rt.draw(pole, rs);
            sf::RectangleShape arm({ 18.f,7.f }); arm.setPosition(fx - 9, fy - 5);
            arm.setFillColor(bC(40, 55, 78, 1.f)); rt.draw(arm, rs);

            float bxs[] = { fx - 7,fx - 3,fx + 1,fx + 5 };
            for (auto bxi : bxs) {
                sf::CircleShape bulb(2.8f); bulb.setOrigin(2.8f, 2.8f);
                bulb.setPosition(bxi, fy - 1);
                bulb.setFillColor(sf::Color(255, 252, 210, B(255)));
                rt.draw(bulb, rs);
            }
        }

        // Pitch ellipse
        float py = H * .73f;
        // Fill (dark green)
        sf::CircleShape pitchFill(W * .56f); // we'll use convex for ellipse
        // Draw pitch as stretched quad
        {
            float rx = W * .56f, ry = H * .36f;
            int segs = 64;
            sf::VertexArray pitch(sf::TriangleFan, segs + 2);
            pitch[0] = { {W / 2, py}, bC(9,58,17,1.f) };
            for (int i = 0; i <= segs; ++i) {
                float a = (float)i / segs * PI * 2.f;
                pitch[i + 1].position = { W / 2 + std::cos(a) * rx, py + std::sin(a) * ry };
                pitch[i + 1].color = bC(4, 32, 8, 1.f);
            }
            rt.draw(pitch, rs);
        }

        // Pitch stripes
        float pitchRx[] = { 186.f,158.f,128.f,98.f };
        for (int pi = 0; pi < 4; ++pi) {
            float rx = pitchRx[pi], ry = H * .36f - pi * 20.f;
            int segs = 64;
            sf::VertexArray ring(sf::LinesStrip, segs + 1);
            sf::Color sc = (pi % 2 == 0) ? bC(22, 108, 38, .55f) : bC(15, 82, 30, .55f);
            for (int i = 0; i <= segs; ++i) {
                float a = (float)i / segs * PI * 2.f;
                ring[i].position = { W / 2 + std::cos(a) * rx, py + 4 + std::sin(a) * ry };
                ring[i].color = sc;
            }
            rt.draw(ring, rs);
        }

        // Centre circle
        {
            int segs = 48;
            sf::VertexArray circle(sf::LinesStrip, segs + 1);
            for (int i = 0; i <= segs; ++i) {
                float a = (float)i / segs * PI * 2.f;
                circle[i].position = { W / 2 + std::cos(a) * 44.f, py + 8 + std::sin(a) * 16.f };
                circle[i].color = sf::Color(255, 255, 255, B(55));
            }
            rt.draw(circle, rs);
        }

        // Centre line
        {
            sf::VertexArray cl(sf::Lines, 2);
            cl[0] = { {W / 2 - 210,py + 8}, sf::Color(255,255,255,B(46)) };
            cl[1] = { {W / 2 + 210,py + 8}, sf::Color(255,255,255,B(46)) };
            rt.draw(cl, rs);
        }

        // Centre spot
        sf::CircleShape spot(3.f); spot.setOrigin(3.f, 3.f);
        spot.setPosition(W / 2, py + 8);
        spot.setFillColor(sf::Color(255, 255, 255, B(90)));
        rt.draw(spot, rs);

        // Goal posts
        float gx[] = { W / 2 - 210.f, W / 2 + 190.f };
        for (float gpx : gx) {
            sf::RectangleShape vbar({ 3.f,20.f }); vbar.setPosition(gpx, py - 8);
            vbar.setFillColor(sf::Color(255, 255, 255, B(165))); rt.draw(vbar, rs);
            sf::RectangleShape hbar({ 22.f,2.f }); hbar.setPosition(gpx, py - 8);
            hbar.setFillColor(sf::Color(255, 255, 255, B(165))); rt.draw(hbar, rs);
        }

        // Crowd dots
        for (int ci = 0; ci < 100; ++ci) {
            float cx2 = 45.f + ci * (W - 90.f) / 100.f;
            float cy2 = 42.f + std::sin(ci * .4f) * 3.f;
            sf::Color crowdCols[] = {
                sf::Color(102,170,187),sf::Color(221,170,119),
                sf::Color(119,187,221),sf::Color(221,170,119),sf::Color(153,187,204)
            };
            sf::RectangleShape body({ 5.f,9.f }); body.setPosition(cx2, cy2);
            body.setFillColor(withAlpha(crowdCols[ci % 5], B(140))); rt.draw(body, rs);
            sf::RectangleShape legs({ 5.f,8.f }); legs.setPosition(cx2, cy2 + 6);
            legs.setFillColor(withAlpha(crowdCols[ci % 5], B(110))); rt.draw(legs, rs);
        }
    }

    // ══════════════════════════════════════════════════════════
    //  TEXT HELPER
    // ══════════════════════════════════════════════════════════
    static void drawText(sf::RenderTarget& rt, sf::Font& font,
        const std::string& str, float x, float y,
        unsigned sz, sf::Color col, bool center = true)
    {
        sf::Text txt(str, font, sz);
        txt.setFillColor(col);
        txt.setStyle(sf::Text::Bold);
        if (center) {
            sf::FloatRect b = txt.getLocalBounds();
            txt.setOrigin(b.width / 2.f, b.height / 2.f);
        }
        txt.setPosition(x, y);
        rt.draw(txt);
    }

    // ══════════════════════════════════════════════════════════
    //  SCENE FUNCTIONS
    // ══════════════════════════════════════════════════════════

    // ── Scene 1 : Space ───────────────────────────────────────
    static void scene1(sf::RenderTarget& rt, sf::Font& font, float t, float st, float& ballAngle)
    {
        float p = clamp(st / 3.f, 0.f, 1.f);

        // Deep space background
        drawGradRect(rt, 0, 0, W, H, sf::Color(4, 9, 26), sf::Color(2, 7, 16));

        // Nebula blobs (radial fade)
        struct Neb { float x, y; sf::Color c; };
        Neb nebs[] = {
            {W * .18f, H * .28f, sf::Color(26,8,56)},
            {W * .74f, H * .38f, sf::Color(8,24,58)},
            {W * .48f, H * .72f, sf::Color(8,40,26)}
        };
        for (auto& nb : nebs) {
            sf::VertexArray nebShape(sf::TriangleFan, 34);
            nebShape[0] = { {nb.x,nb.y}, withAlpha(nb.c,35) };
            for (int i = 0; i <= 32; ++i) {
                float a = (float)i / 32.f * PI * 2.f;
                nebShape[i + 1] = { {nb.x + std::cos(a) * 210.f, nb.y + std::sin(a) * 210.f}, sf::Color(0,0,0,0) };
            }
            rt.draw(nebShape);
        }

        // Stars with parallax
        float sA = (p < .75f) ? 1.f : lerp(1.f, .3f, (p - .75f) / .25f);
        drawStars(rt, t, sA);

        // Ball position
        float fy = std::sin(t * 1.1f) * 9.f;
        float fp = (p < .65f) ? 0.f : easeIn3((p - .65f) / .35f);
        float bx = W / 2.f;
        float by = H / 2.f + fy - fp * H * .85f;

        ballAngle += .012f + fp * .13f;

        // Electric arcs (fade on fall)
        float aa = 1.f - fp * 1.6f;
        if (aa > 0.f) {
            drawArcs(rt, bx, by, 50.f, t, sf::Color(0, 170, 255, sf::Uint8(aa * 200)));
            drawArcs(rt, bx, by, 50.f, t * 1.4f + 1.f, sf::Color(80, 140, 255, sf::Uint8(aa * 150)));
        }

        // Fire trail when falling
        if (fp > .08f) {
            ParticleOpts po;
            po.cols = { sf::Color(255,102,0),sf::Color(255,153,0),sf::Color(255,204,0),sf::Color(255,68,0) };
            po.maxS = 3.f; po.minS = .5f; po.gv = -.08f; po.dec = .055f; po.maxR = 14.f; po.glo = 16.f;
            po.vx = (frand() - .5f) * 2.f;
            for (int i = 0; i < 6; ++i)
                addP(2, bx + (frand() - .5f) * 22.f, by + 50.f + i * 14.f, po);
            addSW(bx, by + 50.f, sf::Color(255, 119, 0, 180), 70 + frand() * 20.f, 2.5f);
        }

        tickP(); tickSW(); drawP(rt); drawSW(rt);
        drawBall(rt, bx, by, 50.f, ballAngle,
            fp > .08f ? sf::Color(255, 102, 0, 200) : sf::Color(0, 144, 255, 200),
            fp > .08f ? 22.f + fp * 50.f : 15.f + std::sin(t * 2.f) * 5.f);

        // Title text
        if (p < .28f) {
            float ta = smoothstep(clamp(p / .18f, 0.f, 1.f)) * (1.f - smoothstep(clamp((p - .18f) / .1f, 0.f, 1.f)));
            if (ta > .01f)
                drawText(rt, font, "FIFA 2D FOOTBALL", W / 2.f, H / 2.f - 120.f,
                    14, sf::Color(200, 230, 255, sf::Uint8(ta * 180)));
        }
    }

    // ── Scene 2 : Atmosphere ──────────────────────────────────
    static void scene2(sf::RenderTarget& rt, sf::Font& /*font*/, float /*t*/, float st, float& ballAngle)
    {
        float p = clamp(st / 2.5f, 0.f, 1.f);
        float fp = smoothstep(p * .82f);

        // Fiery sky
        auto sky0 = sf::Color(sf::Uint8(lerp(4, 25, fp)), sf::Uint8(lerp(7, 8, fp)), sf::Uint8(lerp(18, 4, fp)));
        auto sky1 = sf::Color(sf::Uint8(lerp(18, 55, fp)), sf::Uint8(lerp(22, 72, fp)), sf::Uint8(lerp(45, 18, fp)));
        drawGradRect(rt, 0, 0, W, H, sky0, sky1);

        // Fading stars
        float sA = (1.f - p) * .45f;
        if (sA > .01f) drawStars(rt, st, sA);

        drawClouds(rt, 2.f + p * 3.f, p * .85f, st);

        float bx = W / 2.f + std::sin(p * 5.f) * 22.f;
        float by = -55.f + p * (H + 110.f);
        ballAngle += .06f + p * .18f;

        // Smoke & fire trail
        {
            ParticleOpts fire;
            fire.cols = { sf::Color(255,68,0),sf::Color(255,136,0),sf::Color(255,204,0),sf::Color(255,34,0),sf::Color(255,255,255) };
            fire.maxS = 5.f; fire.minS = 1.f; fire.gv = -.04f; fire.dec = .042f; fire.maxR = 18.f; fire.glo = 22.f;
            fire.vx = (frand() - .5f) * 6.f;
            for (int i = 0; i < 10; ++i) addP(3, bx + (frand() - .5f) * 35.f, by - 58.f - i * 22.f, fire);

            ParticleOpts smoke;
            smoke.cols = { sf::Color(160,160,160,128) };
            smoke.maxS = 2.5f; smoke.minS = .5f; smoke.gv = -.018f; smoke.dec = .012f; smoke.maxR = 30.f;
            for (int i = 0; i < 10; ++i) addP(1, bx + (frand() - .5f) * 25.f, by - 45.f - i * 18.f, smoke);
        }
        if (std::sin(st * 9.f) > .88f)
            addSW(bx, by, sf::Color(255, 153, 0, 180), 90 + frand() * 25.f, 3.5f);

        // Lightning flash
        if (p > .58f && p < .82f) {
            float fa = std::sin((p - .58f) / .24f * PI) * .45f;
            sf::RectangleShape flash({ W,H });
            flash.setFillColor(sf::Color(200, 225, 255, sf::Uint8(fa * 255)));
            rt.draw(flash);
        }

        tickP(); tickSW(); drawP(rt); drawSW(rt);
        drawBall(rt, bx, by, 55.f + p * 12.f, ballAngle, sf::Color(255, 102, 0, 200), 32.f + p * 32.f);

        // Fade out
        if (p > .88f) {
            float fo = smoothstep((p - .88f) / .12f);
            sf::RectangleShape fade({ W,H });
            fade.setFillColor(sf::Color(0, 4, 18, sf::Uint8(fo * 255)));
            rt.draw(fade);
        }
    }

    // ── Scene 3 : Stadium Impact ──────────────────────────────
    static void scene3(sf::RenderTarget& rt, sf::Font& /*font*/, float /*t*/, float st,
        float& ballAngle,
        bool& impactTriggered, float& shakeInt, float& cracksGrow)
    {
        constexpr float IMP_T = 1.1f;
        float p = clamp(st / 3.f, 0.f, 1.f);

        if (st > IMP_T && !impactTriggered) {
            impactTriggered = true;
            spawnCracks(W / 2.f, H * .76f);

            ParticleOpts debris;
            debris.cols = { sf::Color(119,119,119),sf::Color(153,153,153),sf::Color(85,85,85),sf::Color(187,187,187),sf::Color(68,68,68) };
            debris.maxS = 10.f; debris.minS = 2.5f; debris.gv = .28f; debris.dec = .018f; debris.maxR = 14.f; debris.glo = 5.f; debris.vy = -5.f;
            addP(80, W / 2.f, H * .76f, debris);

            ParticleOpts dust;
            dust.cols = { sf::Color(190,160,110,178),sf::Color(210,180,130,128) };
            dust.maxS = 6.f; dust.minS = 1.f; dust.gv = -.008f; dust.dec = .01f; dust.maxR = 35.f;
            addP(40, W / 2.f, H * .76f, dust);

            ParticleOpts spark;
            spark.cols = { sf::Color(0,153,255),sf::Color(255,40,0),sf::Color(0,204,255),sf::Color(255,85,0),sf::Color(255,255,255) };
            spark.maxS = 13.f; spark.minS = 3.f; spark.gv = .12f; spark.dec = .022f; spark.maxR = 10.f; spark.glo = 22.f;
            addP(55, W / 2.f, H * .76f, spark);

            shakeInt = 18.f;
            addSW(W / 2.f, H * .76f, sf::Color(0, 153, 255, 200), 320.f, 5.5f);
            addSW(W / 2.f, H * .76f, sf::Color(255, 40, 0, 180), 440.f, 7.f);
            addSW(W / 2.f, H * .76f, sf::Color(255, 255, 255, 128), 260.f, 9.f);
        }
        if (impactTriggered) shakeInt *= .84f;

        float sa = clamp(p / .28f, 0.f, 1.f);
        drawStadium(rt, sa, shakeInt);

        float preP = clamp(st / IMP_T, 0.f, 1.f);
        float bx = W / 2.f, by, br = 58.f;
        if (st < IMP_T) {
            ballAngle += .14f;
            by = lerp(-65.f, H * .76f, easeIn3(preP));
            ParticleOpts ft;
            ft.cols = { sf::Color(255,68,0),sf::Color(255,136,0),sf::Color(255,204,0) };
            ft.maxS = 3.5f; ft.minS = 1.f; ft.gv = -.04f; ft.dec = .06f; ft.maxR = 14.f; ft.glo = 16.f;
            for (int i = 0; i < 7; ++i) addP(3, bx + (frand() - .5f) * 28.f, by - br - i * 16.f, ft);
        }
        else {
            ballAngle += .004f;
            by = H * .76f - 22.f;
        }

        if (impactTriggered) {
            cracksGrow = clamp(cracksGrow + .038f, 0.f, 1.f);
            drawCracks(rt, cracksGrow);
        }

        tickP(); tickSW(); drawP(rt); drawSW(rt);

        // Ground energy glow
        if (impactTriggered && cracksGrow > .15f) {
            float ga = clamp((cracksGrow - .15f) / .85f, 0.f, 1.f) * .55f;
            sf::VertexArray glow(sf::TriangleFan, 34);
            glow[0] = { {W / 2.f,H * .76f}, sf::Color(0,160,255, sf::Uint8(ga * 200)) };
            for (int i = 0; i <= 32; ++i) {
                float a = (float)i / 32.f * PI * 2.f;
                glow[i + 1] = { {W / 2.f + std::cos(a) * 220.f, H * .76f + std::sin(a) * 100.f}, sf::Color(0,0,0,0) };
            }
            rt.draw(glow);
        }

        drawBall(rt, bx, by, br, ballAngle,
            st < IMP_T ? sf::Color(255, 102, 0, 200) : sf::Color(0, 153, 255, 200),
            st < IMP_T ? 32.f : 22.f);

        if (p > .82f) {
            float fo = smoothstep((p - .82f) / .18f);
            sf::RectangleShape fade({ W,H });
            fade.setFillColor(sf::Color(0, 4, 18, sf::Uint8(fo * 255)));
            rt.draw(fade);
        }
    }

    // ── Scene 4 : Loading ─────────────────────────────────────
    static void scene4(sf::RenderTarget& rt, sf::Font& font, float /*t*/, float st, float& ballAngle)
    {
        float p = clamp(st / 2.5f, 0.f, 1.f);
        sf::Uint8 pct = (sf::Uint8)(p * 100);

        sf::RectangleShape bg({ W,H }); bg.setFillColor(sf::Color(1, 12, 26)); rt.draw(bg);

        // Grid
        for (float gx = 0; gx < W; gx += 40) {
            sf::VertexArray vl(sf::Lines, 2);
            vl[0] = { {gx,0}, sf::Color(0,80,160,46) };
            vl[1] = { {gx,H}, sf::Color(0,80,160,46) };
            rt.draw(vl);
        }
        for (float gy = 0; gy < H; gy += 40) {
            sf::VertexArray hl(sf::Lines, 2);
            hl[0] = { {0,gy}, sf::Color(0,80,160,46) };
            hl[1] = { {W,gy}, sf::Color(0,80,160,46) };
            rt.draw(hl);
        }

        // Ambient particles
        {
            ParticleOpts ap;
            ap.cols = { sf::Color(0,160,255,140),sf::Color(255,60,0,102),sf::Color(100,210,255,128) };
            ap.maxS = .45f; ap.minS = .1f; ap.gv = -.008f; ap.dec = .004f; ap.maxR = 5.f; ap.glo = 10.f;
            addP(2, frand() * W, frand() * H, ap);
        }
        tickP(); drawP(rt);

        float tA = clamp(p / .22f, 0.f, 1.f);

        // Title
        drawText(rt, font, "FIFA 2D", W / 2.f, H * .19f, sf::Uint8(W * .082f / 1.f), sf::Color(255, 255, 255, sf::Uint8(tA * 255)));
        drawText(rt, font, "FOOTBALL", W / 2.f, H * .34f, sf::Uint8(W * .108f / 1.f), sf::Color(0, 221, 255, sf::Uint8(tA * 255)));

        // Loading bar
        float bx = W * .13f, by = H * .61f, bw = W * .74f, bh = 22.f;
        float barA = clamp((p - .08f) / .2f, 0.f, 1.f);
        {
            sf::RectangleShape outline({ bw,bh }); outline.setPosition(bx, by);
            outline.setFillColor(sf::Color(0, 0, 0, 0));
            outline.setOutlineThickness(2.f);
            outline.setOutlineColor(sf::Color(0, 160, 255, sf::Uint8(barA * 204)));
            rt.draw(outline);
        }
        if (bw * p > 2.f) {
            sf::RectangleShape fill({ bw * p - 2.f, bh - 2.f }); fill.setPosition(bx + 1, by + 1);
            // Gradient: dark blue → bright cyan
            sf::VertexArray fillGrad(sf::Quads, 4);
            float fw = bw * p - 2.f;
            fillGrad[0] = { {bx + 1,   by + 1  }, sf::Color(0,53,122, sf::Uint8(barA * 255)) };
            fillGrad[1] = { {bx + 1 + fw,by + 1  }, sf::Color(0,210,255,sf::Uint8(barA * 255)) };
            fillGrad[2] = { {bx + 1 + fw,by + bh - 1},sf::Color(0,210,255,sf::Uint8(barA * 255)) };
            fillGrad[3] = { {bx + 1,   by + bh - 1},sf::Color(0,53,122, sf::Uint8(barA * 255)) };
            rt.draw(fillGrad);

            // Shine sweep
            float ss = std::fmod(st * 280.f, bw);
            sf::RectangleShape shine({ 30.f,bh - 2.f }); shine.setPosition(bx + ss - 30, by + 1);
            shine.setFillColor(sf::Color(255, 255, 255, sf::Uint8(barA * 76))); rt.draw(shine);
        }

        // Ball rolling on bar
        float box = bx + bw * p, boy = by + bh / 2.f;
        ballAngle += .09f;
        // Ring around ball
        sf::CircleShape ring(28.f); ring.setOrigin(28.f, 28.f); ring.setPosition(box, boy);
        ring.setFillColor(sf::Color(0, 0, 0, 0));
        ring.setOutlineThickness(2.f);
        ring.setOutlineColor(sf::Color(0, 180, 255, sf::Uint8(barA * 102)));
        rt.draw(ring);
        drawBall(rt, box, boy, 20.f, ballAngle, sf::Color(0, 136, 255, 200), 22.f, barA);

        // Percentage text
        drawText(rt, font, std::to_string(pct) + "%", W / 2.f, by + bh + 42.f,
            sf::Uint8(W * .034f), sf::Color(0, 170, 255, sf::Uint8(barA * 255)));

        const char* msgs[] = { "LOADING STADIUM...","LOADING PLAYERS...",
                             "LOADING PITCH DATA...","LOADING TACTICS...","KICK OFF READY!" };
        int mi = std::min((int)(p * (5 - .01f)), 4);
        drawText(rt, font, msgs[mi], W / 2.f, by + bh + 66.f,
            sf::Uint8(W * .022f), sf::Color(180, 225, 255, sf::Uint8(barA * 180)));

        // Corner HUD
        sf::Color hudc[] = { sf::Color(0,160,255,140),sf::Color(210,45,45,140) };
        for (int hi = 0; hi < 2; ++hi) {
            float cx = (hi == 0) ? 22.f : W - 22.f;
            float dx = (hi == 0) ? 32.f : -32.f;
            sf::VertexArray hud(sf::Lines, 8);
            // top-left corner of bracket
            hud[0] = { {cx,22},hudc[hi] };        hud[1] = { {cx + dx,22},hudc[hi] };
            hud[2] = { {cx,22},hudc[hi] };        hud[3] = { {cx,52},hudc[hi] };
            hud[4] = { {cx,H - 22},hudc[hi] };      hud[5] = { {cx + dx,H - 22},hudc[hi] };
            hud[6] = { {cx,H - 22},hudc[hi] };      hud[7] = { {cx,H - 52},hudc[hi] };
            rt.draw(hud);
        }

        if (p > .88f) {
            float fo = smoothstep((p - .88f) / .12f);
            sf::RectangleShape fade({ W,H });
            fade.setFillColor(sf::Color(255, 255, 255, sf::Uint8(fo * 255))); rt.draw(fade);
        }
    }

    // ── Scene 5 : Menu Reveal ─────────────────────────────────
    static void scene5(sf::RenderTarget& rt, sf::Font& font, float t, float st)
    {
        float p = clamp(st / 2.f, 0.f, 1.f);

        // No menu image in SFML port — draw a stylised FIFA title screen
        // Background
        sf::Color bg0 = sf::Color(5, 12, 35);
        sf::Color bg1 = sf::Color(1, 4, 15);
        drawGradRect(rt, 0, 0, W, H, bg0, bg1);

        // Green pitch portion at bottom
        sf::RectangleShape pitch({ W, H * .35f }); pitch.setPosition(0, H * .65f);
        pitch.setFillColor(sf::Color(12, 72, 22)); rt.draw(pitch);

        // Pitch lines
        for (int ci = 0; ci < 4; ++ci) {
            float rx = (float)(180 - ci * 50); float ry = H * .12f - ci * 10.f;
            sf::VertexArray ring(sf::LinesStrip, 48);
            for (int i = 0; i < 48; ++i) {
                float a = (float)i / 47.f * PI * 2.f;
                ring[i].position = { W / 2.f + std::cos(a) * rx, H * .78f + std::sin(a) * ry };
                ring[i].color = sf::Color(255, 255, 255, 30);
            }
            rt.draw(ring);
        }

        // Floating ambient particles
        {
            ParticleOpts ap;
            ap.cols = { sf::Color(120,190,255,165),sf::Color(255,255,200,115),sf::Color(0,210,255,140) };
            ap.maxS = .7f; ap.minS = .2f; ap.gv = -.004f; ap.vy = -.55f; ap.dec = .005f; ap.maxR = 3.5f; ap.glo = 12.f;
            addP(1, frand() * W, frand() * H * .35f, ap);
        }
        tickP(); drawP(rt);

        float sc = lerp(1.06f, 1.f, smoothstep(p));
        (void)sc; // scale effect would need a RenderTexture; skip for simplicity

        // Title block
        float tA = easeOut3(p);
        drawText(rt, font, "FIFA 2D", W / 2.f, H * .22f, sf::Uint8(W * .09f),
            sf::Color(255, 255, 255, sf::Uint8(tA * 255)));
        drawText(rt, font, "FOOTBALL", W / 2.f, H * .38f, sf::Uint8(W * .11f),
            sf::Color(0, 221, 255, sf::Uint8(tA * 255)));

        // Year tag
        drawText(rt, font, "2025 EDITION", W / 2.f, H * .47f, sf::Uint8(16),
            sf::Color(180, 220, 255, sf::Uint8(tA * 160)));

        // Menu buttons (visual only)
        if (p > .5f) {
            float ba = (p - .5f) / .5f;
            struct Btn { const char* label; float y; sf::Color c; };
            Btn btns[] = {
                {"KICK OFF",     H * .57f, sf::Color(0,160,255)},
                {"TOURNAMENT",   H * .635f,sf::Color(0,200,120)},
                {"SETTINGS",     H * .70f, sf::Color(180,180,180)},
                {"EXIT",         H * .765f,sf::Color(220,60,60)}
            };
            for (auto& b : btns) {
                sf::RectangleShape box({ 320.f,36.f }); box.setPosition(W / 2.f - 160.f, b.y - 18.f);
                box.setFillColor(sf::Color(b.c.r, b.c.g, b.c.b, sf::Uint8(ba * 55)));
                box.setOutlineThickness(1.5f);
                box.setOutlineColor(sf::Color(b.c.r, b.c.g, b.c.b, sf::Uint8(ba * 180)));
                rt.draw(box);
                drawText(rt, font, b.label, W / 2.f, b.y + 2.f, sf::Uint8(18),
                    sf::Color(b.c.r, b.c.g, b.c.b, sf::Uint8(ba * 220)));
            }

            // Button pulse
            float pu = .5f + .5f * std::sin(t * 3.2f);
            sf::RectangleShape puls({ 370.f,40.f }); puls.setPosition(W / 2.f - 185.f, H * .57f - 20.f);
            puls.setFillColor(sf::Color(0, 136, 255, sf::Uint8(ba * pu * 46))); rt.draw(puls);
        }

        // Light streaks on reveal
        if (p < .45f) {
            float sa = (1.f - p / .45f) * .28f;
            for (int i = 0; i < 6; ++i) {
                float sx = (float)i / 6.f * W + std::sin(t + (float)i) * 48.f;
                sf::VertexArray streak(sf::Quads, 4);
                sf::Color sc2 = sf::Color(210, 235, 255, sf::Uint8(sa * 180));
                streak[0] = { {sx - 140.f, 0},  sc2 };
                streak[1] = { {sx - 110.f, 0},  sc2 };
                streak[2] = { {sx + 140.f, H},  sf::Color(210,235,255,0) };
                streak[3] = { {sx + 110.f, H},  sf::Color(210,235,255,0) };
                rt.draw(streak);
            }
        }

        // Vignette
        if (p > .48f) {
            sf::VertexArray vig(sf::TriangleFan, 34);
            vig[0] = { {W / 2.f,H / 2.f}, sf::Color(0,0,0,0) };
            float vig_a = (p - .48f) / .52f * .22f;
            for (int i = 0; i <= 32; ++i) {
                float a = (float)i / 32.f * PI * 2.f;
                vig[i + 1] = { {W / 2.f + std::cos(a) * W * .9f, H / 2.f + std::sin(a) * H * .9f},
                           sf::Color(0,0,0,sf::Uint8(vig_a * 255)) };
            }
            rt.draw(vig);
        }

        // White flash at start
        if (p < .18f) {
            float fo = 1.f - p / .18f;
            sf::RectangleShape flash({ W,H });
            flash.setFillColor(sf::Color(255, 255, 255, sf::Uint8(fo * 255))); rt.draw(flash);
        }
    }



    // ══════════════════════════════════════════════════════════
    //  INTEGRATED RUNTIME — draws all 5 scenes inside an existing SFML window
    // ══════════════════════════════════════════════════════════
    struct Runtime {
        float t = 0.f;
        float ballAngle = 0.f;
        bool impactTrig = false;
        float shakeInt = 0.f;
        float cracksGrow = 0.f;
        bool initialized = false;

        void reset() {
            t = 0.f;
            ballAngle = 0.f;
            impactTrig = false;
            shakeInt = 0.f;
            cracksGrow = 0.f;
            initialized = true;
            initStars();
            PARTS.clear();
            SHOCKS.clear();
            CRACKS.clear();
        }

        bool update(float dt, bool skip) {
            if (!initialized) reset();

            // Skip means go directly to the real project menu.
            // This removes the default built-in intro menu scene.
            if (skip) {
                t = 11.0f;
                impactTrig = true;
                cracksGrow = 1.f;
                shakeInt = 0.f;
                PARTS.clear();
                SHOCKS.clear();
                return true;
            }

            t += std::min(dt, .05f);

            // Stop intro immediately after the loading cutscene.
            // Scene 5 default menu is intentionally not shown.
            return t >= 11.0f;
        }

        void draw(sf::RenderTarget& window, sf::RenderTexture& rt, sf::Font& font) {
            if (!initialized) reset();
            rt.clear(sf::Color::Black);

            if (t < 3.f)
                scene1(rt, font, t, t, ballAngle);
            else if (t < 5.5f)
                scene2(rt, font, t, t - 3.f, ballAngle);
            else if (t < 8.5f)
                scene3(rt, font, t, t - 5.5f, ballAngle, impactTrig, shakeInt, cracksGrow);
            else
                scene4(rt, font, t, std::min(t - 8.5f, 2.49f), ballAngle);

            rt.display();

            sf::Sprite spr(rt.getTexture());
            const sf::Vector2u ws = window.getSize();
            const float scaleX = static_cast<float>(ws.x) / W;
            const float scaleY = static_cast<float>(ws.y) / H;
            const float sc = std::min(scaleX, scaleY);
            spr.setScale(sc, sc);
            spr.setPosition((static_cast<float>(ws.x) - W * sc) / 2.f,
                (static_cast<float>(ws.y) - H * sc) / 2.f);
            window.draw(spr);
        }
    };

} // namespace FifaIntro


enum class GameState { Splash, MainMenu, TeamSelect, Loading, Settings, Playing, Paused, GoalScene, ThrowIn, GameOver };
enum class Theme { Day, Night, Rain };
enum class Side { Left, Right };
enum class Role { GoalKeeper, Defender, Attacker };
enum class RefPose { Run, Yellow, Red, Idle };
enum class Difficulty { Easy, Medium, Hard };
enum class MatchMode { Normal, PenaltyShootout, Tournament };
enum class MenuSlot { Start = 0, TeamSelection = 1, Difficulty = 2, Penalty = 3, Tournament = 4, Exit = 5, DiffEasy = 6, DiffMedium = 7, DiffHard = 8 };

struct TeamDef {
    std::string id;
    std::string displayName;
    sf::Color primary;
    sf::Color secondary;
};

struct Assets {
    sf::Font font;
    bool hasFont = false;
    sf::Texture stadiumDay, stadiumNight, stadiumRain, crowdDay, crowdNight, ball, goalCage;
    sf::Texture uiMenuDay, uiMenuNight, uiMenuRain;
    sf::Texture uiMenuState[3][9]; // [Theme: day/night/rain][MenuSlot]
    std::vector<sf::Texture> teamSelectionScreens[3]; // [0 day, 1 night, 2 rain] assets/ui/team_selection/<weather>/team_01.png ... team_12.png
    sf::Texture loadingScreen[4][4];
    bool hasLoadingScreen[4][4] = {};
    sf::Texture pauseResumeScreen, pauseExitScreen;
    bool hasPauseResumeScreen = false, hasPauseExitScreen = false;
    bool hasStadiumDay = false, hasStadiumNight = false, hasStadiumRain = false, hasCrowdDay = false, hasCrowdNight = false, hasBall = false, hasGoal = false;
    bool hasUiMenuDay = false, hasUiMenuNight = false, hasUiMenuRain = false;
    bool hasUiMenuState[3][9] = {};
    sf::Texture refRun, refYellow, refRed, refIdle;
    bool hasRefRun = false, hasRefYellow = false, hasRefRed = false, hasRefIdle = false;

    struct TeamTex {
        sf::Texture stance, run1, run2, kick, slide, keeper, flag;
        bool hasStance = false, hasRun1 = false, hasRun2 = false, hasKick = false, hasSlide = false, hasKeeper = false, hasFlag = false;
    } team[4];

    // ================= STUDY FORMAT: CLEAN AUDIO SLOTS =================
    // Optional sound files. Add only the files you want; missing files will be ignored silently.
    // Main recommended names:
    //   assets/sounds/menu_music.ogg      -> looped music on main menu
    //   assets/sounds/menu_beep.wav       -> short beep when changing menu buttons
    //   assets/sounds/intro.ogg           -> intro/cutscene sound
    //   assets/sounds/loading.ogg         -> loading-screen sound
    //   assets/sounds/game_music.ogg      -> looped gameplay music
    //   assets/sounds/game_audience.ogg   -> looped crowd during gameplay
    //   assets/sounds/kick.wav            -> kick/pass/shoot effect
    //   assets/sounds/whistle.wav         -> referee whistle/card/penalty effect
    //   assets/sounds/goal_cheer.wav      -> goal/win cheer
    //   assets/sounds/grief.wav           -> penalty/card/near-goal missed reaction

    // Menu audio
    sf::SoundBuffer menuMusicBuf, menuBeepBuf;
    sf::Sound menuMusic, menuBeep;
    bool hasMenuMusic = false, hasMenuBeep = false;

    // Intro and loading audio
    sf::SoundBuffer introBuf, loadingBuf;
    sf::Sound introSound, loadingSound;
    bool hasIntroSound = false, hasLoadingSound = false;

    // Gameplay loop audio
    sf::SoundBuffer gameMusicBuf, gameAudienceBuf;
    sf::Sound gameMusic, gameAudience;
    bool hasGameMusic = false, hasGameAudience = false;

    // Gameplay one-shot effects
    sf::SoundBuffer kickBuf, whistleBuf, goalCheerBuf, griefBuf;
    sf::Sound kick, whistle, goalCheer, grief;
    bool hasKick = false, hasWhistle = false, hasGoalCheer = false, hasGrief = false;

    // Balanced volumes. Music button controls menu + game + intro + loading music.
    // Volume button controls audience + all sound effects.
    float musicVolume = 32.f;
    float volume = 55.f;
    float menuBeepVolume = 42.f;

    bool loadTexture(sf::Texture& t, const std::string& rel) { return t.loadFromFile(ASSET_ROOT + "/" + rel) || t.loadFromFile("assets/" + rel); }
    bool loadFont() { return font.loadFromFile(ASSET_ROOT + "/fonts/menu.ttf") || font.loadFromFile("assets/fonts/menu.ttf") || font.loadFromFile("C:/Windows/Fonts/arial.ttf"); }

    // Study note:
    // SFML prints "Failed to open sound file" when loadFromFile() is called on a missing file.
    // To keep the console clean, we first check whether the optional file exists.
    bool fileExists(const std::string& path) const {
        std::ifstream f(path, std::ios::binary);
        return f.good();
    }

    // Loads one sound silently. Missing sounds are allowed; the game still runs normally.
    bool loadSound(sf::SoundBuffer& b, const std::string& rel) {
        const std::string fullPath = ASSET_ROOT + "/" + rel;
        if (fileExists(fullPath)) return b.loadFromFile(fullPath);

        const std::string localPath = "assets/" + rel;
        if (fileExists(localPath)) return b.loadFromFile(localPath);

        return false;
    }

    // Loads the first available sound from a small list of accepted filenames.
    // No console error will appear if none of the files are present.
    bool loadSoundAny(sf::SoundBuffer& b, const std::vector<std::string>& names) {
        for (const auto& n : names) {
            if (loadSound(b, n)) return true;
        }
        return false;
    }

    void load(const std::vector<TeamDef>& defs) {
        hasFont = loadFont();
        hasStadiumDay = loadTexture(stadiumDay, "sprites/stadium/stadium_day.png");
        hasStadiumNight = loadTexture(stadiumNight, "sprites/stadium/stadium_night.png");
        hasStadiumRain = loadTexture(stadiumRain, "sprites/stadium/stadium_rain.png");
        hasUiMenuDay = loadTexture(uiMenuDay, "ui/menu_day.png");
        hasUiMenuNight = loadTexture(uiMenuNight, "ui/menu_night.png");
        hasUiMenuRain = loadTexture(uiMenuRain, "ui/menu_rain.png");

        // Full PNG menu images with built-in glow/highlight.
        // Rename your 27 menu PNGs exactly like these names.
        const char* weatherNames[3] = { "day", "night", "rain" };
        const char* slotNames[9] = {
            "start", "team", "difficulty", "penalty", "tournament", "exit",
            "diff_easy", "diff_medium", "diff_hard"
        };
        for (int w = 0; w < 3; ++w) {
            for (int m = 0; m < 9; ++m) {
                hasUiMenuState[w][m] = loadTexture(uiMenuState[w][m], std::string("ui/menu_") + weatherNames[w] + "_" + slotNames[m] + ".png");
            }
        }

        // Team-selection full-screen PNG slots divided by selected weather.
        // Required folders:
        //   assets/ui/team_selection/day/team_01.png   ... team_12.png
        //   assets/ui/team_selection/night/team_01.png ... team_12.png
        //   assets/ui/team_selection/rain/team_01.png  ... team_12.png
        // Behavior: if user selects DAY/NIGHT/RAIN in main menu, only that weather folder is shown.
        for (int w = 0; w < 3; ++w) {
            teamSelectionScreens[w].clear();
            for (int i = 1; i <= 12; ++i) {
                std::ostringstream name1;
                name1 << "ui/team_selection/" << weatherNames[w] << "/team_"
                    << std::setw(2) << std::setfill('0') << i << ".png";
                std::ostringstream name2;
                name2 << "team_selection/" << weatherNames[w] << "/team_"
                    << std::setw(2) << std::setfill('0') << i << ".png";
                sf::Texture tex;
                if (loadTexture(tex, name1.str()) || loadTexture(tex, name2.str())) {
                    teamSelectionScreens[w].push_back(std::move(tex));
                }
            }
        }

        // Backward compatibility: if split folders are missing, load old flat team_01..team_36 files
        // and divide them automatically into 12 DAY, 12 NIGHT, 12 RAIN.
        if (teamSelectionScreens[0].empty() && teamSelectionScreens[1].empty() && teamSelectionScreens[2].empty()) {
            for (int i = 1; i <= 36; ++i) {
                std::ostringstream name;
                name << "ui/team_selection/team_" << std::setw(2) << std::setfill('0') << i << ".png";
                sf::Texture tex;
                if (loadTexture(tex, name.str())) {
                    int w = (i - 1) / 12;
                    if (w >= 0 && w < 3) teamSelectionScreens[w].push_back(std::move(tex));
                }
            }
        }
        // Loading screens for exact selected team pair. Required names are normalized here:
        // assets/ui/loading/barcelona_vs_france.png, france_vs_barcelona.png, etc.
        const char* teamIds[4] = { "barcelona", "france", "brazil", "germany" };
        for (int a = 0; a < 4; ++a) {
            for (int b = 0; b < 4; ++b) {
                if (a == b) continue;
                std::string rel = std::string("ui/loading/") + teamIds[a] + "_vs_" + teamIds[b] + ".png";
                hasLoadingScreen[a][b] = loadTexture(loadingScreen[a][b], rel);
            }
        }
        hasPauseResumeScreen = loadTexture(pauseResumeScreen, "ui/pause_resume.png");
        hasPauseExitScreen = loadTexture(pauseExitScreen, "ui/pause_exit.png");

        hasCrowdDay = loadTexture(crowdDay, "sprites/stadium/crowd_full_day.png");
        hasCrowdNight = loadTexture(crowdNight, "sprites/stadium/crowd_full_night.png");
        hasBall = loadTexture(ball, "sprites/equipment/ball.png");
        hasGoal = loadTexture(goalCage, "sprites/equipment/goal_cage.png");
        hasRefRun = loadTexture(refRun, "sprites/referee/referee_run1.png");
        hasRefYellow = loadTexture(refYellow, "sprites/referee/referee_yellow.png");
        hasRefRed = loadTexture(refRed, "sprites/referee/referee_red.png");
        hasRefIdle = loadTexture(refIdle, "sprites/referee/referee_idle.png");

        for (std::size_t i = 0; i < defs.size() && i < 4; ++i) {
            const std::string base = "sprites/teams/" + defs[i].id + "_";
            team[i].hasStance = loadTexture(team[i].stance, base + "stance.png");
            team[i].hasRun1 = loadTexture(team[i].run1, base + "run1.png");
            team[i].hasRun2 = loadTexture(team[i].run2, base + "run2.png");
            team[i].hasKick = loadTexture(team[i].kick, base + "kick.png");
            team[i].hasSlide = loadTexture(team[i].slide, base + "slide.png");
            team[i].hasFlag = loadTexture(team[i].flag, base + "flag.png");
            team[i].hasKeeper = loadTexture(team[i].keeper, "sprites/goalkeeper/" + defs[i].id + "_keeper.png");
        }

        // ================= CLEAN AUDIO LOADING =================
        // Menu audio
        hasMenuMusic = loadSoundAny(menuMusicBuf, { "sounds/menu_music.ogg", "sounds/menu.ogg", "sounds/menu_music.wav" });
        if (hasMenuMusic) { menuMusic.setBuffer(menuMusicBuf); menuMusic.setLoop(true); menuMusic.setVolume(musicVolume); }

        hasMenuBeep = loadSoundAny(menuBeepBuf, { "sounds/menu_beep.wav", "sounds/beep.wav", "sounds/click.wav", "sounds/menu_beep.ogg" });
        if (hasMenuBeep) { menuBeep.setBuffer(menuBeepBuf); menuBeep.setVolume(menuBeepVolume); }

        // Intro/cutscene sound and match-loading sound
        hasIntroSound = loadSoundAny(introBuf, { "sounds/intro.ogg", "sounds/intro_music.ogg", "sounds/intro.wav" });
        if (hasIntroSound) { introSound.setBuffer(introBuf); introSound.setLoop(false); introSound.setVolume(musicVolume); }

        hasLoadingSound = loadSoundAny(loadingBuf, { "sounds/loading.ogg", "sounds/loading_music.ogg", "sounds/loading.wav" });
        if (hasLoadingSound) { loadingSound.setBuffer(loadingBuf); loadingSound.setLoop(true); loadingSound.setVolume(musicVolume); }

        // Gameplay loops
        hasGameMusic = loadSoundAny(gameMusicBuf, { "sounds/game_music.ogg", "sounds/bgm.ogg", "sounds/background.ogg", "sounds/game_music.wav" });
        if (hasGameMusic) { gameMusic.setBuffer(gameMusicBuf); gameMusic.setLoop(true); gameMusic.setVolume(musicVolume); }

        hasGameAudience = loadSoundAny(gameAudienceBuf, { "sounds/game_audience.ogg", "sounds/audience.ogg", "sounds/audience_loop.ogg", "sounds/crowd.ogg" });
        if (hasGameAudience) { gameAudience.setBuffer(gameAudienceBuf); gameAudience.setLoop(true); gameAudience.setVolume(volume * 0.55f); }

        // Gameplay effects
        hasKick = loadSoundAny(kickBuf, { "sounds/kick.wav", "sounds/kick.ogg" });
        if (hasKick) { kick.setBuffer(kickBuf); kick.setVolume(volume * 0.70f); }

        hasWhistle = loadSoundAny(whistleBuf, { "sounds/whistle.wav", "sounds/referee_whistle.wav", "sounds/whistle.ogg" });
        if (hasWhistle) { whistle.setBuffer(whistleBuf); whistle.setVolume(volume * 0.75f); }

        hasGoalCheer = loadSoundAny(goalCheerBuf, { "sounds/goal_cheer.wav", "sounds/goal_cheer.ogg", "sounds/cheer.wav", "sounds/cheer.ogg" });
        if (hasGoalCheer) { goalCheer.setBuffer(goalCheerBuf); goalCheer.setVolume(volume * 0.85f); }

        hasGrief = loadSoundAny(griefBuf, { "sounds/grief.wav", "sounds/grief.ogg", "sounds/crowd_grief.wav", "sounds/crowd_grief.ogg" });
        if (hasGrief) { grief.setBuffer(griefBuf); grief.setVolume(volume * 0.70f); }
    }

    void setMusicVolume(float v) {
        musicVolume = U::clamp(v, 0.f, 100.f);
        if (hasMenuMusic) menuMusic.setVolume(musicVolume);
        if (hasIntroSound) introSound.setVolume(musicVolume);
        if (hasLoadingSound) loadingSound.setVolume(musicVolume);
        if (hasGameMusic) gameMusic.setVolume(musicVolume);
    }

    void setVolume(float v) {
        volume = U::clamp(v, 0.f, 100.f);
        if (hasGameAudience) gameAudience.setVolume(volume * 0.55f);
        if (hasKick) kick.setVolume(volume * 0.70f);
        if (hasWhistle) whistle.setVolume(volume * 0.75f);
        if (hasGoalCheer) goalCheer.setVolume(volume * 0.85f);
        if (hasGrief) grief.setVolume(volume * 0.70f);
        if (hasMenuBeep) menuBeep.setVolume(std::min(55.f, volume * 0.75f));
    }

    void stopAllLoopsExceptIntro() {
        if (hasMenuMusic) menuMusic.stop();
        if (hasLoadingSound) loadingSound.stop();
        if (hasGameMusic) gameMusic.stop();
        if (hasGameAudience) gameAudience.stop();
    }

    void startIntroAudio() {
        stopAllLoopsExceptIntro();
        if (hasIntroSound && introSound.getStatus() != sf::Sound::Playing) introSound.play();
    }

    void startMenuAudio() {
        if (hasIntroSound && introSound.getStatus() == sf::Sound::Playing) introSound.stop();
        if (hasLoadingSound && loadingSound.getStatus() == sf::Sound::Playing) loadingSound.stop();
        if (hasGameMusic && gameMusic.getStatus() == sf::Sound::Playing) gameMusic.stop();
        if (hasGameAudience && gameAudience.getStatus() == sf::Sound::Playing) gameAudience.stop();
        if (hasMenuMusic && menuMusic.getStatus() != sf::Sound::Playing) menuMusic.play();
    }

    void startLoadingAudio() {
        if (hasIntroSound && introSound.getStatus() == sf::Sound::Playing) introSound.stop();
        if (hasMenuMusic && menuMusic.getStatus() == sf::Sound::Playing) menuMusic.stop();
        if (hasGameMusic && gameMusic.getStatus() == sf::Sound::Playing) gameMusic.stop();
        if (hasGameAudience && gameAudience.getStatus() == sf::Sound::Playing) gameAudience.stop();
        if (hasLoadingSound && loadingSound.getStatus() != sf::Sound::Playing) loadingSound.play();
    }

    void startMatchAudio() {
        if (hasIntroSound && introSound.getStatus() == sf::Sound::Playing) introSound.stop();
        if (hasMenuMusic && menuMusic.getStatus() == sf::Sound::Playing) menuMusic.stop();
        if (hasLoadingSound && loadingSound.getStatus() == sf::Sound::Playing) loadingSound.stop();
        if (hasGameMusic && gameMusic.getStatus() != sf::Sound::Playing) gameMusic.play();
        if (hasGameAudience && gameAudience.getStatus() != sf::Sound::Playing) gameAudience.play();
    }

    void stopLoopAudio() {
        if (hasMenuMusic) menuMusic.stop();
        if (hasLoadingSound) loadingSound.stop();
        if (hasGameMusic) gameMusic.stop();
        if (hasGameAudience) gameAudience.stop();
    }

    void playKick() { if (hasKick) kick.play(); }
    void playGoalCheer() { if (hasGoalCheer) goalCheer.play(); }
    void playCheer() { playGoalCheer(); }
    void playGrief() { if (hasGrief) grief.play(); }
    void playWhistle() { if (hasWhistle) whistle.play(); }
    void playBeep() { if (hasMenuBeep) menuBeep.play(); }
};

struct InputManager {
    bool pass = false, shoot = false, slide = false, sprint = false, switchP = false, pause = false, confirm = false, back = false, up = false, down = false, left = false, right = false, settings = false;
    bool mouseClick = false;
    sf::Vector2f mousePos{ 0.f, 0.f };
    sf::Vector2f move{ 0.f, 0.f };

    void begin() {
        pass = shoot = slide = switchP = pause = confirm = back = up = down = left = right = settings = mouseClick = false;
        move = { 0.f, 0.f };
    }

    void event(const sf::Event& e) {
        if (e.type == sf::Event::MouseButtonPressed && e.mouseButton.button == sf::Mouse::Left) {
            mouseClick = true;
            mousePos = { static_cast<float>(e.mouseButton.x), static_cast<float>(e.mouseButton.y) };
            return;
        }
        if (e.type != sf::Event::KeyPressed) return;

        const sf::Keyboard::Key k = e.key.code;
        if (k == sf::Keyboard::J) pass = true;
        if (k == sf::Keyboard::K) shoot = true;
        if (k == sf::Keyboard::Space) slide = true;
        if (k == sf::Keyboard::I) switchP = true;
        if (k == sf::Keyboard::P || k == sf::Keyboard::Escape) pause = true;
        if (k == sf::Keyboard::Enter) confirm = true;
        if (k == sf::Keyboard::BackSpace) back = true;
        if (k == sf::Keyboard::Up) up = true;
        if (k == sf::Keyboard::Down) down = true;
        if (k == sf::Keyboard::Left) left = true;
        if (k == sf::Keyboard::Right) right = true;
        if (k == sf::Keyboard::S) settings = true;
    }

    void realtime() {
        sprint = sf::Keyboard::isKeyPressed(sf::Keyboard::L);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) move.y -= 1.f;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) move.y += 1.f;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) move.x -= 1.f;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) move.x += 1.f;
        move = U::norm(move);
    }
};

class Ball {
    sf::Vector2f pos{ FIELD_X + FIELD_W / 2.f, FIELD_Y + FIELD_H / 2.f };
    sf::Vector2f vel{ 0.f, 0.f };
    float touchCooldown = 0.f;
    float spin = 0.f;
public:
    Ball() { reset(Side::Left); }
    sf::Vector2f getPos() const { return pos; }
    sf::Vector2f getVel() const { return vel; }
    void setPos(sf::Vector2f p) { pos = p; }
    void setVel(sf::Vector2f v) { vel = v; }
    void stop() { vel = { 0.f, 0.f }; }

    void reset(Side kickoff) {
        pos = { FIELD_X + FIELD_W / 2.f + (kickoff == Side::Left ? -55.f : 55.f), FIELD_Y + FIELD_H / 2.f };
        vel = { 0.f, 0.f };
        touchCooldown = .35f;
    }

    void kick(sf::Vector2f dir, float power) {
        vel = U::norm(dir) * power;
        touchCooldown = .12f;
    }

    bool canTouch() const { return touchCooldown <= 0.f; }
    void cooldown(float v) { touchCooldown = v; }

    void update(float dt) {
        if (touchCooldown > 0.f) touchCooldown -= dt;
        if (U::len(vel) > 80.f) {
            const float curve = std::sin(spin * 0.05f) * 6.f;
            vel += sf::Vector2f(-vel.y, vel.x) * 0.0008f * curve;
        }
        pos += vel * dt;
        spin += U::len(vel) * dt * .45f;
        vel *= std::pow(.984f, dt * 60.f);
        if (U::len(vel) < 7.f) vel = { 0.f, 0.f };
    }

    void draw(sf::RenderTarget& t, const Assets& a) const {
        if (a.hasBall) {
            sf::Sprite s(a.ball);
            const sf::Vector2u z = a.ball.getSize();
            s.setOrigin(U::f(z.x) / 2.f, U::f(z.y) / 2.f);
            s.setPosition(pos);
            s.setRotation(spin);
            const float sc = (BALL_R * 2.6f) / static_cast<float>(std::max(z.x, z.y));
            s.setScale(sc, sc);
            t.draw(s);
        }
        else {
            sf::CircleShape c(BALL_R);
            c.setOrigin(BALL_R, BALL_R);
            c.setFillColor(sf::Color::White);
            c.setOutlineColor(sf::Color::Black);
            c.setOutlineThickness(2.f);
            c.setPosition(pos);
            t.draw(c);
        }
    }
};

class Player {
    sf::Vector2f pos{ 0.f, 0.f };
    sf::Vector2f home{ 0.f, 0.f };
    sf::Vector2f vel{ 0.f, 0.f };
    Side side = Side::Left;
    Role role = Role::Attacker;
    int teamIndex = 0;
    bool controlled = false;
    bool sentOff = false;
    int yellow = 0;
    float slideTime = 0.f;
    float freezeTime = 0.f;
    float anim = 0.f;
    float cardTime = 0.f;
public:
    Player() = default;
    Player(Side s, Role r, int ti, sf::Vector2f p) : pos(p), home(p), side(s), role(r), teamIndex(ti) {}

    sf::Vector2f getPos() const { return pos; }
    sf::Vector2f getHome() const { return home; }
    Side getSide() const { return side; }
    Role getRole() const { return role; }
    int getTeamIndex() const { return teamIndex; }
    bool isControlled() const { return controlled; }
    bool isSentOff() const { return sentOff; }
    bool isSliding() const { return slideTime > 0.f; }
    bool isKeeper() const { return role == Role::GoalKeeper; }

    void setControlled(bool v) { controlled = v; }
    void setHome(sf::Vector2f p) { home = p; }
    void reset() { pos = home; vel = { 0.f, 0.f }; slideTime = 0.f; freezeTime = 0.f; controlled = false; }
    void freeze(float t) { freezeTime = std::max(freezeTime, t); }
    void giveYellow() { ++yellow; cardTime = 1.f; if (yellow >= 2) sentOff = true; }
    void giveRed() { sentOff = true; cardTime = 1.2f; }

    void startSlide(sf::Vector2f dir) {
        if (sentOff || role == Role::GoalKeeper || slideTime > 0.f) return;
        if (U::len(dir) < .01f) dir = (side == Side::Left ? sf::Vector2f(1.f, 0.f) : sf::Vector2f(-1.f, 0.f));
        vel = U::norm(dir) * 560.f;
        slideTime = .34f;
    }

    void update(float dt, const InputManager& in, const Ball& ball, const std::vector<Player*>& all, bool throwInFreeze = false, bool allowedThrow = false) {
        anim += dt;
        if (cardTime > 0.f) cardTime -= dt;
        if (sentOff) return;
        if (throwInFreeze && !allowedThrow) { vel = { 0.f, 0.f }; return; }
        if (freezeTime > 0.f) { freezeTime -= dt; vel = { 0.f, 0.f }; return; }
        if (slideTime > 0.f) {
            slideTime -= dt;
            vel *= std::pow(.90f, dt * 60.f);
            pos += vel * dt;
            bound();
            return;
        }
        if (controlled) vel = in.move * (in.sprint ? 290.f : 205.f);
        else ai(ball, all);
        pos += vel * dt;
        bound();
    }

    void ai(const Ball& ball, const std::vector<Player*>& all) {
        const sf::Vector2f b = ball.getPos();
        const float d = U::dist(pos, b);
        int closer = 0;
        for (auto p : all) if (p != this && p->side == side && !p->sentOff && U::dist(p->pos, b) < d) ++closer;
        sf::Vector2f target = home;
        const float half = FIELD_X + FIELD_W / 2.f;
        const bool dangerous = (side == Side::Left ? b.x < FIELD_X + FIELD_W * .45f : b.x > FIELD_X + FIELD_W * .55f);

        if (role == Role::GoalKeeper) {
            const float gx = (side == Side::Left ? FIELD_X + 38.f : FIELD_X + FIELD_W - 38.f);
            target = { gx, U::clamp(b.y, FIELD_Y + FIELD_H / 2.f - GOAL_H * .46f, FIELD_Y + FIELD_H / 2.f + GOAL_H * .46f) };
            if (d < 135.f) target = b;
        }
        else if (role == Role::Defender) {
            target = { home.x, U::clamp(b.y, home.y - 150.f, home.y + 150.f) };
            if (dangerous && d < 380.f && closer < 2) target = b;
            target.x = U::clamp(target.x, home.x - 80.f, home.x + 110.f);
        }
        else {
            target = { home.x + (side == Side::Left ? 80.f : -80.f), home.y + std::sin(b.x * .01f + home.y * .03f) * 55.f };
            if ((side == Side::Left && b.x > half - 160.f) || (side == Side::Right && b.x < half + 160.f) || closer == 0) target = b;
        }

        if (d < 70.f) {
            const bool ownDanger = side == Side::Left ? b.x < FIELD_X + 250.f : b.x > FIELD_X + FIELD_W - 250.f;
            if (ownDanger) target = { b.x + (side == Side::Left ? 90.f : -90.f), b.y };
        }

        float speed = (role == Role::GoalKeeper ? 175.f : 165.f);
        if (role == Role::Attacker) speed = 178.f;
        vel = U::norm(target - pos) * speed;

        // Keep AI players from making a crowd around the ball.
        // Only the closest few players chase; others keep spacing like real formation.
        sf::Vector2f separation(0.f, 0.f);
        int veryCloseTeamMates = 0;
        for (auto p : all) {
            if (p == this || p->sentOff) continue;
            float d2 = U::dist(pos, p->pos);
            if (d2 < 62.f && d2 > 0.1f) {
                separation += U::norm(pos - p->pos) * (62.f - d2);
                if (p->side == side) ++veryCloseTeamMates;
            }
        }
        if (veryCloseTeamMates > 0) vel += separation * 5.5f;
        if (closer > 2 && role != Role::GoalKeeper) vel += U::norm(home - pos) * 95.f;

        if (U::len(vel) > speed) vel = U::norm(vel) * speed;
        if (U::dist(pos, target) < 12.f) vel = { 0.f, 0.f };
    }

    void bound() {
        sf::FloatRect r(FIELD_X, FIELD_Y, FIELD_W, FIELD_H);
        if (role == Role::Defender) {
            if (side == Side::Left) r.width = FIELD_W * .54f;
            else { r.left = FIELD_X + FIELD_W * .46f; r.width = FIELD_W * .54f; }
        }
        pos = U::clamp(pos, r, PLAYER_R);
    }

    void draw(sf::RenderTarget& t, const Assets& a, bool selected = false) const {
        if (sentOff) return;
        if (selected) {
            sf::CircleShape ring(PLAYER_R + 9.f);
            ring.setOrigin(PLAYER_R + 9.f, PLAYER_R + 9.f);
            ring.setPosition(pos);
            ring.setFillColor(sf::Color::Transparent);
            ring.setOutlineColor(sf::Color(255, 225, 40));
            ring.setOutlineThickness(4.f);
            t.draw(ring);
        }

        const Assets::TeamTex& tex = a.team[teamIndex];
        const sf::Texture* use = nullptr;
        bool has = false;
        if (role == Role::GoalKeeper && tex.hasKeeper) { use = &tex.keeper; has = true; }
        else if (slideTime > 0.f && tex.hasSlide) { use = &tex.slide; has = true; }
        else if (U::len(vel) > 10.f) {
            if ((static_cast<int>(anim * 8.f) % 2) == 0 && tex.hasRun1) { use = &tex.run1; has = true; }
            else if (tex.hasRun2) { use = &tex.run2; has = true; }
        }
        if (!has && tex.hasStance) { use = &tex.stance; has = true; }

        if (has) {
            sf::Sprite s(*use);
            const sf::Vector2u z = use->getSize();
            s.setOrigin(U::f(z.x) / 2.f, U::f(z.y) * .62f);
            s.setPosition(pos);
            const float sc = role == Role::GoalKeeper ? .58f : .54f;
            if (side == Side::Right) s.setScale(-sc, sc);
            else s.setScale(sc, sc);
            t.draw(s);
        }
        else {
            sf::CircleShape c(PLAYER_R);
            c.setOrigin(PLAYER_R, PLAYER_R);
            c.setPosition(pos);
            c.setFillColor(side == Side::Left ? sf::Color::Blue : sf::Color::Red);
            t.draw(c);
        }
    }
};

class Team {
    Side side = Side::Left;
    int teamIndex = 0;
    std::string name = "Team";
    std::vector<Player> players;
public:
    Team() = default;
    Team(Side s, int ti, const std::string& n) : side(s), teamIndex(ti), name(n) { createFormation(); }

    Side getSide() const { return side; }
    int getTeamIndex() const { return teamIndex; }
    std::string getName() const { return name; }

    void createFormation() {
        players.clear();
        const float cx = (side == Side::Left ? FIELD_X + 220.f : FIELD_X + FIELD_W - 220.f);
        const float dir = (side == Side::Left ? 1.f : -1.f);
        const float mid = FIELD_Y + FIELD_H / 2.f;
        players.emplace_back(side, Role::GoalKeeper, teamIndex, sf::Vector2f(side == Side::Left ? FIELD_X + 70.f : FIELD_X + FIELD_W - 70.f, mid));
        players.emplace_back(side, Role::Defender, teamIndex, sf::Vector2f(cx, mid - 210.f));
        players.emplace_back(side, Role::Defender, teamIndex, sf::Vector2f(cx, mid));
        players.emplace_back(side, Role::Defender, teamIndex, sf::Vector2f(cx, mid + 210.f));
        players.emplace_back(side, Role::Attacker, teamIndex, sf::Vector2f(cx + dir * 470.f, mid - 230.f));
        players.emplace_back(side, Role::Attacker, teamIndex, sf::Vector2f(cx + dir * 560.f, mid));
        players.emplace_back(side, Role::Attacker, teamIndex, sf::Vector2f(cx + dir * 470.f, mid + 230.f));
    }

    std::vector<Player>& get() { return players; }
    const std::vector<Player>& get() const { return players; }
    void reset() { createFormation(); }
};

class Referee {
    sf::Vector2f pos{ FIELD_X + FIELD_W / 2.f, FIELD_Y + FIELD_H / 2.f + 80.f };
    RefPose pose = RefPose::Run;
    float cardTimer = 0.f;
public:
    void showCard(bool isRed) { pose = isRed ? RefPose::Red : RefPose::Yellow; cardTimer = 1.6f; }
    void update(float dt, sf::Vector2f ball) {
        if (cardTimer > 0.f) { cardTimer -= dt; if (cardTimer <= 0.f) pose = RefPose::Run; }
        else pose = RefPose::Run;
        const sf::Vector2f target = ball + sf::Vector2f(0.f, 85.f);
        pos += U::norm(target - pos) * 110.f * dt;
        pos = U::clamp(pos, sf::FloatRect(FIELD_X, FIELD_Y, FIELD_W, FIELD_H), 20.f);
    }
    void draw(sf::RenderTarget& t, const Assets& a) {
        const sf::Texture* tex = nullptr;
        bool has = false;
        if (pose == RefPose::Yellow && a.hasRefYellow) { tex = &a.refYellow; has = true; }
        else if (pose == RefPose::Red && a.hasRefRed) { tex = &a.refRed; has = true; }
        else if (a.hasRefRun) { tex = &a.refRun; has = true; }
        else if (a.hasRefIdle) { tex = &a.refIdle; has = true; }
        if (has) {
            sf::Sprite s(*tex);
            const sf::Vector2u z = tex->getSize();
            s.setOrigin(U::f(z.x) / 2.f, U::f(z.y) * .62f);
            s.setPosition(pos);
            s.setScale(.48f, .48f);
            t.draw(s);
        }
        else {
            sf::CircleShape c(15.f);
            c.setOrigin(15.f, 15.f);
            c.setPosition(pos);
            c.setFillColor(sf::Color::Black);
            t.draw(c);
        }
    }
};

class MatchTimer {
    float sim = 0.f;
    bool finished = false;
public:
    void reset() { sim = 0.f; finished = false; }
    void update(float dt) { if (!finished) { sim += dt * 18.f; if (sim >= 90.f * 60.f) { sim = 90.f * 60.f; finished = true; } } }
    bool done() const { return finished; }
    std::string str() const {
        const int m = static_cast<int>(sim) / 60;
        const int s = static_cast<int>(sim) % 60;
        std::ostringstream o;
        o << std::setw(2) << std::setfill('0') << m << ":" << std::setw(2) << s;
        return o.str();
    }
};

struct TeamPick {
    int userTeam = 0;
    int rivalTeam = 1;
};

class Game {
    sf::RenderWindow window;
    sf::View worldView, uiView;
    std::vector<TeamDef> defs;
    Assets assets;
    InputManager input;
    Ball ball;
    Team left, right;
    Referee referee;
    MatchTimer timer;
    GameState state = GameState::Splash;
    Theme theme = Theme::Day;
    Difficulty difficulty = Difficulty::Medium;
    MatchMode matchMode = MatchMode::Normal;
    int selectedMenu = 0; // 0 Start, 1 Team, 2 Difficulty, 3 Match Mode(Penalty/Tournament), 4 Exit
    int selectedDifficultyMenu = 1; // 0 Easy, 1 Medium, 2 Hard
    bool difficultyDropdownOpen = false;
    int selectedPause = 0;
    int selectedSettings = 0;
    float brightness = 1.0f;
    int selectedUserTeam = 0;
    int selectedRivalTeam = 1;
    int currentTeamScreen = 0; // selected team-selection PNG index
    int controlled = 4;
    int leftScore = 0;
    int rightScore = 0;
    int penaltyShots = 0;
    int userPenGoals = 0;
    int aiPenGoals = 0;
    std::string commentary = "Welcome to FIFA 2D.";
    float commentaryHold = 0.f;       // specific commentary line stays visible for 1-2 seconds
    float commentaryAutoTimer = 0.f;  // creates a new live commentary line every few seconds
    float splashTime = 0.f;
    float loadingTime = 0.f;
    float goalSceneTime = 0.f;
    float throwTimer = 0.f;
    float replayTime = 0.f;
    float winAnim = 0.f;
    float griefCooldown = 0.f; // prevents repeated grief sound on goalkeeper clearances
    Side kickoff = Side::Left;
    Side lastTouched = Side::Right;
    Side throwSide = Side::Left;
    FifaIntro::Runtime introRuntime;
    sf::RenderTexture introRenderTexture;

public:
    Game() {
        defs = {
            {"barcelona", "Barcelona", sf::Color(40, 70, 170), sf::Color(155, 20, 55)},
            {"france", "France", sf::Color(30, 85, 190), sf::Color::White},
            {"brazil", "Brazil", sf::Color(245, 210, 35), sf::Color(20, 135, 70)},
            {"germany", "Germany", sf::Color::White, sf::Color::Black}
        };
        const sf::VideoMode mode = sf::VideoMode::getDesktopMode();
        window.create(mode, "FIFA developed by BU droppers", sf::Style::Fullscreen);
        window.setFramerateLimit(60);
        worldView.setSize(STADIUM_W, STADIUM_H);
        worldView.setCenter(STADIUM_W / 2.f, STADIUM_H / 2.f);
        uiView.setSize(U::f(mode.width), U::f(mode.height));
        uiView.setCenter(U::f(mode.width) / 2.f, U::f(mode.height) / 2.f);
        assets.load(defs);
        introRenderTexture.create(static_cast<unsigned int>(FifaIntro::W), static_cast<unsigned int>(FifaIntro::H));
        introRuntime.reset();
        state = GameState::Splash;
        assets.startIntroAudio();
    }

    void run() {
        sf::Clock clock;
        while (window.isOpen()) {
            input.begin();
            sf::Event e;
            while (window.pollEvent(e)) {
                if (e.type == sf::Event::Closed) window.close();
                input.event(e);
            }
            input.realtime();
            const float dt = std::min(clock.restart().asSeconds(), .035f);
            update(dt);
            render();
        }
    }

private:
    void updateAudioForState() {
        if (state == GameState::MainMenu || state == GameState::TeamSelect || state == GameState::Settings) {
            assets.startMenuAudio();
        }
        else if (state == GameState::Loading) {
            assets.startLoadingAudio();
        }
        else if (state == GameState::Playing || state == GameState::Paused ||
            state == GameState::GoalScene || state == GameState::ThrowIn || state == GameState::GameOver) {
            assets.startMatchAudio();
        }
    }

    // Maps your 12 team-selection PNG screens to the actual teams used in the match.
    // Team order in this code: 0=Barcelona, 1=France, 2=Brazil, 3=Germany.
    // Your image order:
    // 01 Germany vs France, 02 Germany vs Barcelona, 03 Germany vs Brazil,
    // 04 Barcelona vs France, 05 Barcelona vs Germany, 06 Barcelona vs Brazil,
    // 07 Brazil vs Barcelona, 08 Brazil vs France, 09 Brazil vs Germany,
    // 10 France vs Barcelona, 11 France vs Germany, 12 France vs Brazil.
    TeamPick teamPickFromScreen(int screenIndex) const {
        static const TeamPick picks[12] = {
            {3,1}, {3,0}, {3,2},
            {0,1}, {0,3}, {0,2},
            {2,0}, {2,1}, {2,3},
            {1,0}, {1,3}, {1,2}
        };
        int i = screenIndex % 12;
        if (i < 0) i += 12;
        return picks[i];
    }

    int screenIndexForTeams(int userTeam, int rivalTeam) const {
        for (int i = 0; i < 12; ++i) {
            TeamPick p = teamPickFromScreen(i);
            if (p.userTeam == userTeam && p.rivalTeam == rivalTeam) return i;
        }
        return 0;
    }

    void startMatch() {
        if (selectedRivalTeam == selectedUserTeam) selectedRivalTeam = (selectedUserTeam + 1) % 4;
        left = Team(Side::Left, selectedUserTeam, defs[selectedUserTeam].displayName);
        right = Team(Side::Right, selectedRivalTeam, defs[selectedRivalTeam].displayName);
        ball.reset(kickoff);
        leftScore = rightScore = 0;
        timer.reset();
        controlled = 4;
        setControlled();
        penaltyShots = 0;
        userPenGoals = 0;
        aiPenGoals = 0;
        setCommentary("Kick off! Keep possession and attack safely.", 2.0f);
        if (matchMode == MatchMode::PenaltyShootout) setupPenaltyRound(true);
        state = GameState::Loading;
        loadingTime = 0.f;
    }

    void cycleTheme(int dir) {
        int t = static_cast<int>(theme) + dir;
        if (t < 0) t = 2;
        if (t > 2) t = 0;
        theme = static_cast<Theme>(t);
    }

    int hitMainMenu(sf::Vector2f p) const {
        const sf::Vector2u sz = window.getSize();
        const float W = U::f(sz.x), H = U::f(sz.y);
        struct R { float x1, y1, x2, y2; };
        // These hotspots are normalized and work with stretched full-screen menu PNGs.
        // 0=start, 1=team, 2=difficulty, 3=mode(penalty/tournament), 4=exit
        const R r[5] = {
            { .318f,.548f,.690f,.620f },
            { .318f,.625f,.690f,.695f },
            { .318f,.700f,.690f,.770f },
            { .318f,.775f,.690f,.845f },
            { .318f,.850f,.690f,.920f }
        };
        for (int i = 0; i < 5; ++i) if (p.x >= r[i].x1 * W && p.x <= r[i].x2 * W && p.y >= r[i].y1 * H && p.y <= r[i].y2 * H) return i;
        return -1;
    }

    int hitWeather(sf::Vector2f p) const {
        const sf::Vector2u sz = window.getSize();
        const float W = U::f(sz.x), H = U::f(sz.y);
        struct R { float x1, y1, x2, y2; };
        const R r[4] = { {.030f,.545f,.190f,.615f},{.030f,.620f,.190f,.690f},{.030f,.695f,.190f,.765f},{.030f,.770f,.190f,.840f} };
        for (int i = 0; i < 4; ++i)
            if (p.x >= r[i].x1 * W && p.x <= r[i].x2 * W && p.y >= r[i].y1 * H && p.y <= r[i].y2 * H) return i;
        return -1;
    }

    void setControlled() {
        for (auto& p : left.get()) p.setControlled(false);
        if (controlled >= 0 && controlled < static_cast<int>(left.get().size())) left.get()[static_cast<std::size_t>(controlled)].setControlled(true);
    }

    std::vector<Player*> allPlayers() {
        std::vector<Player*> a;
        for (auto& p : left.get()) a.push_back(&p);
        for (auto& p : right.get()) a.push_back(&p);
        return a;
    }

    void update(float dt) {
        if (state == GameState::Splash) {
            splashTime += dt;
            if (introRuntime.update(dt, input.confirm || input.pause)) {
                state = GameState::MainMenu;
                assets.startMenuAudio();
            }
            return;
        }

        // Keep looped sounds appropriate for the current screen.
        updateAudioForState();

        if (state == GameState::MainMenu) {
            // Difficulty dropdown: use LEFT/RIGHT only so it never conflicts with UP/DOWN menu navigation.
            if (difficultyDropdownOpen) {
                if (input.left || input.right) { selectedDifficultyMenu = (selectedDifficultyMenu + (input.right ? 1 : 2)) % 3; assets.playBeep(); }
                if (input.back || input.pause) difficultyDropdownOpen = false;
                if (input.confirm) {
                    difficulty = selectedDifficultyMenu == 0 ? Difficulty::Easy : (selectedDifficultyMenu == 1 ? Difficulty::Medium : Difficulty::Hard);
                    difficultyDropdownOpen = false;
                    setCommentary("Difficulty set to " + diffName() + ".", 1.5f);
                }
                return;
            }

            if (input.settings) { state = GameState::Settings; return; }

            // UP/DOWN only changes the main row.
            // Rows are: Start, Team Selection, Difficulty, Match Mode, Exit.
            // Match Mode itself is changed only with LEFT/RIGHT.
            if (input.up) { selectedMenu = (selectedMenu + 4) % 5; assets.playBeep(); }
            if (input.down) { selectedMenu = (selectedMenu + 1) % 5; assets.playBeep(); }

            // LEFT/RIGHT changes the option of the selected row.
            if (input.left || input.right) {
                assets.playBeep();
                if (selectedMenu == 2) {
                    selectedDifficultyMenu = difficulty == Difficulty::Easy ? 0 : (difficulty == Difficulty::Medium ? 1 : 2);
                    selectedDifficultyMenu = (selectedDifficultyMenu + (input.right ? 1 : 2)) % 3;
                    difficulty = selectedDifficultyMenu == 0 ? Difficulty::Easy : (selectedDifficultyMenu == 1 ? Difficulty::Medium : Difficulty::Hard);
                    difficultyDropdownOpen = true;
                }
                else if (selectedMenu == 3) {
                    // Toggle PENALTY SHOOTOUT / TOURNAMENT using LEFT/RIGHT only.
                    matchMode = (matchMode == MatchMode::PenaltyShootout) ? MatchMode::Tournament : MatchMode::PenaltyShootout;
                }
                else {
                    cycleTheme(input.right ? 1 : -1);
                }
            }

            if (input.mouseClick) {
                const int hit = hitMainMenu(input.mousePos);
                if (hit >= 0) { selectedMenu = hit; input.confirm = true; }
                else {
                    const int wh = hitWeather(input.mousePos);
                    if (wh == 0) theme = Theme::Day;
                    else if (wh == 1) theme = Theme::Night;
                    else if (wh == 2) theme = Theme::Rain;
                    else if (wh == 3) cycleTheme(1);
                }
            }
            if (input.confirm) {
                if (selectedMenu == 0) {
                    // Start using the currently selected match mode.
                    if (matchMode != MatchMode::PenaltyShootout && matchMode != MatchMode::Tournament)
                        matchMode = MatchMode::Normal;
                    startMatch();
                }
                else if (selectedMenu == 1) {
                    // Open full-screen PNG team-selection browser.
                    currentTeamScreen = screenIndexForTeams(selectedUserTeam, selectedRivalTeam);
                    state = GameState::TeamSelect;
                }
                else if (selectedMenu == 2) {
                    selectedDifficultyMenu = difficulty == Difficulty::Easy ? 0 : (difficulty == Difficulty::Medium ? 1 : 2);
                    difficultyDropdownOpen = true;
                }
                else if (selectedMenu == 3) {
                    // Confirm the currently shown mode and start.
                    startMatch();
                }
                else if (selectedMenu == 4) window.close();
            }
            return;
        }

        if (state == GameState::Settings) {
            if (input.up) { selectedSettings = (selectedSettings + 2) % 3; assets.playBeep(); }
            if (input.down) { selectedSettings = (selectedSettings + 1) % 3; assets.playBeep(); }

            if (input.left) {
                assets.playBeep();
                if (selectedSettings == 0) brightness = std::max(0.55f, brightness - 0.05f);
                else if (selectedSettings == 1) assets.setMusicVolume(assets.musicVolume - 5.f);
                else if (selectedSettings == 2) assets.setVolume(assets.volume - 5.f);
            }
            if (input.right) {
                assets.playBeep();
                if (selectedSettings == 0) brightness = std::min(1.35f, brightness + 0.05f);
                else if (selectedSettings == 1) assets.setMusicVolume(assets.musicVolume + 5.f);
                else if (selectedSettings == 2) assets.setVolume(assets.volume + 5.f);
            }

            if (input.confirm || input.pause || input.back) state = GameState::MainMenu;
            return;
        }

        if (state == GameState::TeamSelect) {
            const int w = currentWeatherIndex();
            const int count = assets.teamSelectionScreens[w].empty() ? 4 : static_cast<int>(assets.teamSelectionScreens[w].size());
            if (input.left) { currentTeamScreen = (currentTeamScreen + count - 1) % count; assets.playBeep(); }
            if (input.right) { currentTeamScreen = (currentTeamScreen + 1) % count; assets.playBeep(); }
            // IMPORTANT: UP/DOWN is disabled on team selection. Only LEFT/RIGHT changes team image.
            if (input.confirm) {
                TeamPick pick = teamPickFromScreen(currentTeamScreen);
                selectedUserTeam = pick.userTeam;
                selectedRivalTeam = pick.rivalTeam;
                state = GameState::MainMenu;
                setCommentary(defs[selectedUserTeam].displayName + " selected against " + defs[selectedRivalTeam].displayName + ". Press START GAME to begin.", 2.0f);
            }
            if (input.pause || input.back) state = GameState::MainMenu;
            return;
        }

        if (state == GameState::Loading) {
            loadingTime += dt;
            // Cinematic loading duration. Press ENTER to skip.
            if (loadingTime > 7.0f || input.confirm) state = GameState::Playing;
            return;
        }

        if (state == GameState::Paused) {
            if (input.up || input.down) { selectedPause = 1 - selectedPause; assets.playBeep(); }
            if (input.pause) { state = GameState::Playing; return; }
            if (input.confirm) { if (selectedPause == 0) state = GameState::Playing; else state = GameState::MainMenu; }
            return;
        }

        if (state == GameState::GameOver) {
            winAnim += dt;
            if (input.confirm) { leftScore = rightScore = 0; kickoff = Side::Left; startMatch(); }
            if (input.pause) state = GameState::MainMenu;
            return;
        }

        if (state == GameState::GoalScene) {
            goalSceneTime -= dt;
            referee.update(dt, ball.getPos());
            if (goalSceneTime <= 0.f) { resetRound(); state = GameState::Playing; }
            return;
        }

        if (state == GameState::ThrowIn) {
            throwTimer -= dt;
            updatePlaying(dt, true);
            if (throwTimer <= 0.f) state = GameState::Playing;
            return;
        }

        if (input.pause) { state = GameState::Paused; return; }
        updatePlaying(dt, false);
    }

    void updatePlaying(float dt, bool throwFreeze) {
        if (griefCooldown > 0.f) griefCooldown -= dt;
        if (matchMode == MatchMode::PenaltyShootout) { updatePenaltyMode(dt); return; }
        timer.update(dt);
        if (timer.done()) { state = GameState::GameOver; return; }

        auto all = allPlayers();
        if (input.switchP && !throwFreeze) {
            controlled = (controlled + 1) % static_cast<int>(left.get().size());
            if (left.get()[static_cast<std::size_t>(controlled)].isKeeper()) controlled = (controlled + 1) % static_cast<int>(left.get().size());
            setControlled();
        }
        if (input.slide && !throwFreeze) left.get()[static_cast<std::size_t>(controlled)].startSlide(input.move);

        for (auto& p : left.get()) p.update(dt, input, ball, all, throwFreeze, throwSide == Side::Left && p.isControlled());
        for (auto& p : right.get()) p.update(dt, input, ball, all, throwFreeze, throwSide == Side::Right);
        ball.update(dt);
        handleBallTouches();
        handleRules();
        referee.update(dt, ball.getPos());
        if (input.pass) controlledPass();
        if (input.shoot) controlledShoot();
        updateCommentary(dt);
    }

    void handleBallTouches() {
        auto all = allPlayers();
        Player* closest = nullptr;
        float best = 99999.f;
        for (auto p : all) {
            if (p->isSentOff()) continue;
            const float d = U::dist(p->getPos(), ball.getPos());
            if (d < best) { best = d; closest = p; }
        }

        if (closest && best < PLAYER_R + BALL_R + 5.f && ball.canTouch()) {
            lastTouched = closest->getSide();
            sf::Vector2f out = U::norm(ball.getPos() - closest->getPos());
            if (!closest->isControlled()) {
                const bool ownDanger = closest->getSide() == Side::Left ? ball.getPos().x < FIELD_X + 310.f : ball.getPos().x > FIELD_X + FIELD_W - 310.f;
                if (ownDanger) out = (closest->getSide() == Side::Left ? sf::Vector2f(1.f, 0.15f) : sf::Vector2f(-1.f, -0.15f));
                else if (closest->getRole() == Role::Attacker) out = U::norm(sf::Vector2f(closest->getSide() == Side::Left ? 1.f : -1.f, std::sin(ball.getPos().y * .02f) * .28f));
            }
            if (U::len(out) < .1f) out = (closest->getSide() == Side::Left ? sf::Vector2f(1.f, 0.f) : sf::Vector2f(-1.f, 0.f));
            ball.setPos(closest->getPos() + out * (PLAYER_R + BALL_R + 8.f));
            const float power = closest->isSliding() ? 360.f : (closest->isControlled() ? 125.f : 145.f);
            ball.kick(out, power);
            setCommentary(playerName(*closest) + " touches the ball and looks for support.", 1.3f);
            if (closest->isSliding()) {
                referee.showCard(false);
                closest->giveYellow();
                assets.playWhistle();
                assets.playGrief();
                setCommentary("Penalty warning: late sliding tackle!", 1.8f);
            }
        }

        for (auto p : all) {
            if (!p->isKeeper()) continue;
            if (U::dist(p->getPos(), ball.getPos()) < 55.f) {
                const sf::Vector2f dir = (p->getSide() == Side::Left ? sf::Vector2f(1.f, 0.f) : sf::Vector2f(-1.f, 0.f));
                ball.kick(dir + sf::Vector2f(0.f, (ball.getPos().y - p->getPos().y) * .01f), 430.f);

                // If your attack is stopped by the opponent goalkeeper, play audience grief once.
                if (p->getSide() == Side::Right && lastTouched == Side::Left && griefCooldown <= 0.f) {
                    assets.playGrief();
                    griefCooldown = 2.0f;
                }

                lastTouched = p->getSide();
            }
        }
    }

    void controlledPass() {
        Player& p = left.get()[static_cast<std::size_t>(controlled)];
        Player* target = nullptr;
        float best = 99999.f;
        for (auto& q : left.get()) {
            if (&q == &p || q.isSentOff() || q.isKeeper()) continue;
            const float forward = (q.getPos().x - p.getPos().x);
            if (forward > -50.f) {
                const float d = U::dist(p.getPos(), q.getPos());
                if (d < best) { best = d; target = &q; }
            }
        }
        const sf::Vector2f dir = target ? target->getPos() - ball.getPos() : sf::Vector2f(1.f, 0.f);
        ball.kick(dir, 470.f);
        assets.playKick();
        lastTouched = Side::Left;
        setCommentary(playerName(p) + " is passing the ball forward.", 1.5f);
    }

    void controlledShoot() {
        Player& p = left.get()[static_cast<std::size_t>(controlled)];
        const sf::Vector2f goal = { FIELD_X + FIELD_W + 50.f, FIELD_Y + FIELD_H / 2.f };
        sf::Vector2f dir = goal - ball.getPos();
        dir.y += input.move.y * 70.f;
        ball.kick(dir, 760.f);
        assets.playKick();
        lastTouched = Side::Left;
        setCommentary(playerName(p) + " shoots toward goal!", 1.5f);
    }

    void handleRules() {
        const bool inGoalY = ball.getPos().y > FIELD_Y + FIELD_H / 2.f - GOAL_H / 2.f && ball.getPos().y < FIELD_Y + FIELD_H / 2.f + GOAL_H / 2.f;
        if (ball.getPos().x < FIELD_X - BALL_R) {
            if (inGoalY) { ++rightScore; kickoff = Side::Left; setCommentary(right.getName() + " scores! Replay incoming.", 2.0f); goalScene(); }
            else sideKick(lastTouched == Side::Left ? Side::Right : Side::Left);
        }
        if (ball.getPos().x > FIELD_X + FIELD_W + BALL_R) {
            if (inGoalY) { ++leftScore; kickoff = Side::Right; setCommentary(left.getName() + " scores! Replay incoming.", 2.0f); goalScene(); }
            else sideKick(lastTouched == Side::Left ? Side::Right : Side::Left);
        }
        if (ball.getPos().y < FIELD_Y - BALL_R || ball.getPos().y > FIELD_Y + FIELD_H + BALL_R) {
            sideKick(lastTouched == Side::Left ? Side::Right : Side::Left);
        }
        if (leftScore >= WIN_GOALS || rightScore >= WIN_GOALS) {
            state = GameState::GameOver;
            winAnim = 0.f;
            setCommentary((leftScore > rightScore ? left.getName() : right.getName()) + " wins the match!", 3.0f);
            assets.playCheer();
        }
    }

    void sideKick(Side s) {
        throwSide = s;
        state = GameState::ThrowIn;
        throwTimer = 1.8f;
        ball.stop();
        const float x = U::clamp(ball.getPos().x, FIELD_X + 40.f, FIELD_X + FIELD_W - 40.f);
        const float y = U::clamp(ball.getPos().y, FIELD_Y + 18.f, FIELD_Y + FIELD_H - 18.f);
        ball.setPos({ x, y });
        assets.playWhistle();
    }

    void goalScene() { state = GameState::GoalScene; goalSceneTime = 3.2f; replayTime = 2.4f; ball.stop(); assets.playCheer(); }
    void resetRound() { left.reset(); right.reset(); ball.reset(kickoff); controlled = 4; setControlled(); }


    void setCommentary(const std::string& line, float seconds = 1.7f) {
        commentary = line;
        commentaryHold = seconds;
    }

    void updateCommentary(float dt) {
        if (commentaryHold > 0.f) {
            commentaryHold -= dt;
            return;
        }
        commentaryAutoTimer -= dt;
        if (commentaryAutoTimer > 0.f) return;

        auto all = allPlayers();
        if (all.empty()) return;
        std::sort(all.begin(), all.end(), [&](Player* a, Player* b) {
            return U::dist(a->getPos(), ball.getPos()) < U::dist(b->getPos(), ball.getPos());
            });
        Player* p1 = all[0];
        Player* p2 = all.size() > 1 ? all[1] : all[0];
        if (U::len(ball.getVel()) > 420.f) {
            commentary = playerName(*p1) + " is chasing the ball as it moves quickly towards the goal.";
        }
        else if (input.pass) {
            commentary = playerName(*p1) + " is passing the ball to " + playerName(*p2) + " and moving towards cage.";
        }
        else if (input.shoot) {
            commentary = playerName(*p1) + " takes a shot while " + playerName(*p2) + " is closing down.";
        }
        else {
            commentary = playerName(*p1) + " is controlling the ball while " + playerName(*p2) + " is pressing carefully.";
        }
        commentaryAutoTimer = 1.6f + static_cast<float>(std::rand() % 5) * 0.12f;
    }

    std::string playerName(const Player& p) const {
        static const std::vector<std::string> stars = { "Messi","Ronaldo","Mbappe","Neymar","Haaland","Modric","Salah","Kane","Bellingham","Vinicius","Griezmann","De Bruyne","Pedri","Kroos" };
        const int base = p.getTeamIndex() * 3 + static_cast<int>(p.getHome().y / 90.f);
        return stars[static_cast<std::size_t>(std::abs(base)) % stars.size()];
    }

    std::string nearestPlayersText() {
        auto all = allPlayers();
        std::sort(all.begin(), all.end(), [&](Player* a, Player* b) { return U::dist(a->getPos(), ball.getPos()) < U::dist(b->getPos(), ball.getPos()); });
        std::string s = "Around ball: ";
        int c = 0;
        for (auto* p : all) {
            if (p->isSentOff()) continue;
            if (c++) s += "  |  ";
            s += playerName(*p);
            if (c >= 3) break;
        }
        return s;
    }

    void setupPenaltyRound(bool userTurn) {
        left.reset();
        right.reset();
        controlled = 4;
        setControlled();
        ball.setPos({ FIELD_X + FIELD_W - 260.f, FIELD_Y + FIELD_H / 2.f });
        ball.stop();
        setCommentary(userTurn ? "Penalty shootout: press K to shoot." : "Opponent penalty.", 2.0f);
    }

    void updatePenaltyMode(float dt) {
        if (griefCooldown > 0.f) griefCooldown -= dt;
        auto all = allPlayers();

        // Penalty mode rule: only the selected kicker can move; all other players stay fixed.
        for (std::size_t i = 0; i < left.get().size(); ++i) {
            if (static_cast<int>(i) == controlled) left.get()[i].update(dt, input, ball, all, false, true);
        }

        // Only the rival goalkeeper reacts/dives toward the ball.
        for (auto& p : right.get()) {
            if (p.isKeeper()) p.update(dt, input, ball, all, false, false);
        }

        ball.update(dt);
        referee.update(dt, ball.getPos());
        if (input.shoot) { controlledShoot(); ++penaltyShots; }
        const bool goalY = ball.getPos().y > FIELD_Y + FIELD_H / 2.f - GOAL_H / 2.f && ball.getPos().y < FIELD_Y + FIELD_H / 2.f + GOAL_H / 2.f;
        if (ball.getPos().x > FIELD_X + FIELD_W + BALL_R) {
            if (goalY) { ++leftScore; ++userPenGoals; setCommentary("Penalty scored!", 1.7f); assets.playCheer(); }
            else { setCommentary("Penalty missed.", 1.7f); assets.playGrief(); }
            setupPenaltyRound(true);
        }
        updateCommentary(dt);
        if (penaltyShots >= 5 || leftScore >= WIN_GOALS) { state = GameState::GameOver; winAnim = 0.f; assets.playCheer(); }
    }

    sf::View gameplayView() {
        sf::View v = worldView;
        const sf::Vector2f b = ball.getPos();
        const bool goalThreat = (b.x > FIELD_X + FIELD_W - 360.f || b.x < FIELD_X + 360.f) && b.y > FIELD_Y + FIELD_H / 2.f - GOAL_H * .85f && b.y < FIELD_Y + FIELD_H / 2.f + GOAL_H * .85f;
        if (state == GameState::GoalScene) { v.setCenter(ball.getPos()); v.zoom(.48f); return v; }
        if (goalThreat) { v.setCenter(U::clamp(b, sf::FloatRect(0.f, 0.f, STADIUM_W, STADIUM_H), 280.f)); v.zoom(.62f); }
        return v;
    }

    void drawText(sf::RenderTarget& t, const std::string& s, float x, float y, unsigned int size, sf::Color c, bool center = false) {
        if (!assets.hasFont) return;
        sf::Text txt(s, assets.font, size);
        txt.setFillColor(c);
        txt.setOutlineColor(sf::Color(0, 0, 0, 180));
        txt.setOutlineThickness(2.f);
        if (center) {
            const sf::FloatRect b = txt.getLocalBounds();
            txt.setOrigin(b.left + b.width / 2.f, b.top + b.height / 2.f);
        }
        txt.setPosition(x, y);
        t.draw(txt);
    }

    void render() {
        window.clear();
        if (state == GameState::Splash) renderSplash();
        else if (state == GameState::MainMenu) renderMenu();
        else if (state == GameState::TeamSelect) renderTeamSelect();
        else if (state == GameState::Settings) { renderMenu(); renderSettings(); }
        else if (state == GameState::Loading) renderLoadingScreen();
        else { renderGameWorld(); renderUI(); }
        applyBrightnessOverlay();
        window.display();
    }

    void drawMenuButton(float x, float y, const std::string& label, bool active) {
        sf::RectangleShape b({ 410.f, 56.f });
        b.setOrigin(205.f, 28.f);
        b.setPosition(x, y);
        b.setFillColor(active ? sf::Color(255, 224, 70, 235) : sf::Color(20, 35, 70, 220));
        b.setOutlineColor(active ? sf::Color::White : sf::Color(115, 150, 210));
        b.setOutlineThickness(2.f);
        window.draw(b);
        drawText(window, (active ? "> " : "  ") + label + (active ? " <" : "  "), x, y - 4.f, 32, active ? sf::Color(12, 20, 35) : sf::Color::White, true);
    }

    void renderSplash() {
        window.setView(uiView);
        introRuntime.draw(window, introRenderTexture, assets.font);
    }


    std::string diffName() const {
        switch (difficulty) {
        case Difficulty::Easy: return "EASY";
        case Difficulty::Medium: return "MEDIUM";
        case Difficulty::Hard: return "HARD";
        }
        return "MEDIUM";
    }

    std::string modeName() const {
        switch (matchMode) {
        case MatchMode::Normal: return "NORMAL MATCH";
        case MatchMode::PenaltyShootout: return "PENALTY SHOOTOUT";
        case MatchMode::Tournament: return "TOURNAMENT";
        }
        return "NORMAL MATCH";
    }

    int currentWeatherIndex() const {
        return theme == Theme::Day ? 0 : (theme == Theme::Night ? 1 : 2);
    }

    int currentMenuSlotIndex() const {
        if (difficultyDropdownOpen) {
            if (selectedDifficultyMenu == 0) return static_cast<int>(MenuSlot::DiffEasy);
            if (selectedDifficultyMenu == 1) return static_cast<int>(MenuSlot::DiffMedium);
            return static_cast<int>(MenuSlot::DiffHard);
        }
        switch (selectedMenu) {
        case 0: return static_cast<int>(MenuSlot::Start);
        case 1: return static_cast<int>(MenuSlot::TeamSelection);
        case 2: return static_cast<int>(MenuSlot::Difficulty);
        case 3: return matchMode == MatchMode::Tournament ? static_cast<int>(MenuSlot::Tournament) : static_cast<int>(MenuSlot::Penalty);
        case 4: return static_cast<int>(MenuSlot::Exit);
        default: return static_cast<int>(MenuSlot::Start);
        }
    }

    void renderMenu() {
        window.setView(uiView);
        const sf::Vector2u sz = window.getSize();
        const float W = U::f(sz.x), H = U::f(sz.y);

        const int weatherIndex = currentWeatherIndex();
        const int slotIndex = currentMenuSlotIndex();

        // IMPORTANT:
        // This menu uses full PNG screens for each highlight state.
        // Total supported images: 27 = 3 weather modes x 9 menu states.
        // No built-in glow/rectangle is drawn by C++.
        // If the selected image exists, it is displayed exactly as your exported PNG.
        if (assets.hasUiMenuState[weatherIndex][slotIndex]) {
            sf::Sprite menu(assets.uiMenuState[weatherIndex][slotIndex]);
            const sf::Vector2u img = assets.uiMenuState[weatherIndex][slotIndex].getSize();
            menu.setPosition(0.f, 0.f);
            menu.setScale(W / U::f(img.x), H / U::f(img.y)); // Stretch to full screen so it fits the frame.
            window.draw(menu);
            return;
        }

        // Fallback only if one of the 27 required images is missing.
        // This fallback has NO glow and NO selection rectangle.
        const sf::Texture* fallback = nullptr;
        bool hasFallback = false;
        if (theme == Theme::Day) {
            fallback = &assets.uiMenuDay;
            hasFallback = assets.hasUiMenuDay;
        }
        else if (theme == Theme::Night) {
            fallback = &assets.uiMenuNight;
            hasFallback = assets.hasUiMenuNight;
        }
        else {
            fallback = &assets.uiMenuRain;
            hasFallback = assets.hasUiMenuRain;
        }

        if (hasFallback) {
            sf::Sprite menu(*fallback);
            const sf::Vector2u img = fallback->getSize();
            menu.setPosition(0.f, 0.f);
            menu.setScale(W / U::f(img.x), H / U::f(img.y));
            window.draw(menu);
            return;
        }

        // Last fallback: visible warning if no menu images are available.
        sf::RectangleShape back({ W, H });
        back.setFillColor(sf::Color(5, 12, 28));
        window.draw(back);
        drawText(window, "FIFA 2D FOOTBALL", W / 2.f, H * .14f, 60, sf::Color::White, true);
        drawText(window, "Missing PNG menu state image", W / 2.f, H * .28f, 30, sf::Color(255, 230, 80), true);
        drawText(window, "Example: assets/ui/menu_day_start.png", W / 2.f, H * .36f, 24, sf::Color(220, 235, 255), true);
    }

    void renderTeamSelect() {
        window.setView(uiView);
        const sf::Vector2u sz = window.getSize();
        const float W = U::f(sz.x), H = U::f(sz.y);

        // Preferred team-selection UI: full PNG screens, divided by weather.
        // If DAY is selected in main menu -> assets/ui/team_selection/day/team_01..team_12.png
        // If NIGHT is selected -> assets/ui/team_selection/night/team_01..team_12.png
        // If RAIN is selected -> assets/ui/team_selection/rain/team_01..team_12.png
        const int w = currentWeatherIndex();
        if (!assets.teamSelectionScreens[w].empty()) {
            currentTeamScreen = (currentTeamScreen + static_cast<int>(assets.teamSelectionScreens[w].size())) % static_cast<int>(assets.teamSelectionScreens[w].size());
            sf::Texture& tex = assets.teamSelectionScreens[w][currentTeamScreen];
            sf::Sprite screen(tex);
            const sf::Vector2u img = tex.getSize();
            screen.setScale(W / U::f(img.x), H / U::f(img.y));
            window.draw(screen);
            drawText(window, "LEFT/RIGHT: CHANGE TEAM     ENTER: CONFIRM     ESC: BACK", W / 2.f, H - 38.f, 22, sf::Color(255, 255, 255), true);
            return;
        }

        sf::RectangleShape bg({ W, H });
        bg.setFillColor(sf::Color(7, 15, 34));
        window.draw(bg);
        for (int i = 0; i < 16; ++i) {
            sf::RectangleShape ray({ W * 1.2f, 18.f });
            ray.setOrigin(W * .6f, 9.f);
            ray.setPosition(W / 2.f, 50.f + static_cast<float>(i) * 54.f + std::sin(splashTime * 2.f + static_cast<float>(i)) * 6.f);
            ray.setRotation(i % 2 ? 6.f : -6.f);
            ray.setFillColor(sf::Color(35, 130, 210, 15));
            window.draw(ray);
        }
        sf::RectangleShape header({ W * .58f, 58.f });
        header.setOrigin(header.getSize().x / 2.f, 29.f);
        header.setPosition(W / 2.f, 72.f);
        header.setFillColor(sf::Color(190, 20, 35));
        header.setOutlineColor(sf::Color::White);
        header.setOutlineThickness(2.f);
        window.draw(header);
        drawText(window, "TEAM SELECTION", W / 2.f, 72.f, 38, sf::Color::White, true);
        drawText(window, "Left/Right = Change Team     Enter = Confirm     Esc = Back", W / 2.f, 130.f, 22, sf::Color(225, 235, 255), true);
        drawTeamCard(selectedUserTeam, W * .26f, H * .55f, "HOME TEAM");
        drawText(window, "VS", W / 2.f, H * .50f, 82, sf::Color(238, 238, 238), true);
        drawTeamCard(selectedRivalTeam, W * .74f, H * .55f, "AWAY TEAM");
        sf::RectangleShape strip({ W * .70f, 44.f });
        strip.setOrigin(strip.getSize().x / 2.f, 22.f);
        strip.setPosition(W / 2.f, H - 70.f);
        strip.setFillColor(sf::Color(255, 255, 255, 26));
        strip.setOutlineColor(sf::Color(255, 255, 255, 80));
        strip.setOutlineThickness(1.f);
        window.draw(strip);
        drawText(window, "Tip: put your selected font at assets/fonts/menu.ttf for clean FIFA-style text", W / 2.f, H - 70.f, 20, sf::Color(230, 230, 230), true);
    }

    void drawTeamCard(int idx, float x, float y, const std::string& title) {
        sf::RectangleShape shadow({ 410.f, 430.f });
        shadow.setOrigin(205.f, 215.f);
        shadow.setPosition(x + 8.f, y + 10.f);
        shadow.setFillColor(sf::Color(0, 0, 0, 145));
        window.draw(shadow);
        sf::RectangleShape card({ 410.f, 430.f });
        card.setOrigin(205.f, 215.f);
        card.setPosition(x, y);
        card.setFillColor(sf::Color(18, 22, 32, 235));
        card.setOutlineColor(defs[static_cast<std::size_t>(idx)].primary);
        card.setOutlineThickness(5.f);
        window.draw(card);
        sf::RectangleShape top({ 410.f, 42.f });
        top.setOrigin(205.f, 21.f);
        top.setPosition(x, y - 193.f);
        top.setFillColor(defs[static_cast<std::size_t>(idx)].primary);
        window.draw(top);
        drawText(window, title, x, y - 194.f, 23, sf::Color::White, true);
        if (assets.team[idx].hasFlag) {
            sf::Sprite f(assets.team[idx].flag);
            const sf::Vector2u z = assets.team[idx].flag.getSize();
            f.setOrigin(U::f(z.x) / 2.f, U::f(z.y) / 2.f);
            f.setPosition(x, y - 115.f);
            f.setScale(145.f / U::f(z.x), 92.f / U::f(z.y));
            window.draw(f);
        }
        drawText(window, defs[static_cast<std::size_t>(idx)].displayName, x, y - 32.f, 32, sf::Color(255, 235, 90), true);
        drawText(window, "STARTERS", x - 100.f, y + 15.f, 19, sf::Color(120, 255, 170), true);
        drawText(window, "RESERVES", x + 100.f, y + 15.f, 19, sf::Color(220, 220, 220), true);
        for (int i = 0; i < 6; ++i) {
            sf::RectangleShape row({ 150.f, 24.f });
            row.setOrigin(75.f, 12.f);
            row.setPosition(x - 100.f, y + 48.f + static_cast<float>(i) * 31.f);
            row.setFillColor(i % 2 ? sf::Color(255, 255, 255, 24) : sf::Color(255, 255, 255, 14));
            window.draw(row);
            sf::CircleShape dot(8.f);
            dot.setOrigin(8.f, 8.f);
            dot.setPosition(x - 166.f, y + 48.f + static_cast<float>(i) * 31.f);
            dot.setFillColor(defs[static_cast<std::size_t>(idx)].secondary);
            dot.setOutlineColor(sf::Color::White);
            dot.setOutlineThickness(1.f);
            window.draw(dot);
            drawText(window, "Player " + std::to_string(i + 1), x - 96.f, y + 46.f + static_cast<float>(i) * 31.f, 15, sf::Color::White, true);
            sf::RectangleShape r2({ 130.f, 22.f });
            r2.setOrigin(65.f, 11.f);
            r2.setPosition(x + 100.f, y + 48.f + static_cast<float>(i) * 31.f);
            r2.setFillColor(sf::Color(255, 255, 255, 14));
            window.draw(r2);
            drawText(window, "Sub " + std::to_string(i + 1), x + 100.f, y + 46.f + static_cast<float>(i) * 31.f, 14, sf::Color(220, 220, 220), true);
        }
    }

    void renderGameWorld() {
        window.setView(gameplayView());
        const sf::Texture* st = &assets.stadiumDay;
        bool hs = assets.hasStadiumDay;
        if (theme == Theme::Night) { st = &assets.stadiumNight; hs = assets.hasStadiumNight; }
        else if (theme == Theme::Rain) { st = assets.hasStadiumRain ? &assets.stadiumRain : &assets.stadiumDay; hs = assets.hasStadiumRain ? assets.hasStadiumRain : assets.hasStadiumDay; }
        if (hs) { sf::Sprite s(*st); window.draw(s); }
        else drawFallbackField();

        const sf::Texture* cr = (theme == Theme::Day ? &assets.crowdDay : &assets.crowdNight);
        const bool hc = (theme == Theme::Day ? assets.hasCrowdDay : assets.hasCrowdNight);
        if (hc) { sf::Sprite c(*cr); window.draw(c); }
        if (theme == Theme::Night) { sf::RectangleShape grade({ STADIUM_W, STADIUM_H }); grade.setFillColor(sf::Color(20, 45, 95, 42)); window.draw(grade); }
        if (theme == Theme::Rain) {
            sf::RectangleShape grade({ STADIUM_W, STADIUM_H });
            grade.setFillColor(sf::Color(45, 65, 85, 55));
            window.draw(grade);
            for (int i = 0; i < 90; ++i) {
                const float x = static_cast<float>((i * 47) % 2200);
                const float y = static_cast<float>((i * 83) % 1300);
                sf::Vertex line[] = {
                    sf::Vertex(sf::Vector2f(x, y), sf::Color(210, 230, 255, 75)),
                    sf::Vertex(sf::Vector2f(x + 25.f, y + 70.f), sf::Color(210, 230, 255, 10))
                };
                window.draw(line, 2, sf::Lines);
            }
        }
        drawGoals();
        for (auto& p : right.get()) p.draw(window, assets, false);
        for (auto& p : left.get()) p.draw(window, assets, p.isControlled());
        ball.draw(window, assets);
        referee.draw(window, assets);
        if (state == GameState::GoalScene) drawReplayRibbon();
    }

    void drawFallbackField() {
        for (int i = 0; i < 12; ++i) {
            sf::RectangleShape stripe({ FIELD_W / 12.f, FIELD_H });
            stripe.setPosition(FIELD_X + static_cast<float>(i) * FIELD_W / 12.f, FIELD_Y);
            stripe.setFillColor(i % 2 ? sf::Color(45, 135, 70) : sf::Color(65, 165, 80));
            window.draw(stripe);
        }
    }

    void drawGoals() {
        if (assets.hasGoal) {
            sf::Sprite l(assets.goalCage), r(assets.goalCage);
            const sf::Vector2u z = assets.goalCage.getSize();
            l.setOrigin(U::f(z.x) / 2.f, U::f(z.y) / 2.f);
            r.setOrigin(U::f(z.x) / 2.f, U::f(z.y) / 2.f);
            l.setPosition(FIELD_X - 42.f, FIELD_Y + FIELD_H / 2.f);
            r.setPosition(FIELD_X + FIELD_W + 42.f, FIELD_Y + FIELD_H / 2.f);
            l.setScale(.42f, .68f);
            r.setScale(.42f, .68f);
            window.draw(l);
            window.draw(r);
        }
    }

    void renderUI() {
        window.setView(uiView);
        const sf::Vector2u sz = window.getSize();
        const float W = U::f(sz.x), H = U::f(sz.y);
        sf::RectangleShape top({ W, 76.f });
        top.setFillColor(sf::Color(0, 0, 0, 155));
        window.draw(top);
        drawText(window, left.getName() + "  " + std::to_string(leftScore) + " - " + std::to_string(rightScore) + "  " + right.getName(), W / 2.f, 28.f, 30, sf::Color::White, true);
        drawText(window, (matchMode == MatchMode::PenaltyShootout ? "PENALTY MODE" : (matchMode == MatchMode::Tournament ? "TOURNAMENT" : timer.str())), W / 2.f, 62.f, 22, sf::Color(255, 230, 70), true);
        sf::RectangleShape com({ W, 62.f });
        com.setPosition(0.f, H - 62.f);
        com.setFillColor(sf::Color(0, 0, 0, 165));
        window.draw(com);
        drawText(window, commentary, W / 2.f, H - 33.f, 22, sf::Color(240, 245, 255), true);
        if (state == GameState::Paused) renderPauseMenu();
        if (state == GameState::ThrowIn) overlay("SIDE KICK", (throwSide == Side::Left ? left.getName() : right.getName()) + " will restart");
        if (state == GameState::GameOver) renderWinScreen();
        if (state == GameState::GoalScene) overlay("GOAL!", "Replay camera and celebration...");
    }

    void drawReplayRibbon() {
        sf::RectangleShape r({ 360.f, 70.f });
        r.setOrigin(180.f, 35.f);
        r.setPosition(ball.getPos() + sf::Vector2f(0.f, -120.f));
        r.setFillColor(sf::Color(210, 20, 40, 210));
        r.setOutlineColor(sf::Color::White);
        r.setOutlineThickness(3.f);
        window.draw(r);
        drawText(window, "REPLAY  GOAL!", ball.getPos().x, ball.getPos().y - 124.f, 30, sf::Color::White, true);
    }

    void renderWinScreen() {
        const sf::Vector2u sz = window.getSize();
        const float W = U::f(sz.x), H = U::f(sz.y);
        sf::RectangleShape o({ W, H });
        o.setFillColor(sf::Color(0, 0, 0, 190));
        window.draw(o);
        for (int i = 0; i < 80; ++i) {
            const float x = std::fmod(static_cast<float>(i) * 97.f + winAnim * 180.f, W);
            const float y = std::fmod(static_cast<float>(i) * 53.f + std::sin(winAnim + static_cast<float>(i)) * 80.f, H);
            sf::CircleShape c(5.f + static_cast<float>(i % 4));
            c.setOrigin(c.getRadius(), c.getRadius());
            c.setPosition(x, y);
            c.setFillColor(i % 3 == 0 ? sf::Color(255, 230, 70) : i % 3 == 1 ? sf::Color(80, 190, 255) : sf::Color(255, 80, 110));
            window.draw(c);
        }
        const std::string winner = (leftScore > rightScore ? left.getName() : right.getName());
        drawText(window, "CHAMPIONS!", W / 2.f, H * .30f, 82, sf::Color(255, 230, 70), true);
        drawText(window, winner + " WINS BY FIRST TO 3 GOALS", W / 2.f, H * .43f, 38, sf::Color::White, true);
        drawText(window, "Final Score: " + std::to_string(leftScore) + " - " + std::to_string(rightScore), W / 2.f, H * .52f, 32, sf::Color(210, 235, 255), true);
        drawText(window, "ENTER: Play Again     ESC/P: Main Menu", W / 2.f, H * .66f, 25, sf::Color(240, 240, 240), true);
    }

    void renderPauseMenu() {
        window.setView(uiView);
        const sf::Vector2u sz = window.getSize();
        const float W = U::f(sz.x), H = U::f(sz.y);
        const sf::Texture* tex = selectedPause == 0 ? &assets.pauseResumeScreen : &assets.pauseExitScreen;
        const bool has = selectedPause == 0 ? assets.hasPauseResumeScreen : assets.hasPauseExitScreen;
        if (has) {
            sf::Sprite s(*tex);
            const sf::Vector2u img = tex->getSize();
            s.setScale(W / U::f(img.x), H / U::f(img.y));
            window.draw(s);
            return;
        }
        sf::RectangleShape o({ W,H });
        o.setFillColor(sf::Color(0, 0, 0, 165));
        window.draw(o);
        drawText(window, "PAUSED", W / 2.f, H * .25f, 60, sf::Color::White, true);
        drawMenuButton(W / 2.f, H * .45f, "RESUME MATCH", selectedPause == 0);
        drawMenuButton(W / 2.f, H * .60f, "EXIT TO MAIN MENU", selectedPause == 1);
    }

    void renderLoadingScreen() {
        window.setView(uiView);
        const sf::Vector2u sz = window.getSize();
        const float W = U::f(sz.x), H = U::f(sz.y);

        // NOTE:
        // SFML cannot run the uploaded HTML/canvas file directly.
        // This is a C++/SFML remake of that cinematic loading animation:
        // animated background, moving light streaks, title reveal, rotating ball,
        // team-vs-team text, scan lines, and loading progress.

        const float total = 7.0f;
        const float t = loadingTime;
        const float p = std::min(1.0f, t / total);

        // 1) Draw selected loading screen as the cinematic background if available.
        bool has = false;
        const sf::Texture* tex = nullptr;
        if (selectedUserTeam >= 0 && selectedUserTeam < 4 && selectedRivalTeam >= 0 && selectedRivalTeam < 4) {
            has = assets.hasLoadingScreen[selectedUserTeam][selectedRivalTeam];
            tex = &assets.loadingScreen[selectedUserTeam][selectedRivalTeam];
        }

        if (has && tex) {
            sf::Sprite s(*tex);
            const sf::Vector2u img = tex->getSize();
            s.setOrigin(U::f(img.x) / 2.0f, U::f(img.y) / 2.0f);
            s.setPosition(W / 2.0f, H / 2.0f);
            // Slow cinematic zoom/pan effect.
            const float baseScale = std::max(W / U::f(img.x), H / U::f(img.y));
            const float zoom = 1.04f + 0.035f * std::sin(t * 0.85f);
            s.setScale(baseScale * zoom, baseScale * zoom);
            s.move(std::sin(t * 0.55f) * 24.0f, std::cos(t * 0.42f) * 10.0f);
            window.draw(s);
        }
        else {
            sf::RectangleShape bg({ W, H });
            bg.setFillColor(sf::Color(3, 8, 24));
            window.draw(bg);
        }

        // 2) Dark cinematic overlay.
        sf::RectangleShape shade({ W, H });
        shade.setFillColor(sf::Color(0, 0, 0, 115));
        window.draw(shade);

        // 3) Animated diagonal blue/red speed streaks.
        for (int i = 0; i < 18; ++i) {
            const float x = std::fmod(t * (220.0f + i * 9.0f) + i * 173.0f, W + 420.0f) - 260.0f;
            const float y = 80.0f + std::fmod(i * 91.0f + t * 26.0f, H - 160.0f);
            sf::RectangleShape streak({ 300.0f + (i % 3) * 90.0f, 4.0f });
            streak.setOrigin(streak.getSize().x / 2.0f, 2.0f);
            streak.setPosition(x, y);
            streak.setRotation(-18.0f);
            streak.setFillColor(i % 2 == 0 ? sf::Color(0, 190, 255, 95) : sf::Color(255, 40, 100, 80));
            window.draw(streak);
        }

        // 4) Pulsing center glow.
        sf::CircleShape glow(190.0f);
        glow.setOrigin(190.0f, 190.0f);
        glow.setPosition(W / 2.0f, H * 0.42f);
        const sf::Uint8 glowAlpha = static_cast<sf::Uint8>(45.0f + 35.0f * (0.5f + 0.5f * std::sin(t * 3.4f)));
        glow.setFillColor(sf::Color(0, 170, 255, glowAlpha));
        window.draw(glow);

        // 5) Title reveal.
        const float titleAlphaF = U::clamp((t - 0.35f) / 1.1f, 0.0f, 1.0f);
        const sf::Uint8 titleAlpha = static_cast<sf::Uint8>(titleAlphaF * 255.0f);
        drawText(window, "FIFA 2D FOOTBALL", W / 2.0f, H * 0.28f - (1.0f - titleAlphaF) * 45.0f, 64, sf::Color(255, 255, 255, titleAlpha), true);
        drawText(window, "developed by BU droppers", W / 2.0f, H * 0.37f, 28, sf::Color(255, 225, 70, titleAlpha), true);

        // 6) Selected match text.
        const std::string matchLine = defs[selectedUserTeam].displayName + "  VS  " + defs[selectedRivalTeam].displayName;
        drawText(window, matchLine, W / 2.0f, H * 0.49f, 42, sf::Color(235, 245, 255), true);
        drawText(window, theme == Theme::Rain ? "RAIN MATCH" : (theme == Theme::Night ? "NIGHT MATCH" : "DAY MATCH"), W / 2.0f, H * 0.56f, 24, sf::Color(80, 220, 255), true);

        // 7) Rotating ball animation.
        const float ballX = W * 0.78f + std::sin(t * 2.1f) * 18.0f;
        const float ballY = H * 0.49f + std::cos(t * 1.7f) * 12.0f;
        if (assets.hasBall) {
            sf::Sprite b(assets.ball);
            const sf::Vector2u bz = assets.ball.getSize();
            b.setOrigin(U::f(bz.x) / 2.0f, U::f(bz.y) / 2.0f);
            b.setPosition(ballX, ballY);
            b.setRotation(t * 240.0f);
            const float sc = 92.0f / std::max(U::f(bz.x), U::f(bz.y));
            b.setScale(sc, sc);
            window.draw(b);
        }
        else {
            sf::CircleShape b(45.0f);
            b.setOrigin(45.0f, 45.0f);
            b.setPosition(ballX, ballY);
            b.setFillColor(sf::Color::White);
            b.setOutlineColor(sf::Color::Black);
            b.setOutlineThickness(4.0f);
            window.draw(b);
        }

        // 8) Rain particles during rain loading.
        if (theme == Theme::Rain) {
            for (int i = 0; i < 140; ++i) {
                const float rx = std::fmod(i * 61.0f + t * 620.0f, W + 70.0f) - 35.0f;
                const float ry = std::fmod(i * 97.0f + t * 850.0f, H + 80.0f) - 40.0f;
                sf::Vertex line[] = {
                    sf::Vertex(sf::Vector2f(rx, ry), sf::Color(210, 235, 255, 110)),
                    sf::Vertex(sf::Vector2f(rx + 18.0f, ry + 48.0f), sf::Color(210, 235, 255, 25))
                };
                window.draw(line, 2, sf::Lines);
            }
        }

        // 9) Progress bar.
        sf::RectangleShape barBack({ W * 0.48f, 17.0f });
        barBack.setOrigin(barBack.getSize().x / 2.0f, 8.5f);
        barBack.setPosition(W / 2.0f, H * 0.82f);
        barBack.setFillColor(sf::Color(255, 255, 255, 35));
        barBack.setOutlineColor(sf::Color(255, 255, 255, 85));
        barBack.setOutlineThickness(1.0f);
        window.draw(barBack);

        sf::RectangleShape bar({ W * 0.48f * p, 17.0f });
        bar.setOrigin(0.0f, 8.5f);
        bar.setPosition(W / 2.0f - W * 0.24f, H * 0.82f);
        bar.setFillColor(sf::Color(0, 200, 255, 235));
        window.draw(bar);

        drawText(window, "LOADING MATCH  " + std::to_string(static_cast<int>(p * 100.0f)) + "%", W / 2.0f, H * 0.875f, 24, sf::Color::White, true);
        drawText(window, "ENTER: SKIP", W - 112.0f, H - 34.0f, 18, sf::Color(220, 220, 220), true);

        // 10) Fade in/out like a cutscene.
        float fade = 0.0f;
        if (t < 0.7f) fade = 1.0f - t / 0.7f;
        if (t > total - 0.8f) fade = (t - (total - 0.8f)) / 0.8f;
        if (fade > 0.0f) {
            sf::RectangleShape f({ W, H });
            f.setFillColor(sf::Color(0, 0, 0, static_cast<sf::Uint8>(U::clamp(fade, 0.0f, 1.0f) * 255.0f)));
            window.draw(f);
        }
    }

    void renderSettings() {
        window.setView(uiView);
        const sf::Vector2u sz = window.getSize();
        const float W = U::f(sz.x), H = U::f(sz.y);
        sf::RectangleShape shade({ W,H }); shade.setFillColor(sf::Color(0, 0, 0, 150)); window.draw(shade);
        sf::RectangleShape panel({ W * .42f, H * .38f }); panel.setOrigin(panel.getSize().x / 2.f, panel.getSize().y / 2.f); panel.setPosition(W / 2.f, H / 2.f); panel.setFillColor(sf::Color(8, 18, 38, 235)); panel.setOutlineColor(sf::Color(0, 195, 255)); panel.setOutlineThickness(3.f); window.draw(panel);
        drawText(window, "SETTINGS", W / 2.f, H * .34f, 44, sf::Color(255, 255, 255), true);
        drawText(window, std::string(selectedSettings == 0 ? "> " : "  ") + "Brightness: " + std::to_string(static_cast<int>(brightness * 100.f)) + "%", W / 2.f, H * .45f, 28, selectedSettings == 0 ? sf::Color(80, 220, 255) : sf::Color::White, true);
        drawText(window, std::string(selectedSettings == 1 ? "> " : "  ") + "Music: " + std::to_string(static_cast<int>(assets.musicVolume)) + "%", W / 2.f, H * .53f, 28, selectedSettings == 1 ? sf::Color(80, 220, 255) : sf::Color::White, true);
        drawText(window, std::string(selectedSettings == 2 ? "> " : "  ") + "Volume: " + std::to_string(static_cast<int>(assets.volume)) + "%", W / 2.f, H * .61f, 28, selectedSettings == 2 ? sf::Color(80, 220, 255) : sf::Color::White, true);
        drawText(window, "Music controls BGM/Menu music. Volume controls audience crowd loop.", W / 2.f, H * .68f, 17, sf::Color(225, 235, 255), true);
        drawText(window, "UP/DOWN: OPTION   LEFT/RIGHT: ADJUST   ENTER/ESC: BACK", W / 2.f, H * .73f, 18, sf::Color(225, 235, 255), true);
    }

    void applyBrightnessOverlay() {
        window.setView(uiView);
        const sf::Vector2u sz = window.getSize();
        const float W = U::f(sz.x), H = U::f(sz.y);
        if (brightness < 0.99f) {
            sf::RectangleShape o({ W,H }); o.setFillColor(sf::Color(0, 0, 0, static_cast<sf::Uint8>((1.f - brightness) * 180.f))); window.draw(o);
        }
        else if (brightness > 1.01f) {
            sf::RectangleShape o({ W,H }); o.setFillColor(sf::Color(255, 255, 255, static_cast<sf::Uint8>((brightness - 1.f) * 85.f))); window.draw(o);
        }
    }

    void overlay(const std::string& a, const std::string& b) {
        const sf::Vector2u sz = window.getSize();
        const float W = U::f(sz.x), H = U::f(sz.y);
        sf::RectangleShape o({ W, H });
        o.setFillColor(sf::Color(0, 0, 0, 90));
        window.draw(o);
        drawText(window, a, W / 2.f, H * .43f, 54, sf::Color(255, 230, 70), true);
        drawText(window, b, W / 2.f, H * .52f, 28, sf::Color::White, true);
    }
};

int main() {
    try {
        Game game;
        game.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}
