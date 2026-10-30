#include "Bullet.h"
#include "Game.h"
#include "Audio.h"
#include "Render3D.h"

float Bullet::MissileRingRadius() const
{
    // NEON: 세로 반폭이 24px에서 시작해 260px까지 (플레이 영역의 대부분)
    // 3D: 링 반지름 2에서 시작해 최대 16 (플레이어 이동 범위 세로 ±14를 덮음)
    float r = 2.0f + (pos.z - startZ) * 0.14f;
    return r > 16.0f ? 16.0f : r;
}

void BulletSystem::Reset()
{
    for (auto& b : pool) b.active = false;
    searchStart = 0;
}

Bullet* BulletSystem::Spawn(BulletType type, Vec3 pos, Vec3 vel, int damage, uint16_t color, float life)
{
    for (int n = 0; n < BULLET_COUNT; n++) {
        int i = (searchStart + n) % BULLET_COUNT;
        if (pool[i].active) continue;
        searchStart = (i + 1) % BULLET_COUNT;
        Bullet& b = pool[i];
        b.type = type; b.pos = pos; b.prev = pos; b.vel = vel;
        b.age = 0; b.life = life; b.startZ = pos.z;
        b.damage = damage; b.color = color; b.active = true;
        switch (type) {
            case BulletType::P_NORMAL:   b.radius = 0.5f; break;
            case BulletType::P_FLAME:    b.radius = 1.2f; break;
            case BulletType::P_MISSILE:  b.radius = 2.0f; break;
            case BulletType::P_BOMB:     b.radius = 1.5f; break;
            case BulletType::P_SHRAPNEL: b.radius = 0.8f; break;
            case BulletType::E_SMALL:    b.radius = 0.7f; break;
            case BulletType::E_BOSS:     b.radius = 0.9f; break;
        }
        return &b;
    }
    return nullptr;
}

void BulletSystem::ClearEnemyBullets(Game& g, bool withEffect)
{
    for (auto& b : pool) {
        if (!b.active || b.IsPlayerSide()) continue;
        if (withEffect) g.fx.Spawn(b.pos, {}, 0.3f, 1.0f, 0.1f, C_BWHITE, C_GRAY);
        b.active = false;
    }
}

void BulletSystem::ShatterBomb(Game& g, Vec3 pos)
{
    ClearEnemyBullets(g, true);   // 폭탄은 적 탄을 '두 번' 제거 (투척 시 + 폭발 시) - 원본과 동일
    const int fireworkCount = 200;
    for (int i = 0; i < fireworkCount; i++) {
        // 구면에 고르게 퍼지도록 (피보나치 구 배치)
        float y = 1 - (i + 0.5f) * 2.0f / fireworkCount, r = std::sqrt(1 - y * y), a = i * 2.39996f;
        Vec3 d = { std::cos(a) * r, y, std::sin(a) * r };
        Spawn(BulletType::P_SHRAPNEL, pos, d * RandF(40, 65), 5, (i % 3) ? C_LGREEN : C_LCYAN, 1.2f);
    }
    g.fx.Explosion(pos, 3.0f, C_LGREEN);
    g.fx.Shake(2.0f, 0.6f);
    g.fx.Flash(0x20 /* 초록 배경 */, 0.12f);
    Audio::Play(S_BOMB_EXPLODE);
}

void BulletSystem::Update(Game& g, float dt)
{
    for (auto& b : pool) {
        if (!b.active) continue;
        b.age += dt;
        b.prev = b.pos;
        switch (b.type) {
        case BulletType::P_SHRAPNEL: b.vel = b.vel * (1.0f / (1.0f + 1.6f * dt)); break;   // 감속
        case BulletType::P_FLAME:
            b.vel = b.vel * (1.0f / (1.0f + 1.5f * dt));
            if (RandI(0, 7) == 0) g.fx.Spawn(b.pos, { RandF(-2, 2), RandF(1, 4), 0 }, 0.3f, 0.4f, 0.1f, C_LRED, C_RED);
            break;
        case BulletType::P_BOMB:
            if (b.age >= b.life) { b.active = false; ShatterBomb(g, b.pos); continue; }
            break;
        case BulletType::P_MISSILE: {
            // 링 전체에서 뒤로 분사 화염
            float r = b.MissileRingRadius();
            for (int k = 0; k < 2; k++) {
                float a = RandF(0, 2 * PI);
                g.fx.Spawn(b.pos + Vec3{ std::cos(a) * r, std::sin(a) * r, -1.5f }, { 0, 0, -20 },
                           RandF(0.15f, 0.3f), 0.7f, 0.1f, C_LCYAN, C_BLUE, 2.0f);
            }
            break;
        }
        default: break;
        }
        b.pos += b.vel * dt;
        // 적 탄은 플레이어 평면을 지나치면 제거 (카메라 앞에서 커다랗게 보이지 않도록)
        bool passed = !b.IsPlayerSide() && b.pos.z < g.player.pos.z - 4;
        if (passed || b.age >= b.life || b.pos.z > FAR_Z || b.pos.z < -40 ||
            std::fabs(b.pos.x) > 140 || std::fabs(b.pos.y) > 90) b.active = false;
    }
}

void BulletSystem::Draw() const
{
    static const Material MISSILE_MAT[3] = { { C_LCYAN, C_BWHITE }, { C_LCYAN, C_BWHITE }, { C_LCYAN, C_BWHITE } };
    static const Material BOMB_MAT[3] = { { C_GREEN, C_LGREEN }, { C_GREEN, C_LGREEN }, { C_GREEN, C_LGREEN } };
    for (const auto& b : pool) {
        if (!b.active) continue;
        float t = b.age / b.life;
        switch (b.type) {
        case BulletType::P_NORMAL:
            // 진행 방향으로 늘어진 짧은 선 (빠르게 날아가는 느낌)
            R3D::DrawPoint(b.pos, '|', b.color);
            R3D::DrawPoint(b.pos - b.vel * 0.015f, ':', C_CYAN);
            break;
        case BulletType::P_FLAME: {
            // 흰색 -> 노랑 -> 빨강으로 식으며 커짐 (원본 화염 색 단계)
            uint16_t c = t < 0.2f ? C_BWHITE : (t < 0.5f ? C_LYELLOW : (t < 0.8f ? C_LRED : C_RED));
            char ch = t < 0.3f ? '@' : (t < 0.6f ? '*' : '+');
            R3D::DrawBlob(b.pos, 0.2f + t * 0.5f, ch, c);
            break;
        }
        case BulletType::P_MISSILE: {
            // 기가 미사일: 링 모양으로 배치된 미사일 14개 (전진할수록 링이 커짐)
            float r = b.MissileRingRadius();
            for (int k = 0; k < 14; k++) {
                float a = k * 2 * PI / 14 + b.age * 2.0f;
                Vec3 p = b.pos + Vec3{ std::cos(a) * r, std::sin(a) * r, -r * 0.15f };
                R3D::DrawMesh(Meshes::Get(M_MISSILE), p, {}, 1.4f, MISSILE_MAT, false, nullptr, -1.0f);
            }
            R3D::DrawBlob(b.pos, 1.0f, '+', C_LCYAN);
            break;
        }
        case BulletType::P_BOMB:
            R3D::DrawMesh(Meshes::Get(M_BOMB), b.pos, { b.age * 6, b.age * 9, 0 }, 1.3f, BOMB_MAT);
            break;
        case BulletType::P_SHRAPNEL:
            R3D::DrawPoint(b.pos, t < 0.5f ? '*' : '.', b.color);
            break;
        case BulletType::E_SMALL:
            R3D::DrawBlob(b.pos, b.radius, 'o', b.color, -0.5f);
            R3D::DrawBlob(b.pos, 0, '@', C_BWHITE, -1.0f);   // 중심은 흰색 (원본처럼 눈에 띄게)
            break;
        case BulletType::E_BOSS:
            R3D::DrawBlob(b.pos, b.radius, 'O', b.color, -0.5f);
            R3D::DrawBlob(b.pos, 0, '@', C_BWHITE, -1.0f);
            break;
        }
    }
}
