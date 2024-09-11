#include "saturn_imgui.h"
#include <iostream>

#include "saturn/saturn.h"
#include "saturn/saturn_models.h"
#include "saturn/saturn_animations.h"
#include "saturn/saturn_textures.h"
#include "saturn/saturn_colors.h"

#include <SDL2/SDL.h>

#if defined(__MINGW32__) || defined(OSX_BUILD)
# define GLEW_STATIC
# include <GL/glew.h>
#endif
#define GL_GLEXT_PROTOTYPES 1
#include <stb/stb_image_write.h>

#include "saturn/ui/saturn_imgui_colors.h"
#include "saturn/ui/saturn_imgui_models.h"
#include "saturn/ui/saturn_imgui_world.h"
#include "saturn/ui/saturn_imgui_animations.h"
#include "saturn/libs/imgui/imgui.h"
#include "saturn/libs/imgui/imgui_internal.h"
#include "saturn/libs/imgui/imgui-knobs.h"
#include "saturn/libs/imgui/imgui_impl_sdl.h"
#include "saturn/libs/imgui/imgui_impl_opengl3.h"
#include "saturn/libs/imgui/imgui_impl_opengl3_loader.h"
extern "C" {
    #include "pc/pc_main.h"
    #include "pc/gfx/gfx_pc.h"
    #include "pc/djui/djui.h"
    #include "pc/djui/djui_chat_box.h"
    #include "pc/djui/djui_console.h"
    #include "pc/djui/djui_interactable.h"
    #include "pc/network/network_player.h"
    #include "game/object_collision.h"
    #include "game/camera.h"
    #include "game/mario.h"
    #include "engine/math_util.h"
    #include "engine/behavior_script.h"
}

SDL_Window* current_window = nullptr;
ImGuiIO io;

bool show_menu;
bool show_window_machinima = true;
bool show_window_cc_editor = true;
bool show_window_model_settings = true;
bool show_window_animations = true;

bool capture_screenshot;
bool screenshot_hides_skybox;

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
    pluto_animations_list = GetPAnimList("dynos/anims");
}

void imgui_handle_events(SDL_Event* event) {
    ImGui_ImplSDL2_ProcessEvent(event);
    switch (event->type) {
        case SDL_KEYDOWN:
            if (event->key.keysym.sym == SDLK_F12)
                show_menu = !show_menu;

            if (event->key.keysym.sym == SDLK_F2)
                enable_hud = !enable_hud;

            if (event->key.keysym.sym == SDLK_F3)
                capture_screenshot = true;

            if (event->key.keysym.sym == SDLK_F5)
                auto_chroma = !auto_chroma;

            if (!gDjuiInMainMenu && !gDjuiChatBoxFocus && !gDjuiConsoleFocus && allow_game_input) {
                if (event->key.keysym.sym == SDLK_f)
                    freeze_camera = !freeze_camera;

                if (gMarioStates[0].marioObj && freeze_camera) {
                    if (event->key.keysym.sym == SDLK_o) {
                        gMarioStates[0].marioObj->header.gfx.animInfo.animFrame = 0;
                        override_anim = true;
                    }
                    if (event->key.keysym.sym == SDLK_p && override_anim)
                        pause_anim = !pause_anim;
                }
            }
            break;
    }
}

void imgui_update() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame(current_window);
    ImGui::NewFrame();

    allow_game_input = !ImGui::GetIO().WantTextInput;

    if (show_menu) {
        if (gMarioStates[0].marioObj != NULL) {
        SDL_StartTextInput();

        // Main Menu
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("Menu")) {
                if (ImGui::MenuItem("Show Menu", "F12", show_menu)) show_menu = false;
                if (ImGui::BeginMenu("Screenshot")) {
                    ImGui::Checkbox("Hide Skybox", &screenshot_hides_skybox);
#ifdef __MINGW32__
                    if (ImGui::Button("Copy to Clipboard")) capture_screenshot = true;
#else
                    if (ImGui::Button("Save to File")) capture_screenshot = true;
#endif
                    ImGui::EndMenu();
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Machinima", NULL, show_window_machinima)) show_window_machinima = !show_window_machinima;
                if (ImGui::MenuItem("Color Code Editor", NULL, show_window_cc_editor)) show_window_cc_editor = !show_window_cc_editor;
                if (ImGui::MenuItem("Animation", NULL, show_window_animations, freeze_camera && !enable_head_rotation)) show_window_animations = !show_window_animations;
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Avatar")) {
                ImGui::BeginDisabled(active_saturn_model_index == -1);
                if (ImGui::MenuItem("Model Settings", NULL, show_window_model_settings)) show_window_model_settings = !show_window_model_settings;
                ImGui::EndDisabled();

                // Switch Options
                if (ImGui::BeginMenu("Switches")) {
                    OpenSwitchOptions();
                    ImGui::EndMenu();
                }

                // Model Selector
                ImGui::Separator();
                OpenModelSelector();
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("World")) {
                // Chroma Key (Auto-Chroma)
                OpenAutoChromaMenu();
                
                // Quick Options
                ImGui::Checkbox("HUD", &enable_hud);
                ImGui::Checkbox("Shadows", &enable_shadows);

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

        // Model Packs
        OpenModelSettings();

        // Machinima
        if (show_window_machinima) {
            // Camera
            ImGui::Begin("Machinima", &show_window_machinima, ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::Checkbox("Freeze Camera", &freeze_camera);
            ImGui::BeginDisabled(!freeze_camera);
            ImGui::PushItemWidth(150);
            ImGui::SliderFloat("###freeze_camera_speed", &freeze_camera_speed, 0.f, 6.f, "Speed %.1f");
            ImGui::EndDisabled();
            ImGui::SliderFloat("###camera_fov", &camera_fov, 0.f, 100.f, "FOV %.1f");
            ImGui::PopItemWidth();
            ImGui::Dummy(ImVec2(0, 15));

            // Walkpoint
            ImGui::SetNextItemWidth(150);
            ImGui::SliderInt("###walkpoint", &walkpoint_speed, 0, 127, "Walkpoint %d");
            if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
                ImGui::OpenPopup("###walkpointPresets");
            if (ImGui::BeginPopup("###walkpointPresets")) {
                if (ImGui::Selectable("Running")) walkpoint_speed = 127;
                if (ImGui::Selectable("Walking")) walkpoint_speed = 36;
                if (ImGui::Selectable("Tiptoe")) walkpoint_speed = 25;
                ImGui::EndPopup();
            }
            ImGui::Dummy(ImVec2(0, 15));

            // Rotation Angle
            if (gMarioStates[0].marioObj != NULL) {
            if (ImGuiKnobs::Knob("Angle", &face_angle, -180.f, 180.f, 0.f, "%.0f deg", ImGuiKnobVariant_Dot, 0.f, ImGuiKnobFlags_DragHorizontal))
                gMarioStates[0].faceAngle[1] = (s16)(face_angle * 182.04f);
            else
                face_angle = (float)gMarioStates[0].faceAngle[1] / 182.04;
            }

            ImGui::End();
        }

        // Animation Mixtape
        if (show_window_animations &&
            freeze_camera && !enable_head_rotation) {
                ImGui::Begin("Animation Mixtape", &show_window_animations, ImGuiWindowFlags_AlwaysAutoResize);
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

bool skybox_has_deinit = false;
void imgui_capture_screenshot(void* buffer) {
    uint64_t export_size = (uint64_t)gfx_current_dimensions.width * (uint64_t)gfx_current_dimensions.height * 4;
    unsigned char* image = (unsigned char*)malloc(export_size);
    unsigned char* flipped_image = (unsigned char*)malloc(export_size);
    glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)buffer);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    glBindTexture(GL_TEXTURE_2D, 0);

    for (uint64_t y = 0; y < gfx_current_dimensions.height; y++) {
        for (uint64_t x = 0; x < gfx_current_dimensions.width; x++) {
            uint64_t i = (y * gfx_current_dimensions.width + x) * 4;
            uint64_t j = ((gfx_current_dimensions.height - y - 1) * gfx_current_dimensions.width + x) * 4;
            int r = 0, g = 0, b = 0, a = 0;
            r = image[i + 0];
            g = image[i + 1];
            b = image[i + 2];
            a = image[i + 3];
            flipped_image[j + 0] = r;
            flipped_image[j + 1] = g;
            flipped_image[j + 2] = b;
            flipped_image[j + 3] = a;
        }
    }

#ifdef __MINGW32__
    BITMAPV5HEADER header = {
        .bV5Size = sizeof(header),
        .bV5Width = gfx_current_dimensions.width,
        .bV5Height = gfx_current_dimensions.height, // could be negative to vflip, but some applications do not like it
        .bV5Planes = 1,
        .bV5BitCount = 32,
        .bV5Compression = BI_BITFIELDS,
        .bV5RedMask   = 0x000000ff, // update masks for whatever RGBA byte order you have
        .bV5GreenMask = 0x0000ff00,
        .bV5BlueMask  = 0x00ff0000,
        .bV5AlphaMask = 0xff000000,
        .bV5CSType = LCS_WINDOWS_COLOR_SPACE, // required for alpha support
    };

    HGLOBAL global = GlobalAlloc(GMEM_MOVEABLE, sizeof(header) + gfx_current_dimensions.width*gfx_current_dimensions.height*4);
    if (global) {
        BYTE* buffer = (BYTE*)GlobalLock(global);
        if (buffer) {
            CopyMemory(buffer, &header, sizeof(header));
            // vflip the bitmap manually, for better compatibility
            for (int i=0; i<gfx_current_dimensions.height; i++) {
                CopyMemory(buffer + sizeof(header) + i*gfx_current_dimensions.width*4, flipped_image + (gfx_current_dimensions.height-1-i)*gfx_current_dimensions.width*4, gfx_current_dimensions.width*4);
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
    stbi_write_png("screenshot.png", (int)gfx_current_dimensions.width, (int)gfx_current_dimensions.height, 4, flipped_image, 0);
#endif
    free(image);
    free(flipped_image);

    skybox_has_deinit = false;
    capture_screenshot = false;
}