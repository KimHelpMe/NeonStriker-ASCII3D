# 테스트용: Linux 빌드가 저장한 화면 덤프(.bin)를 콘솔처럼 보이는 PNG로 변환
# 색은 Windows 콘솔 기본 팔레트(Campbell)
import struct, sys
from PIL import Image, ImageDraw, ImageFont
PAL = [(12,12,12),(0,55,218),(19,161,14),(58,150,221),(197,15,31),(136,23,152),(193,156,0),(204,204,204),
       (118,118,118),(59,120,255),(22,198,12),(97,214,214),(231,72,86),(180,0,158),(249,241,165),(242,242,242)]
font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf", 14)
CW, CH = 8, 16
for path in sys.argv[1:]:
    data = open(path, "rb").read()
    w, h = struct.unpack("ii", data[:8]); cells = data[8:]
    img = Image.new("RGB", (w * CW, h * CH), PAL[0]); d = ImageDraw.Draw(img)
    for y in range(h):
        for x in range(w):
            ch = chr(cells[(y * w + x) * 2]); a = cells[(y * w + x) * 2 + 1]
            bg = (a >> 4) & 15
            if bg: d.rectangle([x * CW, y * CH, x * CW + CW - 1, y * CH + CH - 1], fill=PAL[bg])
            if ch != " ": d.text((x * CW, y * CH), ch, font=font, fill=PAL[a & 15])
    img.save(path[:-4] + ".png")
