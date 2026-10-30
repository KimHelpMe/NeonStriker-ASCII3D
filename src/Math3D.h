#pragma once
// ============================================================
//  3D 벡터 수학 (게임 엔진이 내부에서 해 주던 계산을 직접 구현)
// ============================================================
#include <cmath>

struct Vec3 {
    float x = 0, y = 0, z = 0;
};

inline Vec3  operator+(Vec3 a, Vec3 b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
inline Vec3  operator-(Vec3 a, Vec3 b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
inline Vec3  operator*(Vec3 a, float s) { return { a.x * s, a.y * s, a.z * s }; }
inline Vec3& operator+=(Vec3& a, Vec3 b) { a.x += b.x; a.y += b.y; a.z += b.z; return a; }
inline float Dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline Vec3  Cross(Vec3 a, Vec3 b) { return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x }; }
inline float Length(Vec3 a) { return std::sqrt(Dot(a, a)); }
inline Vec3  Normalize(Vec3 a) { float l = Length(a); return l > 0.00001f ? a * (1.0f / l) : Vec3{ 0, 0, 1 }; }
inline float Dist(Vec3 a, Vec3 b) { return Length(a - b); }

// 회전: roll(z축) -> pitch(x축) -> yaw(y축) 순서로 적용
// (비행기 기준: roll = 좌우 기울기, pitch = 기수 위아래, yaw = 좌우 방향)
inline Vec3 Rotate(Vec3 v, Vec3 rot)
{
    float cr = std::cos(rot.z), sr = std::sin(rot.z);
    Vec3 a = { v.x * cr - v.y * sr, v.x * sr + v.y * cr, v.z };
    float cp = std::cos(rot.x), sp = std::sin(rot.x);
    Vec3 b = { a.x, a.y * cp - a.z * sp, a.y * sp + a.z * cp };
    float cy = std::cos(rot.y), sy = std::sin(rot.y);
    return { b.x * cy + b.z * sy, b.y, -b.x * sy + b.z * cy };
}

// 점 p에서 선분 ab까지의 최단 거리 (빠른 탄이 한 프레임에 적을 뚫고 지나가는 것 방지용)
inline float DistPointSegment(Vec3 p, Vec3 a, Vec3 b)
{
    Vec3 ab = b - a;
    float len2 = Dot(ab, ab);
    float t = len2 > 0 ? Dot(p - a, ab) / len2 : 0;
    t = t < 0 ? 0 : (t > 1 ? 1 : t);
    return Dist(p, a + ab * t);
}
