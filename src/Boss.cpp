#include "Boss.h"
#include "Game.h"
#include "Audio.h"
#include "Render3D.h"
#include "Screen.h"
#include "OriginalArt.h"
#include <cstring>
#include <cstdio>
#include <initializer_list>

static const float    EXPLODE_TIME = 3.0f;
static const float    BASE_Z = 80.0f;    // 전투 거리
static const MeshId   MESH[5] = { M_BOSS1, M_BOSS1, M_BOSS2, M_BOSS3, M_BOSS4 };
static const uint16_t COLOR[5] = { C_WHITE, C_LMAGENTA, C_LGREEN, C_LYELLOW, C_LRED };
static const Material MAT[5][3] = {
    {}, { { C_MAGENTA, C_LMAGENTA }, { C_BLUE, C_LBLUE }, { C_WHITE, C_BWHITE } },
    { { C_GREEN, C_LGREEN }, { C_CYAN, C_LCYAN }, { C_WHITE, C_BWHITE } },
    { { C_YELLOW, C_LYELLOW }, { C_RED, C_LRED }, { C_WHITE, C_BWHITE } },
    { { C_RED, C_LRED }, { C_MAGENTA, C_LMAGENTA }, { C_LYELLOW, C_BWHITE } },
};

void Boss::Init(int stage)
{
    state = BossState::NONE; defeated = false; killCount = 0;
    step = stage < 1 ? 1 : (stage > 4 ? 4 : stage);
    warningTimer = explodeTimer = moveAcc = fireTimer = invincibleTimer = flash = 0;
    burstCount = 0;
}

void Boss::AddKillCount(Game& g)
{
    if (Present()) return;
    if (++killCount >= g.Rules().killRequirement) ForceAppear(g);
}

void Boss::ForceAppear(Game& g)
{
    if (Present()) return;
    state = BossState::WARNING;
    warningTimer = 3.0f;
    g.monsters.enabled = false;
    g.monsters.Reset();
    g.bullets.ClearEnemyBullets(g, true);
    int baseHp = (step == 4) ? 1500 : 300 * step;   // 원본 보스 체력
    maxHp = hp = (int)(baseHp * g.Rules().bossHpScale);
    pos = { 0, 6, 260 };
    fireTimer = 0;
    Audio::Play(S_WARNING);
}

Vec3 Boss::Half() const
{
    static const Vec3 H[5] = { {}, { 16, 6, 11 }, { 15, 11, 11 }, { 16, 13, 11 }, { 18, 18, 14 } };   // 모델 크기 x 1.8
    return H[step];
}

bool Boss::Overlaps(Vec3 p, float r) const
{
    Vec3 h = Half();
    return std::fabs(p.x - pos.x) <= h.x + r && std::fabs(p.y - pos.y) <= h.y + r && std::fabs(p.z - pos.z) <= h.z + r;
}

bool Boss::Damage(Game& g, int damage, float invincible)
{
    if (state != BossState::FIGHT || invincibleTimer > 0) return false;
    hp -= damage;
    // 피격 번쩍임은 0.2초에 한 번만 (계속 맞으면 하얗게만 보여서 보스 고유색이 안 보임)
    if (flashCooldown <= 0) { flash = 0.05f; flashCooldown = 0.2f; }
    invincibleTimer = invincible;
    if (RandI(0, 3) == 0) Audio::Play(S_HIT, 0.4f, 0.1f);
    if (hp <= 0) {
        hp = 0;
        state = BossState::EXPLODING;
        explodeTimer = EXPLODE_TIME;
        g.bullets.ClearEnemyBullets(g, true);
        Audio::Play(S_BOSS_EXPLODE);
        g.fx.Shake(1.5f, EXPLODE_TIME);
    }
    return true;
}

void Boss::Update(Game& g, float dt)
{
    time += dt;
    if (invincibleTimer > 0) invincibleTimer -= dt;
    if (flash > 0) flash -= dt;
    if (flashCooldown > 0) flashCooldown -= dt;

    if (state == BossState::EXPLODING) {
        explodeTimer -= dt;
        Vec3 h = Half();
        for (int i = 0; i < 3; i++)   // 몸 전체에서 솟는 불길 (원본: 위로 갈수록 퍼지는 불꽃)
            g.fx.Spawn(pos + Vec3{ RandF(-h.x, h.x), RandF(-h.y, h.y), RandF(-h.z, 0) }, { RandF(-4, 4), RandF(8, 20), 0 },
                       RandF(0.4f, 0.9f), RandF(1.5f, 3.0f), 0.3f, C_LYELLOW, C_RED);
        if (RandI(0, 6) == 0) {
            g.fx.Explosion(pos + Vec3{ RandF(-h.x, h.x), RandF(-h.y, h.y), -h.z }, 1.5f, COLOR[step]);
            Audio::Play(S_EXPLODE_MID, 0.6f, 0.2f);
        }
        if (explodeTimer <= 0.6f && explodeTimer + dt > 0.6f) {   // 마지막 0.6초: 백색 대폭발
            g.fx.Explosion(pos, 6.0f, C_BWHITE);
            g.fx.Explosion(pos, 4.0f, COLOR[step]);
            g.fx.Flash(0x70 /* 흰 배경 */, 0.25f);
            g.fx.Shake(3.0f, 0.8f);
        }
        if (explodeTimer <= 0) { state = BossState::NONE; defeated = true; }
        return;
    }

    if (state == BossState::WARNING) {   // 먼 곳에서 전투 거리까지 진입
        warningTimer -= dt;
        pos.z += (BASE_Z - pos.z) * Clampf(dt * 1.2f, 0, 1);
        if (warningTimer <= 0) { state = BossState::FIGHT; moveAcc = 0; }
        return;
    }
    if (state != BossState::FIGHT) return;

    // ---- 이동: 원본의 8자 패턴을 상하좌우 평면에서 ----
    moveAcc += dt;
    if (step == 4) {
        float tri = moveAcc * 1.5f;
        pos.x = std::sin(tri) * 18;
        pos.y = 4 + std::cos(tri * 0.5f) * 10;
        pos.z = BASE_Z + std::sin(moveAcc * 0.7f) * 15;   // 4단계는 앞뒤로도 움직임
    } else {
        float sx = 1.2f + step * 0.2f, sy = 0.8f + step * 0.1f;
        pos.x = std::sin(moveAcc * sx) * (10 + step * 3);
        pos.y = 5 + std::cos(moveAcc * sy) * 7;
        pos.z = BASE_Z;
    }
    if (step >= 2) { pos.x += RandF(-0.2f, 0.2f); pos.y += RandF(-0.2f, 0.2f); }   // 과부하 진동

    fireTimer += dt;
    if (fireTimer > 1.5f / step) { Fire(g); fireTimer = 0; }   // 원본 발사 간격 1.5초 / 단계

    if (g.player.Alive() && Overlaps(g.player.pos, Player::HIT_RADIUS)) g.player.Damage(g, 1);
}

void Boss::Fire(Game& g)
{
    Vec3 fp = pos - Vec3{ 0, 0, Half().z };
    Vec3 to = Normalize(g.player.pos - fp);
    const float speed = 40.0f;
    uint16_t c = COLOR[step];
    // to(조준) 방향 또는 정면(-z)에서 좌우(yaw)/위아래(pitch)로 비튼 방향으로 발사
    auto Shot = [&](Vec3 base, float yaw, float pitch, float spd, uint16_t col) {
        Vec3 d = Normalize(base + Vec3{ yaw, pitch, 0 });
        g.bullets.Spawn(BulletType::E_BOSS, fp, d * spd, 1, col, 10);
    };
    Vec3 fwd = Normalize(Vec3{ g.player.pos.x - fp.x, g.player.pos.y - fp.y, -BASE_Z } * 1.0f);

    if (step == 1) {
        for (float a = -0.32f; a <= 0.33f; a += 0.16f) Shot(fwd, a, 0, speed * 0.9f, c);   // 5방향 확산
        Shot(to, 0, 0, speed * 1.15f, C_BWHITE);                                              // 조준탄
        if (RandI(0, 99) < 10) Shot(fwd, RandF(-0.3f, 0.3f), RandF(-0.2f, 0.2f), speed, c);   // 10% 무작위탄
    } else if (step == 2) {
        float wave = std::sin(moveAcc * 5.0f) * 0.2f;                                         // 상하 웨이브
        for (float a = -0.32f; a <= 0.33f; a += 0.16f) Shot(fwd, a, wave, speed, c);
        for (int i = 0; i < 2; i++) Shot(fwd, RandF(-0.4f, 0.4f), RandF(-0.3f, 0.3f), speed * 0.9f, c);
    } else if (step == 3) {
        burstCount++;
        for (float a = -0.4f; a <= 0.41f; a += 0.1f) Shot(fwd, 0, a, speed, c);              // 넓은 세로 탄막
        for (float a = -0.4f; a <= 0.41f; a += 0.2f) Shot(fwd, a, 0, speed, c);              // + 가로
        if (burstCount % 2 == 0) Shot(to, 0, 0, speed * 1.3f, C_BWHITE);
    } else {
        // 4단계 소용돌이: 6갈래 링이 회전하며 플레이어 쪽으로 밀려옴 -> 3D에서는 '터널' 모양
        spiralAngle += 0.45f;
        for (int i = 0; i < 6; i++) {
            float a = spiralAngle + i * (2 * PI / 6);
            Vec3 d = Normalize(Vec3{ std::cos(a) * 0.35f, std::sin(a) * 0.3f, -1 });
            g.bullets.Spawn(BulletType::E_BOSS, fp, d * speed * 0.9f, 1, C_LMAGENTA, 10);
        }
        float flicker = std::cos(moveAcc * 10.0f) * 0.1f;                                    // 깜빡이 탄막
        for (float a = -0.45f; a <= 0.46f; a += 0.15f) Shot(fwd, a, flicker, speed, c);
        for (float off : { -0.08f, 0.0f, 0.08f }) Shot(to, off, 0, speed * 1.5f, C_BWHITE);  // 3갈래 저격
    }
    g.fx.Spawn(fp, {}, 0.15f, 3.0f, 0.5f, C_BWHITE, c);
}

void Boss::Draw() const
{
    if (state == BossState::NONE) return;
    if (state == BossState::EXPLODING && explodeTimer < 0.6f) return;   // 백색 폭발 때 본체 소멸
    Vec3 p = pos;
    if (state == BossState::EXPLODING) p += Vec3{ RandF(-1, 1), RandF(-1, 1), 0 };   // 원본: 폭발 중 흔들림
    Vec3 rot = { std::sin(time * 0.7f) * 0.15f, std::sin(time * 0.5f) * 0.3f, std::sin(time) * 0.1f };
    if (step == 4) rot = { time * 0.3f, time * 0.8f, 0 };   // 가시 코어는 계속 회전
    R3D::DrawMesh(Meshes::Get(MESH[step]), p, rot, 1.8f, MAT[step], flash > 0);
}

void Boss::DrawUI() const
{
    if (state == BossState::WARNING) {
        // ===== 원본 등장 경고: 화면 테두리 깜빡임 + 노란 박스 안의 WARNING 아트 =====
        bool on = (int)(warningTimer * 10) % 2 == 0;
        Screen::Box(0, 0, SCR_W, VIEW_H, '#', on ? C_LRED : C_MAGENTA);
        if ((int)(warningTimer * 5) % 2 == 0) {
            int w = Screen::ArtWidth(WARNING_ART, WARNING_ART_H);
            int bx = (SCR_W - w - 10) / 2, by = VIEW_H / 2 - 9;
            Screen::FillRect(bx, by, w + 10, 15, ' ', C_WHITE);
            Screen::Box(bx, by, w + 10, 15, '#', C_LYELLOW);
            Screen::ArtC(by + 2, WARNING_ART, WARNING_ART_H, C_LRED);
            Screen::TextC(by + 11, "[ WARNING : BOSS APPROACHING ]", C_LYELLOW);
            char s[64];
            snprintf(s, sizeof(s), "-  %s  -", BOSS_NAMES[step]);
            Screen::TextC(by + 12, s, C_BWHITE);
        }
        return;
    }
    if (state == BossState::FIGHT || state == BossState::EXPLODING) {
        // ===== 원본 HP 바: [ BOSS HP: x / y ]  [####....] =====
        char bar[101];
        int n = hp * 100 / (maxHp > 0 ? maxHp : 1);
        for (int i = 0; i < 100; i++) bar[i] = i < n ? '#' : '.';
        bar[100] = 0;
        char s[200];
        snprintf(s, sizeof(s), "%s  [ BOSS HP: %d / %d ]  [%s]", BOSS_NAMES[step], hp, maxHp, bar);
        Screen::TextC(1, s, COLOR[step]);
    }
}
