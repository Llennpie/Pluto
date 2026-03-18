#include "saturn.h"
#include <iostream>
#include <string>
#include <vector>
#include <SDL2/SDL.h>

#include "pc/djui/djui.h"
#include "pc/djui/djui_chat_box.h"
#include "pc/djui/djui_console.h"
#include "saturn/saturn_animations.h"
#include "saturn/ui/saturn_imgui_animations.h"
extern "C" {
    #include "sm64.h"
    #include "game/camera.h"
    #include "game/mario.h"
    #include "game/game_init.h"
    #include "game/level_update.h"
    #include "engine/math_util.h"
    #include "game/object_helpers.h"
    #include "audio/external.h"
}

bool freeze_camera;
float freeze_camera_speed = 1.f;
float camera_follow_speed = 1.f;
float wiggle_intensity = 1.0f;
bool wind_enabled = false;
float wind_angle = 0.f;
float wind_strength = 1.0f;
float wind_sway = 1.f;
bool wiggle_bone_detected = false;
bool enable_hud;
bool enable_torso_rotation = true;
int head_rotation[2] = { 0, 0 };
bool enable_head_rotation;
bool enable_model_particles;
float face_angle;

bool custom_eyes;
std::vector<std::string> model_expressions_list;

int switch_state_eyes;
int switch_state_hand_right;
int switch_state_hand_left;
int switch_state_cap;
int switch_state_powerup;
int vanish_transparency = 128;

int active_saturn_model_index = -1;

bool override_anim;
int selected_anim_index = MARIO_ANIM_BREAKDANCE;
int selected_panim_index = 0;
bool pause_anim;
int paused_frame;
bool hang_anim;
bool loop_anim;
bool enable_custom_anim;

bool bone_count_matches;

bool is_spinning;
float spinning_speed = 1.f;
int player_speed = 127;
int walkpoint_speed = 127;

/* Returns false when an ImGui text widget is active (i.e. the user is editing text). */
bool allow_game_input;

extern struct Animation *gCurAnim;

/* "Machinima Camera", an extended freeze camera function that allows for free/fly camera and C-Up. */
int saturn_camera_update() {
    if (freeze_camera) {
        fade_volume_scale(SEQ_PLAYER_LEVEL, 0, 6);

        gLakituState.posHSpeed = 32.f;
        gLakituState.posVSpeed = 32.f;
        gLakituState.focHSpeed = 32.f;
        gLakituState.focVSpeed = 32.f;

        // Cancel input when another UI is present
        // This includes DJUI's menu/chat/console, and text inputs in Saturn's UI (i.e. CC editor GameShark)
        if (gDjuiInMainMenu || gDjuiChatBoxFocus || gDjuiConsoleFocus || !allow_game_input) return CAM_FROZEN;

        if (!SDL_GetKeyboardState(NULL)[SDL_SCANCODE_R]) {
            // Movement
            if (SDL_GetKeyboardState(NULL)[SDL_SCANCODE_Y]) {
                gCamera->pos[0] += sins(gCamera->yaw + atan2s(-127, 0)) * 12 * freeze_camera_speed;
                gCamera->pos[2] += coss(gCamera->yaw + atan2s(-127, 0)) * 12 * freeze_camera_speed;
                gCamera->focus[0] += sins(gCamera->yaw + atan2s(-127, 0)) * 12 * freeze_camera_speed;
                gCamera->focus[2] += coss(gCamera->yaw + atan2s(-127, 0)) * 12 * freeze_camera_speed;
            } else if (SDL_GetKeyboardState(NULL)[SDL_SCANCODE_H]) {
                gCamera->pos[0] -= sins(gCamera->yaw + atan2s(-127, 0)) * 12 * freeze_camera_speed;
                gCamera->pos[2] -= coss(gCamera->yaw + atan2s(-127, 0)) * 12 * freeze_camera_speed;
                gCamera->focus[0] -= sins(gCamera->yaw + atan2s(-127, 0)) * 12 * freeze_camera_speed;
                gCamera->focus[2] -= coss(gCamera->yaw + atan2s(-127, 0)) * 12 * freeze_camera_speed;
            }
            if (SDL_GetKeyboardState(NULL)[SDL_SCANCODE_G]) {
                gCamera->pos[0] -= sins(gCamera->yaw + atan2s(0, 127)) * 12 * freeze_camera_speed;
                gCamera->pos[2] -= coss(gCamera->yaw + atan2s(0, 127)) * 12 * freeze_camera_speed;
                gCamera->focus[0] -= sins(gCamera->yaw + atan2s(0, 127)) * 12 * freeze_camera_speed;
                gCamera->focus[2] -= coss(gCamera->yaw + atan2s(0, 127)) * 12 * freeze_camera_speed;
            } else if (SDL_GetKeyboardState(NULL)[SDL_SCANCODE_J]) {
                gCamera->pos[0] += sins(gCamera->yaw + atan2s(0, 127)) * 12 * freeze_camera_speed;
                gCamera->pos[2] += coss(gCamera->yaw + atan2s(0, 127)) * 12 * freeze_camera_speed;
                gCamera->focus[0] += sins(gCamera->yaw + atan2s(0, 127)) * 12 * freeze_camera_speed;
                gCamera->focus[2] += coss(gCamera->yaw + atan2s(0, 127)) * 12 * freeze_camera_speed;
            }
        } else {
            // Rotation
            f32 dist;
            s16 pitch, yaw;
            vec3f_get_dist_and_angle(gCamera->pos, gCamera->focus, &dist, &pitch, &yaw);

            if (SDL_GetKeyboardState(NULL)[SDL_SCANCODE_Y] && pitch < 12000)
                pitch += (1.0f / 1.5f) * 512 * freeze_camera_speed;
            if (SDL_GetKeyboardState(NULL)[SDL_SCANCODE_H] && pitch > -12000)
                pitch -= (1.0f / 1.5f) * 512 * freeze_camera_speed;
            if (SDL_GetKeyboardState(NULL)[SDL_SCANCODE_G])
                yaw += (1.0f / 1.5f) * 512 * freeze_camera_speed;
            if (SDL_GetKeyboardState(NULL)[SDL_SCANCODE_J])
                yaw -= (1.0f / 1.5f) * 512 * freeze_camera_speed;

            vec3f_set_dist_and_angle(gCamera->pos, gCamera->focus, dist, pitch, yaw);
        }

        // Ascend/Descend
        f32 yvel;
        if (SDL_GetKeyboardState(NULL)[SDL_SCANCODE_T])
            yvel += 5.f * freeze_camera_speed;
        else if (SDL_GetKeyboardState(NULL)[SDL_SCANCODE_U])
            yvel -= 5.f * freeze_camera_speed;

        gCamera->pos[1] += yvel;
        gCamera->focus[1] += yvel;
        yvel = approach_f32_symmetric(yvel, 0.f, 2.f);
        yvel = approach_f32_asymptotic(yvel, 0.f, 0.1f);

        // Misc. Animation
        if (gMarioStates[0].marioObj != NULL) {
            if (!pause_anim) paused_frame = gMarioStates[0].marioObj->header.gfx.animInfo.animFrame;
            if (gMarioStates[0].forwardVel != 0.f && (!pause_anim || enable_custom_anim)) override_anim = false;

            if (is_spinning) gMarioState->faceAngle[1] += (s16)(spinning_speed * 15 * 182.04f);
        }

        return CAM_FROZEN;
    } else {
        fade_volume_scale(SEQ_PLAYER_LEVEL, 255, 60);

        if (gLakituState.posHSpeed > 0) gLakituState.posHSpeed = 0.3f * camera_follow_speed;
        if (gLakituState.posVSpeed > 0) gLakituState.posVSpeed = 0.3f * camera_follow_speed;
        if (gLakituState.focHSpeed > 0) gLakituState.focHSpeed = 0.8f * camera_follow_speed;
        if (gLakituState.focVSpeed > 0) gLakituState.focVSpeed = 0.3f * camera_follow_speed;
    }

    return CAM_NORMAL;
}

CharacterAnimID get_idle_anim(struct MarioState *m) {
    if (m->prevAction != ACT_IDLE) {
        m->actionState = 2;
        m->prevAction = ACT_IDLE;
    }

    if (m->action & ACT_FLAG_SWIMMING) return CHAR_ANIM_WATER_IDLE;
    if (m->heldObj != NULL) return CHAR_ANIM_IDLE_WITH_LIGHT_OBJ;
    else {
        if (!enable_head_rotation) return CHAR_ANIM_FIRST_PERSON;
        if (is_anim_at_end(m)) {
            if (m->actionState < 2) m->actionState++;
            else m->actionState = 0;
        }
        switch (m->actionState) {
            case 0: return CHAR_ANIM_IDLE_HEAD_LEFT; break;
            case 1: return CHAR_ANIM_IDLE_HEAD_RIGHT; break;
            case 2: return CHAR_ANIM_IDLE_HEAD_CENTER; break;
            default: return CHAR_ANIM_FIRST_PERSON; break;
        }
    }
}

/* Custom Mario action, active when the player is idle and in machinima mode. */
void saturn_action_idle(struct MarioState *m) {

    if (!(enable_custom_anim && override_anim)) force_set_character_animation(m, (override_anim) ? selected_anim_index : get_idle_anim(m));
    if (m->marioObj == NULL) return;

    struct Animation *targetAnim = m->marioObj->header.gfx.animInfo.curAnim;
    // Animation-specific looping
    if (override_anim) {
        if (loop_anim && !hang_anim) m->animation->targetAnim->flags &= ~ANIM_FLAG_NOLOOP;
        else m->animation->targetAnim->flags |= ANIM_FLAG_NOLOOP;
    }
    bool targetAnimLooping = (m->animation->targetAnim->flags & ANIM_FLAG_NOLOOP) == 0;

    // Flip-flop
    if (override_anim) {
        if (pause_anim) {
            m->marioObj->header.gfx.animInfo.animFrame = paused_frame;
            // If the animation goes OOB
            if (paused_frame > targetAnim->loopEnd-1) paused_frame = targetAnim->loopEnd-1;
            if (paused_frame < targetAnim->loopStart) paused_frame = targetAnim->loopStart;
        } else {
            if (is_anim_at_end(m)) {
                if (hang_anim) { pause_anim = true; paused_frame = m->marioObj->header.gfx.animInfo.animFrame; }
                else if (!targetAnimLooping) {
                    if (saturn_check_for_chainer() == false) {
                        override_anim = false;
                        set_mario_animation(m, MARIO_ANIM_START_CROUCHING);
                    }
                }
            }
        }
    } else {
        pause_anim = false;
        is_editing_panim = false;
        g_root_offset[0] = g_root_offset[1] = g_root_offset[2] = 0.0f;
    }

    // Check if the model's bone count matches the current PlutoAnim's bone count
    // This determines if we animate MCOMP extra bones or not
    bone_count_matches = current_pluto_anim.BoneCount > 20;
}