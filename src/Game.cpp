#include "Game.h"
#include "Audio.h"
#include "Platform.h"
#include "Render3D.h"
#include "UI.h"

void Game::Init()
{
    background.Init();
    fx.Reset();
    boss.Init(1);
    R3D::SetCamera({ 0, 6, -30 });
    ChangeScene(Scene::LOGO);
}

void Game::ChangeScene(Scene s) { scene = s; sceneTime = 0; }

void Game::StopLoopSounds()
{
    Audio::Stop(S_CHARGE); Audio::Stop(S_GIGA_LASER); Audio::Stop(S_FLAME_ORB);
}

void Game::NewGame()
{
    player.Reset();
    fx.Reset();
    StartStage(1);
}

void Game::StartStage(int s)
{
    stage = s;
    bullets.Reset(); items.Reset(); monsters.Reset();
    boss.Init(stage);
    stageEndDelay = 0;
    ChangeScene(Scene::STAGE_INTRO);
    Audio::Play(S_STAGE_START);
}

// ============================================================
void Game::Update(float dt)
{
    sceneTime += dt;
    fpsCount++; fpsTimer += dt;   // 원본 FPS 클래스와 같은 방식 (1초마다 프레임 수 갱신)
    if (fpsTimer >= 1.0f) { fps = fpsCount; fpsCount = 0; fpsTimer = 0; }

    switch (scene) {
    case Scene::LOGO:
        background.Update(dt, 0.6f);
        fx.Update(dt);
        if (sceneTime < 0.5f) break;
        if (Input::Pressed(K_UP) || Input::Pressed(K_DOWN)) {   // 난이도 선택
            difficulty = difficulty == Difficulty::NORMAL ? Difficulty::HARD : Difficulty::NORMAL;
            Audio::Play(S_TICK);
        }
        if (Input::Pressed(K_ENTER) || Input::Pressed(K_SPACE)) {
            Audio::Play(S_SELECT);
            fx.Flash(0x70, 0.12f);   // 원본: 화이트 아웃
            NewGame();
        }
        break;

    case Scene::STAGE_INTRO:
        background.Update(dt, 4.0f);   // 워프
        fx.Update(dt);
        if (sceneTime > 2.5f || (sceneTime > 1.0f && Input::Pressed(K_SPACE))) {
            monsters.Start(Rules());
            ChangeScene(Scene::PLAYING);
        }
        break;

    case Scene::PLAYING:
        UpdatePlaying(dt);
        break;

    case Scene::PAUSED:
        if (Input::Pressed(K_ESC)) { Audio::Play(S_PAUSE); ChangeScene(Scene::PLAYING); }
        else if (Input::Pressed(K_R)) { Audio::Play(S_SELECT); StopLoopSounds(); ChangeScene(Scene::LOGO); }
        else if (Input::Pressed(K_E)) quit = true;
        break;

    case Scene::GAME_OVER:
        background.Update(dt, 0.3f);
        fx.Update(dt);
        if (sceneTime < 2.0f) break;
        countdown -= dt;   // 원본 CONTINUE: 1초에 1씩, 아무 키나 누르면 0.5초 차감
        if (Input::Pressed(K_R)) { Audio::Play(S_SELECT); NewGame(); break; }
        if (Input::Pressed(K_E)) { quit = true; break; }
        if (Input::AnyPressed()) countdown -= 0.5f;
        if ((int)countdown != lastCountdownNum && countdown >= 0) { Audio::Play(S_TICK); lastCountdownNum = (int)countdown; }
        if (countdown < 0) ChangeScene(Scene::LOGO);
        break;

    case Scene::CLEAR:
        background.Update(dt, 1.0f);
        fx.Update(dt);
        if (RandI(0, 8) == 0)   // 축하 불꽃놀이
            fx.Explosion({ RandF(-40, 40), RandF(-5, 25), RandF(60, 140) }, 1.5f, RainbowColor(RandF(0, 1)));
        if (sceneTime > 5.0f || (sceneTime > 1.5f && Input::AnyPressed())) {
            allClear = true; countdown = 9.99f; lastCountdownNum = 9;
            ChangeScene(Scene::GAME_OVER);
            sceneTime = 2.0f;
        }
        break;
    }
    UpdateCamera();
}

void Game::UpdateCamera()
{
    // 플레이어 뒤 위쪽에서 따라감 (플레이어 이동의 60%만 따라가서 화면이 덜 흔들리고 원근감이 생김)
    Vec3 s = fx.ShakeOffset();
    bool follow = scene == Scene::PLAYING || scene == Scene::PAUSED;
    float px = follow ? player.pos.x * 0.6f : 0, py = follow ? player.pos.y * 0.6f : 0;
    R3D::SetCamera({ px + s.x, py + 6 + s.y, -30 });
}

void Game::UpdatePlaying(float dt)
{
    if (Input::Pressed(K_ESC)) { Audio::Play(S_PAUSE); ChangeScene(Scene::PAUSED); return; }
    if (Input::Pressed(K_6)) boss.ForceAppear(*this);   // 테스트 키

    background.Update(dt, boss.Present() ? 1.6f : 1.0f);
    UpdateLaserBlocking();
    player.Update(*this, dt);
    bullets.Update(*this, dt);
    if (monsters.enabled) monsters.Update(*this, dt);
    items.Update(*this, dt);
    boss.Update(*this, dt);
    ResolveCollisions();
    UpdateChargeImpacts(dt);
    fx.Update(dt);

    if (player.GameOverReady()) {
        allClear = false; countdown = 9.99f; lastCountdownNum = 9;
        StopLoopSounds();
        Audio::Play(S_GAME_OVER);
        ChangeScene(Scene::GAME_OVER);
        return;
    }
    if (boss.defeated) {
        stageEndDelay += dt;
        if (stageEndDelay > 0.8f) {
            boss.defeated = false;
            if (stage < LAST_STAGE) StartStage(stage + 1);
            else { StopLoopSounds(); Audio::Play(S_CLEAR); ChangeScene(Scene::CLEAR); }
        }
    }
}

// 레이저가 보스 앞면에서 막히도록 끝 지점 계산
void Game::UpdateLaserBlocking()
{
    player.laserEndZ = FAR_Z;
    player.gigaLaserEndZ = FAR_Z;
    if (!boss.Fighting()) return;
    Vec3 h = boss.Half();
    Box3 l = player.LaserBox(), gl = player.GigaLaserBox();
    auto XYOverlap = [&](const Box3& b) {
        return std::fabs(b.center.x - boss.pos.x) <= b.half.x + h.x && std::fabs(b.center.y - boss.pos.y) <= b.half.y + h.y;
    };
    if (XYOverlap(l)) player.laserEndZ = boss.pos.z - h.z + 1;
    if (XYOverlap(gl)) player.gigaLaserEndZ = boss.pos.z - h.z + 1;
}

// ============================================================
//  충돌 처리 (NEON과 같은 구조, 원/사각형 판정 -> 구/상자 판정)
// ============================================================
void Game::ResolveCollisions()
{
    Bullet*  bp = bullets.Data();
    Monster* mp = monsters.Data();
    bool bossOn = boss.Fighting();
    int wl = player.weaponLevel;
    int pierceDmg = wl <= 2 ? 1 : wl <= 4 ? 2 : 3;

    for (int i = 0; i < BULLET_COUNT; i++) {
        Bullet& b = bp[i];
        if (!b.active || !b.IsPlayerSide()) continue;

        if (b.type == BulletType::P_MISSILE) {
            // 기가 미사일 링: 링이 지나가는 원판 안의 적 전부 + 적 탄 상쇄
            float r = b.MissileRingRadius();
            auto InDisc = [&](Vec3 p, float pr) {
                float dx = p.x - b.pos.x, dy = p.y - b.pos.y;
                return std::fabs(p.z - b.pos.z) < 3.0f + pr && std::sqrt(dx * dx + dy * dy) < r + pr;
            };
            for (int k = 0; k < MONSTER_COUNT; k++)
                if (mp[k].active && InDisc(mp[k].pos, mp[k].Radius())) monsters.Hit(*this, k, 18, 0.15f);
            if (bossOn && std::fabs(boss.pos.z - b.pos.z) < boss.Half().z + 3) {
                Vec3 h = boss.Half();
                float cx = Clampf(b.pos.x, boss.pos.x - h.x, boss.pos.x + h.x), cy = Clampf(b.pos.y, boss.pos.y - h.y, boss.pos.y + h.y);
                if ((cx - b.pos.x) * (cx - b.pos.x) + (cy - b.pos.y) * (cy - b.pos.y) < r * r) boss.Damage(*this, 18, 0.10f);
            }
            for (int j = 0; j < BULLET_COUNT; j++)
                if (bp[j].active && !bp[j].IsPlayerSide() && InDisc(bp[j].pos, bp[j].radius)) {
                    fx.Spawn(bp[j].pos, {}, 0.2f, 0.8f, 0.1f, C_BWHITE, C_GRAY);
                    bp[j].active = false;
                }
            continue;
        }
        if (b.type == BulletType::P_BOMB) {
            bool touch = bossOn && boss.Overlaps(b.pos, b.radius);
            for (int k = 0; k < MONSTER_COUNT && !touch; k++)
                touch = mp[k].active && Dist(b.pos, mp[k].pos) < b.radius + mp[k].Radius();
            if (touch) { b.active = false; bullets.ShatterBomb(*this, b.pos); }
            continue;
        }

        // 일반탄은 '직전 위치 -> 현재 위치' 선분으로 판정 (한 프레임에 2단위 넘게 움직여서 적을 뚫고 지나가는 것 방지)
        for (int k = 0; k < MONSTER_COUNT && b.active; k++) {
            if (!mp[k].active) continue;
            float d = b.type == BulletType::P_NORMAL ? DistPointSegment(mp[k].pos, b.prev, b.pos) : Dist(mp[k].pos, b.pos);
            if (d > b.radius + mp[k].Radius()) continue;
            if (b.type == BulletType::P_NORMAL)        { if (monsters.Hit(*this, k, b.damage, 0)) b.active = false; }
            else if (b.type == BulletType::P_FLAME)    { monsters.Hit(*this, k, b.damage, 0.06f); }
            else if (b.type == BulletType::P_SHRAPNEL) { if (monsters.Hit(*this, k, 5, 0)) b.active = false; }
            if (!b.active) fx.Burst(b.pos, 4, 12, 0.3f, 0.2f, b.color, C_GRAY);
        }
        if (b.active && bossOn && (boss.Overlaps(b.pos, b.radius) || boss.Overlaps((b.prev + b.pos) * 0.5f, b.radius))) {
            if (b.type == BulletType::P_NORMAL) {
                boss.Damage(*this, b.damage, 0.0f);
                fx.Burst(b.pos, 4, 12, 0.3f, 0.2f, b.color, C_GRAY);
                b.active = false;
            } else if (b.type == BulletType::P_FLAME) {
                boss.Damage(*this, b.damage, 0.03f);
            } else if (b.type == BulletType::P_SHRAPNEL) {
                boss.Damage(*this, 3, 0.0f);
                b.active = false;
            }
        }
    }

    // ---- 기체 부착형 공격 ----
    if (player.Alive()) {
        if (player.LaserOn()) {
            Box3 l = player.LaserBox();
            for (int k = 0; k < MONSTER_COUNT; k++)
                if (mp[k].active && l.Contains(mp[k].pos, mp[k].Radius())) monsters.Hit(*this, k, pierceDmg, 0.1f);
            if (bossOn && player.laserEndZ < FAR_Z) boss.Damage(*this, pierceDmg, 0.18f);
        }
        if (player.GigaLaserOn()) {
            Box3 l = player.GigaLaserBox();
            for (int k = 0; k < MONSTER_COUNT; k++)
                if (mp[k].active && l.Contains(mp[k].pos, mp[k].Radius())) monsters.Hit(*this, k, 10, 0.2f);
            if (bossOn && player.gigaLaserEndZ < FAR_Z) boss.Damage(*this, 10, 0.2f);
            for (int j = 0; j < BULLET_COUNT; j++)
                if (bp[j].active && !bp[j].IsPlayerSide() && l.Contains(bp[j].pos, bp[j].radius)) bp[j].active = false;
        }
        if (player.FlameOrbsOn()) {
            Vec3 c[4];
            player.FlameOrbCenters(c);
            const float R = Player::FLAME_ORB_RADIUS;
            for (auto& o : c) {
                for (int k = 0; k < MONSTER_COUNT; k++)
                    if (mp[k].active && Dist(o, mp[k].pos) < R + mp[k].Radius()) monsters.Hit(*this, k, 10, 0.2f);
                if (bossOn && boss.Overlaps(o, R)) boss.Damage(*this, 10, 0.2f);
                for (int j = 0; j < BULLET_COUNT; j++)
                    if (bp[j].active && !bp[j].IsPlayerSide() && Dist(o, bp[j].pos) < R + bp[j].radius) bp[j].active = false;
            }
        }
    }

    // ---- 적 탄 -> 플레이어 (모든 방향의 적탄 판정: 원본의 소용돌이탄 판정 누락 수정 유지) ----
    if (player.Alive() && !player.Invincible()) {
        for (int i = 0; i < BULLET_COUNT; i++) {
            Bullet& b = bp[i];
            if (!b.active || b.IsPlayerSide()) continue;
            if (DistPointSegment(player.pos, b.prev, b.pos) < b.radius * 0.8f + Player::HIT_RADIUS) {
                b.active = false;
                fx.Burst(b.pos, 10, 20, 0.4f, 0.3f, b.color, C_GRAY);
                player.Damage(*this, 1);
                break;
            }
        }
    }
}

// ============================================================
//  차지샷 피격 지점 이펙트 (NEON의 UpdateChargeImpacts와 같은 구조, 1/60초 고정 간격)
// ============================================================
void Game::UpdateChargeImpacts(float dt)
{
    bool anyMissile = false;
    Bullet* bp = bullets.Data();
    for (int i = 0; i < BULLET_COUNT && !anyMissile; i++) anyMissile = bp[i].active && bp[i].type == BulletType::P_MISSILE;
    if (!player.Alive() || !(player.GigaLaserOn() || player.FlameOrbsOn() || anyMissile)) { impactTimer = 0; return; }
    impactTimer += dt;
    const float INTERVAL = 1.0f / 60.0f;
    if (impactTimer < INTERVAL) return;
    impactTimer -= INTERVAL;
    if (impactTimer > INTERVAL) impactTimer = 0;

    auto Impact = [&](Vec3 p, float power, Vec3 dir, uint16_t hot, uint16_t cool) {
        fx.Spawn(p, {}, 0.08f, 2.2f * power, 0.8f, C_BWHITE, hot);                      // 중심 섬광
        for (int i = 0; i < 6; i++) {                                                   // 튀는 불꽃
            Vec3 d = Normalize(dir + Vec3{ RandF(-1, 1), RandF(-1, 1), RandF(-1, 1) });
            fx.Spawn(p, d * RandF(15, 40) * power, RandF(0.15f, 0.35f), 0.3f, 0.1f, hot, cool, 4.0f);
        }
        if (RandI(0, 9) == 0) fx.Explosion(p, 0.5f * power, hot);
    };
    Monster* mp = monsters.Data();
    bool bossOn = boss.Fighting();

    if (player.GigaLaserOn()) {   // 노랑·청록 전기 불꽃이 레이저 반대쪽(-z)으로 튐
        Box3 l = player.GigaLaserBox();
        for (int k = 0; k < MONSTER_COUNT; k++)
            if (mp[k].active && l.Contains(mp[k].pos, mp[k].Radius()))
                Impact(mp[k].pos - Vec3{ 0, 0, mp[k].Radius() }, 1.0f, { 0, 0, -1 }, C_LYELLOW, C_LCYAN);
        if (bossOn && player.gigaLaserEndZ < FAR_Z) {
            for (int i = 0; i < 2; i++)
                Impact({ player.pos.x + RandF(-6, 6), player.pos.y + RandF(-5, 5), player.gigaLaserEndZ }, 1.5f, { 0, 0, -1 }, C_LYELLOW, C_LCYAN);
            fx.Shake(0.6f, 0.05f);
        }
    }
    if (player.FlameOrbsOn()) {   // 화염구가 닿은 지점에서 불꽃
        Vec3 c[4];
        player.FlameOrbCenters(c);
        for (auto& o : c) {
            for (int k = 0; k < MONSTER_COUNT; k++) {
                if (!mp[k].active || Dist(o, mp[k].pos) > Player::FLAME_ORB_RADIUS + mp[k].Radius()) continue;
                Vec3 dir = Normalize(mp[k].pos - o);
                Impact(mp[k].pos - dir * mp[k].Radius(), 1.1f, dir, C_LYELLOW, C_RED);
            }
            if (bossOn && boss.Overlaps(o, Player::FLAME_ORB_RADIUS)) { Impact(o, 1.6f, Normalize(boss.pos - o), C_LYELLOW, C_RED); fx.Shake(0.5f, 0.05f); }
        }
    }
    if (anyMissile) {   // 미사일 링이 닿은 지점에서 푸른 폭발
        bool hitAny = false;
        for (int i = 0; i < BULLET_COUNT; i++) {
            Bullet& b = bp[i];
            if (!b.active || b.type != BulletType::P_MISSILE) continue;
            float r = b.MissileRingRadius();
            for (int k = 0; k < MONSTER_COUNT; k++) {
                if (!mp[k].active || std::fabs(mp[k].pos.z - b.pos.z) > 3 + mp[k].Radius()) continue;
                float dx = mp[k].pos.x - b.pos.x, dy = mp[k].pos.y - b.pos.y;
                if (std::sqrt(dx * dx + dy * dy) > r + mp[k].Radius()) continue;
                Impact(mp[k].pos - Vec3{ 0, 0, mp[k].Radius() }, 1.0f, { 0, 0, 1 }, C_BWHITE, C_LBLUE);
                hitAny = true;
            }
            if (bossOn && std::fabs(boss.pos.z - b.pos.z) < boss.Half().z + 3) {
                Impact({ boss.pos.x + RandF(-8, 8), boss.pos.y + RandF(-6, 6), boss.pos.z - boss.Half().z }, 1.4f, { 0, 0, -1 }, C_BWHITE, C_LBLUE);
                hitAny = true;
            }
        }
        if (hitAny && RandI(0, 7) == 0) Audio::Play(S_EXPLODE_SMALL, 0.5f, 0.2f);
    }
}

// ============================================================
void Game::DrawWorld() const
{
    background.Draw();
    boss.Draw();
    monsters.Draw();
    items.Draw();
    bullets.Draw();
    player.Draw(*this);
    fx.Draw();
}

void Game::Draw() const
{
    switch (scene) {
    case Scene::LOGO:        background.Draw(); fx.Draw(); UI::DrawLogo(*this); break;
    case Scene::STAGE_INTRO: background.Draw(); fx.Draw(); UI::DrawHUD(*this); UI::DrawStageIntro(*this); break;
    case Scene::PLAYING:
    case Scene::PAUSED:
        DrawWorld();
        boss.DrawUI();
        UI::DrawHUD(*this);
        if (scene == Scene::PAUSED) UI::DrawPause(*this);
        break;
    case Scene::GAME_OVER:   background.Draw(); fx.Draw(); UI::DrawGameOver(*this); break;
    case Scene::CLEAR:       background.Draw(); fx.Draw(); UI::DrawClear(*this); break;
    }
    fx.DrawOverlay();
}
