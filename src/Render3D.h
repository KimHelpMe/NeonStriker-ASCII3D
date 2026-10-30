#pragma once
// ============================================================
//  아스키 3D 렌더러
//
//  [한 프레임에 하는 일]
//  1) 모델 좌표 -> 회전/크기/이동 -> 월드 좌표
//  2) 월드 좌표 - 카메라 위치 -> 카메라 기준 좌표
//  3) 원근 투영: 화면x = 중심 + x / z * F   (멀수록 z가 커져서 작게 보임)
//     콘솔 글자는 세로가 가로의 약 2배라서 y에는 0.5를 곱해 비율을 맞춤
//  4) 삼각형을 칸 단위로 채우면서, 칸마다 깊이를 비교해 가까운 것만 남김 (Z-버퍼)
//  5) 면이 빛을 받는 정도(법선 · 빛 방향)를 글자 밀도로 바꿈:  . , : ; - = + * # % @
// ============================================================
#include "Math3D.h"
#include "Mesh.h"
#include "Screen.h"

// 재질: 어두운 면 색 / 밝은 면 색
struct Material {
    uint16_t dark, bright;
};

namespace R3D {
    void SetCamera(Vec3 pos);
    Vec3 CameraPos();

    // 월드 좌표를 화면 칸 좌표로. 카메라 뒤면 false
    bool Project(Vec3 p, float& sx, float& sy, float& depth);
    float ScreenScale(float depth);   // 깊이 depth에서 월드 1단위가 몇 칸인지

    // mats: 재질 배열 (Tri.mat 인덱스로 참조). flash면 전부 밝은 흰색
    // ramp가 있으면 조명 대신 그 글자들로 칠함 (빔 등 발광체). depthBias < 0이면 겹칠 때 앞에 그려짐
    void DrawMesh(const Mesh& m, Vec3 pos, Vec3 rot, float scale, const Material* mats,
                  bool flash = false, const char* ramp = nullptr, float depthBias = 0);

    // 직육면체를 바로 그리기 (레이저 빔 등)
    void DrawBox(Vec3 center, Vec3 half, Material mat, const char* ramp, float depthBias = 0);

    void DrawPoint(Vec3 p, char c, uint16_t attr);
    void DrawLine(Vec3 a, Vec3 b, char c, uint16_t attr, float depthBias = 0);   // 3D 선분
    // 화면에서 원 모양으로 칠한 덩어리 (탄, 파티클). radius는 월드 단위
    void DrawBlob(Vec3 p, float radius, char c, uint16_t attr, float depthBias = 0);
    void DrawLabel(Vec3 p, float yOffsetRows, const char* s, uint16_t attr);   // 3D 위치 위에 글자
}
