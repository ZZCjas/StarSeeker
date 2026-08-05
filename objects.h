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
// Deep sky subtypes
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
    std::string name;       // English name
    std::string name_zh;    // Chinese name
    ObjectType type;
    double ra;    // J2000 or current ra (rad), for static
    double dec;   // rad
    double distance; // light years for stars, AU for solar system, km for satellites
    double mag;      // apparent visual magnitude
    // extra for solar system
    bool dynamic;   // if true, position updated each frame
    int id;         // planet index, etc.
    // For TLE satellite
    std::string tle_line1, tle_line2;
    // Deep sky subtype
    DeepSkyType ds_type;
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
        double ra_h, ra_m, ra_s, dec_d, dec_m, dec_s;
        // Format: Name,NameZh,RAh,RAm,RAs,DecD,DecM,DecS,DistLY,Mag
        std::getline(ss, s.name, ',');
        std::getline(ss, s.name_zh, ',');
        ss >> ra_h; ss.ignore(); ss >> ra_m; ss.ignore(); ss >> ra_s; ss.ignore();
        ss >> dec_d; ss.ignore(); ss >> dec_m; ss.ignore(); ss >> dec_s; ss.ignore();
        ss >> s.distance; ss.ignore(); ss >> s.mag;
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
        std::getline(ss, m.name, ',');        // e.g., M31
        std::getline(ss, m.name_zh, ',');     // Chinese name
        std::string typeStr;
        std::getline(ss, typeStr, ',');
        m.ds_type = parse_ds_type(typeStr);
        double ra_h, ra_m, ra_s, dec_d, dec_m, dec_s;
        ss >> ra_h; ss.ignore(); ss >> ra_m; ss.ignore(); ss >> ra_s; ss.ignore();
        ss >> dec_d; ss.ignore(); ss >> dec_m; ss.ignore(); ss >> dec_s; ss.ignore();
        ss >> m.mag;
        // optional distance (light years)
        if (ss.peek() == ',') {
            ss.ignore();
            ss >> m.distance;
            if (ss.fail()) {
                m.distance = 0;
                ss.clear();
            }
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
// ----- Simplified planet position (J2000 mean elements) -----
struct OrbitElements {
    double a;       // semi-major axis, AU
    double e;       // eccentricity
    double I;       // inclination, deg
    double L;       // mean longitude, deg (at epoch)
    double wbar;    // longitude of perihelion, deg
    double Omega;   // longitude of ascending node, deg
    double epoch;   // JD epoch
    double da, de, dI, dL, dw, dOmega; // rate per century (deg/Cy)
};
static const OrbitElements planet_elements[9] = {
    // Mercury
    {0.387099, 0.205635, 7.0047, 252.2509, 77.4561, 48.3308, 2451545.0, 0, 0.000019, -0.0059, 149472.674, 0.1600, -0.1253},
    // Venus
    {0.723332, 0.006773, 3.3946, 181.9798, 131.6024, 76.6799, 2451545.0, 0, -0.000041, -0.0011, 58517.815, 0.0027, -0.2772},
    // Earth (barycenter, but fine)
    {1.000001, 0.016710, 0.00005, 100.4644, 102.9472, 0.0, 2451545.0, 0, -0.000038, -0.0131, 35999.372, 0.3232, 0},
    // Mars
    {1.523680, 0.093412, 1.8497,  -4.5685, 336.0602, 49.6674, 2451545.0, 0, 0.000119, -0.0081, 19140.299, 0.4443, -0.2926},
    // Jupiter
    {5.20259, 0.048498, 1.3033, 34.3965, 14.3072, 100.4540, 2451545.0, 0, -0.000031, -0.0128, 3034.903, 0.2125, -0.1603},
    // Saturn
    {9.55491, 0.055508, 2.4886, 49.9542, 92.2641, 113.6624, 2451545.0, 0, -0.000041, 0.0040, 1222.116, -0.1566, -0.2567},
    // Uranus
    {19.21845, 0.046296, 0.7734, 313.2381, 170.9542, 74.0161, 2451545.0, 0, -0.000027, 0.0020, 428.467, 0.0204, -0.0959},
    // Neptune
    {30.11038, 0.008598, 1.7700, -55.1203, 37.4051, 131.7841, 2451545.0, 0, 0.000016, 0.0008, 218.459, -0.0093, -0.0792},
    // Pluto (dwarf)
    {39.445, 0.250, 17.14, 244.0, 224.0, 110.0, 2451545.0, 0,0,0, 146.0, -0.45, -0.40}
};
// ----- Planetary satellites (simplified orbits) -----
struct MoonOrbitElements {
    double a;         // semi-major axis, km
    double e;         // eccentricity
    double I;         // inclination to planet equator, deg
    double Omega;     // longitude of ascending node, deg
    double w;         // argument of periapsis, deg
    double M0;        // mean anomaly at epoch J2000, deg
    double period;    // orbital period, days
    double mag;       // approximate visual magnitude
    const char* name;
    const char* name_zh;
    int parent_planet; // 2=Mercury... 5=Jupiter, 6=Saturn, etc.
};

// Major planetary satellites (simplified J2000 elements)
static const MoonOrbitElements planet_moons[] = {
    // Mars
    {9378,    0.0151, 1.093,  0.0,   0.0,   0.0, 0.31891, 11.3, "Phobos",       "火卫一", 4},
    {23459,   0.0005, 0.930,  0.0,   0.0,   0.0, 1.26244, 12.4, "Deimos",       "火卫二", 4},

    // Jupiter - Galilean moons
    {421800,  0.0041, 0.040,  0.0,   0.0,   0.0, 1.76914, 5.0,  "Io",           "木卫一", 5},
    {671100,  0.0094, 0.470,  0.0,   0.0,   0.0, 3.55118, 5.3,  "Europa",       "木卫二", 5},
    {1070400, 0.0011, 0.180,  0.0,   0.0,   0.0, 7.15455, 4.6,  "Ganymede",     "木卫三", 5},
    {1882700, 0.0074, 0.190,  0.0,   0.0,   0.0, 16.689,  5.6,  "Callisto",     "木卫四", 5},

    // Saturn major moons
    {185539,  0.0202, 1.530,  0.0,   0.0,   0.0, 0.94242, 12.9, "Mimas",        "土卫一", 6},
    {237948,  0.0045, 0.020,  0.0,   0.0,   0.0, 1.37022, 11.7, "Enceladus",    "土卫二", 6},
    {294619,  0.0000, 1.860,  0.0,   0.0,   0.0, 1.88780, 10.2, "Tethys",       "土卫三", 6},
    {377396,  0.0022, 0.020,  0.0,   0.0,   0.0, 2.73692, 10.4, "Dione",        "土卫四", 6},
    {527108,  0.0013, 0.350,  0.0,   0.0,   0.0, 4.51750, 9.7,  "Rhea",         "土卫五", 6},
    {1221870, 0.0288, 0.330,  0.0,   0.0,   0.0, 15.945,  8.4,  "Titan",        "土卫六", 6},
    {3560820, 0.0286, 15.55,  0.0,   0.0,   0.0, 79.330,  11.0, "Iapetus",      "土卫八", 6},
};

static const int NUM_PLANET_MOONS = sizeof(planet_moons) / sizeof(planet_moons[0]);
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
    // solve Kepler
    double E = M;
    for (int i = 0; i < 10; ++i) {
        double dE = (M - E + e * sin(E)) / (1 - e * cos(E));
        E += dE;
        if (fabs(dE) < 1e-9) break;
    }
    double cosE = cos(E), sinE = sin(E);
    double nu = atan2(sqrt(1 - e * e) * sinE, cosE - e);
    double r = a * (1 - e * cosE);
    double x_orb = r * cos(nu + w);
    double y_orb = r * sin(nu + w);
    double z_orb = 0;
    // rotate to ecliptic
    x = x_orb * cos(Omega) - y_orb * cos(I) * sin(Omega);
    y = x_orb * sin(Omega) + y_orb * cos(I) * cos(Omega);
    z = y_orb * sin(I);
}
// Get geocentric equatorial coordinates for solar system body
// body_id: 0=sun, 1=moon, 2=mercury, 3=venus, 4=mars, 5=jupiter,
//          6=saturn, 7=uranus, 8=neptune, 9=pluto
static inline void solar_system_position(double jd, int body_id, double &ra, double &dec, double &dist_au) {
    if (body_id == 0) { // Sun
        double x_e, y_e, z_e;
        planet_heliocentric(jd, 2, x_e, y_e, z_e); // Earth
        double x_s = -x_e, y_s = -y_e, z_s = -z_e;
        dist_au = sqrt(x_s * x_s + y_s * y_s + z_s * z_s);
        double ecl_obl = (23.439291 - 0.0130042 * (jd - 2451545.0) / 36525.0) * M_PI / 180.0;
        double xecl = x_s, yecl = y_s * cos(ecl_obl) - z_s * sin(ecl_obl), zecl = y_s * sin(ecl_obl) + z_s * cos(ecl_obl);
        ra = atan2(yecl, xecl);
        if (ra < 0) ra += 2 * M_PI;
        dec = atan2(zecl, sqrt(xecl * xecl + yecl * yecl));
        return;
    }
    if (body_id == 1) { // Moon simplified
        double T = (jd - 2451545.0) / 36525.0;
        double L = fmod(218.316 + 481267.881 * T, 360.0) * M_PI / 180.0;
        double M = fmod(134.963 + 477198.867 * T, 360.0) * M_PI / 180.0;
        double F = fmod(93.272 + 483202.018 * T, 360.0) * M_PI / 180.0;
        double lambda = L + 6.289 * sin(M) * M_PI / 180.0;
        double beta = 5.128 * sin(F) * M_PI / 180.0;
        double dist = 385000.0;
        dist_au = dist / 149597870.7;
        double ecl_obl = (23.439291 - 0.0130042 * T) * M_PI / 180.0;
        double xe = cos(lambda) * cos(beta);
        double ye = sin(lambda) * cos(beta) * cos(ecl_obl) - sin(beta) * sin(ecl_obl);
        double ze = sin(lambda) * cos(beta) * sin(ecl_obl) + sin(beta) * cos(ecl_obl);
        ra = atan2(ye, xe);
        if (ra < 0) ra += 2 * M_PI;
        dec = atan2(ze, sqrt(xe * xe + ye * ye));
        return;
    }
    // Planets (2=Mercury to 9=Pluto)
    int planet_idx = body_id - 2;
    if (planet_idx < 0 || planet_idx > 7) return;
    double x_pl, y_pl, z_pl, x_e, y_e, z_e;
    planet_heliocentric(jd, planet_idx, x_pl, y_pl, z_pl);
    planet_heliocentric(jd, 2, x_e, y_e, z_e); // Earth
    double dx = x_pl - x_e;
    double dy = y_pl - y_e;
    double dz = z_pl - z_e;
    dist_au = sqrt(dx * dx + dy * dy + dz * dz);
    double ecl_obl = (23.439291 - 0.0130042 * (jd - 2451545.0) / 36525.0) * M_PI / 180.0;
    double xeq = dx;
    double yeq = dy * cos(ecl_obl) - dz * sin(ecl_obl);
    double zeq = dy * sin(ecl_obl) + dz * cos(ecl_obl);
    ra = atan2(yeq, xeq);
    if (ra < 0) ra += 2 * M_PI;
    dec = atan2(zeq, sqrt(xeq * xeq + yeq * yeq));
}
// Compute position of a planetary moon relative to planet (equatorial, AU)
static inline void moon_relative(double jd, int moon_idx, double &dx, double &dy, double &dz) {
    const MoonOrbitElements &m = planet_moons[moon_idx];
    
    double T = (jd - 2451545.0) / 36525.0;
    double M = fmod(m.M0 + (jd - 2451545.0) / m.period * 360.0, 360.0) * M_PI / 180.0;
    double e = m.e;
    double I = m.I * M_PI / 180.0;
    double Omega = m.Omega * M_PI / 180.0;
    double w = m.w * M_PI / 180.0;
    
    // Solve Kepler
    double E = M;
    for (int i = 0; i < 8; ++i) {
        double dE = (M - E + e * sin(E)) / (1 - e * cos(E));
        E += dE;
        if (fabs(dE) < 1e-9) break;
    }
    
    double nu = atan2(sqrt(1 - e*e) * sin(E), cos(E) - e);
    double r = m.a * (1 - e * cos(E)); // km
    
    // Position in orbital plane
    double x_orb = r * cos(nu + w);
    double y_orb = r * sin(nu + w);
    double z_orb = 0;
    
    // Rotate by inclination and ascending node
    // Simplified: assume orbit roughly in planet equatorial plane
    double x1 = x_orb * cos(Omega) - y_orb * cos(I) * sin(Omega);
    double y1 = x_orb * sin(Omega) + y_orb * cos(I) * cos(Omega);
    double z1 = y_orb * sin(I);
    
    // Convert km to AU
    const double KM_PER_AU = 149597870.7;
    dx = x1 / KM_PER_AU;
    dy = y1 / KM_PER_AU;
    dz = z1 / KM_PER_AU;
}

// Get geocentric equatorial position for planetary moon
static inline void planetary_moon_position(double jd, int moon_idx,
                                           double &ra, double &dec, double &dist_au) {
    const MoonOrbitElements &m = planet_moons[moon_idx];
    
    // Get parent planet heliocentric position
    double px, py, pz;
    planet_heliocentric(jd, m.parent_planet - 2, px, py, pz);
    
    // Get Earth heliocentric position
    double ex, ey, ez;
    planet_heliocentric(jd, 2, ex, ey, ez);
    
    // Get moon relative to planet (ecliptic frame approximation)
    double mx, my, mz;
    moon_relative(jd, moon_idx, mx, my, mz);
    
    // Moon heliocentric = planet + relative
    double mhx = px + mx;
    double mhy = py + my;
    double mhz = pz + mz;
    
    // Geocentric
    double dx = mhx - ex;
    double dy = mhy - ey;
    double dz = mhz - ez;
    
    dist_au = sqrt(dx*dx + dy*dy + dz*dz);
    
    // Rotate ecliptic to equatorial
    double ecl_obl = (23.439291 - 0.0130042 * (jd - 2451545.0) / 36525.0) * M_PI / 180.0;
    double xeq = dx;
    double yeq = dy * cos(ecl_obl) - dz * sin(ecl_obl);
    double zeq = dy * sin(ecl_obl) + dz * cos(ecl_obl);
    
    ra = atan2(yeq, xeq);
    if (ra < 0) ra += 2 * M_PI;
    dec = atan2(zeq, sqrt(xeq*xeq + yeq*yeq));
}
// ----- Simple SGP4 for TLE satellites -----
#define SGP4_DEG2RAD (M_PI/180.0)
#define SGP4_XJ2 1.082616e-3
#define SGP4_XKE 0.0743669161331734132
#define SGP4_XKMPER 6378.135
#define SGP4_XMNPDA 1440.0

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

// Compute JD from year and day-of-year, accounting for leap years
static inline double jd_from_year_doy(int year, double doy) {
    // January 1.0 of the given year
    int y = year - 1;
    int a = y / 100;
    int b = 2 - a + a / 4;
    double jd_jan1 = floor(365.25 * (y + 4716)) + floor(30.6001 * 14) + 1 + b - 1524.5;
    return jd_jan1 + (doy - 1.0);
}

static inline void sgp4_get_radec(const tle_t &tle, double jd, double &ra, double &dec, double &dist_km) {
    double jd_epoch = jd_from_year_doy(tle.epoch_year, tle.epoch_day);
    double dt_min = (jd - jd_epoch) * 1440.0;
    double a = pow(SGP4_XKE / tle.mean_motion, 2.0/3.0);
    double e = tle.ecc;
    double incl = tle.incl;
    double raan = tle.raan;
    double argp = tle.argp;
    double M0 = tle.mean_anom;
    double n = tle.mean_motion;
    // Propagate secular
    double a1 = a;
    double del1 = 1.5 * SGP4_XJ2 * (3 * cos(incl)*cos(incl) - 1) / (a1*a1 * pow(1-e*e, 1.5));
    double a0 = a1 * (1 - del1/3.0);
    double del0 = 1.5 * SGP4_XJ2 * (3 * cos(incl)*cos(incl) - 1) / (a0*a0 * pow(1-e*e, 1.5));
    double n0 = n / (1 + del0);
    double a_avg = a0 / (1 - del0);
    double M = M0 + n0 * dt_min;
    M = fmod(M, 2*M_PI);
    if (M < 0) M += 2*M_PI;
    // Kepler
    double E = M;
    for (int i = 0; i < 10; i++) {
        double dE = (M - E + e*sin(E)) / (1 - e*cos(E));
        E += dE;
        if (fabs(dE) < 1e-10) break;
    }
    double nu = atan2(sqrt(1-e*e)*sin(E), cos(E)-e);
    double u = nu + argp;
    double r = a_avg * (1 - e*cos(E));
    // Perturbations
    double cos2u = cos(2*u), sin2u = sin(2*u);
    double sin_u = sin(u), cos_u = cos(u);
    double sin_i = sin(incl), cos_i = cos(incl);
    double term = 1.5 * SGP4_XJ2 / (a_avg*a_avg) * n0 / (pow(1-e*e,1.5));
    double delta_r = term * (1 - 3*cos_i*cos_i) * (cos2u + e*cos(nu)) / (1-e*e);
    double delta_u = term * ( (6*cos_i*cos_i - 2) * sin2u + e*(7*cos_i*cos_i - 1)*sin(nu) );
    double delta_Omega = -term * 2 * cos_i * sin_u;
    double delta_i = term * sin_i * cos_u;
    r += delta_r;
    u += delta_u;
    raan += delta_Omega * dt_min;
    incl += delta_i * dt_min;
    // TEME inertial coordinates (x points to vernal equinox)
    double x = r * cos(u);
    double y = r * sin(u);
    double x_eci = x * cos(raan) - y * cos(incl) * sin(raan);
    double y_eci = x * sin(raan) + y * cos(incl) * cos(raan);
    double z_eci = y * sin(incl);
    dist_km = sqrt(x_eci*x_eci + y_eci*y_eci + z_eci*z_eci) * SGP4_XKMPER;
    ra = atan2(y_eci, x_eci);
    if (ra < 0) ra += 2*M_PI;
    dec = atan2(z_eci, sqrt(x_eci*x_eci + y_eci*y_eci));
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
        sats.push_back(sat);
    }
    return sats;
}

// Build complete object list
static inline std::vector<CelestialObject> build_catalog(const ObserverConfig &cfg) {
    std::vector<CelestialObject> catalog;

    auto stars = load_stars(cfg.starsFile);
    auto messier = load_messier(cfg.messierFile);
    catalog.insert(catalog.end(), stars.begin(), stars.end());
    catalog.insert(catalog.end(), messier.begin(), messier.end());

    // Solar system dynamic objects
    // 0=Sun, 1=Moon, 2=Mercury, 3=Venus, 4=Mars, 5=Jupiter,
    // 6=Saturn, 7=Uranus, 8=Neptune, 9=Pluto
    const char* sol_names[] = {
        "Sun", "Moon", "Mercury", "Venus", "Mars",
        "Jupiter", "Saturn", "Uranus", "Neptune", "Pluto"
    };
    const char* sol_names_zh[] = {
        "太阳", "月球", "水星", "金星", "火星",
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
        else if (i == 9) obj.type = PLANET;  // Pluto as planet (dwarf)
        else obj.type = PLANET;
        obj.dynamic = true;
        obj.id = i;
        obj.mag = sol_mag[i];
        obj.distance = 0;
        obj.ds_type = DS_UNKNOWN;
        catalog.push_back(obj);
    }
	    // Planetary satellites
    for (int i = 0; i < NUM_PLANET_MOONS; ++i) {
        CelestialObject obj;
        obj.name = planet_moons[i].name;
        obj.name_zh = planet_moons[i].name_zh;
        obj.type = MOON;  // reuse MOON type, distinguish by name
        obj.dynamic = true;
        obj.id = i;       // moon index in planet_moons array
        obj.mag = planet_moons[i].mag;
        obj.distance = 0;
        obj.ds_type = DS_UNKNOWN;
        // Mark as planetary moon via a special flag: use id offset
        // We'll use a negative id offset to distinguish from Earth's Moon
        // Actually, let's add a new type: modify ObjectType
        // For minimal change, we use id + 100 to indicate planetary moon
        obj.id = 100 + i;
        catalog.push_back(obj);
    }
    // Major asteroids (static approximate positions for demo)
    const char* ast_names[] = {"Ceres", "Vesta", "Pallas"};
    const char* ast_names_zh[] = {"谷神星", "灶神星", "智神星"};
    double ast_ra_h[] = {4.0, 2.0, 0.5};   // approximate
    double ast_dec_d[] = {10.0, 8.0, -5.0};
    double ast_mag[] = {7.0, 6.5, 8.0};
    double ast_dist[] = {2.8, 2.4, 2.7};
    for (int i = 0; i < 3; ++i) {
        CelestialObject obj;
        obj.name = ast_names[i];
        obj.name_zh = ast_names_zh[i];
        obj.type = ASTEROID;
        obj.dynamic = false;
        obj.id = i;
        obj.mag = ast_mag[i];
        obj.distance = ast_dist[i];
        obj.ra = ast_ra_h[i] * 15.0 * M_PI / 180.0;
        obj.dec = ast_dec_d[i] * M_PI / 180.0;
        obj.ds_type = DS_UNKNOWN;
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
                // Planetary moon
                int moon_idx = obj.id - 100;
                double dist_au;
                planetary_moon_position(jd, moon_idx, obj.ra, obj.dec, dist_au);
                obj.distance = dist_au;
            } else {
                // Earth's Moon
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
// Get deep sky object subtype display string
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
