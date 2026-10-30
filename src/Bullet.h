#pragma once
// ============================================================
//  탄환 풀 (NEON STRIKER의 BulletType 구조를 3D로)
//  레이저 / 기가 레이저 / 화염구는 탄환이 아니라 Player 상태로 관리 (NEON과 동일)
// ============================================================
#include "Math3D.h"
#include "Config.h"

class Game;

enum class BulletType {
    P_NORMAL, P_FLAME, P_MISSILE, P_BOMB, P_SHRAPNEL,   // 플레이어 쪽
    E_SMALL, E_BOSS,                                    // 적 쪽
};

struct Bullet {
    BulletType type;
    Vec3  pos, prev, vel;   // prev: 직전 프레임 위치 (빠른 탄의 관통 방지 판정용)
    float age, life;
    float startZ;           // 발사 지점 z (미사일 링 크기 계산용)
    float radius;
    int   damage;
    uint16_t color;
    bool  active;

    bool  IsPlayerSide() const { return type <= BulletType::P_SHRAPNEL; }
    float MissileRingRadius() const;   // 기가 미사일 링의 현재 반지름 (전진할수록 커짐)
};

class BulletSystem {
public:
    void Reset();
    Bullet* Spawn(BulletType type, Vec3 pos, Vec3 vel, int damage, uint16_t color, float life = 6.0f);
    void Update(Game& g, float dt);
    void Draw() const;
    void ClearEnemyBullets(Game& g, bool withEffect);
    void ShatterBomb(Game& g, Vec3 pos);
    Bullet* Data() { return pool; }

private:
    Bullet pool[BULLET_COUNT];
    int searchStart = 0;
};
