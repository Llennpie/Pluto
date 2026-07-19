#include "studio_notifications.h"

#include "saturn/ui/saturn_imgui.h"
#include "saturn/libs/imgui/imgui.h"
#include "saturn/libs/imgui/imgui_internal.h"

#include <SDL2/SDL.h>

#include <string>
#include <vector>
#include <algorithm>

#define NOTIF_WIDTH 300
#define NOTIF_BAR_HEIGHT 4
#define NOTIF_SPACING 8
#define NOTIF_DELAY 5000
#define NOTIF_ANIM_LENGTH 500
#define NOTIF_FLASH_COUNT 5
#define NOTIF_FLASH_PERIOD 250

static int notif_id = 0;

struct Notification {
    int id, show_timer, hide_timer, color;
    float y, target_y;
    std::string title, message;
};

std::vector<Notification> notifications = {};

static int interpolate_cubic_in(int from, int to, float x) {
    return (to - from) * (x * x * x) + from;
}

static int interpolate_cubic_out(int from, int to, float x) {
    return (to - from) * (1 - (1 - x) * (1 - x) * (1 - x)) + from;
}

static ImVec4 interpolate_color(int from, int to, float x) {
    int fr = (from >> 16) & 0xFF;
    int fg = (from >>  8) & 0xFF;
    int fb = (from >>  0) & 0xFF;
    int tr = (to   >> 16) & 0xFF;
    int tg = (to   >>  8) & 0xFF;
    int tb = (to   >>  0) & 0xFF;
    int r = (tr - fr) * x + fr;
    int g = (tg - fg) * x + fg;
    int b = (tb - fb) * x + fb;
    if (r < 0) r = 0; if (r > 255) r = 255;
    if (g < 0) g = 0; if (g > 255) g = 255;
    if (b < 0) b = 0; if (b > 255) b = 255;
    return ImVec4(r / 255.f, g / 255.f, b / 255.f, 1.f);
}

static float get_color_interpolation(int timer) {
    return timer < NOTIF_FLASH_PERIOD * NOTIF_FLASH_COUNT ? (cos((timer / (float)NOTIF_FLASH_PERIOD) * 2 * IM_PI + IM_PI) + 1) / 2 : 0;
}

void studio_render_notifications() {
    static uint64_t last_timestamp = 0;
    uint64_t timestamp = SDL_GetTicks64();
    if (last_timestamp == 0) last_timestamp = timestamp;
    int ms_passed = timestamp - last_timestamp;
    int y = 8;
    int width = ImGui::GetMainViewport()->Size.x;
    for (auto& notif : notifications) {
        notif.target_y = y;
        if (notif.y == 0) notif.y = y;
        notif.y += (notif.target_y - notif.y) / 5;
        ImGui::SetNextWindowSizeConstraints(ImVec2(NOTIF_WIDTH * ui_scale, 0), ImVec2(NOTIF_WIDTH * ui_scale, 1000 * ui_scale));
        if (notif.show_timer < NOTIF_ANIM_LENGTH) ImGui::SetNextWindowPos(ImVec2(interpolate_cubic_out(width, width - NOTIF_WIDTH * ui_scale - NOTIF_SPACING * ui_scale,  notif.show_timer / (float)NOTIF_ANIM_LENGTH), notif.y));
        else if (notif.hide_timer < 0)            ImGui::SetNextWindowPos(ImVec2(interpolate_cubic_in (width - NOTIF_WIDTH * ui_scale - NOTIF_SPACING * ui_scale, width, -notif.hide_timer / (float)NOTIF_ANIM_LENGTH), notif.y));
        else ImGui::SetNextWindowPos(ImVec2(width - NOTIF_WIDTH * ui_scale - NOTIF_SPACING * ui_scale, notif.y));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, interpolate_color(0x101010, notif.color, get_color_interpolation(notif.show_timer)));
        ImGui::Begin(("##notification_" + std::to_string(notif.id)).c_str(), NULL,
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoFocusOnAppearing |
            ImGuiWindowFlags_NoNavFocus |
            ImGuiWindowFlags_AlwaysAutoResize
        );
        ImGui::BringWindowToDisplayFront(ImGui::GetCurrentWindow());
        y += ImGui::GetWindowSize().y * ui_scale + NOTIF_SPACING * ui_scale;
        if (ImGui::IsWindowHovered() && notif.hide_timer >= 0) {
            notif.hide_timer = NOTIF_DELAY;
        }
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + NOTIF_BAR_HEIGHT * ui_scale);
        ImGui::Text("%s", notif.title.c_str());
        if (!notif.message.empty()) {
            ImGui::Separator();
            ImGui::TextWrapped("%s", notif.message.c_str());
        }
        ImGui::SetCursorPos(ImVec2(0, 0));
        ImGui::ProgressBar(fmax(0, notif.hide_timer / (float)NOTIF_DELAY), ImVec2(NOTIF_WIDTH * ui_scale, NOTIF_BAR_HEIGHT * ui_scale), "");
        ImGui::End();
        ImGui::PopStyleColor();
        notif.show_timer += ms_passed;
        notif.hide_timer -= ms_passed;
    }
    notifications.erase(std::remove_if(notifications.begin(), notifications.end(), [](Notification& notif) {
        return notif.hide_timer < -NOTIF_ANIM_LENGTH;
    }), notifications.end());
    last_timestamp = timestamp;
}

#define notif_func(type, color)                                            \
    void studio_notif_##type(const char* title, const char* fmt, ...) {     \
        va_list args;                                                        \
        va_start(args, fmt);                                                  \
        studio_notifv_##type(title, fmt, args);                                \
        va_end(args);                                                           \
    }                                                                            \
    void studio_notifv_##type(const char* title, const char* fmt, va_list args) { \
        char buffer[8192];                                                         \
        vsnprintf(buffer, 8192, fmt, args);                                         \
        notifications.push_back({                                                    \
            notif_id++, 0, NOTIF_DELAY, color, 0, 0, title, buffer                    \
        });                                                                            \
    }

notif_func(info,    0x101010)
notif_func(warn,    0x7F7F10)
notif_func(error,   0x7F1010)
notif_func(success, 0x107F10)