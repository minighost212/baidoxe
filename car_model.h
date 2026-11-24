#pragma once
// ============================================================================
//  car_model.h  -  Toyota Vios (XP90) 'Driving License Test' car, built from a
//  SKELETON of cross-sections that are lofted into one smooth surface.
//  Only needs GLM.  Coordinates: +X forward, +Y up, +Z = car's right, metres.
// ============================================================================
#include "aeon_layout.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <map>
#include <tuple>
#include <cmath>
#include <algorithm>
#include <cstdio>
#include <initializer_list>
#include <utility>

namespace car {
using glm::vec2; using glm::vec3; using glm::mat4;
constexpr float PI = AeonLayout::PI;

enum Mat { BODY = 0, GLASS = 1, RUBBER = 2, ALLOY = 3, CHROME = 4, BLACK = 5, LAMP_HEAD = 6,
           LAMP_TAIL = 7, CABIN = 8, SIGN = 9, BEACON_B = 10, BEACON_A = 11, BEACON_R = 12,
           BRAKE = 13, FABRIC = 14, PAINT = 15, SPOKE = 16, GROUND = 99 };

struct Vertex { vec3 p, n; vec2 uv; float mat; };
struct Mesh {
    std::vector<Vertex> v;
    std::vector<unsigned> i;
    void append(const Mesh& o) {
        unsigned base = (unsigned)v.size();
        v.insert(v.end(), o.v.begin(), o.v.end());
        for (unsigned k : o.i) i.push_back(base + k);
    }
};

// ---------------------------------------------------------------- mesh utils
inline void transform(Mesh& m, const mat4& T) {
    glm::mat3 N = glm::transpose(glm::inverse(glm::mat3(T)));
    for (auto& q : m.v) { q.p = vec3(T * glm::vec4(q.p, 1.f)); q.n = glm::normalize(N * q.n); }
    if (glm::determinant(glm::mat3(T)) < 0)            // mirrored -> keep winding consistent
        for (size_t k = 0; k + 2 < m.i.size(); k += 3) std::swap(m.i[k + 1], m.i[k + 2]);
}

inline void smoothNormals(Mesh& m) {
    for (auto& v : m.v) v.n = vec3(0);
    for (size_t k = 0; k + 2 < m.i.size(); k += 3) {
        Vertex &a = m.v[m.i[k]], &b = m.v[m.i[k + 1]], &c = m.v[m.i[k + 2]];
        vec3 n = glm::cross(b.p - a.p, c.p - a.p);      // area weighted
        a.n += n; b.n += n; c.n += n;
    }
    for (auto& v : m.v) { float l = glm::length(v.n); v.n = l > 1e-12f ? v.n / l : vec3(0, 1, 0); }
}

// average normals of vertices sharing a position (closes seams / poles)
inline void weld(Mesh& m) {
    typedef std::tuple<int, int, int> K;
    std::map<K, vec3> acc;
    auto key = [](const vec3& p) {
        return K((int)std::lround(p.x * 2e4f), (int)std::lround(p.y * 2e4f), (int)std::lround(p.z * 2e4f));
    };
    for (auto& q : m.v) acc[key(q.p)] += q.n;
    for (auto& q : m.v) { vec3 s = acc[key(q.p)]; float l = glm::length(s); if (l > 1e-9f) q.n = s / l; }
}

// ---------------------------------------------------- 1-D monotone cubic curve
// Used to interpolate every design parameter (height, width ...) along the car.
struct Curve {
    std::vector<float> X, Y, M;
    Curve(std::initializer_list<std::pair<float, float>> k) {
        for (auto& q : k) { X.push_back(q.first); Y.push_back(q.second); }
        size_t n = X.size();
        M.assign(n, 0.f);
        std::vector<float> d(n - 1), h(n - 1);
        for (size_t i = 0; i + 1 < n; i++) { h[i] = X[i + 1] - X[i]; d[i] = (Y[i + 1] - Y[i]) / h[i]; }
        M[0] = d[0]; M[n - 1] = d[n - 2];
        for (size_t i = 1; i + 1 < n; i++)
            M[i] = d[i - 1] * d[i] <= 0 ? 0.f : (d[i - 1] * h[i] + d[i] * h[i - 1]) / (h[i - 1] + h[i]);
        for (size_t i = 0; i + 1 < n; i++) {            // Fritsch-Carlson limiter (no overshoot)
            if (d[i] == 0) { M[i] = M[i + 1] = 0; continue; }
            float a = M[i] / d[i], b = M[i + 1] / d[i], s = a * a + b * b;
            if (s > 9) { float t = 3.f / std::sqrt(s); M[i] = t * a * d[i]; M[i + 1] = t * b * d[i]; }
        }
    }
    float operator()(float x) const {
        if (x <= X.front()) return Y.front();
        if (x >= X.back()) return Y.back();
        size_t i = std::upper_bound(X.begin(), X.end(), x) - X.begin() - 1;
        float h = X[i + 1] - X[i], t = (x - X[i]) / h, t2 = t * t, t3 = t2 * t;
        return (2 * t3 - 3 * t2 + 1) * Y[i] + (t3 - 2 * t2 + t) * h * M[i] +
               (-2 * t3 + 3 * t2) * Y[i + 1] + (t3 - t2) * h * M[i + 1];
    }
};

// ------------------------------------------------------------ the skeleton
constexpr float X_REAR = -AeonLayout::CAR_LENGTH_M * AeonLayout::GEOMETRY_HALF_RATIO;
constexpr float X_FRONT = AeonLayout::CAR_LENGTH_M * AeonLayout::GEOMETRY_HALF_RATIO;
constexpr float AXLE_X = AeonLayout::CAR_WHEELBASE_M * AeonLayout::GEOMETRY_HALF_RATIO;
constexpr float TRACK_Z = AeonLayout::CAR_TRACK_WIDTH_M * AeonLayout::GEOMETRY_HALF_RATIO;
constexpr float WHEEL_R = AeonLayout::CAR_WHEEL_RADIUS_M;
// Remaining cross-section control points below are local mesh-shape data,
// not site/layout measurements; the overall envelope above comes from AeonLayout.

// Rounded polygon: every corner replaced by a quadratic-Bezier fillet (K points).
inline void buildRing(const std::vector<vec2>& V, const std::vector<float>& R, int K,
                      std::vector<vec2>& out, std::vector<int>& mid) {
    size_t n = V.size();
    mid.assign(n, 0);
    for (size_t i = 0; i < n; i++) {
        vec2 P = V[i], A = V[(i + n - 1) % n], B = V[(i + 1) % n];
        if (R[i] <= 0.f) { mid[i] = (int)out.size(); out.push_back(P); continue; }
        float la = glm::length(A - P), lb = glm::length(B - P);
        float r = std::min(R[i], 0.45f * std::min(la, lb));
        vec2 p0 = P + (A - P) / la * r, p1 = P + (B - P) / lb * r;
        int start = (int)out.size();
        for (int k = 0; k < K; k++) {
            float t = (float)k / (K - 1);
            out.push_back((1 - t) * (1 - t) * p0 + 2 * t * (1 - t) * P + t * t * p1);
        }
        mid[i] = start + K / 2;
    }
}

// Body = lofted cross-sections ("ribs").  Each rib is a rounded polygon whose
// key points (roof, belt line, widest point, sill ...) are driven by the curves.
// vertex.uv = (arc distance from roof corner, arc distance from belt line):
// the shader uses it to cut the windows into the very same surface.
inline Mesh buildBody() {
    static const Curve Yt({{X_REAR, 0.90f}, {-2.12f, 0.945f}, {-1.90f, 0.975f}, {-1.65f, 0.98f},
        {-1.48f, 0.985f}, {-1.25f, 1.12f}, {-1.05f, 1.27f}, {-0.85f, 1.385f}, {-0.60f, 1.45f},
        {-0.30f, 1.468f}, {0.0f, 1.45f}, {0.22f, 1.40f}, {0.45f, 1.285f}, {0.70f, 1.13f},
        {0.90f, 1.01f}, {1.02f, 0.965f}, {1.20f, 0.935f}, {1.50f, 0.905f}, {1.85f, 0.86f},
        {2.05f, 0.82f}, {2.15f, 0.78f}, {X_FRONT, 0.74f}});                        // roof/hood/deck line
    static const Curve Yb({{X_REAR, 0.34f}, {-2.05f, 0.30f}, {-1.80f, 0.24f}, {-1.40f, 0.20f},
        {-1.0f, 0.19f}, {1.0f, 0.19f}, {1.40f, 0.20f}, {1.80f, 0.21f}, {2.05f, 0.24f}, {X_FRONT, 0.27f}}); // underbody
    static const Curve Ys({{X_REAR, 0.90f}, {-1.9f, 0.965f}, {-1.2f, 0.965f}, {-0.8f, 0.955f},
        {0.0f, 0.945f}, {0.8f, 0.94f}, {1.2f, 0.92f}, {1.6f, 0.885f}, {X_FRONT, 0.80f}});            // belt line
    static const Curve Wm({{X_REAR, 0.78f}, {-2.0f, 0.82f}, {-1.6f, 0.845f}, {-0.5f, 0.855f},
        {0.8f, 0.85f}, {1.6f, 0.845f}, {2.0f, 0.83f}, {X_FRONT, 0.79f}});                              // max half width
    static const Curve Wt({{X_REAR, 0.60f}, {-2.0f, 0.70f}, {-1.75f, 0.76f}, {-1.5f, 0.74f},
        {-1.3f, 0.68f}, {-1.0f, 0.63f}, {-0.8f, 0.615f}, {0.0f, 0.61f}, {0.3f, 0.635f}, {0.6f, 0.69f},
        {0.9f, 0.745f}, {1.1f, 0.76f}, {1.8f, 0.75f}, {X_FRONT, 0.62f}});                               // top half width

    // plan-view rounding of nose and tail: rib width -> 0 at the very ends (superellipse)
    auto cap = [](float x) {
        const float rf = 0.40f, rr = 0.36f, p = 2.4f;
        float d = 0.f;
        if (x > X_FRONT - rf) d = (x - (X_FRONT - rf)) / rf;
        else if (x < X_REAR + rr) d = ((X_REAR + rr) - x) / rr;
        d = std::min(d, 1.f);
        return std::pow(std::max(0.f, 1.f - std::pow(d, p)), 1.f / p);
    };

    const int NS = 180, K = 7;
    Mesh m;
    int N = 0;
    for (int s = 0; s <= NS; s++) {
        float u = (float)s / NS;
        float x = X_REAR + (X_FRONT - X_REAR) * (0.5f - 0.5f * std::cos(PI * u));   // dense at both ends
        float yt = Yt(x), yb = Yb(x), wm = Wm(x), wt = Wt(x);
        float sy = std::min(Ys(x), yt - 0.045f);           // belt / shoulder height
        float crown = 0.04f, wsh = wm - 0.012f, wl = wm - 0.07f;
        float gh = yt - sy;                                // greenhouse height
        float rTC = glm::mix(0.045f, 0.085f, glm::smoothstep(0.08f, 0.35f, gh));
        float ysm = yb + 0.55f * (sy - yb);
        // (z, y) of the rib's control polygon, right half then mirrored
        std::vector<vec2> V = {{0, yt}, {0.5f * wt, yt - 0.25f * crown}, {wt, yt - crown}, {wsh, sy},
            {wm, ysm}, {wl, yb}, {-wl, yb}, {-wm, ysm}, {-wsh, sy}, {-wt, yt - crown},
            {-0.5f * wt, yt - 0.25f * crown}};
        std::vector<float> R = {0, 0, rTC, 0.035f, 0, 0.08f, 0.08f, 0, 0.035f, rTC, 0};
        std::vector<vec2> ring; std::vector<int> mid;
        buildRing(V, R, K, ring, mid);
        N = (int)ring.size();
        std::vector<float> cum(N, 0.f);
        for (int j = 1; j < N; j++) cum[j] = cum[j - 1] + glm::length(ring[j] - ring[j - 1]);
        float perim = cum[N - 1] + glm::length(ring[0] - ring[N - 1]);
        float sTC = cum[mid[2]], sSH = cum[mid[3]];
        float c = cap(x);
        for (int j = 0; j < N; j++) {
            float sj = std::min(cum[j], perim - cum[j]);
            Vertex q;
            q.p = vec3(x, ring[j].y, ring[j].x * c);
            q.n = vec3(0, 1, 0);
            q.uv = vec2(sj - sTC, sSH - sj);
            q.mat = (float)BODY;
            m.v.push_back(q);
        }
    }
    for (int s = 0; s < NS; s++)
        for (int j = 0; j < N; j++) {
            unsigned a = s * N + j, b = s * N + (j + 1) % N, d = (s + 1) * N + j, c = (s + 1) * N + (j + 1) % N;
            m.i.insert(m.i.end(), {a, b, d, b, c, d});
        }
    smoothNormals(m);
    return m;
}

// ------------------------------------------------------------ primitives
// Surface of revolution around local Z. profile = (radius, axial position).
inline Mesh lathe(const std::vector<vec2>& prof, int seg, int mat, float a0 = 0.f, float a1 = 2 * PI) {
    Mesh m;
    int n = (int)prof.size();
    for (int s = 0; s <= seg; s++) {
        float a = a0 + (a1 - a0) * s / seg, c = std::cos(a), sn = std::sin(a);
        for (int k = 0; k < n; k++)
            m.v.push_back({vec3(prof[k].x * c, prof[k].x * sn, prof[k].y), vec3(0, 1, 0), vec2(prof[k].x, a), (float)mat});
    }
    for (int s = 0; s < seg; s++)
        for (int k = 0; k < n - 1; k++) {
            unsigned a = s * n + k, b = a + 1, c = (s + 1) * n + k, d = c + 1;
            m.i.insert(m.i.end(), {a, c, b, b, c, d});
        }
    smoothNormals(m);
    weld(m);
    return m;
}

inline float spow(float v, float e) { return (v < 0 ? -1.f : 1.f) * std::pow(std::fabs(v), e); }

// rounded box / ellipsoid family (e = 1 ellipsoid, e -> 0 box)
inline Mesh superEllipsoid(vec3 r, float e1, float e2, int mat, int nu = 40, int nv = 24) {
    Mesh m;
    for (int i = 0; i <= nu; i++) {
        float u = -PI + 2 * PI * i / nu;
        for (int j = 0; j <= nv; j++) {
            float v = -PI / 2 + PI * j / nv;
            float cv = std::cos(v), sv = std::sin(v), cu = std::cos(u), su = std::sin(u);
            vec3 p(r.x * spow(cv, e1) * spow(cu, e2), r.y * spow(sv, e1), r.z * spow(cv, e1) * spow(su, e2));
            m.v.push_back({p, vec3(0, 1, 0), vec2(0), (float)mat});
        }
    }
    for (int i = 0; i < nu; i++)
        for (int j = 0; j < nv; j++) {
            unsigned a = i * (nv + 1) + j, b = a + 1, c = (i + 1) * (nv + 1) + j, d = c + 1;
            m.i.insert(m.i.end(), {a, c, b, b, c, d});
        }
    smoothNormals(m);
    weld(m);
    return m;
}

// ------------------------------------------------------------------ wheel
inline Mesh buildWheel() {
    Mesh w;
    w.append(lathe({{0.196f, -0.086f}, {0.215f, -0.093f}, {0.250f, -0.0975f}, {0.280f, -0.090f},
                    {0.296f, -0.070f}, {0.300f, -0.045f}, {0.300f, 0.045f}, {0.296f, 0.070f},
                    {0.280f, 0.090f}, {0.250f, 0.0975f}, {0.215f, 0.093f}, {0.196f, 0.086f}}, 96, RUBBER));
    w.append(lathe({{0.196f, -0.088f}, {0.185f, -0.075f}, {0.180f, 0.0f}, {0.185f, 0.070f},
                    {0.192f, 0.086f}}, 64, ALLOY));                                   // rim barrel
    w.append(lathe({{0.186f, 0.084f}, {0.150f, 0.075f}, {0.100f, 0.064f}, {0.062f, 0.057f},
                    {0.030f, 0.055f}, {0.0f, 0.058f}}, 120, SPOKE));                  // 5-spoke face
    w.append(lathe({{0.060f, 0.030f}, {0.145f, 0.030f}, {0.145f, 0.040f}, {0.060f, 0.040f}}, 64, BRAKE));
    return w;
}

// dark wheel-house bowl so you never see the cabin through the arch
inline Mesh buildLiner() {
    return lathe({{0.0f, -0.187f}, {0.345f, -0.187f}, {0.345f, 0.10f}}, 40, BLACK, -0.45f, PI + 0.45f);
}

// ------------------------------------------------------------------ whole car
inline Mesh buildCar() {
    Mesh all;
    all.append(buildBody());

    // wheels (mirrored for the left side)
    for (int ax = 0; ax < 2; ax++)
        for (int sd = 0; sd < 2; sd++) {
            float x = ax == 0 ? AXLE_X : -AXLE_X, zs = sd == 0 ? 1.f : -1.f;
            Mesh w = buildWheel();
            w.append(buildLiner());
            mat4 T = glm::translate(mat4(1), vec3(x, WHEEL_R, TRACK_Z * zs)) * glm::scale(mat4(1), vec3(1, 1, zs)) *
                     glm::rotate(mat4(1), 0.35f * (ax * 2 + sd), vec3(0, 0, 1));
            transform(w, T);
            all.append(w);
        }

    auto put = [&](Mesh m, vec3 pos, float pitchDeg = 0.f) {
        transform(m, glm::translate(mat4(1), pos) * glm::rotate(mat4(1), glm::radians(pitchDeg), vec3(0, 0, 1)));
        all.append(m);
    };

    // side mirrors
    for (int s = -1; s <= 1; s += 2) put(superEllipsoid(vec3(0.045f, 0.065f, 0.105f), 0.7f, 0.8f, PAINT), vec3(0.74f, 1.0f, 0.955f * s), 0);

    // ---- interior (seen through the glass) ----
    put(superEllipsoid(vec3(0.30f, 0.085f, 0.70f), 0.45f, 0.45f, CABIN), vec3(0.66f, 0.84f, 0.0f));      // dashboard
    for (int s = -1; s <= 1; s += 2) {
        float z = 0.36f * s;
        put(superEllipsoid(vec3(0.23f, 0.07f, 0.21f), 0.35f, 0.35f, FABRIC), vec3(-0.12f, 0.50f, z));      // cushion
        put(superEllipsoid(vec3(0.07f, 0.29f, 0.21f), 0.35f, 0.35f, FABRIC), vec3(-0.40f, 0.80f, z), 12);  // backrest
        put(superEllipsoid(vec3(0.05f, 0.09f, 0.11f), 0.5f, 0.5f, FABRIC), vec3(-0.47f, 1.16f, z), 8);     // headrest
    }
    put(superEllipsoid(vec3(0.22f, 0.07f, 0.62f), 0.35f, 0.35f, FABRIC), vec3(-0.98f, 0.50f, 0.f));        // rear bench
    put(superEllipsoid(vec3(0.07f, 0.27f, 0.62f), 0.35f, 0.35f, FABRIC), vec3(-1.20f, 0.80f, 0.f), 14);
    {   // steering wheel (right-hand drive)
        std::vector<vec2> prof;
        for (int k = 0; k <= 12; k++) { float a = 2 * PI * k / 12; prof.push_back({0.17f + 0.018f * std::cos(a), 0.018f * std::sin(a)}); }
        Mesh sw = lathe(prof, 36, BLACK);
        transform(sw, glm::translate(mat4(1), vec3(0.46f, 0.90f, 0.36f)) * glm::rotate(mat4(1), glm::radians(-65.f), vec3(0, 0, 1)) *
                          glm::rotate(mat4(1), glm::radians(90.f), vec3(0, 1, 0)));
        all.append(sw);
    }

    // ---- roof sign "Driving License Test" + 3 beacons ----
    {
        const vec3 r(0.075f, 0.07f, 0.29f);
        Mesh sign = superEllipsoid(r, 0.20f, 0.22f, SIGN, 64, 32);
        for (auto& q : sign.v) {                              // planar UV for the lettering on the +/-X faces
            q.uv.x = (q.p.x >= 0 ? 0.5f - q.p.z / (2 * r.z) : 0.5f + q.p.z / (2 * r.z));
            q.uv.y = 0.5f + q.p.y / (2 * r.y);
        }
        put(sign, vec3(-0.18f, 1.575f, 0.f));
        for (int s = -1; s <= 1; s += 2) put(superEllipsoid(vec3(0.045f, 0.03f, 0.022f), 0.4f, 0.4f, BLACK), vec3(-0.18f, 1.49f, 0.22f * s));
        const int cols[3] = {BEACON_B, BEACON_A, BEACON_R};
        for (int k = 0; k < 3; k++) put(superEllipsoid(vec3(0.034f, 0.036f, 0.034f), 0.8f, 0.8f, cols[k]), vec3(-0.18f, 1.655f, (k - 1) * 0.15f));
    }
    return all;
}

// ------------------------------------------------------------------ OBJ export
inline const char* matName(int m) {
    static const char* n[] = {"body_glass_paint", "glass", "rubber", "alloy", "chrome", "black_plastic", "lamp_head", "lamp_tail",
                              "cabin", "sign", "beacon_blue", "beacon_amber", "beacon_red", "brake", "fabric", "paint", "wheel_spokes"};
    return (m >= 0 && m <= 16) ? n[m] : "other";
}
inline bool writeOBJ(const Mesh& m, const char* path) {
    FILE* f = std::fopen(path, "w");
    if (!f) return false;
    std::fprintf(f, "# Toyota Vios XP90 driving-test car (procedural, lofted from cross-sections)\n");
    for (auto& v : m.v) std::fprintf(f, "v %.5f %.5f %.5f\n", v.p.x, v.p.y, v.p.z);
    for (auto& v : m.v) std::fprintf(f, "vn %.4f %.4f %.4f\n", v.n.x, v.n.y, v.n.z);
    for (int mat = 0; mat <= 16; mat++) {
        bool any = false;
        for (size_t k = 0; k + 2 < m.i.size(); k += 3) {
            if ((int)(m.v[m.i[k]].mat + 0.5f) != mat) continue;
            if (!any) { std::fprintf(f, "usemtl %s\n", matName(mat)); any = true; }
            std::fprintf(f, "f %u//%u %u//%u %u//%u\n", m.i[k] + 1, m.i[k] + 1, m.i[k + 1] + 1, m.i[k + 1] + 1, m.i[k + 2] + 1, m.i[k + 2] + 1);
        }
    }
    std::fclose(f);
    return true;
}
} // namespace car
