# -*- coding: utf-8 -*-
"""
سازنده‌ی صدای موچی ۲
=====================
برای هر جمله‌ی موچی (همان‌هایی که در make_text.py هستند) یک فایل صدا می‌سازد ،
آن را برای بلندگوی کوچک آماده می‌کند (۱۰ کیلوهرتز ، ۸ بیت ، بلند و فشرده)
و در src/moochi_voice.h می‌گذارد.

منبع صدای هر جمله به این ترتیب انتخاب می‌شود:
  1) صدای ضبط‌شده‌ی خودت:  tools/voice/<کلید>.wav  یا  .mp3
     مثلا tools/voice/LOVE1.mp3  (کلیدها را پایین همین فایل یا در make_text.py ببین)
  2) موتور گفتار انتخاب‌شده:
       --engine edge    صدای طبیعی مایکروسافت (اینترنت لازم است)   ← پیشنهادی
       --engine espeak  صدای رباتی و آفلاین (اگر eSpeak NG نصب باشد)

نصب پیش‌نیازها (یک بار):
  & "$env:USERPROFILE\\.platformio\\penv\\Scripts\\python.exe" -m pip install miniaudio edge-tts

اجرا:
  & "$env:USERPROFILE\\.platformio\\penv\\Scripts\\python.exe" tools\\make_voice.py --engine edge
  گزینه‌ها:  --voice fa-IR-DilaraNeural (زن، پیش‌فرض) یا fa-IR-FaridNeural (مرد)
             --pitch +70Hz   (زیرتر = بامزه‌تر)     --rate -5%
             --shift 1.18    صدای دختربچه (۱ = صدای عادی ، ۱٫۳ = خیلی بچه‌گانه)
             --only LOVE1,HELLO   (فقط همین‌ها را دوباره بساز)
بعد Upload کن.
"""
import sys as _s, os as _o
_s.path.insert(0, _o.path.dirname(_o.path.abspath(__file__)))
from names import fill, out_path, PERSONAL, NAMES
import argparse, array, ast, asyncio, math, os, re, shutil, subprocess, sys, tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = out_path("moochi_voice")   # با tools/names.json ← moochi_voice_personal.h
CACHE = os.path.join(HERE, "voice_cache" if PERSONAL else "voice_cache_example")
OWN = os.path.join(HERE, "voice")
RATE = 8000

# متنی که «گفته» می‌شود (اگر با متن روی صفحه فرق دارد). بقیه از make_text.py خوانده می‌شوند.
SPEAK = {
    "OWNER": "یک ربات متعلق به {OWNER}!",
    "TCK1":  "هِه هِه هِه هِه!",
    "TCK3":  "هی هی هی!",
    "ANG4":  "اَه! خودتی!",
    "ANG8":  "می‌زنمت‌ها!",
    "ANG7":  "زلزله اومده؟! خسته نباشی!",
    "NOCLOCK": "ماژول ساعت پیدا نشد",
    "FROM":    "یه پیام از {FRIEND} داری!",
    "MISSED":  "کجا بودی؟! دلم برات تنگ شد!",
    "LONELY":  "دلم گرفته... نازم کن!",
    "ASK":     "سؤالت رو بپرس و تکونم بده!",
    "RHINT":   "هر وقت قلب دیدی، زود بزن!",
    "BDAY":    "تولدت مبارک {OWNER} جونم!",
}
SKIP = {"NAME", "MSEC", "LEVEL", "TO_BDAY", "TO_ANNIV", "WIFIAP"} | {f"WD{i}" for i in range(7)} | {f"M{i}" for i in range(1, 13)}


def load_phrases():
    src = open(os.path.join(HERE, "make_text.py"), encoding="utf-8").read()
    tree = ast.parse(src)
    for node in tree.body:
        if isinstance(node, ast.Assign) and getattr(node.targets[0], "id", "") == "PHRASES":
            return ast.literal_eval(node.value)
    sys.exit("PHRASES در make_text.py پیدا نشد")


# ---------------------------------------------------------------- موتورها
def tts_edge(text, path, voice, pitch, rate):
    import edge_tts
    async def go():
        await edge_tts.Communicate(text, voice, pitch=pitch, rate=rate).save(path)
    asyncio.run(go())


def find_espeak():
    for c in ["espeak-ng", "espeak", r"C:\Program Files\eSpeak NG\espeak-ng.exe"]:
        p = shutil.which(c) or (c if os.path.exists(c) else None)
        if p:
            return p
    return None


# مدل‌های صدای eSpeak (برای نمونه‌ها: voice-sample-*.wav)
ESPEAK_MODELS = {
    "A": ("fa+f4", "99", "145", 1.18),     # پسربچه‌ای (قبلی)
    "B": ("fa+girl1", "60", "135", 1.0),   # دختربچه ، آرام
    "C": ("fa+girl2", "70", "130", 1.0),   # دختربچه ، زیرتر (پیش‌فرض)
    "D": ("fa+girl1", "60", "150", 1.10),  # دختربچه ، تندتر
    "E": ("fa+Annie", "90", "140", 1.15),
}
ESPEAK_MODEL = "C"


def install_girl_voices(exe):
    """فایل‌های صدای دخترانه (tools/espeak_voices) را کنار صداهای eSpeak کپی می‌کند"""
    try:
        out = subprocess.run([exe, "--version"], capture_output=True, text=True).stdout
        m = re.search(r"Data at:\s*(.+)", out)
        if not m:
            return
        dst = os.path.join(m.group(1).strip(), "voices", "!v")
        for f in os.listdir(os.path.join(HERE, "espeak_voices")):
            t = os.path.join(dst, f)
            if not os.path.exists(t):
                shutil.copy(os.path.join(HERE, "espeak_voices", f), t)
    except Exception as ex:
        print("نصب صدای دخترانه‌ی eSpeak نشد:", ex)


def tts_espeak(text, path, exe):
    v, p, s, _ = ESPEAK_MODELS[ESPEAK_MODEL]
    subprocess.run([exe, "-v", v, "-p", p, "-s", s, "-a", "200", "-w", path, text],
                   check=True, capture_output=True)


# ---------------------------------------------------------------- پردازش صدا
def decode(path):
    import miniaudio
    d = miniaudio.decode_file(path, output_format=miniaudio.SampleFormat.SIGNED16,
                              nchannels=1, sample_rate=RATE)
    return [s / 32768.0 for s in d.samples]


def shift_up(x, f):
    """صدا را زیرتر می‌کند (مثل دختربچه): نمونه‌ها را با سرعت f می‌خواند"""
    if f <= 1.0:
        return x
    n = int((len(x) - 1) / f)
    out = []
    for i in range(n):
        p = i * f; j = int(p); a = p - j
        out.append(x[j] * (1 - a) + x[j + 1] * a)
    return out


def process(x, shift=1.0):
    if not x:
        return b""
    x = shift_up(x, shift)
    # حذف سکوت اول و آخر
    thr = 0.03 * max(abs(v) for v in x)
    i0 = next(i for i, v in enumerate(x) if abs(v) > thr)
    i1 = len(x) - next(i for i, v in enumerate(reversed(x)) if abs(v) > thr)
    pad = int(0.03 * RATE)
    x = x[max(0, i0 - pad): min(len(x), i1 + pad)]
    # حذف بم‌های خیلی پایین (بلندگوی کوچک پخش نمی‌کند و فقط خش می‌دهد)
    rc = 1.0 / (2 * math.pi * 300); dt = 1.0 / RATE; a = rc / (rc + dt)
    y, py, px = [], 0.0, 0.0
    for v in x:
        py = a * (py + v - px); px = v; y.append(py)
    # بلندتر کردن: نرمال‌سازی + فشرده‌سازی نرم
    peak = max(abs(v) for v in y) or 1.0
    drive = 2.2; k = math.tanh(drive)   # بیشتر = بلندتر (و کمی خش‌دارتر)
    y = [math.tanh(drive * v / peak) / k for v in y]
    out = bytes(max(0, min(255, int(round(128 + v * 127)))) for v in y)
    return out


# ---------------------------------------------------------------- فشرده‌سازی IMA-ADPCM (۴ بیت)
STEP = [7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60, 66,
        73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408,
        449, 494, 544, 598, 658, 724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
        2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630,
        9493, 10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767]
IDX = [-1, -1, -1, -1, 2, 4, 6, 8]


def adpcm(u8):
    pred, idx, out, nib = 0, 0, bytearray(), []
    for b in u8:
        x = (b - 128) * 256
        st = STEP[idx]; d = x - pred; code = 0
        if d < 0: code = 8; d = -d
        diff = st >> 3
        if d >= st: code |= 4; d -= st; diff += st
        if d >= st >> 1: code |= 2; d -= st >> 1; diff += st >> 1
        if d >= st >> 2: code |= 1; diff += st >> 2
        pred = pred - diff if code & 8 else pred + diff
        pred = max(-32768, min(32767, pred))
        idx = max(0, min(88, idx + IDX[code & 7]))
        nib.append(code)
    if len(nib) % 2: nib.append(0)
    for i in range(0, len(nib), 2):
        out.append(nib[i] | (nib[i + 1] << 4))
    return bytes(out)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--engine", choices=["edge", "espeak", "none"], default="edge")
    ap.add_argument("--voice", default="fa-IR-DilaraNeural")
    ap.add_argument("--pitch", default="+70Hz")
    ap.add_argument("--rate", default="-5%")
    ap.add_argument("--model", default="C", help="مدل صدای eSpeak: A تا E")
    ap.add_argument("--shift", type=float, default=None,
                    help="زیرتر کردن صدا (دختربچه). ۱ = بدون تغییر ، ۱٫۱۸ پیش‌فرض ، ۱٫۳ = خیلی بچه‌گانه")
    ap.add_argument("--only", default="")
    args = ap.parse_args()
    global ESPEAK_MODEL
    ESPEAK_MODEL = args.model.upper()
    if args.shift is None:
        args.shift = ESPEAK_MODELS[ESPEAK_MODEL][3] if args.engine == "espeak" else 1.15
    try:
        import miniaudio  # noqa
    except ImportError:
        sys.exit("اول نصب کن:  python -m pip install miniaudio edge-tts")

    phrases = load_phrases()
    keys = list(phrases.keys())
    only = set(filter(None, args.only.split(",")))
    os.makedirs(CACHE, exist_ok=True)
    esp = find_espeak() if args.engine == "espeak" else None
    if args.engine == "espeak" and not esp:
        sys.exit("eSpeak NG پیدا نشد. یا نصبش کن یا --engine edge بزن.")
    if esp:
        install_girl_voices(esp)

    clips = {}
    named = []
    tmp = tempfile.mkdtemp()
    for key in keys:
        if key in SKIP:
            continue
        raw = SPEAK.get(key, phrases[key].replace("|", " "))
        if "{OWNER}" in raw or "{FRIEND}" in raw:
            named.append(key)
        text = fill(raw)
        own = next((os.path.join(OWN, key + e) for e in (".wav", ".mp3", ".flac")
                    if os.path.exists(os.path.join(OWN, key + e))), None)
        cache = os.path.join(CACHE, key + ".raw")
        if own:
            src, how = own, "ضبط خودت"
        elif os.path.exists(cache) and only and key not in only:
            clips[key] = open(cache, "rb").read(); continue
        elif args.engine == "none":
            continue
        else:
            src = os.path.join(tmp, key + (".mp3" if args.engine == "edge" else ".wav"))
            try:
                if args.engine == "edge":
                    tts_edge(text, src, args.voice, args.pitch, args.rate)
                else:
                    tts_espeak(text, src, esp)
            except Exception as ex:
                print(f"  ✗ {key}: {ex}")
                if os.path.exists(cache):
                    clips[key] = open(cache, "rb").read()
                continue
            how = args.engine
        data = process(decode(src), 1.0 if own else args.shift)
        open(cache, "wb").write(data)
        clips[key] = data
        print(f"  ✓ {key:8s} {len(data) / RATE:4.1f}s  ({how})  {text}")

    total = sum(len(v) for v in clips.values())
    with open(OUT, "w", encoding="utf-8") as f:
        f.write("// ساخته‌شده خودکار با tools/make_voice.py — دستی ویرایش نکن!\n#pragma once\n")
        f.write("#include <Arduino.h>\n#include \"moochi_text_sel.h\"\n\n")
        f.write(f"#define VOICE_RATE {RATE}\n")
        f.write("struct VoiceClip { const uint8_t *d; uint32_t n; };\n\n")
        f.write("// هر بایت = دو نمونه‌ي ۴ بیتی IMA-ADPCM ؛ n = تعداد نمونه‌ها\n")
        for k, v in clips.items():
            v = adpcm(v)
            f.write(f"static const uint8_t v_{k}[] PROGMEM = {{")
            for i in range(0, len(v), 40):
                f.write(",".join(str(b) for b in v[i:i + 40]) + ",\n")
            f.write("};\n")
        f.write("\nstatic const VoiceClip VOICES[P_COUNT] = {\n")
        for k in keys:
            f.write(f"  {{v_{k}, {len(clips[k])}}},\n" if k in clips else "  {nullptr, 0},\n")
        f.write("};\n\n")
        f.write("// جمله‌هایی که صدایشان اسم دارد (اگر اسم عوض شود ، این صداها پخش نمی‌شوند)\n")
        f.write(f'#define VOICE_OWNER "{NAMES["owner"]}"\n#define VOICE_FRIEND "{NAMES["friend"]}"\n')
        f.write("static const uint8_t VOICE_NAMED[] = {" + ",".join("P_" + k for k in named if k in clips) + "};\n")
        f.write("#define VOICE_NAMED_N (sizeof(VOICE_NAMED))\n")
    print(f"ok -> {os.path.normpath(OUT)}  ({len(clips)} جمله ، {total / RATE:.0f} ثانیه ، {total // 2048} کیلوبایت)")


if __name__ == "__main__":
    main()
