#pragma once
// ============================================================
//  3D 모델 (삼각형 목록)
//  이미지 파일 대신 꼭짓점 좌표를 코드로 정의한다.
//  좌표계: x = 오른쪽, y = 위, z = 화면 안쪽(앞)
//  플레이어는 +z를 향하고, 적은 플레이어 쪽(-z)을 향한다.
// ============================================================
#include "Math3D.h"
#include <cstdint>
#include <vector>

struct Tri {
    Vec3    a, b, c;
    uint8_t mat;   // 재질 번호 (그릴 때 넘기는 색 팔레트의 인덱스: 0 본체 / 1 포인트 / 2 발광)
};

struct Mesh {
    std::vector<Tri> tris;
};

enum MeshId {
    M_PLAYER, M_EN_NORMAL, M_EN_MEDIUM, M_EN_HEAVY,
    M_BOSS1, M_BOSS2, M_BOSS3, M_BOSS4,
    M_ITEM, M_BOMB, M_MISSILE, M_CUBE,
    M_COUNT
};

namespace Meshes {
    void Build();
    const Mesh& Get(MeshId id);
}
