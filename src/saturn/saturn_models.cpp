#include "saturn_models.h"

#include <string>
#include <iostream>
#include <vector>
#include <SDL2/SDL.h>

#include <dirent.h>
#include <filesystem>
#include <fstream>

#include "saturn/libs/imgui/imgui.h"
#include "saturn/libs/imgui/imgui_internal.h"
#include "saturn/libs/imgui/imgui_impl_sdl.h"
#include "saturn/libs/imgui/imgui_impl_opengl3.h"

#include "saturn/saturn.h"
#include "saturn/saturn_textures.h"
#include "saturn/saturn_colors.h"
#include "saturn/saturn_animations.h"

#include "pc/pc_main.h"
#include "pc/djui/djui_console.h"
#include "data/dynos.cpp.h"

float marioScaleX = 1.0f;
float marioScaleY = 1.0f;
float marioScaleZ = 1.0f;

char uiCcLabelName[128] = "Default";
char uiGameShark[1024 * 16] =    "8107EC40 FF00\n8107EC42 0000\n8107EC38 7F00\n8107EC3A 0000\n"
                                        "8107EC28 0000\n8107EC2A FF00\n8107EC20 0000\n8107EC22 7F00\n"
                                        "8107EC58 FFFF\n8107EC5A FF00\n8107EC50 7F7F\n8107EC52 7F00\n"
                                        "8107EC70 721C\n8107EC72 0E00\n8107EC68 390E\n8107EC6A 0700\n"
                                        "8107EC88 FEC1\n8107EC8A 7900\n8107EC80 7F60\n8107EC82 3C00\n"
                                        "8107ECA0 7306\n8107ECA2 0000\n8107EC98 3903\n8107EC9A 0000";

ImVec4 uiHatMainColor =          ImVec4(255.0f / 255.0f, 0.0f / 255.0f, 0.0f / 255.0f, 255.0f / 255.0f);
ImVec4 uiHatShadeColor =         ImVec4(127.0f / 255.0f, 0.0f / 255.0f, 0.0f / 255.0f, 255.0f / 255.0f);
ImVec4 uiOverallsMainColor =     ImVec4(0.0f / 255.0f, 0.0f / 255.0f, 255.0f / 255.0f, 255.0f / 255.0f);
ImVec4 uiOverallsShadeColor =    ImVec4(0.0f / 255.0f, 0.0f / 255.0f, 127.0f / 255.0f, 255.0f / 255.0f);
ImVec4 uiGlovesMainColor =       ImVec4(255.0f / 255.0f, 255.0f / 255.0f, 255.0f / 255.0f, 255.0f / 255.0f);
ImVec4 uiGlovesShadeColor =      ImVec4(127.0f / 255.0f, 127.0f / 255.0f, 127.0f / 255.0f, 255.0f / 255.0f);
ImVec4 uiShoesMainColor =        ImVec4(114.0f / 255.0f, 28.0f / 255.0f, 14.0f / 255.0f, 255.0f / 255.0f);
ImVec4 uiShoesShadeColor =       ImVec4(57.0f / 255.0f, 14.0f / 255.0f, 7.0f / 255.0f, 255.0f / 255.0f);
ImVec4 uiSkinMainColor =         ImVec4(254.0f / 255.0f, 193.0f / 255.0f, 121.0f / 255.0f, 255.0f / 255.0f);
ImVec4 uiSkinShadeColor =        ImVec4(127.0f / 255.0f, 96.0f / 255.0f, 60.0f / 255.0f, 255.0f / 255.0f);
ImVec4 uiHairMainColor =         ImVec4(115.0f / 255.0f, 6.0f / 255.0f, 0.0f / 255.0f, 255.0f / 255.0f);
ImVec4 uiHairShadeColor =        ImVec4(57.0f / 255.0f, 3.0f / 255.0f, 0.0f / 255.0f, 255.0f / 255.0f);
// SPARK
ImVec4 uiShirtMainColor =        ImVec4(255.0f / 255.0f, 255.0f / 255.0f, 0.0f / 255.0f, 255.0f / 255.0f);
ImVec4 uiShirtShadeColor =       ImVec4(127.0f / 255.0f, 127.0f / 255.0f, 0.0f / 255.0f, 255.0f / 255.0f);
ImVec4 uiShouldersMainColor =    ImVec4(0.0f / 255.0f, 255.0f / 255.0f, 255.0f / 255.0f, 255.0f / 255.0f);
ImVec4 uiShouldersShadeColor =   ImVec4(0.0f / 255.0f, 127.0f / 255.0f, 127.0f / 255.0f, 255.0f / 255.0f);
ImVec4 uiArmsMainColor =         ImVec4(0.0f / 255.0f, 255.0f / 255.0f, 127.0f / 255.0f, 255.0f / 255.0f);
ImVec4 uiArmsShadeColor =        ImVec4(0.0f / 255.0f, 127.0f / 255.0f, 64.0f / 255.0f, 255.0f / 255.0f);
ImVec4 uiPelvisMainColor =       ImVec4(255.0f / 255.0f, 0.0f / 255.0f, 255.0f / 255.0f, 255.0f / 255.0f);
ImVec4 uiPelvisShadeColor =      ImVec4(127.0f / 255.0f, 0.0f / 255.0f, 127.0f / 255.0f, 255.0f / 255.0f);
ImVec4 uiThighMainColor =        ImVec4(255.0f / 255.0f, 0.0f / 255.0f, 127.0f / 255.0f, 255.0f / 255.0f);
ImVec4 uiThighShadeColor =       ImVec4(127.0f / 255.0f, 0.0f / 255.0f, 64.0f / 255.0f, 255.0f / 255.0f);
ImVec4 uiCalfMainColor =         ImVec4(127.0f / 255.0f, 0.0f / 255.0f, 255.0f / 255.0f, 255.0f / 255.0f);
ImVec4 uiCalfShadeColor =        ImVec4(64.0f / 255.0f, 0.0f / 255.0f, 127.0f / 255.0f, 255.0f / 255.0f);

bool refreshEditorPalette;
int refreshCounter = 0;

std::vector<int> accessory_packs;
int active_accessory_index = -1;

int hat_pos[3] = {0, 0, 0};
int hat_rot[3] = {0, 0, 0};
float hat_scale[3] = {1.f, 1.f, 1.f};

bool canBeReloaded = false;
bool forceReload = false;

/* Update our CC Editor colors with our "defaultColor" values.
This should be called when loading a CC, to insert our new colors into the editor. */
void UpdateEditorFromPalette() {
    uiHatMainColor = ImVec4(float(defaultColorHat.red[0]) / 255.0f, float(defaultColorHat.green[0]) / 255.0f, float(defaultColorHat.blue[0]) / 255.0f, 255.0f / 255.0f);
    uiHatShadeColor = ImVec4(float(defaultColorHat.red[1]) / 255.0f, float(defaultColorHat.green[1]) / 255.0f, float(defaultColorHat.blue[1]) / 255.0f, 255.0f / 255.0f);
    uiOverallsMainColor = ImVec4(float(defaultColorOveralls.red[0]) / 255.0f, float(defaultColorOveralls.green[0]) / 255.0f, float(defaultColorOveralls.blue[0]) / 255.0f, 255.0f / 255.0f);
    uiOverallsShadeColor = ImVec4(float(defaultColorOveralls.red[1]) / 255.0f, float(defaultColorOveralls.green[1]) / 255.0f, float(defaultColorOveralls.blue[1]) / 255.0f, 255.0f / 255.0f);
    uiGlovesMainColor = ImVec4(float(defaultColorGloves.red[0]) / 255.0f, float(defaultColorGloves.green[0]) / 255.0f, float(defaultColorGloves.blue[0]) / 255.0f, 255.0f / 255.0f);
    uiGlovesShadeColor = ImVec4(float(defaultColorGloves.red[1]) / 255.0f, float(defaultColorGloves.green[1]) / 255.0f, float(defaultColorGloves.blue[1]) / 255.0f, 255.0f / 255.0f);
    uiShoesMainColor = ImVec4(float(defaultColorShoes.red[0]) / 255.0f, float(defaultColorShoes.green[0]) / 255.0f, float(defaultColorShoes.blue[0]) / 255.0f, 255.0f / 255.0f);
    uiShoesShadeColor = ImVec4(float(defaultColorShoes.red[1]) / 255.0f, float(defaultColorShoes.green[1]) / 255.0f, float(defaultColorShoes.blue[1]) / 255.0f, 255.0f / 255.0f);
    uiSkinMainColor = ImVec4(float(defaultColorSkin.red[0]) / 255.0f, float(defaultColorSkin.green[0]) / 255.0f, float(defaultColorSkin.blue[0]) / 255.0f, 255.0f / 255.0f);
    uiSkinShadeColor = ImVec4(float(defaultColorSkin.red[1]) / 255.0f, float(defaultColorSkin.green[1]) / 255.0f, float(defaultColorSkin.blue[1]) / 255.0f, 255.0f / 255.0f);
    uiHairMainColor = ImVec4(float(defaultColorHair.red[0]) / 255.0f, float(defaultColorHair.green[0]) / 255.0f, float(defaultColorHair.blue[0]) / 255.0f, 255.0f / 255.0f);
    uiHairShadeColor = ImVec4(float(defaultColorHair.red[1]) / 255.0f, float(defaultColorHair.green[1]) / 255.0f, float(defaultColorHair.blue[1]) / 255.0f, 255.0f / 255.0f);

    uiShirtMainColor = ImVec4(float(sparkColorShirt.red[0]) / 255.0f, float(sparkColorShirt.green[0]) / 255.0f, float(sparkColorShirt.blue[0]) / 255.0f, 255.0f / 255.0f);
    uiShirtShadeColor = ImVec4(float(sparkColorShirt.red[1]) / 255.0f, float(sparkColorShirt.green[1]) / 255.0f, float(sparkColorShirt.blue[1]) / 255.0f, 255.0f / 255.0f);
    uiShouldersMainColor = ImVec4(float(sparkColorShoulders.red[0]) / 255.0f, float(sparkColorShoulders.green[0]) / 255.0f, float(sparkColorShoulders.blue[0]) / 255.0f, 255.0f / 255.0f);
    uiShouldersShadeColor = ImVec4(float(sparkColorShoulders.red[1]) / 255.0f, float(sparkColorShoulders.green[1]) / 255.0f, float(sparkColorShoulders.blue[1]) / 255.0f, 255.0f / 255.0f);
    uiArmsMainColor = ImVec4(float(sparkColorArms.red[0]) / 255.0f, float(sparkColorArms.green[0]) / 255.0f, float(sparkColorArms.blue[0]) / 255.0f, 255.0f / 255.0f);
    uiArmsShadeColor = ImVec4(float(sparkColorArms.red[1]) / 255.0f, float(sparkColorArms.green[1]) / 255.0f, float(sparkColorArms.blue[1]) / 255.0f, 255.0f / 255.0f);
    uiPelvisMainColor = ImVec4(float(sparkColorPelvis.red[0]) / 255.0f, float(sparkColorPelvis.green[0]) / 255.0f, float(sparkColorPelvis.blue[0]) / 255.0f, 255.0f / 255.0f);
    uiPelvisShadeColor = ImVec4(float(sparkColorPelvis.red[1]) / 255.0f, float(sparkColorPelvis.green[1]) / 255.0f, float(sparkColorPelvis.blue[1]) / 255.0f, 255.0f / 255.0f);
    uiThighMainColor = ImVec4(float(sparkColorThigh.red[0]) / 255.0f, float(sparkColorThigh.green[0]) / 255.0f, float(sparkColorThigh.blue[0]) / 255.0f, 255.0f / 255.0f);
    uiThighShadeColor = ImVec4(float(sparkColorThigh.red[1]) / 255.0f, float(sparkColorThigh.green[1]) / 255.0f, float(sparkColorThigh.blue[1]) / 255.0f, 255.0f / 255.0f);
    uiCalfMainColor = ImVec4(float(sparkColorCalf.red[0]) / 255.0f, float(sparkColorCalf.green[0]) / 255.0f, float(sparkColorCalf.blue[0]) / 255.0f, 255.0f / 255.0f);
    uiCalfShadeColor = ImVec4(float(sparkColorCalf.red[1]) / 255.0f, float(sparkColorCalf.green[1]) / 255.0f, float(sparkColorCalf.blue[1]) / 255.0f, 255.0f / 255.0f);

    current_color_code.ParseGameShark();
    strcpy(uiGameShark, current_color_code.GameShark.c_str());
    strcpy(uiCcLabelName, current_color_code.Name.c_str());

    send_palette_to_network();
}

/* Returns true if any models are currently selected. */
bool AnyModelsEnabled() {
    // Cycle through each DynOS pack,
    // Return true soon as one enabled
    for (int i = 0; i < DynOS_Pack_GetCount(); i++) {
        PackData* pack = DynOS_Pack_GetFromIndex(i);
        if (pack->mEnabled) return true;
    }
    return false;
}

/* Returns true if a given DynOS model index is intended for Saturn. */
bool IsSaturnModel(int index) {
    PackData* pack = DynOS_Pack_GetFromIndex(index);

    // If the gfx data is already loaded, just check if it affects Mario at all
    if (pack->mLoaded) {
        for (auto& pair : pack->mGfxData) {
            for (s32 geoIndex = pair.second->mGeoLayouts.Count() - 1; geoIndex >= 0; geoIndex--) {
                auto &_GeoNode = pair.second->mGeoLayouts[geoIndex];
                String _GeoRootName = _GeoNode->mName;
                if (_GeoRootName == "mario_geo") return true;
            }
        }
    }

    // Failsafe by checking for Saturn-specific files and folders
    return (std::filesystem::exists(std::filesystem::path(pack->mPath + "/model.json")) ||
            std::filesystem::exists(std::filesystem::path(pack->mPath + "/mario_geo.bin")));
}

/* Returns true if an expression list consists only of RGBA32 expressions (the format in which Saturn relies on). */
bool IsAllRGBA32(std::vector<Expression> expression_list) {
    bool value = true;
    for (Expression expression : expression_list) {
        if (expression.Visible && (expression.Format != G_IM_FMT_RGBA || expression.Size != G_IM_SIZ_32b)) {
            value = false;
        }
    }
    return value;
}

bool IsAccessoryModel(int index) {
    PackData* pack = DynOS_Pack_GetFromIndex(index);

    if (pack->mLoaded) {
        for (auto& pair : pack->mGfxData) {
            for (s32 geoIndex = pair.second->mGeoLayouts.Count() - 1; geoIndex >= 0; geoIndex--) {
                auto &_GeoNode = pair.second->mGeoLayouts[geoIndex];
                String _GeoRootName = _GeoNode->mName;
                if (_GeoRootName == "accessory_geo") return true;
            }
        }
    }

    return (std::filesystem::is_directory(std::filesystem::path(pack->mPath + "/accessory")) ||
            std::filesystem::exists(std::filesystem::path(pack->mPath + "/accessory_geo.bin")));
}

void LoadAccessories() {
    accessory_packs.clear();
    for (int i = 0; i < DynOS_Pack_GetCount(); i++) {
        PackData* pack = DynOS_Pack_GetFromIndex(i);
        if (IsAccessoryModel(i)) accessory_packs.push_back(i);
    }
}

void DisableAllAccessories() {
    for (int i = 0; i < accessory_packs.size(); i++) {
        if (IsAccessoryModel(i)) {
            PackData* pack = DynOS_Pack_GetFromIndex(accessory_packs[i]);
            DynOS_Pack_SetEnabled(pack, false);
        }
    }
}

bool ModelGeoBinExists(int index) {
    if (index < 0 || index >= DynOS_Pack_GetCount()) return false;
    PackData* pack = DynOS_Pack_GetFromIndex(index);
    if (!pack->mLoaded) return false;
    return std::filesystem::exists(std::filesystem::path(pack->mPath + "/mario_geo.bin"));
}

/* Loads Saturn model data at a given DynOS index. */
void LoadModelData(int index, bool enabled, bool first_use, bool auto_reload) {
    PackData* pack = DynOS_Pack_GetFromIndex(index);
    if (first_use && enabled && IsAccessoryModel(index)) {
        DynOS_Pack_SetEnabled(pack, false);
        return;
    }

    // Disable all accessory models
    if (AnyModelsEnabled() && !enabled) {
        for (int i = 0; i < DynOS_Pack_GetCount(); i++) {
            PackData* pack = DynOS_Pack_GetFromIndex(i);
            if (pack->mEnabled && i != index && IsAccessoryModel(i)) {
                DynOS_Pack_SetEnabled(pack, false);
            }
        }
    }

    // Attempt to enable pack
    DynOS_Pack_SetEnabled(pack, enabled);
    if (!pack->mLoaded) return;
    canBeReloaded = auto_reload == false || ModelGeoBinExists(index);

    switch_state_eyes = 0;
    custom_eyes = false;
    LoadAccessories();

    // ACCESSORIES
    if (IsAccessoryModel(index) && accessory_packs.size() > 0) {
        // Load accessory model data
        if (enabled) {
            std::vector<Expression> accessory_expressions = LoadExpressions(pack->mPath);
            for (Expression expression : accessory_expressions) {
                if (expression.Textures.size() > 0) expression.CurrentIndex = 0;
            }
            current_expressions.insert(current_expressions.end(), accessory_expressions.begin(), accessory_expressions.end());

            active_accessory_index = index;
        } else {
            active_accessory_index = -1;
            // Reload the active Saturn model if the accessory is disabled
            PackData* pack = DynOS_Pack_GetFromIndex(active_saturn_model_index);
            current_expressions.clear();
            LoadModelData(active_saturn_model_index, pack->mEnabled, false, auto_reload);
        }
    // SATURN MODELS
    } else if (IsSaturnModel(index)) {
        active_saturn_model_index = -1;
        
        model_color_code_list = GetColorCodeList(pack->mPath + "/colorcodes");
        if (model_color_code_list.size() > 0) {
            ColorCode overwrite = (enabled) ? LoadGSFile(model_color_code_list[0], pack->mPath + "/colorcodes") : ColorCode();
            if (first_use) {
                current_color_code = overwrite;
                uiCcListId = (enabled) ? -1 : 0;
                PasteGameShark(current_color_code.GameShark, false);
                current_color_code.ParseGameShark();
                UpdateEditorFromPalette();
                send_palette_to_network();
            }
        }

        // Load expressions list
        if (enabled || index == DynOS_Pack_GetCount() - 1) {
            current_expressions.clear();
            current_expressions = LoadExpressions(pack->mPath);
            if (active_accessory_index != -1) {
                std::vector<Expression> accessory_expressions = LoadExpressions(DynOS_Pack_GetFromIndex(active_accessory_index)->mPath);
                current_expressions.insert(current_expressions.end(), accessory_expressions.begin(), accessory_expressions.end());
            }
        }

        for (Expression expression : current_expressions) {
            if (expression.Textures.size() > 0) expression.CurrentIndex = 0;
        }

        if (enabled) {
            active_saturn_model_index = index;
            // Populate bone names from pack's primary geo layout
            std::vector<std::string> boneNames;
            if (pack->mGfxData.Count() > 0) {
                GfxData* gfxData = pack->mGfxData[0].second;
                if (gfxData) {
                    for (s32 i = 0; i < gfxData->mBoneNames.Count(); i++)
                        boneNames.push_back(std::string(gfxData->mBoneNames[i].begin()));
                }
            }
            SetActiveBoneNames(boneNames);
            LoadBoneFlagsFromPackDir(pack->mPath);
        }
        else {
            active_saturn_model_index = -1;
            active_accessory_index = -1;
            ResetBoneFlagsInMemory();
        }
    }
}

/* Refreshes the visibility of the active model's expression list */
void RefreshActiveExpressions() {
    if (active_saturn_model_index == -1 || current_expressions.size() <= 0) return;

    for (int i = 0; i < current_expressions.size(); i++)
        current_expressions[i].Visible = false;
}

bool CheckModelNeedsReload() {
    if (!canBeReloaded) return false;
    if (active_saturn_model_index == -1) return false;
    PackData* pack = DynOS_Pack_GetFromIndex(active_saturn_model_index);

    // If our folder was deleted, we need to reset everything
    if (!std::filesystem::exists(std::filesystem::path(pack->mPath))) {
        for (int i = 0; i < DynOS_Pack_GetCount(); i++) {
            if (!IsSaturnModel(i)) continue;
            LoadModelData(i, false, false, false);
        }
        active_saturn_model_index = -1;
        active_accessory_index = -1;
        return true;
    }

    if (!configAutoReloadModels) return false;
    if (!pack->mLoaded || !pack->mEnabled) return false;

    // Check if mario_geo.bin is missing
    if (!std::filesystem::exists(std::filesystem::path(pack->mPath + "/mario_geo.bin"))) {
        return true;
    }

    return false;
}