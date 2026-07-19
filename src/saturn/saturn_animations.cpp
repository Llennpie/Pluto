#include "saturn_animations.h"

#include <cstdint>
#include <string>
#include <iostream>
#include <cstring>
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
#include "saturn/ui/studio_notifications.h"

#include "src/saturn/saturn.h"
#include "src/pc/pc_main.h"
extern "C" {
    #include "sm64.h"
    #include "game/mario.h"
    #include "game/mario_misc.h"
    #include "game/level_update.h"
    #include "engine/math_util.h"
    #include "pc/network/network_player.h"
    #include "saturn/libs/json2panim.h"
}

const char* saturn_animations[] = {
    "SLOW_LEDGE_GRAB",
    "FALL_OVER_BACKWARDS",
    "BACKWARD_AIR_KB",
    "DYING_ON_BACK",
    "BACKFLIP",
    "CLIMB_UP_POLE",
    "GRAB_POLE_SHORT",
    "GRAB_POLE_SWING_PART1",
    "GRAB_POLE_SWING_PART2",
    "HANDSTAND_IDLE",
    "HANDSTAND_JUMP",
    "START_HANDSTAND",
    "RETURN_FROM_HANDSTAND",
    "IDLE_ON_POLE",
    "A_POSE",
    "SKID_ON_GROUND",
    "STOP_SKID",
    "CROUCH_FROM_FAST_LONGJUMP",
    "CROUCH_FROM_SLOW_LONGJUMP",
    "FAST_LONGJUMP",
    "SLOW_LONGJUMP",
    "AIRBORNE_ON_STOMACH",
    "WALK_WITH_LIGHT_OBJ",
    "RUN_WITH_LIGHT_OBJ",
    "SLOW_WALK_WITH_LIGHT_OBJ",
    "SHIVERING_WARMING_HAND",
    "SHIVERING_RETURN_TO_IDLE",
    "SHIVERING",
    "CLIMB_DOWN_LEDGE",
    "CREDITS_WAVING",
    "CREDITS_LOOK_UP",
    "CREDITS_RETURN_FROM_LOOK_UP",
    "CREDITS_RAISE_HAND",
    "CREDITS_LOWER_HAND",
    "CREDITS_TAKE_OFF_CAP",
    "CREDITS_START_WALK_LOOK_UP",
    "CREDITS_LOOK_BACK_THEN_RUN",
    "FINAL_BOWSER_RAISE_HAND_SPIN",
    "FINAL_BOWSER_WING_CAP_TAKE_OFF",
    "CREDITS_PEACE_SIGN",
    "STAND_UP_FROM_LAVA_BOOST",
    "FIRE_LAVA_BURN",
    "WING_CAP_FLY",
    "HANG_ON_OWL",
    "LAND_ON_STOMACH",
    "AIR_FORWARD_KB",
    "DYING_ON_STOMACH",
    "SUFFOCATING",
    "COUGHING",
    "THROW_CATCH_KEY",
    "DYING_FALL_OVER",
    "IDLE_ON_LEDGE",
    "FAST_LEDGE_GRAB",
    "HANG_ON_CEILING",
    "PUT_CAP_ON",
    "TAKE_CAP_OFF_THEN_ON",
    "QUICKLY_PUT_CAP_ON", // unused
    "HEAD_STUCK_IN_GROUND",
    "GROUND_POUND_LANDING",
    "TRIPLE_JUMP_GROUND_POUND",
    "START_GROUND_POUND",
    "GROUND_POUND",
    "BOTTOM_STUCK_IN_GROUND",
    "IDLE_WITH_LIGHT_OBJ",
    "JUMP_LAND_WITH_LIGHT_OBJ",
    "JUMP_WITH_LIGHT_OBJ",
    "FALL_LAND_WITH_LIGHT_OBJ",
    "FALL_WITH_LIGHT_OBJ",
    "FALL_FROM_SLIDING_WITH_LIGHT_OBJ",
    "SLIDING_ON_BOTTOM_WITH_LIGHT_OBJ",
    "STAND_UP_FROM_SLIDING_WITH_LIGHT_OBJ",
    "RIDING_SHELL",
    "WALKING",
    "FORWARD_FLIP", // unused
    "JUMP_RIDING_SHELL",
    "LAND_FROM_DOUBLE_JUMP",
    "DOUBLE_JUMP_FALL",
    "SINGLE_JUMP",
    "LAND_FROM_SINGLE_JUMP",
    "AIR_KICK",
    "DOUBLE_JUMP_RISE",
    "START_FORWARD_SPINNING", // unused
    "THROW_LIGHT_OBJECT",
    "FALL_FROM_SLIDE_KICK",
    "BEND_KNESS_RIDING_SHELL", // unused
    "LEGS_STUCK_IN_GROUND",
    "GENERAL_FALL",
    "GENERAL_LAND",
    "BEING_GRABBED",
    "GRAB_HEAVY_OBJECT",
    "SLOW_LAND_FROM_DIVE",
    "FLY_FROM_CANNON",
    "MOVE_ON_WIRE_NET_RIGHT",
    "MOVE_ON_WIRE_NET_LEFT",
    "MISSING_CAP",
    "PULL_DOOR_WALK_IN",
    "PUSH_DOOR_WALK_IN",
    "UNLOCK_DOOR",
    "START_REACH_POCKET", // unused", reaching keys maybe?
    "REACH_POCKET", // unused
    "STOP_REACH_POCKET", // unused
    "GROUND_THROW",
    "GROUND_KICK",
    "FIRST_PUNCH",
    "SECOND_PUNCH",
    "FIRST_PUNCH_FAST",
    "SECOND_PUNCH_FAST",
    "PICK_UP_LIGHT_OBJ",
    "PUSHING",
    "START_RIDING_SHELL",
    "PLACE_LIGHT_OBJ",
    "FORWARD_SPINNING",
    "BACKWARD_SPINNING",
    "BREAKDANCE",
    "RUNNING",
    "RUNNING_UNUSED", // unused duplicate", originally part 2?
    "SOFT_BACK_KB",
    "SOFT_FRONT_KB",
    "DYING_IN_QUICKSAND",
    "IDLE_IN_QUICKSAND",
    "MOVE_IN_QUICKSAND",
    "ELECTROCUTION",
    "SHOCKED",
    "BACKWARD_KB",
    "FORWARD_KB",
    "IDLE_HEAVY_OBJ",
    "STAND_AGAINST_WALL",
    "SIDESTEP_LEFT",
    "SIDESTEP_RIGHT",
    "START_SLEEP_IDLE",
    "START_SLEEP_SCRATCH",
    "START_SLEEP_YAWN",
    "START_SLEEP_SITTING",
    "SLEEP_IDLE",
    "SLEEP_START_LYING",
    "SLEEP_LYING",
    "DIVE",
    "SLIDE_DIVE",
    "GROUND_BONK",
    "STOP_SLIDE_LIGHT_OBJ",
    "SLIDE_KICK",
    "CROUCH_FROM_SLIDE_KICK",
    "SLIDE_MOTIONLESS", // unused
    "STOP_SLIDE",
    "FALL_FROM_SLIDE",
    "SLIDE",
    "TIPTOE",
    "TWIRL_LAND",
    "TWIRL",
    "START_TWIRL",
    "STOP_CROUCHING",
    "START_CROUCHING",
    "CROUCHING",
    "CRAWLING",
    "STOP_CRAWLING",
    "START_CRAWLING",
    "SUMMON_STAR",
    "RETURN_STAR_APPROACH_DOOR",
    "BACKWARDS_WATER_KB",
    "SWIM_WITH_OBJ_PART1",
    "SWIM_WITH_OBJ_PART2",
    "FLUTTERKICK_WITH_OBJ",
    "WATER_ACTION_END_WITH_OBJ", // either swimming or flutterkicking
    "STOP_GRAB_OBJ_WATER",
    "WATER_IDLE_WITH_OBJ",
    "DROWNING_PART1",
    "DROWNING_PART2",
    "WATER_DYING",
    "WATER_FORWARD_KB",
    "FALL_FROM_WATER",
    "SWIM_PART1",
    "SWIM_PART2",
    "FLUTTERKICK",
    "WATER_ACTION_END", // either swimming or flutterkicking
    "WATER_PICK_UP_OBJ",
    "WATER_GRAB_OBJ_PART2",
    "WATER_GRAB_OBJ_PART1",
    "WATER_THROW_OBJ",
    "WATER_IDLE",
    "WATER_STAR_DANCE",
    "RETURN_FROM_WATER_STAR_DANCE",
    "GRAB_BOWSER",
    "SWINGING_BOWSER",
    "RELEASE_BOWSER",
    "HOLDING_BOWSER",
    "HEAVY_THROW",
    "WALK_PANTING",
    "WALK_WITH_HEAVY_OBJ",
    "TURNING_PART1",
    "TURNING_PART2",
    "SLIDEFLIP_LAND",
    "SLIDEFLIP",
    "TRIPLE_JUMP_LAND",
    "TRIPLE_JUMP",
    "FIRST_PERSON",
    "IDLE_HEAD_LEFT",
    "IDLE_HEAD_RIGHT",
    "IDLE_HEAD_CENTER",
    "HANDSTAND_LEFT",
    "HANDSTAND_RIGHT",
    "WAKE_FROM_SLEEP",
    "WAKE_FROM_LYING",
    "START_TIPTOE",
    "SLIDEJUMP", // pole jump and wall kick
    "START_WALLKICK",
    "STAR_DANCE",
    "RETURN_FROM_STAR_DANCE",
    "FORWARD_SPINNING_FLIP",
    "TRIPLE_JUMP_FLY"
};

PlutoAnim current_pluto_anim;
std::vector<PlutoAnim> pluto_animations_list;
bool is_editing_panim;
Vec3f g_root_offset = {};

struct BoneFlagEntry { bool BoneHidden, WiggleDisabled, WindDisabled; };
static std::vector<BoneFlagEntry> s_bone_flags;
static bool s_flags_need_restore = false;

// Persistent bone flags loaded from the model pack's bone_flags.cfg
// I don't like that we're going back to model configs but these are only saved once edited, thank god
// Format is just a text file with lines like "Bone 5: Hidden, NoWiggle" that get loaded on anim load and saved on edit
// To-do: Wipe them each time we export from SFast64 since adding/removing bones desyncs the indices
struct PersistentFlagEntry { bool BoneHidden, WiggleDisabled, WindDisabled, valid; };
static std::vector<PersistentFlagEntry> s_pending_bone_flags;

s16 ReadS16(const std::vector<char>& data, int index) {
    return ((unsigned int)(unsigned char)data[index] << 8) | (unsigned int)(unsigned char)data[index + 1];
}

/* Loads a PlutoAnim struct from a given filepath */
PlutoAnim LoadPAnim(const std::string& filePath) {
    PlutoAnim plutoAnim;

    std::ifstream input(filePath, std::ios::binary);

    std::vector<char> bytes(
        (std::istreambuf_iterator<char>(input)),
        (std::istreambuf_iterator<char>()));
    input.close();

    if (bytes.size() > 0) {
        // Metadata - read fixed-length name/author fields, trim trailing nulls
        plutoAnim.Name.assign(bytes.data(), 0x40);
        { auto p = plutoAnim.Name.find('\0'); if (p != std::string::npos) plutoAnim.Name.resize(p); }
        plutoAnim.Author.assign(bytes.data() + 0x40, 0x20);
        { auto p = plutoAnim.Author.find('\0'); if (p != std::string::npos) plutoAnim.Author.resize(p); }

        int length1 = (uint8_t)bytes[0x61];
        int length2 = (uint8_t)bytes[0x62];
        plutoAnim.Length = (length1<<8)|(length2);

        plutoAnim.Looping = (uint8_t)bytes[0x60] != 0x00;

        std::size_t values_pos = 0x6A;
        std::size_t indices_pos;

        // Values
        for (int v = values_pos; v < bytes.size(); v += 2) {
            if (strncmp(bytes.data() + v, "indices", 7) == 0) {
                indices_pos = v + 7;
                break;
            }
            plutoAnim.Values.push_back(ReadS16(bytes, v));
        }

        // Indices
        std::size_t translations_pos = 0;
        for (int i = indices_pos; i < (int)bytes.size(); i += 2) {
            if (i + 12 <= (int)bytes.size() && strncmp(bytes.data() + i, "translations", 12) == 0) {
                translations_pos = i + 12;
                break;
            }
            plutoAnim.Indices.push_back(ReadS16(bytes, i));
        }

        // Translations (optional: sparse bitmask format, custom bones only)
        // Format: [2 bytes frame_count][2 bytes custom_count][ceil(custom_count/8) bytes bitmask][data]
        if (translations_pos > 0 && translations_pos + 4 <= (int)bytes.size()) {
            int frame_count  = ((uint8_t)bytes[translations_pos]     << 8) | (uint8_t)bytes[translations_pos + 1];
            int custom_count = ((uint8_t)bytes[translations_pos + 2] << 8) | (uint8_t)bytes[translations_pos + 3];
            int bitmask_off  = translations_pos + 4;
            int bitmask_bytes = (custom_count + 7) / 8;
            int data_pos     = bitmask_off + bitmask_bytes;
            plutoAnim.CustomBoneCount = custom_count;
            plutoAnim.Translations.assign(custom_count * frame_count * 3, 0);
            for (int bi = 0; bi < custom_count; bi++) {
                int bm_byte = bi / 8, bm_bit = bi % 8;
                bool present = (bitmask_off + bm_byte < (int)bytes.size()) &&
                               (((uint8_t)bytes[bitmask_off + bm_byte] >> bm_bit) & 1);
                if (present) {
                    for (int f = 0; f < frame_count; f++) {
                        int dst = (bi * frame_count + f) * 3;
                        int src = data_pos + f * 6;
                        if (src + 5 < (int)bytes.size()) {
                            plutoAnim.Translations[dst]   = ReadS16(bytes, src);
                            plutoAnim.Translations[dst+1] = ReadS16(bytes, src+2);
                            plutoAnim.Translations[dst+2] = ReadS16(bytes, src+4);
                        }
                    }
                    data_pos += frame_count * 6;
                }
            }
        }

        // Scales (optional: sparse bitmask format, same layout as translations)
        // Format: b'scales' + [2 bytes frame_count][2 bytes bone_count][bitmask][data]
        // Scale encoding: s16 / 1024.0f = scale factor (1024 = 1.0x identity)
        {
            std::size_t scales_pos = 0;
            std::size_t scan_start = (translations_pos > 0) ? translations_pos : indices_pos;
            for (int i = (int)scan_start; i + 6 <= (int)bytes.size(); i++) {
                if (strncmp(bytes.data() + i, "scales", 6) == 0) {
                    scales_pos = i + 6;
                    break;
                }
            }
            if (scales_pos > 0 && scales_pos + 4 <= (int)bytes.size()) {
                int frame_count  = ((uint8_t)bytes[scales_pos]     << 8) | (uint8_t)bytes[scales_pos + 1];
                int custom_count = ((uint8_t)bytes[scales_pos + 2] << 8) | (uint8_t)bytes[scales_pos + 3];
                int bitmask_off  = scales_pos + 4;
                int bitmask_bytes = (custom_count + 7) / 8;
                int data_pos     = bitmask_off + bitmask_bytes;
                plutoAnim.CustomBoneCountScale = custom_count;
                // Default all entries to identity (1024)
                plutoAnim.Scales.assign(custom_count * frame_count * 3, 1024);
                for (int bi = 0; bi < custom_count; bi++) {
                    int bm_byte = bi / 8, bm_bit = bi % 8;
                    bool present = (bitmask_off + bm_byte < (int)bytes.size()) &&
                                   (((uint8_t)bytes[bitmask_off + bm_byte] >> bm_bit) & 1);
                    if (present) {
                        for (int f = 0; f < frame_count; f++) {
                            int dst = (bi * frame_count + f) * 3;
                            int src = data_pos + f * 6;
                            if (src + 5 < (int)bytes.size()) {
                                plutoAnim.Scales[dst]   = ReadS16(bytes, src);
                                plutoAnim.Scales[dst+1] = ReadS16(bytes, src+2);
                                plutoAnim.Scales[dst+2] = ReadS16(bytes, src+4);
                            }
                        }
                        data_pos += frame_count * 6;
                    }
                }
            }
        }

        // I'm sure this can be improved and less complicated so please reach out if you think you can do it!

        plutoAnim.BoneCount = plutoAnim.Indices.size() / 6 - 1;
    }
    return plutoAnim;
}

std::vector<PlutoAnim> GetPAnimList(const std::string& folderPath) {
    std::vector<PlutoAnim> panim_list;

    std::filesystem::create_directory(folderPath);
    if (std::filesystem::exists(folderPath)) {
        convert_count = 0;
        for (const auto & entry : std::filesystem::recursive_directory_iterator(folderPath)) {
            std::filesystem::path path = entry.path();
            
            if (std::filesystem::is_directory(path)) continue;
            if (path.extension().generic_string() == ".panim") {
                PlutoAnim panim;
                panim.FileName = path.filename().generic_string();
                panim.FilePath = path.generic_string();
                panim_list.push_back(panim);
            }
            if (path.extension().generic_string() == ".json" &&
            !std::filesystem::exists(path.parent_path().generic_string() + "/" + path.stem().generic_string() + ".panim")) {
                std::string json_path = path.generic_string();
                std::string panim_path = path.parent_path().generic_string() + "/" + path.stem().generic_string() + ".panim";
                std::cout << "Converting " << json_path << " to " << panim_path << std::endl;
                convert_mcomp_to_panim((char*)json_path.c_str(), (char*)panim_path.c_str());
                if (!std::filesystem::exists(panim_path)) continue;
                PlutoAnim panim;
                panim.FileName = path.stem().generic_string() + ".panim";
                panim.FilePath = panim_path;
                panim_list.push_back(panim);
            }
        }
    }

    if (convert_count > 0) studio_notif_success("JSON -> PAnim", "Converted %d JSON animations to PAnim. Backups saved.", convert_count);

    return panim_list;
}

PlutoAnim ConvertFromVanilla() {
    PlutoAnim plutoAnim;
    struct Animation* vanillaAnim = gMarioStates[0].marioObj->header.gfx.animInfo.curAnim;

    plutoAnim.Name = saturn_animations[selected_anim_index];
    plutoAnim.Author = "Super Mario 64";
    plutoAnim.Length = vanillaAnim->loopEnd - vanillaAnim->loopStart;
    plutoAnim.Looping = (vanillaAnim->flags & ANIM_FLAG_NOLOOP) == 0;
    plutoAnim.Values = std::vector<s16>(vanillaAnim->values, vanillaAnim->values + vanillaAnim->valuesLength);
    plutoAnim.Indices = std::vector<u16>(vanillaAnim->index, vanillaAnim->index + vanillaAnim->indexLength);
    plutoAnim.BoneCount = 20;

    return plutoAnim;
}

std::vector<s16> bone_anim_values;
std::vector<u16> bone_anim_indices;

/* Overwrites the currently played animation with the actively selected PlutoAnim */
void saturn_play_pluto_animation() {
    if (!(override_anim && freeze_camera &&
          !current_pluto_anim.Values.empty() && !current_pluto_anim.Indices.empty()))
        return;

    int bone_count = GetTotalBoneCount() + 1;

    // Send current animation frame into Bones[] so pose editor starts from the correct pose
    if (!is_editing_panim && gMarioStates[0].marioObj) {
        if ((int)current_pluto_anim.Bones.size() < bone_count - 1)
            current_pluto_anim.Bones.resize(bone_count - 1);

        // Restore user bone flags after switching to PAnim (bones may appear/disappear)
        if (s_flags_need_restore) {
            for (int i = 0; i < (int)s_bone_flags.size() && i < (int)current_pluto_anim.Bones.size(); i++) {
                current_pluto_anim.Bones[i].BoneHidden    = s_bone_flags[i].BoneHidden;
                current_pluto_anim.Bones[i].WiggleDisabled = s_bone_flags[i].WiggleDisabled;
                current_pluto_anim.Bones[i].WindDisabled   = s_bone_flags[i].WindDisabled;
            }
            s_flags_need_restore = false;
        }

        struct AnimInfo* anim_info = &gMarioStates[0].marioObj->header.gfx.animInfo;
        const u16* index = current_pluto_anim.Indices.data();
        for (int i = 0; i < bone_count; i++) {
            for (int j = 0; j < 3; j++) {
                int frame  = anim_info->animFrame;
                int offset = (frame < index[0]) ? index[1] + frame : index[1] + index[0] - 1;
                index += 2;
                if (offset >= 0 && offset < (int)current_pluto_anim.Values.size()) {
                    float val = (float)current_pluto_anim.Values[offset] * (i == 0 ? 1.0f : 360.f / 65536.f);
                    if (i == 0) g_root_offset[j] = val;
                    else if (i - 1 < (int)current_pluto_anim.Bones.size()) current_pluto_anim.Bones[i-1].Rotation[j] = val;
                }
            }
            if (i > 0 && i - 1 < (int)current_pluto_anim.Bones.size()) {
                PlutoBone& bone = current_pluto_anim.Bones[i - 1];
                int panim_idx = i - 2;
                vec3s_set(bone.Translation, 0, 0, 0);
                if (panim_idx >= 0 && !current_pluto_anim.Translations.empty()) {
                    int tc = current_pluto_anim.CustomBoneCount;
                    if (tc > 0 && panim_idx < tc) {
                        int fc = (int)current_pluto_anim.Translations.size() / (tc * 3);
                        if (fc > 0) {
                            int f = anim_info->animFrame;
                            f = f < 0 ? 0 : (f >= fc ? fc - 1 : f);
                            int base = (panim_idx * fc + f) * 3;
                            bone.Translation[0] = current_pluto_anim.Translations[base];
                            bone.Translation[1] = current_pluto_anim.Translations[base + 1];
                            bone.Translation[2] = current_pluto_anim.Translations[base + 2];
                        }
                    }
                }
                bone.Scale[0] = bone.Scale[1] = bone.Scale[2] = 1.0f;
                if (panim_idx >= 0 && !current_pluto_anim.Scales.empty()) {
                    int tc = current_pluto_anim.CustomBoneCountScale;
                    if (tc > 0 && panim_idx < tc) {
                        int fc = (int)current_pluto_anim.Scales.size() / (tc * 3);
                        if (fc > 0) {
                            int f = anim_info->animFrame;
                            f = f < 0 ? 0 : (f >= fc ? fc - 1 : f);
                            int base = (panim_idx * fc + f) * 3;
                            bone.Scale[0] = current_pluto_anim.Scales[base]     / 1024.0f;
                            bone.Scale[1] = current_pluto_anim.Scales[base + 1] / 1024.0f;
                            bone.Scale[2] = current_pluto_anim.Scales[base + 2] / 1024.0f;
                        }
                    }
                }
            }
        }
    }

    bone_anim_values.resize(bone_count * 3);
    bone_anim_indices.resize(bone_count * 6);

    for (int i = 0; i < bone_count; i++) {
        for (int j = 0; j < 3; j++) {
            float val = (i == 0) ? g_root_offset[j]
                      : (i - 1 < (int)current_pluto_anim.Bones.size()) ? current_pluto_anim.Bones[i-1].Rotation[j] : 0.0f;
            bone_anim_values[i * 3 + j] = val * (i == 0 ? 1 : 65536 / 360);
        }
    }

    for (int i = 0; i < bone_count; i++) {
        int base = i * 6;
        for (int j = 0; j < 3; j++) {
            bone_anim_indices[base + j * 2]     = 0x0001;
            bone_anim_indices[base + j * 2 + 1] = i * 3 + j;
        }
    }

    static Animation custom_animation;
    custom_animation.flags             = is_editing_panim ? ANIM_FLAG_2 : 0;
    custom_animation.animYTransDivisor = 0;
    custom_animation.startFrame        = 0;
    custom_animation.loopStart         = 0;
    custom_animation.loopEnd           = (s16)current_pluto_anim.Length;
    custom_animation.values            = is_editing_panim ? bone_anim_values.data()   : current_pluto_anim.Values.data();
    custom_animation.valuesLength      = is_editing_panim ? bone_anim_values.size()   : current_pluto_anim.Values.size();
    custom_animation.index             = is_editing_panim ? bone_anim_indices.data()  : current_pluto_anim.Indices.data();
    custom_animation.indexLength       = is_editing_panim ? bone_anim_indices.size()  : current_pluto_anim.Indices.size();
    custom_animation.length            = (s16)current_pluto_anim.Length;
    gMarioStates[0].marioObj->header.gfx.animInfo.curAnim    = &custom_animation;
    gMarioStates[0].marioObj->header.gfx.animInfo.animYTrans = 0xBD;

    struct Object* accessory = find_hat_object();
    if (accessory) {
        accessory->header.gfx.animInfo.curAnim    = &custom_animation;
        accessory->header.gfx.animInfo.animYTrans = 0xBD;
    }
}

/* Returns true if the next custom animation is part of the "Chainer queue" i.e. anim_00.panim, anim_01.panim, anim_02.panim... */
// To be removed for a better implementation
// 5/19/26 probably just gonna remove this entirely and have Pluto merge all files into one
bool saturn_check_for_chainer() {
    if (!enable_custom_anim) return false;
    if (selected_panim_index >= pluto_animations_list.size() - 1) return false;

    if (pluto_animations_list[selected_panim_index].HasQueue() && pluto_animations_list[selected_panim_index+1].HasQueue()) {
        selected_panim_index += 1;
        PreserveBoneFlagsAsPending();
        current_pluto_anim = LoadPAnim(pluto_animations_list[selected_panim_index+1].FilePath);
        gMarioStates[0].marioObj->header.gfx.animInfo.animFrame = 0;
        override_anim = true;
        return true;
    }

    return false;
}

static int s_add_bone_idx = 0;
static int s_vanilla_bone_idx = 0;
static int s_apply_bone_idx = 0;
static int s_translation_lookup_idx = 0;
static int s_scale_apply_idx = 0;
static int s_scale_lookup_idx = 0;
static int s_store_pos_idx = 0;
static std::vector<std::string> s_ActiveBoneNames;

void SaveAndScheduleRestoreBoneFlags() {
    s_bone_flags.resize(current_pluto_anim.Bones.size());
    for (int i = 0; i < (int)current_pluto_anim.Bones.size(); i++)
        s_bone_flags[i] = { current_pluto_anim.Bones[i].BoneHidden,
                            current_pluto_anim.Bones[i].WiggleDisabled,
                            current_pluto_anim.Bones[i].WindDisabled };
    s_flags_need_restore = true;
}

// Feeds the current live bone flags back into s_pending_bone_flags so that
// AddToBoneCountList re-applies them unconditionally on the next frame.
// Use this before any LoadPAnim call to survive the fresh-Bones replacement
void PreserveBoneFlagsAsPending() {
    s_pending_bone_flags.resize(current_pluto_anim.Bones.size(), { false, false, false, false });
    for (int i = 0; i < (int)current_pluto_anim.Bones.size(); i++) {
        const auto& b = current_pluto_anim.Bones[i];
        s_pending_bone_flags[i] = { b.BoneHidden, b.WiggleDisabled, b.WindDisabled,
                                    b.BoneHidden || b.WiggleDisabled || b.WindDisabled };
    }
}

void ClearBoneFlags() {
    s_bone_flags.clear();
    s_flags_need_restore = false;
}

// Clears the in-memory flag state on all bones without touching the config file
// or s_pending_bone_flags, so a subsequent LoadBoneFlagsFromPackDir + model reload
// will re-apply them from disk
void ResetBoneFlagsInMemory() {
    for (auto& bone : current_pluto_anim.Bones) {
        bone.BoneHidden     = false;
        bone.WiggleDisabled = false;
        bone.WindDisabled   = false;
    }
}

void SetActiveBoneNames(const std::vector<std::string>& names) {
    s_ActiveBoneNames = names;
    ClearBoneFlags(); // new model loaded - bone flags from previous model no longer apply
}

void LoadBoneFlagsFromPackDir(const std::string& pack_path) {
    s_pending_bone_flags.clear();
    std::string path = pack_path + "/bone_flags.cfg";
    std::ifstream file(path);
    if (!file.is_open()) return;
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        int idx, hidden, wiggle, wind;
        if (sscanf(line.c_str(), "%d %d %d %d", &idx, &hidden, &wiggle, &wind) == 4 && idx >= 0) {
            if (idx >= (int)s_pending_bone_flags.size())
                s_pending_bone_flags.resize(idx + 1, { false, false, false, false });
            s_pending_bone_flags[idx] = { (bool)hidden, (bool)wiggle, (bool)wind, true };
        }
    }
}

void SaveBoneFlagsToPackDir(const std::string& pack_path) {
    std::string path = pack_path + "/bone_flags.cfg";
    bool any = false;
    for (const auto& b : current_pluto_anim.Bones)
        if (b.BoneHidden || b.WiggleDisabled || b.WindDisabled) { any = true; break; }
    if (!any) {
        std::filesystem::remove(path);
        return;
    }
    std::ofstream file(path);
    if (!file.is_open()) return;
    file << "# Pluto bone flags - bone_index hidden wiggle_disabled wind_disabled\n";
    for (int i = 0; i < (int)current_pluto_anim.Bones.size(); i++) {
        const auto& b = current_pluto_anim.Bones[i];
        if (b.BoneHidden || b.WiggleDisabled || b.WindDisabled)
            file << i << " " << (int)b.BoneHidden << " " << (int)b.WiggleDisabled << " " << (int)b.WindDisabled << "\n";
    }
}

void AddToBoneCountList(bool is_custom, bool is_wiggle, Vec3s rotation) {
    int idx = s_add_bone_idx++;
    int vidx = is_custom ? -1 : s_vanilla_bone_idx++;
    std::string customName = (idx < (int)s_ActiveBoneNames.size()) ? s_ActiveBoneNames[idx] : "";
    if (idx < (int)current_pluto_anim.Bones.size()) {
        current_pluto_anim.Bones[idx].Index = idx;
        current_pluto_anim.Bones[idx].VanillaIndex = vidx;
        current_pluto_anim.Bones[idx].IsCustom = is_custom;
        current_pluto_anim.Bones[idx].IsWiggle = is_wiggle;
        current_pluto_anim.Bones[idx].CustomName = customName;
        // Apply persistent flags loaded from file (one-shot per model load)
        if (idx < (int)s_pending_bone_flags.size() && s_pending_bone_flags[idx].valid) {
            current_pluto_anim.Bones[idx].BoneHidden    = s_pending_bone_flags[idx].BoneHidden;
            current_pluto_anim.Bones[idx].WiggleDisabled = s_pending_bone_flags[idx].WiggleDisabled;
            current_pluto_anim.Bones[idx].WindDisabled   = s_pending_bone_flags[idx].WindDisabled;
            s_pending_bone_flags[idx].valid = false;
        }
        // BoneHidden, WiggleDisabled, WindDisabled are otherwise preserved (bone existed)
    } else {
        PlutoBone bone;
        bone.Index = idx;
        bone.VanillaIndex = vidx;
        bone.IsCustom = is_custom;
        bone.IsWiggle = is_wiggle;
        bone.CustomName = customName;
        // Apply persistent flags loaded from file (one-shot per model load)
        if (idx < (int)s_pending_bone_flags.size() && s_pending_bone_flags[idx].valid) {
            bone.BoneHidden    = s_pending_bone_flags[idx].BoneHidden;
            bone.WiggleDisabled = s_pending_bone_flags[idx].WiggleDisabled;
            bone.WindDisabled   = s_pending_bone_flags[idx].WindDisabled;
            s_pending_bone_flags[idx].valid = false;
        }
        current_pluto_anim.Bones.push_back(std::move(bone));
    }
}

void ResetBoneCountList() {
    if (!is_editing_panim && s_add_bone_idx > 0) {
        if ((int)current_pluto_anim.Bones.size() > s_add_bone_idx)
            current_pluto_anim.Bones.resize(s_add_bone_idx);
    }
    s_add_bone_idx = 0;
    s_vanilla_bone_idx = 0;
    s_apply_bone_idx = 0;
    s_translation_lookup_idx = 0;
    s_scale_apply_idx = 0;
    s_scale_lookup_idx = 0;
    s_store_pos_idx = 0;
}

// Unholy anmount of helper functions we feed to the graph node

bool SaturnShouldApplyBoneTranslation(void) {
    return is_editing_panim || (override_anim && enable_custom_anim && !current_pluto_anim.Translations.empty());
}

bool SaturnShouldApplyBoneScale(void) {
    return is_editing_panim || (override_anim && enable_custom_anim && !current_pluto_anim.Scales.empty());
}

bool SaturnIsEditingPAnim(void) {
    return is_editing_panim;
}

void BumpBoneTranslationCounter(void) {
    s_apply_bone_idx++;
}

void BumpBoneScaleCounter(void) {
    s_scale_apply_idx++;
}

void ResetBoneApplyCounters(void) {
    s_apply_bone_idx = 0;
    s_translation_lookup_idx = 0;
    s_scale_apply_idx = 0;
    s_scale_lookup_idx = 0;
}

extern s16 gPrevAnimFrame;

void ApplyBoneTranslation(Vec3s out, Vec3s out_prev) {
    int idx = s_apply_bone_idx++;
    vec3s_set(out, 0, 0, 0);
    vec3s_set(out_prev, 0, 0, 0);

    // Bone 0 (root) translation is driven by the SM64 animation system directly
    if (idx == 0) return;

    // Pose editor: use static per-bone translation
    if (is_editing_panim) {
        // idx=0 already returned early; Bones[N] corresponds to animated part N (same traversal order)
        if (idx < (int)current_pluto_anim.Bones.size()) {
            vec3s_copy(out, current_pluto_anim.Bones[idx].Translation);
            vec3s_copy(out_prev, current_pluto_anim.Bones[idx].Translation);
        }
        return;
    }

    if (!override_anim || current_pluto_anim.Translations.empty()) return;

    // Use a dedicated lookup counter that only advances in the IF (in-bounds) path.
    // BumpBoneTranslationCounter (ELSE path) only advances s_apply_bone_idx, not this.
    int bone_idx = s_translation_lookup_idx++;
    int total_count = current_pluto_anim.CustomBoneCount;
    if (total_count <= 0 || bone_idx >= total_count) return;

    int frame_count = (int)current_pluto_anim.Translations.size() / (total_count * 3);
    if (frame_count <= 0) return;

    auto clamp_frame = [&](int f) { return f < 0 ? 0 : (f >= frame_count ? frame_count - 1 : f); };
    auto read_trans = [&](int bone, int f, Vec3s dst) {
        int b = (bone * frame_count + clamp_frame(f)) * 3;
        dst[0] = current_pluto_anim.Translations[b];
        dst[1] = current_pluto_anim.Translations[b + 1];
        dst[2] = current_pluto_anim.Translations[b + 2];
    };

    if (g_saturn_anim_blend_t >= 0.0f) {
        Vec3s t0, t1;
        read_trans(bone_idx, g_saturn_floor_frame, t0);
        read_trans(bone_idx, g_saturn_ceil_frame,  t1);
        float frac = g_saturn_anim_blend_t;
        out[0] = (s16)(t0[0] + (t1[0] - t0[0]) * frac);
        out[1] = (s16)(t0[1] + (t1[1] - t0[1]) * frac);
        out[2] = (s16)(t0[2] + (t1[2] - t0[2]) * frac);
        if (g_saturn_prev_blend_t >= 0.0f) {
            Vec3s p0, p1;
            read_trans(bone_idx, g_saturn_prev_floor_frame, p0);
            read_trans(bone_idx, g_saturn_prev_ceil_frame,  p1);
            float pfrac = g_saturn_prev_blend_t;
            out_prev[0] = (s16)(p0[0] + (p1[0] - p0[0]) * pfrac);
            out_prev[1] = (s16)(p0[1] + (p1[1] - p0[1]) * pfrac);
            out_prev[2] = (s16)(p0[2] + (p1[2] - p0[2]) * pfrac);
        } else {
            vec3s_copy(out_prev, out);
        }
        return;
    }

    int frame = gMarioStates[0].marioObj->header.gfx.animInfo.animFrame;
    frame = frame < 0 ? 0 : (frame >= frame_count ? frame_count - 1 : frame);
    int base = (bone_idx * frame_count + frame) * 3;
    out[0] = current_pluto_anim.Translations[base];
    out[1] = current_pluto_anim.Translations[base + 1];
    out[2] = current_pluto_anim.Translations[base + 2];

    int prev_frame = gPrevAnimFrame;
    prev_frame = prev_frame < 0 ? 0 : (prev_frame >= frame_count ? frame_count - 1 : prev_frame);
    int prev_base = (bone_idx * frame_count + prev_frame) * 3;
    out_prev[0] = current_pluto_anim.Translations[prev_base];
    out_prev[1] = current_pluto_anim.Translations[prev_base + 1];
    out_prev[2] = current_pluto_anim.Translations[prev_base + 2];
}

void StoreBoneWorldPosition(Vec3f pos) {
    int idx = s_store_pos_idx++;
    if (idx >= (int)current_pluto_anim.Bones.size()) return;
    vec3f_copy(current_pluto_anim.Bones[idx].WorldPosition, pos);
}

void ApplyBoneScale(Vec3f out, Vec3f out_prev) {
    int idx = s_scale_apply_idx++;
    out[0] = out[1] = out[2] = 1.0f;
    out_prev[0] = out_prev[1] = out_prev[2] = 1.0f;
    // Bone 0 (root) scale not supported
    if (idx == 0) return;

    // Pose editor: use static per-bone scale
    if (is_editing_panim) {
        // idx=0 already returned early; Bones[N] corresponds to animated part N (same traversal order)
        if (idx < (int)current_pluto_anim.Bones.size()) {
            out[0] = current_pluto_anim.Bones[idx].Scale[0];
            out[1] = current_pluto_anim.Bones[idx].Scale[1];
            out[2] = current_pluto_anim.Bones[idx].Scale[2];
            out_prev[0] = out[0];
            out_prev[1] = out[1];
            out_prev[2] = out[2];
        }
        return;
    }

    if (!override_anim || current_pluto_anim.Scales.empty()) return;

    int bone_idx = s_scale_lookup_idx++;
    int total_count = current_pluto_anim.CustomBoneCountScale;
    if (total_count <= 0 || bone_idx >= total_count) return;

    int frame_count = (int)current_pluto_anim.Scales.size() / (total_count * 3);
    if (frame_count <= 0) return;

    auto clamp_frame = [&](int f) { return f < 0 ? 0 : (f >= frame_count ? frame_count - 1 : f); };
    auto read_scale = [&](int bone, int f, Vec3f dst) {
        int b = (bone * frame_count + clamp_frame(f)) * 3;
        dst[0] = current_pluto_anim.Scales[b]     / 1024.0f;
        dst[1] = current_pluto_anim.Scales[b + 1] / 1024.0f;
        dst[2] = current_pluto_anim.Scales[b + 2] / 1024.0f;
    };

    if (g_saturn_anim_blend_t >= 0.0f) {
        Vec3f s0, s1;
        read_scale(bone_idx, g_saturn_floor_frame, s0);
        read_scale(bone_idx, g_saturn_ceil_frame,  s1);
        float frac = g_saturn_anim_blend_t;
        out[0] = s0[0] + (s1[0] - s0[0]) * frac;
        out[1] = s0[1] + (s1[1] - s0[1]) * frac;
        out[2] = s0[2] + (s1[2] - s0[2]) * frac;
        if (g_saturn_prev_blend_t >= 0.0f) {
            Vec3f p0, p1;
            read_scale(bone_idx, g_saturn_prev_floor_frame, p0);
            read_scale(bone_idx, g_saturn_prev_ceil_frame,  p1);
            float pfrac = g_saturn_prev_blend_t;
            out_prev[0] = p0[0] + (p1[0] - p0[0]) * pfrac;
            out_prev[1] = p0[1] + (p1[1] - p0[1]) * pfrac;
            out_prev[2] = p0[2] + (p1[2] - p0[2]) * pfrac;
        } else {
            out_prev[0] = out[0]; out_prev[1] = out[1]; out_prev[2] = out[2];
        }
        return;
    }

    int frame = gMarioStates[0].marioObj->header.gfx.animInfo.animFrame;
    frame = frame < 0 ? 0 : (frame >= frame_count ? frame_count - 1 : frame);
    int base = (bone_idx * frame_count + frame) * 3;
    out[0] = current_pluto_anim.Scales[base]     / 1024.0f;
    out[1] = current_pluto_anim.Scales[base + 1] / 1024.0f;
    out[2] = current_pluto_anim.Scales[base + 2] / 1024.0f;

    int prev_frame = gPrevAnimFrame;
    prev_frame = prev_frame < 0 ? 0 : (prev_frame >= frame_count ? frame_count - 1 : prev_frame);
    int prev_base = (bone_idx * frame_count + prev_frame) * 3;
    out_prev[0] = current_pluto_anim.Scales[prev_base]     / 1024.0f;
    out_prev[1] = current_pluto_anim.Scales[prev_base + 1] / 1024.0f;
    out_prev[2] = current_pluto_anim.Scales[prev_base + 2] / 1024.0f;
}

void CacheBoneCount() {
    // Reset bone-add counter for next frame; trim Bones[] to actual geo count if it grew larger.
    // Do NOT touch current_pluto_anim.BoneCount - that value comes from the animation file and
    // is used by ExtraBoneInBounds to decide how many custom bones this animation handles.
    if (!is_editing_panim && s_add_bone_idx > 0) {
        if ((int)current_pluto_anim.Bones.size() > s_add_bone_idx)
            current_pluto_anim.Bones.resize(s_add_bone_idx);
    }
    s_add_bone_idx = 0;
    s_vanilla_bone_idx = 0;
}

int GetTotalBoneCount() {
    if (pause_anim) {
        int n = (int)current_pluto_anim.Bones.size();
        if (n > 0) return n;
    }
    return current_pluto_anim.BoneCount + 1;
}

bool ExtraBoneInBounds(int index) {
    if (is_editing_panim) return true;
    // For custom animations, only allow processing extra bones if we haven't exceeded 
    // the number of bones that this specific PlutoAnim is designed to handle
    return index <= current_pluto_anim.BoneCount - 20;
}

/* Returns the hidden state for the bone most recently registered via AddToBoneCountList.
 * Called from the renderer immediately after AddToBoneCountList for any bone type. */
bool SaturnCurrentBoneIsHidden(void) {
    int idx = s_add_bone_idx - 1;
    if (idx < 0 || idx >= (int)current_pluto_anim.Bones.size()) return false;
    return current_pluto_anim.Bones[idx].BoneHidden;
}

bool SaturnCurrentBoneWiggleDisabled(void) {
    int idx = s_add_bone_idx - 1;
    if (idx < 0 || idx >= (int)current_pluto_anim.Bones.size()) return false;
    return current_pluto_anim.Bones[idx].WiggleDisabled;
}

bool SaturnCurrentBoneWindDisabled(void) {
    int idx = s_add_bone_idx - 1;
    if (idx < 0 || idx >= (int)current_pluto_anim.Bones.size()) return false;
    return current_pluto_anim.Bones[idx].WindDisabled;
}

void SaturnGetCurrentBonePoseRotation(Vec3s out, Vec3s out_prev) {
    vec3s_set(out,      0, 0, 0);
    vec3s_set(out_prev, 0, 0, 0);
    if (!is_editing_panim) return;
    int idx = s_add_bone_idx - 1;
    if (idx < 0 || idx >= (int)current_pluto_anim.Bones.size()) return;
    const float* rot = current_pluto_anim.Bones[idx].Rotation;
    s16 r0 = (s16)(rot[0] * (65536.f / 360.f));
    s16 r1 = (s16)(rot[1] * (65536.f / 360.f));
    s16 r2 = (s16)(rot[2] * (65536.f / 360.f));
    vec3s_set(out,      r0, r1, r2);
    vec3s_set(out_prev, r0, r1, r2);
}

/* Automatically pushes custom bone values down when entering pose editor */
void AutoPushCustomBones() {
    if (current_pluto_anim.Bones.size() <= 0) return;
    
    int bone_count = GetTotalBoneCount() + 1;

    int custom_bone_count = 0;
    for (const auto& bone : current_pluto_anim.Bones) {
        if (bone.IsCustom) {
            custom_bone_count++;
        }
    }
    
    // If we have no custom bones, nothing to push
    if (custom_bone_count == 0) return;
    
    int default_bone_count = 20;
    int bones_to_push = custom_bone_count - (current_pluto_anim.BoneCount - default_bone_count);
    if (bones_to_push <= 0) return;
    
    // Find and push down the first N custom bones (where N = bones_to_push)
    int pushed_count = 0;
    for (int i = 1; i < bone_count && pushed_count < bones_to_push; i++) {
        bool is_custom_bone = i < (int)current_pluto_anim.Bones.size() + 1 && current_pluto_anim.Bones[i-1].IsCustom;
        
        if (is_custom_bone) {
            // Perform push down operation for this custom bone
            for (int j = bone_count - 1; j > i; j--) {
                current_pluto_anim.Bones[j-1].Rotation[0] = current_pluto_anim.Bones[j-2].Rotation[0];
                current_pluto_anim.Bones[j-1].Rotation[1] = current_pluto_anim.Bones[j-2].Rotation[1];
                current_pluto_anim.Bones[j-1].Rotation[2] = current_pluto_anim.Bones[j-2].Rotation[2];
            }
            // Reset current bone to 0
            current_pluto_anim.Bones[i-1].Rotation[0] = 0.0f;
            current_pluto_anim.Bones[i-1].Rotation[1] = 0.0f;
            current_pluto_anim.Bones[i-1].Rotation[2] = 0.0f;
            
            pushed_count++;
        }
    }

    // Sync translation data into Bones[].Translation using the same two-step approach
    // as Rotation above: fill sequentially first, then push-down custom bone slots.
    if (!current_pluto_anim.Translations.empty() && current_pluto_anim.CustomBoneCount > 0) {
        int frame_count = (int)current_pluto_anim.Translations.size() / (current_pluto_anim.CustomBoneCount * 3);
        int frame = 0;
        if (gMarioStates[0].marioObj) {
            frame = gMarioStates[0].marioObj->header.gfx.animInfo.animFrame;
            frame = frame < 0 ? 0 : (frame >= frame_count ? frame_count - 1 : frame);
        }
        // Step 1: fill sequentially - T[bi] maps to Bones[bi+1], same order as the PAnim player
        for (int bi = 0; bi < (int)current_pluto_anim.Bones.size() - 1; bi++) {
            if (bi < current_pluto_anim.CustomBoneCount) {
                int base = (bi * frame_count + frame) * 3;
                current_pluto_anim.Bones[bi + 1].Translation[0] = current_pluto_anim.Translations[base];
                current_pluto_anim.Bones[bi + 1].Translation[1] = current_pluto_anim.Translations[base + 1];
                current_pluto_anim.Bones[bi + 1].Translation[2] = current_pluto_anim.Translations[base + 2];
            } else {
                vec3s_set(current_pluto_anim.Bones[bi + 1].Translation, 0, 0, 0);
            }
        }
        // Step 2: apply same push-down as Rotation above - shift custom bone slots down
        int pushed_trans = 0;
        for (int i = 1; i < bone_count && pushed_trans < bones_to_push; i++) {
            bool is_custom_bone = i < (int)current_pluto_anim.Bones.size() + 1 && current_pluto_anim.Bones[i-1].IsCustom;
            if (is_custom_bone) {
                for (int j = bone_count - 1; j > i; j--) {
                    vec3s_copy(current_pluto_anim.Bones[j-1].Translation, current_pluto_anim.Bones[j-2].Translation);
                }
                vec3s_set(current_pluto_anim.Bones[i-1].Translation, 0, 0, 0);
                pushed_trans++;
            }
        }
    }
}

