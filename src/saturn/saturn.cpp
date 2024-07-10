#include "saturn.h"
#include <iostream>
#include <string>
#include <vector>
#include <SDL2/SDL.h>

#include "pc/djui/djui.h"
#include "pc/djui/djui_chat_box.h"
#include "pc/djui/djui_console.h"
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
float camera_fov = 45.f;
bool enable_hud;
bool enable_shadows = true;
bool enable_torso_rotation = true;
bool enable_head_rotation;
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
bool pause_anim;
int paused_frame;
bool hang_anim;
bool loop_anim;

int player_speed = 127;
int walkpoint_speed = 127;

/* Returns false when an ImGui text widget is active (i.e. the user is editing text). */
bool allow_game_input;

/* "Machinima Camera", an extended freeze camera function that allows for free/fly camera and C-Up. */
int saturn_camera_update() {
    if (freeze_camera) {
        fade_volume_scale(SEQ_PLAYER_LEVEL, 0, 6);

        // Cancel input when another UI is present
        // This includes DJUI's menu/chat/console, and text inputs in Saturn's UI (i.e. CC editor GameShark)
        if (gDjuiInMainMenu || gDjuiChatBoxFocus || gDjuiConsoleFocus || !allow_game_input) return CAM_FROZEN;

        if (!SDL_GetKeyboardState(NULL)[SDL_SCANCODE_R]) {
            // Rotation
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
            // Movement
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
            if (gMarioStates[0].forwardVel != 0.f && !pause_anim) override_anim = false;
        }

        return CAM_FROZEN;
    } else fade_volume_scale(SEQ_PLAYER_LEVEL, 255, 60);

    return CAM_NORMAL;
}

/* Custom Mario action, active when the player is idle and in machinima mode.
To-do: Cycle animations via the mixtape */
void saturn_action_idle(struct MarioState *m) {
    set_character_animation(m, (override_anim) ? selected_anim_index : CHAR_ANIM_FIRST_PERSON);
    if (m->marioObj == NULL) return;

    // Spin/Angle
    walkpoint_speed = (m->action & ACT_FLAG_MOVING) ? 127 : player_speed;

    struct Animation *targetAnim = m->animation->targetAnim;
    // Animation-specific looping
    if (override_anim) {
        if (loop_anim && !hang_anim) m->animation->targetAnim->flags &= ~ANIM_FLAG_NOLOOP;
        else m->animation->targetAnim->flags |= ANIM_FLAG_NOLOOP;
    }
    bool targetAnimLooping = (m->animation->targetAnim->flags & ANIM_FLAG_NOLOOP) == 0;

    // Flip-flop
    if (pause_anim && override_anim) {
        m->marioObj->header.gfx.animInfo.animFrame = paused_frame;
        // If the animation goes OOB
        if (paused_frame > targetAnim->loopEnd-1) paused_frame = targetAnim->loopEnd-1;
        if (paused_frame < targetAnim->loopStart) paused_frame = targetAnim->loopStart;
    } else
        if (!hang_anim && is_anim_past_end(m) && !targetAnimLooping) override_anim = false;
}