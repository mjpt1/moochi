# -*- coding: utf-8 -*-
"""
سازنده‌ی متن‌های فارسی موچی ۲
================================
نمایشگر OLED فونت فارسی ندارد و حروف فارسی باید به هم بچسبند (شکل‌دهی)
و از راست به چپ نوشته شوند. پس همه‌ی جمله‌ها این‌جا روی کامپیوتر با فونت
وزیرمتن به تصویر سیاه‌سفید تبدیل می‌شوند و در src/moochi_text.h ذخیره می‌شوند.
اسم‌ها: {OWNER} و {FRIEND} از tools/names.json (یا names.example.json) پر می‌شوند.

برای عوض کردن یا اضافه کردن جمله:
  1) پایین همین فایل، در PHRASES / GROUPS جمله را عوض کن.
     علامت | یعنی «برو خط بعد» (حداکثر ۲ خط ، هر خط حداکثر ۱۲۸ پیکسل).
  2) اجرا کن:
       %USERPROFILE%\\.platformio\\penv\\Scripts\\python.exe tools\\make_text.py
     (یا هر پایتونی که Pillow دارد: pip install pillow)
     اگر حروف جدا جدا درآمدند:  pip install arabic-reshaper python-bidi
  3) دوباره Upload کن.
"""
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from names import fill, out_path
from PIL import Image, ImageDraw, ImageFont, features

HERE = os.path.dirname(os.path.abspath(__file__))
FONT_BOLD = os.path.join(HERE, "fonts", "Vazirmatn-Bold.ttf")
FONT_BLACK = os.path.join(HERE, "fonts", "Vazirmatn-Black.ttf")
OUT = out_path("moochi_text")   # با tools/names.json ← moochi_text_personal.h
MAXW = 128

# --------------------------------------------------------------------------
# جمله‌ها:  کلید -> متن
# --------------------------------------------------------------------------
PHRASES = {
    # شروع
    "HELLO":      "سلام!",
    "IAM":        "من موچی هستم",
    "OWNER":      "یک ربات متعلق به",
    "NAME":        "{OWNER}",              # با فونت درشت
    # خشم (وقتی شدید تکانش بدهی) — شوخی و بامزه
    "ANG1":       "هوی! تکونم نده!",
    "ANG2":       "دیوونه شدی؟!",
    "ANG3":       "بی‌تربیت!|ولم کن!",
    "ANG4":       "اَه! خودتی!",
    "ANG5":       "مغزم ریخت به هم!",
    "ANG6":       "به {OWNER} میگم‌ها!",
    "ANG7":       "زلزله اومده؟!|خسته نباشی!",
    "ANG8":       "میزنمتا!",
    "ANG9":       "بچه پررو!",
    # بعد از خشم
    "SULK":       "قهرم!",
    "SORRY":      "باشه... بخشیدمت",
    # غافلگیری (ضربه‌ی کوچک)
    "SUR1":       "وای!",
    "SUR2":       "چی شد؟!",
    "SUR3":       "ترسیدم!",
    # کج کردن
    "TILT":       "کجم کردی!",
    # قلقلک (تاچ چپ/راست)
    "TCK1":       "هههه!",
    "TCK2":       "قلقلکم نده!",
    "TCK3":       "هی هی هی!",
    # نوازش
    "PET1":       "آخیش... چه کیفی!",
    "PET2":       "بازم ناز کن",
    # بغل (چپ و راست با هم)
    "HUG":        "بغلم کن!",
    # زیاد دست زدن
    "ANNOY1":     "بسه دیگه!",
    "ANNOY2":     "اذیت نکن!",
    # عاشقانه
    "LOVE1":      "دوستت دارم {OWNER}",
    "LOVE2":      "دلم برات تنگ شده",
    "LOVE3":      "{OWNER} جون سلام!",
    "LOVE4":      "تو بهترینی!",
    "LOVE5":      "مواظب خودت باش",
    # زمان
    "MORNING":    "صبح بخیر {OWNER}!",
    "NIGHT":      "شب بخیر...",
    "SLEEPY":     "خوابم میاد...",
    "WAKE":       "بیدار شدم!",
    "NOCLOCK":    "ماژول ساعت|پیدا نشد",
    # پیام از راه دور
    "FROM":       "پیام از {FRIEND}",
    "THINK":      "{FRIEND} به یادته!",
    "NEWMSG":     "یه پیام داری!",
    # حیوون خونگی
    "MISSED":     "کجا بودی؟!|دلم برات تنگ شد",
    "LONELY":     "دلم گرفته...|نازم کن!",
    # زنگ و تایمر
    "ALARM":      "پاشو {OWNER}!",
    "SNOOZE":     "پنج دقیقه دیگه...",
    "TDONE":      "تموم شد!",
    # توپ جادویی و تاس
    "ASK":        "سؤالت رو بپرس|و تکونم بده",
    "SHAKEME":    "تکونم بده!",
    "AN1": "آره حتماً!", "AN2": "نه اصلاً!", "AN3": "شاید...", "AN4": "فردا دوباره بپرس",
    "AN5": "صد در صد!", "AN6": "بهش فکر نکن!", "AN7": "از دلت بپرس", "AN8": "معلومه که آره!",
    "AN9": "فعلاً نه", "AN10": "قول نمیدم!",
    # بازی‌ها
    "RHINT":      "قلب دیدی|زود بزن!",
    "EARLY":      "زود زدی!",
    "RECORD":     "رکورد جدید!",
    "MSEC":       "میلی‌ثانیه",
    "SHINT":      "تکرار کن!",
    "LOSE":       "باختی!",
    "LEVEL":      "امتیاز",
    "WATCH":      "خوب نگاه کن...",
    # منو
    "MN_8BALL":   "توپ جادویی",
    "MN_DICE":    "تاس",
    "MN_REACT":   "بازی سرعت",
    "MN_SIMON":   "بازی حافظه",
    "MN_TIMER":   "تایمر",
    "MN_NIGHT":   "چراغ خواب",
    "MN_LOVE":    "عشق‌سنج",
    "MN_FAL":     "فال حافظ",
    "MN_MUSIC":   "جعبه موسیقی",
    "MN_REPLY":   "جواب به {FRIEND}",
    "MN_EXIT":    "خروج",
    # روزهای خاص
    "BDAY":       "تولدت مبارک|{OWNER} جونم!",
    "ANNIV":      "سالگردمون مبارک!",
    "NOWRUZ":     "نوروز مبارک!",
    "YALDA":      "شب یلدا مبارک!",
    "LOVEDAY":    "روز عشق مبارک!",
    "TO_BDAY":    "روز تا تولد {OWNER}",
    "TO_ANNIV":   "روز تا سالگرد",
    "WIFIAP":     "وای‌فای پیدا نشد|به Moochi وصل شو",
    # روزهای هفته (۰ = شنبه)
    "WD0": "شنبه", "WD1": "یکشنبه", "WD2": "دوشنبه", "WD3": "سه‌شنبه",
    "WD4": "چهارشنبه", "WD5": "پنجشنبه", "WD6": "جمعه",
    # ماه‌های شمسی
    "M1": "فروردین", "M2": "اردیبهشت", "M3": "خرداد", "M4": "تیر",
    "M5": "مرداد", "M6": "شهریور", "M7": "مهر", "M8": "آبان",
    "M9": "آذر", "M10": "دی", "M11": "بهمن", "M12": "اسفند",
}
BIG = {"NAME"}          # این‌ها درشت نوشته می‌شوند

# گروه‌ها: موچی از هر گروه یکی را تصادفی انتخاب می‌کند
GROUPS = {
    "ANGRY":    ["ANG1", "ANG2", "ANG3", "ANG4", "ANG5", "ANG6", "ANG7", "ANG8", "ANG9"],
    "SURPRISE": ["SUR1", "SUR2", "SUR3"],
    "TICKLE":   ["TCK1", "TCK2", "TCK3"],
    "PET":      ["PET1", "PET2"],
    "ANNOY":    ["ANNOY1", "ANNOY2"],
    "LOVE":     ["LOVE1", "LOVE2", "LOVE3", "LOVE4", "LOVE5"],
    "ANSWER":   ["AN1", "AN2", "AN3", "AN4", "AN5", "AN6", "AN7", "AN8", "AN9", "AN10"],
    "MENU":     ["MN_8BALL", "MN_DICE", "MN_REACT", "MN_SIMON", "MN_TIMER", "MN_NIGHT", "MN_LOVE", "MN_FAL", "MN_MUSIC", "MN_REPLY", "MN_EXIT"],
    "WEEKDAY":  ["WD0", "WD1", "WD2", "WD3", "WD4", "WD5", "WD6"],
    "MONTH":    ["M1", "M2", "M3", "M4", "M5", "M6", "M7", "M8", "M9", "M10", "M11", "M12"],
}

SMALL_SIZE = 15
BIG_SIZE = 26
DIGIT_BIG_SIZE = 46
DIGIT_SMALL_SIZE = 15

# --------------------------------------------------------------------------
HAVE_RAQM = features.check("raqm")
if not HAVE_RAQM:
    try:
        import arabic_reshaper
        from bidi.algorithm import get_display
    except ImportError:
        sys.exit("خطا: Pillow بدون raqm است. اجرا کن:  pip install arabic-reshaper python-bidi")


def render(text, font):
    if HAVE_RAQM:
        kw = dict(direction="rtl", language="fa")
    else:
        text = get_display(arabic_reshaper.reshape(text))
        kw = {}
    asc, desc = font.getmetrics()
    W = 400
    img = Image.new("L", (W, asc + desc + 8), 0)
    d = ImageDraw.Draw(img)
    d.fontmode = "1"                      # بدون نرم‌کردن لبه = تیز روی OLED
    d.text((4, 2), text, font=font, fill=255, **kw)
    return img


def crop(img, fixed_rows=None):
    bw = img.point(lambda v: 255 if v > 110 else 0)
    box = bw.getbbox()
    if not box:
        return Image.new("1", (1, 1))
    x0, y0, x1, y1 = box
    if fixed_rows:
        y0, y1 = fixed_rows
    return bw.crop((x0, y0, x1, y1)).convert("1")


def rows_of(font, sample):
    """ارتفاع ثابت برای یک اندازه‌ی فونت (تا خط‌ها هم‌تراز باشند)"""
    img = render(sample, font).point(lambda v: 255 if v > 110 else 0)
    b = img.getbbox()
    return (b[1], b[3])


def to_xbm(im):
    w, h = im.size
    px = im.load()
    out = []
    for y in range(h):
        for bx in range(0, w, 8):
            byte = 0
            for i in range(8):
                x = bx + i
                if x < w and px[x, y]:
                    byte |= 1 << i
            out.append(byte)
    return out


def main():
    fs = ImageFont.truetype(FONT_BOLD, SMALL_SIZE)
    fb = ImageFont.truetype(FONT_BLACK, BIG_SIZE)
    fdb = ImageFont.truetype(FONT_BLACK, DIGIT_BIG_SIZE)
    fds = ImageFont.truetype(FONT_BOLD, DIGIT_SMALL_SIZE)

    # یک نمونه با بلندترین/پایین‌ترین حروف برای ارتفاع ثابت
    sample = "آگچپژلکیپ‌گ،؟!"
    rs = rows_of(fs, sample)
    rb = rows_of(fb, sample)

    lines_code, phrase_code = [], []
    names = list(PHRASES.keys())
    for key in names:
        parts = fill(PHRASES[key]).split("|")
        font, rows = (fb, rb) if key in BIG else (fs, rs)
        refs = []
        for li, part in enumerate(parts):
            im = crop(render(part, font), rows)
            if im.size[0] > MAXW:
                print(f"هشدار: «{part}» {im.size[0]} پیکسل است (بیشتر از ۱۲۸) — کوتاهش کن یا با | بشکن")
            nm = f"t_{key}_{li}"
            data = to_xbm(im)
            lines_code.append(
                f"static const uint8_t {nm}_bits[] PROGMEM = {{{','.join(str(b) for b in data)}}};")
            refs.append(f"{{{im.size[0]},{im.size[1]},{nm}_bits}}")
        lines_code.append(f"static const TxtLine t_{key}_lines[] = {{{','.join(refs)}}};")
        phrase_code.append(f"  {{{len(parts)}, t_{key}_lines}},   // {fill(PHRASES[key]).replace('|', ' / ')}")

    # رقم‌های فارسی درشت (ساعت) و ریز (تاریخ)
    digits = "۰۱۲۳۴۵۶۷۸۹"
    def digit_set(font, prefix):
        rws = rows_of(font, digits)
        code, refs = [], []
        for i, ch in enumerate(digits):
            im = crop(render(ch, font), rws)
            nm = f"{prefix}{i}"
            code.append(f"static const uint8_t {nm}_bits[] PROGMEM = {{{','.join(str(b) for b in to_xbm(im))}}};")
            refs.append(f"{{{im.size[0]},{im.size[1]},{nm}_bits}}")
        code.append(f"static const TxtLine {prefix}[10] = {{{','.join(refs)}}};")
        return code
    dig_code = digit_set(fdb, "DIG_BIG") + digit_set(fds, "DIG_SMALL")

    enum = ",\n".join(f"  P_{k}" for k in names)
    groups = []
    for g, keys in GROUPS.items():
        groups.append(f"static const uint8_t G_{g}[] = {{{','.join('P_' + k for k in keys)}}};")
        groups.append(f"#define G_{g}_N {len(keys)}")

    with open(OUT, "w", encoding="utf-8") as f:
        f.write("// ساخته‌شده خودکار با tools/make_text.py — دستی ویرایش نکن!\n")
        f.write("#pragma once\n#include <Arduino.h>\n\n")
        f.write("struct TxtLine { uint8_t w, h; const uint8_t *bits; };\n")
        f.write("struct Phrase { uint8_t n; const TxtLine *lines; };\n\n")
        f.write(f"enum PhraseId : uint8_t {{\n{enum},\n  P_COUNT\n}};\n\n")
        f.write("\n".join(lines_code) + "\n\n")
        f.write("static const Phrase PHRASES[] = {\n" + "\n".join(phrase_code) + "\n};\n\n")
        f.write("\n".join(groups) + "\n\n")
        f.write("\n".join(dig_code) + "\n")
    print("ok ->", os.path.normpath(OUT), f"({len(names)} جمله، raqm={HAVE_RAQM})")


if __name__ == "__main__":
    main()
