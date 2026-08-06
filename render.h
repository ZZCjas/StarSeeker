#pragma once
#include <graphics.h>
#include <string>
#include <algorithm>
#include <unordered_map>
#include <cstdio>
#include <ctime>
#include "objects.h"
#include "astronomy.h"
#include "config.h"
#include "language.h"

struct Camera {
    double center_az;
    double center_alt;
    double fov;
};

// Compute star pixel size from magnitude
static inline double star_size(double mag, double fov_deg) {
    double base = 1.0;
    if (mag < 0.0) base = 3.8;
    else if (mag < 1.0) base = 3.4;
    else if (mag < 2.0) base = 3.0;
    else if (mag < 3.0) base = 2.6;
    else if (mag < 4.0) base = 2.2;
    else if (mag < 5.0) base = 1.8;
    else if (mag < 6.0) base = 1.4;
    else base = 1.0;
    double zoom_factor = 120.0 / fov_deg;
    if (zoom_factor < 0.5) zoom_factor = 0.5;
    if (zoom_factor > 4.0) zoom_factor = 4.0;
    return base * (0.7 + 0.3 * zoom_factor);
}

// ============================================================
// 【改进】恒星颜色：优先使用 B-V 颜色指数，回退到星等估计
// ============================================================
static inline COLORREF star_color(double mag, double bv) {
    int r, g, b;
    if (bv >= -1.0) {
        // 有 B-V 数据，使用基于光谱型的颜色
        bv_to_rgb(bv, r, g, b);
    } else {
        // 无 B-V 数据，用星等做粗略估计（降级方案）
        if (mag < 0.0)       { r = 200; g = 215; b = 255; }
        else if (mag < 1.0)  { r = 220; g = 225; b = 255; }
        else if (mag < 2.0)  { r = 240; g = 240; b = 250; }
        else if (mag < 3.0)  { r = 255; g = 245; b = 220; }
        else if (mag < 4.0)  { r = 255; g = 240; b = 190; }
        else if (mag < 5.0)  { r = 240; g = 230; b = 160; }
        else                  { r = 210; g = 200; b = 170; }
    }
    return RGB(r, g, b);
}

// Get deep sky object color by subtype
static inline COLORREF deepsky_color(DeepSkyType dst) {
    switch (dst) {
    case DS_GALAXY:             return RGB(180, 200, 255);
    case DS_NEBULA:             return RGB(255, 150, 200);
    case DS_OPEN_CLUSTER:       return RGB(255, 255, 200);
    case DS_GLOBULAR_CLUSTER:   return RGB(255, 220, 150);
    case DS_PLANETARY_NEBULA:   return RGB(150, 220, 255);
    case DS_SUPERNOVA_REMNANT:  return RGB(255, 180, 150);
    default:                    return RGB(180, 180, 180);
    }
}

// Draw a glowing circle
static inline void draw_glow_circle(int cx, int cy, double radius, COLORREF color, int glow_layers = 3) {
    for (int i = glow_layers; i >= 0; --i) {
        double r = radius * (1.0 + i * 0.6);
        BYTE r_c = GetRValue(color);
        BYTE g_c = GetGValue(color);
        BYTE b_c = GetBValue(color);
        setfillcolor(RGB(
            (int)(r_c * (0.3 + 0.7 * (1.0 - (double)i / (glow_layers + 1)))),
            (int)(g_c * (0.3 + 0.7 * (1.0 - (double)i / (glow_layers + 1)))),
            (int)(b_c * (0.3 + 0.7 * (1.0 - (double)i / (glow_layers + 1))))
        ));
        solidcircle(cx, cy, (int)r);
    }
    setfillcolor(color);
    solidcircle(cx, cy, (int)radius);
}

static inline const char* get_display_name(const CelestialObject& obj, LanguageId lang) {
    if (lang == LANGID_CHINESE && !obj.name_zh.empty())
        return obj.name_zh.c_str();
    return obj.name.c_str();
}

static inline void draw_sky(const std::vector<CelestialObject>& catalog,
                            double lst, double observer_lat,
                            const Camera& cam, int width, int height,
                            int selected_index, const ObserverConfig& cfg,
                            double frame_time) {
    cleardevice();
    setbkcolor(BLACK);
    setfillcolor(BLACK);
    solidrectangle(0, 0, width, height);

    // 地平辉光
    for (int y = 0; y < height; ++y) {
        double sy = y;
        double alt_approx;
        double dy = (height / 2.0 - sy) / (height / 2.0);
        alt_approx = cam.center_alt + dy * (cam.fov / 2.0);
        double alt_deg = alt_approx * 180.0 / M_PI;
        if (alt_deg < 5.0 && alt_deg > -5.0) {
            double t = 1.0 - fabs(alt_deg) / 5.0;
            int r = (int)(20 * t);
            int g = (int)(15 * t);
            int b = (int)(40 * t);
            setcolor(RGB(r, g, b));
            line(0, y, width, y);
        }
    }

    setbkmode(TRANSPARENT);
    double aspect = (double)width / height;
    double half_fov = cam.fov / 2.0;
    double fov_deg = cam.fov * 180.0 / M_PI;

    double ra_cam, dec_cam;
    horizon_to_equatorial(cam.center_alt, cam.center_az, lst, observer_lat, ra_cam, dec_cam);

    // 【修正】小视场跳过预剔除
    bool skip_culling = (cam.fov < 20.0 * M_PI / 180.0);
    double cos_dist_threshold = -1.0;
    if (!skip_culling) {
        double dist_threshold = half_fov * sqrt(aspect * aspect + 1.0) * 1.2;
        cos_dist_threshold = cos(dist_threshold) - 1e-6;
    }

    for (size_t i = 0; i < catalog.size(); ++i) {
        const auto& obj = catalog[i];

        if (!skip_culling) {
            double dra = obj.ra - ra_cam;
            double cos_dist = sin(obj.dec) * sin(dec_cam) + cos(obj.dec) * cos(dec_cam) * cos(dra);
            if (cos_dist < cos_dist_threshold) continue;
        }

        double mag_limit = 6.0 + (120.0 - fov_deg) * 0.03;
        if (obj.mag > mag_limit) continue;

        double alt, az;
        equatorial_to_horizon(obj.ra, obj.dec, lst, observer_lat, alt, az);

        double dim_factor = 1.0;
        bool below_horizon = false;
        if (alt < 0) {
            below_horizon = true;
            double alt_deg = alt * 180.0 / M_PI;
            if (alt_deg > -10.0)
                dim_factor = 0.3 + 0.2 * (1.0 + alt_deg / 10.0);
            else
                dim_factor = 0.15;
        }

        double sx, sy;
        horizon_to_screen(az, alt, cam.center_az, cam.center_alt, cam.fov, width, height, sx, sy);

        if (sx < -20 || sx > width + 20 || sy < -20 || sy > height + 20) continue;

        bool is_selected = ((int)i == selected_index);
        LanguageId lang = cfg.language;
        const char* disp_name = get_display_name(obj, lang);

        // ---- 绘制各类天体 ----
        if (obj.type == DEEPSKY) {
            COLORREF ds_col_raw = deepsky_color(obj.ds_type);
            BYTE dr = (BYTE)(GetRValue(ds_col_raw) * dim_factor);
            BYTE dg = (BYTE)(GetGValue(ds_col_raw) * dim_factor);
            BYTE db = (BYTE)(GetBValue(ds_col_raw) * dim_factor);
            COLORREF ds_col = RGB(dr, dg, db);
            setfillcolor(ds_col);
            if (obj.ds_type == DS_GALAXY) {
                ellipse((int)sx - 4, (int)sy - 2, (int)sx + 4, (int)sy + 2);
            } else if (obj.ds_type == DS_GLOBULAR_CLUSTER) {
                solidcircle((int)sx, (int)sy, 3);
            } else if (obj.ds_type == DS_OPEN_CLUSTER) {
                circle((int)sx, (int)sy, 3);
            } else if (obj.ds_type == DS_PLANETARY_NEBULA) {
                circle((int)sx, (int)sy, 2);
                setfillcolor(ds_col);
                solidcircle((int)sx, (int)sy, 1);
            } else {
                solidcircle((int)sx, (int)sy, 2);
            }
            if (cfg.showLabels && fov_deg < 90 && !below_horizon) {
                settextcolor(ds_col);
                settextstyle(13, 0, "宋体");
                outtextxy((int)sx + 5, (int)sy - 14, disp_name);
            }
        } else if (obj.type == SUN) {
            COLORREF sun_col = RGB((int)(255 * dim_factor), (int)(255 * dim_factor), (int)(100 * dim_factor));
            draw_glow_circle((int)sx, (int)sy, 7, sun_col, 5);
            if (cfg.showLabels && !below_horizon) {
                settextcolor(sun_col);
                settextstyle(15, 0, "宋体");
                outtextxy((int)sx + 10, (int)sy - 8, disp_name);
            }
        } else if (obj.type == MOON) {
            if (obj.id >= 100) {
                double ssize = star_size(obj.mag, fov_deg);
                COLORREF mcol = RGB((int)(220 * dim_factor), (int)(220 * dim_factor), (int)(240 * dim_factor));
                setfillcolor(mcol);
                solidcircle((int)sx, (int)sy, (int)ssize);
                if (cfg.showLabels && fov_deg < 30 && !below_horizon) {
                    settextcolor(RGB((int)(180 * dim_factor), (int)(180 * dim_factor), (int)(200 * dim_factor)));
                    settextstyle(12, 0, "宋体");
                    outtextxy((int)sx + 3, (int)sy + 3, disp_name);
                }
            } else {
                COLORREF moon_col = RGB((int)(230 * dim_factor), (int)(230 * dim_factor), (int)(230 * dim_factor));
                draw_glow_circle((int)sx, (int)sy, 5, moon_col, 4);
                if (cfg.showLabels && !below_horizon) {
                    settextcolor(RGB((int)(200 * dim_factor), (int)(200 * dim_factor), (int)(200 * dim_factor)));
                    settextstyle(13, 0, "宋体");
                    outtextxy((int)sx + 8, (int)sy - 7, disp_name);
                }
            }
        } else if (obj.type == PLANET) {
            COLORREF pcol_raw = RGB(255, 210, 120);
            if (obj.name == "Mars") pcol_raw = RGB(255, 120, 80);
            else if (obj.name == "Jupiter") pcol_raw = RGB(255, 200, 140);
            else if (obj.name == "Saturn") pcol_raw = RGB(255, 220, 160);
            else if (obj.name == "Venus") pcol_raw = RGB(255, 240, 200);
            else if (obj.name == "Uranus") pcol_raw = RGB(180, 240, 255);
            else if (obj.name == "Neptune") pcol_raw = RGB(100, 150, 255);
            else if (obj.name == "Mercury") pcol_raw = RGB(200, 180, 160);
            else if (obj.name == "Pluto") pcol_raw = RGB(180, 160, 140);
            COLORREF pcol = RGB(
                (int)(GetRValue(pcol_raw) * dim_factor),
                (int)(GetGValue(pcol_raw) * dim_factor),
                (int)(GetBValue(pcol_raw) * dim_factor));
            double psize = star_size(obj.mag, fov_deg) + 1.0;
            draw_glow_circle((int)sx, (int)sy, psize, pcol, 2);
            if (cfg.showLabels && obj.mag < 6.0 && !below_horizon) {
                settextcolor(pcol);
                settextstyle(13, 0, "宋体");
                outtextxy((int)sx + 5, (int)sy - 12, disp_name);
            }
        } else if (obj.type == SATELLITE) {
            COLORREF sat_col = RGB((int)(200 * dim_factor), (int)(255 * dim_factor), (int)(255 * dim_factor));
            setfillcolor(sat_col);
            solidcircle((int)sx, (int)sy, 2);
            setcolor(RGB((int)(100 * dim_factor), (int)(200 * dim_factor), (int)(255 * dim_factor)));
            line((int)sx - 4, (int)sy, (int)sx + 4, (int)sy);
            if (cfg.showLabels) {
                settextcolor(sat_col);
                settextstyle(12, 0, "宋体");
                outtextxy((int)sx + 5, (int)sy - 10, disp_name);
            }
        } else if (obj.type == ASTEROID) {
            COLORREF ast_col = RGB((int)(180 * dim_factor), (int)(170 * dim_factor), (int)(150 * dim_factor));
            setfillcolor(ast_col);
            solidcircle((int)sx, (int)sy, 2);
            if (cfg.showLabels && fov_deg < 60 && !below_horizon) {
                settextcolor(ast_col);
                settextstyle(11, 0, "宋体");
                outtextxy((int)sx + 4, (int)sy - 10, disp_name);
            }
        } else {
            // STAR
            if (fov_deg > 90.0 && obj.mag > 6.0) continue;
            double ssize = star_size(obj.mag, fov_deg);
            COLORREF scol_raw = star_color(obj.mag, obj.bv_color);
            COLORREF scol = RGB(
                (int)(GetRValue(scol_raw) * dim_factor),
                (int)(GetGValue(scol_raw) * dim_factor),
                (int)(GetBValue(scol_raw) * dim_factor));
            double twinkle = 1.0;
            if (obj.mag < 3.0 && !below_horizon) {
                double seed = obj.ra * 1000.0 + obj.dec * 500.0 + frame_time * 2.0;
                twinkle = 0.94 + 0.06 * sin(seed * 7.3);
            }
            if (obj.mag < 2.0) {
                BYTE r = GetRValue(scol);
                BYTE g = GetGValue(scol);
                BYTE b = GetBValue(scol);
                draw_glow_circle((int)sx, (int)sy, ssize * twinkle,
                    RGB((int)(r * twinkle), (int)(g * twinkle), (int)(b * twinkle)), 2);
            } else {
                setfillcolor(scol);
                solidcircle((int)sx, (int)sy, (int)(ssize * twinkle));
            }
            if (cfg.showLabels && obj.mag < cfg.labelMagLimit && !below_horizon) {
                settextcolor(RGB((int)(140 * dim_factor), (int)(140 * dim_factor), (int)(140 * dim_factor)));
                settextstyle(12, 0, "宋体");
                outtextxy((int)sx + 3, (int)sy + 3, disp_name);
            }
        }

        // 选中高亮
        if (is_selected) {
            double sel_dim = below_horizon ? 0.4 : 1.0;
            setcolor(RGB(0, (int)(255 * sel_dim), (int)(128 * sel_dim)));
            setlinestyle(PS_SOLID, 1);
            circle((int)sx, (int)sy, 12);
            line((int)sx - 16, (int)sy, (int)sx - 10, (int)sy);
            line((int)sx + 10, (int)sy, (int)sx + 16, (int)sy);
            line((int)sx, (int)sy - 16, (int)sx, (int)sy - 10);
            line((int)sx, (int)sy + 10, (int)sx, (int)sy + 16);
            if (obj.type == DEEPSKY) {
                const char* ds_type = get_ds_type_str(obj.ds_type, lang);
                settextcolor(RGB((int)(100 * sel_dim), (int)(255 * sel_dim), (int)(180 * sel_dim)));
                settextstyle(11, 0, "宋体");
                int tw = textwidth(ds_type);
                outtextxy((int)sx - tw / 2, (int)sy + 18, ds_type);
            }
        }
    }
}

// --- Reference line clipping helpers ---
static inline bool in_screen(double x, double y, int w, int h) {
    return x >= 0 && x <= w && y >= 0 && y <= h;
}

static int compute_outcode(double x, double y, int w, int h) {
    int code = 0;
    if (x < 0)      code |= 1;
    else if (x > w) code |= 2;
    if (y < 0)      code |= 4;
    else if (y > h) code |= 8;
    return code;
}

static bool clip_line(double &x0, double &y0, double &x1, double &y1, int w, int h) {
    int outcode0 = compute_outcode(x0, y0, w, h);
    int outcode1 = compute_outcode(x1, y1, w, h);
    while (true) {
        if (!(outcode0 | outcode1)) return true;
        if (outcode0 & outcode1) return false;
        int outcodeOut = outcode0 ? outcode0 : outcode1;
        double x, y;
        if (outcodeOut & 8) {
            x = x0 + (x1 - x0) * (h - y0) / (y1 - y0);
            y = h;
        } else if (outcodeOut & 4) {
            x = x0 + (x1 - x0) * (-y0) / (y1 - y0);
            y = 0;
        } else if (outcodeOut & 2) {
            y = y0 + (y1 - y0) * (w - x0) / (x1 - x0);
            x = w;
        } else {
            y = y0 + (y1 - y0) * (-x0) / (x1 - x0);
            x = 0;
        }
        if (outcodeOut == outcode0) {
            x0 = x; y0 = y;
            outcode0 = compute_outcode(x0, y0, w, h);
        } else {
            x1 = x; y1 = y;
            outcode1 = compute_outcode(x1, y1, w, h);
        }
    }
}

// ============================================================
// 【改进】参考线 — 使用真黄赤交角（含章动）
// ============================================================
static inline void draw_sky_reference_lines(double lst, double lat, const Camera& cam,
                                            int width, int height, LanguageId lang,
                                            double jd) {
    const int steps = 180;
    // 【改进】使用真黄赤交角（含章动）
    double eps = true_obliquity(jd);

    auto draw_curve = [&](const std::vector<std::pair<double, double>>& eq_points,
                          COLORREF color, int style = PS_SOLID, int thickness = 1) {
        setlinestyle(style, thickness);
        double prev_sx = -1, prev_sy = -1;
        double prev_alt = 0;
        bool prev_valid = false;
        BYTE base_r = GetRValue(color);
        BYTE base_g = GetGValue(color);
        BYTE base_b = GetBValue(color);
        for (size_t i = 0; i < eq_points.size(); ++i) {
            double ra = eq_points[i].first;
            double dec = eq_points[i].second;
            double alt, az;
            equatorial_to_horizon(ra, dec, lst, lat, alt, az);
            double sx, sy;
            horizon_to_screen(az, alt, cam.center_az, cam.center_alt, cam.fov,
                              width, height, sx, sy);
            if (prev_valid) {
                double avg_alt = (alt + prev_alt) / 2.0;
                double dim = 1.0;
                if (avg_alt < 0) {
                    double alt_deg = avg_alt * 180.0 / M_PI;
                    if (alt_deg > -15.0)
                        dim = 0.3 + 0.4 * (1.0 + alt_deg / 15.0);
                    else
                        dim = 0.15;
                }
                setcolor(RGB((int)(base_r * dim), (int)(base_g * dim), (int)(base_b * dim)));
                double x0 = prev_sx, y0 = prev_sy;
                double x1 = sx, y1 = sy;
                if (clip_line(x0, y0, x1, y1, width, height)) {
                    line((int)x0, (int)y0, (int)x1, (int)y1);
                }
            }
            prev_sx = sx; prev_sy = sy;
            prev_alt = alt;
            prev_valid = true;
        }
        setlinestyle(PS_SOLID);
    };

    // Celestial equator
    std::vector<std::pair<double, double>> celestial_equator;
    for (int i = 0; i <= steps; ++i) {
        double ra = i * 2.0 * M_PI / steps;
        celestial_equator.push_back({ra, 0.0});
    }
    draw_curve(celestial_equator, RGB(0, 100, 200), PS_SOLID, 2);

    {
        double label_sx = -1, label_sy = -1;
        for (size_t i = 0; i < celestial_equator.size(); ++i) {
            double ra = celestial_equator[i].first;
            double dec = celestial_equator[i].second;
            double alt, az;
            equatorial_to_horizon(ra, dec, lst, lat, alt, az);
            if (alt < 0) continue;
            double sx, sy;
            horizon_to_screen(az, alt, cam.center_az, cam.center_alt, cam.fov,
                              width, height, sx, sy);
            if (sx > 20 && sx < width - 100 && sy > 20 && sy < height - 60) {
                label_sx = sx; label_sy = sy;
                break;
            }
        }
        if (label_sx > 0) {
            settextcolor(RGB(0, 140, 220));
            settextstyle(12, 0, "宋体");
            outtextxy((int)label_sx + 5, (int)label_sy - 14, get_str(STR_CELESTIAL_EQUATOR, lang));
        }
    }

    // Ecliptic — 【改进】使用真黄赤交角
    std::vector<std::pair<double, double>> ecliptic;
    for (int i = 0; i <= steps; ++i) {
        double lambda = i * 2.0 * M_PI / steps;
        double ra = atan2(sin(lambda) * cos(eps), cos(lambda));
        if (ra < 0) ra += 2.0 * M_PI;
        double dec = asin(sin(eps) * sin(lambda));
        ecliptic.push_back({ra, dec});
    }
    draw_curve(ecliptic, RGB(200, 160, 0), PS_SOLID, 2);

    {
        double label_sx = -1, label_sy = -1;
        for (size_t i = 0; i < ecliptic.size(); ++i) {
            double ra = ecliptic[i].first;
            double dec = ecliptic[i].second;
            double alt, az;
            equatorial_to_horizon(ra, dec, lst, lat, alt, az);
            if (alt < 0) continue;
            double sx, sy;
            horizon_to_screen(az, alt, cam.center_az, cam.center_alt, cam.fov,
                              width, height, sx, sy);
            if (sx > 20 && sx < width - 100 && sy > 20 && sy < height - 60) {
                label_sx = sx; label_sy = sy;
                break;
            }
        }
        if (label_sx > 0) {
            settextcolor(RGB(220, 180, 0));
            settextstyle(12, 0, "宋体");
            outtextxy((int)label_sx + 5, (int)label_sy - 14, get_str(STR_ECLIPTIC, lang));
        }
    }

    // Galactic equator (J2000 rotation matrix)
    double M_mat[3][3] = {
        {-0.05487556, -0.87343709, -0.48383502},
        { 0.49410945, -0.44482963,  0.74698224},
        {-0.86766615, -0.19807637,  0.45598378}
    };
    std::vector<std::pair<double, double>> galactic_equator;
    for (int i = 0; i <= steps; ++i) {
        double l = i * 2.0 * M_PI / steps;
        double x = M_mat[0][0] * cos(l) + M_mat[0][1] * sin(l);
        double y = M_mat[1][0] * cos(l) + M_mat[1][1] * sin(l);
        double z = M_mat[2][0] * cos(l) + M_mat[2][1] * sin(l);
        double ra = atan2(y, x);
        if (ra < 0) ra += 2.0 * M_PI;
        double dec = asin(z);
        galactic_equator.push_back({ra, dec});
    }
    draw_curve(galactic_equator, RGB(140, 140, 140), PS_DASH, 1);

    {
        double label_sx = -1, label_sy = -1;
        for (size_t i = 0; i < galactic_equator.size(); ++i) {
            double ra = galactic_equator[i].first;
            double dec = galactic_equator[i].second;
            double alt, az;
            equatorial_to_horizon(ra, dec, lst, lat, alt, az);
            if (alt < 0) continue;
            double sx, sy;
            horizon_to_screen(az, alt, cam.center_az, cam.center_alt, cam.fov,
                              width, height, sx, sy);
            if (sx > 20 && sx < width - 100 && sy > 20 && sy < height - 60) {
                label_sx = sx; label_sy = sy;
                break;
            }
        }
        if (label_sx > 0) {
            settextcolor(RGB(160, 160, 160));
            settextstyle(12, 0, "宋体");
            outtextxy((int)label_sx + 5, (int)label_sy - 14, get_str(STR_GALACTIC_EQUATOR, lang));
        }
    }
}

// Draw horizon direction markers (N/E/S/W)
static inline void draw_horizon_markers(const Camera& cam, int width, int height, LanguageId lang) {
    struct DirMarker {
        double az;
        StringId str_id;
        COLORREF color;
    };
    DirMarker markers[] = {
        {0.0,            STR_DIR_N, RGB(180, 180, 220)},
        {M_PI / 2,       STR_DIR_E, RGB(180, 220, 180)},
        {M_PI,           STR_DIR_S, RGB(220, 180, 180)},
        {3.0 * M_PI / 2, STR_DIR_W, RGB(220, 200, 180)},
    };
    setbkmode(TRANSPARENT);
    for (int i = 0; i < 4; ++i) {
        double sx, sy;
        horizon_to_screen(markers[i].az, 0.0, cam.center_az, cam.center_alt, cam.fov,
                          width, height, sx, sy);
        if (sx > 20 && sx < width - 20 && sy > 20 && sy < height - 20) {
            settextcolor(markers[i].color);
            settextstyle(16, 0, "宋体");
            const char* s = get_str(markers[i].str_id, lang);
            int tw = textwidth(s);
            outtextxy((int)sx - tw / 2, (int)sy - 8, s);
        }
    }
}

// Draw help overlay
static inline void draw_help(int width, int height, LanguageId lang) {
    int box_w = 260;
    int box_h = 125;
    int box_x = 10;
    int box_y = 10;
    setfillcolor(RGB(20, 20, 30));
    solidrectangle(box_x, box_y, box_x + box_w, box_y + box_h);
    setcolor(RGB(80, 80, 120));
    rectangle(box_x, box_y, box_x + box_w, box_y + box_h);
    setbkmode(TRANSPARENT);
    settextcolor(RGB(200, 200, 255));
    settextstyle(13, 0, "宋体");
    const char* lines[] = {
        get_str(STR_HELP_1, lang),
        get_str(STR_HELP_2, lang),
        get_str(STR_HELP_3, lang),
        get_str(STR_HELP_4, lang),
        get_str(STR_HELP_5, lang),
        get_str(STR_HELP_6, lang),
    };
    for (int i = 0; i < 6; ++i) {
        outtextxy(box_x + 10, box_y + 10 + i * 18, lines[i]);
    }
}

// Draw about overlay
static inline void draw_about(int width, int height, LanguageId lang) {
    int box_w = 380;
    int box_h = 280;
    int box_x = (width - box_w) / 2;
    int box_y = (height - box_h) / 2;
    setfillcolor(RGB(15, 15, 25));
    solidrectangle(box_x, box_y, box_x + box_w, box_y + box_h);
    setcolor(RGB(100, 100, 160));
    rectangle(box_x, box_y, box_x + box_w, box_y + box_h);
    setbkmode(TRANSPARENT);
    settextcolor(RGB(255, 220, 120));
    settextstyle(20, 0, "宋体");
    outtextxy(box_x + 20, box_y + 15, get_str(STR_ABOUT_TITLE, lang));
    settextcolor(RGB(180, 180, 200));
    settextstyle(15, 0, "宋体");
    outtextxy(box_x + 20, box_y + 45, get_str(STR_ABOUT_VERSION, lang));
    settextcolor(RGB(200, 200, 220));
    settextstyle(13, 0, "宋体");
    const char* desc_lines[] = {
        get_str(STR_ABOUT_DESC1, lang),
        get_str(STR_ABOUT_DESC2, lang),
        get_str(STR_ABOUT_DESC3, lang),
        "",
        get_str(STR_ABOUT_FEATURES, lang),
        get_str(STR_ABOUT_F1, lang),
        get_str(STR_ABOUT_F2, lang),
        get_str(STR_ABOUT_F3, lang),
        get_str(STR_ABOUT_F4, lang),
        get_str(STR_ABOUT_F5, lang),
        "",
        get_str(STR_ABOUT_CLOSE, lang),
    };
    for (int i = 0; i < 12; ++i) {
        outtextxy(box_x + 20, box_y + 70 + i * 17, desc_lines[i]);
    }
}

// Status bar at bottom
static inline void draw_status_bar(int width, int height,
                                   const ObserverConfig& cfg,
                                   double jd,
                                   double center_az, double center_alt, double fov,
                                   double lst, double observer_lat,
                                   const CelestialObject* selected_obj = nullptr)
{
    LanguageId lang = cfg.language;
    int bar_h = 48;
    int bar_top = height - bar_h;
    setfillcolor(RGB(25, 25, 35));
    solidrectangle(0, bar_top, width, height);
    setcolor(RGB(60, 60, 80));
    line(0, bar_top, width, bar_top);
    setbkmode(TRANSPARENT);

    // UTC time
    time_t now = time(0);
    struct tm utc;
#ifdef _WIN32
    struct tm* gmt = gmtime(&now);
    utc = *gmt;
#else
    gmtime_r(&now, &utc);
#endif
    char buf[128];
    snprintf(buf, sizeof(buf), "%s%04d-%02d-%02d %02d:%02d:%02d",
             get_str(STR_UTC_TIME, lang),
             utc.tm_year+1900, utc.tm_mon+1, utc.tm_mday,
             utc.tm_hour, utc.tm_min, utc.tm_sec);
    settextcolor(WHITE);
    settextstyle(13, 0, "宋体");
    outtextxy(8, bar_top + 4, buf);

    // Observer location
    snprintf(buf, sizeof(buf), "%s%.4f  %s%.4f",
             get_str(STR_LAT, lang), cfg.latitude,
             get_str(STR_LON, lang), cfg.longitude);
    settextcolor(RGB(180, 180, 200));
    settextstyle(13, 0, "宋体");
    outtextxy(8, bar_top + 26, buf);

    // Camera info
    snprintf(buf, sizeof(buf), "%s%.1f  %s%.1f  %s%.1f%s",
             get_str(STR_AZ, lang), center_az * 180 / M_PI,
             get_str(STR_ALT, lang), center_alt * 180 / M_PI,
             get_str(STR_FOV, lang), fov * 180 / M_PI,
             get_str(STR_DEG, lang));
    settextcolor(RGB(180, 200, 255));
    outtextxy(280, bar_top + 14, buf);

    // Selected object info
    if (selected_obj) {
        double obj_alt, obj_az;
        equatorial_to_horizon(selected_obj->ra, selected_obj->dec, lst, observer_lat, obj_alt, obj_az);
        double obj_alt_deg = obj_alt * 180.0 / M_PI;
        double obj_az_deg = obj_az * 180.0 / M_PI;

        char info[512] = "";
        strcat(info, get_str(STR_SELECTED, lang));
        strcat(info, get_display_name(*selected_obj, lang));
        strcat(info, "  ");
        if (selected_obj->distance > 0) {
            char dist_str[80];
            if (selected_obj->type == SATELLITE || selected_obj->type == PLANET ||
                selected_obj->type == SUN || selected_obj->type == MOON ||
                selected_obj->type == ASTEROID)
                snprintf(dist_str, sizeof(dist_str), get_str(STR_DIST_AU, lang), selected_obj->distance);
            else
                snprintf(dist_str, sizeof(dist_str), get_str(STR_DIST_LY, lang), selected_obj->distance);
            strcat(info, dist_str);
        } else {
            strcat(info, get_str(STR_DIST_UNKNOWN, lang));
        }
        char mag_str[40];
        snprintf(mag_str, sizeof(mag_str), get_str(STR_MAG, lang), selected_obj->mag);
        strcat(info, mag_str);
        settextcolor(RGB(100, 255, 150));
        outtextxy(540, bar_top + 4, info);

        char coord_str[128];
        snprintf(coord_str, sizeof(coord_str),
                 "%s%.2f  %s%.2f",
                 get_str(STR_AZ, lang), obj_az_deg,
                 get_str(STR_ALT, lang), obj_alt_deg);
        settextcolor(RGB(150, 220, 255));
        outtextxy(540, bar_top + 26, coord_str);
    } else {
        char info[100];
        snprintf(info, sizeof(info), "%s%s", get_str(STR_SELECTED, lang), get_str(STR_NONE, lang));
        settextcolor(RGB(120, 120, 140));
        outtextxy(540, bar_top + 14, info);
    }
}
