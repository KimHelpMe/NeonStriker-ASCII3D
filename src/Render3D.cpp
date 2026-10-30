#include "Render3D.h"
#include <cstring>

static Vec3 g_cam = { 0, 0, -30 };
static const float F  = 150.0f;               // 초점 거리 (클수록 확대 = 시야각이 좁아짐)
static const float CX = SCR_W * 0.5f;
static const float CY = VIEW_H * 0.5f;
static const float ASPECT = 0.5f;            // 콘솔 글자 가로:세로 = 1:2 보정
static const float NEAR_Z = 8.0f;            // 카메라에 이보다 가까운 것은 안 그림 (화면을 가득 덮는 것 방지)
static const Vec3  LIGHT = Normalize({ -0.45f, 0.75f, -0.5f });   // 왼쪽 위 앞에서 비추는 빛
static const char* DEFAULT_RAMP = ".,:;-=+*#%@";

void R3D::SetCamera(Vec3 pos) { g_cam = pos; }
Vec3 R3D::CameraPos() { return g_cam; }

bool R3D::Project(Vec3 p, float& sx, float& sy, float& depth)
{
    Vec3 c = p - g_cam;
    if (c.z < NEAR_Z) return false;
    sx = CX + c.x / c.z * F;
    sy = CY - c.y / c.z * F * ASPECT;
    depth = c.z;
    return true;
}

float R3D::ScreenScale(float depth) { return depth > NEAR_Z ? F / depth : F; }

// 삼각형 하나를 칸 단위로 채움 (무게중심 좌표로 안쪽 판정 + 깊이 보간)
static void RasterTri(float x0, float y0, float z0, float x1, float y1, float z1, float x2, float y2, float z2,
                      char c, uint16_t attr)
{
    float area = (x1 - x0) * (y2 - y0) - (x2 - x0) * (y1 - y0);
    if (std::fabs(area) < 0.0001f) {
        // 정면에서 보면 선처럼 얇은 면(날개 옆면 등): 꼭짓점만 찍어서 아예 사라지지 않게
        Screen::PutDepth((int)x0, (int)y0, z0, c, attr);
        return;
    }
    int minX = (int)std::floor(std::fmin(x0, std::fmin(x1, x2)));
    int maxX = (int)std::ceil(std::fmax(x0, std::fmax(x1, x2)));
    int minY = (int)std::floor(std::fmin(y0, std::fmin(y1, y2)));
    int maxY = (int)std::ceil(std::fmax(y0, std::fmax(y1, y2)));
    if (minX < 0) minX = 0;
    if (minY < 0) minY = 0;
    if (maxX >= SCR_W) maxX = SCR_W - 1;
    if (maxY >= VIEW_H) maxY = VIEW_H - 1;
    float inv = 1.0f / area;
    bool drew = false;
    for (int y = minY; y <= maxY; y++)
        for (int x = minX; x <= maxX; x++) {
            float px = x + 0.5f, py = y + 0.5f;
            float w0 = ((x1 - px) * (y2 - py) - (x2 - px) * (y1 - py)) * inv;
            float w1 = ((x2 - px) * (y0 - py) - (x0 - px) * (y2 - py)) * inv;
            float w2 = 1.0f - w0 - w1;
            if (w0 < 0 || w1 < 0 || w2 < 0) continue;
            Screen::PutDepth(x, y, w0 * z0 + w1 * z1 + w2 * z2, c, attr);
            drew = true;
        }
    // 칸 하나보다 작은 삼각형은 칸 중심을 못 덮어서 사라짐 -> 무게중심 칸에 하나 찍음
    if (!drew) Screen::PutDepth((int)((x0 + x1 + x2) / 3), (int)((y0 + y1 + y2) / 3), (z0 + z1 + z2) / 3, c, attr);
}

void R3D::DrawMesh(const Mesh& m, Vec3 pos, Vec3 rot, float scale, const Material* mats,
                   bool flash, const char* ramp, float depthBias)
{
    const char* r = ramp ? ramp : DEFAULT_RAMP;
    int rampLen = (int)strlen(r);
    for (const Tri& t : m.tris) {
        // 1) 모델 -> 월드
        Vec3 a = Rotate(t.a * scale, rot) + pos;
        Vec3 b = Rotate(t.b * scale, rot) + pos;
        Vec3 c = Rotate(t.c * scale, rot) + pos;

        // 2~3) 투영 (꼭짓점이 하나라도 카메라 뒤면 생략: 화면 밖으로 나가는 중인 물체)
        float ax, ay, az, bx, by, bz, cx, cy, cz;
        if (!Project(a, ax, ay, az) || !Project(b, bx, by, bz) || !Project(c, cx, cy, cz)) continue;

        // 5) 셰이딩: 면의 법선을 카메라 쪽으로 돌린 뒤 빛 방향과 내적
        Vec3 n = Normalize(Cross(b - a, c - a));
        if (Dot(n, g_cam - a) < 0) n = n * -1.0f;   // 양면 처리 (감는 방향 실수에 강하게)
        float light = ramp ? 0.5f + 0.5f * Dot(n, Normalize(g_cam - a)) : 0.25f + 0.75f * std::fmax(0.0f, Dot(n, LIGHT));
        int idx = (int)(light * (rampLen - 1) + 0.5f);
        idx = idx < 0 ? 0 : (idx >= rampLen ? rampLen - 1 : idx);

        const Material& mt = mats[t.mat];
        uint16_t attr = flash ? C_BWHITE : (light > 0.55f ? mt.bright : mt.dark);
        // 4) 채우기 + 깊이 비교
        RasterTri(ax, ay, az + depthBias, bx, by, bz + depthBias, cx, cy, cz + depthBias, r[idx], attr);
    }
}

void R3D::DrawBox(Vec3 center, Vec3 half, Material mat, const char* ramp, float depthBias)
{
    // 단위 정육면체를 x,y,z 각각 다른 비율로 늘려서 그림
    const Mesh& cube = Meshes::Get(M_CUBE);
    Mesh box;
    box.tris.reserve(cube.tris.size());
    for (const Tri& t : cube.tris) {
        Vec3 s = { half.x * 2, half.y * 2, half.z * 2 };
        box.tris.push_back({ { t.a.x * s.x, t.a.y * s.y, t.a.z * s.z },
                             { t.b.x * s.x, t.b.y * s.y, t.b.z * s.z },
                             { t.c.x * s.x, t.c.y * s.y, t.c.z * s.z }, 0 });
    }
    DrawMesh(box, center, { 0, 0, 0 }, 1.0f, &mat, false, ramp, depthBias);
}

void R3D::DrawPoint(Vec3 p, char c, uint16_t attr)
{
    float sx, sy, d;
    if (Project(p, sx, sy, d)) Screen::PutDepth((int)sx, (int)sy, d, c, attr);
}

void R3D::DrawLine(Vec3 a, Vec3 b, char c, uint16_t attr, float depthBias)
{
    float ax, ay, ad, bx, by, bd;
    if (!Project(a, ax, ay, ad) || !Project(b, bx, by, bd)) return;
    int steps = (int)std::fmax(std::fabs(bx - ax), std::fabs(by - ay)) + 1;
    if (steps > 400) steps = 400;
    for (int i = 0; i <= steps; i++) {
        // 화면에서 균등하게 나누되, 각 점은 3D에서 보간해서 다시 투영 (원근 때문에 직선 보간과 다름)
        float t = (float)i / steps, sx, sy, d;
        if (Project(a + (b - a) * t, sx, sy, d)) Screen::PutDepth((int)sx, (int)sy, d + depthBias, c, attr);
    }
}

void R3D::DrawBlob(Vec3 p, float radius, char c, uint16_t attr, float depthBias)
{
    float sx, sy, d;
    if (!Project(p, sx, sy, d)) return;
    float rx = radius * F / d;          // 가로 반지름 (칸)
    if (rx > 14.0f) rx = 14.0f;          // 너무 크게 번지지 않도록
    float ry = rx * ASPECT;             // 세로 반지름 (줄)
    if (rx < 0.7f) { Screen::PutDepth((int)sx, (int)sy, d + depthBias, c, attr); return; }
    for (int y = (int)(sy - ry); y <= (int)(sy + ry); y++)
        for (int x = (int)(sx - rx); x <= (int)(sx + rx); x++) {
            float dx = (x + 0.5f - sx) / rx, dy = (y + 0.5f - sy) / (ry > 0.5f ? ry : 0.5f);
            if (dx * dx + dy * dy <= 1.0f) Screen::PutDepth(x, y, d + depthBias, c, attr);
        }
}

void R3D::DrawLabel(Vec3 p, float yOffsetRows, const char* s, uint16_t attr)
{
    float sx, sy, d;
    if (!Project(p, sx, sy, d)) return;
    int len = (int)strlen(s);
    int x0 = (int)sx - len / 2, y = (int)(sy + yOffsetRows);
    for (int i = 0; i < len; i++) Screen::PutDepth(x0 + i, y, d - 2.0f, s[i], attr);
}
