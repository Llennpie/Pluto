#ifndef SaturnAnimations
#define SaturnAnimations

#include <PR/ultratypes.h>
#include "include/types.h"
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
        int BoneCount = 20;

        std::string FileName;
        std::string FilePath;
        bool HasQueue() {
            return (this->FileName.find_last_of("_") == this->FileName.find_last_of(".") - 3);
        }
};

extern PlutoAnim LoadPAnim(std::string filePath);
extern std::vector<PlutoAnim> GetPAnimList(std::string folderPath);
extern void saturn_play_pluto_animation();
extern bool saturn_check_for_chainer();

extern std::vector<PlutoAnim> pluto_animations_list;

extern s16 bone_anim_values[63];
extern u16 bone_anim_indices[126];
extern bool is_editing_panim;
extern Vec3f bone_rotations[21];

extern PlutoAnim current_pluto_anim;

#endif

#endif