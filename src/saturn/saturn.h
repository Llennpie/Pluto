#ifndef Saturn
#define Saturn

#include <PR/ultratypes.h>

#ifdef __cplusplus

#include <mario_animation_ids.h>

extern "C" {
#endif
    extern bool allow_game_input;

    #define CAM_NORMAL 0
    #define CAM_FROZEN 1

    extern bool freeze_camera;
    extern float freeze_camera_speed;
    extern float wiggle_intensity;
    extern bool wiggle_bone_detected;
    extern bool wind_enabled;
    extern float wind_angle;
    extern float wind_strength;
    extern float wind_sway;
    int saturn_camera_update();
    extern float camera_follow_speed;

    extern bool enable_hud;
    extern bool enable_torso_rotation;
    extern bool enable_head_rotation;
    extern int head_rotation[2];
    extern bool enable_model_particles;
    extern float face_angle;
    
    extern bool custom_eyes;
    extern int switch_state_eyes;
    extern int switch_state_hand_right;
    extern int switch_state_hand_left;
    extern int switch_state_cap;
    extern int switch_state_powerup;
    extern int vanish_transparency;

    extern int active_saturn_model_index;

    extern bool override_anim;
    extern int selected_anim_index;
    extern int selected_panim_index;
    extern bool pause_anim;
    extern int paused_frame;
    extern bool hang_anim;
    extern bool loop_anim;
    extern bool enable_custom_anim;
    extern bool bone_count_matches;

    extern bool is_spinning;
    extern float spinning_speed;
    extern int player_speed;
    extern int walkpoint_speed;

    void saturn_action_idle(struct MarioState*);
    void saturn_update_frame();

#ifdef __cplusplus
}
#endif

#endif