#pragma once
// ============================================================
//  플레이어 (NEON STRIKER Player를 3D로)
//  z = 0 평면에서 상하좌우 이동, 공격은 +z(화면 안쪽)로 나감
// ============================================================
#include "Math3D.h"
#include "Config.h"

class Game;

// 빔 같은 '상자 모양 공격'의 영역 (중심 + 반 크기)
struct Box3 {
    Vec3 center, half;
    bool Contains(Vec3 p, float r) const {
        return std::fabs(p.x - center.x) <= half.x + r && std::fabs(p.y - center.y) <= half.y + r &&
               std::fabs(p.z - center.z) <= half.z + r;
    }
};

class Player {
public:
    void Reset();
    void Update(Game& g, float dt);
    void Draw(const Game& g) const;

    void Damage(Game& g, int damage);
    void AddExp(Game& g);
    void ApplyItem(Game& g, ItemType type);

    // --- 기체에 붙어 있는 공격 ---
    bool  LaserOn() const { return laserOn; }
    Box3  LaserBox() const;
    bool  GigaLaserOn() const { return gigaLaserTimer > 0; }
    Box3  GigaLaserBox() const;
    bool  FlameOrbsOn() const { return flameOrbTimer > 0; }
    void  FlameOrbCenters(Vec3 out[4]) const;
    float OrbSpread() const;
    static constexpr float FLAME_ORB_TIME   = 4.0f;
    static constexpr float FLAME_ORB_RADIUS = 6.0f;   // 화염구 하나의 반지름
    float laserEndZ = FAR_Z;        // 레이저가 보스에 막히는 지점 (Game이 매 프레임 계산)
    float gigaLaserEndZ = FAR_Z;

    Vec3 pos{};
    static constexpr float HIT_RADIUS = 1.0f;

    bool  Alive() const { return !dead; }
    bool  Invincible() const { return invincibleTimer > 0 || hurtTimer > 0; }
    bool  GameOverReady() const { return dead && lives < 0 && respawnTimer <= 0; }
    int   hp = 5, maxHp = 5;
    int   level = 1, exp = 0, maxExp = 10;
    int   bombs = 3, lives = 2, kills = 0;
    WeaponType weapon = WeaponType::NORMAL;
    int   weaponLevel = 1;
    bool  fever = false;
    float feverTimer = 0;
    bool  charging = false;
    float chargeTimer = 0;
    float bank = 0;                 // 좌우 이동 시 기체 기울기 (연출)

private:
    void Fire(Game& g, float dt);
    void FireNormal(Game& g);
    void FireFlame(Game& g);
    void ReleaseCharge(Game& g);
    void Die(Game& g);

    bool  dead = false;
    float respawnTimer = 0, invincibleTimer = 0, hurtTimer = 0;
    float fireTimer = 0, flameOrbit = 0;
    bool  laserOn = false;
    float laserSoundTimer = 0, flameSoundTimer = 0;
    bool  chargeReadyPlayed = false;
    int   missileQueue = 0;
    float missileTimer = 0;
    float gigaLaserTimer = 0, flameOrbTimer = 0, orbAngle = 0, orbFxTimer = 0;
    float time = 0;
    Vec3  vel{};                     // 이번 프레임 이동 방향 (기울기 연출용)
};
