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
    #include "pc/network/network_player.h"
    #include "game/object_collision.h"
    #include "game/camera.h"
    #include "game/mario.h"
    #include "engine/math_util.h"
    #include "engine/behavior_script.h"
}

#include <SDL2/SDL.h>

static char animSearchTerm[128];

void BoneEditorWindow() {
    if (current_pluto_anim.Values.size() > 0 && pause_anim && is_editing_panim && override_anim) {
        ImGui::Begin("Animation Pose Editor", &is_editing_panim, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::PushItemWidth(150);
        int currbone = 0;
#define BONE_ENTRY(name) ImGui::DragFloat3(name, bone_rotations[currbone++], 1.0f, 0.0f, 0.0f, "%.0f", ImGuiSliderFlags_AlwaysClamp);
        BONE_ENTRY("Translation"    );
        BONE_ENTRY("Root"           );
        ImGui::Separator();
        BONE_ENTRY("Body"           );
        BONE_ENTRY("Torso"          );
        BONE_ENTRY("Head"           );
        ImGui::Separator();
        BONE_ENTRY("Left Arm"       );
        BONE_ENTRY("Upper Left Arm" );
        BONE_ENTRY("Lower Left Arm" );
        BONE_ENTRY("Left Hand"      );
        ImGui::Separator();
        BONE_ENTRY("Right Arm"      );
        BONE_ENTRY("Upper Right Arm");
        BONE_ENTRY("Lower Right Arm");
        BONE_ENTRY("Right Hand"     );
        ImGui::Separator();
        BONE_ENTRY("Left Leg"       );
        BONE_ENTRY("Upper Left Leg" );
        BONE_ENTRY("Lower Left Leg" );
        BONE_ENTRY("Left Foot"      );
        ImGui::Separator();
        BONE_ENTRY("Right Leg"      );
        BONE_ENTRY("Upper Right Leg");
        BONE_ENTRY("Lower Right Leg");
        BONE_ENTRY("Right Foot"     );
#undef BONE_ENTRY
        ImGui::PopItemWidth();
        ImGui::End();
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
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::EndDisabled();
            ImGui::EndTabItem();
        }
        if (ImGui::IsItemClicked())
            force_set_character_animation(&gMarioStates[0], CHAR_ANIM_FIRST_PERSON);

        // Pluto Animations
        ImGui::BeginDisabled(pluto_animations_list.size() <= 0 || is_editing_panim);
        enable_custom_anim = ImGui::BeginTabItem("PAnim");
        ImGui::EndDisabled();
        // Refresh the list every time the tab opens
        if (ImGui::IsMouseClicked(0) && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            pluto_animations_list = GetPAnimList("dynos/anims");

        if (pluto_animations_list.size() > 0) {
            if (enable_custom_anim) {
                ImGui::BeginDisabled(is_editing_panim);
                saturn_file_browser_filter_extension("panim");
                saturn_file_browser_scan_directory("dynos/anims");
                saturn_file_browser_height(150);
                if (saturn_file_browser_show("panim", -1)) {
                    for (int n = 0; n < pluto_animations_list.size(); n++) {
                        if (pluto_animations_list[n].FilePath.find(saturn_file_browser_get_selected().generic_string()) != std::string::npos) {
                            // Overwrite current animation
                            selected_panim_index = n;
                            current_pluto_anim = LoadPAnim(pluto_animations_list[n].FilePath);
                            loop_anim = current_pluto_anim.Looping;
                            mcomp_extra_bone = current_pluto_anim.BoneCount > 20 ? true : false;
                            if (override_anim && !pause_anim) gMarioStates[0].marioObj->header.gfx.animInfo.animFrame = 0;
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
    if (ImGui::IsItemClicked()) {
        selected_panim_index = 0;
        current_pluto_anim = LoadPAnim(pluto_animations_list[0].FilePath);
        loop_anim = current_pluto_anim.Looping;
    }

    if (ImGui::Checkbox("Override Animation", &override_anim)) {
        is_editing_panim = false;
        pause_anim = false;
        if (!override_anim) set_character_animation(&gMarioStates[0], CHAR_ANIM_START_CROUCHING);
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
            current_pluto_anim = ConvertFromVanilla();
            saturn_play_pluto_animation();
        }
        is_editing_panim = !is_editing_panim;
        if (!is_editing_panim && !enable_custom_anim) override_anim = false;
    }
    ImGui::EndDisabled();
}