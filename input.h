#pragma once
#include <graphics.h>
#include <windows.h>
#include <cmath>
#include "config.h"

struct InputState {
    bool move_up;
    bool move_down;
    bool move_left;
    bool move_right;
    int zoom_delta;
    int click_x, click_y;
    bool click_done;
    bool toggle_language;  // L key pressed
    bool toggle_help;      // H key pressed
    bool toggle_labels;    // T key pressed
    bool toggle_about;     // A key pressed
    bool fine_adjust;      // Shift key pressed
};
static inline void process_input(InputState &in) {
    in.zoom_delta = 0;
    in.click_done = false;
    in.toggle_language = false;
    in.toggle_help = false;
    in.toggle_labels = false;
    in.toggle_about = false;

    ExMessage msg;
    while (peekmessage(&msg, EX_KEY | EX_MOUSE)) {
        if (msg.message == WM_KEYDOWN) {
            switch (msg.vkcode) {
            case VK_UP:    in.move_up = true; break;
            case VK_DOWN:  in.move_down = true; break;
            case VK_LEFT:  in.move_left = true; break;
            case VK_RIGHT: in.move_right = true; break;
            case VK_OEM_PLUS:  in.zoom_delta += 1; break;
            case VK_ADD:       in.zoom_delta += 1; break;
            case VK_OEM_MINUS: in.zoom_delta -= 1; break;
            case VK_SUBTRACT:  in.zoom_delta -= 1; break;
            case VK_SHIFT:
            case VK_LSHIFT:
            case VK_RSHIFT:
                in.fine_adjust = true; break;
            case 0x4C:  in.toggle_language = true; break;  // 'L' key
            case 0x48:  in.toggle_help = true; break;      // 'H' key
            case 0x54:  in.toggle_labels = true; break;    // 'T' key
            case 0x41:  in.toggle_about = true; break;     // 'A' key
            }
        } else if (msg.message == WM_KEYUP) {
            switch (msg.vkcode) {
            case VK_UP:    in.move_up = false; break;
            case VK_DOWN:  in.move_down = false; break;
            case VK_LEFT:  in.move_left = false; break;
            case VK_RIGHT: in.move_right = false; break;
            case VK_SHIFT:
            case VK_LSHIFT:
            case VK_RSHIFT:
                in.fine_adjust = false; break;
            }
        } else if (msg.message == WM_LBUTTONUP) {
            in.click_x = msg.x;
            in.click_y = msg.y;
            in.click_done = true;
        }
    }
}

static inline void update_camera(Camera &cam, const InputState &in, double dt) {
    // 基准视场（度）：在此视场下速度等于基准速度
    const double base_fov_deg = 120.0;
    // 基准角速度（度/秒）：对应 base_fov_deg 时的旋转速度
    const double base_rot_speed_deg = 90.0;
    
    // 当前视场（度）
    double fov_deg = cam.fov * 180.0 / M_PI;
    
    // 动态速度系数：与视场成正比
    // 视场缩小一半，移动速度也缩小一半
    double speed_scale = fov_deg / base_fov_deg;
    
    // 速度上下限：防止过快或完全不动
    if (speed_scale < 0.02) speed_scale = 0.02;
    if (speed_scale > 1.5)  speed_scale = 1.5;
    
    // Shift 键：进一步减速到 1/4（精细微调）
    if (in.fine_adjust) {
        speed_scale *= 0.25;
    }
    
    // 计算当前帧的旋转速度（弧度）
    double rot_speed = base_rot_speed_deg * speed_scale * M_PI / 180.0 * dt;
    
    // 应用移动
    if (in.move_up)    cam.center_alt += rot_speed;
    if (in.move_down)  cam.center_alt -= rot_speed;
    if (in.move_left)  cam.center_az -= rot_speed;
    if (in.move_right) cam.center_az += rot_speed;
    
    // 限制高度角，避免万向节锁
    const double max_alt = 89.9 * M_PI / 180.0;
    if (cam.center_alt > max_alt) cam.center_alt = max_alt;
    if (cam.center_alt < -max_alt) cam.center_alt = -max_alt;
    
    // 方位角环绕
    cam.center_az = fmod(cam.center_az, 2 * M_PI);
    if (cam.center_az < 0) cam.center_az += 2 * M_PI;
    
    // 缩放：缩放速度也随倍率自适应
    if (in.zoom_delta != 0) {
        // 缩放系数同样与视场相关，高倍率下缩放步长更小
        double zoom_factor = pow(0.9, in.zoom_delta);
        // 高倍时缩小缩放步长
        if (fov_deg < 10.0) {
            zoom_factor = pow(zoom_factor, 0.5); // 步长减半
        }
        cam.fov *= zoom_factor;
        
        const double min_fov = 0.1 * M_PI / 180.0;
        const double max_fov = M_PI;
        if (cam.fov < min_fov) cam.fov = min_fov;
        if (cam.fov > max_fov) cam.fov = max_fov;
    }
}
