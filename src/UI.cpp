#include "UI.h"
#include "Game.h"
#include "Screen.h"
#include "OriginalArt.h"
#include <cstdio>
#include <cstring>

static const uint16_t STAGE_COLOR[5] = { C_WHITE, C_LCYAN, C_LGREEN, C_LYELLOW, C_LMAGENTA };

// 원본 GetStatBar: [■■■□□] -> 콘솔 코드페이지와 상관없이 보이도록 [###..] 로
static void StatBar(char* out, int current, int max, int width)
{
    int filled = max > 0 ? current * width / max : 0;
    if (filled < 0) filled = 0;
    if (filled > width) filled = width;
    out[0] = '[';
    for (int i = 0; i < width; i++) out[1 + i] = i < filled ? '#' : '.';
    out[1 + width] = ']';
    out[2 + width] = 0;
}

// ------------------------------------------------------------
void UI::DrawLogo(const Game& g)
{
    float t = g.sceneTime;
    int artW = Screen::ArtWidth(TITLE_ART, TITLE_ART_H);
    int startY = SCR_H / 2 - 16;
    int boxW = artW + 12, boxH = 27;
    int bX = (SCR_W - boxW) / 2, bY = startY - 3;

    // 테두리 깜빡임 (노랑/흰색) - 원본 Logo와 같은 연출
    Screen::FillRect(bX, bY, boxW, boxH, ' ', C_WHITE);
    Screen::Box(bX, bY, boxW, boxH, '#', ((int)(t * 2) % 2 == 0) ? C_LYELLOW : C_BWHITE);
    // 타이틀 페이드 (sin 값에 따라 밝은/어두운 청록)
    Screen::ArtC(startY, TITLE_ART, TITLE_ART_H, std::sin(t * 1.2f) > 0 ? C_LCYAN : C_CYAN);
    Screen::TextC(startY + 8, "-  A S C I I   3 D  -", C_LMAGENTA);
    Screen::TextC(startY + 10, "-----------------------------------------------------------------------------------------", C_BLUE);

    // 난이도 메뉴
    for (int i = 0; i < 2; i++) {
        bool sel = (int)g.difficulty == i;
        char s[48];
        snprintf(s, sizeof(s), sel ? ">>   %s   <<" : "%s", DIFFICULTY_RULES[i].name);
        uint16_t c = sel ? ((int)(t * 4) % 2 ? (i ? C_LRED : C_LCYAN) : C_BWHITE) : C_GRAY;
        Screen::TextC(startY + 12 + i * 2, s, c);
    }
    const DifficultyRules& r = g.Rules();
    char s[96];
    snprintf(s, sizeof(s), "ENEMIES %d   BOSS HP x%.0f   BOSS AFTER %d KILLS", r.monsterCount, r.bossHpScale, r.killRequirement);
    Screen::TextC(startY + 17, s, C_WHITE);

    // 원본 PRESS ANY KEY 페이드 (밝은 노랑 -> 노랑 -> 회색 -> 사라짐)
    float f = std::sin(t * 1.5f * 3);
    const char* msg = "- UP / DOWN : SELECT     ENTER : START -";
    if (f > 0.6f) Screen::TextC(startY + 20, msg, C_LYELLOW);
    else if (f > 0.2f) Screen::TextC(startY + 20, msg, C_YELLOW);
    else if (f > -0.2f) Screen::TextC(startY + 20, msg, C_GRAY);

    Screen::TextC(bY + boxH + 2, "MOVE: ARROWS    SHOOT: SPACE    CHARGE (Lv5): X    BOMB: C    PAUSE: ESC", C_WHITE);
    Screen::TextC(bY + boxH + 4, "TEST KEYS   1: Weapon Lv+   2/3/4: Normal/Flame/Laser   5: Fever   6: Boss", C_GRAY);
}

void UI::DrawStageIntro(const Game& g)
{
    float t = g.sceneTime;
    // 원본 ShowStageStart: "STAGE" 로고 + 숫자 아트를 나란히
    static const char* const* NUM[5] = { nullptr, STAGE_NUM1, STAGE_NUM2, STAGE_NUM3, STAGE_NUM4 };
    int logoW = Screen::ArtWidth(STAGE_ART, STAGE_ART_H), numW = Screen::ArtWidth(NUM[g.stage], 7);
    int total = logoW + 5 + numW, x = (SCR_W - total) / 2, y = SCR_H / 2 - 8;
    uint16_t c = STAGE_COLOR[g.stage];

    // 시작 직후 0.35초: 원본 Logo의 '수평 비산' 줄무늬
    if (t < 0.35f) {
        static const char* streaks[] = { "==========", "====================", "==============================" };
        for (int i = 0; i < 14; i++)
            Screen::Text(RandI(0, SCR_W - 30), RandI(y - 6, y + 14), streaks[RandI(0, 2)], RandI(0, 1) ? C_BWHITE : C_LYELLOW);
        return;
    }
    Screen::Art(x, y, STAGE_ART, STAGE_ART_H, c);
    Screen::Art(x + logoW + 5, y, NUM[g.stage], 7, c);
    if ((int)(t * 4) % 2 == 0 || t < 1.0f)
        Screen::TextC(y + 10, g.stage == LAST_STAGE ? "- FINAL BATTLE -" : "- GET READY -", C_BWHITE);
}

// ------------------------------------------------------------
//  하단 HUD (원본 Player::PlayerDraw 하단 UI 레이아웃)
// ------------------------------------------------------------
void UI::DrawHUD(const Game& g)
{
    const Player& p = g.player;
    float t = g.sceneTime;
    int lineBar = SCR_H - 4;
    for (int x = 0; x < SCR_W; x++) Screen::Put(x, lineBar, '=', C_BLUE);
    Screen::FillRect(0, lineBar + 1, SCR_W, 3, ' ', C_WHITE);

    uint16_t fever = RainbowColor(t);
    char s[160], bar[64];

    // 좌측: FPS / 스테이지 / 킬
    snprintf(s, sizeof(s), "FPS: %d", g.fps);
    Screen::Text(2, lineBar + 1, s, C_GRAY);
    snprintf(s, sizeof(s), "STAGE %d   [%s]", g.stage, g.Rules().name);
    Screen::Text(2, lineBar + 2, s, STAGE_COLOR[g.stage]);
    if (!g.boss.Present()) snprintf(s, sizeof(s), "KILL %d   BOSS %d / %d", p.kills, g.boss.killCount, g.Rules().killRequirement);
    else snprintf(s, sizeof(s), "KILL %d", p.kills);
    Screen::Text(2, lineBar + 3, s, C_LMAGENTA);

    // 중앙: EXP / HP
    StatBar(bar, p.exp, p.maxExp, 30);
    snprintf(s, sizeof(s), "LV.%d EXP %s %3d%%", p.level, bar, p.exp * 100 / (p.maxExp > 0 ? p.maxExp : 1));
    Screen::Text(80, lineBar + 1, s, p.fever ? fever : C_LCYAN);
    StatBar(bar, p.hp, p.maxHp, 5 * 6);
    snprintf(s, sizeof(s), "HP       %s", bar);
    Screen::Text(80, lineBar + 2, s, p.fever ? fever : C_LYELLOW);
    if (p.fever) { snprintf(s, sizeof(s), "*** FEVER TIME %.1f ***", p.feverTimer); Screen::Text(80, lineBar + 3, s, fever); }
    else if (p.charging) {
        StatBar(bar, (int)(p.chargeTimer * 100), (int)(MAX_CHARGE_TIME * 100), 30);
        snprintf(s, sizeof(s), "CHARGE   %s", bar);
        Screen::Text(80, lineBar + 3, s, p.chargeTimer >= MAX_CHARGE_TIME ? C_BWHITE : C_LYELLOW);
    }

    // 우측: 무기 정보 / 폭탄 / 목숨 (원본: 5레벨이면 노랑-흰색 반짝임)
    const char* wname = p.weapon == WeaponType::NORMAL ? "NORMAL" : p.weapon == WeaponType::FLAME ? "FLAME" : "LASER";
    uint16_t wc = p.weapon == WeaponType::NORMAL ? C_LCYAN : p.weapon == WeaponType::FLAME ? C_LRED : C_LYELLOW;
    if (p.weaponLevel >= MAX_WEAPON_LEVEL) wc = (int)(t * 5) % 2 ? C_LYELLOW : C_BWHITE;
    if (p.weaponLevel >= MAX_WEAPON_LEVEL) snprintf(s, sizeof(s), "WEAPON : %s  [ MAX ]", wname);
    else snprintf(s, sizeof(s), "WEAPON : %s  [ Lv.%d ]", wname, p.weaponLevel);
    Screen::Text(SCR_W - (int)strlen(s) - 2, lineBar + 1, s, wc);

    char bombs[40] = "";
    if (p.fever) snprintf(bombs, sizeof(bombs), "INFINITE");
    else for (int i = 0; i < (p.bombs > 8 ? 8 : p.bombs); i++) strcat(bombs, "(*)");
    char lives[24] = "";
    for (int i = 0; i < (p.lives > 0 ? p.lives : 0); i++) strcat(lives, "<A> ");
    snprintf(s, sizeof(s), "BOMB : %-24s LIFE : %s", bombs, lives);
    Screen::Text(SCR_W - (int)strlen(s) - 2, lineBar + 2, s, C_BWHITE);
}

// ------------------------------------------------------------
void UI::DrawPause(const Game& g)
{
    float t = g.sceneTime;
    int artW = Screen::ArtWidth(PAUSE_ART, PAUSE_ART_H);
    int boxW = artW + 12, boxH = 17;
    int bX = (SCR_W - boxW) / 2, bY = SCR_H / 2 - 10;
    // 원본 ShowPause: 흰색/회색 깜빡이는 박스 + PAUSE 아트
    bool bright = (int)(t * 2) % 2 == 0;
    Screen::FillRect(bX, bY, boxW, boxH, ' ', C_WHITE);
    Screen::Box(bX, bY, boxW, boxH, '#', bright ? C_BWHITE : C_GRAY);
    Screen::ArtC(bY + 2, PAUSE_ART, PAUSE_ART_H, bright ? C_BWHITE : C_GRAY);
    Screen::TextC(bY + 10, "------------------------------------------------", C_LCYAN);
    if ((int)(t * 3) % 3) Screen::TextC(bY + 12, "ESC : RESUME", C_LYELLOW);
    Screen::TextC(bY + 14, "R : TITLE          E : EXIT", C_WHITE);
}

void UI::DrawGameOver(const Game& g)
{
    float t = g.sceneTime;
    if (t < 2.0f) {   // 원본: MISSION FAILED 정지 화면
        Screen::ArtC(SCR_H / 2 - 8, GAMEOVER_ART, GAMEOVER_ART_H, C_LRED);
        Screen::TextC(SCR_H / 2 + 2, "- MISSION FAILED -", C_BWHITE);
        return;
    }
    // 원본 CONTINUE? + 15줄짜리 큰 숫자 카운트다운
    char s[96];
    if (g.allClear) Screen::TextC(3, "*** ALL STAGES CLEAR ***", C_LYELLOW);
    snprintf(s, sizeof(s), "SCORE   KILL %d   LV.%d   [%s]", g.player.kills, g.player.level, g.Rules().name);
    Screen::TextC(5, s, C_WHITE);
    Screen::ArtC(9, CONTINUE_ART, CONTINUE_ART_H, C_LRED);

    int num = (int)g.countdown;
    if (num < 0) num = 0;
    if (num > 9) num = 9;
    if (std::sin(g.countdown * 12.0f) > -0.3f)   // 번쩍이며 깜빡이는 숫자 (원본)
        Screen::ArtC(20, BIG_DIGITS[num], 15, std::sin(g.countdown * 12.0f) > 0.3f ? C_LYELLOW : C_YELLOW);
    if ((int)(t * 2) % 2 == 0) Screen::TextC(40, "R : RESTART          E : EXIT", C_LRED);
    Screen::TextC(42, "( any other key speeds up the countdown )", C_GRAY);
}

void UI::DrawClear(const Game& g)
{
    // 원본 GameClear: MISSION COMPLETE 아트가 밝은 노랑/노랑으로 반짝임
    float t = g.sceneTime;
    Screen::ArtC(SCR_H / 2 - 12, CLEAR_ART, CLEAR_ART_H, std::sin(t * 10) > 0 ? C_LYELLOW : C_YELLOW);
    Screen::TextC(SCR_H / 2 + 6, "CONGRATULATIONS!  YOU SAVED THE GALAXY", C_BWHITE);
    if (t > 1.5f && (int)(t * 2) % 2 == 0) Screen::TextC(SCR_H / 2 + 9, "- PRESS ANY KEY -", C_GRAY);
}
