#include "saturn.h"
#include "saturn/saturn_keyframe.h"
#include <iostream>
#include <string>
#include <vector>
#include <SDL2/SDL.h>

#include <filesystem>
namespace fs = std::filesystem;

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
    #include "game/rendering_graph_node.h"
    #include "engine/math_util.h"
    #include "game/object_helpers.h"
    #include "audio/external.h"
}

bool freeze_camera;
float freeze_camera_speed = 1.f;
float saturn_camera_fov = 45.f;
float saturn_camera_tilt = 0.f;
float camera_follow_speed = 1.f;
float wiggle_intensity = 1.0f;
bool wind_enabled = false;
float wind_angle[3] = { 0.f, 0.f, 0.f };
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
float anim_speed = 1.0f;
bool enable_custom_anim;

bool bone_count_matches;

bool is_spinning;
float spinning_speed = 1.f;
int player_speed = 127;
int walkpoint_speed = 127;
bool anim_sync_to_timeline = false;

/* Returns false when an ImGui text widget is active (i.e. the user is editing text). */
bool allow_game_input;

float camera_kf_state[6] = {};

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

        // Timeline playback
        bool camera_automated = timelines.count("Camera###timeline_camera") > 0;
        if (camera_automated) {
            for (int i = 0; i < 3; i++) {
                gCamera->pos[i]   = camera_kf_state[i];
                gCamera->focus[i] = camera_kf_state[3 + i];
            }
            if (timeline_is_playing) return CAM_FROZEN;
        }

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
        static f32 yvel = 0.f;
        if (SDL_GetKeyboardState(NULL)[SDL_SCANCODE_T])
            yvel += 3.f * freeze_camera_speed;
        else if (SDL_GetKeyboardState(NULL)[SDL_SCANCODE_U])
            yvel -= 3.f * freeze_camera_speed;
        else
            yvel = 0.f;

        gCamera->pos[1] += yvel;
        gCamera->focus[1] += yvel;
        yvel = approach_f32_symmetric(yvel, 0.f, 2.f);
        yvel = approach_f32_asymptotic(yvel, 0.f, 0.1f);

        // Timeline playback
        if (camera_automated) {
            for (int i = 0; i < 3; i++) {
                camera_kf_state[i]     = gCamera->pos[i];
                camera_kf_state[3 + i] = gCamera->focus[i];
            }
        }

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
    bool just_entered = (m->prevAction != ACT_IDLE);
    if (just_entered) {
        m->actionState = 0;
        m->prevAction = ACT_IDLE;
    }

    if (m->action & ACT_FLAG_SWIMMING) return CHAR_ANIM_WATER_IDLE;
    if (m->heldObj != NULL) return CHAR_ANIM_IDLE_WITH_LIGHT_OBJ;
    else {
        if (!enable_head_rotation || m->action == ACT_FIRST_PERSON) return CHAR_ANIM_FIRST_PERSON;
        if (!just_entered && is_anim_at_end(m)) {
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

// Interpolation for slow-mo
float g_saturn_anim_blend_t = -1.0f;
s16 g_saturn_floor_frame  = 0;
s16 g_saturn_ceil_frame   = 0;
float g_saturn_prev_blend_t  = -1.0f;
s16 g_saturn_prev_floor_frame = 0;
s16 g_saturn_prev_ceil_frame  = 0;

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
        }
    } else {
        // Reset animation state
        pause_anim = false;
        is_editing_panim = false;
        g_root_offset[0] = g_root_offset[1] = g_root_offset[2] = 0.0f;
    }

    // Animation speed, follows interpolation for slow-motion
    static float s_float_frame = 0.0f;
    static float s_extra_accum = 0.0f;
    static bool  s_slow_active = false;
    static bool  s_fast_active = false;

    if (override_anim && !pause_anim && targetAnim != NULL) {
        int loopStart = targetAnim->loopStart;
        int loopEnd = targetAnim->loopEnd;
        int loopLen = loopEnd - loopStart;

        if (loopLen > 0 && anim_speed < 0.999f) {
            // Slow motion
            if (!s_slow_active && m->marioObj->header.gfx.animInfo.animFrame < loopStart) {
            } else {
            if (!s_slow_active) {
                s_float_frame = (float)m->marioObj->header.gfx.animInfo.animFrame;
                s_slow_active = true;
            }
            s_float_frame += anim_speed;

            if (s_float_frame >= (float)loopEnd) {
                if (targetAnimLooping && !hang_anim) {
                    while (s_float_frame >= (float)loopEnd) s_float_frame -= (float)loopLen;
                } else {
                    s_float_frame = (float)(loopEnd - 1);
                    if (hang_anim) { pause_anim = true; paused_frame = loopEnd - 1; }
                    else if (!targetAnimLooping) {
                        if (saturn_check_for_chainer() == false) {
                            override_anim = false;
                            set_mario_animation(m, MARIO_ANIM_START_CROUCHING);
                        }
                    }
                }
            }

            g_saturn_prev_floor_frame = g_saturn_floor_frame;
            g_saturn_prev_ceil_frame = g_saturn_ceil_frame;
            g_saturn_prev_blend_t = g_saturn_anim_blend_t;

            g_saturn_floor_frame  = (s16)(int)s_float_frame;
            s16 ceil_f = g_saturn_floor_frame + 1;
            g_saturn_ceil_frame   = (ceil_f >= (s16)loopEnd) ? (s16)loopStart : ceil_f;
            g_saturn_anim_blend_t = s_float_frame - (float)g_saturn_floor_frame;

            m->marioObj->header.gfx.animInfo.animFrame = g_saturn_floor_frame;
            m->marioObj->header.gfx.animInfo.animTimer = gAreaUpdateCounter;
            paused_frame  = g_saturn_floor_frame;
            s_extra_accum = 1.0f - anim_speed; // sync
            s_fast_active = false;
            }

        } else {
            // Normal speed (or fast forward), no interpolation
            s_slow_active = false;
            g_saturn_anim_blend_t = -1.0f;

            if (!s_fast_active) {
                s_extra_accum  = 1.0f - anim_speed;
                s_fast_active  = true;
            }

            if (is_anim_at_end(m)) {
                if (hang_anim) { pause_anim = true; paused_frame = m->marioObj->header.gfx.animInfo.animFrame; }
                else if (!targetAnimLooping) {
                    if (saturn_check_for_chainer() == false) {
                        override_anim = false;
                        set_mario_animation(m, MARIO_ANIM_START_CROUCHING);
                    }
                }
            }

            // Fast forward, skip frames
            if (loopLen > 0 && anim_speed > 1.001f) {
                int frame = m->marioObj->header.gfx.animInfo.animFrame;
                if (frame >= loopStart) {
                    s_extra_accum += (anim_speed - 1.0f);
                    int extra = (int)s_extra_accum;
                    if (s_extra_accum < 0.0f && (float)extra != s_extra_accum) extra--;
                    s_extra_accum -= (float)extra;
                    frame += extra;
                    // Fast forward can go OOB, skipping the end frame and looping
                    if (frame >= loopEnd && !targetAnimLooping) {
                        if (hang_anim) { pause_anim = true; paused_frame = loopEnd - 1; }
                        else {
                            if (saturn_check_for_chainer() == false) {
                                override_anim = false;
                                set_mario_animation(m, MARIO_ANIM_START_CROUCHING);
                            }
                        }
                        frame = loopEnd - 1;
                    } else {
                        while (frame >= loopEnd)  frame -= loopLen;
                        while (frame < loopStart) frame += loopLen;
                    }
                    m->marioObj->header.gfx.animInfo.animFrame = (s16)frame;
                    paused_frame = frame;
                }
            }
        }
    } else {
        s_slow_active = false;
        s_fast_active = false;
        g_saturn_anim_blend_t = -1.0f;
        g_saturn_prev_blend_t = -1.0f;
        s_extra_accum = 1.0f - anim_speed;
        if (pause_anim) s_float_frame = (float)paused_frame; // re-sync on pause
    }

    // Check if the model's bone count matches the current PlutoAnim's bone count
    // This determines if we animate MCOMP extra bones or not
    bone_count_matches = current_pluto_anim.BoneCount > 20;
}

static char title_screen_path[SYS_MAX_PATH];

char* saturn_get_title_screen_song_path(const char* dst, const char* src) {
    std::string name = "Panimotion.mp3";
    fs::path dst_path = fs::path(dst) / name;
    fs::path src_path = fs::path(src) / name;

    if (fs::exists(dst_path)) strcpy(title_screen_path, dst_path.c_str());
    else if (fs::exists(src_path)) {
        fs::copy_file(dst_path, src_path);
        strcpy(title_screen_path, dst_path.c_str());
    }
    else return NULL;

    return title_screen_path;
}
