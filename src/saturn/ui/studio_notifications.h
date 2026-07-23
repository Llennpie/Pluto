#ifndef StudioNotifications
#define StudioNotifications

#include <SDL2/SDL.h>
#include <PR/ultratypes.h>

#include <functional>

#include <string>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <sm64.h>

extern void studio_render_notifications(void);

enum class NotifType {
    MESSAGE,
    CONFIRM,
    PROGRESS,
};

struct NotifColor {
    bool none;
    uint8_t r, g, b;

    NotifColor(): none(true) {}
    NotifColor(uint8_t r, uint8_t g, uint8_t b): r(r), g(g), b(b), none(false) {}
    NotifColor(uint32_t rgb): r(rgb >> 16), g(rgb >> 8), b(rgb >> 0), none(false) {}
    NotifColor(float r, float g, float b): r(r * 255), g(g * 255), b(b * 255), none(false) {}

    operator uint32_t() {
        return (r << 16) | (g << 8) | b;
    }

    static const NotifColor COL_ERROR;
    static const NotifColor COL_WARN;
    static const NotifColor COL_SUCCESS;
    static const NotifColor COL_INFO;
};

class Notif {
    static uint32_t id_counter;

    NotifType type;
    uint32_t id;
    uint64_t last_timestamp = 0;

    int show_timer = 0, hide_timer;
    float y = 0, target_y = 0;
    std::string title, message, yes, no;
    std::function<void(bool)> confirm_callback;
    NotifColor color = NotifColor::COL_INFO;
    float progress = 0;

    Notif(NotifType type, std::string title, std::string message);
    Notif* push();
    template<typename T> Notif& set(T Notif::* field, T value) {
        this->*field = value;
        return *this;
    }

public:
    bool should_erase();
    void render(int* y);

    static Notif* create_message(NotifColor color, std::string title, std::string message);
    static Notif* create_progress(std::string title, std::string message);
    static Notif* create_confirm(std::string title, std::string message, std::function<void(bool)> callback, std::string yes = "OK", std::string no = "Cancel");

    void set_progress(float progress) { this->progress = progress; }
};

template<typename... T> std::string format(const char* fmt, T... args) {
    int len = snprintf(NULL, 0, fmt, args...);
    std::string str(len, 0);
    sprintf(str.data(), fmt, args...);
    return str;
}

#endif