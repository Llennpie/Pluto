#include "title_music.h"

#include <stdio.h>
#include <string.h>

#include "include/bass/bass.h"
#include "include/bass/bass_fx.h"
#include "src/bass_audio/bass_audio_helpers.h"
#include "pc/platform.h"
#include "pc/configfile.h"
#include "pc/debuglog.h"
#include "saturn/saturn.h"

static HSTREAM sTitleStream = 0;

// streamed music that i definitely did not steal haha
// coop might have an api for this already but i know what i know

void title_music_start(void) {
    if (sTitleStream != 0) return;

    char* path = saturn_get_title_screen_song_path(sys_user_path(), sys_exe_path());
    if (!path) {
        LOG_ERROR("title_music: could not find song");
        return;
    }

    HSTREAM raw = BASS_StreamCreateFile(FALSE, path, 0, 0, BASS_STREAM_PRESCAN | BASS_STREAM_DECODE);
    if (!raw) {
        LOG_ERROR("title_music: failed to open '%s' (BASS error %d)", path, BASS_ErrorGetCode());
        return;
    }

    sTitleStream = BASS_FX_TempoCreate(raw, BASS_STREAM_PRESCAN);
    if (!sTitleStream) {
        LOG_ERROR("title_music: BASS_FX_TempoCreate failed (BASS error %d)", BASS_ErrorGetCode());
        return;
    }

    float master = (float)configMasterVolume / 127.0f;
    float music = (float)configMusicVolume  / 127.0f;
    BASS_ChannelSetAttribute(sTitleStream, BASS_ATTRIB_VOL, master * music);
    BASS_ChannelFlags(sTitleStream, BASS_SAMPLE_LOOP, BASS_SAMPLE_LOOP);
    BASS_ChannelPlay(sTitleStream, TRUE);
}

void title_music_update_volume(void) {
    if (sTitleStream == 0) return;
    float master = (float)configMasterVolume / 127.0f;
    float music = (float)configMusicVolume  / 127.0f;
    BASS_ChannelSetAttribute(sTitleStream, BASS_ATTRIB_VOL, master * music);
}

void title_music_stop(void) {
    if (sTitleStream == 0) return;
    BASS_ChannelStop(sTitleStream);
    BASS_StreamFree(sTitleStream);
    sTitleStream = 0;
}
