// ============================================================
//  NEON STRIKER 3D (ASCII) - 진입점
//  원본 ShootingGame의 main 루프와 같은 흐름:
//    화면 지우기 -> Tick(Update) -> Draw -> 화면 교체(Present) -> 잠깐 쉬기
// ============================================================
#include "Game.h"
#include "Platform.h"
#include "Audio.h"
#include "Mesh.h"
#include "Render3D.h"
#include <ctime>

int main()
{
    std::srand((unsigned)std::time(nullptr));
    Platform::GoToExeDirectory();   // assets/sounds를 실행 파일 폴더 기준으로 찾음
    Platform::Init();
    Audio::Init();
    Meshes::Build();

    static Game game;   // 탄환 4000개 등 크기가 커서 스택 대신 정적 영역에 둠
    game.Init();

    const double FRAME = 1.0 / 60.0;
    double prev = Platform::Now();
    while (!game.quit && !Platform::QuitRequested()) {
        double frameStart = Platform::Now();
        float dt = (float)(frameStart - prev);
        prev = frameStart;
        if (dt > 1.0f / 20.0f) dt = 1.0f / 20.0f;   // 창 드래그 등으로 멈췄다 돌아와도 순간이동 방지

        Input::Update();
        game.Update(dt);

        Screen::Clear();
        game.Draw();
        Platform::Present(Screen::Data());

        // 60fps 유지 (원본은 Sleep(8) 고정)
        double spent = Platform::Now() - frameStart;
        if (spent < FRAME) Platform::SleepMs((int)((FRAME - spent) * 1000));
    }

    Audio::Shutdown();
    Platform::Shutdown();
    return 0;
}
