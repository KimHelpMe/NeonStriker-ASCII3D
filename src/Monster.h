#pragma once
// ============================================================
//  일반 몬스터 3종 (NEON STRIKER Monster를 3D로)
//  먼 곳(z 180~260)에서 생성되어 플레이어 쪽(-z)으로 다가오며, 지나치면 다시 멀리서 생성
// ============================================================
#include "Math3D.h"
#include "Config.h"

class Game;
enum class MonsterType { NORMAL, MEDIUM, HEAVY };

struct Monster {
    MonsterType type;
    Vec3  pos, drift;      // drift: 상하좌우 흔들림 방향
    float speed;           // z 방향 접근 속도
    int   hp, maxHp;
    float fireTimer, hitCooldown, flash, spin;
    bool  active;
    static constexpr float DRAW_SCALE = 1.3f;   // 멀리서도 보이도록 모델을 키워서 그림
    float Radius() const { return (type == MonsterType::NORMAL ? 2.4f : type == MonsterType::MEDIUM ? 3.3f : 4.6f) * DRAW_SCALE; }
};

class MonsterSystem {
public:
    void Start(const DifficultyRules& rules);
    void Reset();
    void Update(Game& g, float dt);
    void Draw() const;
    bool Hit(Game& g, int idx, int damage, float cooldown);
    Monster* Data() { return m; }
    bool enabled = true;

private:
    void Init(int i, bool firstWave);
    void Kill(Game& g, int i, bool rammed);
    Monster m[MONSTER_COUNT];
    int activeCount = 20, normalCount = 18, mediumCount = 1;
};
