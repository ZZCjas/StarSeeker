#pragma once
#include <vector>
#include <string>
#include <cstring>
#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>
#include "astronomy.h"
#include "config.h"
#include "language.h"

enum ObjectType { STAR, PLANET, MOON, SATELLITE, DEEPSKY, SUN, COMET, ASTEROID };

enum DeepSkyType {
    DS_GALAXY = 0,
    DS_NEBULA,
    DS_OPEN_CLUSTER,
    DS_GLOBULAR_CLUSTER,
    DS_PLANETARY_NEBULA,
    DS_SUPERNOVA_REMNANT,
    DS_UNKNOWN
};

struct CelestialObject {
    std::string name;
    std::string name_zh;
    ObjectType type;
    double ra;
    double dec;
    double distance;
    double mag;
    bool dynamic;
    int id;
    std::string tle_line1, tle_line2;
    DeepSkyType ds_type;
    double bv_color;
};

// ----- Static catalog loading -----
static inline std::vector<CelestialObject> load_stars(const std::string &filename) {
    std::vector<CelestialObject> stars;
    std::ifstream f(filename);
    if (!f) return stars;
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream ss(line);
        CelestialObject s;
        s.type = STAR;
        s.dynamic = false;
        s.ds_type = DS_UNKNOWN;
        s.bv_color = -1.0;
        double ra_h, ra_m, ra_s, dec_d, dec_m, dec_s;
        std::getline(ss, s.name, ',');
        std::getline(ss, s.name_zh, ',');
        ss >> ra_h; ss.ignore(); ss >> ra_m; ss.ignore(); ss >> ra_s; ss.ignore();
        ss >> dec_d; ss.ignore(); ss >> dec_m; ss.ignore(); ss >> dec_s; ss.ignore();
        ss >> s.distance; ss.ignore(); ss >> s.mag;
        if (ss.peek() == ',') {
            ss.ignore();
            ss >> s.bv_color;
            if (ss.fail()) { s.bv_color = -1.0; ss.clear(); }
        }
        s.ra = (ra_h + ra_m / 60.0 + ra_s / 3600.0) * 15.0 * M_PI / 180.0;
        double dec_deg = fabs(dec_d) + dec_m / 60.0 + dec_s / 3600.0;
        if (dec_d < 0) dec_deg = -dec_deg;
        s.dec = dec_deg * M_PI / 180.0;
        if (!s.name.empty()) stars.push_back(s);
    }
    return stars;
}

static inline DeepSkyType parse_ds_type(const std::string &typeStr) {
    if (typeStr == "Galaxy") return DS_GALAXY;
    if (typeStr == "Nebula") return DS_NEBULA;
    if (typeStr == "OpenCluster") return DS_OPEN_CLUSTER;
    if (typeStr == "GlobularCluster") return DS_GLOBULAR_CLUSTER;
    if (typeStr == "PlanetaryNebula") return DS_PLANETARY_NEBULA;
    if (typeStr == "SupernovaRemnant") return DS_SUPERNOVA_REMNANT;
    return DS_UNKNOWN;
}

static inline std::vector<CelestialObject> load_messier(const std::string &filename) {
    std::vector<CelestialObject> objs;
    std::ifstream f(filename);
    if (!f) return objs;
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream ss(line);
        CelestialObject m;
        m.type = DEEPSKY;
        m.dynamic = false;
        m.bv_color = -1.0;
        std::getline(ss, m.name, ',');
        std::getline(ss, m.name_zh, ',');
        std::string typeStr;
        std::getline(ss, typeStr, ',');
        m.ds_type = parse_ds_type(typeStr);
        double ra_h, ra_m, ra_s, dec_d, dec_m, dec_s;
        ss >> ra_h; ss.ignore(); ss >> ra_m; ss.ignore(); ss >> ra_s; ss.ignore();
        ss >> dec_d; ss.ignore(); ss >> dec_m; ss.ignore(); ss >> dec_s; ss.ignore();
        ss >> m.mag;
        if (ss.peek() == ',') {
            ss.ignore();
            ss >> m.distance;
            if (ss.fail()) { m.distance = 0; ss.clear(); }
        } else {
            m.distance = 0;
        }
        m.ra = (ra_h + ra_m / 60.0 + ra_s / 3600.0) * 15.0 * M_PI / 180.0;
        double dec_deg = fabs(dec_d) + dec_m / 60.0 + dec_s / 3600.0;
        if (dec_d < 0) dec_deg = -dec_deg;
        m.dec = dec_deg * M_PI / 180.0;
        if (!m.name.empty()) objs.push_back(m);
    }
    return objs;
}

// ============================================================
// 行星轨道要素（J2000，含长期变化）
// ============================================================
struct OrbitElements {
    double a;
    double e;
    double I;
    double L;
    double wbar;
    double Omega;
    double epoch;
    double da, de, dI, dL, dw, dOmega;
};

static const OrbitElements planet_elements[9] = {
    // Mercury
    {0.387099, 0.205635, 7.0047, 252.2509, 77.4561, 48.3308, 2451545.0, 0, 0.000019, -0.0059, 149472.674, 0.1600, -0.1253},
    // Venus
    {0.723332, 0.006773, 3.3946, 181.9798, 131.6024, 76.6799, 2451545.0, 0, -0.000041, -0.0011, 58517.815, 0.0027, -0.2772},
    // Earth
    {1.000001, 0.016710, 0.00005, 100.4644, 102.9472, 0.0, 2451545.0, 0, -0.000038, -0.0131, 35999.372, 0.3232, 0},
    // Mars
    {1.523680, 0.093412, 1.8497, -4.5685, 336.0602, 49.6674, 2451545.0, 0, 0.000119, -0.0081, 19140.299, 0.4443, -0.2926},
    // Jupiter
    {5.20259, 0.048498, 1.3033, 34.3965, 14.3072, 100.4540, 2451545.0, 0, -0.000031, -0.0128, 3034.903, 0.2125, -0.1603},
    // Saturn
    {9.55491, 0.055508, 2.4886, 49.9542, 92.2641, 113.6624, 2451545.0, 0, -0.000041, 0.0040, 1222.116, -0.1566, -0.2567},
    // Uranus
    {19.21845, 0.046296, 0.7734, 313.2381, 170.9542, 74.0161, 2451545.0, 0, -0.000027, 0.0020, 428.467, 0.0204, -0.0959},
    // Neptune
    {30.11038, 0.008598, 1.7700, -55.1203, 37.4051, 131.7841, 2451545.0, 0, 0.000016, 0.0008, 218.459, -0.0093, -0.0792},
    // Pluto
    {39.445, 0.250, 17.14, 244.0, 224.0, 110.0, 2451545.0, 0, 0, 0, 146.0, -0.45, -0.40}
};

// ============================================================
// 行星卫星轨道要素
// ============================================================
struct MoonOrbitElements {
    double a;
    double e;
    double I;
    double Omega;
    double w;
    double M0;
    double period;
    double mag;
    const char* name;
    const char* name_zh;
    int parent_planet;
    double planet_pole_ra;
    double planet_pole_dec;
};

static const MoonOrbitElements planet_moons[] = {
    // Mars moons - Mars pole: RA=317.7, Dec=52.9
    {9378,    0.0151, 1.093,  0.0,   0.0,   0.0, 0.31891, 11.3, "Phobos",    "火卫一", 4, 317.7, 52.9},
    {23459,   0.0005, 0.930,  0.0,   0.0,   0.0, 1.26244, 12.4, "Deimos",    "火卫二", 4, 317.7, 52.9},
    // Jupiter Galilean moons - Jupiter pole: RA=268.1, Dec=64.5
    {421800,  0.0041, 0.040,  0.0,   0.0,   0.0, 1.76914, 5.0,  "Io",        "木卫一", 5, 268.1, 64.5},
    {671100,  0.0094, 0.470,  0.0,   0.0,   0.0, 3.55118, 5.3,  "Europa",    "木卫二", 5, 268.1, 64.5},
    {1070400, 0.0011, 0.180,  0.0,   0.0,   0.0, 7.15455, 4.6,  "Ganymede",  "木卫三", 5, 268.1, 64.5},
    {1882700, 0.0074, 0.190,  0.0,   0.0,   0.0, 16.689,  5.6,  "Callisto",  "木卫四", 5, 268.1, 64.5},
    // Saturn moons - Saturn pole: RA=40.6, Dec=83.5
    {185539,  0.0202, 1.530,  0.0,   0.0,   0.0, 0.94242, 12.9, "Mimas",     "土卫一", 6, 40.6, 83.5},
    {237948,  0.0045, 0.020,  0.0,   0.0,   0.0, 1.37022, 11.7, "Enceladus", "土卫二", 6, 40.6, 83.5},
    {294619,  0.0000, 1.860,  0.0,   0.0,   0.0, 1.88780, 10.2, "Tethys",    "土卫三", 6, 40.6, 83.5},
    {377396,  0.0022, 0.020,  0.0,   0.0,   0.0, 2.73692, 10.4, "Dione",     "土卫四", 6, 40.6, 83.5},
    {527108,  0.0013, 0.350,  0.0,   0.0,   0.0, 4.51750, 9.7,  "Rhea",      "土卫五", 6, 40.6, 83.5},
    {1221870, 0.0288, 0.330,  0.0,   0.0,   0.0, 15.945,  8.4,  "Titan",     "土卫六", 6, 40.6, 83.5},
    {3560820, 0.0286, 15.55,  0.0,   0.0,   0.0, 79.330,  11.0, "Iapetus",   "土卫八", 6, 40.6, 83.5},
};
static const int NUM_PLANET_MOONS = sizeof(planet_moons) / sizeof(planet_moons[0]);

// ============================================================
// 小行星轨道要素
// ============================================================
struct AsteroidElements {
    double a;       // AU
    double e;
    double I;       // deg
    double Omega;   // deg
    double omega;   // deg (argument of perihelion)
    double M0;      // deg (mean anomaly at J2000)
    double period;  // days
    const char* name;
    const char* name_zh;
    double mag;
};

static const AsteroidElements asteroids[] = {
    {2.766, 0.079, 10.59, 80.3, 73.6, 95.99, 1680.0, "Ceres", "Ceres", 7.0},
    {2.362, 0.089, 7.14, 103.9, 151.0, 47.68, 1325.0, "Vesta", "Vesta", 6.5},
    {2.773, 0.231, 34.84, 173.1, 309.9, 170.12, 1686.0, "Pallas", "Pallas", 8.0},
};
static const int NUM_ASTEROIDS = sizeof(asteroids) / sizeof(asteroids[0]);

// Compute planet ecliptic heliocentric position (AU) at JD
static inline void planet_heliocentric(double jd, int planet_idx, double &x, double &y, double &z) {
    const OrbitElements &o = planet_elements[planet_idx];
    double T = (jd - o.epoch) / 36525.0;
    double a = o.a;
    double e = o.e + o.de * T;
    double I = (o.I + o.dI * T) * M_PI / 180.0;
    double L = fmod(o.L + o.dL * T, 360.0) * M_PI / 180.0;
    double wbar = fmod(o.wbar + o.dw * T, 360.0) * M_PI / 180.0;
    double Omega = fmod(o.Omega + o.dOmega * T, 360.0) * M_PI / 180.0;
    double w = wbar - Omega;
    double M = fmod(L - wbar, 2 * M_PI);
    if (M < 0) M += 2 * M_PI;
    double E = M;
    for (int i = 0; i < 15; ++i) {
        double dE = (M - E + e * sin(E)) / (1 - e * cos(E));
        E += dE;
        if (fabs(dE) < 1e-12) break;
    }
    double cosE = cos(E), sinE = sin(E);
    double nu = atan2(sqrt(1 - e * e) * sinE, cosE - e);
    double r = a * (1 - e * cosE);
    double x_orb = r * cos(nu + w);
    double y_orb = r * sin(nu + w);
    double z_orb = 0;
    x = x_orb * cos(Omega) - y_orb * cos(I) * sin(Omega);
    y = x_orb * sin(Omega) + y_orb * cos(I) * cos(Omega);
    z = y_orb * sin(I);
}

// Asteroid heliocentric position
static inline void asteroid_heliocentric(double jd, int ast_idx, double &x, double &y, double &z) {
    const AsteroidElements &o = asteroids[ast_idx];
    double M = fmod(o.M0 + (jd - 2451545.0) / o.period * 360.0, 360.0) * M_PI / 180.0;
    if (M < 0) M += 2 * M_PI;
    double e = o.e;
    double E = M;
    for (int i = 0; i < 15; ++i) {
        double dE = (M - E + e * sin(E)) / (1 - e * cos(E));
        E += dE;
        if (fabs(dE) < 1e-12) break;
    }
    double nu = atan2(sqrt(1 - e*e) * sin(E), cos(E) - e);
    double r = o.a * (1 - e * cos(E));
    double I = o.I * M_PI / 180.0;
    double Omega = o.Omega * M_PI / 180.0;
    double w = o.omega * M_PI / 180.0;
    double x_orb = r * cos(nu + w);
    double y_orb = r * sin(nu + w);
    x = x_orb * cos(Omega) - y_orb * cos(I) * sin(Omega);
    y = x_orb * sin(Omega) + y_orb * cos(I) * cos(Omega);
    z = y_orb * sin(I);
}

// ============================================================
// 太阳系天体位置 — 增加光行时间修正（2次迭代）
// ============================================================
static inline void solar_system_position(double jd, int body_id, double &ra, double &dec, double &dist_au) {
    if (body_id == 0) { // Sun
        double x_e, y_e, z_e;
        planet_heliocentric(jd, 2, x_e, y_e, z_e);
        double x_s = -x_e, y_s = -y_e, z_s = -z_e;
        dist_au = sqrt(x_s * x_s + y_s * y_s + z_s * z_s);
        // 光行时间修正（2次迭代）
        double light_time = dist_au * 499.005 / 86400.0;
        for (int iter = 0; iter < 2; ++iter) {
            planet_heliocentric(jd - light_time, 2, x_e, y_e, z_e);
            x_s = -x_e; y_s = -y_e; z_s = -z_e;
            dist_au = sqrt(x_s * x_s + y_s * y_s + z_s * z_s);
            light_time = dist_au * 499.005 / 86400.0;
        }
        double ecl_obl = true_obliquity(jd);
        double xecl = x_s, yecl = y_s * cos(ecl_obl) - z_s * sin(ecl_obl), zecl = y_s * sin(ecl_obl) + z_s * cos(ecl_obl);
        ra = atan2(yecl, xecl);
        if (ra < 0) ra += 2 * M_PI;
        dec = atan2(zecl, sqrt(xecl * xecl + yecl * yecl));
        apply_nutation_to_radec(jd, ra, dec);
        return;
    }

    if (body_id == 1) { // Moon
        double T = (jd - 2451545.0) / 36525.0;
        double Lp = fmod(218.3164563 + 481267.88134236 * T, 360.0) * M_PI / 180.0;
        double D  = fmod(297.8501921 + 445267.1114034 * T, 360.0) * M_PI / 180.0;
        double M  = fmod(357.5291092 + 35999.0502909 * T, 360.0) * M_PI / 180.0;
        double Mm = fmod(134.9633964 + 477198.8675055 * T, 360.0) * M_PI / 180.0;
        double F  = fmod(93.2720950 + 483202.0175233 * T, 360.0) * M_PI / 180.0;

        struct LunarTerm {
            int dD, dM, dMm, dF;
            double coeff_sine;
            double coeff_cosine;
        };
        static const LunarTerm longitude_terms[] = {
            {0,  0,  0,  1,  -0.0048,  0.0},
            {2,  0,  0, -1,   6.289,   0.0},
            {2,  0,  0,  1,   1.274,   0.0},
            {0,  0,  0,  2,  -0.228,   0.0},
            {2,  0,  0,  2,  -0.280,   0.0},
            {2, -1,  0, -1,   0.658,   0.0},
            {2,  0, -1, -1,   0.342,   0.0},
            {4,  0,  0, -1,  -0.186,   0.0},
            {0,  1,  0,  0,  -0.114,   0.0},
            {4,  0,  0, -2,  -0.059,   0.0},
            {4, -1,  0, -1,   0.057,   0.0},
            {1,  0,  0,  0,   0.046,   0.0},
        };
        static const LunarTerm latitude_terms[] = {
            {0,  0,  0,  1,   5.128,   0.0},
            {0,  0,  1, -1,   0.281,   0.0},
            {2,  0,  0, -1,  -0.280,   0.0},
            {2,  0,  0,  1,  -0.277,   0.0},
            {2,  0, -1, -1,  -0.174,   0.0},
            {2, -1,  0, -1,   0.173,   0.0},
            {4,  0,  0, -1,   0.055,   0.0},
            {0,  0,  2,  1,   0.046,   0.0},
            {0,  0,  2, -1,   0.041,   0.0},
        };

        double delta_lambda = 0;
        for (const auto &t : longitude_terms) {
            double angle = t.dD * D + t.dM * M + t.dMm * Mm + t.dF * F;
            delta_lambda += t.coeff_sine * sin(angle);
        }
        double lambda = Lp + delta_lambda * M_PI / 180.0;

        double beta = 0;
        for (const auto &t : latitude_terms) {
            double angle = t.dD * D + t.dM * M + t.dMm * Mm + t.dF * F;
            beta += t.coeff_sine * sin(angle);
        }
        beta = beta * M_PI / 180.0;

        double dist_km = 385000.66
            + 20905.0  * cos(Mm)
            - 3699.0   * cos(2 * D - Mm)
            - 2956.0   * cos(2 * D)
            - 570.0    * cos(2 * Mm)
            + 246.0    * cos(2 * D - 2 * Mm)
            - 205.0    * cos(M - Mm)
            - 171.0    * cos(2 * D + Mm);
        dist_au = dist_km / 149597870.7;

        double ecl_obl = true_obliquity(jd);
        double xe = cos(lambda) * cos(beta);
        double ye = sin(lambda) * cos(beta) * cos(ecl_obl) - sin(beta) * sin(ecl_obl);
        double ze = sin(lambda) * cos(beta) * sin(ecl_obl) + sin(beta) * cos(ecl_obl);
        ra = atan2(ye, xe);
        if (ra < 0) ra += 2 * M_PI;
        dec = atan2(ze, sqrt(xe * xe + ye * ye));
        apply_nutation_to_radec(jd, ra, dec);
        return;
    }

    // Planets (2=Mercury to 9=Pluto)
    int planet_idx = body_id - 2;
    if (planet_idx < 0 || planet_idx > 7) return;

    // 光行时间修正（2次迭代）
    double x_pl, y_pl, z_pl, x_e, y_e, z_e;
    planet_heliocentric(jd, planet_idx, x_pl, y_pl, z_pl);
    planet_heliocentric(jd, 2, x_e, y_e, z_e);
    double dx = x_pl - x_e;
    double dy = y_pl - y_e;
    double dz = z_pl - z_e;
    double est_dist = sqrt(dx*dx + dy*dy + dz*dz);
    double light_time = est_dist * 499.005 / 86400.0;

    for (int iter = 0; iter < 2; ++iter) {
        planet_heliocentric(jd - light_time, planet_idx, x_pl, y_pl, z_pl);
        planet_heliocentric(jd - light_time, 2, x_e, y_e, z_e);
        dx = x_pl - x_e;
        dy = y_pl - y_e;
        dz = z_pl - z_e;
        est_dist = sqrt(dx*dx + dy*dy + dz*dz);
        light_time = est_dist * 499.005 / 86400.0;
    }

    dist_au = est_dist;
    double ecl_obl = true_obliquity(jd);
    double xeq = dx;
    double yeq = dy * cos(ecl_obl) - dz * sin(ecl_obl);
    double zeq = dy * sin(ecl_obl) + dz * cos(ecl_obl);
    ra = atan2(yeq, xeq);
    if (ra < 0) ra += 2 * M_PI;
    dec = atan2(zeq, sqrt(xeq * xeq + yeq * yeq));
    apply_nutation_to_radec(jd, ra, dec);
}

// ============================================================
// 【修正】行星卫星相对位置 — 使用升交点方向构建旋转矩阵
// ============================================================
static inline void moon_relative(double jd, int moon_idx, double &dx, double &dy, double &dz) {
    const MoonOrbitElements &m = planet_moons[moon_idx];

    double M = fmod(m.M0 + (jd - 2451545.0) / m.period * 360.0, 360.0) * M_PI / 180.0;
    if (M < 0) M += 2 * M_PI;
    double e = m.e;
    double I = m.I * M_PI / 180.0;
    double Omega = m.Omega * M_PI / 180.0;
    double w = m.w * M_PI / 180.0;

    double E = M;
    for (int i = 0; i < 12; ++i) {
        double dE = (M - E + e * sin(E)) / (1 - e * cos(E));
        E += dE;
        if (fabs(dE) < 1e-10) break;
    }

    double nu = atan2(sqrt(1 - e*e) * sin(E), cos(E) - e);
    double r = m.a * (1 - e * cos(E)); // km

    double x_orb = r * cos(nu + w);
    double y_orb = r * sin(nu + w);

    // 行星赤道坐标系中的位置（z轴为行星北极）
    double x1 = x_orb * cos(Omega) - y_orb * cos(I) * sin(Omega);
    double y1 = x_orb * sin(Omega) + y_orb * cos(I) * cos(Omega);
    double z1 = y_orb * sin(I);

    // 构造从行星赤道坐标系到黄道坐标系的旋转矩阵
    // z轴 = 行星北极方向
    double pole_ra = m.planet_pole_ra * M_PI / 180.0;
    double pole_dec = m.planet_pole_dec * M_PI / 180.0;
    double z_axis[3] = {
        cos(pole_dec) * cos(pole_ra),
        cos(pole_dec) * sin(pole_ra),
        sin(pole_dec)
    };

    // x轴 = 升交点方向：黄道法线 (0,0,1) 与 z_axis 的叉积
    double y_temp[3] = {0, 0, 1};
    double x_axis[3] = {
        y_temp[1]*z_axis[2] - y_temp[2]*z_axis[1],
        y_temp[2]*z_axis[0] - y_temp[0]*z_axis[2],
        y_temp[0]*z_axis[1] - y_temp[1]*z_axis[0]
    };
    double x_len = sqrt(x_axis[0]*x_axis[0] + x_axis[1]*x_axis[1] + x_axis[2]*x_axis[2]);
    if (x_len > 1e-12) {
        x_axis[0] /= x_len; x_axis[1] /= x_len; x_axis[2] /= x_len;
    } else {
        // 若行星极点接近黄道法线，则退化使用原来的方法
        double x_temp[3] = {1, 0, 0};
        double dot = x_temp[0]*z_axis[0] + x_temp[1]*z_axis[1] + x_temp[2]*z_axis[2];
        x_axis[0] = x_temp[0] - dot*z_axis[0];
        x_axis[1] = x_temp[1] - dot*z_axis[1];
        x_axis[2] = x_temp[2] - dot*z_axis[2];
        x_len = sqrt(x_axis[0]*x_axis[0] + x_axis[1]*x_axis[1] + x_axis[2]*x_axis[2]);
        if (x_len > 1e-12) {
            x_axis[0] /= x_len; x_axis[1] /= x_len; x_axis[2] /= x_len;
        } else {
            x_axis[0] = 1; x_axis[1] = 0; x_axis[2] = 0;
        }
    }

    // y轴 = z × x
    double y_axis[3] = {
        z_axis[1]*x_axis[2] - z_axis[2]*x_axis[1],
        z_axis[2]*x_axis[0] - z_axis[0]*x_axis[2],
        z_axis[0]*x_axis[1] - z_axis[1]*x_axis[0]
    };

    const double KM_PER_AU = 149597870.7;
    dx = (x_axis[0]*x1 + y_axis[0]*y1 + z_axis[0]*z1) / KM_PER_AU;
    dy = (x_axis[1]*x1 + y_axis[1]*y1 + z_axis[1]*z1) / KM_PER_AU;
    dz = (x_axis[2]*x1 + y_axis[2]*y1 + z_axis[2]*z1) / KM_PER_AU;
}

// Get geocentric equatorial position for planetary moon
static inline void planetary_moon_position(double jd, int moon_idx,
                                           double &ra, double &dec, double &dist_au) {
    const MoonOrbitElements &m = planet_moons[moon_idx];

    double px, py, pz;
    planet_heliocentric(jd, m.parent_planet - 2, px, py, pz);
    double ex, ey, ez;
    planet_heliocentric(jd, 2, ex, ey, ez);

    double mx, my, mz;
    moon_relative(jd, moon_idx, mx, my, mz);

    double mhx = px + mx;
    double mhy = py + my;
    double mhz = pz + mz;

    double dx = mhx - ex;
    double dy = mhy - ey;
    double dz = mhz - ez;

    dist_au = sqrt(dx*dx + dy*dy + dz*dz);

    double ecl_obl = true_obliquity(jd);
    double xeq = dx;
    double yeq = dy * cos(ecl_obl) - dz * sin(ecl_obl);
    double zeq = dy * sin(ecl_obl) + dz * cos(ecl_obl);

    ra = atan2(yeq, xeq);
    if (ra < 0) ra += 2 * M_PI;
    dec = atan2(zeq, sqrt(xeq*xeq + yeq*yeq));
    apply_nutation_to_radec(jd, ra, dec);
}

// ============================================================
// 简化 SGP4 — 修正单位处理
// ============================================================
#define SGP4_DEG2RAD (M_PI/180.0)
#define SGP4_XJ2 1.082616e-3
#define SGP4_XKE 0.0743669161331734132
#define SGP4_XKMPER 6378.135
#define SGP4_XMNPDA 1440.0
#define SGP4_XJ3 -2.53881e-6

struct tle_t {
    char name[32];
    int epoch_year;
    double epoch_day;
    double bstar;
    double incl, raan, ecc, argp, mean_anom, mean_motion;
};

static inline int parse_tle(const std::string &name, const std::string &line1, const std::string &line2, tle_t &tle) {
    strncpy(tle.name, name.c_str(), 31);
    tle.name[31] = 0;
    tle.epoch_year = atoi(line1.substr(18, 2).c_str());
    tle.epoch_year += (tle.epoch_year < 57) ? 2000 : 1900;
    tle.epoch_day = atof(line1.substr(20, 12).c_str());
    tle.bstar = atof(line1.substr(53, 8).c_str()) * 1e-5;
    tle.incl = atof(line2.substr(8, 8).c_str()) * SGP4_DEG2RAD;
    tle.raan = atof(line2.substr(17, 8).c_str()) * SGP4_DEG2RAD;
    tle.ecc = atof(("0." + line2.substr(26, 7)).c_str());
    tle.argp = atof(line2.substr(34, 8).c_str()) * SGP4_DEG2RAD;
    tle.mean_anom = atof(line2.substr(43, 8).c_str()) * SGP4_DEG2RAD;
    tle.mean_motion = atof(line2.substr(52, 11).c_str()) * 2 * M_PI / SGP4_XMNPDA;
    return 0;
}

static inline double jd_from_year_doy(int year, double doy) {
    int y = year - 1;
    int a = y / 100;
    int b = 2 - a + a / 4;
    double jd_jan1 = floor(365.25 * (y + 4716)) + floor(30.6001 * 14) + 1 + b - 1524.5;
    return jd_jan1 + (doy - 1.0);
}

static inline void sgp4_get_radec(const tle_t &tle, double jd, double &ra, double &dec, double &dist_km) {
    double jd_epoch = jd_from_year_doy(tle.epoch_year, tle.epoch_day);
    double dt_min = (jd - jd_epoch) * 1440.0;
    double dt_day = (jd - jd_epoch);

    double a = pow(SGP4_XKE / tle.mean_motion, 2.0/3.0);
    double e = tle.ecc;
    double incl = tle.incl;
    double raan = tle.raan;
    double argp = tle.argp;
    double M0 = tle.mean_anom;
    double n = tle.mean_motion;

    // Secular elements (J2)
    double a1 = a;
    double del1 = 1.5 * SGP4_XJ2 * (3 * cos(incl)*cos(incl) - 1) / (a1*a1 * pow(1-e*e, 1.5));
    double a0 = a1 * (1 - del1/3.0);
    double del0 = 1.5 * SGP4_XJ2 * (3 * cos(incl)*cos(incl) - 1) / (a0*a0 * pow(1-e*e, 1.5));
    double n0 = n / (1 + del0);
    double a_avg = a0 / (1 - del0);

    // 长期 RAAN 和 Argp 变化率
    double raan_rate = -1.5 * SGP4_XJ2 * n0 * cos(incl) /
                       (a_avg * a_avg * pow(1 - e*e, 1.5));
    double argp_rate = 0.75 * SGP4_XJ2 * n0 * (5 * cos(incl)*cos(incl) - 1) /
                       (a_avg * a_avg * pow(1 - e*e, 1.5));

    // 【修正】B* 大气阻力：将 bstar 乘以地球半径（标准单位）
    double bstar_earth = tle.bstar * SGP4_XKMPER; // 单位: 1/km? 但后续计算中会与 a 抵消，保持量纲一致
    double a_drag = a_avg;
    double n_drag = n0;
    if (tle.bstar != 0.0 && fabs(tle.bstar) < 1e-3) {
        double drag_factor = bstar_earth * n0 * dt_day;
        a_drag = a_avg * (1 - drag_factor / 3.0);
        n_drag = n0 * (1 + drag_factor);
        e = e * (1 - tle.bstar * n0 * dt_day * 0.5); // 简化
        if (e < 0.0) e = 1e-6;
        if (e > 0.999) e = 0.999;
    }

    double M = M0 + n_drag * dt_min;
    M = fmod(M, 2*M_PI);
    if (M < 0) M += 2*M_PI;

    raan += raan_rate * dt_min;
    argp += argp_rate * dt_min;

    double E = M;
    for (int i = 0; i < 15; i++) {
        double dE = (M - E + e*sin(E)) / (1 - e*cos(E));
        E += dE;
        if (fabs(dE) < 1e-12) break;
    }
    double nu = atan2(sqrt(1-e*e)*sin(E), cos(E)-e);
    double u = nu + argp;
    double r = a_drag * (1 - e*cos(E));

    // 短期摄动（不累积）
    double cos2u = cos(2*u), sin2u = sin(2*u);
    double sin_u = sin(u), cos_u = cos(u);
    double sin_i = sin(incl), cos_i = cos(incl);
    double term = 1.5 * SGP4_XJ2 / (a_drag*a_drag) * n0 / (pow(1-e*e,1.5));
    double delta_r = term * (1 - 3*cos_i*cos_i) * (cos2u + e*cos(nu)) / (1-e*e);
    double delta_u = term * ( (6*cos_i*cos_i - 2) * sin2u + e*(7*cos_i*cos_i - 1)*sin(nu) );
    double delta_Omega = -term * 2 * cos_i * sin_u;
    double delta_i = term * sin_i * cos_u;

    // J3 对偏心率的修正
    double delta_e_j3 = -SGP4_XJ3 / (2 * SGP4_XJ2) * sin_i * (1 - 5*cos_i*cos_i) / (1 - e*e) * sin(nu + 2*argp);
    e += delta_e_j3;
    if (e < 1e-6) e = 1e-6;
    if (e > 0.999) e = 0.999;

    r += delta_r;
    u += delta_u;
    raan += delta_Omega;
    incl += delta_i;

    // TEME inertial coordinates
    double x = r * cos(u);
    double y = r * sin(u);
    double x_eci = x * cos(raan) - y * cos(incl) * sin(raan);
    double y_eci = x * sin(raan) + y * cos(incl) * cos(raan);
    double z_eci = y * sin(incl);
    dist_km = sqrt(x_eci*x_eci + y_eci*y_eci + z_eci*z_eci) * SGP4_XKMPER;
    ra = atan2(y_eci, x_eci);
    if (ra < 0) ra += 2*M_PI;
    dec = atan2(z_eci, sqrt(x_eci*x_eci + y_eci*y_eci));
    apply_nutation_to_radec(jd, ra, dec);
}

static inline std::vector<CelestialObject> load_satellites(const std::string &filename) {
    std::vector<CelestialObject> sats;
    std::ifstream f(filename);
    if (!f) return sats;
    std::string name, l1, l2;
    while (std::getline(f, name)) {
        if (name.empty() || name[0] == '#') continue;
        if (!std::getline(f, l1) || !std::getline(f, l2)) break;
        CelestialObject sat;
        sat.name = name;
        sat.type = SATELLITE;
        sat.dynamic = true;
        sat.tle_line1 = l1;
        sat.tle_line2 = l2;
        sat.distance = 0;
        sat.mag = 4.0;
        sat.ds_type = DS_UNKNOWN;
        sat.bv_color = -1.0;
        sats.push_back(sat);
    }
    return sats;
}

// ============================================================
// 基于 B-V 颜色指数的恒星颜色
// ============================================================
static inline void bv_to_rgb(double bv, int &r, int &g, int &b) {
    if (bv < -0.4) { r = 155; g = 188; b = 255; }
    else if (bv < -0.2) { r = 170; g = 195; b = 255; }
    else if (bv < 0.0)  { r = 200; g = 210; b = 255; }
    else if (bv < 0.2)  { r = 230; g = 230; b = 255; }
    else if (bv < 0.4)  { r = 255; g = 250; b = 240; }
    else if (bv < 0.6)  { r = 255; g = 240; b = 200; }
    else if (bv < 0.8)  { r = 255; g = 220; b = 170; }
    else if (bv < 1.2)  { r = 255; g = 190; b = 140; }
    else if (bv < 1.6)  { r = 255; g = 165; b = 120; }
    else                 { r = 255; g = 140; b = 110; }
}

// Build complete object list
static inline std::vector<CelestialObject> build_catalog(const ObserverConfig &cfg) {
    std::vector<CelestialObject> catalog;
    auto stars = load_stars(cfg.starsFile);
    auto messier = load_messier(cfg.messierFile);
    catalog.insert(catalog.end(), stars.begin(), stars.end());
    catalog.insert(catalog.end(), messier.begin(), messier.end());

    const char* sol_names[] = {
        "Sun", "Moon", "Mercury", "Venus", "Mars",
        "Jupiter", "Saturn", "Uranus", "Neptune", "Pluto"
    };
    const char* sol_names_zh[] = {
        "太阳", "月亮", "水星", "金星", "火星",
        "木星", "土星", "天王星", "海王星", "冥王星"
    };
    double sol_mag[] = {
        -26.74, -12.74, -0.6, -4.4, -2.0,
        -2.7, 0.5, 5.7, 7.8, 14.0
    };
    for (int i = 0; i < 10; ++i) {
        CelestialObject obj;
        obj.name = sol_names[i];
        obj.name_zh = sol_names_zh[i];
        if (i == 0) obj.type = SUN;
        else if (i == 1) obj.type = MOON;
        else obj.type = PLANET;
        obj.dynamic = true;
        obj.id = i;
        obj.mag = sol_mag[i];
        obj.distance = 0;
        obj.ds_type = DS_UNKNOWN;
        obj.bv_color = -1.0;
        catalog.push_back(obj);
    }

    // Planetary satellites
    for (int i = 0; i < NUM_PLANET_MOONS; ++i) {
        CelestialObject obj;
        obj.name = planet_moons[i].name;
        obj.name_zh = planet_moons[i].name_zh;
        obj.type = MOON;
        obj.dynamic = true;
        obj.id = 100 + i;
        obj.mag = planet_moons[i].mag;
        obj.distance = 0;
        obj.ds_type = DS_UNKNOWN;
        obj.bv_color = -1.0;
        catalog.push_back(obj);
    }

    // Asteroids
    for (int i = 0; i < NUM_ASTEROIDS; ++i) {
        CelestialObject obj;
        obj.name = asteroids[i].name;
        obj.name_zh = asteroids[i].name_zh;
        obj.type = ASTEROID;
        obj.dynamic = true;
        obj.id = 200 + i;
        obj.mag = asteroids[i].mag;
        obj.distance = asteroids[i].a;
        obj.ra = 0;
        obj.dec = 0;
        obj.ds_type = DS_UNKNOWN;
        obj.bv_color = -1.0;
        catalog.push_back(obj);
    }

    auto sats = load_satellites(cfg.satellitesFile);
    catalog.insert(catalog.end(), sats.begin(), sats.end());
    return catalog;
}

// Update dynamic objects positions
static inline void update_dynamic(std::vector<CelestialObject> &catalog, double jd) {
    for (auto &obj : catalog) {
        if (!obj.dynamic) continue;

        if (obj.type == SUN || obj.type == PLANET) {
            double dist_au;
            solar_system_position(jd, obj.id, obj.ra, obj.dec, dist_au);
            obj.distance = dist_au;
        } else if (obj.type == MOON) {
            if (obj.id >= 100) {
                int moon_idx = obj.id - 100;
                double dist_au;
                planetary_moon_position(jd, moon_idx, obj.ra, obj.dec, dist_au);
                obj.distance = dist_au;
            } else {
                double dist_au;
                solar_system_position(jd, obj.id, obj.ra, obj.dec, dist_au);
                obj.distance = dist_au;
            }
        } else if (obj.type == SATELLITE) {
            tle_t tle;
            if (parse_tle(obj.name, obj.tle_line1, obj.tle_line2, tle) == 0) {
                double dist_km;
                sgp4_get_radec(tle, jd, obj.ra, obj.dec, dist_km);
                obj.distance = dist_km / 149597870.7;
            }
        } else if (obj.type == ASTEROID) {
            int ast_idx = obj.id - 200;
            if (ast_idx >= 0 && ast_idx < NUM_ASTEROIDS) {
                double x_ast, y_ast, z_ast, x_e, y_e, z_e;
                asteroid_heliocentric(jd, ast_idx, x_ast, y_ast, z_ast);
                planet_heliocentric(jd, 2, x_e, y_e, z_e);
                double dx = x_ast - x_e;
                double dy = y_ast - y_e;
                double dz = z_ast - z_e;
                obj.distance = sqrt(dx*dx + dy*dy + dz*dz);
                // 光行时修正（2次迭代）
                double light_time = obj.distance * 499.005 / 86400.0;
                for (int iter = 0; iter < 2; ++iter) {
                    asteroid_heliocentric(jd - light_time, ast_idx, x_ast, y_ast, z_ast);
                    planet_heliocentric(jd - light_time, 2, x_e, y_e, z_e);
                    dx = x_ast - x_e;
                    dy = y_ast - y_e;
                    dz = z_ast - z_e;
                    obj.distance = sqrt(dx*dx + dy*dy + dz*dz);
                    light_time = obj.distance * 499.005 / 86400.0;
                }
                double ecl_obl = true_obliquity(jd);
                double xeq = dx;
                double yeq = dy * cos(ecl_obl) - dz * sin(ecl_obl);
                double zeq = dy * sin(ecl_obl) + dz * cos(ecl_obl);
                obj.ra = atan2(yeq, xeq);
                if (obj.ra < 0) obj.ra += 2 * M_PI;
                obj.dec = atan2(zeq, sqrt(xeq*xeq + yeq*yeq));
                apply_nutation_to_radec(jd, obj.ra, obj.dec);
            }
        }
    }
}

// Get object type display string
static inline const char* get_type_str(ObjectType type, LanguageId lang) {
    switch (type) {
    case STAR:      return get_str(STR_TYPE_STAR, lang);
    case PLANET:    return get_str(STR_TYPE_PLANET, lang);
    case MOON:      return get_str(STR_TYPE_MOON, lang);
    case SATELLITE: return get_str(STR_TYPE_SATELLITE, lang);
    case DEEPSKY:   return get_str(STR_TYPE_DEEPSKY, lang);
    case SUN:       return get_str(STR_TYPE_SUN, lang);
    case COMET:     return get_str(STR_TYPE_COMET, lang);
    default:        return "?";
    }
}

static inline const char* get_ds_type_str(DeepSkyType dst, LanguageId lang) {
    switch (dst) {
    case DS_GALAXY:             return get_str(STR_TYPE_GALAXY, lang);
    case DS_NEBULA:             return get_str(STR_TYPE_NEBULA, lang);
    case DS_OPEN_CLUSTER:       return get_str(STR_TYPE_OPENCLUSTER, lang);
    case DS_GLOBULAR_CLUSTER:   return get_str(STR_TYPE_GLOBULARCLUSTER, lang);
    case DS_PLANETARY_NEBULA:   return get_str(STR_TYPE_PLANETARYNEBULA, lang);
    case DS_SUPERNOVA_REMNANT:  return get_str(STR_TYPE_SUPERNOVAREMNANT, lang);
    default:                    return get_str(STR_TYPE_DEEPSKY, lang);
    }
}
