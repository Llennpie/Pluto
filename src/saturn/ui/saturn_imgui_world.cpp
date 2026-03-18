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
#include "pc/gfx/gfx_pc.h"
#include "libc/math.h"

#include <SDL2/SDL.h>

ImVec4 uiChromaColor = ImVec4(0.0f / 255.0f, 255.0f / 255.0f, 0.0f / 255.0f, 255.0f / 255.0f);
ImVec4 uiLightingColor = ImVec4(255.0f / 255.0f, 255.0f / 255.0f, 255.0f / 255.0f, 255.0f / 255.0f);
ImVec4 uiVertexColor = ImVec4(255.0f / 255.0f, 255.0f / 255.0f, 255.0f / 255.0f, 255.0f / 255.0f);
ImVec4 uiFogColor = ImVec4(255.0f / 255.0f, 255.0f / 255.0f, 255.0f / 255.0f, 255.0f / 255.0f);

bool Color4Widget(const char* label, ImVec4 *uiColor, u8 intColor[3]) {
    if (ImGui::ColorEdit4(label, (float*)uiColor, ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_InputRGB |
                                                ImGuiColorEditFlags_Uint8 | ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_NoInputs)) {
        intColor[0] = (u8)(uiColor->x * 255);
        intColor[1] = (u8)(uiColor->y * 255);
        intColor[2] = (u8)(uiColor->z * 255);
        return true;
    }
    return false;
}

void saturn_set_chroma_color(ImVec4 color) {
    uiChromaColor = color;
    chromaColor.red[0] = (int)(uiChromaColor.x * 255);
    chromaColor.green[0] = (int)(uiChromaColor.y * 255);
    chromaColor.blue[0] = (int)(uiChromaColor.z * 255);
    // Unused
    chromaColor.red[1] = chromaColor.red[0];
    chromaColor.green[1] = chromaColor.green[0];
    chromaColor.blue[1] = chromaColor.blue[0];

    // Some custom maps prefer this instead
    // 16 bit RGBA color used in background nodes
    // https://n64squid.com/homebrew/n64-sdk/textures/image-formats/
    
    int r5 = ((int)(uiChromaColor.x * 255) * 31 / 255);
    int g5 = ((int)(uiChromaColor.y * 255) * 31 / 255);
    int b5 = ((int)(uiChromaColor.z * 255) * 31 / 255);
    int rShift = (int) r5 << 11;
    int bShift = (int) g5 << 6;
    int gShift = (int) b5 << 1;
    gChromaKeyColor = (int) (bShift | gShift | rShift | 1);
}

void OpenAutoChromaMenu() {
    ImGui::Checkbox("Chroma Key", &auto_chroma);
    ImGui::BeginChild("auto_chroma", ImVec2(175, 0), ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeY);
    ImGui::BeginDisabled(!auto_chroma);
    // Skybox Color
    ImGui::BeginDisabled(chroma_transparent_background);
    if (ImGui::ColorEdit4("Skybox Color", (float*)&uiChromaColor,   ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_InputRGB |
                                                                    ImGuiColorEditFlags_Uint8 | ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_NoInputs))
        saturn_set_chroma_color(uiChromaColor);
    ImGui::EndDisabled();

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
        ImGui::OpenPopup("###chromaColorPresets");
    if (ImGui::BeginPopup("###chromaColorPresets")) {
        ImGui::Checkbox("Transparent", &chroma_transparent_background);

        ImGui::BeginDisabled(chroma_transparent_background);
        if (ImGui::Selectable("Green"))     saturn_set_chroma_color(ImVec4(0.0f / 255.0f, 255.0f / 255.0f, 0.0f / 255.0f, 255.0f / 255.0f));
        if (ImGui::Selectable("Blue"))      saturn_set_chroma_color(ImVec4(0.0f / 255.0f, 0.0f / 255.0f, 255.0f / 255.0f, 255.0f / 255.0f));
        if (ImGui::Selectable("Pink"))      saturn_set_chroma_color(ImVec4(255.0f / 255.0f, 0.0f / 255.0f, 255.0f / 255.0f, 255.0f / 255.0f));
        if (ImGui::Selectable("Black"))     saturn_set_chroma_color(ImVec4(0.0f / 255.0f, 0.0f / 255.0f, 0.0f / 255.0f, 255.0f / 255.0f));
        if (ImGui::Selectable("White"))     saturn_set_chroma_color(ImVec4(255.0f / 255.0f, 255.0f / 255.0f, 255.0f / 255.0f, 255.0f / 255.0f));
        ImGui::EndDisabled();
        ImGui::EndPopup();
    }
    ImGui::Separator();
    // Other Auto-chroma Settings
    ImGui::Checkbox("Show Objects", &chroma_show_objects);
    ImGui::Checkbox("Show Level Geometry", &chroma_show_geo);
    ImGui::Checkbox("Affected by Light", &chroma_affects_light);
    ImGui::Checkbox("Opaque Floor", &chroma_show_floor);

    ImGui::EndDisabled();
    ImGui::EndChild();
}

ImGuiCol frame_color;

void JoystickSlider(float& _x, float& _y, float scale = 100.f, float b_scale = 15.f, int bg_color = IM_COL32(0, 0, 0, 155), int button_color = IM_COL32(215, 215, 215, 255), const int& mouse_button = 0u) {
    const auto& p = ImGui::GetCursorScreenPos();
    static bool button_clicked = false;
    const auto& mouse = ImGui::GetIO().MousePos;
    ImGui::GetWindowDrawList()->AddCircleFilled(ImVec2(p.x + scale, p.y + scale), scale, ImGui::GetColorU32(frame_color), 50);

    ImVec2 circle_center = ImVec2(p.x + scale, p.y + scale);
    float distance = sqrt(pow(mouse.x - circle_center.x, 2) + pow(mouse.y - circle_center.y, 2));
    if (distance <= scale) {
        frame_color = ImGuiCol_FrameBgHovered;
        if (ImGui::GetIO().MouseClicked[mouse_button]) {
            button_clicked = true;

            // Move the button to the clicked position within the circle
            if (distance <= scale - b_scale) {
                _x = (mouse.x - p.x - scale) / (scale - b_scale);
                _y = (mouse.y - p.y - scale) / (scale - b_scale);
            } else {
                float angle = atan2(mouse.y - circle_center.y, mouse.x - circle_center.x);
                _x = cos(angle);
                _y = sin(angle);
            }
        }
    } else {
        frame_color = ImGuiCol_FrameBg;
    }

    auto button_x = _x * (scale - b_scale) + p.x + scale;
    auto button_y = _y * (scale - b_scale) + p.y + scale;
    static float toward = 0.f;
    ImGui::ButtonBehavior(ImRect({ button_x - b_scale, button_y - b_scale}, { button_x + b_scale, button_y + b_scale }), ImGui::GetCurrentWindow()->ID, nullptr, nullptr, 0);
    
    float distance_to_center = sqrtf(pow(mouse.x - (p.x + scale), 2) + pow(mouse.y - (p.y + scale), 2));
    if (sqrtf(pow(mouse.x - button_x, 2) + pow(mouse.y - button_y, 2)) < b_scale)
        if (ImGui::GetIO().MouseClicked[mouse_button])
            button_clicked = true;
    if (!ImGui::GetIO().MouseDown[mouse_button])
        button_clicked = false;
    if (button_clicked) {
        button_x = mouse.x;
        button_y = mouse.y;
    };
    toward = atan2(button_y - p.y - scale, button_x - p.x - scale);
    if (sqrtf(pow(p.x - button_x + scale, 2) + pow(p.y - button_y + scale, 2)) > scale - b_scale) {
        button_x = p.x + scale + cos(toward - 0.5f * M_PI) * (scale - b_scale);
        button_y = p.y + scale - sin(toward - 0.5f * M_PI) * (scale - b_scale);
    };
    
    _x = (button_x - p.x - scale) / (scale - b_scale);
    _y = (button_y - p.y - scale) / (scale - b_scale);

    ImGui::GetWindowDrawList()->AddCircleFilled(ImVec2(button_x, button_y), b_scale, ImGui::GetColorU32(ImGuiCol_ButtonActive), 25);
    ImGui::Dummy(ImVec2(scale*2, scale*2));
};

void OpenQuickOptions() {
    ImGui::Checkbox("Custom Lighting", &shade_lighting_enabled);
    if (shade_lighting_enabled) {
        ImGui::BeginChild("##lighting", ImVec2(175, 140), ImGuiChildFlags_Border);
        JoystickSlider(shade_lighting_dir[0], shade_lighting_dir[1], 35.f, 7.f);
        ImGui::SameLine();
        ImGui::VSliderFloat("##lighting_z", ImVec2(20, 35*2), &shade_lighting_dir[2], -1.f, 1.f, "");
        ImGui::SameLine();
        ImGui::BeginChild("##lighting_dir", ImVec2(69, 35*2+5), ImGuiChildFlags_None, ImGuiWindowFlags_None);
        if (ImGui::MenuItem("Reset")) {
            shade_lighting_dir[0] = 0.f;
            shade_lighting_dir[1] = 0.f;
            shade_lighting_dir[2] = 0.f;
        }
        ImGui::EndChild();
        if (Color4Widget("Color", &uiLightingColor, shade_lighting_color)) {
            uiVertexColor = uiLightingColor;
            uiFogColor = uiLightingColor;
            shade_lighting_fog[0] = shade_lighting_vertex[0] = shade_lighting_color[0];
            shade_lighting_fog[1] = shade_lighting_vertex[1] = shade_lighting_color[1];
            shade_lighting_fog[2] = shade_lighting_vertex[2] = shade_lighting_color[2];
        }
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::TextDisabled("Right click for more options");
            ImGui::EndTooltip();
            if (ImGui::IsMouseReleased(ImGuiMouseButton_Right))
                ImGui::OpenPopup("###lightingColorPresets");
        }
        if (ImGui::BeginPopup("###lightingColorPresets")) {
            if (ImGui::MenuItem("Reset")) {
                uiFogColor = uiVertexColor = uiLightingColor = ImVec4(255.0f / 255.0f, 255.0f / 255.0f, 255.0f / 255.0f, 255.0f / 255.0f);
                shade_lighting_color[0] = shade_lighting_color[1] = shade_lighting_color[2] = 255;
                shade_lighting_vertex[0] = shade_lighting_vertex[1] = shade_lighting_vertex[2] = 255;
                shade_lighting_fog[0] = shade_lighting_fog[1] = shade_lighting_fog[2] = 255;
            }
            Color4Widget("Lighting Color", &uiLightingColor, shade_lighting_color);
            Color4Widget("Background Color", &uiVertexColor, shade_lighting_vertex);
            Color4Widget("Fog Color", &uiFogColor, shade_lighting_fog);
            ImGui::EndPopup();
        }
        ImGui::SetNextItemWidth(155);
        ImGui::DragFloat3("##lighting_pos", shade_lighting_dir, 0.01f, -1.f, 1.f, "%.2f");
        ImGui::EndChild();
    }

    // Quick Options
    //ImGui::Checkbox("HUD", &enable_hud);
    ImGui::Checkbox("Shadows", &configPlutoShadows);

    if (wiggle_bone_detected) {
        ImGui::Separator();
        ImGui::Checkbox("Wind###wind_enabled", &wind_enabled);
        ImGui::BeginDisabled(!wind_enabled);
        ImGui::PushItemWidth(175);
        ImGui::SliderFloat("###wind_angle",    &wind_angle,    0.f, 360.f, "Angle %.0f deg",  ImGuiSliderFlags_NoRoundToFormat);
        ImGui::SliderFloat("###wind_strength", &wind_strength, 0.f, 5.f,   "Strength %.2f",   ImGuiSliderFlags_NoRoundToFormat);
        ImGui::SliderFloat("###wind_sway",     &wind_sway,     0.f, 3.f,   "Sway %.2f",       ImGuiSliderFlags_NoRoundToFormat);
        ImGui::PopItemWidth();
        ImGui::EndDisabled();
    }
}