#ifndef SaturnModels
#define SaturnModels

#include <PR/ultratypes.h>
#include "saturn_textures.h"

#ifdef __cplusplus

#include <vector>
#include <string>
#include "saturn/libs/imgui/imgui.h"

extern char uiCcLabelName[128];
extern char uiGameShark[1024 * 16];
extern ImVec4 uiHatMainColor;
extern ImVec4 uiHatShadeColor;
extern ImVec4 uiOverallsMainColor;
extern ImVec4 uiOverallsShadeColor;
extern ImVec4 uiGlovesMainColor;
extern ImVec4 uiGlovesShadeColor;
extern ImVec4 uiShoesMainColor;
extern ImVec4 uiShoesShadeColor;
extern ImVec4 uiSkinMainColor;
extern ImVec4 uiSkinShadeColor;
extern ImVec4 uiHairMainColor;
extern ImVec4 uiHairShadeColor;
extern ImVec4 uiShirtMainColor;
extern ImVec4 uiShirtShadeColor;
extern ImVec4 uiShouldersMainColor;
extern ImVec4 uiShouldersShadeColor;
extern ImVec4 uiArmsMainColor;
extern ImVec4 uiArmsShadeColor;
extern ImVec4 uiPelvisMainColor;
extern ImVec4 uiPelvisShadeColor;
extern ImVec4 uiThighMainColor;
extern ImVec4 uiThighShadeColor;
extern ImVec4 uiCalfMainColor;
extern ImVec4 uiCalfShadeColor;
extern void UpdateEditorFromPalette();

extern bool IsSaturnModel(int);
extern bool AnyModelsEnabled();
extern bool IsAllRGBA32(std::vector<Expression>);

extern "C" {
#endif    
    extern float marioScaleX;
    extern float marioScaleY;
    extern float marioScaleZ;
    extern void LoadModelData(int, bool, bool);
    extern void RefreshActiveExpressions();
    extern bool refreshEditorPalette;
    extern int refreshCounter;
#ifdef __cplusplus
}
#endif

#endif