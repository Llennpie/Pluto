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
    #include "pc/network/network_player.h"
    #include "game/object_collision.h"
    #include "game/camera.h"
    #include "game/mario.h"
    #include "engine/math_util.h"
    #include "engine/behavior_script.h"
    #include "data/dynos.c.h"
}

#include <SDL2/SDL.h>

static char animSearchTerm[128];

// Get bone name for current model and bone index
const char* GetBoneName(int boneIndex) {
    static char bone_label[64];
    
    // Fallback to smart naming based on bone index patterns
    if (boneIndex == 0) return "Translation";
    if (boneIndex == 1) return "Root";
    
    int bone_count = current_pluto_anim.BoneCount + 1;
    
    // For models with the standard SM64 structure (21 bones)
    if (bone_count >= 21) {
        switch (boneIndex) {
            case 2: return "Pelvis";
            case 3: return "Torso";
            case 4: return "Head";
            case 5: return "Left Arm";
            case 6: return "Left Upper Arm";
            case 7: return "Left Lower Arm";
            case 8: return "Left Hand";
            case 9: return "Right Arm";
            case 10: return "Right Upper Arm";
            case 11: return "Right Lower Arm";
            case 12: return "Right Hand";
            case 13: return "Left Leg";
            case 14: return "Left Upper Leg";
            case 15: return "Left Lower Leg";
            case 16: return "Left Foot";
            case 17: return "Right Leg";
            case 18: return "Right Upper Leg";
            case 19: return "Right Lower Leg";
            case 20: return "Right Foot";
        }
    }
    
    // For custom models with different bone counts, use generic names
    snprintf(bone_label, sizeof(bone_label), "Unassigned %d", boneIndex);
    return bone_label;
}

bool UseBoneDivider(int bone_name_index) {
    return (bone_name_index == 2 || bone_name_index == 5 || bone_name_index == 9 ||
        bone_name_index == 13 || bone_name_index == 17 || bone_name_index == 21);
}

void BoneEditorWindow() {
    if (GetTotalBoneCount() <= 0) return;
    if (current_pluto_anim.Values.size() > 0 && pause_anim && is_editing_panim && override_anim) {
        ImGui::Begin("Animation Pose Editor", &is_editing_panim, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::PushItemWidth(150);
        
        // Add 1 to include the "Translation" bone
        int bone_count = GetTotalBoneCount() + 1;

        int bone_name_index = 0, num_custom_bones = 0;
        for (int i = 0; i < bone_count; i++) {
            
            if (UseBoneDivider(bone_name_index)) ImGui::Separator();
            const char* bone_name;
            bool is_custom_bone = i > 0 && i < model_bone_list.size() + 1 && model_bone_list[i-1].second;
            
            if (is_custom_bone) {
                if (!UseBoneDivider(bone_name_index)) ImGui::Separator();
                bone_name = ("> Custom " + std::to_string(i) + " <").c_str();
                num_custom_bones++;
            } else {
                bone_name = GetBoneName(bone_name_index);
                bone_name_index++;
            }
            ImGui::DragFloat3(bone_name, bone_rotations[i], 1.0f, 0.0f, 0.0f, "%.0f", ImGuiSliderFlags_AlwaysClamp);

            // Add button for custom bones to shift values down
            // This helps to fix the bone order without manually entering values
            if (is_custom_bone) {
                ImGui::PushID(i);
                if (ImGui::SmallButton("Push Values Down")) {
                    if (bone_rotations.size() != bone_count)
                        bone_rotations.resize(bone_count);

                    // Shift all values from current position downward
                    for (int j = bone_count - 1; j > i; j--) {
                        if (j - 1 >= 0) {
                            bone_rotations[j][0] = bone_rotations[j-1][0];
                            bone_rotations[j][1] = bone_rotations[j-1][1];
                            bone_rotations[j][2] = bone_rotations[j-1][2];
                        }
                    }
                    // Reset current bone to 0
                    bone_rotations[i][0] = 0.0f;
                    bone_rotations[i][1] = 0.0f;
                    bone_rotations[i][2] = 0.0f;
                }
                ImGui::PopID();
                if (!UseBoneDivider(bone_name_index)) ImGui::Separator();
            }
        }
        
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
                        // Reset pose editor state when switching animations to prevent crashes
                        is_editing_panim = false;
                        bone_rotations.clear();
                        // Force bone count recalculation on next frame
                        ResetBoneCountList();
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
                            current_pluto_anim = LoadPAnim(pluto_animations_list[n].FilePath);
                            loop_anim = current_pluto_anim.Looping;
                            if (override_anim && !pause_anim) gMarioStates[0].marioObj->header.gfx.animInfo.animFrame = 0;
                            // Reset pose editor state when switching animations to prevent crashes
                            is_editing_panim = false;
                            bone_rotations.clear();
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
        current_pluto_anim = LoadPAnim(pluto_animations_list[0].FilePath);
        loop_anim = current_pluto_anim.Looping;
        // Reset pose editor state when switching animations to prevent crashes
        is_editing_panim = false;
        bone_rotations.clear();
    }

    if (ImGui::Checkbox("Override Animation", &override_anim)) {
        is_editing_panim = false;
        bone_rotations.clear();
        pause_anim = false;
        // Force bone count recalculation on next frame
        ResetBoneCountList();
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
        bool was_editing = is_editing_panim;
        is_editing_panim = !is_editing_panim;
        
        // Auto-push custom bones when entering pose editor mode
        if (!was_editing && is_editing_panim) {
            AutoPushCustomBones();
        }
        
        if (!is_editing_panim && !enable_custom_anim) override_anim = false;
    }
    ImGui::EndDisabled();
}