#ifndef SaturnAnimations
#define SaturnAnimations

#include <PR/ultratypes.h>
extern const char* saturn_animations[];

#ifdef __cplusplus
#include <string>
#include <vector>

class PlutoAnim {
    public:
        std::string Name;
        std::string Author;
        bool Looping = false;
        int Length = -1;
        int Nodes;
        std::vector<s16> Values;
        std::vector<u16> Indices;
};

extern PlutoAnim LoadPAnim(std::string filePath);
extern std::vector<std::string> GetPAnimList(std::string folderPath);
extern void saturn_play_pluto_animation();

extern std::vector<std::string> pluto_animations_list;

#endif

#endif