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

#include <algorithm>

#include <stb/stb_image_write.h>
#include <stb/stb_image.h>

#include "saturn/saturn_keyframe.h"
#include "saturn/ui/saturn_imgui_colors.h"
#include "saturn/ui/saturn_imgui_models.h"
#include "saturn/ui/saturn_imgui_world.h"
#include "saturn/ui/saturn_imgui_animations.h"
#include "saturn/libs/imgui/imgui.h"
#include "saturn/libs/imgui/imgui_stdlib.h"
#include "saturn/libs/imgui/imgui_internal.h"
#include "saturn/libs/imgui/imgui_neo_sequencer.h"
#include "saturn/libs/imgui/imgui_impl_sdl.h"
#include "saturn/libs/imgui/imgui_impl_opengl3.h"
#include "saturn/ui/studio_notifications.h"
#include "libc/math.h"

extern "C" {
    #include "pc/pc_main.h"
    #include "pc/gfx/gfx_pc.h"
    #include "pc/controller/controller_api.h"
    #include "pc/djui/djui.h"
    #include "pc/djui/djui_chat_box.h"
    #include "pc/djui/djui_console.h"
    #include "pc/djui/djui_interactable.h"
    #include "pc/djui/djui_hud_utils.h"
    #include "pc/lua/utils/smlua_text_utils.h"
    #include "pc/network/network_player.h"
    #include "game/object_collision.h"
    #include "game/camera.h"
    #include "game/mario.h"
    #include "game/hud.h"
    #include "engine/math_util.h"
    #include "engine/behavior_script.h"
    #include "game/obj_behaviors.h"
    #include "game/rendering_graph_node.h"
    //#include "data/dynos.c.h"
}

#include "data/dynos.cpp.h"

SDL_Window* current_window = nullptr;

bool show_menu;
bool show_window_machinima = true;
bool show_window_cc_editor = true;
bool show_window_model_settings = false;
bool show_window_animations = true;
bool saturn_any_bone_dot_hovered = false;
bool show_window_dialog = false;
bool show_window_timeline = false;
bool dialog_open = false;

char status_text[256] = { 0 };

std::vector<PlayerWindow> player_windows;

bool capture_screenshot;
bool screenshot_custom_res;
int screenshot_multiplier = 1;
int screenshot_size[2] = { 320, 240 };

float ui_scale = 1.0f;

char uiDialogText[1024 * 16] = "Welcome to Pluto!";

bool show_rule_of_thirds = false;
ALIGNED8 const u8 rule_of_thirds[] = {
#include "textures/segment2/custom_ruleofthirds.rgba32.inc.c"
};
GLuint rot_texture;

bool wireframe_mode = false;

struct CameraSaveState {
    int id;
    std::string name;
    Vec3f pos;
    Vec3f foc;
};
std::vector<CameraSaveState> saved_camera_positions;

// ImGui Theading
#include <pthread.h>

static pthread_t s_build_thread;

static pthread_mutex_t s_kick_mtx  = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t s_kick_cond = PTHREAD_COND_INITIALIZER;
static bool s_kick_flag = false;
static bool s_build_quit = false;

static pthread_mutex_t s_stable_mtx = PTHREAD_MUTEX_INITIALIZER;
static ImVector<ImDrawList*> s_stable_owned;
static ImDrawData s_stable_data;
static bool s_stable_valid = false;

static void stable_copy_draw_data() {
    ImDrawData* src = ImGui::GetDrawData();
    if (!src || !src->Valid) return;

    while (s_stable_owned.Size > src->CmdListsCount) {
        IM_DELETE(s_stable_owned.back());
        s_stable_owned.pop_back();
    }

    while (s_stable_owned.Size < src->CmdListsCount)
        s_stable_owned.push_back(IM_NEW(ImDrawList)(src->CmdLists[0]->_Data));

    for (int i = 0; i < src->CmdListsCount; i++) {
        ImDrawList* s = src->CmdLists[i];
        ImDrawList* d = s_stable_owned[i];
        d->_Data = s->_Data;
        d->Flags = s->Flags;
        d->CmdBuffer = s->CmdBuffer;
        d->VtxBuffer = s->VtxBuffer;
        d->IdxBuffer = s->IdxBuffer;
    }

    s_stable_data = *src;
    s_stable_data.CmdLists.resize(src->CmdListsCount);
    for (int i = 0; i < src->CmdListsCount; i++)
        s_stable_data.CmdLists[i] = s_stable_owned[i];

    s_stable_valid = true;
}

// Expression Previews (queue because of threading)
struct PendingPreview { TexturePath* tex; u8* pixels; int w, h; };
static std::vector<PendingPreview> s_preview_queue;
static pthread_mutex_t s_preview_mtx = PTHREAD_MUTEX_INITIALIZER;

void saturn_request_preview(TexturePath* texture) {
    if (texture->RawData != 0) return;

    FILE* f = fopen(texture->FilePath.c_str(), "rb");
    if (!f) return;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    rewind(f);
    std::vector<unsigned char> buf((size_t)sz);
    fread(buf.data(), 1, (size_t)sz, f);
    fclose(f);

    int w, h;
    u8* pixels = stbi_load_from_memory(buf.data(), (int)sz, &w, &h, NULL, 4);
    if (!pixels) return;

    texture->RawData = pixels;
    texture->Width   = w;
    texture->Height  = h;

    pthread_mutex_lock(&s_preview_mtx);
    s_preview_queue.push_back({texture, pixels, w, h});
    pthread_mutex_unlock(&s_preview_mtx);
}

static void* imgui_build_thread_func(void* arg);

void imgui_init() {
    // Create directories for Pluto content
    // These are located at %appdata%/Llennpie/Pluto on Windows, and ~/.local/share/Llennpie/Pluto on Linux
    std::string user_path = sys_user_path();
    std::error_code ec;

    if (std::filesystem::exists("Panimotion.mp3", ec))
        std::filesystem::copy_file("Panimotion.mp3", user_path + "/Panimotion.mp3",
            std::filesystem::copy_options::skip_existing, ec);

    std::filesystem::create_directories(user_path + "/dynos/colorcodes", ec);
    std::filesystem::create_directories(user_path + "/dynos/anims", ec);
    std::filesystem::create_directories(user_path + "/dynos/eyes", ec);
    std::filesystem::create_directories(user_path + "/dynos/packs", ec);
    pluto_animations_list = GetPAnimList(user_path + "/dynos/anims");
}

void imgui_init_backend(SDL_Window* window, SDL_GLContext ctx) {
    current_window = window;

    const char* glsl_version = "#version 120";
    ImGuiContext* imgui = ImGui::CreateContext();
    ImGui::SetCurrentContext(imgui);
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    io.WantSetMousePos = false;
    io.ConfigWindowsMoveFromTitleBarOnly = true;
    io.ConfigFlags |= ImGuiConfigFlags_NavNoCaptureKeyboard;

    ImGui::StyleColorsPluto();

    ImGui_ImplSDL2_InitForOpenGL(current_window, ctx);
    ImGui_ImplOpenGL3_Init(glsl_version);

    RefreshColorCodeList();

    glGenTextures(1, &rot_texture);
    glBindTexture(GL_TEXTURE_2D, rot_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1280, 720, 0, GL_RGBA, GL_UNSIGNED_BYTE, rule_of_thirds);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    pthread_create(&s_build_thread, NULL, imgui_build_thread_func, NULL);
}

void imgui_handle_events(SDL_Event* event) {
    ImGui_ImplSDL2_ProcessEvent(event);
}

void imgui_handle_binds(int scancode) {
    // Handle Pluto keybinds
    // These are treated like regular SM64 binds and can be changed in Djui
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
                
            if (scancode == (int)configKeyPlutoPlayTimeline[i])
                PlayTimeline();

            if (gMarioStates[0].marioObj && freeze_camera && !is_editing_panim) {
                if (scancode == (int)configKeyPlutoPlayAnim[i]) {
                    // If play and pause anim keys are the same and animation is active, toggle pause
                    if (configKeyPlutoPlayAnim[i] == configKeyPlutoPauseAnim[i] && override_anim) {
                        pause_anim = !pause_anim;
                    } else {
                        gMarioStates[0].marioObj->header.gfx.animInfo.animFrame = 0;
                        override_anim = true;
                    }
                }
                else if (scancode == (int)configKeyPlutoPauseAnim[i])
                    pause_anim = !pause_anim;
            }

            if (scancode == (int)configKeyPlutoFlushTextures[i]) {
                gfx_texture_cache_clear();
                forceReload = true;
            }

            if (scancode == (int)configKeyPlutoCreateDialog[i]) {
                smlua_text_utils_dialog_replace(DIALOG_CUSTOM,1,6,30,200, uiDialogText);
                create_dialog_box(DIALOG_CUSTOM);
            }

            if (scancode == (int)configKeyPlutoRuleOfThirds[i])
                show_rule_of_thirds = !show_rule_of_thirds;
        }
    }
}

void imgui_update() {
    bool camera_automated = timelines.count("Camera###timeline_camera") > 0;
    if (gCamera && !camera_automated && !(timeline_is_playing && freeze_camera)) {
        for (int i = 0; i < 3; i++) {
            camera_kf_state[i]     = gCamera->pos[i];
            camera_kf_state[3 + i] = gCamera->focus[i];
        }
    }

    UpdateTimelines();
    if (timelines.count("Angle") && gMarioStates[0].marioObj)
        gMarioStates[0].faceAngle[1] = (s16)(face_angle * 182.04f);

    // Mini dialog box handler (uses bool for open/close so it can be keyframed)
    {
        static bool s_prev_dialog_open = false;
        static char s_prev_dialog_text[1024 * 16] = {};
        extern s16 gDialogID;
        extern s8  gDialogBoxState;
        extern f32 gDialogBoxOpenTimer;
        extern f32 gDialogBoxScale;
        if (dialog_open && !s_prev_dialog_open) {
            smlua_text_utils_dialog_replace(DIALOG_CUSTOM, 1, 6, 30, 200, uiDialogText);
            create_dialog_box(DIALOG_CUSTOM);
        } else if (!dialog_open && s_prev_dialog_open) {
            // Reset state
            if (gDialogID != -1) {
                gDialogBoxOpenTimer = 0.0f;
                gDialogBoxScale = 1.0f;
                gDialogBoxState = 3; // DIALOG_STATE_CLOSING
            }
        } else if (dialog_open && s_prev_dialog_open && gDialogID == -1) {
            // Dialog was closed elsewhere
            smlua_text_utils_dialog_replace(DIALOG_CUSTOM, 1, 6, 30, 200, uiDialogText);
            create_dialog_box(DIALOG_CUSTOM);
        } else if (dialog_open && gDialogID != -1 && strncmp(uiDialogText, s_prev_dialog_text, sizeof(s_prev_dialog_text)) != 0) {
            // Update text string if it was changed
            smlua_text_utils_dialog_replace(DIALOG_CUSTOM, 1, 6, 30, 200, uiDialogText);
        }
        strncpy(s_prev_dialog_text, uiDialogText, sizeof(s_prev_dialog_text) - 1);
        s_prev_dialog_open = dialog_open;
    }

    if (anim_sync_to_timeline && (is_editing_panim || override_anim) && gMarioStates[0].marioObj) {
        struct Animation* curAnim = gMarioStates[0].marioObj->header.gfx.animInfo.curAnim;
        if (curAnim) {
            int anim_len = curAnim->loopEnd - curAnim->loopStart;
            if (anim_len > 0) {
                timeline_end = anim_len - 1;
                pause_anim = true;
                int clamped = timeline_position < 0 ? 0 : (timeline_position > anim_len - 1 ? anim_len - 1 : timeline_position);
                paused_frame = curAnim->loopStart + clamped;
            }
        }
    }

    pthread_mutex_lock(&s_preview_mtx);
    for (auto& p : s_preview_queue) {
        GLuint tex;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, p.w, p.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, p.pixels);
        glBindTexture(GL_TEXTURE_2D, 0);
        p.tex->Preview = tex;
    }
    s_preview_queue.clear();
    pthread_mutex_unlock(&s_preview_mtx);

    // Apply screenshot size multiplier
    if (!screenshot_custom_res) {
        screenshot_size[0] = gfx_current_dimensions.width  * screenshot_multiplier;
        screenshot_size[1] = gfx_current_dimensions.height * screenshot_multiplier;
    }

    pthread_mutex_lock(&s_stable_mtx);
    if (s_stable_valid) {
        GLint last_program;
        glGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
        glUseProgram(0);
        ImGui_ImplOpenGL3_RenderDrawData(&s_stable_data);
        glUseProgram(last_program);
    }
    pthread_mutex_unlock(&s_stable_mtx);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame(current_window);

    // Rebuild fonts on scale change
    static float s_applied_scale = 1.0f;
    if (ui_scale != s_applied_scale && !ImGui::GetIO().MouseDown[0]) {
        ImGuiIO& io = ImGui::GetIO();
        io.Fonts->Clear();
        ImFontConfig cfg;
        cfg.SizePixels = floorf(13.0f * ui_scale);
        io.Fonts->AddFontDefault(&cfg);
        io.Fonts->Build();
        ImGui_ImplOpenGL3_DestroyFontsTexture();
        ImGui_ImplOpenGL3_CreateFontsTexture();
        s_applied_scale = ui_scale;
    }

    pthread_mutex_lock(&s_kick_mtx);
    s_kick_flag = true;
    pthread_cond_signal(&s_kick_cond);
    pthread_mutex_unlock(&s_kick_mtx);
}

static void imgui_build_widgets();

static void* imgui_build_thread_func(void*) {
    while (true) {
        pthread_mutex_lock(&s_kick_mtx);
        while (!s_kick_flag && !s_build_quit)
            pthread_cond_wait(&s_kick_cond, &s_kick_mtx);
        if (s_build_quit) { pthread_mutex_unlock(&s_kick_mtx); break; }
        s_kick_flag = false;
        pthread_mutex_unlock(&s_kick_mtx);

        ImGui::NewFrame();
        allow_game_input = !ImGui::GetIO().WantTextInput;
        imgui_build_widgets();
        ImGui::Render();

        pthread_mutex_lock(&s_stable_mtx);
        stable_copy_draw_data();
        pthread_mutex_unlock(&s_stable_mtx);
    }
    return NULL;
}

static void imgui_build_widgets() {
    studio_render_notifications();
    
    if (show_rule_of_thirds) {
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
        ImGui::SetNextWindowBgAlpha(0.0f);
        ImGui::Begin("###rule_of_thirds", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoInputs);
        ImGui::Image((ImTextureID)(intptr_t)rot_texture, ImVec2(gfx_current_dimensions.width, gfx_current_dimensions.height));
        ImGui::End();
    }

    if (show_menu) {
        if (gMarioStates[0].marioObj != NULL) {
        //SDL_StartTextInput(); // did i really need this >:(

        // Model Settings
        PopupModelSettings();
        if (!gDjuiInMainMenu && !gDjuiChatBoxFocus && !gDjuiConsoleFocus && !gInteractableOverridePad) {

            if (player_windows.size() > 0) {
                for (int i = 0; i < player_windows.size(); i++) {
                    if (!player_windows[i].active) continue;
                    if (!gNetworkPlayers[i].connected) continue;

                    // Player Windows
                    ImGui::SetNextWindowPos(ImVec2(player_windows[i].x, player_windows[i].y));
                    ImGui::SetNextWindowSize(ImVec2(player_windows[i].size, player_windows[i].size));
                    ImGui::SetNextWindowBgAlpha(0.1f);
                    std::string window_name = std::string(gNetworkPlayers[i].name) + "###player_window_" + std::to_string(i);
                    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoFocusOnAppearing |
                        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus |
                        ImGuiWindowFlags_NoMouseInputs;
                    // Remove title bar if only one player is connected
                    if (network_player_connected_count() <= 1)
                        window_flags |= ImGuiWindowFlags_NoTitleBar;

                    ImGui::Begin(window_name.c_str(), NULL, window_flags);
                    ImGui::End();
                }

                // Right click for model settings
                // WIP: Currently only works for player 1
                if (player_windows[0].active && player_windows[0].hovered &&
                AnyModelsEnabled() && active_saturn_model_index != -1) {
                    if (!show_window_model_settings && !saturn_any_bone_dot_hovered && !ImGui::IsAnyItemHovered() && !ImGui::GetIO().WantCaptureMouse) {
                        ImGui::BeginTooltip();
                        ImGui::Text("Right-click to open model settings");
                        ImGui::EndTooltip();
                        if (ImGui::IsMouseReleased(1))
                            OpenModelSettingsAtCursor();
                    }
                }
            }
        }

        // Main Menu
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("Menu")) {
                if (ImGui::MenuItem("Show Menu", NULL, show_menu)) show_menu = false;
                ImGui::PushItemWidth(150);
                ImGui::SliderFloat("UI Scale", &ui_scale, 1.0f, 3.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_NoInput);
                if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
                    ui_scale = 1.0f;
                ImGui::PopItemWidth();
                if (ImGui::BeginMenu("Screenshot")) {
                    ImGui::Checkbox("Custom Size", &screenshot_custom_res);
                    if (screenshot_custom_res) {
                        ImGui::InputInt2("###screenshot_size", screenshot_size, ImGuiInputTextFlags_CharsDecimal);
                    } else {
                        ImGui::SetNextItemWidth(100 * ui_scale);
                        ImGui::SliderInt("Multiplier", &screenshot_multiplier, 1, 4);
                        ImGui::TextDisabled("%dx%d", screenshot_size[0], screenshot_size[1]);
                    }
                    if (ImGui::Button("Save Screenshot")) capture_screenshot = true;
                    ImGui::EndMenu();
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Color Code Editor", NULL, show_window_cc_editor)) show_window_cc_editor = !show_window_cc_editor;
                if (ImGui::MenuItem("Animation Mixtape", NULL, show_window_animations, freeze_camera)) show_window_animations = !show_window_animations;
                if (ImGui::MenuItem("Textbox Editor", NULL, show_window_dialog)) show_window_dialog = !show_window_dialog;
                if (ImGui::MenuItem("Timeline", NULL, show_window_timeline)) show_window_timeline = !show_window_timeline;
                ImGui::Separator();
                ImGui::Checkbox("Show Wireframes", &wireframe_mode);
                ImGui::EndMenu();
            }

            // Machinima Camera
            if (ImGui::BeginMenu("Camera")) {
                // To-do: This UI is super ugly
                ImGui::PushItemWidth(150 * ui_scale);

                ImGui::Checkbox("Freeze Camera", &freeze_camera);
                ImGui::BeginDisabled(!freeze_camera);

                // Camera Keyframing
                ImGui::SameLine(166 * ui_scale);
                TimelineButton("Camera###timeline_camera", (Timeline) {
                    (void*)camera_kf_state, sizeof(camera_kf_state), false, false,
                    [](void* out, void* a, void* b, float x) {
                        float* o = (float*)out; float* fa = (float*)a; float* fb = (float*)b;
                        for (int i = 0; i < 6; i++) o[i] = fa[i] + (fb[i] - fa[i]) * x;
                    },
                    [](void* a, void* b) { return memcmp(a, b, 6 * sizeof(float)) == 0; },
                    [](void* value) {
                        float* v = (float*)value;
                        ImGui::BeginTooltip();
                        ImGui::Text("Pos (%.1f, %.1f, %.1f)", v[0], v[1], v[2]);
                        ImGui::EndTooltip();
                    }
                });

                if (ImGui::BeginMenu("Settings###camera_settings")) {
                    ImGui::SliderFloat("###freeze_camera_speed", &freeze_camera_speed, 0.f, 6.f, "Move Speed %.1f");
                    if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
                        freeze_camera_speed = 1.f;

                    ImGui::SeparatorText("Position");
                    if (ImGui::InputFloat3("Position###camera_pos", camera_kf_state) && gCamera) {
                        // Maintain focus offset
                        float dx = gCamera->focus[0] - gCamera->pos[0];
                        float dy = gCamera->focus[1] - gCamera->pos[1];
                        float dz = gCamera->focus[2] - gCamera->pos[2];
                        gCamera->pos[0] = camera_kf_state[0];
                        gCamera->pos[1] = camera_kf_state[1];
                        gCamera->pos[2] = camera_kf_state[2];
                        gCamera->focus[0] = camera_kf_state[0] + dx;
                        gCamera->focus[1] = camera_kf_state[1] + dy;
                        gCamera->focus[2] = camera_kf_state[2] + dz;
                        camera_kf_state[3] = gCamera->focus[0];
                        camera_kf_state[4] = gCamera->focus[1];
                        camera_kf_state[5] = gCamera->focus[2];
                    }
                    ImGui::SliderFloat("###camera_tilt", &saturn_camera_tilt, -45.f, 45.f, "Tilt %.1f");
                    if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
                        saturn_camera_tilt = 0.f;
                    ImGui::SameLine(); TimelineButton("Tilt", &saturn_camera_tilt);

                    if (ImGui::MenuItem("Save Current")) {
                        static int counter = 1;
                        saved_camera_positions.push_back({ counter, "Unnamed Camera " + std::to_string(counter),
                            gCamera->pos[0], gCamera->pos[1], gCamera->pos[2],
                            gCamera->focus[0], gCamera->focus[1], gCamera->focus[2]
                        });
                        counter++;
                    }
                    if (!saved_camera_positions.empty()) ImGui::Separator();
                    saved_camera_positions.erase(std::remove_if(
                        saved_camera_positions.begin(), saved_camera_positions.end(),
                        [](CameraSaveState& cam) {
                            std::string delete_id = "X##camdel_" + std::to_string(cam.id);
                            std::string restore_id = ">##camres_" + std::to_string(cam.id);
                            std::string name_id = "##camname_" + std::to_string(cam.id);
    
                            if (ImGui::Button(delete_id.c_str())) return true;
                            if (ImGui::IsItemHovered()) {
                                ImGui::BeginTooltip();
                                ImGui::Text("Remove");
                                ImGui::EndTooltip();
                            }
                            ImGui::SameLine();
                            if (ImGui::Button(restore_id.c_str())) {
                                // Apply saved pos/foc to camera_kf_state so a keyframe can be created if needed
                                camera_kf_state[0] = cam.pos[0];
                                camera_kf_state[1] = cam.pos[1];
                                camera_kf_state[2] = cam.pos[2];
                                camera_kf_state[3] = cam.foc[0];
                                camera_kf_state[4] = cam.foc[1];
                                camera_kf_state[5] = cam.foc[2];
                                if (gCamera) {
                                    vec3f_copy(gCamera->pos, cam.pos);
                                    vec3f_copy(gCamera->focus, cam.foc);
                                    vec3f_copy(gLakituState.goalPos, cam.pos);
                                    vec3f_copy(gLakituState.goalFocus, cam.foc);
                                    vec3f_copy(gLakituState.pos, cam.pos);
                                    vec3f_copy(gLakituState.focus, cam.foc);
                                }
                            }
                            if (ImGui::IsItemHovered()) {
                                ImGui::BeginTooltip();
                                ImGui::Text("Restore");
                                ImGui::EndTooltip();
                            }
                            ImGui::SameLine();
                            ImGui::InputText(name_id.c_str(), &cam.name);

                            return false;
                        }
                    ), saved_camera_positions.end());
                    if (!saved_camera_positions.empty()) {
                        if (ImGui::MenuItem("Remove All")) saved_camera_positions.clear();
                    } 

                    ImGui::EndMenu();
                }
                ImGui::EndDisabled();
                
                ImGui::Separator();

                ImGui::SliderFloat("###camera_follow_speed", &camera_follow_speed, 0.01f, 1.f, "Follow %.2f");
                if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
                    camera_follow_speed = 1.f;
                ImGui::Separator();

                ImGui::SliderFloat("###camera_fov", &saturn_camera_fov, 0.f, 100.f, "FOV %.1f");
                if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
                    saturn_camera_fov = 45.f;
                ImGui::SameLine(); TimelineButton("FOV", &saturn_camera_fov);

                ImGui::Checkbox("Rule of Thirds", &show_rule_of_thirds);

                ImGui::PopItemWidth();
                ImGui::EndMenu();
            }

            // Models
            if (ImGui::BeginMenu("Avatar")) {     
                ImGui::BeginDisabled(!AnyModelsEnabled() || active_saturn_model_index == -1);
                if (ImGui::BeginMenu("Model")) {
                    OpenModelSettings();
                    ImGui::EndMenu();
                }
                ImGui::EndDisabled();
                
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

        // Status Bar
        // Used for displaying extra info (i.e. model auto-reloading)
        ImGuiViewportP* viewport = (ImGuiViewportP*)(void*)ImGui::GetMainViewport();
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoFocusOnAppearing;
        if (status_text[0] != '\0') {
            if (ImGui::BeginViewportSideBar("##MainStatusBar", viewport, ImGuiDir_Up, ImGui::GetFrameHeight(), window_flags)) {
                if (ImGui::BeginMenuBar()) {
                    ImGui::Text(status_text);
                    ImGui::EndMenuBar();
                }
                ImGui::End();
            }
        }

        // CC Editor
        if (show_window_cc_editor) {
            ImGui::Begin("Color Code Editor", &show_window_cc_editor, ImGuiWindowFlags_AlwaysAutoResize);
            OpenCCEditor();
            ImGui::End();
        }

        // Animation Mixtape
        if (show_window_animations &&
            freeze_camera) {
                ImGui::Begin("Animation Mixtape", &show_window_animations, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing);
                OpenAnimationsMenu();
                ImGui::End();
                BoneEditorWindow();
            }
        }

        // Dialog Editor
        if (show_window_dialog) {
            ImGui::Begin("Dialog Textbox", &show_window_dialog, ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::InputTextMultiline("###dialog_box", uiDialogText, IM_ARRAYSIZE(uiDialogText),
                ImVec2(150 * ui_scale, ImGui::GetTextLineHeight() * 6.5f), ImGuiInputTextFlags_None);
            if (show_window_timeline) {
                ImGui::SameLine(); TimelineButton("Textbox Text###timeline_dialog_text", uiDialogText, sizeof(uiDialogText));
                ImGui::Checkbox("Open on Timeline", &dialog_open);
                ImGui::SameLine(); TimelineButton("Textbox###timeline_dialog_open", &dialog_open);
            } else {
                if (ImGui::Button("Create###create_dialog_box")) {
                    smlua_text_utils_dialog_replace(DIALOG_CUSTOM,1,6,30,200, uiDialogText);
                    create_dialog_box(DIALOG_CUSTOM);
                }
            }
            ImGui::End();
        }

        // Timeline
        if (show_window_timeline) {
            // Lower opacity when not focused, because the UI takes up so much screen space lol
            static bool timeline_focused = true;
            bool was_focused = timeline_focused;
            if (!was_focused) ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.2f);
            ImGui::SetNextWindowSizeConstraints(ImVec2(300.0f, 150.0f), ImVec2(FLT_MAX, FLT_MAX));
            ImGui::Begin("Timeline", &show_window_timeline);
            timeline_focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
            if (!was_focused)
                for (int i = 0; i < ImGuiNeoSequencerCol_COUNT; i++) {
                    ImVec4 col = ImGui::GetStyleNeoSequencerColorVec4(i);
                    col.w *= 0.2f;
                    ImGui::PushNeoSequencerStyleColor(i, col);
                }
            RenderTimelineWidget();
            if (!was_focused) ImGui::PopNeoSequencerStyleColor(ImGuiNeoSequencerCol_COUNT);
            ImGui::End();
            if (!was_focused) ImGui::PopStyleVar();

            /*static float value = 0;
            ImGui::Begin("Test", &show_window_timeline, ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::DragFloat("Value", &value);
            ImGui::SameLine();
            TimelineButton("Value", &value);
            ImGui::End();*/
        }
    }

    //ImGui::ShowDemoWindow();
}

#ifdef __MINGW32__
#include <windows.h>
#endif

// Timestamped filename for the screenshot
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
    if (!show_menu) return;
    djui_hud_set_resolution(RESOLUTION_N64);

    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (!is_player_active(&gMarioStates[i])) continue;
        if (gMarioStates[i].marioObj == NULL) continue;

        if (player_windows.size() <= i) player_windows.resize(i + 1);

        Vec3f pos, out;
        vec3f_copy(pos, gMarioStates[i].marioObj->header.gfx.pos);
        if (djui_hud_world_pos_to_screen_pos(pos, out)) {
            float dist = vec3f_dist(gLakituState.pos, gMarioStates[i].pos);
            float size = marioScaleX * (45.f / saturn_camera_fov) * 7.5f * 7000.f / dist;

            player_windows[i].active = dist < 2500.f;
            if (!player_windows[i].active) continue;

            // Find a golden ratio for our resolution and N64 default (320x240)
            float scale_y = gfx_current_dimensions.height / 360.f;
            float new_width = gfx_current_dimensions.width / scale_y;
            float new_height = gfx_current_dimensions.height;

            // Get the cursor position in N64 coordinates
            float cursor_x = djui_hud_get_mouse_x() * djui_gfx_get_scale() / scale_y;
            float cursor_y = djui_hud_get_mouse_y() * djui_gfx_get_scale() / scale_y;

            out[1] -= 4.f / dist / 100.f - 5;

            // Calculate the box coordinates
            float box_top_left = (out[0] - size / 2) * 1.5f;
            float box_top_right = (out[0] + size / 2) * 1.5f;
            float box_bottom_left = (out[1] - size) * 1.5f;
            float box_bottom_right = (out[1]) * 1.5f;

            player_windows[i].hovered = (cursor_x >= box_top_left && cursor_y >= box_bottom_left && cursor_x <= box_top_right && cursor_y <= box_bottom_right);
            player_windows[i].x = box_top_left * scale_y;
            player_windows[i].y = box_bottom_left * scale_y;
            player_windows[i].size = size * 1.5f * scale_y;
        }
    }

}