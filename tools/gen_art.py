# 원본 소스(MainGame.cpp, Boss.cpp)에서 아스키 아트 배열을 그대로 뽑아 OriginalArt.h를 만든다.
# 타이틀 "NEON STRIKER"는 원본 아트에서 글자를 잘라 조합 (K만 같은 스타일로 새로 그림)
import re, sys
SRC = sys.argv[1]; OUT = sys.argv[2]

def arrays(fn):
    s = open(f"{SRC}/{fn}", encoding="utf-8-sig").read()
    out = {}
    for m in re.finditer(r'const char\*\s*(\w+)\s*(\[[^\]]*\])+\s*=\s*\{', s):
        i = s.index('{', m.start()); d = 0; j = i
        while True:
            if s[j] == '{': d += 1
            elif s[j] == '}':
                d -= 1
                if d == 0: break
            j += 1
        out[m.group(1)] = [l.encode().decode('unicode_escape') for l in re.findall(r'"([^"]*)"', s[i:j])]
    return out

A = arrays("MainGame.cpp"); B = arrays("Boss.cpp")

def glyphs(art):
    """빈 열(모든 줄이 공백)을 기준으로 글자 단위로 자른다"""
    w = max(len(l) for l in art); art = [l.ljust(w) for l in art]
    cols = [all(l[x] == ' ' for l in art) for x in range(w)]
    out = []; x = 0
    while x < w:
        if cols[x]: x += 1; continue
        s = x
        while x < w and not cols[x]: x += 1
        out.append([l[s:x] for l in art])
    return out

shoot = glyphs(A["titleArt"])           # S H O O T I N G
cont  = glyphs(A["continueArt"])        # C O N T I N U E ?
warn  = glyphs(B["artSquare"])          # W A R N I N G
L = {"S": shoot[0], "O": shoot[2], "T": shoot[4], "N": shoot[5], "I": warn[4], "E": cont[7], "R": warn[2]}  # 원본 TITLE은 T와 I가 붙어 있어 I는 WARNING에서 가져옴
L["K"] = ["####    ####", "####   #### ", "####  ####  ", "#########   ", "####  ####  ", "####   #### ", "####    ####"]

def word(text, gap=2, space=8):
    rows = [""] * 7
    for ch in text:
        if ch == " ":
            rows = [r + " " * space for r in rows]; continue
        g = L[ch]; w = max(len(l) for l in g)
        rows = [r + g[i].ljust(w) + " " * gap for i, r in enumerate(rows)]
    return [r.rstrip() for r in rows]

title = word("NEON STRIKER")
w = max(len(r) for r in title); title = [r.ljust(w) for r in title]

def arr(name, lines):
    body = ",\n".join('    "' + l.replace("\\", "\\\\").replace('"', '\\"') + '"' for l in lines)
    return f"static const char* const {name}[] = {{\n{body}\n}};\nstatic const int {name}_H = {len(lines)};\n\n"

h = "#pragma once\n// ============================================================\n"
h += "//  원본 ShootingGame의 아스키 아트 (tools/gen_art.py로 원본 소스에서 자동 추출)\n"
h += "//  TITLE_ART만 원본 글자를 잘라 'NEON STRIKER'로 조합 (K는 같은 스타일로 새로 그림)\n"
h += "// ============================================================\n\n"
h += arr("TITLE_ART", title)
h += arr("STAGE_ART", A["stageLogo"])
for k in ("num1", "num2", "num3", "num4"): h += arr("STAGE_" + k.upper(), A[k])
h += arr("PAUSE_ART", A["pauseArt"])
h += arr("WARNING_ART", B["artSquare"])
h += arr("GAMEOVER_ART", A["endArt"])
h += arr("CONTINUE_ART", A["continueArt"])
h += arr("CLEAR_ART", A["clearArt"])
nums = A["nums"]
h += "// CONTINUE 카운트다운 숫자 0~9 (각 15줄)\nstatic const char* const BIG_DIGITS[10][15] = {\n"
for d in range(10):
    h += "    { " + ", ".join('"' + l + '"' for l in nums[d * 15:(d + 1) * 15]) + " },\n"
h += "};\n\n"
h += "// 원본 보스 이름 (단계 1~4)\nstatic const char* const BOSS_NAMES[6] = { " + ", ".join('"' + n + '"' for n in B["bossNames"]) + " };\n"
open(OUT, "w").write(h)
print("\n".join(title))
