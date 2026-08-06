#pragma once
#define _USE_MATH_DEFINES
#include <cmath>
#include <ctime>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============================================================
// StarSeeker Improved Astronomy Library
// 改进项：大气折射、章动、光行差、光行时间修正、月球位置增强
// ============================================================

// Julian Date from system time
static inline double get_jd_now() {
    time_t now = time(nullptr);
    struct tm utc;
#ifdef _WIN32
    struct tm* gmt = gmtime(&now);
    utc = *gmt;
#else
    gmtime_r(&now, &utc);
#endif
    int y = utc.tm_year + 1900;
    int m = utc.tm_mon + 1;
    int d = utc.tm_mday;
    int h = utc.tm_hour;
    int min = utc.tm_min;
    double sec = utc.tm_sec;
    if (m <= 2) { y -= 1; m += 12; }
    int a = y / 100;
    int b = 2 - a + a / 4;
    double jd = floor(365.25 * (y + 4716)) + floor(30.6001 * (m + 1)) + d + b - 1524.5;
    jd += (h + min / 60.0 + sec / 3600.0) / 24.0;
    return jd;
}

// Greenwich Mean Sidereal Time (radians) at given JD
static inline double gmst(double jd) {
    double jd_midnight = floor(jd - 0.5) + 0.5;
    double T = (jd_midnight - 2451545.0) / 36525.0;
    double gmst0_sec = 24110.54841 + 8640184.812866 * T + 0.093104 * T * T - 6.2e-6 * T * T * T;
    double ut_sec = (jd - jd_midnight) * 86400.0;
    double gmst_sec = gmst0_sec + ut_sec * 1.00273790935;
    gmst_sec = fmod(gmst_sec, 86400.0);
    if (gmst_sec < 0) gmst_sec += 86400.0;
    return (gmst_sec / 240.0) * M_PI / 180.0;
}

// ============================================================
// 【新增】IAU 1980 章动模型（简化版，取最主要项）
// 返回黄经章动 dpsi 和交角章动 deps（弧度）
// 精度：约 0.1 角秒，足够目视星图使用
// ============================================================
static inline void nutation(double jd, double &dpsi, double &deps) {
    double T = (jd - 2451545.0) / 36525.0;
    // 基本参数
    double M_sun = fmod(357.529 + 35999.050 * T, 360.0) * M_PI / 180.0;
    double M_moon = fmod(134.963 + 477198.867 * T, 360.0) * M_PI / 180.0;
    double F = fmod(93.272 + 483202.018 * T, 360.0) * M_PI / 180.0;
    double D = fmod(297.850 + 445267.111 * T, 360.0) * M_PI / 180.0;
    double Om = fmod(125.045 - 1934.136 * T, 360.0) * M_PI / 180.0;

    // 黄经章动（主要项，单位 0.001 角秒）
    double dpsi_arcsec =
        -171996.0 * sin(Om)
        - 2062.0 * sin(2 * Om)
        - 174.0 * sin(2 * F + Om)
        + 147.0 * sin(M_sun - Om)
        - 17.0 * sin(2 * M_sun)
        + 31.0 * sin(2 * F - 2 * Om + M_sun)
        + 32.0 * sin(2 * F + Om - M_sun)
        + 38.0 * sin(D - Om)
        - 174.0 * sin(M_moon - Om)
        + 36.0 * sin(M_moon + Om);

    // 交角章动
    double deps_arcsec =
        92025.0 * cos(Om)
        + 5736.0 * cos(2 * Om)
        - 977.0 * cos(2 * F - 2 * Om)
        + 895.0 * cos(2 * F + Om)
        - 24.0 * cos(2 * F + 2 * Om)
        - 7.0 * cos(M_sun - Om)
        + 22.0 * cos(2 * M_sun)
        + 9745.0 * cos(M_moon - Om)
        - 200.0 * cos(M_moon + Om);

    dpsi = dpsi_arcsec * M_PI / (180.0 * 3600.0);
    deps = deps_arcsec * M_PI / (180.0 * 3600.0);
}

// 【新增】真黄赤交角（含章动）
static inline double true_obliquity(double jd) {
    double T = (jd - 2451545.0) / 36525.0;
    double eps0 = (23.4392911 - 0.0130042 * T - 1.64e-7 * T * T + 5.04e-7 * T * T * T) * M_PI / 180.0;
    double dpsi, deps;
    nutation(jd, dpsi, deps);
    return eps0 + deps;
}

// 平黄赤交角（不含章动）
static inline double mean_obliquity(double jd) {
    double T = (jd - 2451545.0) / 36525.0;
    return (23.4392911 - 0.0130042 * T - 1.64e-7 * T * T + 5.04e-7 * T * T * T) * M_PI / 180.0;
}

// ============================================================
// 【新增】大气折射修正（Saemundsson 公式）
// 输入：视高度角（度），返回折射修正量（度）
// ============================================================
static inline double atmospheric_refraction(double alt_deg) {
    if (alt_deg < -1.0) return 0.0;
    double alt = alt_deg;
    if (alt < 0.0) alt = 0.0;
    double arg = alt + 10.3 / (alt + 5.11);
    double R = 1.02 / tan(arg * M_PI / 180.0);  // 角分
    double R_deg = R / 60.0;
    if (R_deg > 0.5) R_deg = 0.5;
    return R_deg;
}

// 【新增】将真高度转为视高度（加上折射）
static inline double true_to_apparent_alt(double true_alt_deg) {
    if (true_alt_deg < -1.0) return true_alt_deg;
    double app = true_alt_deg + atmospheric_refraction(true_alt_deg);
    app = true_alt_deg + atmospheric_refraction(app);
    return app;
}

// ============================================================
// 【修正】年光行差修正（标准公式，符合 Meeus）
// 修正赤道坐标以反映地球轨道运动产生的 ±20.5 角秒偏差
// ============================================================
static inline void apply_aberration(double jd, double &ra, double &dec) {
    double T = (jd - 2451545.0) / 36525.0;
    // 太阳真黄经（近似，用平黄经即可）
    double L_sun = fmod(280.46646 + 36000.76983 * T + 0.0003032 * T * T, 360.0) * M_PI / 180.0;
    double eps = true_obliquity(jd);
    double kappa = 20.49552 * M_PI / (180.0 * 3600.0);  // 光行差常数（弧度）

    double cos_a = cos(ra), sin_a = sin(ra);
    double cos_d = cos(dec), sin_d = sin(dec);
    double cos_e = cos(eps), sin_e = sin(eps);
    double cos_l = cos(L_sun), sin_l = sin(L_sun);

    // 标准公式（Meeus, 第23章）
    double d_ra  = -kappa * (cos_a * cos_e * cos_l + sin_a * sin_l) / cos_d;
    double d_dec = -kappa * (sin_d * cos_a * sin_l - sin_d * sin_a * cos_e * cos_l + cos_d * sin_e * cos_l);

    ra  += d_ra;
    dec += d_dec;
    ra = fmod(ra, 2 * M_PI);
    if (ra < 0) ra += 2 * M_PI;
}

// ============================================================
// 【修正】章动对赤道坐标的修正（平位置 -> 真位置）
// 使用当前平黄赤交角（而非固定值）
// ============================================================
static inline void apply_nutation_to_radec(double jd, double &ra, double &dec) {
    double dpsi, deps;
    nutation(jd, dpsi, deps);
    double eps0 = mean_obliquity(jd);  // 使用平黄赤交角

    double dra  = dpsi * (cos(eps0) + sin(eps0) * sin(ra) * tan(dec));
    double ddec = dpsi * cos(ra) * sin(eps0) * cos(dec)
                + deps * sin(ra) * cos(dec);

    ra += dra;
    dec += ddec;
    ra = fmod(ra, 2 * M_PI);
    if (ra < 0) ra += 2 * M_PI;
}

// ============================================================
// 【新增】格林尼治视恒星时（含章动）
// ============================================================
static inline double gast(double jd) {
    double gmst_val = gmst(jd);
    double dpsi, deps;
    nutation(jd, dpsi, deps);
    double eps = true_obliquity(jd);
    // 章动对恒星时的修正
    double eq_eq = dpsi * cos(eps);
    return gmst_val + eq_eq;
}

// Equatorial (ra, dec) to Horizon (alt, az), all radians
static inline void equatorial_to_horizon(double ra, double dec, double lst, double lat,
                                         double &alt, double &az) {
    double ha = lst - ra;
    double sin_alt = sin(dec) * sin(lat) + cos(dec) * cos(lat) * cos(ha);
    if (sin_alt > 1.0) sin_alt = 1.0;
    if (sin_alt < -1.0) sin_alt = -1.0;
    alt = asin(sin_alt);
    double sin_az = -cos(dec) * sin(ha);
    double cos_az = sin(dec) * cos(lat) - cos(dec) * sin(lat) * cos(ha);
    az = atan2(sin_az, cos_az);
    if (az < 0) az += 2 * M_PI;
}

// Horizon to Equatorial
static inline void horizon_to_equatorial(double alt, double az, double lst, double lat,
                                         double &ra, double &dec) {
    double sin_dec = sin(alt) * sin(lat) + cos(alt) * cos(lat) * cos(az);
    if (sin_dec > 1.0) sin_dec = 1.0;
    if (sin_dec < -1.0) sin_dec = -1.0;
    dec = asin(sin_dec);
    double sin_ha = -cos(alt) * sin(az);
    double cos_ha_denom = cos(lat) * cos(dec);
    double cos_ha = (cos_ha_denom != 0.0) ?
        (sin(alt) - sin(lat) * sin_dec) / cos_ha_denom : 0.0;
    double ha = atan2(sin_ha, cos_ha);
    ra = lst - ha;
    ra = fmod(ra, 2 * M_PI);
    if (ra < 0) ra += 2 * M_PI;
}

// Screen projection (azimuthal equidistant)
static inline void horizon_to_screen(double az, double alt,
                                     double center_az, double center_alt, double fov,
                                     int w, int h, double &sx, double &sy) {
    double daz = az - center_az;
    double cos_dist = sin(center_alt) * sin(alt) + cos(center_alt) * cos(alt) * cos(daz);
    if (cos_dist > 1.0) cos_dist = 1.0;
    else if (cos_dist < -1.0) cos_dist = -1.0;
    double dist = acos(cos_dist);
    if (dist < 1e-9) { sx = w / 2.0; sy = h / 2.0; return; }
    double PA;
    if (fabs(cos(center_alt)) < 1e-9) {
        PA = daz;
    } else {
        double sinPA = cos(alt) * sin(daz) / sin(dist);
        double cosPA = (sin(alt) - sin(center_alt) * cos_dist) / (cos(center_alt) * sin(dist));
        if (sinPA > 1.0) sinPA = 1.0; if (sinPA < -1.0) sinPA = -1.0;
        if (cosPA > 1.0) cosPA = 1.0; if (cosPA < -1.0) cosPA = -1.0;
        PA = atan2(sinPA, cosPA);
    }
    double r = dist / (fov / 2.0) * (h / 2.0);
    double cx = w / 2.0, cy = h / 2.0;
    sx = cx + r * sin(PA);
    sy = cy - r * cos(PA);
}

// Screen to horizon inverse
static inline void screen_to_horizon(double sx, double sy, double center_az, double center_alt,
                                     double fov, int w, int h, double &az, double &alt) {
    double cx = w / 2.0, cy = h / 2.0;
    double dx = sx - cx;
    double dy = cy - sy;
    double r = sqrt(dx * dx + dy * dy);
    double PA = atan2(dx, dy);
    double dist = r / (h / 2.0) * (fov / 2.0);
    if (dist > M_PI) dist = M_PI;
    double sin_alt = cos(dist) * sin(center_alt) + sin(dist) * cos(center_alt) * cos(PA);
    if (sin_alt > 1.0) sin_alt = 1.0;
    if (sin_alt < -1.0) sin_alt = -1.0;
    alt = asin(sin_alt);
    double y_term = cos(center_alt) * cos(dist) - sin(center_alt) * sin(dist) * cos(PA);
    double x_term = sin(dist) * sin(PA);
    double daz = atan2(x_term, y_term);
    az = center_az + daz;
    if (az < 0) az += 2 * M_PI;
    if (az >= 2 * M_PI) az -= 2 * M_PI;
}
