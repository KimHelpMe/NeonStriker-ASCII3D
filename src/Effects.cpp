#include "Effects.h"
#include "Render3D.h"
#include "Screen.h"
#include <cstring>

void Effects::Reset()
{
    for (auto& p : particles) p.active = false;
    for (auto& p : popups) p.life = 0;
    shakeTime = 0; flashTime = 0; shakeOffset = {};
}

void Effects::Spawn(Vec3 pos, Vec3 vel, float life, float size, float endSize, uint16_t hot, uint16_t cool, float drag)
{
    Particle& p = particles[cursor];
    cursor = (cursor + 1) % PARTICLE_COUNT;   // 가득 차면 가장 오래된 것부터 재사용
    p = { pos, vel, life, life, size, endSize, drag, hot, cool, true };
}

void Effects::Burst(Vec3 pos, int count, float speed, float size, float life, uint16_t hot, uint16_t cool)
{
    for (int i = 0; i < count; i++) {
        Vec3 d = Normalize({ RandF(-1, 1), RandF(-1, 1), RandF(-1, 1) });
        Spawn(pos, d * RandF(speed * 0.3f, speed), RandF(life * 0.5f, life), size, 0.1f, hot, cool, 2.5f);
    }
}

void Effects::Explosion(Vec3 pos, float scale, uint16_t color)
{
    Spawn(pos, {}, 0.2f, 2.5f * scale, 4.0f * scale, C_BWHITE, C_LYELLOW);              // 중심 섬광
    for (int i = 0; i < (int)(14 * scale); i++) {                                       // 불덩이
        Vec3 d = Normalize({ RandF(-1, 1), RandF(-1, 1), RandF(-1, 1) });
        Spawn(pos, d * RandF(4, 18) * scale, RandF(0.4f, 0.8f), RandF(0.8f, 1.6f) * scale, 0.2f, C_LYELLOW, color, 2.5f);
    }
    for (int i = 0; i < (int)(18 * scale); i++) {                                       // 파편
        Vec3 d = Normalize({ RandF(-1, 1), RandF(-1, 1), RandF(-1, 1) });
        Spawn(pos, d * RandF(20, 45) * scale, RandF(0.3f, 0.6f), 0.3f, 0.1f, C_BWHITE, color, 3.0f);
    }
}

void Effects::Shake(float amount, float time)
{
    if (amount > shakeAmount || shakeTime <= 0) shakeAmount = amount;
    if (time > shakeTime) shakeTime = time;
}

void Effects::Flash(uint16_t bgAttr, float time) { flashAttr = bgAttr; flashTime = time; }

void Effects::PopupText(const char* text, Vec3 pos, uint16_t color)
{
    for (auto& p : popups) {
        if (p.life > 0) continue;
        strncpy(p.text, text, sizeof(p.text) - 1);
        p.text[sizeof(p.text) - 1] = 0;
        p.pos = pos; p.life = 1.2f; p.color = color;
        return;
    }
}

void Effects::Update(float dt)
{
    for (auto& p : particles) {
        if (!p.active) continue;
        p.life -= dt;
        if (p.life <= 0) { p.active = false; continue; }
        float k = 1.0f / (1.0f + p.drag * dt);   // 공기 저항
        p.vel = p.vel * k;
        p.pos += p.vel * dt;
    }
    for (auto& p : popups) {
        if (p.life <= 0) continue;
        p.life -= dt;
        p.pos.y += 4.0f * dt;
    }
    if (shakeTime > 0) {
        shakeTime -= dt;
        shakeOffset = { RandF(-shakeAmount, shakeAmount), RandF(-shakeAmount, shakeAmount) * 0.6f, 0 };
        if (shakeTime <= 0) { shakeOffset = {}; shakeAmount = 0; }
    }
    if (flashTime > 0) flashTime -= dt;
}

void Effects::Draw() const
{
    static const char RAMP[] = "@#*+:.";   // 수명이 줄수록 가벼운 글자
    for (const auto& p : particles) {
        if (!p.active) continue;
        float t = 1.0f - p.life / p.maxLife;   // 0 -> 1
        int idx = (int)(t * 5.99f);
        float size = p.size + (p.endSize - p.size) * t;
        R3D::DrawBlob(p.pos, size, RAMP[idx], t < 0.45f ? p.hot : p.cool, -0.5f);
    }
}

void Effects::DrawOverlay() const
{
    for (const auto& p : popups) {
        if (p.life <= 0) continue;
        if (p.life < 0.3f && (int)(p.life * 20) % 2) continue;   // 사라지기 직전 깜빡임
        R3D::DrawLabel(p.pos, -3, p.text, p.color);
    }
    if (flashTime > 0) Screen::Tint(flashAttr);
}
