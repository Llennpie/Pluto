#ifndef SaturnImGuiColors
#define SaturnImGuiColors

#include <SDL2/SDL.h>
#include <PR/ultratypes.h>

#ifdef __cplusplus

#include <string>

extern void SaveActiveColorCode(std::string);

extern "C" {
#endif
    void UpdateEditorFromPalette();
    void UpdatePaletteFromEditor(int);
    void RefreshColorCodeList();
    void OpenCCEditor();
    void UpdateEditorLabels();
#ifdef __cplusplus
}
#endif

#endif