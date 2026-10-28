# -*- coding: utf-8 -*-
"""
فونت فارسی کوچک برای نوشتن «هر متنی» روی OLED (پیام‌هایی که از گوشی می‌رسند).
حروف فارسی به شکل‌های چسبان (اول/وسط/آخر/تنها) تبدیل می‌شوند و از راست به چپ کشیده می‌شوند.
خروجی: src/moochi_font.h      اجرا:  python tools/make_font.py
"""
import os, unicodedata
from PIL import Image, ImageDraw, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
FONT = os.path.join(HERE, "fonts", "Vazirmatn-Bold.ttf")
OUT = os.path.join(HERE, "..", "src", "moochi_font.h")
SIZE = 13

f = ImageFont.truetype(FONT, SIZE, layout_engine=ImageFont.Layout.BASIC)

# حروف پایه (فارسی + عربی رایج)
BASE = [0x621, 0x622, 0x623, 0x624, 0x625, 0x626, 0x627, 0x628, 0x629, 0x62A, 0x62B, 0x62C, 0x62D,
        0x62E, 0x62F, 0x630, 0x631, 0x632, 0x633, 0x634, 0x635, 0x636, 0x637, 0x638, 0x639, 0x63A,
        0x641, 0x642, 0x643, 0x644, 0x645, 0x646, 0x647, 0x648, 0x649, 0x64A, 0x67E, 0x686, 0x698,
        0x6A9, 0x6AF, 0x6CC]

forms = {}       # base -> {iso,fin,ini,med}
lamalef = {}     # alef -> (iso, fin)
for cp in list(range(0xFB50, 0xFE00)) + list(range(0xFE70, 0xFF00)):
    try:
        dec = unicodedata.decomposition(chr(cp))
    except ValueError:
        continue
    if not dec.startswith("<"):
        continue
    tag, *codes = dec.split()
    codes = [int(c, 16) for c in codes]
    kind = {"<isolated>": "iso", "<final>": "fin", "<initial>": "ini", "<medial>": "med"}.get(tag)
    if not kind:
        continue
    if len(codes) == 1 and codes[0] in BASE:
        forms.setdefault(codes[0], {}).setdefault(kind, cp)
    elif len(codes) == 2 and codes[0] == 0x644 and codes[1] in (0x622, 0x623, 0x625, 0x627):
        d = lamalef.setdefault(codes[1], {})
        d.setdefault(kind, cp)


def has_ink(cp):
    return f.getmask(chr(cp)).getbbox() is not None


# فقط شکل‌هایی که فونت دارد
for b in list(forms):
    forms[b] = {k: v for k, v in forms[b].items() if has_ink(v)}

glyph_cps = set(range(0x20, 0x7F)) | set(range(0x6F0, 0x6FA)) | set(range(0x660, 0x66A))
glyph_cps |= {0x60C, 0x61B, 0x61F, 0xAB, 0xBB, 0x2026, 0x640}
glyph_cps |= set(BASE)
for d in forms.values():
    glyph_cps |= set(d.values())
for d in lamalef.values():
    glyph_cps |= set(d.values())
glyph_cps = sorted(c for c in glyph_cps if c == 0x20 or has_ink(c))

PAD, BASEY, H = 8, 22, 34
imgs = {}
top, bot = 99, 0
for cp in glyph_cps:
    im = Image.new("L", (40, H), 0)
    d = ImageDraw.Draw(im)
    d.fontmode = "1"
    d.text((PAD, BASEY), chr(cp), font=f, fill=255, anchor="ls")
    im = im.point(lambda v: 255 if v > 110 else 0)
    imgs[cp] = im
    bb = im.getbbox()
    if bb:
        top = min(top, bb[1]); bot = max(bot, bb[3])
FH = bot - top

glyphs, pool = [], []
for cp in glyph_cps:
    im = imgs[cp].crop((0, top, 40, bot))
    bb = im.getbbox()
    adv = int(round(f.getlength(chr(cp))))
    if cp == 0x20:
        adv = max(adv, 4)
    if not bb:
        glyphs.append((cp, 0, 0, adv, len(pool))); continue
    x0, x1 = bb[0], bb[2]
    w = x1 - x0
    px = im.load()
    off = len(pool)
    for y in range(FH):
        for bx in range(0, w, 8):
            byte = 0
            for i in range(8):
                x = x0 + bx + i
                if x < x1 and px[x, y]:
                    byte |= 1 << i
            pool.append(byte)
    glyphs.append((cp, x0 - PAD, w, adv, off))

with open(OUT, "w", encoding="utf-8") as o:
    o.write("// ساخته‌شده خودکار با tools/make_font.py — دستی ویرایش نکن!\n#pragma once\n#include <Arduino.h>\n\n")
    o.write(f"#define PF_HEIGHT {FH}\n")
    o.write("struct PGlyph { uint16_t cp; int8_t xoff; uint8_t w, adv; uint16_t off; };\n")
    o.write("struct PShape { uint16_t base, iso, fin, ini, med; };\n\n")
    o.write("static const uint8_t PF_BITS[] PROGMEM = {")
    o.write(",".join(str(b) for b in pool) + "};\n")
    o.write("static const PGlyph PF_GLYPHS[] = {\n")
    for g in glyphs:
        o.write(f"  {{0x{g[0]:04X},{g[1]},{g[2]},{g[3]},{g[4]}}},\n")
    o.write("};\n#define PF_NGLYPH (sizeof(PF_GLYPHS)/sizeof(PF_GLYPHS[0]))\n\n")
    o.write("// شکل‌های چسبان: تنها ، آخر ، اول ، وسط  (۰ = ندارد)\nstatic const PShape PF_SHAPES[] = {\n")
    for b in sorted(forms):
        d = forms[b]
        o.write(f"  {{0x{b:04X},0x{d.get('iso', 0):04X},0x{d.get('fin', 0):04X},0x{d.get('ini', 0):04X},0x{d.get('med', 0):04X}}},\n")
    o.write("};\n#define PF_NSHAPE (sizeof(PF_SHAPES)/sizeof(PF_SHAPES[0]))\n\n")
    o.write("// لا : الف -> (تنها ، آخر)\nstatic const uint16_t PF_LAMALEF[][3] = {\n")
    for a in sorted(lamalef):
        d = lamalef[a]
        o.write(f"  {{0x{a:04X},0x{d.get('iso', 0):04X},0x{d.get('fin', 0):04X}}},\n")
    o.write("};\n")
print(f"ok -> {os.path.normpath(OUT)}  ({len(glyphs)} glyph ، ارتفاع {FH} ، {len(pool)} بایت)")
