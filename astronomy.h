#pragma once
#define _USE_MATH_DEFINES
#include <cmath>
#include <ctime>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
// Julian Date from system time
static inline double get_jd_now() {
    time_t now = time(nullptr);
    // UTC broken down
    struct tm utc;
#ifdef _WIN32
    struct tm* gmt = gmtime(&now);   // C标准函数
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
    // 计算当日UT子夜对应的儒略日
    double jd_midnight = floor(jd - 0.5) + 0.5;
    double T = (jd_midnight - 2451545.0) / 36525.0;
    // 子夜时刻的格林尼治平恒星时（秒）
    double gmst0_sec = 24110.54841 + 8640184.812866 * T + 0.093104 * T * T - 6.2e-6 * T * T * T;
    // 当日UT时间换算为恒星时增量（太阳时转恒星时系数）
    double ut_sec = (jd - jd_midnight) * 86400.0;
    double gmst_sec = gmst0_sec + ut_sec * 1.00273790935;

    gmst_sec = fmod(gmst_sec, 86400.0);
    if (gmst_sec < 0) gmst_sec += 86400.0;
    return (gmst_sec / 240.0) * M_PI / 180.0; // to rad
}
// Equatorial (ra, dec) to Horizon (alt, az), all radians
// lst: local sidereal time (rad), lat: observer latitude (rad)
static inline void equatorial_to_horizon(double ra, double dec, double lst, double lat,
                                         double &alt, double &az) {
    double ha = lst - ra;
    double sin_alt = sin(dec) * sin(lat) + cos(dec) * cos(lat) * cos(ha);
    alt = asin(sin_alt);
    double sin_az = -cos(dec) * sin(ha);
    double cos_az = sin(dec) * cos(lat) - cos(dec) * sin(lat) * cos(ha);
    az = atan2(sin_az, cos_az); // north=0, east=pi/2
    if (az < 0) az += 2 * M_PI;
}
// Horizon to Equatorial
static inline void horizon_to_equatorial(double alt, double az, double lst, double lat,
                                         double &ra, double &dec) {
    double sin_dec = sin(alt) * sin(lat) + cos(alt) * cos(lat) * cos(az);
    dec = asin(sin_dec);
    double sin_ha = -cos(alt) * sin(az);
    double cos_ha = (sin(alt) - sin(lat) * sin_dec) / (cos(lat) * cos(dec));
    double ha = atan2(sin_ha, cos_ha);
    ra = lst - ha;
    ra = fmod(ra, 2 * M_PI);
    if (ra < 0) ra += 2 * M_PI;
}
// Screen projection (azimuthal equidistant)
// center_az, center_alt in rad, fov full angle in rad, screen width/height
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
    // 处理极区（中心高度角接近 ±90°）的退化情况
    if (fabs(cos(center_alt)) < 1e-9) {
        PA = daz;   // 直接使用方位角差作为位置角
    } else {
        double sinPA = cos(alt) * sin(daz) / sin(dist);
        double cosPA = (sin(alt) - sin(center_alt) * cos_dist) / (cos(center_alt) * sin(dist));
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
    double dy = cy - sy; // up positive
    double r = sqrt(dx * dx + dy * dy);
    double PA = atan2(dx, dy); // from north clockwise
    double dist = r / (h / 2.0) * (fov / 2.0);
    double sin_alt = cos(dist) * sin(center_alt) + sin(dist) * cos(center_alt) * cos(PA);
    alt = asin(sin_alt);
    double y_term = cos(center_alt) * cos(dist) - sin(center_alt) * sin(dist) * cos(PA);
    double x_term = sin(dist) * sin(PA);
    double daz = atan2(x_term, y_term);
    az = center_az + daz;
    if (az < 0) az += 2 * M_PI;
    if (az >= 2 * M_PI) az -= 2 * M_PI;
}
