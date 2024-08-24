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
    int Width;
    int Height;
    bool IsModelTexture = false;
};

class Expression {
public:
    std::string Name;
    std::string FolderPath;
    /* Returns true if the replace key was detected in the texture pool
       (Note: This ignores whether or not an expression folder is present) */
    bool Visible;
    /* The material's texture format, as read in the replace key's filepath
       (Note: This may be misleading if the geo.inc.c was manually edited) */
    std::string Format;
    
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

    /* Returns true if the expression's textures are formatted for a checkbox
       This is when an expression has two non-model textures and no subfolders */
    bool IsToggleFormat() {
        if (this->Textures.size() >= 2 && this->Folders.size() == 0) {
            int countWithoutModelTextures = 0;
            for (int i = 0; i < this->Textures.size(); i++) {
                // Exclude model textures
                if (this->Textures[i].IsModelTexture) continue;
                countWithoutModelTextures++;
            }
            return(countWithoutModelTextures == 2);
        }
        return false;
    }

    /* An array of indexes making up a custom blink cycle; Only used by eyes */
    int BlinkIndex[2] = {-1, -1};
};

extern u8* GetTextureData(TexturePath, int*, int*);

extern Expression LoadEyesFolder();
std::vector<TexturePath> LoadExpressionTextures(std::string, Expression);
std::vector<Expression> LoadExpressions(std::string);
extern int GetValidExpressionCount(std::vector<Expression>);

extern std::vector<Expression> current_expressions;

extern "C" {
#endif    
    #include "include/types.h"
    const void* saturn_bind_texture(const void*, struct Object*);
    void saturn_custom_blink(s16* switch_eyes, s16 blink_frame, s8 eye_state);
#ifdef __cplusplus
}
#endif

#endif