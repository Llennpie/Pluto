#include "saturn_imgui_colors.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>

#include "saturn/saturn.h"
#include "saturn/saturn_colors.h"
#include "saturn/saturn_models.h"
#include "saturn/saturn_textures.h"
#include "saturn/ui/saturn_imgui_file_browser.h"
#include "saturn/libs/imgui/imgui.h"
#include "saturn/libs/imgui/imgui_internal.h"
#include "saturn/libs/imgui/imgui_impl_sdl.h"
#include "saturn/libs/imgui/imgui_impl_opengl3.h"
#include "saturn/ui/studio_notifications.h"
#include "pc/djui/djui.h"
#include "pc/djui/djui_chat_box.h"
#include "pc/djui/djui_console.h"
#include "pc/network/network_player.h"
#include "pc/network/packets/packet.h"
#include "pc/gfx/gfx_pc.h"
#include "pc/pc_main.h"

#include <SDL2/SDL.h>

bool show_cap, show_overalls, show_gloves, show_shoes, show_skin, show_hair;
bool show_shirt, show_shoulders, show_arms, show_pelvis, show_thigh, show_calf;

/* Applies UI values in the CC Editor to the active player.
This should be called when loading a CC, to insert our new colors into the editor. */
void UpdatePaletteFromEditor(int networkIndex) {
    if (networkIndex != 0) return;

    /* Here, each ColorTemplate (e.g. defaultColorHat) is updated from the editor values.
    The network palette is also updated, allowing other players to see a simplified version of the color code.

    For some reason, solid colors like #FF0000 and #0000FF are not sent, so we multiply by 254 instead.
    The change is inconspicuous in-game.*/

    defaultColorHat.red[0] = (int)(uiHatMainColor.x * 255);
    defaultColorHat.green[0] = (int)(uiHatMainColor.y * 255);
    defaultColorHat.blue[0] = (int)(uiHatMainColor.z * 255);
    gNetworkPlayers[networkIndex].overridePalette.parts[1][0] = (int)(uiHatMainColor.x * 254);
    gNetworkPlayers[networkIndex].overridePalette.parts[1][1] = (int)(uiHatMainColor.y * 254);
    gNetworkPlayers[networkIndex].overridePalette.parts[1][2] = (int)(uiHatMainColor.z * 254);
    defaultColorHat.red[1] = (int)(uiHatShadeColor.x * 255);
    defaultColorHat.green[1] = (int)(uiHatShadeColor.y * 255);
    defaultColorHat.blue[1] = (int)(uiHatShadeColor.z * 255);
    // Cap
    gNetworkPlayers[networkIndex].overridePalette.parts[6][0] = (int)(uiHatMainColor.x * 254);
    gNetworkPlayers[networkIndex].overridePalette.parts[6][1] = (int)(uiHatMainColor.y * 254);
    gNetworkPlayers[networkIndex].overridePalette.parts[6][2] = (int)(uiHatMainColor.z * 254);

    defaultColorOveralls.red[0] = (int)(uiOverallsMainColor.x * 255);
    defaultColorOveralls.green[0] = (int)(uiOverallsMainColor.y * 255);
    defaultColorOveralls.blue[0] = (int)(uiOverallsMainColor.z * 255);
    gNetworkPlayers[networkIndex].overridePalette.parts[0][0] = (int)(uiOverallsMainColor.x * 254);
    gNetworkPlayers[networkIndex].overridePalette.parts[0][1] = (int)(uiOverallsMainColor.y * 254);
    gNetworkPlayers[networkIndex].overridePalette.parts[0][2] = (int)(uiOverallsMainColor.z * 254);
    defaultColorOveralls.red[1] = (int)(uiOverallsShadeColor.x * 255);
    defaultColorOveralls.green[1] = (int)(uiOverallsShadeColor.y * 255);
    defaultColorOveralls.blue[1] = (int)(uiOverallsShadeColor.z * 255);

    defaultColorGloves.red[0] = (int)(uiGlovesMainColor.x * 255);
    defaultColorGloves.green[0] = (int)(uiGlovesMainColor.y * 255);
    defaultColorGloves.blue[0] = (int)(uiGlovesMainColor.z * 255);
    gNetworkPlayers[networkIndex].overridePalette.parts[2][0] = (int)(uiGlovesMainColor.x * 254);
    gNetworkPlayers[networkIndex].overridePalette.parts[2][1] = (int)(uiGlovesMainColor.y * 254);
    gNetworkPlayers[networkIndex].overridePalette.parts[2][2] = (int)(uiGlovesMainColor.z * 254);
    defaultColorGloves.red[1] = (int)(uiGlovesShadeColor.x * 255);
    defaultColorGloves.green[1] = (int)(uiGlovesShadeColor.y * 255);
    defaultColorGloves.blue[1] = (int)(uiGlovesShadeColor.z * 255);

    defaultColorShoes.red[0] = (int)(uiShoesMainColor.x * 255);
    defaultColorShoes.green[0] = (int)(uiShoesMainColor.y * 255);
    defaultColorShoes.blue[0] = (int)(uiShoesMainColor.z * 255);
    gNetworkPlayers[networkIndex].overridePalette.parts[3][0] = (int)(uiShoesMainColor.x * 254);
    gNetworkPlayers[networkIndex].overridePalette.parts[3][1] = (int)(uiShoesMainColor.y * 254);
    gNetworkPlayers[networkIndex].overridePalette.parts[3][2] = (int)(uiShoesMainColor.z * 254);
    defaultColorShoes.red[1] = (int)(uiShoesShadeColor.x * 255);
    defaultColorShoes.green[1] = (int)(uiShoesShadeColor.y * 255);
    defaultColorShoes.blue[1] = (int)(uiShoesShadeColor.z * 255);

    defaultColorSkin.red[0] = (int)(uiSkinMainColor.x * 255);
    defaultColorSkin.green[0] = (int)(uiSkinMainColor.y * 255);
    defaultColorSkin.blue[0] = (int)(uiSkinMainColor.z * 255);
    gNetworkPlayers[networkIndex].overridePalette.parts[5][0] = (int)(uiSkinMainColor.x * 254);
    gNetworkPlayers[networkIndex].overridePalette.parts[5][1] = (int)(uiSkinMainColor.y * 254);
    gNetworkPlayers[networkIndex].overridePalette.parts[5][2] = (int)(uiSkinMainColor.z * 254);
    defaultColorSkin.red[1] = (int)(uiSkinShadeColor.x * 255);
    defaultColorSkin.green[1] = (int)(uiSkinShadeColor.y * 255);
    defaultColorSkin.blue[1] = (int)(uiSkinShadeColor.z * 255);

    defaultColorHair.red[0] = (int)(uiHairMainColor.x * 255);
    defaultColorHair.green[0] = (int)(uiHairMainColor.y * 255);
    defaultColorHair.blue[0] = (int)(uiHairMainColor.z * 255);
    gNetworkPlayers[networkIndex].overridePalette.parts[4][0] = (int)(uiHairMainColor.x * 254);
    gNetworkPlayers[networkIndex].overridePalette.parts[4][1] = (int)(uiHairMainColor.y * 254);
    gNetworkPlayers[networkIndex].overridePalette.parts[4][2] = (int)(uiHairMainColor.z * 254);
    defaultColorHair.red[1] = (int)(uiHairShadeColor.x * 255);
    defaultColorHair.green[1] = (int)(uiHairShadeColor.y * 255);
    defaultColorHair.blue[1] = (int)(uiHairShadeColor.z * 255);

    // SPARK
    sparkColorShirt.red[0] = (int)(uiShirtMainColor.x * 255);
    sparkColorShirt.green[0] = (int)(uiShirtMainColor.y * 255);
    sparkColorShirt.blue[0] = (int)(uiShirtMainColor.z * 255);
    sparkColorShirt.red[1] = (int)(uiShirtShadeColor.x * 255);
    sparkColorShirt.green[1] = (int)(uiShirtShadeColor.y * 255);
    sparkColorShirt.blue[1] = (int)(uiShirtShadeColor.z * 255);

    sparkColorShoulders.red[0] = (int)(uiShouldersMainColor.x * 255);
    sparkColorShoulders.green[0] = (int)(uiShouldersMainColor.y * 255);
    sparkColorShoulders.blue[0] = (int)(uiShouldersMainColor.z * 255);
    sparkColorShoulders.red[1] = (int)(uiShouldersShadeColor.x * 255);
    sparkColorShoulders.green[1] = (int)(uiShouldersShadeColor.y * 255);
    sparkColorShoulders.blue[1] = (int)(uiShouldersShadeColor.z * 255);

    sparkColorArms.red[0] = (int)(uiArmsMainColor.x * 255);
    sparkColorArms.green[0] = (int)(uiArmsMainColor.y * 255);
    sparkColorArms.blue[0] = (int)(uiArmsMainColor.z * 255);
    sparkColorArms.red[1] = (int)(uiArmsShadeColor.x * 255);
    sparkColorArms.green[1] = (int)(uiArmsShadeColor.y * 255);
    sparkColorArms.blue[1] = (int)(uiArmsShadeColor.z * 255);

    sparkColorPelvis.red[0] = (int)(uiPelvisMainColor.x * 255);
    sparkColorPelvis.green[0] = (int)(uiPelvisMainColor.y * 255);
    sparkColorPelvis.blue[0] = (int)(uiPelvisMainColor.z * 255);
    sparkColorPelvis.red[1] = (int)(uiPelvisShadeColor.x * 255);
    sparkColorPelvis.green[1] = (int)(uiPelvisShadeColor.y * 255);
    sparkColorPelvis.blue[1] = (int)(uiPelvisShadeColor.z * 255);

    sparkColorThigh.red[0] = (int)(uiThighMainColor.x * 255);
    sparkColorThigh.green[0] = (int)(uiThighMainColor.y * 255);
    sparkColorThigh.blue[0] = (int)(uiThighMainColor.z * 255);
    sparkColorThigh.red[1] = (int)(uiThighShadeColor.x * 255);
    sparkColorThigh.green[1] = (int)(uiThighShadeColor.y * 255);
    sparkColorThigh.blue[1] = (int)(uiThighShadeColor.z * 255);

    sparkColorCalf.red[0] = (int)(uiCalfMainColor.x * 255);
    sparkColorCalf.green[0] = (int)(uiCalfMainColor.y * 255);
    sparkColorCalf.blue[0] = (int)(uiCalfMainColor.z * 255);
    sparkColorCalf.red[1] = (int)(uiCalfShadeColor.x * 255);
    sparkColorCalf.green[1] = (int)(uiCalfShadeColor.y * 255);
    sparkColorCalf.blue[1] = (int)(uiCalfShadeColor.z * 255);

    current_color_code.ParseGameShark();
    strcpy(uiGameShark, current_color_code.GameShark.c_str());
    current_color_code.Name = uiCcLabelName;
}

void ColorPartDrag(int id) {
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("COLORCODE")) {
            ColorCode dragged = *(ColorCode*)payload->Data;
            if (dragged.GameShark.size() > 0) {
                // Find address position in GameShark string
                // This should account for all formats; Never assume the string has a consistent order
                std::string::size_type pos1 = 0;
                std::string::size_type pos2 = 30;
                switch(id) {
                    case 0:     /* Shirt */     pos1 = dragged.GameShark.find("07EC40")-2; pos2 = dragged.GameShark.find("07EC42")-2; break;
                    case 1:                     pos1 = dragged.GameShark.find("07EC38")-2; pos2 = dragged.GameShark.find("07EC3A")-2; break;
                    case 2:     /* Overalls */  pos1 = dragged.GameShark.find("07EC28")-2; pos2 = dragged.GameShark.find("07EC2A")-2; break;
                    case 3:                     pos1 = dragged.GameShark.find("07EC20")-2; pos2 = dragged.GameShark.find("07EC22")-2; break;
                    case 4:     /* Gloves */    pos1 = dragged.GameShark.find("07EC58")-2; pos2 = dragged.GameShark.find("07EC5A")-2; break;
                    case 5:                     pos1 = dragged.GameShark.find("07EC50")-2; pos2 = dragged.GameShark.find("07EC52")-2; break;
                    case 6:     /* Shoes */     pos1 = dragged.GameShark.find("07EC70")-2; pos2 = dragged.GameShark.find("07EC72")-2; break;
                    case 7:                     pos1 = dragged.GameShark.find("07EC68")-2; pos2 = dragged.GameShark.find("07EC6A")-2; break;
                    case 8:     /* Skin */      pos1 = dragged.GameShark.find("07EC88")-2; pos2 = dragged.GameShark.find("07EC8A")-2; break;
                    case 9:                     pos1 = dragged.GameShark.find("07EC80")-2; pos2 = dragged.GameShark.find("07EC82")-2; break;
                    case 10:    /* Hair*/       pos1 = dragged.GameShark.find("07ECA0")-2; pos2 = dragged.GameShark.find("07ECA2")-2; break;
                    case 11:                    pos1 = dragged.GameShark.find("07EC98")-2; pos2 = dragged.GameShark.find("07EC9A")-2; break;
                    case 12:    /* Shirt */     pos1 = dragged.GameShark.find("07ECB8")-2; pos2 = dragged.GameShark.find("07ECBA")-2; break;
                    case 13:                    pos1 = dragged.GameShark.find("07ECB0")-2; pos2 = dragged.GameShark.find("07ECB2")-2; break;
                    case 14:    /* Shoulders */ pos1 = dragged.GameShark.find("07ECD0")-2; pos2 = dragged.GameShark.find("07ECD2")-2; break;
                    case 15:                    pos1 = dragged.GameShark.find("07ECC8")-2; pos2 = dragged.GameShark.find("07ECCA")-2; break;
                    case 16:    /* Arms */      pos1 = dragged.GameShark.find("07ECE8")-2; pos2 = dragged.GameShark.find("07ECEA")-2; break;
                    case 17:                    pos1 = dragged.GameShark.find("07ECE0")-2; pos2 = dragged.GameShark.find("07ECE2")-2; break;
                    case 18:    /* Pelvis */    pos1 = dragged.GameShark.find("07ED00")-2; pos2 = dragged.GameShark.find("07ED02")-2; break;
                    case 19:                    pos1 = dragged.GameShark.find("07ECF8")-2; pos2 = dragged.GameShark.find("07ECFA")-2; break;
                    case 20:    /* Thighs*/     pos1 = dragged.GameShark.find("07ED18")-2; pos2 = dragged.GameShark.find("07ED1A")-2; break;
                    case 21:                    pos1 = dragged.GameShark.find("07ED10")-2; pos2 = dragged.GameShark.find("07ED12")-2; break;
                    case 22:    /* Calves */    pos1 = dragged.GameShark.find("07ED30")-2; pos2 = dragged.GameShark.find("07ED32")-2; break;
                    case 23:                    pos1 = dragged.GameShark.find("07ED28")-2; pos2 = dragged.GameShark.find("07ED2A")-2; break;
                }

                ImGui::LogToClipboard();
                ImGui::LogText("%s", dragged.GameShark.c_str());
                ImGui::LogFinish();

                // Verify we didn't load a null value
                if (pos1 != std::string::npos-2 && pos2 != std::string::npos-2) {
                    // Paste only a small segment
                    PasteGameShark(dragged.GameShark.substr(pos1, 13) + "\n" + dragged.GameShark.substr(pos2, 13), false);
                    UpdateEditorFromPalette();
                } else {
                    PasteGameShark("8107EC40 60C6\n8107EC42 2000\n8107EC38 3063\n8107EC3A 1000\n8107EC28 FFFF\n8107EC2A FF00\n8107EC20 7F7F\n8107EC22 7F00\n8107EC58 61AA\n8107EC5A EC00\n8107EC50 3055\n8107EC52 7600\n8107EC70 8A7A\n8107EC72 2B00\n8107EC68 453D\n8107EC6A 1500\n8107EC88 D961\n8107EC8A 6700\n8107EC80 6C30\n8107EC82 3300\n8107ECA0 8C82\n8107ECA2 BC00\n8107EC98 4641\n8107EC9A 5E00\n8107ECB8 0000\n8107ECBA 0000\n8107ECB0 FFFF\n8107ECB2 0000\n8107ECD0 0000\n8107ECD2 0000\n8107ECC8 0000\n8107ECCA 0000\n8107ECE8 0000\n8107ECEA 0000\n8107ECE0 FFFF\n8107ECE2 FF00\n8107ED00 0000\n8107ED02 0000\n8107ECF8 FFFF\n8107ECFA FF00\n8107ED18 0000\n8107ED1A 0000\n8107ED10 FFFF\n8107ED12 FF00\n8107ED30 0000\n8107ED32 0000\n8107ED28 FFFF\n8107ED2A FF00", false);
                    UpdateEditorFromPalette();
                }
            }
        }
        ImGui::EndDragDropTarget();
    }
}

ImGuiColorEditFlags color_part_box_flags =  ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_InputRGB | ImGuiColorEditFlags_Uint8 |
                                            ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_NoInputs;

bool ColorPartBox(int id, const char* name, const char* mainId, float* mainColorValue, const char* shadeId, float* shadeColorValue) {
    bool value_changed;

    // Main Color
    if (ImGui::ColorEdit4(mainId, mainColorValue, color_part_box_flags))
        if (ImGui::IsItemHovered()) value_changed = true;
    ColorPartDrag(id);
    ImGui::SameLine();

    // Shade Color
    if (ImGui::ColorEdit4(shadeId, shadeColorValue, color_part_box_flags))
        if (ImGui::IsItemHovered()) value_changed = true;
    ColorPartDrag(id+1);

    if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
        ImVec4* mainColor = (ImVec4*)mainColorValue;
        ImVec4* shadeColor = (ImVec4*)shadeColorValue;
        shadeColor->x = floor(mainColor->x / 2.0f * 255.0f) / 255.0f;
        shadeColor->y = floor(mainColor->y / 2.0f * 255.0f) / 255.0f;
        shadeColor->z = floor(mainColor->z / 2.0f * 255.0f) / 255.0f;
        shadeColorValue = (float*)shadeColor;
        value_changed = true;
    }

    ImGui::SameLine();
    ImGui::Text(name);
    return value_changed;
}

/* Copies the vanilla palette to the SPARK palette, in the way that was intended for ExMo models */
void SparkilizeEditor() {
    uiShirtMainColor = uiHatMainColor;
    uiShirtShadeColor = uiHatShadeColor;
    uiShouldersMainColor = uiHatMainColor;
    uiShouldersShadeColor = uiHatShadeColor;
    uiArmsMainColor = uiHatMainColor;
    uiArmsShadeColor = uiHatShadeColor;
    uiPelvisMainColor = uiOverallsMainColor;
    uiPelvisShadeColor = uiOverallsShadeColor;
    uiThighMainColor = uiOverallsMainColor;
    uiThighShadeColor = uiOverallsShadeColor;
    uiCalfMainColor = uiOverallsMainColor;
    uiCalfShadeColor = uiOverallsShadeColor;
    UpdatePaletteFromEditor(0);
}

void RefreshColorCodeList() {
    color_code_list.clear();
    color_code_list = GetColorCodeList("dynos/colorcodes");
    UpdatePaletteFromEditor(0);
    send_palette_to_network();
}

void SaveActiveColorCode(std::string save_path) {
    std::ofstream ccfile;
    if (!std::filesystem::is_directory(save_path))
        std::filesystem::create_directory(save_path);
    ccfile.open(save_path + "/" + std::string(uiCcLabelName) + ".gs");
    ccfile << uiGameShark;
    ccfile.close();

    std::string short_path = save_path;
    std::replace(short_path.begin(), short_path.end(), '\\', '/');
    size_t pos = short_path.find("dynos/packs/");
    short_path.erase(0, pos);

    if (std::filesystem::exists(save_path + "/" + std::string(uiCcLabelName) + ".gs"))
        studio_notif_success(uiCcLabelName, "Saved color code to:\n%s/%s.gs", short_path.c_str(), uiCcLabelName);
    else
        studio_notif_error(uiCcLabelName, "Failed to save color code to:\n%s/%s.gs", short_path.c_str(), uiCcLabelName);

    RefreshColorCodeList();
}

void OpenCCSelector() {
    ImGui::BeginChild("###menu_cc_selector", ImVec2(150, 100), ImGuiChildFlags_Border);
    for (int n = 0; n < color_code_list.size(); n++) {
        const bool is_selected = (uiCcListId == n + 1);
        std::string label_name = color_code_list[n].substr(0, color_code_list[n].find_last_of('.')) + "###cc_list_" + std::to_string(n);;
        if (ImGui::Selectable(label_name.c_str(), is_selected)) {
            uiCcListId = n + 1;

            // Overwrite current color code
            current_color_code = LoadGSFile(color_code_list[n], "dynos/colorcodes");
            PasteGameShark(current_color_code.GameShark, false);
            UpdateEditorFromPalette();
            UpdatePaletteFromEditor(0);
            send_palette_to_network();
        }
        if (ImGui::BeginPopupContextItem()) {
            if (label_name != "Mario") {
                ImGui::TextDisabled("#%i", n); ImGui::SameLine();
                ImGui::Text(color_code_list[n].c_str());
                ImGui::TextDisabled(("dynos/colorcodes/" + color_code_list[n]).c_str());
            }
            if (ImGui::Button("Copy GS to Clipboard")) {
                ImGui::LogToClipboard();
                ColorCode paste = LoadGSFile(color_code_list[n], "dynos/colorcodes");
                ImGui::LogText(paste.GameShark.c_str());
                ImGui::LogFinish();
            }
            ImGui::Separator();
            ImGui::TextDisabled("%i color code(s)", color_code_list.size());
            if (ImGui::Button("Refresh")) {
                RefreshColorCodeList();
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
            ColorCode dragging = LoadGSFile(color_code_list[n], "dynos/colorcodes");
            dragging.IsModel = false;
            ImGui::SetDragDropPayload("COLORCODE", &dragging, sizeof(ColorCode));
            ImGui::Text(dragging.Name.c_str());
            ImGui::EndDragDropSource();
        }
    }
    ImGui::EndChild();
}

void OpenCCEditor() {
    bool value_changed;
    OpenCCSelector();

    if (ImGui::BeginTabBar("###cc_editor_tabs", ImGuiTableFlags_None)) {
        if (ImGui::BeginTabItem("Editor")) {
            if (!show_cap && !show_overalls && !show_gloves && !show_shoes && !show_skin && !show_hair && !spark_enabled) {
                // ImGui please make a TextWrappedDisabled() ...... <3
                ImGui::BeginDisabled();
                ImGui::TextWrapped("Active model does not contain any editable color values");
                ImGui::EndDisabled();
            }
            // Visual Editor
            if (show_cap) {       if (ColorPartBox(0,  "Cap", "Cap (Main)", (float*)&uiHatMainColor, "Cap (Shade)", (float*)&uiHatShadeColor)) value_changed = true; }
            if (show_overalls) {  if (ColorPartBox(2,  "Overalls", "Overalls (Main)", (float*)&uiOverallsMainColor, "Overalls (Shade)", (float*)&uiOverallsShadeColor)) value_changed = true; }
            if (show_gloves) {    if (ColorPartBox(4,  "Gloves", "Gloves (Main)", (float*)&uiGlovesMainColor, "Gloves (Shade)", (float*)&uiGlovesShadeColor)) value_changed = true; }
            if (show_shoes) {     if (ColorPartBox(6,  "Shoes", "Shoes (Main)", (float*)&uiShoesMainColor, "Shoes (Shade)", (float*)&uiShoesShadeColor)) value_changed = true; }
            if (show_skin) {      if (ColorPartBox(8,  "Skin", "Skin (Main)", (float*)&uiSkinMainColor, "Skin (Shade)", (float*)&uiSkinShadeColor)) value_changed = true; }
            if (show_hair) {      if (ColorPartBox(10, "Hair", "Hair (Main)", (float*)&uiHairMainColor, "Hair (Shade)", (float*)&uiHairShadeColor)) value_changed = true; }
            if (AnyModelsEnabled() && active_saturn_model_index != -1) {
                if (spark_enabled)
                    if (ImGui::SmallButton("v SPARKILIZE v###cc_editor_sparkilize"))
                        SparkilizeEditor();
                if (show_shirt) {     if (ColorPartBox(12, "Extra 1", "Shirt (Main)", (float*)&uiShirtMainColor, "Shirt (Shade)", (float*)&uiShirtShadeColor)) value_changed = true; }
                if (show_shoulders) { if (ColorPartBox(14, "Extra 2", "Shoulders (Main)", (float*)&uiShouldersMainColor, "Shoulders (Shade)", (float*)&uiShouldersShadeColor)) value_changed = true; }
                if (show_arms) {      if (ColorPartBox(16, "Extra 3", "Arms (Main)", (float*)&uiArmsMainColor, "Arms (Shade)", (float*)&uiArmsShadeColor)) value_changed = true; }
                if (show_pelvis) {    if (ColorPartBox(18, "Extra 4", "Pelvis (Main)", (float*)&uiPelvisMainColor, "Pelvis (Shade)", (float*)&uiPelvisShadeColor)) value_changed = true; }
                if (show_thigh) {     if (ColorPartBox(20, "Extra 5", "Thigh (Main)", (float*)&uiThighMainColor, "Thigh (Shade)", (float*)&uiThighShadeColor)) value_changed = true; }
                if (show_calf) {      if (ColorPartBox(22, "Extra 6", "Calf (Main)", (float*)&uiCalfMainColor, "Calf (Shade)", (float*)&uiCalfShadeColor)) value_changed = true; }
            }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("GameShark")) {
            // GameShark
            if (ImGui::InputTextMultiline("###gameshark_box", uiGameShark, IM_ARRAYSIZE(uiGameShark), ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 24.5f),
                ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_CtrlEnterForNewLine)) {
                    PasteGameShark(std::string(uiGameShark), true);
                    current_color_code.ParseGameShark();
                    UpdateEditorFromPalette();
            }
            ImGui::EndTabItem();
            ImGui::PushItemWidth(100);
            ImGui::InputText(".gs", uiCcLabelName, IM_ARRAYSIZE(uiCcLabelName));
            ImGui::PopItemWidth();
            if (ImGui::Button("Add to List")) {
                UpdatePaletteFromEditor(0);
                if (std::filesystem::exists("dynos/colorcodes/" + current_color_code.Name + ".gs"))
                    ImGui::OpenPopup("###overwrite_gs");
                else SaveActiveColorCode("dynos/colorcodes");
            }
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
                ColorCode dragging;
                dragging.Name = uiCcLabelName;
                dragging.GameShark = uiGameShark;
                ImGui::SetDragDropPayload("SAVE_COLORCODE", &dragging, sizeof(ColorCode));
                ImGui::Text("/%s.gs", dragging.Name.c_str());
                ImGui::EndDragDropSource();
            }

            // "Are you sure?"
            if (ImGui::BeginPopup("###overwrite_gs")) {
                ImGui::Text("Overwrite %s.gs? This action is irreversible!", uiCcLabelName);
                if (ImGui::Button("Yes")) {
                    SaveActiveColorCode("dynos/colorcodes");
                    ImGui::CloseCurrentPopup();
                } ImGui::SameLine();
                if (ImGui::Button("No")) ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
            }
        }
        ImGui::EndTabBar();
    }

    if (value_changed) {
        UpdatePaletteFromEditor(0);
        send_palette_to_network();
        value_changed = false;
    }
}

void UpdateEditorLabels() {
    show_cap = false;
    show_overalls = false;
    show_gloves = false;
    show_shoes = false;
    show_skin = false;
    show_hair = false;
    show_shirt = false;
    show_shoulders = false;
    show_arms = false;
    show_pelvis = false;
    show_thigh = false;
    show_calf = false;
    spark_enabled = false;
}