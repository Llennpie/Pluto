#include "saturn_textures.h"

#include <string>
#include <iostream>
#include <vector>
#include <SDL2/SDL.h>
#include <GL/glew.h>

#include <dirent.h>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <stdlib.h>

#include "saturn/libs/imgui/imgui.h"
#include "saturn/libs/imgui/imgui_internal.h"
#include "saturn/libs/imgui/imgui_impl_sdl.h"
#include "saturn/libs/imgui/imgui_impl_opengl3.h"

#include "src/saturn/saturn.h"
#include "src/saturn/saturn_models.h"

#include "data/dynos.cpp.h"
#include "pc/debuglog.h"
#include "pc/gfx/gfx_rendering_api.h"
#include "pc/gfx/gfx_pc.h"
#include "src/game/rendering_graph_node.h"
extern "C" {
    #include "src/game/mario_misc.h"
}

std::vector<Expression> current_expressions;
bool format_warning_dismissed;

bool loading_texture_data;

static u8 *RGBA32_RGBA16(const u8 *aData, u64 aLength) {
    u8 *_Buffer = New<u8>(aLength);
    u8 *pBuffer = _Buffer;
    for (u64 i = 0; i != aLength; i += 4) {
        u8 _Red   = (aData[i + 0] * 31) / 255;
        u8 _Grn   = (aData[i + 1] * 31) / 255;
        u8 _Blu   = (aData[i + 2] * 31) / 255;
        //u8 _Alp   = (aData[i + 3] * 1) / 255;
        //u16 _Col  = (_Red << 11) | (_Grn << 6) | (_Blu << 1) | (_Alp << 0);
        u8 _Alp   = (aData[i + 3] > 128) ? 1 : 0;
        u16 _Col  = (_Red << 11) | (_Grn << 6) | (_Blu << 1) | _Alp;
        *(pBuffer++) = (_Col >> 8) & 0xFF;
        *(pBuffer++) = (_Col >> 0) & 0xFF;
    }
    return _Buffer;
}

uint32_t gTextureId = 0;

/* Fetches a TexturePath's raw texture data in RGBA32 format */
u8* SetTextureData(TexturePath Texture, int* Width, int* Height, unsigned int* Preview) {
    //if (Texture.RawData != 0) return Texture.RawData;
    if (loading_texture_data) return 0;
    loading_texture_data = true;

    // Open texture file for reading
    FILE *TextureFile = fopen(Texture.FilePath.c_str(), "rb");
    if (!TextureFile) return 0;

    // Scan as PNG data
    fseek(TextureFile, 0, SEEK_END);
    TexData* TextureData = New<TexData>();
    TextureData->mPngData.Resize(ftell(TextureFile)); rewind(TextureFile);
    fread(TextureData->mPngData.begin(), sizeof(u8), TextureData->mPngData.Count(), TextureFile);
    fclose(TextureFile);

    // Convert to RGBA-32 texture data
    u8 *RawData = stbi_load_from_memory(TextureData->mPngData.begin(), TextureData->mPngData.Count(), &TextureData->mRawWidth, &TextureData->mRawHeight, NULL, 4);
    std::cout << Texture.FilePath << " --> w" << TextureData->mRawWidth << " h" << TextureData->mRawHeight << " c-> " << TextureData->mPngData.Count() << std::endl;
    TextureData->mRawFormat = G_IM_FMT_RGBA;
    TextureData->mRawSize   = G_IM_SIZ_32b;
    TextureData->mRawData   = Array<u8>(RawData, RawData + (TextureData->mRawWidth * TextureData->mRawHeight * 4));

    GLuint ExpressionPreview;
    glGenTextures(1, &ExpressionPreview);
    glBindTexture(GL_TEXTURE_2D, ExpressionPreview);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, TextureData->mRawWidth, TextureData->mRawHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, RawData);
    glBindTexture(GL_TEXTURE_2D, 0);
    *Preview = ExpressionPreview;
    stbi_image_free(RawData);

    u8 *_Buffer = TextureData->mRawData.begin();
    *Width = TextureData->mRawWidth;
    *Height = TextureData->mRawHeight;

    loading_texture_data = false;
    return _Buffer;
}

bool sortAlphabetically (TexturePath a, TexturePath b) { return a.FilePath < b.FilePath; }

/* Loads textures into an Expression */
std::vector<TexturePath> LoadExpressionTextures(std::string FolderPath, Expression expression) {
    std::vector<TexturePath> textures;

    // Check if the expression's folder exists
    if (std::filesystem::is_directory(std::filesystem::path(FolderPath))) {
        for (const auto & entry : std::filesystem::recursive_directory_iterator(FolderPath)) {
            if (!std::filesystem::is_directory(entry.path())) {
                if (entry.path().extension() == ".png") {
                    // Only allow PNG files
                    TexturePath texture;
                    texture.FileName = entry.path().filename().generic_u8string();
                    texture.FilePath = entry.path().generic_u8string();
                    textures.push_back(texture);
                }
            }
        }
        std::sort(textures.begin(), textures.end(), sortAlphabetically);
    }
    return textures;
}

/* Returns true if a model is missing an eyes folder */
bool UsingVanillaEyes(std::string modelFolderPath) {
    if (AnyModelsEnabled() &&
        std::filesystem::is_directory(modelFolderPath + "/expressions/eyes") ||
        std::filesystem::is_directory(modelFolderPath + "/expressions/eye"))
            return false;

    return true;
}

/* Loads an expression list into a specified model */
std::vector<Expression> LoadExpressions(std::string modelFolderPath) {
    std::vector<Expression> expressions_list;
    switch_state_eyes = 0;
    custom_eyes = false;

    // Check if the model's /expressions folder exists
    if (std::filesystem::is_directory(std::filesystem::path(modelFolderPath + "/expressions"))) {
        for (const auto & entry : std::filesystem::directory_iterator(modelFolderPath + "/expressions")) {
            // Load individual expression folders
            if (std::filesystem::is_directory(entry.path())) {
                Expression expression;
                expression.FolderPath = entry.path().generic_u8string();
                expression.Name = entry.path().filename().generic_u8string();
                if (expression.Name == "eye")
                    expression.Name = "eyes";

                // Load all PNG files
                expression.Textures = LoadExpressionTextures(expression.FolderPath, expression);
                if (expression.Textures.size() <= 0) continue;

                // Eyes will always appear first
                if (expression.Name == "eyes") {
                    expressions_list.insert(expressions_list.begin(), expression);
                } else {
                    expressions_list.push_back(expression);
                }

                expression.ModelFolderPath = modelFolderPath;
            }
        }
    }

    // If no eyes folder was loaded, load from vanilla eyes
    if (UsingVanillaEyes(modelFolderPath))
        expressions_list.insert(expressions_list.begin(), LoadEyesFolder());

    return expressions_list;
}

/* Loads textures from dynos/eyes/ into a global Expression */
Expression LoadEyesFolder() {
    Expression VanillaEyes;
    // Check if the dynos/eyes/ folder exists
    if (std::filesystem::is_directory("dynos/eyes")) {
        VanillaEyes.Name = "eyes";
        VanillaEyes.FolderPath = "dynos/eyes";
        VanillaEyes.Textures = LoadExpressionTextures(VanillaEyes.FolderPath, VanillaEyes);
    }
    return VanillaEyes;
}

/* Refresh an individual expression's texture images and update changed files */
void Expression::Refresh() {
    if (std::filesystem::is_directory(std::filesystem::path(this->FolderPath))) {
        for (const auto & entry : std::filesystem::directory_iterator(this->FolderPath)) {
            if (std::filesystem::is_directory(entry.path())) {
                this->Textures = LoadExpressionTextures(this->FolderPath, *this);
                if (this->CurrentIndex <= this->Textures.size())
                    this->CurrentIndex = 0;
                // Clear texture data for later re-loading
                this->Textures[this->CurrentIndex].RawData = 0;
            }
        }
    }
}

/* Returns the number of "valid expressions" in a list- i.e. expressions that are actually editable in the UI, excluding eyes */
int GetValidExpressionCount(std::vector<Expression> expressions_list) {
    int count;
    for (Expression expression : expressions_list) {
        if (expression.Name == "eyes") continue;
        if (expression.Textures.size() < 2) continue;
        if ((expression.Format != G_IM_FMT_RGBA || expression.Size != G_IM_SIZ_32b) && !format_warning_dismissed) continue;
        count++;
    }
    return count;
}

/* Loads a texture's raw data if it hasn't already */
void InitTextureData(int exp_index, int tex_index, int tile) {
    if (current_expressions[exp_index].Textures[tex_index].RawData == 0) {
        // Generate new texture data
        current_expressions[exp_index].Textures[tex_index].RawData =
            SetTextureData(current_expressions[exp_index].Textures[tex_index],
                           &current_expressions[exp_index].Textures[tex_index].Width,
                           &current_expressions[exp_index].Textures[tex_index].Height,
                           &current_expressions[exp_index].Textures[tex_index].Preview);

        if (gTextureId == 0) gTextureId = gfx_get_current_rendering_api()->new_texture();
        gfx_get_current_rendering_api()->select_texture(tile, gTextureId);
        gfx_get_current_rendering_api()->set_sampler_parameters(tile, false, 0, 0);
        gfx_get_current_rendering_api()->upload_texture(current_expressions[exp_index].Textures[tex_index].RawData,
                                                        current_expressions[exp_index].Textures[tex_index].Width,
                                                        current_expressions[exp_index].Textures[tex_index].Height);
    }
}

/* Handles texture replacement. Called from gfx_pc.c */
const void* saturn_bind_texture(const void* input, uint32_t format, uint32_t size, uint8_t tile, uint8_t* is_expression, Object* currentObj) {
    if (input == nullptr || gDjuiInMainMenu) return input;
    const char* inputTexture = static_cast<const char*>(input);
    const char* outputTexture;
    std::string texName = inputTexture;

    if (texName.find("saturn_") != std::string::npos && (currentObj == gMarioStates[0].marioObj || (find_hat_object() != NULL && currentObj == find_hat_object()))) {
        for (int i = 0; i < current_expressions.size(); i++) {
            Expression expression = current_expressions[i];
            if (expression.CurrentIndex < 0) return input;

            // Checks if the incoming texture has the expression's "key"
            // This could be "saturn_eye", "saturn_mouth", etc.
            if (expression.PathHasReplaceKey(texName, "saturn_")) {
                current_expressions[i].Visible = true;

                // Set texture format
                current_expressions[i].Format = format;
                current_expressions[i].Size = size;
                // Only support RGBA32 textures
                if ((format != G_IM_FMT_RGBA || size != G_IM_SIZ_32b) && !format_warning_dismissed) return input;

                // Load texture data if it hasn't been loaded yet
                // This is to prevent the game from crashing when the texture is missing
                InitTextureData(i, expression.CurrentIndex, (int)tile);

                *is_expression = 1;
                saturn_update_texture_expression((const uint8_t*)expression.Textures[expression.CurrentIndex].RawData,
                                                tile, size,
                                                expression.Textures[expression.CurrentIndex].Width,
                                                expression.Textures[expression.CurrentIndex].Height);

                // Custom blink cycle
                if (expression.Name == "eyes" && expression.Textures.size() >= 3 &&
                        expression.BlinkIndex[0] != -1 &&
                        expression.BlinkIndex[1] != -1) {
                    // Create an 8 frame animation cycle (synced with gMarioBlinkAnimation)
                    s16 blink_frame = ((7 * 32 + gAreaUpdateCounter) >> 1) & 0x1F;
                    switch (blink_frame) {
                        default:
                        case 3:
                            // Eyes Open
                            return expression.Textures[expression.CurrentIndex].RawData;
                        case 0:
                        case 2:
                        case 4:
                        case 6:
                            // Eyes Half
                            InitTextureData(i, expression.BlinkIndex[0], (int)tile);
                            saturn_update_texture_expression((const uint8_t*)expression.Textures[expression.BlinkIndex[0]].RawData,
                                                tile, size,
                                                expression.Textures[expression.BlinkIndex[0]].Width,
                                                expression.Textures[expression.BlinkIndex[0]].Height);
                            return expression.Textures[expression.BlinkIndex[0]].RawData;
                        case 1:
                        case 5:
                            // Eyes Closed
                            InitTextureData(i, expression.BlinkIndex[1], (int)tile);
                            saturn_update_texture_expression((const uint8_t*)expression.Textures[expression.BlinkIndex[1]].RawData,
                                                tile, size,
                                                expression.Textures[expression.BlinkIndex[1]].Width,
                                                expression.Textures[expression.BlinkIndex[1]].Height);
                            return expression.Textures[expression.BlinkIndex[1]].RawData;
                    }
                }

                return expression.Textures[expression.CurrentIndex].RawData;
            }
        }
    }

    return input;
}

/**
 * The eye texture on succesive frames of Mario's blink animation.
 * He intentionally blinks twice each time.
 */
static s8 gMarioBlinkAnimation[7] = { 1, 2, 1, 0, 1, 2, 1 };

/* Custom blink cycle, handling eye switch options including custom eyes */
void saturn_custom_blink(s16* switch_eyes, s16 blink_frame, s8 eye_state) {
    if (switch_state_eyes <= 3 || switch_state_eyes == 8) custom_eyes = false;
    switch (switch_state_eyes) {
        // Blink Cycle
        case 0:
            if (eye_state == 0) {
                if (blink_frame < 7) *switch_eyes = gMarioBlinkAnimation[blink_frame];
                else *switch_eyes = 0;
            } else *switch_eyes = eye_state - 1;
            break;
        // Open, Half, Closed
        case 1:
        case 2:
        case 3:
        // Dead
        case 8:
            *switch_eyes = switch_state_eyes - 1;
            break;
        // Custom Eyes
        default:
            custom_eyes = true;
            *switch_eyes = switch_state_eyes - 1;
            break;
    }
}