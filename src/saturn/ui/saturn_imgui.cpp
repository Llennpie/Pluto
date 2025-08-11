#include "saturn_imgui.h"
#include <iostream>

#include "saturn/saturn.h"
#include "saturn/saturn_models.h"
#include "saturn/saturn_animations.h"
#include "saturn/saturn_textures.h"
#include "saturn/saturn_colors.h"

#if defined(__MINGW32__) || defined(OSX_BUILD)
# define GLEW_STATIC
# include <GL/glew.h>
#endif
#define GL_GLEXT_PROTOTYPES 1
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_opengl_glext.h>

bool is_wayland() {
#if defined(__MINGW32__) || defined(OSX_BUILD)
    return false;
#else
    return getenv("WAYLAND_DISPLAY");
#endif
}

#include <stdio.h>
#include <time.h>

#include <stb/stb_image_write.h>

#include "saturn/ui/saturn_imgui_colors.h"
#include "saturn/ui/saturn_imgui_models.h"
#include "saturn/ui/saturn_imgui_world.h"
#include "saturn/ui/saturn_imgui_animations.h"
#include "saturn/libs/imgui/imgui.h"
#include "saturn/libs/imgui/imgui_internal.h"
#include "saturn/libs/imgui/imgui_impl_sdl.h"
#include "saturn/libs/imgui/imgui_impl_opengl3.h"
#include "saturn/ui/studio_notifications.h"

extern "C" {
    #include "pc/pc_main.h"
    #include "pc/gfx/gfx_pc.h"
    #include "pc/djui/djui.h"
    #include "pc/djui/djui_chat_box.h"
    #include "pc/djui/djui_console.h"
    #include "pc/djui/djui_interactable.h"
    #include "pc/djui/djui_hud_utils.h"
    #include "pc/network/network_player.h"
    #include "game/object_collision.h"
    #include "game/camera.h"
    #include "game/mario.h"
    #include "game/hud.h"
    #include "engine/math_util.h"
    #include "engine/behavior_script.h"
    #include "game/obj_behaviors.h"
}

#include "data/dynos.cpp.h"

SDL_Window* current_window = nullptr;
ImGuiIO io;

bool show_menu;
bool show_window_machinima = true;
bool show_window_cc_editor = true;
bool show_window_model_settings = true;
bool show_window_animations = true;

bool show_window_mario = false;
int modelw_x, modelw_y, modelw_s;

bool capture_screenshot;
bool screenshot_custom_res;
int screenshot_multiplier = 1;
int screenshot_size[2] = { 320, 240 };

void imgui_init() {
    pluto_animations_list = GetPAnimList("dynos/anims");
}

void imgui_init_backend(SDL_Window* window, SDL_GLContext ctx) {
    current_window = window;

    const char* glsl_version = "#version 120";
    ImGuiContext* imgui = ImGui::CreateContext();
    ImGui::SetCurrentContext(imgui);
    io = ImGui::GetIO(); (void)io;

    io.WantSetMousePos = false;
    io.ConfigWindowsMoveFromTitleBarOnly = true;

    ImGui::StyleColorsPluto();

    ImGui_ImplSDL2_InitForOpenGL(current_window, ctx);
    ImGui_ImplOpenGL3_Init(glsl_version);

    RefreshColorCodeList();
}

void imgui_handle_events(SDL_Event* event) {
    ImGui_ImplSDL2_ProcessEvent(event);
}

void imgui_handle_binds(int scancode) {
    for (int i = 0; i < MAX_BINDS; i++) {
        if (!gDjuiInMainMenu && !gDjuiChatBoxFocus && !gDjuiConsoleFocus && allow_game_input && !gInteractableOverridePad) {
            if (scancode == (int)configKeyPlutoMenu[i])
                show_menu = !show_menu;
            if (scancode == (int)configKeyPlutoScreenshot[i])
                capture_screenshot = true;
            if (scancode == (int)configKeyPlutoChroma[i])
                auto_chroma = !auto_chroma;
        
            if (scancode == (int)configKeyPlutoFreezeCamera[i])
                freeze_camera = !freeze_camera;
            if (scancode == (int)configKeyPlutoHud[i])
                enable_hud = !enable_hud;

            if (gMarioStates[0].marioObj && freeze_camera) {
                if (scancode == (int)configKeyPlutoPlayAnim[i]) {
                    gMarioStates[0].marioObj->header.gfx.animInfo.animFrame = 0;
                    override_anim = true;
                }
                if (scancode == (int)configKeyPlutoPauseAnim[i])
                    pause_anim = !pause_anim;
            }
        }
    }
}

void imgui_update() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame(current_window);
    ImGui::NewFrame();

    allow_game_input = !ImGui::GetIO().WantTextInput;

    studio_render_notifications();

    if (show_menu) {
        if (gMarioStates[0].marioObj != NULL) {
        SDL_StartTextInput();

        // Model Settings
        OpenModelSettings();
        if (!gDjuiInMainMenu && !gDjuiChatBoxFocus && !gDjuiConsoleFocus && !gInteractableOverridePad &&
            AnyModelsEnabled() && active_saturn_model_index != -1) {

                ImGui::SetNextWindowPos(ImVec2(modelw_x, modelw_y));
                ImGui::SetNextWindowSize(ImVec2(modelw_s, modelw_s));
                ImGui::SetNextWindowBgAlpha(0.1f);
                ImGui::Begin("Model", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);
                ImGui::End();

                if (show_window_mario) {
                    if (ImGui::IsMouseReleased(1) && !ImGui::IsAnyItemHovered()) ImGui::OpenPopup("###model_settings");
                    if (!show_window_model_settings) {
                        ImGui::BeginTooltip();
                        ImGui::Text(DynOS_Pack_GetFromIndex(active_saturn_model_index)->mDisplayName.begin());
                        ImGui::TextDisabled("Right-click to open settings");
                        ImGui::EndTooltip();
                    }
                }
        }

        // Main Menu
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("Menu")) {
                if (ImGui::MenuItem("Show Menu", NULL, show_menu)) show_menu = false;
                if (ImGui::BeginMenu("Screenshot")) {
                    ImGui::Checkbox("Custom Size", &screenshot_custom_res);
                    if (screenshot_custom_res) {
                        ImGui::InputInt2("###screenshot_size", screenshot_size, ImGuiInputTextFlags_CharsDecimal);
                    } else {
                        ImGui::SetNextItemWidth(100);
                        ImGui::SliderInt("Multiplier", &screenshot_multiplier, 1, 4);
                        ImGui::TextDisabled("%dx%d", screenshot_size[0], screenshot_size[1]);
                        screenshot_size[0] = gfx_current_dimensions.width * screenshot_multiplier;
                        screenshot_size[1] = gfx_current_dimensions.height * screenshot_multiplier;
                    }
                    if (ImGui::Button("Save Screenshot")) capture_screenshot = true;
                    ImGui::EndMenu();
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Color Code Editor", NULL, show_window_cc_editor)) show_window_cc_editor = !show_window_cc_editor;
                if (ImGui::MenuItem("Animation", NULL, show_window_animations, freeze_camera && !enable_head_rotation)) show_window_animations = !show_window_animations;
                ImGui::EndMenu();
            }

            // Machinima Camera
            if (ImGui::BeginMenu("Camera")) {
                ImGui::Checkbox("Freeze Camera", &freeze_camera);
                ImGui::BeginDisabled(!freeze_camera);
                ImGui::PushItemWidth(150);
                ImGui::SliderFloat("###freeze_camera_speed", &freeze_camera_speed, 0.f, 6.f, "Speed %.1f");
                if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
                    freeze_camera_speed = 1.f;
                ImGui::EndDisabled();
                ImGui::SliderFloat("###camera_fov", &camera_fov, 0.f, 100.f, "FOV %.1f");
                if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
                    camera_fov = 45.f;
                ImGui::PopItemWidth();
                ImGui::EndMenu();
            }

            // Models
            if (ImGui::BeginMenu("Avatar")) {      
                // Switch Options
                if (ImGui::BeginMenu("Switches")) {
                    OpenSwitchOptions();
                    ImGui::EndMenu();
                }

                ImGui::BeginDisabled(gMarioStates[0].marioObj == NULL);
                if (ImGui::BeginMenu("Extra")) {
                    OpenExtraOptions();
                    ImGui::EndMenu();
                }
                ImGui::EndDisabled();

                // Model Selector
                ImGui::Separator();
                OpenModelSelector();
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("World")) {
                OpenAutoChromaMenu();
                OpenQuickOptions();
                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }

        // CC Editor
        if (show_window_cc_editor) {
            ImGui::Begin("Color Code Editor", &show_window_cc_editor, ImGuiWindowFlags_AlwaysAutoResize);
            OpenCCEditor();
            ImGui::End();
        }

        // Animation Mixtape
        if (show_window_animations &&
            freeze_camera && !enable_head_rotation) {
                ImGui::Begin("Animation Mixtape", &show_window_animations, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing);
                OpenAnimationsMenu();
                ImGui::End();
                BoneEditorWindow();
            }
        }
    }

    //ImGui::ShowDemoWindow();

    ImGui::Render();
    GLint last_program;
    glGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
    glUseProgram(0);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glUseProgram(last_program);
}

#ifdef __MINGW32__
#include <windows.h>
#endif

std::string get_screenshot_name() {
    time_t now = time(0);
    tm* ltm = localtime(&now);
    char buffer[80];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d_%H%M%S", ltm);
    std::string filename = "Pluto " + std::string(buffer) + ".png";
    return filename;
}

bool skybox_has_deinit = false;
void imgui_capture_screenshot(void* buffer) {
    GLuint resolve_framebuffer_id;
    GLuint resolve_texture_id;
    glGenFramebuffers(1, &resolve_framebuffer_id);
    glBindFramebuffer(GL_FRAMEBUFFER, resolve_framebuffer_id);

    glGenTextures(1, &resolve_texture_id);
    glBindTexture(GL_TEXTURE_2D, resolve_texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, gfx_current_dimensions.width, gfx_current_dimensions.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, resolve_texture_id, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE) {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, (GLuint)(intptr_t)buffer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, resolve_framebuffer_id);
        glBlitFramebuffer(0, 0, gfx_current_dimensions.width, gfx_current_dimensions.height, 0, gfx_current_dimensions.height ,gfx_current_dimensions.width, 0, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_FRAMEBUFFER, resolve_framebuffer_id);

        unsigned char* pixels = (unsigned char*)malloc(4 * gfx_current_dimensions.width * gfx_current_dimensions.height);
        glReadPixels(0, 0, gfx_current_dimensions.width, gfx_current_dimensions.height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

        const char* path = sys_user_path();
        std::filesystem::path new_path = std::filesystem::path(path) / "screenshots";
        if (!std::filesystem::exists(new_path)) std::filesystem::create_directory(new_path);
        std::string filename = get_screenshot_name();
        new_path /= filename;
        stbi_write_png(new_path.generic_string().c_str(), gfx_current_dimensions.width, gfx_current_dimensions.height, 4, pixels, gfx_current_dimensions.width * 4);

#ifdef __MINGW32__
        // Copy to clipboard (Windows)
        BITMAPV5HEADER header = {
            .bV5Size = sizeof(header),
            .bV5Width = (int)gfx_current_dimensions.width,
            .bV5Height = (int)gfx_current_dimensions.height, // could be negative to vflip, but some applications do not like it
            .bV5Planes = 1,
            .bV5BitCount = 32,
            .bV5Compression = BI_BITFIELDS,
            .bV5RedMask   = 0x000000ff, // update masks for whatever RGBA byte order you have
            .bV5GreenMask = 0x0000ff00,
            .bV5BlueMask  = 0x00ff0000,
            .bV5AlphaMask = 0xff000000,
            .bV5CSType = *(DWORD*)"Win ", // required for alpha support
        };

        HGLOBAL global = GlobalAlloc(GMEM_MOVEABLE, sizeof(header) + gfx_current_dimensions.width * gfx_current_dimensions.height * 4);
        if (global) {
            BYTE* buffer = (BYTE*)GlobalLock(global);
            if (buffer) {
                CopyMemory(buffer, &header, sizeof(header));
                // vflip the bitmap manually, for better compatibility
                for (int i = 0; i < gfx_current_dimensions.height; i++) {
                    CopyMemory(buffer + sizeof(header) + i * gfx_current_dimensions.width * 4, pixels + (gfx_current_dimensions.height - 1 - i) * gfx_current_dimensions.width * 4, gfx_current_dimensions.width * 4);
                }
                GlobalUnlock(global);
            }
            if (OpenClipboard(NULL)) {
                EmptyClipboard();
                SetClipboardData(CF_DIBV5, global);
                CloseClipboard();
            }
        }
#else
#ifndef OSX_BUILD
        // Copy to clipboard (Linux)
        int outlen;
        unsigned char* data = stbi_write_png_to_mem((unsigned char*)pixels, gfx_current_dimensions.width * 4, gfx_current_dimensions.width, gfx_current_dimensions.height, 4, &outlen);
        FILE* command;
        if (is_wayland()) command = popen("wl-copy --type image/png", "w");
        else command = popen("xclip -selection clipboard -t image/png -i", "w");
        fwrite(data, outlen, 1, command);
        fclose(command);
#else
        // Copy to clipboard (macOS)
        FILE* command = popen("osascript -e 'set the clipboard to (read (\"/dev/stdin\") as \"PNG picture\")'", "w");
        fwrite(pixels, 1, gfx_current_dimensions.width * gfx_current_dimensions.height * 4, command);
        fclose(command);
#endif
#endif
        free(pixels);

        studio_notif_info(filename.c_str(), "Saved screenshot to:\n%s/screenshots", std::filesystem::path(path).generic_string().c_str());
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &resolve_framebuffer_id);
    glDeleteTextures(1, &resolve_texture_id);

    skybox_has_deinit = false;
    capture_screenshot = false;
}

void imgui_hud() {
    if (!is_player_active(&gMarioStates[0])) return;
    if (gMarioStates[0].marioObj == NULL) return;

    djui_hud_set_resolution(RESOLUTION_N64);

    if (show_menu) {
        Vec3f pos, out;
        vec3f_copy(pos, gMarioStates[0].marioObj->header.gfx.pos);
        if (djui_hud_world_pos_to_screen_pos(pos, out)) {
            float dist = vec3f_dist(gLakituState.pos, gMarioStates[0].pos);
            float size = 7.f * 7000.f / dist;

            // Find a golden ratio for our resolution and N64 default (320x240)
            float scale_y = gfx_current_dimensions.height / 360.f;
            float new_width = gfx_current_dimensions.width / scale_y;
            float new_height = gfx_current_dimensions.height;

            // Get the cursor position in N64 coordinates
            float cursor_x = djui_hud_get_mouse_x() * djui_gfx_get_scale() / scale_y;
            float cursor_y = djui_hud_get_mouse_y() * djui_gfx_get_scale() / scale_y;

            out[1] += 4;

            // Calculate the box coordinates
            float box_top_left = (out[0] - size / 2) * 1.5f;
            float box_top_right = (out[0] + size / 2) * 1.5f;
            float box_bottom_left = (out[1] - size) * 1.5f;
            float box_bottom_right = (out[1]) * 1.5f;

            show_window_mario = (cursor_x >= box_top_left && cursor_y >= box_bottom_left && cursor_x <= box_top_right && cursor_y <= box_bottom_right);
            modelw_x = box_top_left * scale_y;
            modelw_y = box_bottom_left * scale_y;
            modelw_s = size * 1.5f * scale_y;
        }
    }
}