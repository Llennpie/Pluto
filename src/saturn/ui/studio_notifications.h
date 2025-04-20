#ifndef StudioNotifications
#define StudioNotifications

#include <SDL2/SDL.h>
#include <PR/ultratypes.h>

#ifdef __cplusplus

#include <string>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>

#include <sm64.h>

extern void studio_render_notifications(void);

extern "C" {
#endif
    void studio_notif_info   (const char* title, const char* fmt, ...);
    void studio_notif_warn   (const char* title, const char* fmt, ...);
    void studio_notif_error  (const char* title, const char* fmt, ...);
    void studio_notif_success(const char* title, const char* fmt, ...);
    void studio_notifv_info   (const char* title, const char* fmt, va_list args);
    void studio_notifv_warn   (const char* title, const char* fmt, va_list args);
    void studio_notifv_error  (const char* title, const char* fmt, va_list args);
    void studio_notifv_success(const char* title, const char* fmt, va_list args);
#ifdef __cplusplus
}
#endif

#endif