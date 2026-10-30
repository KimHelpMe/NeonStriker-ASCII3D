# NEON STRIKER 3D (ASCII)

NEON STRIKER의 게임 로직을 기반으로, 원본 콘솔 ShootingGame의 화면 감성(글자 출력, 원본 아스키 아트 화면)을 살려서 만든 **콘솔 아스키 3D 슈팅 게임**.
기체 뒤에서 앞을 보는 시점이고, 적은 화면 안쪽에서 다가온다.

## 바로 실행

`Play\NeonStriker3D.exe`를 실행한다. 같은 폴더의 `assets\sounds`가 함께 있어야 소리가 난다.
- **화면이 깨지거나 줄이 밀리면:** Windows 11 기본 터미널(Windows Terminal)은 콘솔 폰트·창 크기 변경을 무시한다. 원본 게임과 같은 제약이다.
  - 설정 → 개인 정보 및 보안 → 개발자용 → 터미널을 **"Windows 콘솔 호스트"**로 바꾼 뒤 다시 실행한다.
- 서명 없는 exe라서 SmartScreen 경고가 뜨면 "추가 정보 → 실행"을 누른다.

## 조작

| 키 | 기능 |
|---|---|
| 방향키 | 상하좌우 이동 (타이틀에서는 ↑↓로 난이도 선택) |
| Enter / Space | 시작 |
| Space | 발사 |
| X (무기 Lv5) | 3초 차지 → 떼면 차지샷 |
| C | 폭탄 |
| ESC | 일시정지 (ESC 재개 / R 타이틀 / E 종료) |
| 테스트 키 | 1 무기 레벨업 · 2/3/4 NORMAL/FLAME/LASER · 5 피버 · 6 보스 즉시 등장 |

## 어떻게 만들었나

| 부분 | 가져온 곳 | 설명 |
|---|---|---|
| 게임 로직 | NEON STRIKER | 씬 상태, enum 탄 종류, 난이도 표, 무기 3종 × Lv5, 차지샷 3종, 폭탄, 피버, 아이템, 보스 4단계, 충돌을 한 곳에 모은 구조 |
| 화면 출력 | 원본 ShootingGame | 240×75 글자 버퍼, 화면 버퍼 2개 교체(더블버퍼링), 모니터에 맞춘 폰트 자동 계산 |
| 아스키 아트 | 원본 ShootingGame | WARNING / STAGE / PAUSE / GAME OVER / CONTINUE + 숫자 0~9 / MISSION COMPLETE / 보스 이름. `tools/gen_art.py`가 원본 소스에서 자동으로 뽑는다. 타이틀 "NEON STRIKER"는 원본 글자를 잘라 조합했다(K만 새로 그림) |
| 3D 렌더러 | 새로 작성 | `Render3D.cpp` (아래 참고) |
| 효과음 | NEON STRIKER | 같은 WAV 28개를 miniaudio로 재생 |

### 3D 렌더러가 하는 일 (`src/Render3D.cpp`)
1. **모델 → 월드:** 모델 좌표에 회전·크기·이동을 적용한다(`Math3D.h`의 `Rotate`).
2. **카메라 기준으로 변환:** 월드 좌표에서 카메라 위치를 뺀다. 카메라는 기체 뒤 위쪽에 있다.
3. **원근 투영:** `화면x = 중심 + x / z × F`. 멀수록(z가 클수록) 작게 보인다. 콘솔 글자는 세로가 가로의 2배라서 y에는 0.5를 곱한다.
4. **삼각형 채우기 + 깊이 버퍼(Z-버퍼):** 칸마다 가장 가까운 물체만 남긴다(`Screen::PutDepth`).
5. **셰이딩:** 면이 빛을 받는 정도에 따라 글자를 고른다. `. , : ; - = + * # % @` 순서로 밝아진다.

모델은 이미지가 아니라 꼭짓점 좌표로 정의한다(`src/Mesh.cpp`).

## 원본·NEON과 달라진 점
- **시점이 바뀌어서 탄막 패턴을 3D로 다시 설계했다.** 예를 들어 4단계 소용돌이는 회전하며 다가오는 '터널'이 되었다. 체력, 데미지, 킬수, 드랍률 같은 수치는 그대로다.
- **화염 Lv5는 원본의 '기체 주변 12갈래 회전 화염' 대신 기수에서 앞으로 곧게 뻗는 원뿔 모양이다.** 3D에서는 회전 화염이 카메라 가까이 겹쳐 화면을 덮고, 옆으로 퍼져서 멀리 있는 보스에 닿지 않았기 때문이다.
- **카메라 가까운 물체는 그리지 않는다.** 플레이어를 지나친 적과 적 탄이 화면을 덮지 않도록 제거한다.
- **보스 피격 번쩍임은 0.2초에 한 번만 일어난다.** 계속 맞으면 하얗게만 보여서 보스 고유색이 안 보였기 때문이다.

## 직접 빌드
- **Visual Studio:** `NeonStriker3D.sln`을 열고 x64 / Release로 빌드한다. `assets`는 `bin\Release\`로 자동 복사된다. 외부 라이브러리 설치는 필요 없다(miniaudio는 헤더 파일 하나).
- **MinGW:** `g++ -std=c++17 -O2 src/*.cpp -o NeonStriker3D.exe -static -lwinmm`

### 자동 테스트 (Linux)
Windows가 아닌 환경에서 빌드하면 `Platform.cpp`의 테스트용 구현이 쓰인다. 스크립트로 키 입력을 흉내 내고, 화면을 파일로 저장한다.
```
g++ -std=c++17 -O2 src/*.cpp -o ns3d -ldl -lpthread -lm
NS3D_SCRIPT=/절대경로/script.txt NS3D_SHOTDIR=/절대경로/out ./ns3d
python3 tools/render_shot.py out/*.bin      # 저장한 화면을 PNG로
```
스크립트 예시: `1.0 tap ENTER` / `5.0 down SPACE` / `8.0 up SPACE` / `6.0 shot play` / `10 quit x`
`-DNEON_GODMODE`로 빌드하면 플레이어가 무적이 된다(테스트용).

## 라이선스
- miniaudio: Public Domain 또는 MIT-0 (`src/miniaudio.h` 끝부분 참고)
- 효과음: NEON STRIKER에서 코드로 직접 생성한 것
- 아스키 아트: 원본 ShootingGame
