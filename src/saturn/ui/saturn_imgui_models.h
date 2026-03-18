#ifndef SaturnImGuiModels
#define SaturnImGuiModels

#include <SDL2/SDL.h>
#include <PR/ultratypes.h>

#ifdef __cplusplus

#include <string>
#include "saturn/saturn_textures.h"

extern "C" {
#endif
    void OpenExpressionPreview(TexturePath* texture);
    void OpenEyeSelector();
    void OpenSwitchOptions();
    void OpenExtraOptions();
    void OpenAccessorySettings();
    void OpenModelSettings();
    void PopupModelSettings();
    void OpenModelSettingsAtCursor();
    void OpenModelSelector();
#ifdef __cplusplus
}
#endif

#endif