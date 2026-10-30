#include "Screen.h"
#include <cstring>

static Cell  g_cells[SCR_H][SCR_W];
static float g_depth[SCR_H][SCR_W];

void Screen::Clear()
{
    for (int y = 0; y < SCR_H; y++)
        for (int x = 0; x < SCR_W; x++) {
            g_cells[y][x] = { ' ', C_WHITE };
            g_depth[y][x] = 1e9f;
        }
}

Cell* Screen::Data() { return &g_cells[0][0]; }

void Screen::Put(int x, int y, char c, uint16_t attr)
{
    if (x < 0 || x >= SCR_W || y < 0 || y >= SCR_H) return;
    g_cells[y][x] = { c, attr };
    g_depth[y][x] = -1e9f;   // UI는 항상 맨 앞
}

bool Screen::PutDepth(int x, int y, float depth, char c, uint16_t attr)
{
    if (x < 0 || x >= SCR_W || y < 0 || y >= VIEW_H) return false;
    if (depth >= g_depth[y][x]) return false;   // 이미 더 가까운 것이 그려져 있음
    g_depth[y][x] = depth;
    g_cells[y][x] = { c, attr };
    return true;
}

void Screen::Text(int x, int y, const char* s, uint16_t attr)
{
    for (int i = 0; s[i]; i++) Put(x + i, y, s[i], attr);
}

void Screen::TextC(int y, const char* s, uint16_t attr)
{
    Text((SCR_W - (int)strlen(s)) / 2, y, s, attr);
}

int Screen::ArtWidth(const char* const* lines, int n)
{
    int w = 0;
    for (int i = 0; i < n; i++) { int l = (int)strlen(lines[i]); if (l > w) w = l; }
    return w;
}

void Screen::Art(int x, int y, const char* const* lines, int n, uint16_t attr, bool transparent)
{
    for (int i = 0; i < n; i++)
        for (int j = 0; lines[i][j]; j++) {
            if (transparent && lines[i][j] == ' ') continue;
            Put(x + j, y + i, lines[i][j], attr);
        }
}

void Screen::ArtC(int y, const char* const* lines, int n, uint16_t attr, bool transparent)
{
    Art((SCR_W - ArtWidth(lines, n)) / 2, y, lines, n, attr, transparent);
}

void Screen::Box(int x, int y, int w, int h, char c, uint16_t attr)
{
    for (int i = 0; i < w; i++) { Put(x + i, y, c, attr); Put(x + i, y + h - 1, c, attr); }
    for (int j = 0; j < h; j++) { Put(x, y + j, c, attr); Put(x + w - 1, y + j, c, attr); }
}

void Screen::FillRect(int x, int y, int w, int h, char c, uint16_t attr)
{
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++) Put(x + i, y + j, c, attr);
}

void Screen::Tint(uint16_t bgAttr)
{
    for (int y = 0; y < SCR_H; y++)
        for (int x = 0; x < SCR_W; x++) g_cells[y][x].attr = (uint16_t)((g_cells[y][x].attr & 0x0F) | bgAttr);
}
