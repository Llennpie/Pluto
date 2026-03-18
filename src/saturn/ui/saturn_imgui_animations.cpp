#include "saturn_imgui_animations.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>

#include "saturn/saturn.h"
#include "saturn/saturn_colors.h"
#include "saturn/saturn_models.h"
#include "saturn/saturn_textures.h"
#include "saturn/saturn_animations.h"
#include "data/dynos.cpp.h"
#include "saturn/ui/saturn_imgui_file_browser.h"
#include "saturn/libs/imgui/imgui.h"
#include "saturn/libs/imgui/imgui_internal.h"
#include "saturn/libs/imgui/imgui_impl_sdl.h"
#include "saturn/libs/imgui/imgui_impl_opengl3.h"
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
    #include "game/rendering_graph_node.h"
    #include "engine/math_util.h"
    #include "engine/behavior_script.h"
    #include "data/dynos.c.h"
}

#include <SDL2/SDL.h>
#include "libc/math.h"
#include "saturn/ui/saturn_imgui.h"

static char animSearchTerm[128];

static void reset_bone_offsets() {
    g_root_offset[0] = g_root_offset[1] = g_root_offset[2] = 0.0f;
    for (auto& bone : current_pluto_anim.Bones) {
        vec3s_set(bone.Translation, 0, 0, 0);
        bone.Rotation[0] = bone.Rotation[1] = bone.Rotation[2] = 0.0f;
        bone.Scale[0] = bone.Scale[1] = bone.Scale[2] = 1.0f;
    }
}

void BoneEditorWindow() {
    if (current_pluto_anim.Bones.empty()) return;
    static bool bone_editor_was_open = false;
    if (current_pluto_anim.Values.size() > 0 && pause_anim && is_editing_panim && override_anim) {
        ImGui::SetNextWindowSizeConstraints(ImVec2(0.0f, 0.0f), ImVec2(FLT_MAX, 650.0f));
        ImGui::Begin("Animation Pose Editor", &is_editing_panim, ImGuiWindowFlags_AlwaysAutoResize);
        bone_editor_was_open = true;
        ImGui::Text("Edit the current animation pose by\ndragging the translation/rotation/scale\ngizmos in the 3D view.");
        ImGui::PushItemWidth(150);
        ImGui::DragFloat3("Rotation###pose_rotation", current_pluto_anim.Bones[0].Rotation, 1.0f, 0.0f, 0.0f, "%.0f", ImGuiSliderFlags_AlwaysClamp);
        ImGui::DragFloat3("Offset###pose_offset", g_root_offset, 1.0f, 0.0f, 0.0f, "%.0f", ImGuiSliderFlags_AlwaysClamp);
        ImGui::PopItemWidth();
        ImGui::End();
    }
    else if (bone_editor_was_open && !enable_custom_anim) {
        override_anim = false;
        set_character_animation(&gMarioStates[0], CHAR_ANIM_START_CROUCHING);
        bone_editor_was_open = false;
    }

    // Draw bone world-position indicators when the mixtape is open or when hovering the player window
    // To-do: ImGuizmo would be nice here, but the matrixes point weirdly offset to the gizmos and I don't know how to fix it
    static int    s_popup_bone    = -1;
    static ImVec2 s_popup_pos     = ImVec2(0, 0);
    static bool   s_bone_win_open = false;
    saturn_any_bone_dot_hovered = false;

    // Kill whenever the animation override is turned off and camera isn't frozen
    if (!override_anim && !freeze_camera && active_saturn_model_index != -1 && s_popup_bone != -1) {
        s_popup_bone    = -1;
        s_bone_win_open = false;
    }

    bool player_hovered = !player_windows.empty() && player_windows[0].active && player_windows[0].hovered;
    if (show_window_animations && (override_anim || freeze_camera) && active_saturn_model_index != -1 && player_hovered && !current_pluto_anim.Bones.empty()) {
        djui_hud_set_resolution(RESOLUTION_N64);
        float scale_y = gfx_current_dimensions.height / 360.f;
        float factor  = 1.5f * scale_y;
        ImDrawList* draw_list = ImGui::GetBackgroundDrawList();

        float fovDefault = tanf(not_zero(45.0f, gOverrideFOV) * ((float)M_PI / 360.0f));
        float fovCurrent = tanf((gFOVState.fov + gFOVState.fovOffset) * ((float)M_PI / 360.0f));
        float fovFactor  = (fovDefault / fovCurrent) * 1.13f;
        float halfW = (float)djui_hud_get_screen_width()  / 2.0f;
        float halfH = (float)djui_hud_get_screen_height() / 2.0f;

        ImVec2 mouse     = ImGui::GetIO().MousePos;
        bool lmb_clicked = ImGui::GetIO().MouseClicked[0];
        bool clicked_on_dot = false;

        // Don't register dot clicks when the mouse is over the bone editor window
        bool mouse_over_bone_win = s_bone_win_open &&
            ImGui::FindWindowByName("##bone_editor_win") != nullptr &&
            ImGui::FindWindowByName("##bone_editor_win")->Rect().Contains(mouse);

        // Save selection before the loop so mid-loop mutations don't interfere
        int prev_selected = s_popup_bone;

        // Best click candidate: prefer non-selected dots over the selected one
        int    click_bi          = -1;
        ImVec2 click_pos         = ImVec2(0, 0);
        bool   click_is_selected = false;

        for (int bi = 1; bi < (int)current_pluto_anim.Bones.size(); bi++) {
            const PlutoBone& bone = current_pluto_anim.Bones[bi];
            // Hide custom/wiggle bone dots when posing a vanilla animation
            if (!enable_custom_anim && (bone.IsCustom || bone.IsWiggle)) continue;
            const f32* cam = bone.WorldPosition;
            if (cam[2] >= 0.0f) continue;  // behind camera

            float sx = cam[0] * 256.0f / -cam[2];
            float sy = cam[1] * 256.0f /  cam[2];
            sx *= fovFactor;
            sy *= fovFactor;
            sx += halfW;
            sy += halfH;

            float px = sx * factor;
            float py = sy * factor;

            bool is_selected = (prev_selected == bi);
            float dx = mouse.x - px, dy = mouse.y - py;
            float dist2 = dx*dx + dy*dy;
            bool hovered = dist2 <= (5.0f + 6.0f) * (5.0f + 6.0f);
            float dot_r = (is_selected || hovered) ? 7.0f : 5.0f;

            ImU32 fill;
            if (bone.IsWiggle)
                fill = IM_COL32(80, 220, 255, 220);
            else if (bone.IsCustom)
                fill = IM_COL32(255, 165, 0, 220);
            else
                fill = IM_COL32(255, 255, 255, 220);

            if (is_selected)
                draw_list->AddCircleFilled(ImVec2(px, py), dot_r + 2.0f, IM_COL32(255, 255, 100, 80));
            draw_list->AddCircleFilled(ImVec2(px, py), dot_r, fill);
            draw_list->AddCircle(ImVec2(px, py), dot_r, IM_COL32(0, 0, 0, 180), 0, 1.5f);

            // Bone name tooltip on hover
            if (hovered) {
                saturn_any_bone_dot_hovered = true;
                ImGui::BeginTooltip();
                ImGui::TextUnformatted(bone.Name().c_str());
                ImGui::EndTooltip();
            }

            // Click detection - collect best candidate; prefer non-selected
            if (lmb_clicked && !mouse_over_bone_win && dist2 <= (dot_r + 6.0f) * (dot_r + 6.0f)) {
                clicked_on_dot = true;
                // Accept this dot if we have nothing yet, or if it replaces a selected candidate with a non-selected one
                if (click_bi == -1 || (click_is_selected && !is_selected)) {
                    click_bi          = bi;
                    click_pos         = ImVec2(px, py);
                    click_is_selected = is_selected;
                }
            }
        }

        // Apply the best click candidate after the full loop
        if (click_bi != -1) {
            if (click_is_selected) {
                // Clicked the already-selected dot with no better alternative, deselect
                s_popup_bone    = -1;
                s_bone_win_open = false;
            } else {
                s_popup_bone    = click_bi;
                s_popup_pos     = ImVec2(click_pos.x + 80.0f, click_pos.y);
                s_bone_win_open = true;
            }
        }

        // Close window when clicking anywhere that isn't a dot or the bone editor window
        if (lmb_clicked && !clicked_on_dot && !mouse_over_bone_win && s_bone_win_open) {
            s_popup_bone    = -1;
            s_bone_win_open = false;
        }
    }

    // Bone editor window
    if (s_bone_win_open && s_popup_bone >= 1 && s_popup_bone < (int)current_pluto_anim.Bones.size()) {
        PlutoBone& pb = current_pluto_anim.Bones[s_popup_bone];

        ImGui::SetNextWindowPos(s_popup_pos, ImGuiCond_Appearing, ImVec2(0.0f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(270, 0), ImGuiCond_Appearing);
        ImGui::SetNextWindowBgAlpha(0.92f);
        if (ImGui::Begin("##bone_editor_win", &s_bone_win_open,
                         ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_AlwaysAutoResize
                         | ImGuiWindowFlags_NoSavedSettings)) {

            ImGui::TextColored(ImVec4(1.f, 1.f, 0.4f, 1.f), "%s", pb.Name().c_str());
            ImGui::Separator();
            ImGui::PushItemWidth(150);

            if (is_editing_panim && pause_anim && override_anim) {                ImGui::DragFloat3("Rotation", pb.Rotation, 1.0f, -360.0f, 360.0f, "%.0f");
                if (ImGui::IsItemHovered() && ImGui::GetIO().MouseClicked[1])
                    pb.Rotation[0] = pb.Rotation[1] = pb.Rotation[2] = 0;

                float tr[3] = { (float)pb.Translation[0], (float)pb.Translation[1], (float)pb.Translation[2] };
                if (ImGui::DragFloat3("Offset", tr, 1.0f, -32768.0f, 32767.0f, "%.0f")) {
                    pb.Translation[0] = (s16)tr[0];
                    pb.Translation[1] = (s16)tr[1];
                    pb.Translation[2] = (s16)tr[2];
                }
                if (ImGui::IsItemHovered() && ImGui::GetIO().MouseClicked[1])
                    pb.Translation[0] = pb.Translation[1] = pb.Translation[2] = 0;

                float sc[3] = { (float)pb.Scale[0], (float)pb.Scale[1], (float)pb.Scale[2] };
                if (ImGui::DragFloat3("Scale", sc, 0.01f, 0.0f, 0.0f, "%.3f")) {
                    pb.Scale[0] = sc[0];
                    pb.Scale[1] = sc[1];
                    pb.Scale[2] = sc[2];
                }
                if (ImGui::IsItemHovered() && ImGui::GetIO().MouseClicked[1])
                    pb.Scale[0] = pb.Scale[1] = pb.Scale[2] = 1.0f;

                ImGui::Separator();
            }

            // Bone flags!!!
            if (ImGui::Checkbox("Hide Mesh", &pb.BoneHidden)) {
                if (active_saturn_model_index != -1)
                    SaveBoneFlagsToPackDir(DynOS_Pack_GetFromIndex(active_saturn_model_index)->mPath);
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Hide the mesh attached to this bone\nand all its children.");
            if (pb.IsWiggle) {
                if (ImGui::Checkbox("Disable Wiggle", &pb.WiggleDisabled)) {
                    if (active_saturn_model_index != -1)
                        SaveBoneFlagsToPackDir(DynOS_Pack_GetFromIndex(active_saturn_model_index)->mPath);
                }
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Disable spring physics for this wiggle bone.");
                if (ImGui::Checkbox("Disable Wind", &pb.WindDisabled)) {
                    if (active_saturn_model_index != -1)
                        SaveBoneFlagsToPackDir(DynOS_Pack_GetFromIndex(active_saturn_model_index)->mPath);
                }
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Exempt this bone from wind physics.\nIf root of chain: no wind applied.\nIf mid-chain: doesn't increment chain\ndepth for child bones.");
            }

            ImGui::PopItemWidth();
        }
        ImGui::End();

        if (!s_bone_win_open)
            s_popup_bone = -1;
    }
}

void OpenAnimationsMenu() {
    if (ImGui::BeginTabBar("###animation_tab_bar")) {
        // Vanilla Animations
        ImGui::BeginDisabled(is_editing_panim);
        bool vanillaMenuOpen = ImGui::BeginTabItem("Vanilla");
        ImGui::EndDisabled();

        if (vanillaMenuOpen) {
            ImGui::BeginDisabled(is_editing_panim);
            ImGui::SetNextItemWidth(208);
            if (ImGui::BeginCombo("###v_anim_combo", saturn_animations[selected_anim_index], ImGuiComboFlags_None)) {
                ImGui::InputTextWithHint("###anim_search", "Search...", animSearchTerm, IM_ARRAYSIZE(animSearchTerm), ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_CharsUppercase);
                ImGui::Separator();
                for (int n = 0; n < 209; n++) {
                    const bool is_selected = (selected_anim_index == n);

                    if (std::string(saturn_animations[n]).find(animSearchTerm) == std::string::npos &&
                        std::string(animSearchTerm) != "")
                        continue;

                    if (ImGui::Selectable(saturn_animations[n], is_selected)) {
                        selected_anim_index = n;
                        if (override_anim && !pause_anim) gMarioStates[0].marioObj->header.gfx.animInfo.animFrame = 0;
                        // Reset pose editor state when switching animations to prevent crashes
                        is_editing_panim = false;
                        reset_bone_offsets();
                        // Force bone count recalculation on next frame
                        ResetBoneCountList();
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::EndDisabled();
            ImGui::EndTabItem();
        }
        if (ImGui::IsItemClicked()) {
            force_set_character_animation(&gMarioStates[0], CHAR_ANIM_FIRST_PERSON);
            loop_anim = false;
        }

        // Pluto Animations
        ImGui::BeginDisabled(pluto_animations_list.size() <= 0 || is_editing_panim);
        bool prev_custom_anim = enable_custom_anim;
        enable_custom_anim = ImGui::BeginTabItem("PAnim");
        if (prev_custom_anim && !enable_custom_anim)
            SaveAndScheduleRestoreBoneFlags();
        ImGui::EndDisabled();
        // Refresh the list every time the tab opens
        if (ImGui::IsMouseClicked(0) && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            pluto_animations_list = GetPAnimList(std::string(sys_user_path()).append("/dynos/anims"));

        if (pluto_animations_list.size() > 0) {
            if (enable_custom_anim) {
                ImGui::BeginDisabled(is_editing_panim);
                saturn_file_browser_filter_extension("panim");
                saturn_file_browser_scan_directory(std::string(sys_user_path()).append("/dynos/anims"));
                saturn_file_browser_height(150);
                if (saturn_file_browser_show("panim", -1)) {
                    for (int n = 0; n < pluto_animations_list.size(); n++) {
                        if (pluto_animations_list[n].FilePath.find(saturn_file_browser_get_selected().generic_string()) != std::string::npos) {
                            // Overwrite current animation
                            selected_panim_index = n;
                            PreserveBoneFlagsAsPending();
                            current_pluto_anim = LoadPAnim(pluto_animations_list[n].FilePath);
                            loop_anim = current_pluto_anim.Looping;
                            if (override_anim && !pause_anim) gMarioStates[0].marioObj->header.gfx.animInfo.animFrame = 0;
                            // Reset pose editor state when switching animations to prevent crashes
                            is_editing_panim = false;
                            reset_bone_offsets();
                            // Force bone count recalculation on next frame
                            ResetBoneCountList();
                            break;
                        }
                    }
                }
                ImGui::EndDisabled();

                // Metadata
                ImGui::BeginChild("###p_metadata", ImVec2(208, 48), ImGuiChildFlags_Border);
                ImGui::Text("%s", current_pluto_anim.Name.c_str());
                if (ImGui::BeginItemTooltip()) {
                    ImGui::TextUnformatted(current_pluto_anim.Name.c_str());
                    ImGui::EndTooltip();
                }
                ImGui::TextDisabled("@ %s", current_pluto_anim.Author.c_str());
                if (ImGui::BeginItemTooltip()) {
                    ImGui::TextUnformatted(current_pluto_anim.Author.c_str());
                    ImGui::EndTooltip();
                }
                ImGui::EndChild();
                ImGui::EndTabItem();
            }
        }
        ImGui::EndTabBar();
    }
    if (ImGui::IsItemClicked() && pluto_animations_list.size() > 0) {
        selected_panim_index = 0;
        PreserveBoneFlagsAsPending();
        current_pluto_anim = LoadPAnim(pluto_animations_list[0].FilePath);
        loop_anim = current_pluto_anim.Looping;
        // Reset pose editor state when switching animations to prevent crashes
        is_editing_panim = false;
        reset_bone_offsets();
    }

    if (ImGui::Checkbox("Override Animation", &override_anim)) {
        is_editing_panim = false;
        reset_bone_offsets();
        pause_anim = false;
        // Force bone count recalculation on next frame
        ResetBoneCountList();
        if (!override_anim) set_character_animation(&gMarioStates[0], CHAR_ANIM_START_CROUCHING);
        else gMarioStates[0].marioObj->header.gfx.animInfo.animFrame = 0;
    }
    ImGui::Separator();
    ImGui::BeginDisabled(is_editing_panim);
        ImGui::BeginDisabled(!override_anim);
            if (enable_custom_anim && override_anim) ImGui::TextWrapped("Now Playing: %s", current_pluto_anim.Name.c_str());
            else ImGui::TextWrapped("Now Playing: %s", saturn_animations[gMarioStates[0].marioObj->header.gfx.animInfo.animID]);
            ImGui::BeginDisabled(!pause_anim);
                ImGui::SliderInt("###animation_frame", &paused_frame, gMarioStates[0].marioObj->header.gfx.animInfo.curAnim->loopStart, gMarioStates[0].marioObj->header.gfx.animInfo.curAnim->loopEnd-1, "frame %d", ImGuiSliderFlags_AlwaysClamp);
            ImGui::EndDisabled();
            if (ImGui::Checkbox("Paused", &pause_anim))
                if (!pause_anim) hang_anim = false;
        ImGui::EndDisabled();
        ImGui::SameLine(); ImGui::Checkbox("Hang", &hang_anim);
        ImGui::BeginDisabled(hang_anim);
            ImGui::SameLine(); ImGui::Checkbox("Loop", &loop_anim);
        ImGui::EndDisabled();
    ImGui::EndDisabled();

    // Pose Editor
    ImGui::BeginDisabled(!pause_anim && override_anim || !override_anim);
    if (ImGui::Button("Edit Pose")) {
        if (!enable_custom_anim && !is_editing_panim) {
            SaveAndScheduleRestoreBoneFlags();
            current_pluto_anim = ConvertFromVanilla();
            saturn_play_pluto_animation();
        }
        is_editing_panim = !is_editing_panim;
        
        // Auto-push custom bones when entering pose editor mode
        if (is_editing_panim && enable_custom_anim) {
            AutoPushCustomBones();
        }
        
        if (!is_editing_panim && !enable_custom_anim) override_anim = false;
    }
    ImGui::EndDisabled();
}