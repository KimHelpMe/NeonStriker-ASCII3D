#include "Platform.h"
#include <chrono>
#include <thread>

// ------------------------------------------------------------
//  공통: 입력 상태 (이전 프레임 / 현재 프레임 비교)
// ------------------------------------------------------------
static bool g_prev[K_COUNT], g_curr[K_COUNT];

void Input::Update()
{
    for (int i = 0; i < K_COUNT; i++) {
        g_prev[i] = g_curr[i];
        g_curr[i] = Platform::RawKeyDown((Key)i);
    }
}
bool Input::Down(Key k) { return g_curr[k]; }
bool Input::Pressed(Key k) { return g_curr[k] && !g_prev[k]; }
bool Input::AnyPressed()
{
    for (int i = 0; i < K_COUNT; i++) if (Pressed((Key)i)) return true;
    return false;
}

double Platform::Now()
{
    static auto start = std::chrono::steady_clock::now();
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
}

void Platform::SleepMs(int ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }

#ifdef _WIN32
// ============================================================
//  Windows 콘솔
// ============================================================
#include <windows.h>
#include <mmsystem.h>

static HANDLE g_screen[2];
static int    g_index = 0;
static CHAR_INFO g_ci[SCR_W * SCR_H];

bool Platform::Init()
{
    timeBeginPeriod(1);   // Sleep 정밀도를 1ms로 (기본 약 15ms라 프레임이 들쭉날쭉해짐)
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTitleA("NEON STRIKER 3D");

    // ---- 원본 BeginPlay: 75줄 x 240칸이 화면에 꽉 차도록 폰트 크기 자동 계산 ----
    int screenPixelH = GetSystemMetrics(SM_CYSCREEN);
    int fontH = (screenPixelH - 120) / SCR_H;
    if (fontH < 8) fontH = 8;
    if (fontH > 18) fontH = 18;
    int fontW = fontH / 2;   // 가로:세로 = 1:2
    if (fontW < 4) fontW = 4;
    if (fontW > 10) fontW = 10;

    CONSOLE_FONT_INFOEX cfi = {};
    cfi.cbSize = sizeof(cfi);
    cfi.dwFontSize.X = (SHORT)fontW;
    cfi.dwFontSize.Y = (SHORT)fontH;
    cfi.FontFamily = FF_DONTCARE;
    cfi.FontWeight = FW_NORMAL;
    wcscpy_s(cfi.FaceName, L"Consolas");
    SetCurrentConsoleFontEx(hOut, FALSE, &cfi);

    COORD bufferSize = { (SHORT)SCR_W, (SHORT)SCR_H };
    SetConsoleScreenBufferSize(hOut, bufferSize);
    SMALL_RECT windowSize = { 0, 0, (SHORT)(SCR_W - 1), (SHORT)(SCR_H - 1) };
    SetConsoleWindowInfo(hOut, TRUE, &windowSize);

    HWND hwnd = GetConsoleWindow();
    LONG style = GetWindowLong(hwnd, GWL_STYLE);
    style &= ~WS_SIZEBOX;
    style &= ~WS_MAXIMIZEBOX;
    SetWindowLong(hwnd, GWL_STYLE, style);
    SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);

    RECT cr, wr;
    GetClientRect(hwnd, &cr);
    GetWindowRect(hwnd, &wr);
    int borderW = (wr.right - wr.left) - (cr.right - cr.left);
    int borderH = (wr.bottom - wr.top) - (cr.bottom - cr.top);
    SetWindowPos(hwnd, HWND_TOP, 0, 0, SCR_W * fontW + borderW, SCR_H * fontH + borderH, SWP_NOMOVE);

    // ---- 원본 DoubleBuffering::ScreenInit: 화면 버퍼 2개 ----
    CONSOLE_CURSOR_INFO cci = { 1, FALSE };
    for (int i = 0; i < 2; i++) {
        g_screen[i] = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
        SetConsoleScreenBufferSize(g_screen[i], bufferSize);
        SetConsoleCursorInfo(g_screen[i], &cci);
        // 새로 만든 버퍼에도 같은 폰트 적용 (안 하면 기본 폰트로 돌아가는 경우가 있음)
        SetCurrentConsoleFontEx(g_screen[i], FALSE, &cfi);
    }
    return true;
}

void Platform::Shutdown()
{
    SetConsoleActiveScreenBuffer(GetStdHandle(STD_OUTPUT_HANDLE));
    for (int i = 0; i < 2; i++) CloseHandle(g_screen[i]);
    timeEndPeriod(1);
}

void Platform::Present(const Cell* cells)
{
    // 원본 ScreenFlipping: 화면 전체를 한 번에 복사한 뒤 활성 버퍼를 교체
    for (int i = 0; i < SCR_W * SCR_H; i++) {
        g_ci[i].Char.AsciiChar = cells[i].ch;
        g_ci[i].Attributes = cells[i].attr;
    }
    COORD size = { (SHORT)SCR_W, (SHORT)SCR_H };
    COORD origin = { 0, 0 };
    SMALL_RECT region = { 0, 0, (SHORT)(SCR_W - 1), (SHORT)(SCR_H - 1) };
    WriteConsoleOutputA(g_screen[g_index], g_ci, size, origin, &region);
    SetConsoleActiveScreenBuffer(g_screen[g_index]);
    g_index = !g_index;
}

bool Platform::RawKeyDown(Key k)
{
    // 원본과 같은 GetAsyncKeyState
    // (창 포커스 검사는 넣지 않음: Windows 11의 Windows Terminal에서는 GetConsoleWindow가
    //  실제 창이 아니라서, 검사하면 입력이 전부 막힘)
    static const int VK[K_COUNT] = {
        VK_LEFT, VK_RIGHT, VK_UP, VK_DOWN, VK_SPACE, VK_RETURN, VK_ESCAPE,
        'X', 'C', 'R', 'E', '1', '2', '3', '4', '5', '6',
    };
    return (GetAsyncKeyState(VK[k]) & 0x8000) != 0;
}

bool Platform::QuitRequested() { return false; }

void Platform::GoToExeDirectory()
{
    char path[MAX_PATH];
    DWORD n = GetModuleFileNameA(NULL, path, MAX_PATH);
    for (int i = (int)n - 1; i >= 0; i--)
        if (path[i] == '\\' || path[i] == '/') { path[i] = 0; break; }
    SetCurrentDirectoryA(path);
}

#else
// ============================================================
//  Linux 자동 테스트용
//  NS3D_SCRIPT=파일 : "시간 명령 인자" 줄 목록
//     예) 1.0 tap ENTER / 2.0 down SPACE / 4.0 up SPACE / 5.0 shot title / 30 quit
//  NS3D_SHOTDIR=폴더 : shot 명령 시 그 시점 화면을 <이름>.bin 으로 저장 (tools/render_shot.py로 PNG 변환)
// ============================================================
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <unistd.h>

struct ScriptCmd { double t; std::string cmd, arg; bool done; };
static std::vector<ScriptCmd> g_script;
static bool g_keys[K_COUNT];
static bool g_quit = false;
static std::string g_shotDir = ".";
static const char* KEY_NAMES[K_COUNT] = {
    "LEFT", "RIGHT", "UP", "DOWN", "SPACE", "ENTER", "ESC", "X", "C", "R", "E", "1", "2", "3", "4", "5", "6",
};

static int KeyFromName(const std::string& s)
{
    for (int i = 0; i < K_COUNT; i++) if (s == KEY_NAMES[i]) return i;
    return -1;
}

bool Platform::Init()
{
    if (const char* d = getenv("NS3D_SHOTDIR")) g_shotDir = d;
    if (const char* f = getenv("NS3D_SCRIPT")) {
        FILE* fp = fopen(f, "r");
        char cmd[64], arg[64];
        double t;
        while (fp && fscanf(fp, "%lf %63s %63s", &t, cmd, arg) >= 2) {
            g_script.push_back({ t, cmd, arg, false });
            if (std::string(cmd) == "tap") g_script.push_back({ t + 0.12, "up", arg, false });
            arg[0] = 0;
        }
        if (fp) fclose(fp);
    }
    return true;
}

void Platform::Shutdown() {}

void Platform::Present(const Cell* cells)
{
    double now = Now();
    for (auto& c : g_script) {
        if (c.done || c.t > now) continue;
        c.done = true;
        int k = KeyFromName(c.arg);
        if ((c.cmd == "down" || c.cmd == "tap") && k >= 0) g_keys[k] = true;
        else if (c.cmd == "up" && k >= 0) g_keys[k] = false;
        else if (c.cmd == "quit") g_quit = true;
        else if (c.cmd == "shot") {
            std::string path = g_shotDir + "/" + c.arg + ".bin";
            if (FILE* fp = fopen(path.c_str(), "wb")) {
                int wh[2] = { SCR_W, SCR_H };
                fwrite(wh, sizeof(int), 2, fp);
                for (int i = 0; i < SCR_W * SCR_H; i++) {
                    fputc(cells[i].ch, fp);
                    fputc(cells[i].attr & 0xFF, fp);
                }
                fclose(fp);
            }
        }
    }
}

bool Platform::RawKeyDown(Key k) { return g_keys[k]; }
bool Platform::QuitRequested() { return g_quit; }

void Platform::GoToExeDirectory()
{
    char path[4096];
    ssize_t n = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (n <= 0) return;
    path[n] = 0;
    if (char* p = strrchr(path, '/')) *p = 0;
    if (chdir(path) != 0) {}
}
#endif
