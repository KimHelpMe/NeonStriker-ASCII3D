#include "Monster.h"
#include "Game.h"
#include "Audio.h"
#include "Render3D.h"

static const uint16_t MON_COLOR[3] = { C_LMAGENTA, C_LYELLOW, C_LRED };

void MonsterSystem::Init(int i, bool firstWave)
{
    Monster& o = m[i];
    o.active = true;
    // 처음엔 거리를 넓게 흩뿌려서 한꺼번에 몰려오지 않게
    o.pos = { RandF(-ENEMY_X_LIMIT, ENEMY_X_LIMIT), RandF(-ENEMY_Y_LIMIT, ENEMY_Y_LIMIT),
              firstWave ? RandF(70, 200) : RandF(140, 200) };
    o.drift = { (float)RandI(-1, 1), (float)RandI(-1, 1), 0 };   // 원본처럼 대각/수평 이동 방향
    o.hitCooldown = 0; o.flash = 0; o.spin = RandF(0, 6);
    o.type = i < normalCount ? MonsterType::NORMAL : i < normalCount + mediumCount ? MonsterType::MEDIUM : MonsterType::HEAVY;
    switch (o.type) {   // 접근 속도 / 체력 / 첫 발사 시간 (원본 비율 유지)
    case MonsterType::NORMAL: o.speed = RandF(25, 45); o.hp = 3;  o.fireTimer = RandF(2.0f, 5.0f); break;
    case MonsterType::MEDIUM: o.speed = RandF(18, 28); o.hp = 15; o.fireTimer = RandF(1.5f, 3.5f); break;
    case MonsterType::HEAVY:  o.speed = RandF(12, 18); o.hp = 30; o.fireTimer = RandF(1.0f, 3.0f); break;
    }
    o.maxHp = o.hp;
}

void MonsterSystem::Start(const DifficultyRules& rules)
{
    enabled = true;
    activeCount = rules.monsterCount; normalCount = rules.normalCount; mediumCount = rules.mediumCount;
    for (auto& o : m) o.active = false;
    for (int i = 0; i < activeCount; i++) Init(i, true);
}

void MonsterSystem::Reset() { for (auto& o : m) o.active = false; }

bool MonsterSystem::Hit(Game& g, int idx, int damage, float cooldown)
{
    Monster& o = m[idx];
    if (!o.active || o.pos.z > 210) return false;   // 너무 멀어서 안 보이는 적은 피격 제외
    if (cooldown > 0) {
        if (o.hitCooldown > 0) return false;
        o.hitCooldown = cooldown;
    }
    o.hp -= damage;
    o.flash = 0.08f;
    if (o.hp <= 0) { Kill(g, idx, false); return true; }
    if (RandI(0, 2) == 0) Audio::Play(S_HIT, 0.35f, 0.15f);
    return true;
}

void MonsterSystem::Kill(Game& g, int i, bool rammed)
{
    Monster& o = m[i];
    int t = (int)o.type;
    int chance = rammed ? (t == 0 ? 10 : t == 1 ? 30 : 50) : (t == 0 ? 3 : t == 1 ? 15 : 30);   // 원본 드랍률
    if (RandI(0, 99) < chance) g.items.Drop(o.pos);
    g.fx.Explosion(o.pos, t == 0 ? 0.8f : t == 1 ? 1.3f : 2.0f, MON_COLOR[t]);
    if (t > 0) g.fx.Shake(0.4f * t, 0.2f);
    Audio::Play(t == 0 ? S_EXPLODE_SMALL : S_EXPLODE_MID, 0.7f, 0.1f);
    if (!rammed) { g.player.AddExp(g); g.player.kills++; g.boss.AddKillCount(g); }
    if (enabled) Init(i, false); else o.active = false;
}

void MonsterSystem::Update(Game& g, float dt)
{
    Vec3 pp = g.player.pos;
    for (int i = 0; i < MONSTER_COUNT; i++) {
        Monster& o = m[i];
        if (!o.active) continue;
        if (o.hitCooldown > 0) o.hitCooldown -= dt;
        if (o.flash > 0) o.flash -= dt;
        o.spin += dt * (o.type == MonsterType::NORMAL ? 2.5f : 1.0f);

        // 이동: 앞으로 다가오면서 상하좌우로 흘러감, 경계에서 반사 (원본과 같은 반사 규칙)
        o.pos.z -= o.speed * dt;
        o.pos.x += o.drift.x * o.speed * 0.18f * dt;
        o.pos.y += o.drift.y * o.speed * 0.12f * dt;
        if (std::fabs(o.pos.x) > ENEMY_X_LIMIT) { o.pos.x = Clampf(o.pos.x, -ENEMY_X_LIMIT, ENEMY_X_LIMIT); o.drift.x = -o.drift.x; }
        if (std::fabs(o.pos.y) > ENEMY_Y_LIMIT) { o.pos.y = Clampf(o.pos.y, -ENEMY_Y_LIMIT, ENEMY_Y_LIMIT); o.drift.y = -o.drift.y; }

        // 발사: 보이는 거리에 들어왔고 플레이어보다 충분히 앞에 있을 때
        o.fireTimer -= dt;
        if (o.fireTimer <= 0 && o.pos.z < 130 && o.pos.z > 20 && g.player.Alive()) {
            Vec3 to = Normalize(pp - o.pos);
            const float bs = 42.0f;
            uint16_t c = MON_COLOR[(int)o.type];
            auto Shot = [&](float yaw, float pitch, float spd) {   // to 방향에서 좌우/위아래로 비튼 방향
                Vec3 d = Normalize(to + Vec3{ yaw, pitch, 0 });
                g.bullets.Spawn(BulletType::E_SMALL, o.pos + to * o.Radius(), d * spd, 1, c, 8);
            };
            switch (o.type) {
            case MonsterType::NORMAL:   // 조준 1발 + 좌우 2방향 (가운데는 비워 회피 공간)
                Shot(0, 0, bs); Shot(-0.15f, 0, bs); Shot(0.15f, 0, bs);
                o.fireTimer = RandF(2.5f, 5.0f);
                break;
            case MonsterType::MEDIUM:   // 70% 확률로 조준탄 + 확산 3발
                if (RandI(0, 99) < 70) { Shot(0, 0, bs * 1.2f); for (int k = -1; k <= 1; k++) Shot(k * 0.25f, 0.1f, bs); }
                o.fireTimer = RandF(1.5f, 3.5f);
                break;
            case MonsterType::HEAVY:    // 조준탄 + 십자 확산 4발
                Shot(0, 0, bs * 1.3f);
                Shot(-0.3f, 0, bs); Shot(0.3f, 0, bs); Shot(0, -0.25f, bs); Shot(0, 0.25f, bs);
                o.fireTimer = RandF(1.0f, 2.5f);
                break;
            }
        }
        if (o.pos.z < -6) Init(i, false);    // 플레이어를 지나쳐 가면 다시 멀리서
    }

    // 플레이어 몸통 충돌
    if (g.player.Alive()) {
        for (int i = 0; i < MONSTER_COUNT; i++) {
            Monster& o = m[i];
            if (!o.active || Dist(o.pos, pp) > o.Radius() + 1.8f) continue;
            bool wasInv = g.player.Invincible();
            g.player.Damage(g, 1);
            if (!wasInv && --o.hp <= 0) Kill(g, i, true);
        }
    }
}

void MonsterSystem::Draw() const
{
    static const Material MAT[3][3] = {
        { { C_MAGENTA, C_LMAGENTA }, { C_MAGENTA, C_LMAGENTA }, { C_WHITE, C_BWHITE } },
        { { C_YELLOW, C_LYELLOW }, { C_RED, C_LRED }, { C_WHITE, C_BWHITE } },
        { { C_RED, C_LRED }, { C_YELLOW, C_LYELLOW }, { C_WHITE, C_BWHITE } },
    };
    static const MeshId MESH[3] = { M_EN_NORMAL, M_EN_MEDIUM, M_EN_HEAVY };
    for (const auto& o : m) {
        if (!o.active) continue;
        int t = (int)o.type;
        Vec3 rot = t == 2 ? Vec3{ 0, std::sin(o.spin) * 0.3f, 0 } : Vec3{ 0, 0, o.spin };
        R3D::DrawMesh(Meshes::Get(MESH[t]), o.pos, rot, Monster::DRAW_SCALE, MAT[t], o.flash > 0);
        if (t > 0 && o.hp < o.maxHp && o.pos.z < 140) {   // 중/대형 HP 바
            char bar[12];
            int n = o.hp * 8 / o.maxHp;
            for (int k = 0; k < 8; k++) bar[k] = k < n ? '=' : '.';
            bar[8] = 0;
            R3D::DrawLabel(o.pos + Vec3{ 0, o.Radius() + 0.5f, 0 }, -1, bar, MON_COLOR[t]);
        }
    }
}
