#include "saturn_imgui_world.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "saturn/saturn.h"
#include "saturn/saturn_colors.h"
#include "saturn/saturn_models.h"
#include "saturn/saturn_textures.h"
#include "saturn/libs/imgui/imgui.h"
#include "saturn/libs/imgui/imgui_internal.h"
#include "saturn/libs/imgui/imgui_impl_sdl.h"
#include "saturn/libs/imgui/imgui_impl_opengl3.h"
#include "saturn/libs/imgui/imgui_impl_opengl3_loader.h"

#include <SDL2/SDL.h>

ImVec4 uiChromaColor = ImVec4(0.0f / 255.0f, 255.0f / 255.0f, 0.0f / 255.0f, 255.0f / 255.0f);

void saturn_set_chroma_color(ImVec4 color) {
    uiChromaColor = color;
    chromaColor.red[0] = (int)(uiChromaColor.x * 255);
    chromaColor.green[0] = (int)(uiChromaColor.y * 255);
    chromaColor.blue[0] = (int)(uiChromaColor.z * 255);
    // Unused
    chromaColor.red[1] = chromaColor.red[0];
    chromaColor.green[1] = chromaColor.green[0];
    chromaColor.blue[1] = chromaColor.blue[0];
}

void OpenAutoChromaMenu() {
    ImGui::Checkbox("Chroma Key", &auto_chroma);
    ImGui::BeginChild("auto_chroma", ImVec2(175, 150), ImGuiChildFlags_Border);
    ImGui::BeginDisabled(!auto_chroma);
    // Skybox Color
    if (ImGui::ColorEdit4("Skybox Color", (float*)&uiChromaColor,   ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_InputRGB |
                                                                    ImGuiColorEditFlags_Uint8 | ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_NoInputs))
        saturn_set_chroma_color(uiChromaColor);

    if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
        ImGui::OpenPopup("###chromaColorPresets");
    if (ImGui::BeginPopup("###chromaColorPresets")) {
        if (ImGui::Selectable("Green"))     saturn_set_chroma_color(ImVec4(0.0f / 255.0f, 255.0f / 255.0f, 0.0f / 255.0f, 255.0f / 255.0f));
        if (ImGui::Selectable("Blue"))      saturn_set_chroma_color(ImVec4(0.0f / 255.0f, 0.0f / 255.0f, 255.0f / 255.0f, 255.0f / 255.0f));
        if (ImGui::Selectable("Pink"))      saturn_set_chroma_color(ImVec4(255.0f / 255.0f, 0.0f / 255.0f, 255.0f / 255.0f, 255.0f / 255.0f));
        if (ImGui::Selectable("Black"))     saturn_set_chroma_color(ImVec4(0.0f / 255.0f, 0.0f / 255.0f, 0.0f / 255.0f, 255.0f / 255.0f));
        if (ImGui::Selectable("White"))     saturn_set_chroma_color(ImVec4(255.0f / 255.0f, 255.0f / 255.0f, 255.0f / 255.0f, 255.0f / 255.0f));
        ImGui::EndPopup();
    }
    ImGui::Separator();
    // Other Auto-chroma Settings
    ImGui::Checkbox("Show Objects", &chroma_show_objects);
    ImGui::Checkbox("Show Level Geometry", &chroma_show_geo);

    ImGui::EndDisabled();
    ImGui::EndChild();
}