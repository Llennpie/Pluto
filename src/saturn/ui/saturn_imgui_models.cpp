#include "saturn_imgui_models.h"
#include <iostream>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>
#include <algorithm>

#include "saturn/saturn.h"
#include "saturn/saturn_colors.h"
#include "saturn/saturn_models.h"
#include "saturn/saturn_textures.h"
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

/* Expression selector UI for the eye expression */
void OpenEyeSelector() {
    if (current_expressions.size() <= 0) return;
    if (current_expressions[0].Textures.size() <= 0) return;
    if (current_expressions[0].Name == "eyes") {
        // The file browser is always visible, but only interactable when custom eyes are enabled
        ImGui::BeginDisabled(!custom_eyes || (!current_expressions[0].Visible && !ignore_expression_visibility));

        saturn_file_browser_filter_extension("png");
        saturn_file_browser_scan_directory(current_expressions[0].FolderPath);
        saturn_file_browser_height(150);
        if (saturn_file_browser_show("eyes", 0)) {
            // Iterate between textures and find the selected one
            for (int tex = 0; tex < current_expressions[0].Textures.size(); tex++) {
                if (current_expressions[0].Textures[tex].FilePath.find(saturn_file_browser_get_selected().generic_string()) != std::string::npos) {
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

/* Expression selector UI for all other expressions, i.e. individual dropdowns or checkboxes */
void OpenComboSelector(Expression* expression, int index) {
    if (expression->Name == "eyes") return;
    if (expression->Textures.size() <= 1) return;
    if (expression->Visible && (expression->Format != G_IM_FMT_RGBA || expression->Size != G_IM_SIZ_32b) && !format_warning_dismissed) return;

    // Fun table UI
    ImGui::TableNextRow();
    ImGui::BeginDisabled(!expression->Visible && !ignore_expression_visibility);

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
    } else {
        // Use dropdown
        std::string defaultLabel = expression->Textures[expression->CurrentIndex].ShortFileName();
        ImGui::PushItemWidth((current_expressions.size() > 8) ? ImGui::GetColumnWidth(1) - 14 : ImGui::GetColumnWidth(1));

        if (ImGui::BeginCombo(label_name.c_str(), defaultLabel.c_str(), ImGuiComboFlags_None)) {
            saturn_file_browser_filter_extension("png");
            saturn_file_browser_scan_directory(expression->FolderPath);
            if (saturn_file_browser_show_tree("expr_" + std::to_string(index), index)) {
                for (int tex = 0; tex < expression->Textures.size(); tex++) {
                    if (expression->Textures[tex].FilePath.find(saturn_file_browser_get_selected().generic_string()) != std::string::npos) {
                        expression->CurrentIndex = tex;
                        break;
                    }
                }
            }
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
        if (ImGui::Checkbox("Custom Eyes", &custom_eyes)) {
            // Swap switch state
            if (!custom_eyes) switch_state_eyes = 0;
            else if (switch_state_eyes <= 3 || switch_state_eyes == 8) switch_state_eyes = 4;
        }
    }
    OpenEyeSelector();

    // Other Expressions
    if (current_expressions.size() > 1) {
        // A scrollbar is visible if the expressions list is too long
        if (GetValidExpressionCount(current_expressions) > 8)
            ImGui::BeginChild(("###menu_exp_model"), ImVec2(200, 190), ImGuiChildFlags_ResizeY, ImGuiWindowFlags_NoBackground);
        
        // Create table UI
        ImGui::BeginTable("###menu_exp_table", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg, ImVec2(200, 0));
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
        ImGui::BeginChild("###menu_model_warning", ImVec2(200, 75), ImGuiChildFlags_Border, ImGuiWindowFlags_None);
        ImGui::BeginDisabled();
        ImGui::TextWrapped("Some materials are not set to texture format RGBA-32 and have been disabled.");
        //ImGui::TextWrapped("Some materials are not set to texture format RGBA-32 - this may cause graphical errors!");
        ImGui::EndDisabled();
        if (ImGui::MenuItem("Show Anyway###dismiss_model_warning"))
            format_warning_dismissed = true;
        ImGui::EndChild();
    }
}

void OpenModelCCSelector(PackData* pack, std::vector<std::string> cc_list) {
    if (!IsSaturnModel(pack->mIndex) || active_saturn_model_index == -1) return;

    ImGui::BeginChild("###menu_model_cc_selector", ImVec2(200, 100), ImGuiChildFlags_Border);
    for (int n = 0; n < cc_list.size(); n++) {
        const bool is_selected = (uiCcListId == ((n + 1) * -1) && pack->mIndex == active_saturn_model_index);
        std::string label_name = cc_list[n].substr(0, cc_list[n].find_last_of('.')) + "###mcc_list_" + std::to_string(n);
        if (ImGui::Selectable(label_name.c_str(), is_selected)) {
            if (pack->mIndex == active_saturn_model_index) uiCcListId = (n + 1) * -1;
            else uiCcListId = 0;

            // Overwrite current color code
            current_color_code = LoadGSFile(cc_list[n], pack->mPath + "/colorcodes");
            PasteGameShark(current_color_code.GameShark, false);
            UpdateEditorFromPalette();
            UpdatePaletteFromEditor(0);
            send_palette_to_network();
        }
        if (ImGui::BeginPopupContextItem()) {
            if (label_name != "../default") {
                ImGui::TextDisabled("#%i", n); ImGui::SameLine();
                ImGui::Text(cc_list[n].c_str());
                ImGui::TextDisabled(((std::string)pack->mDisplayName.begin() + "/colorcodes/" + cc_list[n]).c_str());
            }
            if (ImGui::Button("Copy GS to Clipboard")) {
                ImGui::LogToClipboard();
                ColorCode paste = LoadGSFile(cc_list[n], pack->mPath + "/colorcodes");
                ImGui::LogText(paste.GameShark.c_str());
                ImGui::LogFinish();
            }
            ImGui::Separator();
            ImGui::TextDisabled("%i color code(s)", cc_list.size());
            if (ImGui::Button("Refresh")) {
                UpdateEditorLabels();
                format_warning_dismissed = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
            ColorCode dragging = LoadGSFile(cc_list[n], pack->mPath + "/colorcodes");
            ImGui::SetDragDropPayload("COLORCODE", &dragging, sizeof(ColorCode));
            ImGui::Text("%s (%s)", pack->mDisplayName, dragging.Name.c_str());
            ImGui::EndDragDropSource();
        }
    }
    ImGui::EndChild();
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SAVE_COLORCODE")) {
            ColorCode dragged = *(const ColorCode*)payload->Data;
            SaveActiveColorCode(pack->mPath + "/colorcodes");

            current_expressions.clear();
            UpdateEditorLabels();
            LoadModelData(pack->mIndex, pack->mEnabled, false);
        }
        ImGui::EndDragDropTarget();
    }
}

void OpenSwitchOptions() {
    ImGui::PushItemWidth(150);
    if (ImGui::MenuItem("Reset")) {
        switch_state_eyes = 0;
        switch_state_hand_right = 0;
        switch_state_hand_left = 0;
        switch_state_cap = 0;
        switch_state_powerup = 0;
        vanish_transparency = 128;
        enable_head_rotation = false;
        enable_torso_rotation = true;
    }
    ImGui::Separator();

    ImGui::Combo("Eyes###eye_state", &switch_state_eyes, eye_switches, IM_ARRAYSIZE(eye_switches));
    ImGui::Combo("Right Hand###right_hand_state", &switch_state_hand_right, right_hand_switches, IM_ARRAYSIZE(right_hand_switches));
    ImGui::Combo("Left Hand###left_hand_state", &switch_state_hand_left, left_hand_switches, IM_ARRAYSIZE(left_hand_switches));
    ImGui::Combo("Cap###cap_state", &switch_state_cap, cap_switches, IM_ARRAYSIZE(cap_switches));

    ImGui::Separator();
    ImGui::BeginDisabled(!freeze_camera);
    ImGui::Checkbox("Head Rotations", &enable_head_rotation);
    ImGui::EndDisabled();
    ImGui::Checkbox("Torso Rotations", &enable_torso_rotation);

    ImGui::Separator();
    if (ImGui::Combo("Powerup###powerup_state", &switch_state_powerup, powerup_switches, IM_ARRAYSIZE(powerup_switches)))
        vanish_transparency = 128;
    ImGui::BeginDisabled(switch_state_powerup <= 1 || switch_state_powerup == 3);
    ImGui::SliderInt("###transparency", &vanish_transparency, 0, 255, "Alpha %d", ImGuiSliderFlags_AlwaysClamp);
    ImGui::EndDisabled();

    ImGui::PopItemWidth();
    ImGui::Separator();
    
    if (gMarioStates[0].marioObj != NULL) {
    if (ImGuiKnobs::Knob("Angle", &face_angle, -180.f, 180.f, 0.f, "%.0f deg", ImGuiKnobVariant_Dot, 0.f, ImGuiKnobFlags_DragHorizontal))
        gMarioStates[0].faceAngle[1] = (s16)(face_angle * 182.04f);
    else
        face_angle = (float)gMarioStates[0].faceAngle[1] / 182.04;
    }

    /*ImGui::SetNextItemWidth(150);
    ImGui::SliderInt("###walkpoint", &walkpoint_speed, 0, 127, "Walkpoint %d");*/

    ImGui::SameLine();
    ImGuiKnobs::KnobInt("Walkpoint", &walkpoint_speed, 0, 127, 0, "%d", ImGuiKnobVariant_Tick, 0, ImGuiKnobFlags_DragHorizontal);
    if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
        ImGui::OpenPopup("###walkpointPresets");
    if (ImGui::BeginPopup("###walkpointPresets")) {
        if (ImGui::Selectable("Running")) walkpoint_speed = 127;
        if (ImGui::Selectable("Walking")) walkpoint_speed = 36;
        if (ImGui::Selectable("Tiptoe")) walkpoint_speed = 25;
        ImGui::EndPopup();
    }
}

void OpenModelSettings() {
    if (show_window_model_settings && AnyModelsEnabled() && active_saturn_model_index != -1) {
        PackData* pack = DynOS_Pack_GetFromIndex(active_saturn_model_index);
        std::string popup_label = pack->mDisplayName.begin();
        popup_label += " Settings###model_settings";
        ImGui::Begin(popup_label.c_str(), &show_window_model_settings, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_MenuBar);
        
        // Switch Options
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("Model")) {
                if (ImGui::MenuItem("Refresh")) {
                    current_expressions.clear();
                    UpdateEditorLabels();
                    LoadModelData(active_saturn_model_index, pack->mEnabled, false);
                }
                ImGui::Checkbox("Show All Expressions", &ignore_expression_visibility);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Switches")) {
                OpenSwitchOptions();
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        // Model Color Codes
        if (model_color_code_list.size() > 0)
            OpenModelCCSelector(pack, model_color_code_list);

        // Expressions
        OpenModelExpressionSelector(pack);
        ImGui::End();
    }
}

std::vector<std::string> popup_color_code_list;
static char model_search_term[256] = "";

void OpenModelSelector() {
    if (DynOS_Pack_GetCount() >= 20) {
        ImGui::SetNextItemWidth(200);
        ImGui::InputTextWithHint("###model_packs_search", "Search models...", model_search_term, IM_ARRAYSIZE(model_search_term), ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_CharsLowercase);
        ImGui::Separator();
    } else if (model_search_term != "") strcpy(model_search_term, "");

    if (ImGui::BeginListBox("###model_packs_list", ImVec2(200, 200))) {
        for (int i = 0; i < DynOS_Pack_GetCount(); i++) {
            PackData* pack = DynOS_Pack_GetFromIndex(i);
            std::string pack_label = pack->mDisplayName.begin();
            std::string pack_id = pack_label + "###model_pack_" + std::to_string(i);

            std::string pack_search_meta = pack_label;
            std::transform(pack_search_meta.begin(), pack_search_meta.end(), pack_search_meta.begin(),
                    [](unsigned char c1){ return std::tolower(c1); });
            if (model_search_term != "" && pack_search_meta.find(model_search_term) == std::string::npos) continue;

            ImGui::BeginDisabled(IsSaturnModel(i) && active_saturn_model_index != -1 && active_saturn_model_index != i);
            if (ImGui::Selectable(pack_id.c_str(), &pack->mEnabled)) {
                // Toggle model
                if (IsSaturnModel(i)) {
                    current_expressions.clear();
                    UpdateEditorLabels();
                    format_warning_dismissed = false;
                    spawn_object(gMarioStates[0].marioObj, 0x95, bhvGoldenCoinSparkles);
                }
                LoadModelData(i, pack->mEnabled, false);
                if (active_saturn_model_index == -1) custom_eyes = false;
            }
            ImGui::EndDisabled();
            
            std::string popup_id = "###model_pack_popup_" + std::to_string(i);
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
                ImGui::OpenPopup(popup_id.c_str());
                popup_color_code_list.clear();
                popup_color_code_list = GetColorCodeList(pack->mPath + "/colorcodes");
            }
            if (ImGui::BeginPopup(popup_id.c_str())) {
                ImGui::Text(pack->mDisplayName.begin());
                if (popup_color_code_list.size() > 0)
                    OpenModelCCSelector(pack, popup_color_code_list);
                
                OpenModelExpressionSelector(pack);
                ImGui::EndPopup();
            }
        }
        ImGui::EndListBox();
    }
}