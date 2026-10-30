#pragma once
// ============================================================
//  3D 배경: 원본의 3레이어 별(원경 30 / 중경 10 / 근경 3)을 '앞에서 다가오는 별'로 바꿈
//  + 바닥 격자 (원근감을 주는 네온 그리드)
// ============================================================
#include "Math3D.h"

struct Star {
    Vec3 pos;
    float speed;
    char shape;
    unsigned short color;
};

class Background {
public:
    void Init();
    void Update(float dt, float speedScale = 1.0f);
    void Draw() const;

private:
    Star  farLayer[60];
    Star  midLayer[25];
    Star  nearLayer[6];
    float gridScroll = 0;
};
