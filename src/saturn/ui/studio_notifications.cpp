#include "studio_notifications.h"

#include "saturn/ui/saturn_imgui.h"
#include "saturn/libs/imgui/imgui.h"
#include "saturn/libs/imgui/imgui_internal.h"

#include <SDL2/SDL.h>

#include <string>
#include <vector>
#include <algorithm>
#include <memory>

#define NOTIF_WIDTH 300
#define NOTIF_BAR_HEIGHT 4
#define NOTIF_SPACING 8
#define NOTIF_DELAY 5000
#define NOTIF_ANIM_LENGTH 500
#define NOTIF_FLASH_COUNT 5
#define NOTIF_FLASH_PERIOD 250

#define FORCE_HIDE (-NOTIF_ANIM_LENGTH)

uint32_t Notif::id_counter = 0;
static std::vector<std::unique_ptr<Notif>> notifications = {};

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
    int y = 8;
    for (auto& notif : notifications)
        notif->render(&y);

    notifications.erase(std::remove_if(notifications.begin(), notifications.end(), [](std::unique_ptr<Notif>& notif){
        return notif->should_erase();
    }), notifications.end());
}

const NotifColor NotifColor::COL_ERROR   = NotifColor(0x7F1010);
const NotifColor NotifColor::COL_WARN    = NotifColor(0x7F7F10);
const NotifColor NotifColor::COL_SUCCESS = NotifColor(0x107F10);
const NotifColor NotifColor::COL_INFO    = NotifColor();

Notif::Notif(NotifType type, std::string title, std::string message) {
    this->id = Notif::id_counter++;
    this->type = type;
    this->title = title;
    this->message = message;
    this->hide_timer = NOTIF_DELAY;
}

Notif* Notif::push() {
    notifications.push_back(std::make_unique<Notif>(*this));
    return notifications.back().get();
}

Notif* Notif::create_message(NotifColor color, std::string title, std::string message) {
    return Notif(NotifType::MESSAGE, title, message).set(&Notif::color, color).push();
}

Notif* Notif::create_confirm(std::string title, std::string message, std::function<void(bool)> callback, std::string yes, std::string no) {
    return Notif(NotifType::CONFIRM, title, message).set(&Notif::confirm_callback, callback).set(&Notif::yes, yes).set(&Notif::no, no).push();
}

Notif* Notif::create_progress(std::string title, std::string message) {
    return Notif(NotifType::PROGRESS, title, message).push();
}

bool Notif::should_erase() {
    return this->hide_timer < -NOTIF_ANIM_LENGTH;
}

void Notif::render(int* y) {
    int callback_state = -1;

    uint64_t timestamp = SDL_GetTicks64();
    if (this->last_timestamp == 0) last_timestamp = timestamp;
    int ms_passed = timestamp - last_timestamp;

    int width = ImGui::GetMainViewport()->Size.x;

    this->target_y = *y;
    if (this->y == 0) this->y = *y;

    float factor = 8 * (ms_passed / 1000.f);
    if (factor > 1) factor = 1;
    this->y += (this->target_y - this->y) * factor;

    ImGui::SetNextWindowSizeConstraints(ImVec2(NOTIF_WIDTH * ui_scale, 0), ImVec2(NOTIF_WIDTH * ui_scale, 1000 * ui_scale));
    if (this->show_timer < NOTIF_ANIM_LENGTH) ImGui::SetNextWindowPos(ImVec2(interpolate_cubic_out(width, width - NOTIF_WIDTH * ui_scale - NOTIF_SPACING * ui_scale,  this->show_timer / (float)NOTIF_ANIM_LENGTH), this->y));
    else if (this->hide_timer < 0)            ImGui::SetNextWindowPos(ImVec2(interpolate_cubic_in (width - NOTIF_WIDTH * ui_scale - NOTIF_SPACING * ui_scale, width, -this->hide_timer / (float)NOTIF_ANIM_LENGTH), this->y));
    else ImGui::SetNextWindowPos(ImVec2(width - NOTIF_WIDTH * ui_scale - NOTIF_SPACING * ui_scale, this->y));

    if (this->type == NotifType::MESSAGE)
        ImGui::PushStyleColor(ImGuiCol_WindowBg, interpolate_color(0x101010, this->color, get_color_interpolation(this->show_timer)));

    ImGui::Begin(("##notification_" + std::to_string(this->id)).c_str(), NULL,
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_AlwaysAutoResize
    );
    ImGui::BringWindowToDisplayFront(ImGui::GetCurrentWindow());
    *y += ImGui::GetWindowSize().y * ui_scale + NOTIF_SPACING * ui_scale;
    if (ImGui::IsWindowHovered() && this->hide_timer >= 0) {
        this->hide_timer = NOTIF_DELAY;
    }
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + NOTIF_BAR_HEIGHT * ui_scale);
    ImGui::Text("%s", this->title.c_str());
    if (!this->message.empty()) {
        ImGui::Separator();
        ImGui::TextWrapped("%s", this->message.c_str());
    }

    switch (this->type) {
        case NotifType::MESSAGE:
            ImGui::SetCursorPos(ImVec2(0, 0));
            ImGui::ProgressBar(fmax(0, this->hide_timer / (float)NOTIF_DELAY), ImVec2(NOTIF_WIDTH * ui_scale, NOTIF_BAR_HEIGHT * ui_scale), "");
            break;
        case NotifType::PROGRESS:
            ImGui::ProgressBar(this->progress);
            if (this->progress >= 1 && this->hide_timer > 0) this->hide_timer = 0;
            break;
        case NotifType::CONFIRM:
            if (ImGui::Button(this->yes.c_str())) this->confirm_callback(true), this->hide_timer = 0;
            ImGui::SameLine();
            if (ImGui::Button(this->no.c_str())) this->confirm_callback(false), this->hide_timer = 0;
            break;
    }
    
    ImGui::End();

    if (this->type == NotifType::MESSAGE)
        ImGui::PopStyleColor();

    if (this->type == NotifType::MESSAGE || this->hide_timer <= 0) this->hide_timer -= ms_passed;
    this->show_timer += ms_passed;

    this->last_timestamp = timestamp;

    if (callback_state != -1) this->confirm_callback(callback_state);
}
