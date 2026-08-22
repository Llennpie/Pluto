#ifndef SaturnImGuiColors
#define SaturnImGuiColors

#include <SDL2/SDL.h>
#include <PR/ultratypes.h>

#ifdef __cplusplus

#include <string>

extern void SaveActiveColorCode(std::string);
extern bool saving_to_model;

extern "C" {
#endif
    void UpdatePaletteFromEditor(int);
    void RefreshColorCodeList();
    void OpenCCEditor();
#ifdef __cplusplus
}
#endif

#endif