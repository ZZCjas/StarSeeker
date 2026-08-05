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

    InputState input = {false, false, false, false, 0, 0, 0, false,false, false, false, false, false};
    int selected = -1;
    bool show_about = false;
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
        double lst = fmod(gmst(jd) + observer_lon, 2 * M_PI);
        if (last_jd > 0) {
            frame_time += (jd - last_jd) * 86400.0;  // seconds
        }
        last_jd = jd;

        update_dynamic(catalog, jd);
        process_input(input);

        // Toggle language
        if (input.toggle_language) {
            cfg.language = (cfg.language == LANGID_ENGLISH) ? LANGID_CHINESE : LANGID_ENGLISH;
        }
        // Toggle help
        if (input.toggle_help) {
            cfg.showHelp = !cfg.showHelp;
        }
        // Toggle labels
        if (input.toggle_labels) {
            cfg.showLabels = !cfg.showLabels;
        }
        // Toggle about
        if (input.toggle_about) {
            show_about = !show_about;
        }

        update_camera(cam, input, 0.008);

        // Draw sky
        draw_sky(catalog, lst, observer_lat, cam, width, height, selected, cfg, frame_time);

        // Draw reference lines (celestial equator, ecliptic, galactic)
        draw_sky_reference_lines(lst, observer_lat, cam, width, height, cfg.language);
        // Draw horizon direction markers
        draw_horizon_markers(cam, width, height, cfg.language);

        // Click to select object (including below horizon)
        if (input.click_done) {
            double click_az, click_alt;
            screen_to_horizon((double)input.click_x, (double)input.click_y,
                              cam.center_az, cam.center_alt, cam.fov,
                              width, height, click_az, click_alt);
            int best = -1;
            double best_dist = 1e9;
            double fov_deg = cam.fov * 180.0 / M_PI;
            double threshold = 0.02 * (fov_deg / 60.0);
            if (threshold < 0.005) threshold = 0.005;
            if (threshold > 0.08) threshold = 0.08;
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

        // Status bar
        const CelestialObject* sel_obj = (selected >= 0 && selected < (int)catalog.size()) ?
                                         &catalog[selected] : nullptr;
        draw_status_bar(width, height, cfg, jd,
                        cam.center_az, cam.center_alt, cam.fov,
                        lst, observer_lat,
                        sel_obj);

        // Help overlay
        if (cfg.showHelp) {
            draw_help(width, height, cfg.language);
        }

        // About overlay
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
