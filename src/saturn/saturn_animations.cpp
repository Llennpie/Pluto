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

#include "src/saturn/saturn.h"
#include "src/pc/pc_main.h"
extern "C" {
    #include "sm64.h"
    #include "game/mario.h"
    #include "game/level_update.h"
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

std::vector<std::string> pluto_animations_list;

/* Loads a PlutoAnim struct from a given filepath */
PlutoAnim LoadPAnim(std::string filePath) {
    PlutoAnim plutoAnim;

    std::ifstream input(filePath, std::ios::binary);

    std::vector<char> bytes(
        (std::istreambuf_iterator<char>(input)),
        (std::istreambuf_iterator<char>()));
    input.close();

    if (bytes.size() > 0) {
        // Metadata
        for (int i = 0x00; i < 0x20; i++) {plutoAnim.Name += bytes[i];}
        for (int j = 0x20; j < 0x40; j++) {plutoAnim.Author += bytes[j];}
        
        int length1 = (uint8_t)bytes[0x41];
        int length2 = (uint8_t)bytes[0x42];
        plutoAnim.Length = (length1<<8)|(length2);

        plutoAnim.Looping = (uint8_t)bytes[0x40] != 0x00;
        plutoAnim.Nodes = (uint8_t)bytes[0x43];

        std::size_t values_pos = 0x4A;
        std::size_t indices_pos;

        // Values
        std::vector<std::string> values;
        for (int v = values_pos; v < bytes.size(); v++) {
            if (bytes[v] == 'i' && bytes[v+1] == 'n' && bytes[v+2] == 'd' && bytes[v+3] == 'i' && bytes[v+4] == 'c' && bytes[v+5] == 'e' && bytes[v+6] == 's') {
                indices_pos = v + 0x08;
                //plutoAnim.Length = (uint8_t)bytes[indices_pos];
                break;
            }
            if (v % 2 != 0 && v > 0) {
                std::stringstream first, second;
                first << std::setfill('0') << std::setw(2) << std::hex << (0xff & (unsigned int)bytes[v-1]);
                second << std::setfill('0') << std::setw(2) << std::hex << (0xff & (unsigned int)bytes[v]);

                int output = std::stoi(first.str(), 0, 16) * 256 + std::stoi(second.str(), 0, 16);
                values.push_back(std::to_string(output));

                plutoAnim.Values.push_back(output);
            }
        }

        // Indices
        std::vector<std::string> indices;
        for (int i = indices_pos; i < bytes.size(); i++) {
            if (i % 2 == 0 && i > 0) {
                std::stringstream first, second;
                first << std::setfill('0') << std::setw(2) << std::hex << (0xff & (unsigned int)bytes[i-1]);
                second << std::setfill('0') << std::setw(2) << std::hex << (0xff & (unsigned int)bytes[i]);

                int output = std::stoi(first.str(), 0, 16) * 256 + std::stoi(second.str(), 0, 16);
                indices.push_back(std::to_string(output));

                plutoAnim.Indices.push_back(output);
            }
        }
    }

    return plutoAnim;
}

std::vector<std::string> GetPAnimList(std::string folderPath) {
    std::vector<std::string> panim_list;
    
    if (std::filesystem::exists(folderPath)) {
        for (const auto & entry : std::filesystem::directory_iterator(folderPath)) {
            std::filesystem::path path = entry.path();
            
            if (std::filesystem::is_directory(path)) continue;
            if (path.extension().u8string() == ".panim")
                panim_list.push_back(path.filename().u8string());
        }
    }

    return panim_list;
}

/* Overwrites the currently played animation with the actively selected PlutoAnim */
void saturn_play_pluto_animation() {
    PlutoAnim plutoAnim = LoadPAnim("dynos/anims/" + pluto_animations_list[selected_panim_index]);
    if (override_anim &&
    plutoAnim.Values.size() > 0 && plutoAnim.Indices.size() > 0) {
        // Overwrite values
        gMarioStates[0].animation->targetAnim->flags = 0;
        gMarioStates[0].animation->targetAnim->animYTransDivisor = 0;
        gMarioStates[0].animation->targetAnim->startFrame = 0;
        gMarioStates[0].animation->targetAnim->loopStart = 0;
        gMarioStates[0].animation->targetAnim->loopEnd = (s16)plutoAnim.Length;
        gMarioStates[0].animation->targetAnim->unusedBoneCount = plutoAnim.Indices.size() / 6 - 1;
        gMarioStates[0].animation->targetAnim->values = plutoAnim.Values.data();
        gMarioStates[0].animation->targetAnim->index = plutoAnim.Indices.data();
        gMarioStates[0].animation->targetAnim->length = 0;
        gMarioStates[0].marioObj->header.gfx.animInfo.curAnim = gMarioStates[0].animation->targetAnim;
    }
}