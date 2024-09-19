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

#include "pc/pc_main.h"
#include "pc/djui/djui_console.h"
#include "data/dynos.cpp.h"

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
    return (std::filesystem::is_directory(std::filesystem::path(pack->mPath + "/expressions")) ||
            std::filesystem::is_directory(std::filesystem::path(pack->mPath + "/colorcodes")) ||
            std::filesystem::exists(std::filesystem::path(pack->mPath + "/model.json")) ||
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

/* Loads Saturn model data at a given DynOS index. */
void LoadModelData(int index, bool enabled) {
    PackData* pack = DynOS_Pack_GetFromIndex(index);
    // Attempt to enable pack
    DynOS_Pack_SetEnabled(pack, enabled);
    if (!pack->mLoaded) return;

    if (IsSaturnModel(index)) {
        active_saturn_model_index = -1;
        
        model_color_code_list = GetColorCodeList(pack->mPath + "/colorcodes");

        /*current_color_code = LoadGSFile(model_color_code_list[0], pack->mPath + "/colorcodes");
        PasteGameShark(current_color_code.GameShark, false);
        send_palette_to_network();*/

        // Disable previous Saturn models
        /*for (int j = 0; j < DynOS_Pack_GetCount(); j++) {
            PackData* dummy = DynOS_Pack_GetFromIndex(j);
            if (IsSaturnModel(j) && dummy != pack) DynOS_Pack_SetEnabled(dummy, false);
        }*/

        // Load expressions list
        if (enabled || index == DynOS_Pack_GetCount() - 1) {
            current_expressions.clear();
            current_expressions = LoadExpressions(pack->mPath);
        }

        for (Expression expression : current_expressions) {
            if (expression.Textures.size() > 0) expression.CurrentIndex = 0;
        }

        if (enabled) active_saturn_model_index = index;
        else active_saturn_model_index = -1;
    }
}

/* Refreshes the visibility of the active model's expression list */
void RefreshActiveExpressions() {
    if (active_saturn_model_index == -1 || current_expressions.size() <= 0) return;

    for (int i = 0; i < current_expressions.size(); i++)
        current_expressions[i].Visible = false;
}