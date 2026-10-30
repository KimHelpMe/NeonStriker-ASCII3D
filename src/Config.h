#pragma once
// ============================================================
//  공용 설정 / 열거형 / 헬퍼  (NEON STRIKER Config.h를 3D 콘솔용으로 변환)
// ============================================================
#include <cstdint>
#include <cmath>
#include <cstdlib>

// --- 콘솔 화면 (원본 DF_BG_SIZE_X x DF_SCREEN_HEIGHT) ---
constexpr int SCR_W  = 240;
constexpr int SCR_H  = 75;
constexpr int VIEW_H = 71;          // 3D 게임 화면 줄 수 (아래 4줄은 HUD)

// --- 콘솔 16색 (Windows FOREGROUND_* 비트 조합과 같은 값) ---
constexpr uint16_t
    C_BLACK = 0, C_BLUE = 1, C_GREEN = 2, C_CYAN = 3, C_RED = 4, C_MAGENTA = 5, C_YELLOW = 6, C_WHITE = 7,
    C_GRAY = 8, C_LBLUE = 9, C_LGREEN = 10, C_LCYAN = 11, C_LRED = 12, C_LMAGENTA = 13, C_LYELLOW = 14, C_BWHITE = 15;

// --- 3D 월드 ---
// 플레이어는 z=0 평면에서 상하좌우로 움직이고, 적은 먼 곳(+z)에서 다가온다.
constexpr float PLAYER_X_LIMIT = 28.0f;
constexpr float PLAYER_Y_LIMIT = 14.0f;
constexpr float ENEMY_X_LIMIT  = 40.0f;
constexpr float ENEMY_Y_LIMIT  = 20.0f;
constexpr float FAR_Z          = 300.0f;   // 이보다 먼 탄은 제거

// --- 오브젝트 풀 크기 ---
constexpr int BULLET_COUNT   = 4000;
constexpr int MONSTER_COUNT  = 30;
constexpr int ITEM_COUNT     = 40;
constexpr int PARTICLE_COUNT = 3000;

// --- 게임 규칙 (NEON STRIKER와 동일) ---
constexpr int   LAST_STAGE       = 4;
constexpr int   MAX_WEAPON_LEVEL = 5;
constexpr float MAX_CHARGE_TIME  = 3.0f;
constexpr float FEVER_TIME       = 10.0f;
constexpr float ITEM_LIFETIME    = 15.0f;

enum class WeaponType { NORMAL, FLAME, LASER };
enum class ItemType   { NORMAL, FLAME, LASER, BOMB, FEVER };   // 앞 3개는 WeaponType과 순서가 같아야 함

enum class Difficulty { NORMAL, HARD };
struct DifficultyRules {
    const char* name;
    int   monsterCount;     // 동시에 있는 몬스터 수
    int   normalCount;      // 그중 NORMAL 수
    int   mediumCount;      // 그중 MEDIUM 수 (나머지 HEAVY)
    float bossHpScale;      // 보스 체력 배율
    int   killRequirement;  // 보스 등장 킬수
};
constexpr DifficultyRules DIFFICULTY_RULES[2] = {
    { "NORMAL", 20, 18, 1, 1.0f, 200 },
    { "HARD",   30, 24, 4, 2.0f, 300 },
};

// --- 헬퍼 ---
inline float RandF(float a, float b) { return a + (b - a) * (std::rand() / (float)RAND_MAX); }
inline int   RandI(int a, int b) { return a + std::rand() % (b - a + 1); }
inline float Clampf(float v, float a, float b) { return v < a ? a : (v > b ? b : v); }
constexpr float PI = 3.14159265f;

// 무지개 색 순환 (피버 연출) - 원본 Item.cpp의 7색 순환과 같은 순서
inline uint16_t RainbowColor(float t)
{
    static const uint16_t c[7] = { C_LRED, C_LYELLOW, C_LGREEN, C_LCYAN, C_LBLUE, C_LMAGENTA, C_BWHITE };
    int i = (int)(t * 10) % 7;
    return c[i < 0 ? 0 : i];
}
