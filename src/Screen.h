#pragma once
// ============================================================
//  화면 버퍼 (원본 DoubleBuffering의 g_szBuffer 역할) + 깊이 버퍼
//  - 모든 그리기는 여기 버퍼에 하고, 한 프레임에 한 번 Platform::Present로 콘솔에 출력
//  - depth: 칸마다 '카메라에서 얼마나 먼지' 기록 -> 가까운 것만 덮어쓰게 함 (3D 가림 처리)
// ============================================================
#include "Config.h"

struct Cell {
    char     ch;
    uint16_t attr;
};

namespace Screen {
    void  Clear();
    Cell* Data();

    void Put(int x, int y, char c, uint16_t attr);                 // 깊이 무시 (UI용)
    bool PutDepth(int x, int y, float depth, char c, uint16_t attr); // 깊이 테스트 (3D용)

    void Text(int x, int y, const char* s, uint16_t attr);
    void TextC(int y, const char* s, uint16_t attr);                // 가운데 정렬
    // 아스키 아트 출력. transparent면 공백은 건너뜀(뒤 배경이 비침)
    void Art(int x, int y, const char* const* lines, int n, uint16_t attr, bool transparent = true);
    void ArtC(int y, const char* const* lines, int n, uint16_t attr, bool transparent = true);
    int  ArtWidth(const char* const* lines, int n);
    void Box(int x, int y, int w, int h, char c, uint16_t attr);   // 테두리
    void FillRect(int x, int y, int w, int h, char c, uint16_t attr);
    void Tint(uint16_t bgAttr);                                     // 모든 칸에 배경색 (화면 번쩍임)
}
