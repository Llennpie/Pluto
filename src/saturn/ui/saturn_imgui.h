#ifndef SaturnImGui
#define SaturnImGui

#include <SDL2/SDL.h>
#include <PR/ultratypes.h>

#ifdef __cplusplus

#include <string>
#include <vector>

struct PlayerWindow {
    bool active, hovered;
    int x, y, size;
};

extern std::vector<PlayerWindow> player_windows;
extern bool saturn_any_bone_dot_hovered;

extern "C" {
#endif
    extern bool show_menu;
    extern bool show_window_model_settings;
    extern bool show_window_animations;
    extern bool show_window_timeline;
    
    extern bool capture_screenshot;
    extern int screenshot_size[2];
    extern int screenshot_multiplier;
    extern bool screenshot_custom_res;

    extern char status_text[256];
    extern bool wireframe_mode;

    void imgui_init();
    void imgui_init_backend(SDL_Window*, SDL_GLContext);
    void imgui_handle_events(SDL_Event*);
    void imgui_handle_binds(int);
    void imgui_update();
    void imgui_hud();
    extern bool skybox_has_deinit;
    void imgui_capture_screenshot(void*);

    void UpdatePaletteFromEditor(int);
#ifdef __cplusplus
}
#endif

#endif