#include <windows.h>
#include <graphics.h>
#include <cmath>
#include <vector>
#include <string>
#include <cstdio>
#include "config.h"
#include "astronomy.h"
#include "objects.h"
#include "render.h"
#include "input.h"

int main() {
    ObserverConfig cfg = load_config("config.ini");
    std::vector<CelestialObject> catalog = build_catalog(cfg);

    int width = 1024, height = 600;
    initgraph(width, height);
    setbkcolor(BLACK);
    cleardevice();

    Camera cam;
    cam.center_az = 0;
    cam.center_alt = M_PI / 4;
    cam.fov = 120.0 * M_PI / 180.0;

    InputState input = {false, false, false, false, 0, 0, 0, false,false, false, false, false, false, false};
    int selected = -1;
    bool show_about = false;
    bool show_ref_lines = true;
    double observer_lat = cfg.latitude * M_PI / 180.0;
    double observer_lon = cfg.longitude * M_PI / 180.0;

    double frame_time = 0.0;
    double last_jd = 0.0;

    BeginBatchDraw();
    HWND hWnd = GetHWnd();
    SetWindowText(hWnd, "Star Seeker");
    while (true) {
        if (GetAsyncKeyState(VK_ESCAPE)) break;

        double jd = get_jd_now();
        double lst = fmod(gast(jd) + observer_lon, 2 * M_PI);
        if (lst < 0) lst += 2 * M_PI;
        if (last_jd > 0) {
            frame_time += (jd - last_jd) * 86400.0;
        }
        last_jd = jd;

        update_dynamic(catalog, jd);
        process_input(input);

        if (input.toggle_language) {
            cfg.language = (cfg.language == LANGID_ENGLISH) ? LANGID_CHINESE : LANGID_ENGLISH;
        }
        if (input.toggle_help) {
            cfg.showHelp = !cfg.showHelp;
        }
        if (input.toggle_labels) {
            cfg.showLabels = !cfg.showLabels;
        }
        if (input.toggle_about) {
            show_about = !show_about;
        }
        if (input.toggle_ref_lines) {
            show_ref_lines = !show_ref_lines;
        }

        update_camera(cam, input, 0.008);

        draw_sky(catalog, lst, observer_lat, cam, width, height, selected, cfg, frame_time);

        if (show_ref_lines) {
            draw_sky_reference_lines(lst, observer_lat, cam, width, height, cfg.language, jd);
        }
        draw_horizon_markers(cam, width, height, cfg.language);

        if (input.click_done) {
            double click_az, click_alt;
            screen_to_horizon((double)input.click_x, (double)input.click_y,
                              cam.center_az, cam.center_alt, cam.fov,
                              width, height, click_az, click_alt);
            int best = -1;
            double best_dist = 1e9;
            double fov_deg = cam.fov * 180.0 / M_PI;
            // 【修正】阈值以度为单位，转换为弧度
            double threshold_deg = 0.5;  // 基础0.5度
            double scale = fov_deg / 60.0;
            if (scale < 0.5) scale = 0.5;
            if (scale > 2.0) scale = 2.0;
            double threshold = (threshold_deg * scale) * M_PI / 180.0;
            if (threshold < 0.005) threshold = 0.005;   // 最小约0.29度
            if (threshold > 0.08) threshold = 0.08;     // 最大约4.6度

            for (size_t i = 0; i < catalog.size(); ++i) {
                double alt, az;
                equatorial_to_horizon(catalog[i].ra, catalog[i].dec, lst, observer_lat, alt, az);
                double daz = fabs(az - click_az);
                if (daz > M_PI) daz = 2 * M_PI - daz;
                double dalt = fabs(alt - click_alt);
                double dist = sqrt(daz * daz + dalt * dalt);
                if (dist < threshold && dist < best_dist) {
                    best = (int)i;
                    best_dist = dist;
                }
            }
            selected = best;
            input.click_done = false;
        }

        const CelestialObject* sel_obj = (selected >= 0 && selected < (int)catalog.size()) ?
                                         &catalog[selected] : nullptr;
        draw_status_bar(width, height, cfg, jd,
                        cam.center_az, cam.center_alt, cam.fov,
                        lst, observer_lat,
                        sel_obj);

        if (cfg.showHelp) {
            draw_help(width, height, cfg.language);
        }

        if (show_about) {
            draw_about(width, height, cfg.language);
        }

        FlushBatchDraw();
        Sleep(8);
    }
    EndBatchDraw();
    closegraph();
    return 0;
}
