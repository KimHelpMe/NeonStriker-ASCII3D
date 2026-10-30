#include "Player.h"
#include "Game.h"
#include "Audio.h"
#include "Platform.h"
#include "Render3D.h"
#include <cstdio>
#include <initializer_list>

static const float SHOT_DELAY = 0.12f;
static uint16_t WeaponColor(WeaponType w)
{
    return w == WeaponType::NORMAL ? C_LCYAN : w == WeaponType::FLAME ? C_LRED : C_LYELLOW;
}

void Player::Reset()
{
    pos = { 0, -4, 0 };
    hp = maxHp = 5;
    level = 1; exp = 0; maxExp = 10;
    bombs = 3; lives = 2; kills = 0;
    weapon = WeaponType::NORMAL; weaponLevel = 1;
    fever = false; feverTimer = 0;
    charging = false; chargeTimer = 0; chargeReadyPlayed = false;
    dead = false; respawnTimer = 0; invincibleTimer = 2.0f; hurtTimer = 0;
    fireTimer = 0; laserOn = false; missileQueue = 0; gigaLaserTimer = 0; flameOrbTimer = 0;
    bank = 0;
}

// ------------------------------------------------------------
void Player::Update(Game& g, float dt)
{
    time += dt;
    laserOn = false;

    if (dead) {   // 사망 연출 중
        respawnTimer -= dt;
        if (respawnTimer <= 0 && lives >= 0) {
            dead = false; hp = maxHp; pos = { 0, -4, 0 }; invincibleTimer = 2.0f;
        }
        return;
    }
    if (invincibleTimer > 0) invincibleTimer -= dt;
    if (hurtTimer > 0) hurtTimer -= dt;
    if (fever) {
        feverTimer -= dt;
        if (feverTimer <= 0) { fever = false; feverTimer = 0; }
        if (RandI(0, 1) == 0)   // 무지개 잔상
            g.fx.Spawn(pos + Vec3{ RandF(-3, 3), 0, -2 }, { 0, 0, -25 }, 0.4f, 0.6f, 0.1f, RainbowColor(time), RainbowColor(time + 0.3f));
    }

    // ===================== TEST KEY (원본과 동일 + 6: 보스) =====================
    if (Input::Pressed(K_1) && weaponLevel < MAX_WEAPON_LEVEL) { weaponLevel++; Audio::Play(S_POWERUP); }
    if (Input::Pressed(K_2)) { weapon = WeaponType::NORMAL; Audio::Play(S_SELECT); }
    if (Input::Pressed(K_3)) { weapon = WeaponType::FLAME;  Audio::Play(S_SELECT); }
    if (Input::Pressed(K_4)) { weapon = WeaponType::LASER;  Audio::Play(S_SELECT); }
    if (Input::Pressed(K_5)) ApplyItem(g, ItemType::FEVER);

    // ---- 1. 이동 ----
    float speed = 26.0f * (charging ? 0.4f : 1.0f);
    Vec3 d = {};
    if (Input::Down(K_LEFT))  d.x -= 1;
    if (Input::Down(K_RIGHT)) d.x += 1;
    if (Input::Down(K_UP))    d.y += 1;
    if (Input::Down(K_DOWN))  d.y -= 1;
    if (d.x != 0 && d.y != 0) d = d * 0.7071f;
    vel = d;
    pos.x = Clampf(pos.x + d.x * speed * dt, -PLAYER_X_LIMIT, PLAYER_X_LIMIT);
    pos.y = Clampf(pos.y + d.y * speed * dt, -PLAYER_Y_LIMIT, PLAYER_Y_LIMIT);
    bank += ((-d.x * 0.5f) - bank) * Clampf(dt * 8, 0, 1);   // 좌우로 움직이면 기체가 기울어짐

    // 엔진 불꽃 (뒤로)
    g.fx.Spawn(pos + Vec3{ RandF(-0.4f, 0.4f), RandF(-0.3f, 0.3f), -3.2f }, { 0, 0, -18 }, 0.15f, 0.5f, 0.1f, C_LMAGENTA, C_MAGENTA);

    // ---- 2. 공격 ----
    Fire(g, dt);

    // ---- 3. 폭탄 (C) ----
    if (Input::Pressed(K_C) && (fever || bombs > 0)) {
        if (!fever) bombs--;
        g.bullets.ClearEnemyBullets(g, true);
        g.bullets.Spawn(BulletType::P_BOMB, pos + Vec3{ 0, 0, 3 }, { 0, 0, 90 }, 0, C_LGREEN, 0.55f);
        Audio::Play(S_BOMB_THROW);
    }

    // ---- 4. 차지 (무기 5레벨, X) ----
    if (weaponLevel >= MAX_WEAPON_LEVEL) {
        if (Input::Down(K_X)) {
            if (!charging) Audio::Play(S_CHARGE, 0.8f);
            charging = true;
            chargeTimer += dt;
            if (chargeTimer >= MAX_CHARGE_TIME) {
                chargeTimer = MAX_CHARGE_TIME;
                if (!chargeReadyPlayed) { Audio::Play(S_CHARGE_READY); chargeReadyPlayed = true; }
            }
            // 기 모으기: 바깥에서 기체로 빨려 들어오는 입자
            float ratio = chargeTimer / MAX_CHARGE_TIME;
            for (int i = 0; i < 1 + (int)(ratio * 2); i++) {
                Vec3 dir = Normalize({ RandF(-1, 1), RandF(-1, 1), RandF(-0.3f, 1) });
                Vec3 from = pos + dir * (8.0f - ratio * 3);
                g.fx.Spawn(from, (pos - from) * 3.5f, 0.28f, 0.4f, 0.1f, WeaponColor(weapon), C_BWHITE);
            }
        } else if (charging) {
            Audio::Stop(S_CHARGE);
            if (chargeTimer >= MAX_CHARGE_TIME) ReleaseCharge(g);
            charging = false; chargeTimer = 0; chargeReadyPlayed = false;
        }
    } else if (charging) {
        Audio::Stop(S_CHARGE);
        charging = false; chargeTimer = 0; chargeReadyPlayed = false;
    }

    // ---- 5. 차지 공격 진행 ----
    if (missileQueue > 0) {   // NORMAL 차지: 기가 미사일 링 10연사
        missileTimer += dt;
        if (missileTimer >= SHOT_DELAY) {
            g.bullets.Spawn(BulletType::P_MISSILE, pos + Vec3{ 0, 0, 3 }, { 0, 0, 75 }, 18, C_LCYAN, 4.0f);
            Audio::Play(S_MISSILE_LAUNCH, 0.9f, 0.08f);
            g.fx.Spawn(pos + Vec3{ 0, 0, 4 }, {}, 0.15f, 3.0f, 1.0f, C_BWHITE, C_LCYAN);
            for (int k = 0; k < 14; k++) {
                float a = k * 2 * PI / 14;
                g.fx.Spawn(pos + Vec3{ 0, 0, 4 }, { std::cos(a) * 30, std::sin(a) * 30, 15 }, 0.3f, 0.5f, 0.1f, C_BWHITE, C_LBLUE, 3.0f);
            }
            g.fx.Shake(0.5f, 0.08f);
            missileQueue--;
            missileTimer = 0;
        }
    }
    if (gigaLaserTimer > 0) {
        gigaLaserTimer -= dt;
        g.fx.Shake(0.4f, 0.05f);
        if (gigaLaserTimer > 0 && !Audio::IsPlaying(S_GIGA_LASER)) Audio::Play(S_GIGA_LASER, 0.8f);   // 파지지직 반복
        if (gigaLaserTimer <= 0) Audio::Stop(S_GIGA_LASER);
    }
    if (flameOrbTimer > 0) {
        flameOrbTimer -= dt;
        orbAngle += dt * 3.0f;
        if (flameOrbTimer > 0 && !Audio::IsPlaying(S_FLAME_ORB)) Audio::Play(S_FLAME_ORB, 0.8f);   // 화르륵 반복
        if (flameOrbTimer <= 0) Audio::Stop(S_FLAME_ORB);
        orbFxTimer += dt;
        while (orbFxTimer >= 1.0f / 60.0f) {   // 1초에 60번 고정 방출
            orbFxTimer -= 1.0f / 60.0f;
            Vec3 c[4];
            FlameOrbCenters(c);
            for (auto& p : c) {
                for (int k = 0; k < 2; k++)
                    g.fx.Spawn(p + Vec3{ RandF(-3, 3), RandF(-3, 3), RandF(-3, 3) }, { RandF(-3, 3), RandF(3, 8), RandF(-6, 0) },
                               RandF(0.3f, 0.5f), RandF(1.2f, 2.0f), 0.2f, C_LYELLOW, C_RED, 1.0f);
            }
            float ra = RandF(0, 2 * PI);   // 궤도를 따라 이어지는 불의 고리
            g.fx.Spawn(pos + Vec3{ std::cos(ra) * 14 * OrbSpread(), std::sin(ra) * 9 * OrbSpread(), 14 }, { 0, 2, 0 },
                       0.35f, 0.8f, 0.1f, C_LRED, C_RED, 1.0f);
        }
    }
}

// ------------------------------------------------------------
void Player::Fire(Game& g, float dt)
{
    fireTimer += dt;
    flameOrbit += dt * 3.0f;
    if (!Input::Down(K_SPACE)) return;
    switch (weapon) {
    case WeaponType::NORMAL: if (fireTimer >= 0.15f) { FireNormal(g); fireTimer = 0; } break;
    case WeaponType::FLAME:  if (fireTimer >= 0.03f) { FireFlame(g); fireTimer = 0; } break;
    case WeaponType::LASER:
        laserOn = true;
        laserSoundTimer -= dt;
        if (laserSoundTimer <= 0) { Audio::Play(S_SHOT_LASER, 0.6f, 0.03f); laserSoundTimer = 0.1f; }
        break;
    }
}

void Player::FireNormal(Game& g)
{
    const float spd = 140.0f;
    Vec3 nose = pos + Vec3{ 0, 0, 4.5f };
    int dmg = weaponLevel;   // 레벨당 데미지 +1 (원본과 동일)
    // yaw: 좌우로 벌어지는 각도, pitch: 위아래로 벌어지는 각도
    auto Shot = [&](float ox, float yaw, float pitch) {
        Vec3 dir = Normalize({ std::sin(yaw), std::sin(pitch), 1 });
        g.bullets.Spawn(BulletType::P_NORMAL, nose + Vec3{ ox, 0, 0 }, dir * spd, dmg, C_LCYAN, 2.5f);
    };
    switch (weaponLevel) {
    case 1: Shot(0, 0, 0); break;                                                   // 정면 1발
    case 2: Shot(-1.2f, 0, 0); Shot(0, 0, 0); Shot(1.2f, 0, 0); break;             // 정면 3발
    case 3: Shot(-1.2f, 0, 0); Shot(0, 0, 0); Shot(1.2f, 0, 0);                    // + 좌우 대각
            Shot(0, -0.12f, 0); Shot(0, 0.12f, 0); break;
    case 4: Shot(-1.2f, 0, 0); Shot(0, 0, 0); Shot(1.2f, 0, 0);                    // + 부채꼴 5방향
            for (int i = 0; i < 5; i++) Shot(0, -0.2f + i * 0.1f, 0);
            break;
    default:                                                                         // 원뿔 모양 8방향 + 정면
            for (int i = 0; i < 8; i++) { float a = i * PI / 4; Shot(0, std::cos(a) * 0.18f, std::sin(a) * 0.18f); }
            Shot(-1.2f, 0, 0); Shot(1.2f, 0, 0); Shot(0, 0, 0);
            break;
    }
    g.fx.Spawn(nose, {}, 0.06f, 1.2f, 0.3f, C_BWHITE, C_LCYAN);   // 총구 섬광
    Audio::Play(S_SHOT_NORMAL, 0.5f, 0.08f);
}

void Player::FireFlame(Game& g)
{
    float life = 0.28f + weaponLevel * 0.06f;   // 레벨이 오를수록 사거리 증가
    int dmg = weaponLevel <= 2 ? 1 : weaponLevel <= 4 ? 2 : 3;
    Vec3 nose = pos + Vec3{ 0, 0, 4.5f };
    // 보스전 사거리 연장: 화염은 매 프레임 감속(1.5)하므로 앞으로 가는 거리는 약 vz/1.5 * (1 - e^(-1.5*life)).
    // 보스 앞면(+4)까지 못 가는 화염만 속도를 그 비율만큼 올림. 일반전에서는 그대로.
    float bossFrontZ = g.boss.Fighting() ? g.boss.pos.z - g.boss.Half().z + 4.0f : 0.0f;
    float reachRatio = (1.0f - std::exp(-1.5f * life)) / 1.5f;
    auto Puff = [&](Vec3 from, Vec3 dir, float spd) {
        Vec3 d = Normalize(dir);
        if (bossFrontZ > 0 && d.z > 0.1f) {
            float reach = d.z * spd * reachRatio;
            float need = bossFrontZ - from.z;
            if (reach < need) spd *= need / reach;
        }
        g.bullets.Spawn(BulletType::P_FLAME, from, d * spd, dmg, C_LRED, life);
    };
    if (weaponLevel <= 3) {
        float s = weaponLevel == 1 ? 0.04f : weaponLevel == 2 ? 0.08f : 0.13f;
        for (int i = 0; i < 2; i++) Puff(nose, { RandF(-s, s), RandF(-s, s), 1 }, RandF(85, 105));
    } else if (weaponLevel == 4) {
        for (float c : { -0.22f, 0.0f, 0.22f }) Puff(nose, { c + RandF(-0.04f, 0.04f), RandF(-0.04f, 0.04f), 1 }, RandF(85, 105));
    } else {
        // 5레벨: 기수에서 앞으로 곧게 뻗는 원뿔 모양 화염
        // - 각도를 매번 무작위로 뽑음: 순서대로 회전시키면 같은 각도끼리 줄지어 '프로펠러 날개'처럼 보이기 때문
        // - 옆 퍼짐 0.08: 보스 거리(z≈70)에서도 반폭 약 6.6이라 보스 피격 범위(세로 반폭 최소 6 + 화염 반경 1.2) 안에 들어감
        for (int i = 0; i < 6; i++) {
            float a = RandF(0, 2 * PI);
            Vec3 out = { std::cos(a), std::sin(a), 0 };
            Puff(nose + out * 1.0f, out * 0.08f + Vec3{ 0, 0, 1 }, RandF(85, 105));
        }
        Puff(nose, { 0, 0, 1 }, 105);
    }
    flameSoundTimer -= 0.03f;
    if (flameSoundTimer <= 0) { Audio::Play(S_SHOT_FLAME, 0.5f, 0.1f); flameSoundTimer = 0.09f; }
}

void Player::ReleaseCharge(Game& g)
{
    switch (weapon) {
    case WeaponType::NORMAL: missileQueue = 10; missileTimer = SHOT_DELAY; break;
    case WeaponType::FLAME:
        flameOrbTimer = FLAME_ORB_TIME; orbFxTimer = 0;
        for (int k = 0; k < 80; k++) {   // 방출 순간 불의 충격파
            float a = k * 2 * PI / 80;
            g.fx.Spawn(pos, { std::cos(a) * RandF(40, 70), std::sin(a) * RandF(30, 50), RandF(0, 20) },
                       RandF(0.4f, 0.7f), 1.4f, 0.2f, C_LYELLOW, C_RED, 2.5f);
        }
        g.fx.Explosion(pos + Vec3{ 0, 0, 6 }, 2.0f, C_LRED);
        break;
    case WeaponType::LASER: gigaLaserTimer = 5.0f; break;
    }
    Audio::Play(S_CHARGE_SHOT);
    g.fx.Flash(weapon == WeaponType::NORMAL ? 0x30 : weapon == WeaponType::FLAME ? 0x40 : 0x60, 0.12f);
    g.fx.Shake(1.5f, 0.4f);
    g.fx.PopupText("CHARGE SHOT!", pos + Vec3{ 0, 4, 0 }, WeaponColor(weapon));
}

// ------------------------------------------------------------
Box3 Player::LaserBox() const
{
    float half = 0.3f + weaponLevel * 0.25f;   // 레벨마다 굵어짐
    float z0 = pos.z + 4.5f;
    return { { pos.x, pos.y, (z0 + laserEndZ) * 0.5f }, { half, half, (laserEndZ - z0) * 0.5f } };
}

Box3 Player::GigaLaserBox() const
{
    float z0 = pos.z + 4.0f;
    return { { pos.x, pos.y, (z0 + gigaLaserEndZ) * 0.5f }, { 9.0f, 8.0f, (gigaLaserEndZ - z0) * 0.5f } };
}

float Player::OrbSpread() const
{
    float t = (FLAME_ORB_TIME - flameOrbTimer) / 0.4f;   // 방출 후 0.4초 동안 소용돌이치며 퍼짐
    return t >= 1.0f ? 1.0f : (t <= 0 ? 0 : t);
}

void Player::FlameOrbCenters(Vec3 out[4]) const
{
    // 기체 앞쪽(z+14)을 중심으로 4개가 가로로 긴 궤도를 돈다 -> 다가오는 적과 탄을 막는 방패
    float s = OrbSpread();
    for (int i = 0; i < 4; i++) {
        float a = orbAngle + i * (PI / 2);
        out[i] = pos + Vec3{ std::cos(a) * 14 * s, std::sin(a) * 9 * s, 14 };
    }
}

// ------------------------------------------------------------
void Player::Damage(Game& g, int damage)
{
#ifdef NEON_GODMODE
    return;   // 자동 테스트용 무적 빌드
#endif
    if (dead || Invincible()) return;
    hp -= damage;
    g.fx.Shake(1.0f, 0.2f);
    g.fx.Flash(0x40 /* 빨간 배경 */, 0.06f);
    if (hp <= 0) { Die(g); return; }
    hurtTimer = 1.0f;
    Audio::Play(S_PLAYER_HIT);
}

void Player::Die(Game& g)
{
    hp = 0; dead = true; respawnTimer = 2.0f; lives--;
    weaponLevel = 1; weapon = WeaponType::NORMAL; bombs = 3;   // 원본과 동일하게 무기/폭탄 초기화
    charging = false; chargeTimer = 0; missileQueue = 0; gigaLaserTimer = 0; flameOrbTimer = 0;
    Audio::Stop(S_CHARGE); Audio::Stop(S_GIGA_LASER); Audio::Stop(S_FLAME_ORB);
    g.fx.Explosion(pos, 2.5f, C_LCYAN);
    g.fx.Explosion(pos, 1.5f, C_LMAGENTA);
    g.fx.Shake(2.5f, 0.8f);
    Audio::Play(S_PLAYER_DIE);
}

void Player::AddExp(Game& g)
{
    if (++exp >= maxExp) {
        level++; hp = maxHp; exp = 0; maxExp = level * 10;   // 레벨업 시 HP 회복 (원본과 동일)
        g.fx.PopupText("LEVEL UP!", pos + Vec3{ 0, 3, 0 }, C_LCYAN);
        g.fx.Burst(pos, 25, 25, 0.5f, 0.6f, C_LCYAN, C_BWHITE);
        Audio::Play(S_LEVELUP);
    }
}

void Player::ApplyItem(Game& g, ItemType type)
{
    if (type == ItemType::FEVER) {
        fever = true; feverTimer = FEVER_TIME;
        g.fx.PopupText("FEVER TIME!", pos + Vec3{ 0, 4, 0 }, C_LYELLOW);
        Audio::Play(S_FEVER);
    } else if (type == ItemType::BOMB) {
        bombs++;
        g.fx.PopupText("+BOMB", pos + Vec3{ 0, 3, 0 }, C_LGREEN);
        Audio::Play(S_ITEM);
    } else {
        WeaponType w = (WeaponType)type;
        if (weapon == w) {
            if (weaponLevel < MAX_WEAPON_LEVEL) weaponLevel++;
            g.fx.PopupText(weaponLevel >= MAX_WEAPON_LEVEL ? "MAX POWER!" : "POWER UP", pos + Vec3{ 0, 3, 0 }, WeaponColor(w));
        } else {
            weapon = w; weaponLevel = 1;
            g.fx.PopupText(w == WeaponType::NORMAL ? "NORMAL" : w == WeaponType::FLAME ? "FLAME" : "LASER", pos + Vec3{ 0, 3, 0 }, WeaponColor(w));
        }
        Audio::Play(S_POWERUP);
    }
}

// ------------------------------------------------------------
void Player::Draw(const Game&) const
{
    if (dead) return;
    if (invincibleTimer > 0 && std::fmod(invincibleTimer, 0.2f) < 0.1f) return;   // 부활 무적 깜빡임

    // --- 레이저 / 기가 레이저 / 화염구 ---
    // 빔은 depthBias를 크게(+) 줘서 빔 안에 들어온 적이 빔에 가려지지 않게 함
    if (laserOn) {
        Box3 b = LaserBox();
        R3D::DrawBox(b.center, b.half * 2.0f, { C_YELLOW, C_LYELLOW }, "-=", 60);
        R3D::DrawBox(b.center, b.half, { C_LYELLOW, C_BWHITE }, "=#", 59);
    }
    if (GigaLaserOn()) {
        // 기가 레이저 = 앞으로 흘러가는 사각 고리들로 된 '에너지 터널' + 가운데 굵은 코어
        // (상자로 꽉 채우면 앞면이 화면을 덮어 버려서, 테두리만 그려 안이 비쳐 보이게 함)
        Box3 b = GigaLaserBox();
        float z0 = b.center.z - b.half.z, z1 = b.center.z + b.half.z;
        float scroll = std::fmod(time * 120.0f, 10.0f);
        int k = 0;
        for (float z = z0 + scroll; z < z1; z += 10, k++) {
            float hx = b.half.x + RandF(-0.8f, 0.8f), hy = b.half.y + RandF(-0.8f, 0.8f);   // 불안정하게 떨림
            Vec3 c = { b.center.x, b.center.y, z };
            Vec3 p[4] = { c + Vec3{ -hx, -hy, 0 }, c + Vec3{ hx, -hy, 0 }, c + Vec3{ hx, hy, 0 }, c + Vec3{ -hx, hy, 0 } };
            uint16_t col = (k % 3 == 0) ? C_LYELLOW : (k % 3 == 1 ? C_LRED : C_LCYAN);
            for (int e = 0; e < 4; e++) R3D::DrawLine(p[e], p[(e + 1) % 4], '#', col, 20);
        }
        for (int e = 0; e < 4; e++) {   // 터널 모서리 4줄 (원근이 잘 보이게)
            float sx = (e == 0 || e == 3) ? -1.0f : 1.0f, sy = (e < 2) ? -1.0f : 1.0f;
            R3D::DrawLine({ b.center.x + sx * b.half.x, b.center.y + sy * b.half.y, z0 },
                          { b.center.x + sx * b.half.x, b.center.y + sy * b.half.y, z1 }, '=', C_LRED, 20);
        }
        R3D::DrawBox(b.center, { 2.6f, 2.2f, b.half.z }, { C_YELLOW, C_LYELLOW }, ":=", 40);
        R3D::DrawBox(b.center, { 1.0f, 1.0f, b.half.z }, { C_BWHITE, C_BWHITE }, "X@", 39);
    }
    if (FlameOrbsOn()) {
        Vec3 c[4];
        FlameOrbCenters(c);
        float flick = 1.0f + std::sin(time * 30) * 0.1f;
        for (auto& p : c) {
            R3D::DrawBlob(p, FLAME_ORB_RADIUS * flick, '#', C_RED, 1.0f);
            R3D::DrawBlob(p, FLAME_ORB_RADIUS * 0.7f, '%', C_LYELLOW, 0.5f);
            R3D::DrawBlob(p, FLAME_ORB_RADIUS * 0.35f, '@', C_BWHITE, 0.0f);
        }
    }

    // --- 기체 ---
    Material mats[3] = { { C_CYAN, C_LCYAN }, { C_BLUE, C_LBLUE }, { C_LMAGENTA, C_BWHITE } };
    bool flash = false;
    if (fever) { uint16_t r = RainbowColor(time); mats[0] = { r, r }; }
    if (hurtTimer > 0 && std::fmod(hurtTimer, 0.16f) < 0.08f) { mats[0] = { C_RED, C_LRED }; mats[1] = mats[0]; }
    if (charging && chargeTimer >= MAX_CHARGE_TIME) flash = std::fmod(time, 0.12f) < 0.06f;   // 풀 차징 번쩍임
    R3D::DrawMesh(Meshes::Get(M_PLAYER), pos, { -vel.y * 0.15f, 0, bank }, 1.3f, mats, flash);

    // 차지 게이지 (기체 아래)
    if (charging) {
        char bar[16];
        int n = (int)(chargeTimer / MAX_CHARGE_TIME * 10);
        for (int i = 0; i < 10; i++) bar[i] = i < n ? '=' : ' ';
        bar[10] = 0;
        char s[24];
        snprintf(s, sizeof(s), "[%s]", bar);
        R3D::DrawLabel(pos + Vec3{ 0, -2.5f, 0 }, 1, s, n >= 10 ? C_BWHITE : WeaponColor(weapon));
    }
}
