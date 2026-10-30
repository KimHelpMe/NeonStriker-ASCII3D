#define MINIAUDIO_IMPLEMENTATION
#define MA_NO_ENCODING          // 녹음/인코딩은 쓰지 않음 (빌드 크기 절약)
#define MA_NO_GENERATION
#include "miniaudio.h"
#include "Audio.h"
#include "Config.h"
#include <cstdio>

static ma_engine g_engine;
static bool      g_ok = false;

// 같은 효과음을 겹쳐 재생할 수 있도록 효과음마다 사본을 여러 개 둠 (NEON STRIKER의 alias와 같은 방식)
constexpr int VOICES = 6;
static ma_sound g_snd[S_COUNT][VOICES];
static bool     g_loaded[S_COUNT][VOICES];
static int      g_next[S_COUNT];

static const char* FILES[S_COUNT] = {
    "shot_normal.wav", "shot_flame.wav", "shot_laser.wav",
    "charge.wav", "charge_ready.wav", "charge_shot.wav",
    "bomb_throw.wav", "bomb_explode.wav",
    "hit.wav", "explode_small.wav", "explode_mid.wav", "boss_explode.wav",
    "player_hit.wav", "player_die.wav",
    "item.wav", "powerup.wav", "levelup.wav", "fever.wav",
    "warning.wav", "select.wav", "pause.wav", "tick.wav", "stage_start.wav", "game_over.wav", "clear.wav",
    "giga_laser.wav", "flame_orb.wav", "missile_launch.wav",
};

void Audio::Init()
{
    if (ma_engine_init(NULL, &g_engine) != MA_SUCCESS) return;   // 오디오 장치 없음 -> 소리 없이
    g_ok = true;
    char path[256];
    for (int i = 0; i < S_COUNT; i++)
        for (int v = 0; v < VOICES; v++) {
            snprintf(path, sizeof(path), "assets/sounds/%s", FILES[i]);
            // MA_SOUND_FLAG_DECODE: 미리 디코딩해서 메모리에 둠 (같은 파일은 엔진이 한 번만 디코딩해서 공유)
            g_loaded[i][v] = ma_sound_init_from_file(&g_engine, path, MA_SOUND_FLAG_DECODE, NULL, NULL, &g_snd[i][v]) == MA_SUCCESS;
        }
}

void Audio::Shutdown()
{
    if (!g_ok) return;
    for (int i = 0; i < S_COUNT; i++)
        for (int v = 0; v < VOICES; v++)
            if (g_loaded[i][v]) ma_sound_uninit(&g_snd[i][v]);
    ma_engine_uninit(&g_engine);
    g_ok = false;
}

void Audio::Play(SfxId id, float volume, float pitchVar)
{
    if (!g_ok) return;
    int v = g_next[id];
    g_next[id] = (v + 1) % VOICES;
    if (!g_loaded[id][v]) return;
    ma_sound* s = &g_snd[id][v];
    ma_sound_stop(s);
    ma_sound_seek_to_pcm_frame(s, 0);
    ma_sound_set_volume(s, volume);
    ma_sound_set_pitch(s, pitchVar > 0 ? RandF(1.0f - pitchVar, 1.0f + pitchVar) : 1.0f);
    ma_sound_start(s);
}

void Audio::Stop(SfxId id)
{
    if (!g_ok) return;
    for (int v = 0; v < VOICES; v++) if (g_loaded[id][v]) ma_sound_stop(&g_snd[id][v]);
}

bool Audio::IsPlaying(SfxId id)
{
    if (!g_ok) return false;
    for (int v = 0; v < VOICES; v++) if (g_loaded[id][v] && ma_sound_is_playing(&g_snd[id][v])) return true;
    return false;
}
