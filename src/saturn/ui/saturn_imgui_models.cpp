#include "saturn_imgui_models.h"

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#endif
#include <iostream>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>
#include <algorithm>
#include <unordered_map>

#include "saturn/saturn.h"
#include "saturn/saturn_colors.h"
#include "saturn/saturn_models.h"
#include "saturn/saturn_textures.h"
#include "saturn/saturn_keyframe.h"
#include "saturn/ui/saturn_imgui.h"
#include "saturn/ui/saturn_imgui_colors.h"
#include "saturn/ui/saturn_imgui_file_browser.h"
#include "saturn/libs/imgui/imgui-knobs.h"
#include "saturn/libs/imgui/imgui.h"
#include "saturn/libs/imgui/imgui_internal.h"
#include "saturn/libs/imgui/imgui_impl_sdl.h"
#include "saturn/libs/imgui/imgui_impl_opengl3.h"
#include "pc/djui/djui.h"
#include "pc/djui/djui_chat_box.h"
#include "pc/djui/djui_console.h"
#include "pc/network/network_player.h"
#include "pc/network/packets/packet.h"
#include "pc/gfx/gfx_pc.h"
#include "pc/pc_main.h"

#include <SDL2/SDL.h>

#include "data/dynos.cpp.h"

extern "C" {
#include "src/game/object_helpers.h"
#include "include/behavior_data.h"
}

bool ignore_expression_visibility;

const char* eye_switches[] = {"Default (Blink)", "Open", "Half", "Closed", "Left", "Right", "Up", "Down", "Dead"};
const char* right_hand_switches[] = {"Default", "Fist", "Open", "Peace", "With Cap", "With Wing Cap"};
const char* left_hand_switches[] = {"Default", "Fist", "Open"};
const char* cap_switches[] = {"Default (On)", "Off", "Wing Cap"}; // unused "wing cap off" not included
const char* powerup_switches[] = {"Default", "None", "Vanish", "Metal", "Vanish & Metal"};

void OpenExpressionPreview(TexturePath* texture) {
    if (ImGui::IsItemHovered() && configExpressionPreviews) {
        if (texture->RawData == 0) {
            // Moved to avoid threading issues with ImGui
            saturn_request_preview(texture);
        } else if (texture->Preview != 0) {
            float maxSize = 128.0f;
            float width = (float)texture->Width;
            float height = (float)texture->Height;
            float scale = 1.0f;
            if (width > maxSize || height > maxSize) {
                scale = maxSize / max(width, height);
                width *= scale;
                height *= scale;
            }
            ImGui::BeginTooltip();
            ImGui::Image((void*)(intptr_t)texture->Preview, ImVec2(width * ui_scale, height * ui_scale));
            ImGui::EndTooltip();
        }
    }
}

/* Expression selector UI for the eye expression */
void OpenEyeSelector() {
    if (current_expressions.size() <= 0) return;
    if (current_expressions[0].Textures.size() <= 0) return;
    if (current_expressions[0].Name == "eyes") {
        static DelayedBool s_eyes_visible;
        bool d_visible = s_eyes_visible(current_expressions[0].Visible);
        // The file browser is always visible, but only interactable when custom eyes are enabled
        ImGui::BeginDisabled(!custom_eyes || (!d_visible && !ignore_expression_visibility));

        saturn_file_browser_filter_extension("png");
        saturn_file_browser_scan_directory(current_expressions[0].FolderPath);
        saturn_file_browser_height(150);
        if (saturn_file_browser_show("eyes", 0)) {
            // Iterate between textures and find the selected one
            for (int tex = 0; tex < current_expressions[0].Textures.size(); tex++) {
                if (current_expressions[0].Textures[tex].SmallExpressionPath("eyes") == saturn_file_browser_get_selected().generic_string()) {
                    if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl)) {
                        // Left Ctrl sets the custom blink cycle
                        if (current_expressions[0].BlinkIndex[0] == -1) current_expressions[0].BlinkIndex[0] = tex;
                        else if (current_expressions[0].BlinkIndex[1] == -1) current_expressions[0].BlinkIndex[1] = tex;
                        else {
                            current_expressions[0].CurrentIndex = tex;
                            current_expressions[0].BlinkIndex[0] = -1;
                            current_expressions[0].BlinkIndex[1] = -1;
                        }
                    } else {
                        // Set expression index
                        current_expressions[0].CurrentIndex = tex;
                        current_expressions[0].BlinkIndex[0] = -1;
                        current_expressions[0].BlinkIndex[1] = -1;
                    }
                    break;
                }
            }
        }

        // Show custom blink cycle UI
        if (ImGui::IsItemHovered() && ImGui::IsKeyDown(ImGuiKey_LeftCtrl)) {
            ImGui::BeginTooltip();
            ImGui::Text("Frame 1: %s", current_expressions[0].Textures[current_expressions[0].CurrentIndex].ShortFileName().c_str());
            ImGui::BeginDisabled(current_expressions[0].BlinkIndex[0] == -1);
                ImGui::Text("Frame 2: %s", (current_expressions[0].BlinkIndex[0] == -1) ? "Click to set" : current_expressions[0].Textures[current_expressions[0].BlinkIndex[0]].ShortFileName().c_str());
            ImGui::EndDisabled();
            ImGui::BeginDisabled(current_expressions[0].BlinkIndex[1] == -1);
                if (current_expressions[0].BlinkIndex[0] == -1) ImGui::Text("Frame 3:"); else
                ImGui::Text("Frame 3: %s", (current_expressions[0].BlinkIndex[1] == -1) ? "Click to set" : current_expressions[0].Textures[current_expressions[0].BlinkIndex[1]].ShortFileName().c_str());
            ImGui::EndDisabled();
            ImGui::EndTooltip();
        }

        ImGui::EndDisabled();
    }
}

void OpenComboPopupMenu(Expression* expression, int index) {
    saturn_file_browser_filter_extension("png");
    saturn_file_browser_scan_directory(expression->FolderPath);
    if (saturn_file_browser_show_tree("expr_" + std::to_string(index), index)) {
        for (int tex = 0; tex < expression->Textures.size(); tex++) {
            if (expression->Textures[tex].SmallExpressionPath(expression->Name) == saturn_file_browser_get_selected().generic_string()) {
                expression->CurrentIndex = tex;
                break;
            }
        }
    }
}

/* Expression selector UI for all other expressions, i.e. individual dropdowns or checkboxes */
void OpenComboSelector(Expression* expression, int index) {
    if (expression->Name == "eyes") return;
    if (expression->Textures.size() <= 1) return;
    if (expression->Visible && (expression->Format != G_IM_FMT_RGBA || expression->Size != G_IM_SIZ_32b) && !format_warning_dismissed) return;

    static std::unordered_map<std::string, DelayedBool> s_visible_map;
    bool d_visible = s_visible_map[expression->Name](expression->Visible);

    // Fun table UI
    ImGui::TableNextRow();
    ImGui::BeginDisabled(!d_visible && !ignore_expression_visibility);

    ImGui::TableSetColumnIndex(0);
    std::string label_name = "###menu_exp_label_" + expression->Name;
    ImGui::Text(expression->Name.c_str());

    ImGui::TableSetColumnIndex(1);
    if (expression->IsToggleFormat() &&
        // Use checkbox
        (expression->Textures[0].FileName.find("default") != std::string::npos ||
        expression->Textures[1].FileName.find("default") != std::string::npos)) {

            // Check which index "default.png" is (0 or 1) to determine default value
            int select_index = (expression->Textures[0].FileName.find("default") != std::string::npos) ? 0 : 1;
            int deselect_index = (select_index == 0) ? 1 : 0;
            bool is_selected = (expression->CurrentIndex == select_index);

            if (ImGui::Checkbox(label_name.c_str(), &is_selected)) {
                if (is_selected) expression->CurrentIndex = select_index;
                else expression->CurrentIndex = deselect_index;
            }
            if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
                ImGui::OpenPopup((std::string(label_name) + "Extended").c_str());
            if (ImGui::BeginPopup((std::string(label_name) + "Extended").c_str())) {
                OpenComboPopupMenu(expression, index);
                ImGui::EndPopup();
            }

            OpenExpressionPreview(&expression->Textures[expression->CurrentIndex]);
    } else {
        // Use dropdown
        std::string defaultLabel = expression->Textures[expression->CurrentIndex].ShortFileName();
        ImGui::PushItemWidth((current_expressions.size() > 8) ? ImGui::GetColumnWidth(1) - 14 : ImGui::GetColumnWidth(1));

        if (ImGui::BeginCombo(label_name.c_str(), defaultLabel.c_str(), ImGuiComboFlags_None)) {
            OpenComboPopupMenu(expression, index);
            ImGui::EndCombo();
        }

        ImGui::PopItemWidth();
    }
    ImGui::EndDisabled();
}

void OpenModelExpressionSelector(PackData* pack) {
    if (!pack->mEnabled || !pack->mLoaded || !IsSaturnModel(pack->mIndex) || current_expressions.size() <= 0 || active_saturn_model_index == -1) return;
    if (model_color_code_list.size() > 0) ImGui::Separator();

    // Eyes
    if (current_expressions[0].Name == "eyes") {
        ImGui::BeginDisabled(current_expressions[0].Textures.size() <= 0);
        if (ImGui::Checkbox("Custom Eyes", &custom_eyes)) {
            // Swap switch state
            if (!custom_eyes) switch_state_eyes = 0;
            else if (switch_state_eyes <= 3 || switch_state_eyes == 8) switch_state_eyes = 4;
        }
        ImGui::EndDisabled();
        OpenEyeSelector();
    }

    // Other Expressions
    if (current_expressions.size() > 1) {
        // A scrollbar is visible if the expressions list is too long
        if (GetValidExpressionCount(current_expressions) > 8)
            ImGui::BeginChild(("###menu_exp_model"), ImVec2(200 * ui_scale, 190 * ui_scale), ImGuiChildFlags_ResizeY, ImGuiWindowFlags_NoBackground);
        
        // Create table UI
        ImGui::BeginTable("###menu_exp_table", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg, ImVec2(200 * ui_scale, 0));
        ImGui::TableSetupColumn("Expression", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

        // Individual expression selector
        for (int i = 0; i < current_expressions.size(); i++)
            OpenComboSelector(&current_expressions[i], i);

        ImGui::EndTable();
        if (GetValidExpressionCount(current_expressions) > 8)
            ImGui::EndChild();
    }

    // Warning label for non-RGBA32 textures
    if (!IsAllRGBA32(current_expressions) && !format_warning_dismissed) {
        ImGui::BeginChild("###menu_model_warning", ImVec2(200 * ui_scale, 75 * ui_scale), ImGuiChildFlags_Border, ImGuiWindowFlags_None);
        ImGui::BeginDisabled();
        ImGui::TextWrapped("Some materials are not set to texture format RGBA-32 and have been disabled.");
        ImGui::TextWrapped("Did you delete the mario_geo.bin?");
        //ImGui::TextWrapped("Some materials are not set to texture format RGBA-32 - this may cause graphical errors!");
        ImGui::EndDisabled();
        if (ImGui::MenuItem("Show Anyway###dismiss_model_warning"))
            format_warning_dismissed = true;
        ImGui::EndChild();
    }
}

void OpenModelCCSelector(PackData* pack) {
    if (!IsSaturnModel(pack->mIndex) || active_saturn_model_index == -1) return;

    bool has_default = std::filesystem::exists(pack->mPath + "/default.gs");
    bool has_colorcodes = std::filesystem::is_directory(pack->mPath + "/colorcodes");
    if (!has_default && !has_colorcodes) return;

    if (has_default)
        saturn_file_browser_item("../default.gs");
    saturn_file_browser_filter_extension("gs");
    saturn_file_browser_filter_extension("txt");
    if (has_colorcodes)
        saturn_file_browser_scan_directory(pack->mPath + "/colorcodes");
    saturn_file_browser_height(100);
    std::string cc_base = pack->mPath + "/colorcodes";
    saturn_file_browser_set_drag_callback([cc_base](std::string path) {
        ColorCode dragging = LoadGSFile(path, cc_base);
        dragging.IsModel = false;
        ImGui::SetDragDropPayload("COLORCODE", &dragging, sizeof(ColorCode));
        ImGui::Text("%s", dragging.Name.c_str());
    });
    std::string browser_id = "modelcc_" + std::to_string(pack->mIndex);
    if (saturn_file_browser_show(browser_id, -1)) {
        std::string selected = saturn_file_browser_get_selected().generic_string();
        if (pack->mIndex == active_saturn_model_index) uiCcListId = -1;
        else uiCcListId = 0;
        current_color_code = LoadGSFile(selected, pack->mPath + "/colorcodes");
        PasteGameShark(current_color_code.GameShark, false);
        UpdateEditorFromPalette();
        UpdatePaletteFromEditor(0);
        send_palette_to_network();
    }
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SAVE_COLORCODE")) {
            ColorCode dragged = *(const ColorCode*)payload->Data;
            SaveActiveColorCode(pack->mPath + "/colorcodes");
            current_expressions.clear();
            add_to_model_queue(pack->mIndex, pack->mEnabled, false);
        }
        ImGui::EndDragDropTarget();
    }
}

void SwitchOption(const char* label, int* state, const char* array[], int size) {
    TimelineButton(std::string(label) + "Key", state, true);
    ImGui::SameLine();
    
    const char* preview_label = (*state >= size) ? "Custom" : array[*state];
    if (ImGui::BeginCombo(label, preview_label)) {
        for (int i = 0; i < size; i++) {
            bool is_selected = (*state == i);
            if (ImGui::Selectable(array[i], is_selected)) {
                *state = i;
                if (array == powerup_switches) vanish_transparency = 128; // Reset transparency when powerup changes
            }
            if (is_selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::TextDisabled("Right click for more options");
        ImGui::EndTooltip();
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Right))
            ImGui::OpenPopup((std::string(label) + "Extended").c_str());
    }
    if (ImGui::BeginPopup((std::string(label) + "Extended").c_str())) {
        ImGui::PushItemWidth(100 * ui_scale);
        if (ImGui::InputInt("###switch_state", state, 1, 10, ImGuiInputTextFlags_None))
            if (array == powerup_switches) vanish_transparency = 128; // Reset transparency when powerup changes
        if (ImGui::Selectable("Reset")) {
            *state = 0;
            if (array == powerup_switches) vanish_transparency = 128; // Reset transparency when powerup changes
        }
        ImGui::PopItemWidth();
        ImGui::EndPopup();
    }
}

void OpenSwitchOptions() {
    ImGui::PushItemWidth(150 * ui_scale);
    if (ImGui::MenuItem("Reset###reset_switches")) {
        switch_state_eyes = 0;
        switch_state_hand_right = 0;
        switch_state_hand_left = 0;
        switch_state_cap = 0;
        switch_state_powerup = 0;
        vanish_transparency = 128;
    }
    ImGui::Separator();

    SwitchOption("Eyes###eye_state", &switch_state_eyes, eye_switches, IM_ARRAYSIZE(eye_switches));
    SwitchOption("Right Hand###right_hand_state", &switch_state_hand_right, right_hand_switches, IM_ARRAYSIZE(right_hand_switches));
    SwitchOption("Left Hand###left_hand_state", &switch_state_hand_left, left_hand_switches, IM_ARRAYSIZE(left_hand_switches));
    SwitchOption("Cap###cap_state", &switch_state_cap, cap_switches, IM_ARRAYSIZE(cap_switches));

    ImGui::Separator();
    ImGui::Checkbox("Head Rotations", &enable_head_rotation);
    ImGui::Checkbox("Torso Rotations", &enable_torso_rotation);

    ImGui::Separator();
    SwitchOption("Powerup###powerup_state", &switch_state_powerup, powerup_switches, IM_ARRAYSIZE(powerup_switches));
    ImGui::BeginDisabled(switch_state_powerup <= 1 || switch_state_powerup == 3);
    ImGui::SliderInt("###transparency", &vanish_transparency, 0, 255, "Alpha %d", ImGuiSliderFlags_AlwaysClamp);
    ImGui::EndDisabled();

    ImGui::PopItemWidth();
}

void ScaleWidget(std::string label, float* scaleX, float* scaleY, float* scaleZ) {
    ImGui::PushItemWidth(150 * ui_scale);
    if (ImGui::SliderFloat(label.c_str(), scaleX, 0.f, 5.f, "Scale %.2f", ImGuiSliderFlags_NoRoundToFormat))
        *scaleY = *scaleX, *scaleZ = *scaleX;
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::TextDisabled("Right click for more options");
        ImGui::EndTooltip();
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Right))
            ImGui::OpenPopup((label + "Presets").c_str());
    }
    if (ImGui::BeginPopup((label + "Presets").c_str())) {
        if (ImGui::MenuItem("Reset")) *scaleX = 1.f, *scaleY = 1.f, *scaleZ = 1.f;
        ImGui::SliderFloat((label + "_x").c_str(), scaleX, 0.f, 5.f, "X %.2f", ImGuiSliderFlags_NoRoundToFormat);
        ImGui::SameLine(); TimelineButton("Scale X", scaleX);
        ImGui::SliderFloat((label + "_y").c_str(), scaleY, 0.f, 5.f, "Y %.2f", ImGuiSliderFlags_NoRoundToFormat);
        ImGui::SameLine(); TimelineButton("Scale Y", scaleY);
        ImGui::SliderFloat((label + "_z").c_str(), scaleZ, 0.f, 5.f, "Z %.2f", ImGuiSliderFlags_NoRoundToFormat);
        ImGui::SameLine(); TimelineButton("Scale Z", scaleZ);
        ImGui::EndPopup();
    }
    ImGui::PopItemWidth();
}

void OpenExtraOptions() {
    if (gMarioStates[0].marioObj != NULL) {
        ImGui::PushItemWidth(150 * ui_scale);
        if (ImGui::MenuItem("Reset###reset_extra")) {
            is_spinning = false;
            spinning_speed = 0.f;
            walkpoint_speed = 127;
            marioScaleX = marioScaleY = marioScaleZ = 1.f;
        }
        ImGui::Separator();

        ScaleWidget("###linkedScale", &marioScaleX, &marioScaleY, &marioScaleZ);

        ImGui::Dummy(ImVec2(15 * ui_scale, 0));
        if (ImGuiKnobs::Knob("Angle", &face_angle, -180.f, 180.f, 0.f, "%.0f deg", ImGuiKnobVariant_Dot, 0.f, ImGuiKnobFlags_DragHorizontal))
            gMarioStates[0].faceAngle[1] = (s16)(face_angle * 182.04f);
        else if (!timelines.count("Angle"))
            face_angle = (float)gMarioStates[0].faceAngle[1] / 182.04;
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::TextDisabled("Right click for more options");
            ImGui::EndTooltip();
            if (ImGui::IsMouseReleased(ImGuiMouseButton_Right))
                ImGui::OpenPopup("###anglePresets");
        }
        if (ImGui::BeginPopup("###anglePresets")) {
            ImGui::Checkbox("Spin", &is_spinning);
            ImGui::BeginDisabled(!is_spinning);
            ImGui::SetNextItemWidth(150);
            ImGui::SliderFloat("Speed###spin_speed", &spinning_speed, -3.f, 3.f, "%.1f");
            ImGui::EndDisabled();
            ImGui::EndPopup();
        }

        ImGui::SameLine(); TimelineButton("Angle", &face_angle);
        ImGui::SameLine();
        ImGuiKnobs::KnobInt("Walkpoint", &walkpoint_speed, 0, 127, 0, "%d", ImGuiKnobVariant_Tick, 0, ImGuiKnobFlags_DragHorizontal);
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::TextDisabled("Right click for more options");
            ImGui::EndTooltip();
            if (ImGui::IsMouseReleased(ImGuiMouseButton_Right))
                ImGui::OpenPopup("###walkpointPresets");
        }
        if (ImGui::BeginPopup("###walkpointPresets")) {
            if (ImGui::Selectable("Running")) walkpoint_speed = 127;
            if (ImGui::Selectable("Walking")) walkpoint_speed = 36;
            if (ImGui::Selectable("Tiptoe")) walkpoint_speed = 25;
            ImGui::EndPopup();
        }

        ImGui::Separator();
        ImGui::Checkbox("Movement Particles", &enable_model_particles);

        ImGui::PopItemWidth();
    }
}

void OpenAccessorySettings() {
    ImGui::BeginDisabled(accessory_packs.size() <= 0);
    if (accessory_packs.size() > 0) {
        if (ImGui::BeginListBox("###accessory_packs_list", ImVec2(150 * ui_scale, 100 * ui_scale))) {
            for (int i = 0; i < accessory_packs.size(); i++) {
                PackData* pack = DynOS_Pack_GetFromIndex(accessory_packs[i]);
                std::string pack_label = pack->mDisplayName.begin();
                std::string pack_id = pack_label + "###accessory_pack_" + std::to_string(i);

                ImGui::BeginDisabled(active_accessory_index != accessory_packs[i] && active_accessory_index != -1);
                if (ImGui::Selectable(pack_id.c_str(), &pack->mEnabled)) {
                    // Toggle model
                    add_to_model_queue(accessory_packs[i], pack->mEnabled, false);
                }
                ImGui::EndDisabled();

            }
            ImGui::EndListBox();
        }
        ImGui::BeginDisabled(active_accessory_index == -1);
        ImGui::Separator();
        ImGui::PushItemWidth(150 * ui_scale);
        ImGui::InputInt3("Position", hat_pos, ImGuiInputTextFlags_CharsDecimal);
        ImGui::InputInt3("Rotation", hat_rot, ImGuiInputTextFlags_CharsDecimal);
        ScaleWidget("###accessoryScale", &hat_scale[0], &hat_scale[1], &hat_scale[2]);
        ImGui::PopItemWidth();
        ImGui::EndDisabled();
    } else ImGui::Text("No accessory packs found");
    ImGui::EndDisabled();
}

void OpenModelSettings() {
    if (AnyModelsEnabled() && active_saturn_model_index != -1) {
        PackData* pack = DynOS_Pack_GetFromIndex(active_saturn_model_index);
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("Model")) {
                if (ImGui::MenuItem("Refresh")) {
                    current_expressions.clear();
                    //add_to_model_queue(active_saturn_model_index, pack->mEnabled, false);
                    forceReload = true;
                }
                ImGui::BeginDisabled(accessory_packs.size() <= 0);
                // To be added
                /*if (ImGui::BeginMenu("Accessories")) {
                    OpenAccessorySettings();
                    ImGui::EndMenu();
                }*/
                ImGui::EndDisabled();
                ImGui::Separator();
                ImGui::Checkbox("Show All Expressions", &ignore_expression_visibility);
                ImGui::Checkbox("Preview Textures", &configExpressionPreviews);
                if (pack->mModelVersion != "") {
                    ImGui::Separator();
                    ImGui::TextDisabled("%s", pack->mModelVersion.begin());
                }
                ImGui::EndMenu();
            }

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
            
            ImGui::EndMenuBar();
        }

        if (pack->mModelName != "" && pack->mModelAuthor != "") {
            ImGui::BeginChild("###model_meta", ImVec2(200, ImGui::GetTextLineHeightWithSpacing()*2), ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
            ImGui::Text("%s", pack->mModelName.begin());
            ImGui::TextDisabled("@%s", pack->mModelAuthor.begin());
            ImGui::EndChild();
        }

        // Model Color Codes
        OpenModelCCSelector(pack);

        // Expressions
        OpenModelExpressionSelector(pack);
    }
}

static ImVec2 s_model_settings_open_pos = ImVec2(0, 0);

void PopupModelSettings() {
    if (!show_window_model_settings) return;
    if (!AnyModelsEnabled() || active_saturn_model_index == -1) {
        show_window_model_settings = false;
        return;
    }
    PackData* pack = DynOS_Pack_GetFromIndex(active_saturn_model_index);
    std::string win_label = (pack->mModelName != "" ? pack->mModelName.begin() : pack->mDisplayName.begin());
    win_label += "###model_settings";
    ImGui::SetNextWindowPos(s_model_settings_open_pos, ImGuiCond_Appearing, ImVec2(0.0f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(0, 0), ImGuiCond_Appearing);
    if (ImGui::Begin(win_label.c_str(), &show_window_model_settings,
                     ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_MenuBar)) {
        OpenModelSettings();
    }
    ImGui::End();
}

void OpenModelSettingsAtCursor() {
    ImVec2 mouse = ImGui::GetIO().MousePos;
    s_model_settings_open_pos = ImVec2(mouse.x + 80.0f, mouse.y);
    show_window_model_settings = true;
}

static char model_search_term[256] = "";

std::vector<std::pair<PackData*, bool>> model_packs;

void OpenModelSelector() {
    /*if (model_packs.size() <= 0 || forceReload) {
        model_packs.clear();
        for (int i = 0; i < DynOS_Pack_GetCount(); i++) {
            PackData* pack = DynOS_Pack_GetFromIndex(i);
            model_packs.push_back(std::make_pair(pack, IsAccessoryModel(i)));
        }
    }*/

    if (DynOS_Pack_GetCount() > model_packs.size()) {
        for (int i = model_packs.size(); i < DynOS_Pack_GetCount(); i++) {
            PackData* pack = DynOS_Pack_GetFromIndex(i);
            model_packs.push_back(std::make_pair(pack, IsAccessoryModel(i)));
        }
    } else if (DynOS_Pack_GetCount() < model_packs.size()) {
        model_packs.clear();
        active_saturn_model_index = -1;
        active_accessory_index = -1;
    }

    if (model_packs.size() <= 0) {
        ImGui::TextDisabled("No model packs found.");
        if (ImGui::Button("Open Folder")) {
            std::string path = std::string(sys_user_path()).append("/dynos/packs");
#if defined(_WIN32) || defined(_WIN64)
            // Windows
            ShellExecuteA(NULL, "open", path.c_str(), NULL, NULL, SW_SHOWNORMAL);
#elif __linux__
            // Linux
            char command[512];
            snprintf(command, sizeof(command), "xdg-open %s", path.c_str());
            system(command);
#elif __APPLE__
            // macOS
            char command[512];
            snprintf(command, sizeof(command), "open %s", path.c_str());
            system(command);
#endif
        }
        return;
    }

    if (DynOS_Pack_GetCount() >= 20) {
        ImGui::SetNextItemWidth(200 * ui_scale);
        ImGui::InputTextWithHint("###model_packs_search", "Search models...", model_search_term, IM_ARRAYSIZE(model_search_term), ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_CharsLowercase);
    } else if (model_search_term != "") strcpy(model_search_term, "");

    if (ImGui::SmallButton("Refresh###model_packs_refresh")) {
        model_packs.clear();
        active_saturn_model_index = -1;
        active_accessory_index = -1;
        gfx_texture_cache_clear();
        forceReload = true;
    }
    ImGui::Separator();

    if (ImGui::BeginListBox("###model_packs_list", ImVec2(200 * ui_scale, 200 * ui_scale))) {
        for (int i = 0; i < model_packs.size(); i++) {
            if (model_packs[i].second) continue; // Skip accessory packs
            PackData* pack = model_packs[i].first;
            std::string pack_label = pack->mDisplayName.begin();
            std::string pack_id = pack_label + "###model_pack_" + std::to_string(i);

            std::string pack_search_meta = pack_label;
            std::transform(pack_search_meta.begin(), pack_search_meta.end(), pack_search_meta.begin(),
                    [](unsigned char c1){ return std::tolower(c1); });
            if (model_search_term != "" && pack_search_meta.find(model_search_term) == std::string::npos) continue;

            ImGui::BeginDisabled(IsSaturnModel(i) && active_saturn_model_index != -1 && active_saturn_model_index != i);
            ImGui::BeginDisabled(!std::filesystem::exists(std::filesystem::path(pack->mPath)));
            if (ImGui::Selectable(pack_id.c_str(), &pack->mEnabled)) {
                // Toggle model
                if (IsSaturnModel(i)) {
                    current_expressions.clear();
                    format_warning_dismissed = false;
                }
                add_to_model_queue(i, pack->mEnabled, false);
                if (active_saturn_model_index == -1) custom_eyes = false;
            }
            if (pack->mModelAuthor != "" && pack->mEnabled) {
                ImGui::SameLine();
                ImGui::TextDisabled("@%s", pack->mModelAuthor.begin());
            }
            ImGui::EndDisabled();
            ImGui::EndDisabled();
            
            std::string popup_id = "###model_pack_popup_" + std::to_string(i);
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
                ImGui::OpenPopup(popup_id.c_str());
            }
            if (ImGui::BeginPopup(popup_id.c_str())) {
                ImGui::Text(pack->mDisplayName.begin());
                OpenModelCCSelector(pack);
                
                OpenModelExpressionSelector(pack);
                ImGui::EndPopup();
            }
        }
        ImGui::EndListBox();
    }

    if (active_saturn_model_index != -1) {
        if (ImGui::Selectable("Reset", false, ImGuiSelectableFlags_DontClosePopups)) {
            for (int i = 0; i < DynOS_Pack_GetCount(); i++) {
                if (IsSaturnModel(i)) LoadModelData(i, false, false, true);
            }
        }
    }
}