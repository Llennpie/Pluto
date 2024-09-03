#include "saturn_imgui.h"
#include <iostream>

#include "saturn/saturn.h"
#include "saturn/saturn_models.h"
#include "saturn/saturn_animations.h"
#include "saturn/saturn_textures.h"
#include "saturn/saturn_colors.h"
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

#include <SDL2/SDL.h>

SDL_Window* current_window = nullptr;
ImGuiIO io;

bool show_menu;
bool show_window_machinima = true;
bool show_window_cc_editor = true;
bool show_window_model_settings = true;
bool show_window_animations = true;

void imgui_init_backend(SDL_Window* window, SDL_GLContext ctx) {
    current_window = window;

    const char* glsl_version = "#version 120";
    ImGuiContext* imgui = ImGui::CreateContext();
    ImGui::SetCurrentContext(imgui);
    io = ImGui::GetIO(); (void)io;

    io.WantSetMousePos = false;
    io.ConfigWindowsMoveFromTitleBarOnly = true;

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
                ImGui::Separator();
                if (ImGui::MenuItem("Machinima", NULL, show_window_machinima)) show_window_machinima = !show_window_machinima;
                if (ImGui::MenuItem("Color Code Editor", NULL, show_window_cc_editor)) show_window_cc_editor = !show_window_cc_editor;
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

                if (ImGui::MenuItem("Animation", NULL, show_window_animations, freeze_camera && !enable_head_rotation)) show_window_animations = !show_window_animations;

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
            // Freeze Camera
            ImGui::Begin("Machinima", &show_window_machinima, ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::Checkbox("Freeze Camera", &freeze_camera);
            ImGui::BeginDisabled(!freeze_camera);
            ImGui::PushItemWidth(150);
            ImGui::SliderFloat("###freeze_camera_speed", &freeze_camera_speed, 0.f, 6.f, "Speed %.1f");
            ImGui::EndDisabled();
            ImGui::SliderFloat("###camera_fov", &camera_fov, 0.f, 100.f, "FOV %.1f");
            ImGui::PopItemWidth();

            ImGui::Dummy(ImVec2(0, 15));

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

            // Angle
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

    /*ImGui::Begin("Test");
    if (ImGui::Button("Load")) {
        LoadPAnim("C:\\Users\\llenn\\Documents\\tilt-spaz.panim");
    }
    ImGui::Text("Name: %s", p_name.c_str());
    ImGui::Text("Author: %s", p_author.c_str());
    ImGui::Text("Looping: %02x", p_loop);
    ImGui::Text("Length: %i", p_length);
    ImGui::Text("Nodes: %i", p_nodes);
    if (p_values.size() > 0) ImGui::Text("Values: %02x..%02x (%i)", p_values[0], p_values[p_values.size()-1], p_values.size());
    if (p_indices.size() > 0) ImGui::Text("Indices: %02x..%02x (%i)", p_indices[0], p_indices[p_indices.size()-1], p_indices.size());
    ImGui::Checkbox("Custom", &enable_custom_anim);
    if (ImGui::Button("Play")) {
        set_character_animation(&gMarioStates[0], CHAR_ANIM_A_POSE);
        saturn_play_pluto_animation();
    }
    ImGui::End();*/

    //ImGui::ShowDemoWindow();

    ImGui::Render();
    GLint last_program;
    glGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
    glUseProgram(0);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glUseProgram(last_program);
}