#pragma once
// ============================================================
//  3D 파티클 / 화면 흔들림 / 번쩍임 / 떠오르는 글자
//  파티클은 수명에 따라 글자가 바뀜:  @ -> # -> * -> + -> : -> .   (뜨거운 색 -> 식은 색)
// ============================================================
#include "Math3D.h"
#include "Config.h"

struct Particle {
    Vec3  pos, vel;
    float life, maxLife;
    float size, endSize;   // 월드 단위 반지름
    float drag;
    uint16_t hot, cool;    // 수명 앞부분 색 / 뒷부분 색
    bool  active;
};

struct Popup {
    char  text[32];
    Vec3  pos;
    float life;
    uint16_t color;
};

class Effects {
public:
    void Reset();
    void Update(float dt);
    void Draw() const;          // 3D 파티클 (깊이 테스트)
    void DrawOverlay() const;   // 팝업 글자 + 화면 번쩍임

    void Spawn(Vec3 pos, Vec3 vel, float life, float size, float endSize, uint16_t hot, uint16_t cool, float drag = 0);
    void Burst(Vec3 pos, int count, float speed, float size, float life, uint16_t hot, uint16_t cool);
    void Explosion(Vec3 pos, float scale, uint16_t color);
    void Shake(float amount, float time);
    void Flash(uint16_t bgAttr, float time);
    void PopupText(const char* text, Vec3 pos, uint16_t color);

    Vec3 ShakeOffset() const { return shakeOffset; }

private:
    Particle particles[PARTICLE_COUNT];
    int      cursor = 0;
    Popup    popups[12];
    float    shakeAmount = 0, shakeTime = 0;
    Vec3     shakeOffset{};
    uint16_t flashAttr = 0;
    float    flashTime = 0;
};
