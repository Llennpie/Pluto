#ifndef SaturnImGuiModels
#define SaturnImGuiModels

#include <SDL2/SDL.h>
#include <PR/ultratypes.h>

#ifdef __cplusplus

#include <string>

extern "C" {
#endif
    void OpenEyeSelector();
    void OpenSwitchOptions();
    void OpenExtraOptions();
    void OpenAccessorySettings();
    void OpenModelSettings();
    void OpenModelSelector();
#ifdef __cplusplus
}
#endif

#endif