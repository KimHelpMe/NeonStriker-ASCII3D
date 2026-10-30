#include "Background.h"
#include "Render3D.h"
#include "Config.h"

static void Respawn(Star& s, bool anywhere)
{
    s.pos = { RandF(-160, 160), RandF(-80, 90), anywhere ? RandF(5, 400) : RandF(350, 420) };
}

void Background::Init()
{
    // 원경: 작고 느린 회색 점 / 중경: 파란 별 / 근경: 빠르게 스쳐 가는 흰 파편 (원본과 같은 구성)
    for (auto& s : farLayer)  { Respawn(s, true); s.speed = RandF(20, 35);   s.shape = '.'; s.color = C_GRAY; }
    for (auto& s : midLayer)  { Respawn(s, true); s.speed = RandF(50, 80);   s.shape = '*'; s.color = C_LBLUE; }
    for (auto& s : nearLayer) { Respawn(s, true); s.speed = RandF(140, 200); s.shape = 'O'; s.color = C_WHITE; }
}

void Background::Update(float dt, float speedScale)
{
    auto UpdateLayer = [&](Star* layer, int n) {
        for (int i = 0; i < n; i++) {
            layer[i].pos.z -= layer[i].speed * speedScale * dt;
            if (layer[i].pos.z < R3D::CameraPos().z + 2) Respawn(layer[i], false);   // 카메라를 지나치면 멀리서 다시
        }
    };
    UpdateLayer(farLayer, 60);
    UpdateLayer(midLayer, 25);
    UpdateLayer(nearLayer, 6);
    gridScroll = std::fmod(gridScroll + 30.0f * speedScale * dt, 18.0f);
}

void Background::Draw() const
{
    // 바닥 격자 (y = -24 평면). 깊이 테스트를 하므로 물체 뒤에 가려짐
    const float FLOOR_Y = -32.0f;
    for (float x = -128; x <= 128; x += 16)
        for (float z = 0; z < 320; z += 6)
            R3D::DrawPoint({ x, FLOOR_Y, z - gridScroll }, '.', z < 140 ? C_MAGENTA : C_BLUE);
    for (float z = 18 - gridScroll; z < 320; z += 18)
        for (float x = -128; x <= 128; x += 3)
            R3D::DrawPoint({ x, FLOOR_Y, z }, z < 100 ? '-' : '.', z < 140 ? C_MAGENTA : C_BLUE);

    for (const auto& s : farLayer) R3D::DrawPoint(s.pos, s.shape, s.color);
    for (const auto& s : midLayer) R3D::DrawPoint(s.pos, s.shape, s.color);
    for (const auto& s : nearLayer) {
        // 근경은 가까워질수록 커지고 꼬리가 생김
        R3D::DrawBlob(s.pos, 0.4f, s.shape, C_BWHITE);
        for (int k = 1; k <= 3; k++) R3D::DrawPoint(s.pos + Vec3{ 0, 0, k * 4.0f }, '.', C_GRAY);
    }
}
