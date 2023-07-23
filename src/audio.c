#include "audio.h"

#if !(defined(NO_SOUND) && defined(NO_MUSIC))
#  include <maxmod.h>
    // from mmutil
#  include "soundbank.h"
#  include "soundbank_bin.h"
#endif

static int mus_vol = 768;
static int sfx_vol = 255;
static bool music_mute = false;
static bool sfx_mute = false;
enum Music last_music = MUSIC_NONE;

void audio_init() {
#if !(defined(NO_SOUND) && defined(NO_MUSIC))
    // Maxmod init
    mmInitDefault((mm_addr)soundbank_bin, 8);
    irq_add(II_VBLANK, mmVBlank);
#endif
}

void audio_frame() {
#if !(defined(NO_SOUND) && defined(NO_MUSIC))
    mmFrame();
#endif
}

void play_music(enum Music mus) {
    if(mus != MUSIC_NONE) last_music = mus;
    if(music_mute) return;
#ifndef NO_MUSIC
    switch (mus) {
        case MUSIC_INTRO:
            mmStart(MOD_INTRO, MM_PLAY_LOOP);
            mmSetModuleVolume(512);
            break;
        case MUSIC_INGAME:
            mmStart(MOD_INGAME, MM_PLAY_LOOP);
            mmSetModuleVolume(768);
            break;
        case MUSIC_GAMEOVER:
        default:
            mmStop();
            break;
    }
#endif
}

void pause_music(bool pause) {
#ifndef NO_MUSIC
    if(pause)
        mmPause();
    else
        mmResume();
#endif
}

void volume_music(int vol) {
    mus_vol = 204 * vol;
    mmSetModuleVolume(vol);
}

void mute_music(bool mute) {
    music_mute = mute;

    if(mute) {
        mmStop();
    } else {
        play_music(last_music);
    }
}

void play_sfx(enum Sound snd) {
    if(sfx_mute) return;
#ifndef NO_SOUND
#if 0
    // generic
    mm_sound_effect sfx = {
        { 0 }, // id
        (int)(1.0f * (1<<10)), // rate
        0, // handle
        255, // volume
        128, // panning
    };
    sfx.id = SFX_SELECT;
    mmEffectEx(&sfx);
#endif
    mm_sfxhand sh;

    switch (snd) {
        case SOUND_ANIMATE:
            sh = mmEffect(SFX_ANIMATE);
        case SOUND_SELECT:
            sh = mmEffect(SFX_SELECT);
            break;
        case SOUND_DESELECT:
            sh = mmEffect(SFX_DESELECT);
            break;
        case SOUND_FAIL:
            sh = mmEffect(SFX_FAIL);
            break;
        case SOUND_WIN:
            sh = mmEffect(SFX_WIN);
            break;
        case SOUND_LOOSE:
            //break;
        default:
            return;
            break;
    }
    mmEffectVolume(sh, sfx_vol);
#endif
}

void volume_sfx(int vol) {
    sfx_vol = 51 * vol;
}

void mute_sfx(bool mute) {
    sfx_mute = mute;
}
