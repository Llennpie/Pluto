#ifndef SaturnTextures
#define SaturnTextures

#include <stdio.h>
#include <stdbool.h>
#include <PR/ultratypes.h>

#ifdef __cplusplus
#include <string>
#include <vector>
#include <filesystem>
#include <iostream>
#include <map>

class TexturePath {
public:
    std::string FileName;
    /* File name without extension */
    std::string ShortFileName() {
        size_t pos = FileName.find_last_of(".");
        if (pos > 0 && pos != std::string::npos)
            return FileName.substr(0, pos);
        return FileName;
    }
    std::string FilePath;
    u8 *RawData = 0;
    int Width = 32;
    int Height = 32;
    /* Returns a mini path following a specified expression name */
    std::string SmallExpressionPath(std::string ExpressionName) {
        size_t pos = FilePath.find("/" + ExpressionName + "/");
        if (pos == std::string::npos) {
            pos = FilePath.find("/" + ExpressionName.substr(0, ExpressionName.size() - 1) + "/");
            if (pos == std::string::npos) return FilePath;
            return FilePath.substr(pos + ExpressionName.size() + 3);
        }
        return FilePath.substr(pos + ExpressionName.size() + 2);
    }
    unsigned int Preview;
};

class Expression {
public:
    std::string Name;
    std::string FolderPath;
    /* Returns true if the replace key was detected in the texture pool
       (Note: This ignores whether or not an expression folder is present) */
    bool Visible;

    /* The material's texture format, identified in saturn_bind_texture when it is first displayed onscreen */
    uint32_t Format;
    uint32_t Size;
    
    std::vector<TexturePath> Textures;
    std::vector<TexturePath> Folders;

    /* The index of the current selected texture */
    int CurrentIndex = 0;

    /* Used in popouts */
    int RightClickIndex = 0;

    /* Returns true if a given path contains a prefix (saturn_) followed by the expression's replacement keywords
       For example, "/eye" can be "saturn_eye" or "saturn_eyes" */
    bool PathHasReplaceKey(std::string path, std::string prefix) {
        // We also append a _ or . to the end, so "saturn_eyebrow" doesn't get read as "saturn_eye"
        return (path.find(prefix + this->Name + "_") != std::string::npos ||
                path.find(prefix + this->Name + "s_") != std::string::npos ||
                path.find(prefix + this->Name.substr(0, this->Name.size() - 1) + "_") != std::string::npos);
    }

    /* Returns true if the expression's textures are formatted for a checkbox */
    bool IsToggleFormat() {
        return (this->Textures.size() == 2 && this->Folders.size() == 0);
    }

    void Refresh();

    /* An array of indexes making up a custom blink cycle; Only used by eyes */
    int BlinkIndex[2] = {-1, -1};

    /* Stored model folder path for later refreshes */
    std::string ModelFolderPath;
};

extern bool format_warning_dismissed;

extern u8* SetTextureData(TexturePath, int*, int*, unsigned int*);

extern Expression LoadEyesFolder();
std::vector<TexturePath> LoadExpressionTextures(std::string, Expression);
std::vector<Expression> LoadExpressions(std::string);
extern int GetValidExpressionCount(std::vector<Expression>);

extern std::vector<Expression> current_expressions;

extern "C" {
#endif    
    #include "include/types.h"
    const void* saturn_bind_texture(const void*, uint32_t, uint32_t, uint8_t, uint8_t*, struct Object*);
    void saturn_custom_blink(s16* switch_eyes, s16 blink_frame, s8 eye_state);
#ifdef __cplusplus
}
#endif

#endif