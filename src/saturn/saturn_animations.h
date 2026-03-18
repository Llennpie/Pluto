#ifndef SaturnAnimations
#define SaturnAnimations

#include <PR/ultratypes.h>
#include "include/types.h"
extern const char* saturn_animations[];

#ifdef __cplusplus
#include <string>
#include <vector>

class PlutoBone {
    public:
        int Index = 0;
        int VanillaIndex = -1;
        std::string CustomName;
        std::string Name() const {
            if (!CustomName.empty()) return CustomName;
            if (IsCustom) return "> Custom " + std::to_string(Index + 1) + " <";
            switch (VanillaIndex + 1) {
                case 1:  return "Root";
                case 2:  return "Pelvis";
                case 3:  return "Torso";
                case 4:  return "Head";
                case 5:  return "Left Arm";
                case 6:  return "Left Upper Arm";
                case 7:  return "Left Lower Arm";
                case 8:  return "Left Hand";
                case 9:  return "Right Arm";
                case 10: return "Right Upper Arm";
                case 11: return "Right Lower Arm";
                case 12: return "Right Hand";
                case 13: return "Left Leg";
                case 14: return "Left Upper Leg";
                case 15: return "Left Lower Leg";
                case 16: return "Left Foot";
                case 17: return "Right Leg";
                case 18: return "Right Upper Leg";
                case 19: return "Right Lower Leg";
                case 20: return "Right Foot";
                default: return "Unassigned " + std::to_string(Index + 1);
            }
        }
        Vec3s Translation = {};
        f32   Rotation[3] = {};
        f32   Scale[3]    = { 1.0f, 1.0f, 1.0f };
        Vec3f WorldPosition = {};
        bool IsCustom      = false;
        bool IsWiggle      = false;
        bool BoneHidden    = false;
        bool WiggleDisabled = false;
        bool WindDisabled   = false;
};

class PlutoAnim {
    public:
        std::string Name;
        std::string Author;
        bool Looping = false;
        int Length = -1;
        int Nodes;
        std::vector<s16> Values;
        std::vector<u16> Indices;
        std::vector<s16> Translations; // optional; non-root-bone-major [(idx-1) * frame_count + frame] * 3
        std::vector<s16> Scales;        // optional; same layout as Translations; s16 / 1024.0f = scale factor
        int BoneCount = 20;
        int CustomBoneCount = 0;
        int CustomBoneCountScale = 0;
        std::vector<PlutoBone> Bones;
        std::string FileName;
        std::string FilePath;
        bool HasQueue() const {
            return (this->FileName.find_last_of("_") == this->FileName.find_last_of(".") - 3);
        }
};

extern PlutoAnim LoadPAnim(const std::string& filePath);
extern std::vector<PlutoAnim> GetPAnimList(const std::string& folderPath);
extern PlutoAnim ConvertFromVanilla();
extern void saturn_play_pluto_animation();
extern bool saturn_check_for_chainer();

extern std::vector<PlutoAnim> pluto_animations_list;

extern std::vector<s16> bone_anim_values;
extern std::vector<u16> bone_anim_indices;
extern bool is_editing_panim;
extern Vec3f g_root_offset;

extern PlutoAnim current_pluto_anim;
extern void AutoPushCustomBones();

extern "C" {
#endif    
    extern void AddToBoneCountList(bool is_custom, bool is_wiggle, Vec3s rotation);
    extern void ApplyBoneTranslation(Vec3s out, Vec3s out_prev);
    extern void ApplyBoneScale(Vec3f out, Vec3f out_prev);
    extern bool SaturnShouldApplyBoneTranslation(void);
    extern bool SaturnShouldApplyBoneScale(void);

    extern bool SaturnIsEditingPAnim(void);
    extern void BumpBoneTranslationCounter(void);
    extern void BumpBoneScaleCounter(void);
    extern void ResetBoneApplyCounters(void);
    extern void StoreBoneWorldPosition(Vec3f pos);
    extern void ResetBoneCountList();
    extern void CacheBoneCount();
    extern bool ExtraBoneInBounds(int);
    extern int GetTotalBoneCount();
    extern bool SaturnCurrentBoneIsHidden(void);
    extern bool SaturnCurrentBoneWiggleDisabled(void);
    extern bool SaturnCurrentBoneWindDisabled(void);
#ifdef __cplusplus
}
    extern void SetActiveBoneNames(const std::vector<std::string>& names);
    extern void SaveAndScheduleRestoreBoneFlags();
    extern void PreserveBoneFlagsAsPending();
    extern void ClearBoneFlags();
    extern void ResetBoneFlagsInMemory();
    extern void LoadBoneFlagsFromPackDir(const std::string& pack_path);
    extern void SaveBoneFlagsToPackDir(const std::string& pack_path);
#endif

#endif