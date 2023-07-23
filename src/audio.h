#pragma once

#include <tonc.h>

enum Music { MUSIC_NONE, MUSIC_INTRO, MUSIC_INGAME, MUSIC_GAMEOVER };
enum Sound { SOUND_SELECT, SOUND_DESELECT, SOUND_ANIMATE, SOUND_FAIL, SOUND_WIN, SOUND_LOOSE };

void audio_init();
void audio_frame();

void play_music(enum Music mus);
void pause_music(bool pause);
void volume_music(int vol);
void mute_music(bool mute);

void play_sfx(enum Sound snd);
void volume_sfx(int vol);
void mute_sfx(bool mute);
