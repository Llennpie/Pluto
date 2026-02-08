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

        bool WasVanilla = false;

        std::string FileName;
        std::string FilePath;
        bool HasQueue() {
            return (this->FileName.find_last_of("_") == this->FileName.find_last_of(".") - 3);
        }
};

extern PlutoAnim LoadPAnim(std::string filePath);
extern std::vector<PlutoAnim> GetPAnimList(std::string folderPath);
extern PlutoAnim ConvertFromVanilla();
extern void saturn_play_pluto_animation();
extern bool saturn_check_for_chainer();

extern std::vector<PlutoAnim> pluto_animations_list;

// Wrapper struct for Vec3f to make it compatible with std::vector
struct Vec3fWrapper {
    f32 data[3];
    
    f32& operator[](int index) { return data[index]; }
    const f32& operator[](int index) const { return data[index]; }
    
    // Allow implicit conversion to Vec3f for compatibility
    operator f32*() { return data; }
    operator const f32*() const { return data; }
};

extern std::vector<s16> bone_anim_values;
extern std::vector<u16> bone_anim_indices;
extern bool is_editing_panim;
extern std::vector<Vec3fWrapper> bone_rotations;

extern PlutoAnim current_pluto_anim;
extern void AutoPushCustomBones();
extern std::vector<std::pair<int, bool>> model_bone_list;
extern int cached_total_bone_count;
extern int cached_custom_bone_count;

extern bool WasVanilla();

extern "C" {
#endif    
    extern void AddToBoneCountList(bool is_custom);
    extern void ResetBoneCountList();
    extern bool ExtraBoneInBounds(int);
    extern void CacheBoneCount();
    extern int GetTotalBoneCount();
#ifdef __cplusplus
}
#endif

#endif