#pragma once
// ============================================================
//  화면 UI - 원본 MainGame의 아스키 아트 화면(로고/STAGE/PAUSE/GAME OVER/CONTINUE/CLEAR)과
//  원본 Player의 하단 HUD 레이아웃을 그대로 사용
// ============================================================
class Game;

namespace UI {
    void DrawLogo(const Game& g);
    void DrawStageIntro(const Game& g);
    void DrawHUD(const Game& g);
    void DrawPause(const Game& g);
    void DrawGameOver(const Game& g);
    void DrawClear(const Game& g);
}
