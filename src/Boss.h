#pragma once
// ============================================================
//  보스 (NEON STRIKER Boss를 3D로)
//  NONE -> WARNING(경고 3초, 먼 곳에서 진입) -> FIGHT -> EXPLODING -> NONE
// ============================================================
#include "Math3D.h"
#include "Config.h"

class Game;
enum class BossState { NONE, WARNING, FIGHT, EXPLODING };

class Boss {
public:
    void Init(int stage);
    void AddKillCount(Game& g);
    void ForceAppear(Game& g);
    void Update(Game& g, float dt);
    void Draw() const;
    void DrawUI() const;   // 원본 스타일 상단 HP바 + WARNING 화면

    bool Damage(Game& g, int damage, float invincible);
    Vec3 Half() const;     // 피격 상자 반 크기
    bool Overlaps(Vec3 p, float r) const;

    BossState state = BossState::NONE;
    bool  Fighting() const { return state == BossState::FIGHT; }
    bool  Present() const { return state != BossState::NONE; }
    bool  defeated = false;
    int   killCount = 0, step = 1, hp = 0, maxHp = 1;
    Vec3  pos{};

private:
    void Fire(Game& g);
    float warningTimer = 0, explodeTimer = 0, moveAcc = 0, fireTimer = 0;
    float spiralAngle = 0, invincibleTimer = 0, flash = 0, flashCooldown = 0, time = 0;
    int   burstCount = 0;
};
