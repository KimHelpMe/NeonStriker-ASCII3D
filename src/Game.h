#pragma once
// ============================================================
//  게임 전체 관리 (NEON STRIKER Game 구조 그대로: 씬 상태 + 월드 객체 + 충돌 처리)
// ============================================================
#include "Config.h"
#include "Background.h"
#include "Effects.h"
#include "Bullet.h"
#include "Player.h"
#include "Monster.h"
#include "Item.h"
#include "Boss.h"

enum class Scene { LOGO, STAGE_INTRO, PLAYING, PAUSED, GAME_OVER, CLEAR };

class Game {
public:
    void Init();
    void Update(float dt);
    void Draw() const;
    void DrawWorld() const;
    bool quit = false;

    Background    background;
    Effects       fx;
    BulletSystem  bullets;
    Player        player;
    MonsterSystem monsters;
    ItemSystem    items;
    Boss          boss;

    Scene scene = Scene::LOGO;
    float sceneTime = 0;
    int   stage = 1;
    float countdown = 9.99f;
    bool  allClear = false;
    int   fps = 0;
    Difficulty difficulty = Difficulty::NORMAL;
    const DifficultyRules& Rules() const { return DIFFICULTY_RULES[(int)difficulty]; }

private:
    void ChangeScene(Scene s);
    void NewGame();
    void StartStage(int s);
    void UpdatePlaying(float dt);
    void UpdateCamera();
    void UpdateLaserBlocking();
    void ResolveCollisions();
    void UpdateChargeImpacts(float dt);
    void StopLoopSounds();

    float stageEndDelay = 0, impactTimer = 0, fpsTimer = 0;
    int   fpsCount = 0, lastCountdownNum = 9;
};
