#include "Mesh.h"
#include "Config.h"

static Mesh g_mesh[M_COUNT];

// ------------------------------------------------------------
//  도형 조립 헬퍼
// ------------------------------------------------------------
static void AddTri(Mesh& m, Vec3 a, Vec3 b, Vec3 c, uint8_t mat) { m.tris.push_back({ a, b, c, mat }); }
static void AddQuad(Mesh& m, Vec3 a, Vec3 b, Vec3 c, Vec3 d, uint8_t mat) { AddTri(m, a, b, c, mat); AddTri(m, a, c, d, mat); }

// 직육면체 (중심 c, 반 크기 h)
static void AddBox(Mesh& m, Vec3 c, Vec3 h, uint8_t mat)
{
    Vec3 p[8];
    for (int i = 0; i < 8; i++)
        p[i] = { c.x + ((i & 1) ? h.x : -h.x), c.y + ((i & 2) ? h.y : -h.y), c.z + ((i & 4) ? h.z : -h.z) };
    AddQuad(m, p[0], p[1], p[3], p[2], mat);  // 앞(-z)
    AddQuad(m, p[4], p[5], p[7], p[6], mat);  // 뒤(+z)
    AddQuad(m, p[0], p[1], p[5], p[4], mat);  // 아래
    AddQuad(m, p[2], p[3], p[7], p[6], mat);  // 위
    AddQuad(m, p[0], p[2], p[6], p[4], mat);  // 왼쪽
    AddQuad(m, p[1], p[3], p[7], p[5], mat);  // 오른쪽
}

// 팔면체 (다이아몬드). 축마다 반지름이 달라 길쭉하게 만들 수 있음
static void AddOcta(Mesh& m, Vec3 c, float rx, float ry, float rzFront, float rzBack, uint8_t mat)
{
    Vec3 f = { c.x, c.y, c.z - rzFront }, b = { c.x, c.y, c.z + rzBack };
    Vec3 r[4] = { { c.x + rx, c.y, c.z }, { c.x, c.y + ry, c.z }, { c.x - rx, c.y, c.z }, { c.x, c.y - ry, c.z } };
    for (int i = 0; i < 4; i++) {
        AddTri(m, f, r[i], r[(i + 1) % 4], mat);
        AddTri(m, b, r[i], r[(i + 1) % 4], mat);
    }
}

// z축 방향 n각기둥 (정면에서 보면 n각형)
static void AddPrism(Mesh& m, Vec3 c, int n, float radius, float halfDepth, float rot, uint8_t mat)
{
    std::vector<Vec3> f(n), b(n);
    for (int i = 0; i < n; i++) {
        float a = rot + i * 2 * PI / n;
        f[i] = { c.x + std::cos(a) * radius, c.y + std::sin(a) * radius, c.z - halfDepth };
        b[i] = { c.x + std::cos(a) * radius, c.y + std::sin(a) * radius, c.z + halfDepth };
    }
    Vec3 fc = { c.x, c.y, c.z - halfDepth }, bc = { c.x, c.y, c.z + halfDepth };
    for (int i = 0; i < n; i++) {
        int j = (i + 1) % n;
        AddTri(m, fc, f[i], f[j], mat);
        AddTri(m, bc, b[i], b[j], mat);
        AddQuad(m, f[i], f[j], b[j], b[i], mat);
    }
}

// 구 (rings x segs 개의 사각형)
static void AddSphere(Mesh& m, Vec3 c, float r, int rings, int segs, uint8_t mat)
{
    auto P = [&](int i, int j) {
        float th = PI * i / rings, ph = 2 * PI * j / segs;
        return Vec3{ c.x + r * std::sin(th) * std::cos(ph), c.y + r * std::cos(th), c.z + r * std::sin(th) * std::sin(ph) };
    };
    for (int i = 0; i < rings; i++)
        for (int j = 0; j < segs; j++)
            AddQuad(m, P(i, j), P(i + 1, j), P(i + 1, j + 1), P(i, j + 1), mat);
}

// 뿔 (밑면 중심 base, 밑면 반지름 r, 꼭짓점 tip)
static void AddSpike(Mesh& m, Vec3 base, float r, Vec3 tip, int sides, uint8_t mat)
{
    Vec3 axis = Normalize(tip - base);
    Vec3 up = std::fabs(axis.y) < 0.9f ? Vec3{ 0, 1, 0 } : Vec3{ 1, 0, 0 };
    Vec3 u = Normalize(Cross(axis, up)), v = Cross(axis, u);
    for (int i = 0; i < sides; i++) {
        float a0 = i * 2 * PI / sides, a1 = (i + 1) * 2 * PI / sides;
        Vec3 p0 = base + u * (std::cos(a0) * r) + v * (std::sin(a0) * r);
        Vec3 p1 = base + u * (std::cos(a1) * r) + v * (std::sin(a1) * r);
        AddTri(m, p0, p1, tip, mat);
        AddTri(m, p0, p1, base, mat);
    }
}

// ------------------------------------------------------------
//  모델 정의
// ------------------------------------------------------------
static void BuildPlayer(Mesh& m)
{
    // 동체: 기수(+z)가 길쭉한 다이아몬드
    AddOcta(m, { 0, 0, 0 }, 1.4f, 1.3f, -4.5f, 0, 0);       // 앞쪽 절반 (rzFront 음수 = +z 방향으로 뾰족)
    AddOcta(m, { 0, 0, 0 }, 1.4f, 1.3f, 2.5f, 0, 0);        // 뒤쪽 절반
    // 날개: 약간 위로 꺾인(상반각) 두께 있는 쐐기
    for (int s = -1; s <= 1; s += 2) {
        Vec3 root0 = { 0.8f * s, 0, 1.0f }, root1 = { 0.8f * s, 0, -2.0f };
        Vec3 tip0  = { 3.8f * s, 0.7f, -1.6f }, tip1 = { 3.8f * s, 0.7f, -2.6f };
        AddQuad(m, root0, tip0, tip1, root1, 1);
        AddQuad(m, { root0.x, -0.35f, root0.z }, { tip0.x, 0.35f, tip0.z }, { tip1.x, 0.35f, tip1.z }, { root1.x, -0.35f, root1.z }, 1);
        AddQuad(m, tip0, tip1, { tip1.x, 0.35f, tip1.z }, { tip0.x, 0.35f, tip0.z }, 1);
    }
    // 수직 꼬리날개 2개
    for (int s = -1; s <= 1; s += 2)
        AddQuad(m, { 0.5f * s, 0.6f, -0.8f }, { 0.9f * s, 2.3f, -2.4f }, { 0.9f * s, 2.3f, -2.9f }, { 0.5f * s, 0.6f, -2.4f }, 1);
    // 엔진 노즐 (뒤에서 보이는 발광부)
    AddBox(m, { 0, 0, -2.4f }, { 0.55f, 0.45f, 0.25f }, 2);
}

static void BuildEnemyNormal(Mesh& m)
{
    // 플레이어 쪽(-z)으로 뾰족한 다트 + 십자 지느러미
    AddOcta(m, { 0, 0, 0 }, 1.5f, 1.1f, 3.0f, 1.5f, 0);
    AddSpike(m, { 1.2f, 0, 0.6f }, 0.4f, { 3.0f, 0, 1.6f }, 3, 1);
    AddSpike(m, { -1.2f, 0, 0.6f }, 0.4f, { -3.0f, 0, 1.6f }, 3, 1);
    AddSpike(m, { 0, 0.9f, 0.6f }, 0.35f, { 0, 2.2f, 1.6f }, 3, 1);
    AddOcta(m, { 0, 0, -1.0f }, 0.45f, 0.45f, 0.6f, 0.4f, 2);   // 눈(발광)
}

static void BuildEnemyMedium(Mesh& m)
{
    // 정면에서 보면 육각형인 원반 + 가운데 코어
    AddPrism(m, { 0, 0, 0 }, 6, 2.8f, 0.8f, 0, 0);
    AddPrism(m, { 0, 0, -0.9f }, 6, 1.5f, 0.3f, PI / 6, 1);
    AddOcta(m, { 0, 0, -1.3f }, 0.7f, 0.7f, 0.7f, 0.3f, 2);
    AddSpike(m, { 0, 0, -0.8f }, 0.35f, { 0, 0, -3.0f }, 4, 1);   // 포신
}

static void BuildEnemyHeavy(Mesh& m)
{
    // 두꺼운 장갑 상자 + 양옆 포드 + 정면 포신 2개
    AddBox(m, { 0, 0, 0 }, { 2.6f, 2.0f, 2.2f }, 0);
    AddBox(m, { 3.6f, 0, 0.4f }, { 1.0f, 1.0f, 2.6f }, 1);
    AddBox(m, { -3.6f, 0, 0.4f }, { 1.0f, 1.0f, 2.6f }, 1);
    AddBox(m, { 1.0f, -0.8f, -3.0f }, { 0.35f, 0.35f, 1.0f }, 1);
    AddBox(m, { -1.0f, -0.8f, -3.0f }, { 0.35f, 0.35f, 1.0f }, 1);
    AddOcta(m, { 0, 0.6f, -2.3f }, 1.0f, 0.8f, 0.6f, 0.2f, 2);
}

static void BuildBoss1(Mesh& m)   // UNIT-A : 보라색 순양함
{
    AddOcta(m, { 0, 0, 0 }, 9.0f, 3.2f, 7.0f, 5.0f, 0);
    AddBox(m, { 6.5f, 2.2f, 1.5f }, { 3.0f, 0.8f, 4.0f }, 1);
    AddBox(m, { -6.5f, 2.2f, 1.5f }, { 3.0f, 0.8f, 4.0f }, 1);
    AddBox(m, { 6.5f, -2.2f, 1.5f }, { 3.0f, 0.8f, 4.0f }, 1);
    AddBox(m, { -6.5f, -2.2f, 1.5f }, { 3.0f, 0.8f, 4.0f }, 1);
    AddOcta(m, { 0, 0, -4.5f }, 2.0f, 1.6f, 2.0f, 0.5f, 2);
}

static void BuildBoss2(Mesh& m)   // UNIT-B : 녹색 생체 병기
{
    AddSphere(m, { 0, 0, 0 }, 6.5f, 6, 10, 0);
    for (int i = 0; i < 8; i++) {   // 촉수
        float a = i * 2 * PI / 8;
        Vec3 base = { std::cos(a) * 5.0f, std::sin(a) * 5.0f, -1.0f };
        Vec3 tip = { std::cos(a) * 12.0f, std::sin(a) * 9.0f, -6.0f + (i % 2) * 3.0f };
        AddSpike(m, base, 1.0f, tip, 4, 1);
    }
    AddSphere(m, { 0, 0, -5.5f }, 2.4f, 4, 8, 2);   // 눈
}

static void BuildBoss3(Mesh& m)   // CORE-OMEGA : 주황 요새
{
    AddBox(m, { 0, 0, 0 }, { 9.0f, 7.0f, 6.0f }, 0);
    AddPrism(m, { 0, 0, -6.4f }, 8, 4.5f, 0.5f, 0, 1);
    for (int i = 0; i < 4; i++) {   // 정면 포탑 4개
        float x = (i % 2 ? 6.0f : -6.0f), y = (i / 2 ? 4.5f : -4.5f);
        AddBox(m, { x, y, -7.0f }, { 1.2f, 1.2f, 1.2f }, 1);
        AddBox(m, { x, y, -9.2f }, { 0.4f, 0.4f, 1.2f }, 1);
    }
    AddOcta(m, { 0, 0, -7.4f }, 2.4f, 2.4f, 1.6f, 0.4f, 2);
}

static void BuildBoss4(Mesh& m)   // ULTIMATE-DREAD : 붉은 가시 코어
{
    AddSphere(m, { 0, 0, 0 }, 7.0f, 6, 10, 0);
    // 사방으로 뻗은 가시 (정이십면체 꼭짓점 방향)
    const float g = 1.618f;
    Vec3 dirs[12] = { { -1, g, 0 }, { 1, g, 0 }, { -1, -g, 0 }, { 1, -g, 0 }, { 0, -1, g }, { 0, 1, g },
                      { 0, -1, -g }, { 0, 1, -g }, { g, 0, -1 }, { g, 0, 1 }, { -g, 0, -1 }, { -g, 0, 1 } };
    for (auto d : dirs) {
        Vec3 n = Normalize(d);
        AddSpike(m, n * 6.0f, 1.6f, n * 14.0f, 4, 1);
    }
    AddSphere(m, { 0, 0, -6.0f }, 3.0f, 4, 8, 2);   // 코어
}

void Meshes::Build()
{
    BuildPlayer(g_mesh[M_PLAYER]);
    BuildEnemyNormal(g_mesh[M_EN_NORMAL]);
    BuildEnemyMedium(g_mesh[M_EN_MEDIUM]);
    BuildEnemyHeavy(g_mesh[M_EN_HEAVY]);
    BuildBoss1(g_mesh[M_BOSS1]);
    BuildBoss2(g_mesh[M_BOSS2]);
    BuildBoss3(g_mesh[M_BOSS3]);
    BuildBoss4(g_mesh[M_BOSS4]);
    AddOcta(g_mesh[M_ITEM], { 0, 0, 0 }, 1.6f, 1.6f, 1.6f, 1.6f, 0);
    AddOcta(g_mesh[M_BOMB], { 0, 0, 0 }, 1.0f, 1.0f, 1.0f, 1.0f, 0);
    AddOcta(g_mesh[M_MISSILE], { 0, 0, 0 }, 0.45f, 0.45f, -1.4f, 0.6f, 0);
    AddBox(g_mesh[M_CUBE], { 0, 0, 0 }, { 0.5f, 0.5f, 0.5f }, 0);
}

const Mesh& Meshes::Get(MeshId id) { return g_mesh[id]; }
