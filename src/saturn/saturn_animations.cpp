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
std::vector<Vec3fWrapper> bone_rotations;

s16 ReadS16(std::vector<char> data, int index) {
    return ((unsigned int)(unsigned char)data[index] << 8) | (unsigned int)(unsigned char)data[index + 1];
}

/* Loads a PlutoAnim struct from a given filepath */
PlutoAnim LoadPAnim(std::string filePath) {
    PlutoAnim plutoAnim;
    if (pluto_animations_list.size() <= 0) return plutoAnim;

    std::ifstream input(filePath, std::ios::binary);

    std::vector<char> bytes(
        (std::istreambuf_iterator<char>(input)),
        (std::istreambuf_iterator<char>()));
    input.close();

    if (bytes.size() > 0) {
        // Metadata
        for (int i = 0x00; i < 0x40; i++) {plutoAnim.Name += bytes[i];}
        for (int j = 0x40; j < 0x60; j++) {plutoAnim.Author += bytes[j];}
        
        int length1 = (uint8_t)bytes[0x61];
        int length2 = (uint8_t)bytes[0x62];
        plutoAnim.Length = (length1<<8)|(length2);

        plutoAnim.Looping = (uint8_t)bytes[0x60] != 0x00;
        plutoAnim.Nodes = (uint8_t)bytes[0x63];

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
        for (int i = indices_pos; i < bytes.size(); i += 2) {
            plutoAnim.Indices.push_back(ReadS16(bytes, i));
        }

        plutoAnim.BoneCount = plutoAnim.Indices.size() / 6 - 1;
    }
    return plutoAnim;
}

std::vector<PlutoAnim> GetPAnimList(std::string folderPath) {
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
    plutoAnim.Values = std::vector<s16>(vanillaAnim->values, vanillaAnim->values + vanillaAnim->length);
    plutoAnim.Indices = std::vector<u16>(vanillaAnim->index, vanillaAnim->index + vanillaAnim->length);
    plutoAnim.BoneCount = 20;

    return plutoAnim;
}

std::vector<s16> bone_anim_values;
std::vector<u16> bone_anim_indices;

/* Overwrites the currently played animation with the actively selected PlutoAnim */
void saturn_play_pluto_animation() {
    if (override_anim && freeze_camera &&
        current_pluto_anim.Values.size() > 0 && current_pluto_anim.Indices.size() > 0) {
            // Get the bone count from current animation
            int bone_count = (current_bone_count.size() > current_pluto_anim.BoneCount ? current_bone_count.size() : current_pluto_anim.BoneCount) + 1;
            
            // Resize bone_rotations vector to match the current animation's bone count
            bone_rotations.resize(bone_count);
            
            // Pose Editor
            if (!is_editing_panim) {
                // Copies the current frame's animation data into the pose editor when launched
                struct AnimInfo* anim_info = &gMarioStates[0].marioObj->header.gfx.animInfo;
                const u16* index = current_pluto_anim.Indices.data();
                for (int i = 0; i < bone_count; i++) {
                    for (int j = 0; j < 3; j++) {
                        int frame = anim_info->animFrame;
                        int offset = 0;
                        if (frame < index[0]) offset = index[1] + frame;
                        else offset = index[1] + index[0] - 1;
                        index += 2;
                        bone_rotations[i][j] = (float)(current_pluto_anim.Values[offset]) * (i == 0 ? 1 : 360.f / 65536);
                    }
                }
            }
            
            // Initialize vectors with proper size based on bone count
            bone_anim_values.resize(bone_count * 3);  // bone_count × 3 components (XYZ)
            bone_anim_indices.resize(bone_count * 6); // bone_count × 6 indices per bone
            
            // Generate values
            for (int i = 0; i < bone_count; i++) {
                for (int j = 0; j < 3; j++) {
                    bone_anim_values[i * 3 + j] = bone_rotations[i][j] * (i == 0 ? 1 : 65536 / 360);
                }
            }
            
            // Generate indices with repeating pattern: 0x0001, bone_index
            for (int i = 0; i < bone_count; i++) {
                int base_index = i * 6;
                for (int j = 0; j < 3; j++) {
                    bone_anim_indices[base_index + j * 2] = 0x0001;
                    bone_anim_indices[base_index + j * 2 + 1] = i * 3 + j;
                }
            }

            // Create the custom animation struct
            static Animation custom_animation;
            custom_animation.flags = is_editing_panim ? ANIM_FLAG_2 : 0;
            custom_animation.animYTransDivisor = 0;
            custom_animation.startFrame = 0;
            custom_animation.loopStart = 0;
            custom_animation.loopEnd = (s16)current_pluto_anim.Length;
            custom_animation.unusedBoneCount = current_pluto_anim.Indices.size() / 6 - 1;
            custom_animation.values = (is_editing_panim) ? bone_anim_values.data() : current_pluto_anim.Values.data();
            custom_animation.valuesLength = current_pluto_anim.Values.size();
            custom_animation.index = (is_editing_panim) ? bone_anim_indices.data() : current_pluto_anim.Indices.data();
            custom_animation.indexLength = current_pluto_anim.Indices.size();
            custom_animation.length = (s16)current_pluto_anim.Length;
            gMarioStates[0].marioObj->header.gfx.animInfo.curAnim = &custom_animation;
            gMarioStates[0].marioObj->header.gfx.animInfo.animYTrans = 0xBD;

            struct Object* accessory = find_hat_object();
            if (accessory != NULL) {
                accessory->header.gfx.animInfo.curAnim = &custom_animation;
                accessory->header.gfx.animInfo.animYTrans = 0xBD;
            }
    }
}

/* Returns true if the next custom animation is part of the "Chainer queue" i.e. anim_00.panim, anim_01.panim, anim_02.panim... */
bool saturn_check_for_chainer() {
    if (!enable_custom_anim) return false;
    if (selected_panim_index >= pluto_animations_list.size() - 1) return false;

    if (pluto_animations_list[selected_panim_index].HasQueue() && pluto_animations_list[selected_panim_index+1].HasQueue()) {
        selected_panim_index += 1;
        current_pluto_anim = LoadPAnim(pluto_animations_list[selected_panim_index+1].FilePath);
        gMarioStates[0].marioObj->header.gfx.animInfo.animFrame = 0;
        override_anim = true;
        return true;
    }

    return false;
}

// index, is_custom
std::vector<std::pair<int, bool>> current_bone_count;
int processed_bones = 0;

void AddToBoneCountList(bool is_custom) {
    for (auto& pair : current_bone_count) {
        if (pair.first == current_bone_count.size()) {
            pair.second = is_custom;
            return;
        }
    }
    current_bone_count.push_back(std::make_pair(current_bone_count.size(), is_custom));
}
void ResetBoneCountList() {
    current_bone_count.clear();
    processed_bones = 0;
}

bool CanProcessExtraBone() {
    if (current_pluto_anim.BoneCount <= 20) return false;
    int max_bones = current_pluto_anim.BoneCount - 20;
    processed_bones++;
    return processed_bones <= max_bones;
}