#pragma once
// ============================================================
//  효과음 (miniaudio) - NEON STRIKER의 효과음 파일과 ID를 그대로 재사용
//  miniaudio: 헤더 파일 하나짜리 오디오 라이브러리 (raylib 내부에서도 사용). 설치 불필요
// ============================================================

enum SfxId {
    S_SHOT_NORMAL, S_SHOT_FLAME, S_SHOT_LASER,
    S_CHARGE, S_CHARGE_READY, S_CHARGE_SHOT,
    S_BOMB_THROW, S_BOMB_EXPLODE,
    S_HIT, S_EXPLODE_SMALL, S_EXPLODE_MID, S_BOSS_EXPLODE,
    S_PLAYER_HIT, S_PLAYER_DIE,
    S_ITEM, S_POWERUP, S_LEVELUP, S_FEVER,
    S_WARNING, S_SELECT, S_PAUSE, S_TICK, S_STAGE_START, S_GAME_OVER, S_CLEAR,
    S_GIGA_LASER, S_FLAME_ORB, S_MISSILE_LAUNCH,
    S_COUNT
};

namespace Audio {
    void Init();       // 실패해도(오디오 장치 없음) 게임은 소리 없이 진행
    void Shutdown();
    void Play(SfxId id, float volume = 1.0f, float pitchVar = 0.0f);
    void Stop(SfxId id);
    bool IsPlaying(SfxId id);
}
