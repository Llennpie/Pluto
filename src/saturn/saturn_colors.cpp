#include "saturn_colors.h"

#include <string>
#include <iostream>
#include <algorithm>
#include <vector>
#include <SDL2/SDL.h>

#include <dirent.h>
#include <filesystem>
#include <fstream>

#include "saturn/libs/imgui/imgui.h"
#include "saturn/libs/imgui/imgui_internal.h"
#include "saturn/libs/imgui/imgui_impl_sdl.h"
#include "saturn/libs/imgui/imgui_impl_opengl3.h"

#include "src/saturn/saturn.h"
#include "src/pc/pc_main.h"
#include "pc/network/network_player.h"

int uiCcListId = 1;

ColorTemplate defaultColorHat       {255, 127, 0,   0,   0,   0  };
ColorTemplate defaultColorOveralls  {0,   0,   0,   0,   255, 127};
ColorTemplate defaultColorGloves    {255, 127, 255, 127, 255, 127};
ColorTemplate defaultColorShoes     {114, 57,  28,  14,  14,  7  };
ColorTemplate defaultColorSkin      {254, 127, 193, 96,  121, 60 };
ColorTemplate defaultColorHair      {115, 57,  6,   3,   0,   0  };
bool spark_enabled;
ColorTemplate sparkColorShirt       {255, 127, 255, 127, 0,   0  };
ColorTemplate sparkColorShoulders   {0,   0,   255, 127, 255, 127};
ColorTemplate sparkColorArms        {0,   0,   255, 127, 127, 64 };
ColorTemplate sparkColorPelvis      {255, 127, 0,   0,   255, 127}; 
ColorTemplate sparkColorThigh       {255, 127, 0,   0,   127, 64 };
ColorTemplate sparkColorCalf        {127, 64,  0,   0,   255, 127};

ColorCode current_color_code;
std::vector<std::string> color_code_list;
std::vector<std::string> model_color_code_list;

bool auto_chroma;
ColorTemplate chromaColor           {0,   0,   255, 127, 0,   0  };
u16 gChromaKeyColor = 0x07C1;
bool chroma_show_objects;
bool chroma_show_geo;
bool chroma_affects_light = true;
bool chroma_transparent_background;

bool shade_lighting_enabled;
float shade_lighting_dir[3] = {0.0f, 0.0f, 0.0f};

/* Converts a color template into a hex string */
std::string HexifyColorTemplate(ColorTemplate &colorBodyPart) {
    char colour[64];
    ImFormatString(colour, IM_ARRAYSIZE(colour), "%02X%02X%02X%02X%02X%02X"
            , ImClamp((int)colorBodyPart.red[0], 0, 255)
            , ImClamp((int)colorBodyPart.green[0], 0, 255)
            , ImClamp((int)colorBodyPart.blue[0], 0, 255)
            , ImClamp((int)colorBodyPart.red[1], 0, 255)
            , ImClamp((int)colorBodyPart.green[1], 0, 255)
            , ImClamp((int)colorBodyPart.blue[1], 0, 255));
    std::string col = colour;
    return col;
}

/* Updates the current palette with a specified GameShark string */
void PasteGameShark(std::string GameShark, bool checkSize) {
    std::istringstream f(GameShark);
    std::string line;
        
    while (std::getline(f, line)) {
        if (line.size() != 13 && checkSize) return;
        if (line.rfind("81", 0) == 0) {
            int address = std::stoi(line.substr(2, 6), 0, 16);
            int value1 = std::stoi(line.substr(9, 2), 0, 16);
            int value2 = std::stoi(line.substr(11, 2), 0, 16);

            switch(address) {

                /* Address (80000000 + Offset)  RED     GREEN
                                                BLUE          */

                // Cap
                case 0x07EC40:  defaultColorHat.red[0] = value1;        defaultColorHat.green[0] = value2;          break;
                case 0x07EC42:  defaultColorHat.blue[0] = value1;                                                   break;
                case 0x07EC38:  defaultColorHat.red[1] = value1;        defaultColorHat.green[1] = value2;          break;
                case 0x07EC3A:  defaultColorHat.blue[1] = value1;                                                   break;
                // Overalls
                case 0x07EC28:  defaultColorOveralls.red[0] = value1;   defaultColorOveralls.green[0] = value2;     break;
                case 0x07EC2A:  defaultColorOveralls.blue[0] = value1;                                              break;
                case 0x07EC20:  defaultColorOveralls.red[1] = value1;   defaultColorOveralls.green[1] = value2;     break;
                case 0x07EC22:  defaultColorOveralls.blue[1] = value1;                                              break;
                // Gloves
                case 0x07EC58:  defaultColorGloves.red[0] = value1;     defaultColorGloves.green[0] = value2;       break;
                case 0x07EC5A:  defaultColorGloves.blue[0] = value1;                                                break;
                case 0x07EC50:  defaultColorGloves.red[1] = value1;     defaultColorGloves.green[1] = value2;       break;
                case 0x07EC52:  defaultColorGloves.blue[1] = value1;                                                break;
                // Shoes
                case 0x07EC70:  defaultColorShoes.red[0] = value1;      defaultColorShoes.green[0] = value2;        break;
                case 0x07EC72:  defaultColorShoes.blue[0] = value1;                                                 break;
                case 0x07EC68:  defaultColorShoes.red[1] = value1;      defaultColorShoes.green[1] = value2;        break;
                case 0x07EC6A:  defaultColorShoes.blue[1] = value1;                                                 break;
                // Skin
                case 0x07EC88:  defaultColorSkin.red[0] = value1;       defaultColorSkin.green[0] = value2;         break;
                case 0x07EC8A:  defaultColorSkin.blue[0] = value1;                                                  break;
                case 0x07EC80:  defaultColorSkin.red[1] = value1;       defaultColorSkin.green[1] = value2;         break;
                case 0x07EC82:  defaultColorSkin.blue[1] = value1;                                                  break;
                // Hair
                case 0x07ECA0:  defaultColorHair.red[0] = value1;       defaultColorHair.green[0] = value2;         break;
                case 0x07ECA2:  defaultColorHair.blue[0] = value1;                                                  break;
                case 0x07EC98:  defaultColorHair.red[1] = value1;       defaultColorHair.green[1] = value2;         break;
                case 0x07EC9A:  defaultColorHair.blue[1] = value1;                                                  break;

                // (Optional) SPARK Addresses

                // Shirt
                case 0x07ECB8:  sparkColorShirt.red[0] = value1;        sparkColorShirt.green[0] = value2;          break;
                case 0x07ECBA:  sparkColorShirt.blue[0] = value1;                                                   break;
                case 0x07ECB0:  sparkColorShirt.red[1] = value1;        sparkColorShirt.green[1] = value2;          break;
                case 0x07ECB2:  sparkColorShirt.blue[1] = value1;                                                   break;
                // Shoulders
                case 0x07ECD0:  sparkColorShoulders.red[0] = value1;    sparkColorShoulders.green[0] = value2;      break;
                case 0x07ECD2:  sparkColorShoulders.blue[0] = value1;                                               break;
                case 0x07ECC8:  sparkColorShoulders.red[1] = value1;    sparkColorShoulders.green[1] = value2;      break;
                case 0x07ECCA:  sparkColorShoulders.blue[1] = value1;                                               break;
                // Arms
                case 0x07ECE8:  sparkColorArms.red[0] = value1;         sparkColorArms.green[0] = value2;           break;
                case 0x07ECEA:  sparkColorArms.blue[0] = value1;                                                    break;
                case 0x07ECE0:  sparkColorArms.red[1] = value1;         sparkColorArms.green[1] = value2;           break;
                case 0x07ECE2:  sparkColorArms.blue[1] = value1;                                                    break;
                // Pelvis
                case 0x07ED00:  sparkColorPelvis.red[0] = value1;       sparkColorPelvis.green[0] = value2;         break;
                case 0x07ED02:  sparkColorPelvis.blue[0] = value1;                                                  break;
                case 0x07ECF8:  sparkColorPelvis.red[1] = value1;       sparkColorPelvis.green[1] = value2;         break;
                case 0x07ECFA:  sparkColorPelvis.blue[1] = value1;                                                  break;
                // Thighs
                case 0x07ED18:  sparkColorThigh.red[0] = value1;        sparkColorThigh.green[0] = value2;          break;
                case 0x07ED1A:  sparkColorThigh.blue[0] = value1;                                                   break;
                case 0x07ED10:  sparkColorThigh.red[1] = value1;        sparkColorThigh.green[1] = value2;          break;
                case 0x07ED12:  sparkColorThigh.blue[1] = value1;                                                   break;
                // Calves
                case 0x07ED30:  sparkColorCalf.red[0] = value1;         sparkColorCalf.green[0] = value2;           break;
                case 0x07ED32:  sparkColorCalf.blue[0] = value1;                                                    break;
                case 0x07ED28:  sparkColorCalf.red[1] = value1;         sparkColorCalf.green[1] = value2;           break;
                case 0x07ED2A:  sparkColorCalf.blue[1] = value1;                                                    break;
            }

            // Shirt
            gNetworkPlayers[0].overridePalette.parts[1][0] = defaultColorHat.red[0];
            gNetworkPlayers[0].overridePalette.parts[1][1] = defaultColorHat.green[0];
            gNetworkPlayers[0].overridePalette.parts[1][2] = defaultColorHat.blue[0];
            // Cap
            gNetworkPlayers[0].overridePalette.parts[6][0] = defaultColorHat.red[0];
            gNetworkPlayers[0].overridePalette.parts[6][1] = defaultColorHat.green[0];
            gNetworkPlayers[0].overridePalette.parts[6][2] = defaultColorHat.blue[0];
            // Overalls
            gNetworkPlayers[0].overridePalette.parts[0][0] = defaultColorOveralls.red[0];
            gNetworkPlayers[0].overridePalette.parts[0][1] = defaultColorOveralls.green[0];
            gNetworkPlayers[0].overridePalette.parts[0][2] = defaultColorOveralls.blue[0];
            // Gloves
            gNetworkPlayers[0].overridePalette.parts[2][0] = defaultColorGloves.red[0];
            gNetworkPlayers[0].overridePalette.parts[2][1] = defaultColorGloves.green[0];
            gNetworkPlayers[0].overridePalette.parts[2][2] = defaultColorGloves.blue[0];
            // Shoes
            gNetworkPlayers[0].overridePalette.parts[3][0] = defaultColorShoes.red[0];
            gNetworkPlayers[0].overridePalette.parts[3][1] = defaultColorShoes.green[0];
            gNetworkPlayers[0].overridePalette.parts[3][2] = defaultColorShoes.blue[0];
            // Skin
            gNetworkPlayers[0].overridePalette.parts[5][0] = defaultColorSkin.red[0];
            gNetworkPlayers[0].overridePalette.parts[5][1] = defaultColorSkin.green[0];
            gNetworkPlayers[0].overridePalette.parts[5][2] = defaultColorSkin.blue[0];
            // Hair
            gNetworkPlayers[0].overridePalette.parts[4][0] = defaultColorHair.red[0];
            gNetworkPlayers[0].overridePalette.parts[4][1] = defaultColorHair.green[0];
            gNetworkPlayers[0].overridePalette.parts[4][2] = defaultColorHair.blue[0];
        }
    }
}

/* Returns a list of GS filepaths with a given folder directory */
std::vector<std::string> GetColorCodeList(std::string folderPath) {
    std::vector<std::string> cc_list;
    uiCcListId = 0;

    if (folderPath.find("dynos/packs") != std::string::npos) {
        // Model-specific color codes
        if (std::filesystem::exists(folderPath + "/../default.gs")) {
            cc_list.push_back("../default.gs");
        }
        
        if (std::filesystem::exists(folderPath)) {
            for (const auto & entry : std::filesystem::directory_iterator(folderPath)) {
                std::filesystem::path path = entry.path();

                if (path.filename().generic_string() != "Mario") {
                    if (path.extension().generic_string() == ".gs" ||
                        path.extension().generic_string() == ".txt")
                            cc_list.push_back(path.filename().generic_string());
                }
            }
        }
    } else {
        // Vanilla colorcodes
        if (std::filesystem::exists(folderPath)) {
            for (const auto & entry : std::filesystem::directory_iterator(folderPath)) {
                std::filesystem::path path = entry.path();
                
                if (std::filesystem::is_directory(path)) continue;
                if (path.filename().generic_string() != "Mario") {
                    if (path.extension().generic_string() == ".gs" ||
                        path.extension().generic_string() == ".txt")
                            cc_list.push_back(path.filename().generic_string());
                }
            }
        }

        cc_list.insert(cc_list.begin(), "Mario.gs");
    }

    send_palette_to_network();
    return cc_list;
}

ColorCode LoadGSFile(std::string fileName, std::string filePath) {
    ColorCode colorCode;

    if (fileName == "../default.gs") {
        filePath = std::filesystem::path(filePath).parent_path().generic_string();
        fileName = "default.gs";
    }

    if (fileName != "Mario.gs") {
        std::ifstream file(filePath + "/" + fileName, std::ios::in | std::ios::binary);
        if (file.good()) {
            std::cout << "Loaded CC file: " << filePath << "/" << fileName << std::endl;

            // Read GS File
            const std::size_t& size = std::filesystem::file_size(filePath + "/" + fileName);
            std::string content(size, '\0');
            file.read(content.data(), size);
            file.close();

            // Write to CC
            colorCode.Name = fileName.substr(0, fileName.find_last_of('.'));
            colorCode.FilePath = filePath;
            colorCode.GameShark = content;
            colorCode.IsModel = (filePath.find("dynos/packs") != std::string::npos);

        } else {
            // Load failed; Refresh active list
            std::cout << "Failed to load " << fileName.c_str() << std::endl;
            if (filePath.find("dynos/packs") != std::string::npos) model_color_code_list = GetColorCodeList(filePath);
            else color_code_list = GetColorCodeList(filePath);
        }
    }

    // Change conflicting file names
    if (colorCode.Name == "Mario" || colorCode.Name == "default")
        colorCode.Name = "Default";

    return colorCode;
}