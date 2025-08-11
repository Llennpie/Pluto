#include "game/mario.h"
#include "game/mario_misc.h"
#include "game/obj_behaviors.h"
#include "game/object_helpers.h"
#include "engine/math_util.h"
#include "behavior_data.h"
#include "src/saturn/saturn_models.h"

void bhv_hat_follow_init(void) {
    // Optionally set model or animation here
}

void bhv_hat_follow_loop(void) {
    if (gMarioStates[0].marioObj == NULL) return;
    if (active_accessory_index == -1) return;

    struct MarioState *m = gMarioState;

    o->oPosX = m->pos[0];
    o->oPosY = m->pos[1];
    o->oPosZ = m->pos[2];
    o->oFaceAnglePitch = m->faceAngle[0];
    o->oFaceAngleYaw = m->faceAngle[1];
    o->oFaceAngleRoll = m->faceAngle[2];
    vec3s_copy(o->header.gfx.angle, m->marioObj->header.gfx.angle);

    o->header.gfx.pos[0] = m->marioObj->header.gfx.pos[0];
    o->header.gfx.pos[1] = m->marioObj->header.gfx.pos[1];
    o->header.gfx.pos[2] = m->marioObj->header.gfx.pos[2];
    o->header.gfx.angle[0] = m->marioObj->header.gfx.angle[0];
    o->header.gfx.angle[1] = m->marioObj->header.gfx.angle[1];
    o->header.gfx.angle[2] = m->marioObj->header.gfx.angle[2];
    o->header.gfx.scale[0] = m->marioObj->header.gfx.scale[0];
    o->header.gfx.scale[1] = m->marioObj->header.gfx.scale[1];
    o->header.gfx.scale[2] = m->marioObj->header.gfx.scale[2];

    // Copy animation frame and state
    o->header.gfx.animInfo.animID = m->marioObj->header.gfx.animInfo.animID;
    o->header.gfx.animInfo.animFrame = m->marioObj->header.gfx.animInfo.animFrame;
    o->header.gfx.animInfo.animFrameAccelAssist = m->marioObj->header.gfx.animInfo.animFrameAccelAssist;
    o->header.gfx.animInfo.curAnim = m->marioObj->header.gfx.animInfo.curAnim;
    o->header.gfx.animInfo.animAccel = m->marioObj->header.gfx.animInfo.animAccel;
    o->header.gfx.animInfo.animYTrans = m->marioObj->header.gfx.animInfo.animYTrans;
}