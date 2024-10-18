#ifndef SaturnImGui
#define SaturnImGui

#include <SDL2/SDL.h>
#include <PR/ultratypes.h>

#ifdef __cplusplus

#include <string>

extern "C" {
#endif
    extern bool show_menu;
    extern bool show_window_model_settings;
    
    extern bool capture_screenshot;
    extern int screenshot_multiplier;

    void imgui_init_backend(SDL_Window*, SDL_GLContext);
    void imgui_handle_events(SDL_Event*);
    void imgui_handle_binds(int);
    void imgui_update();
    extern bool skybox_has_deinit;
    void imgui_capture_screenshot(void*);

    void UpdatePaletteFromEditor(int);
#ifdef __cplusplus
}
#endif

#endif