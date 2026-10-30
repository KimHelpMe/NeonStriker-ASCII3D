#include "Item.h"
#include "Game.h"
#include "Render3D.h"

void ItemSystem::Reset() { for (auto& it : pool) it.active = false; }

void ItemSystem::Drop(Vec3 pos)
{
    for (auto& it : pool) {
        if (it.active) continue;
        int r = RandI(0, 99);   // 가중치 드랍 (원본과 동일: 35/25/25/10/5)
        it.type = r < 35 ? ItemType::NORMAL : r < 60 ? ItemType::FLAME : r < 85 ? ItemType::LASER : r < 95 ? ItemType::BOMB : ItemType::FEVER;
        float a = RandF(0, 2 * PI);
        it.dir = { std::cos(a), std::sin(a), 0 };
        it.pos = { Clampf(pos.x, -PLAYER_X_LIMIT, PLAYER_X_LIMIT), Clampf(pos.y, -PLAYER_Y_LIMIT, PLAYER_Y_LIMIT), pos.z };
        it.lifeTimer = 0;
        it.active = true;
        return;
    }
}

void ItemSystem::Update(Game& g, float dt)
{
    for (auto& it : pool) {
        if (!it.active) continue;
        it.lifeTimer += dt;
        if (it.lifeTimer >= ITEM_LIFETIME) { it.active = false; continue; }
        if (it.pos.z > 0) it.pos.z = std::fmax(0.0f, it.pos.z - 45.0f * dt);   // 플레이어 평면으로 다가옴
        it.pos.x += it.dir.x * 8.0f * dt;
        it.pos.y += it.dir.y * 8.0f * dt;
        if (std::fabs(it.pos.x) > PLAYER_X_LIMIT) { it.dir.x = -it.dir.x; it.pos.x = Clampf(it.pos.x, -PLAYER_X_LIMIT, PLAYER_X_LIMIT); }
        if (std::fabs(it.pos.y) > PLAYER_Y_LIMIT) { it.dir.y = -it.dir.y; it.pos.y = Clampf(it.pos.y, -PLAYER_Y_LIMIT, PLAYER_Y_LIMIT); }
        if (g.player.Alive() && Dist(it.pos, g.player.pos) < 4.0f) {
            it.active = false;
            g.fx.Burst(it.pos, 16, 20, 0.4f, 0.4f, C_BWHITE, C_GRAY);
            g.player.ApplyItem(g, it.type);
        }
    }
}

void ItemSystem::Draw() const
{
    static const uint16_t COL[5] = { C_LCYAN, C_LRED, C_LYELLOW, C_LGREEN, C_BWHITE };
    static const char* LABEL[5] = { "[N]", "[F]", "[L]", "[B]", "[S]" };   // 원본 아이템 표기 그대로
    for (const auto& it : pool) {
        if (!it.active) continue;
        float remain = ITEM_LIFETIME - it.lifeTimer;
        if (remain < 3.0f && (int)(it.lifeTimer * (remain < 1.0f ? 20 : 10)) % 2 == 0) continue;   // 사라지기 전 깜빡임
        uint16_t c = it.type == ItemType::FEVER ? RainbowColor(it.lifeTimer) : COL[(int)it.type];
        Material mat[1] = { { (uint16_t)(c & 7), c } };
        R3D::DrawMesh(Meshes::Get(M_ITEM), it.pos, { 0, it.lifeTimer * 3, it.lifeTimer * 2 }, 1.0f, mat);
        R3D::DrawLabel(it.pos, -2, LABEL[(int)it.type], c);
    }
}
