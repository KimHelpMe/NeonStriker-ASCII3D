#pragma once
// ============================================================
//  플랫폼 계층: 콘솔 출력 / 키 입력 / 시간
//  - Windows: 원본 BeginPlay(폰트 자동 맞춤) + DoubleBuffering(화면 버퍼 2개 교체) 방식
//  - 그 외(Linux): 자동 테스트용. 스크립트로 키 입력을 흉내 내고 화면을 파일로 저장
//  게임 코드는 이 인터페이스만 쓰므로, 출력 방식이 바뀌어도 게임 코드는 그대로다.
// ============================================================
#include "Screen.h"

enum Key {
    K_LEFT, K_RIGHT, K_UP, K_DOWN, K_SPACE, K_ENTER, K_ESC,
    K_X, K_C, K_R, K_E, K_1, K_2, K_3, K_4, K_5, K_6,
    K_COUNT
};

namespace Platform {
    bool   Init();
    void   Shutdown();
    void   Present(const Cell* cells);   // SCR_W x SCR_H 버퍼를 화면에 출력
    bool   RawKeyDown(Key k);
    double Now();                        // 초 단위 시간
    void   SleepMs(int ms);
    bool   QuitRequested();              // 테스트 스크립트 종료 요청
    void   GoToExeDirectory();           // 실행 파일 폴더 기준으로 assets를 찾도록
}

// 매 프레임 한 번 Update -> '방금 눌렀는지(Pressed)'를 계산 (원본의 bCKeyPush 같은 플래그를 일반화)
namespace Input {
    void Update();
    bool Down(Key k);
    bool Pressed(Key k);
    bool AnyPressed();
}
