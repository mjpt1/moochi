// ===========================================================================
//  موچی ۲ — کشیدن چشم‌ها ، متن ، ساعت روی OLED ۱۲۸×۶۴
//  (فقط نقاشی؛ منطق ربات در main.cpp است)
// ===========================================================================
#pragma once
#include <math.h>
#include "moochi_text_sel.h"

// جمله‌هایی که اسم دارند و اسم از صفحه‌ی وب عوض شده: نسخه‌ی تازه در RAM (main.cpp → rebuildNamed)
static const Phrase *phOverride[P_COUNT];
static inline const Phrase &PH(int p) { return phOverride[p] ? *phOverride[p] : PHRASES[p]; }

extern U8G2 &oled;

#ifndef EYE_SIZE
#define EYE_SIZE 1.35f
#endif
#ifndef EYE_GAP
#define EYE_GAP  30
#endif

// ---------------- حالت‌ها (Mood) ----------------
enum Mood : uint8_t {
  M_NEUTRAL, M_HAPPY, M_LAUGH, M_LOVE, M_SURPRISED, M_ANGRY, M_SAD,
  M_SLEEPY, M_SLEEP, M_ANNOYED, M_CURIOUS, M_DIZZY, M_WINK, M_SULK,
  M_COUNT
};
static const char *MOOD_NAMES[M_COUNT] = {
  "neutral", "happy", "laugh", "love", "surprised", "angry", "sad",
  "sleepy", "sleep", "annoyed", "curious", "dizzy", "wink", "sulk"
};

// پارامترهای یک جفت چشم — بین حالت‌ها نرم تغییر می‌کنند
struct EyeP {
  float w, h, r;      // اندازه و گردی گوشه
  float lidTop;       // ۰..۱ پلک بالا چقدر پایین آمده
  float lidAng;       // + اخم (عصبانی) ، - غمگین
  float happy;        // ۰..۱ چشم هلالی ^ ^
  float ls, rs;       // بزرگی چشم چپ و راست (کنجکاو)
  float lookX, lookY; // نگاه
};

static EyeP moodTarget(Mood m) {
  //            w   h   r   lid  ang   hap  ls   rs   lx ly
  switch (m) {
    case M_HAPPY:     return {32, 34, 10, 0.0f, 0.0f, 1.0f, 1, 1, 0, -2};
    case M_LAUGH:     return {32, 34, 10, 0.0f, 0.0f, 1.0f, 1, 1, 0, -2};
    case M_LOVE:      return {30, 36,  8, 0.0f, 0.0f, 0.0f, 1, 1, 0, 0};
    case M_SURPRISED: return {34, 42, 16, 0.0f, 0.0f, 0.0f, 1, 1, 0, -2};
    case M_ANGRY:     return {32, 30,  6, 0.12f, 0.50f, 0.0f, 1, 1, 0, 2};
    case M_SAD:       return {28, 32,  9, 0.08f, -0.45f, 0.0f, 1, 1, 0, 4};
    case M_SLEEPY:    return {30, 36,  8, 0.58f, -0.1f, 0.0f, 1, 1, 0, 3};
    case M_SLEEP:     return {30,  3,  1, 0.0f, 0.0f, 0.0f, 1, 1, 0, 6};
    case M_ANNOYED:   return {32, 30,  6, 0.50f, 0.0f, 0.0f, 1, 1, 7, 0};
    case M_SULK:      return {30, 30,  6, 0.42f, -0.2f, 0.0f, 1, 1, -7, 4};
    case M_CURIOUS:   return {30, 36,  9, 0.0f, 0.0f, 0.0f, 0.78f, 1.18f, 6, -3};
    case M_DIZZY:     return {30, 36,  8, 0.0f, 0.0f, 0.0f, 1, 1, 0, 0};
    case M_WINK:      return {30, 36,  8, 0.0f, 0.0f, 0.0f, 1, 1, 0, 0};
    default:          return {30, 36,  8, 0.0f, 0.0f, 0.0f, 1, 1, 0, 0};
  }
}

static inline float lerpf(float a, float b, float t) { return a + (b - a) * t; }
static void eyeLerp(EyeP &c, const EyeP &t, float k) {
  c.w = lerpf(c.w, t.w, k);   c.h = lerpf(c.h, t.h, k);   c.r = lerpf(c.r, t.r, k);
  c.lidTop = lerpf(c.lidTop, t.lidTop, k); c.lidAng = lerpf(c.lidAng, t.lidAng, k);
  c.happy = lerpf(c.happy, t.happy, k);
  c.ls = lerpf(c.ls, t.ls, k); c.rs = lerpf(c.rs, t.rs, k);
  c.lookX = lerpf(c.lookX, t.lookX, k); c.lookY = lerpf(c.lookY, t.lookY, k);
}

// ---------------- شکل‌های کمکی ----------------
static void fillTri(float x0, float y0, float x1, float y1, float x2, float y2) {
  oled.drawTriangle((int)lroundf(x0), (int)lroundf(y0), (int)lroundf(x1), (int)lroundf(y1),
                    (int)lroundf(x2), (int)lroundf(y2));
}

static void drawHeart(int cx, int cy, float s) {     // s = پهنای قلب
  if (s < 3) return;
  float r = s * 0.27f;
  oled.drawDisc(cx - r * 0.95f, cy - r * 0.35f, r);
  oled.drawDisc(cx + r * 0.95f, cy - r * 0.35f, r);
  fillTri(cx - s * 0.5f, cy - r * 0.1f, cx + s * 0.5f, cy - r * 0.1f, cx, cy + s * 0.5f);
}

static void drawSpiral(int cx, int cy, float size, float rot) {
  float px = cx, py = cy;
  for (float t = 0.3f; t < 15.0f; t += 0.25f) {
    float rr = t / 15.0f * size;
    float x = cx + rr * cosf(t + rot), y = cy + rr * sinf(t + rot);
    oled.drawLine(px, py, x, y);
    oled.drawLine(px + 1, py, x + 1, y);
    px = x; py = y;
  }
}

static void drawTxt(const TxtLine &t, int x, int y) {
  oled.drawXBMP(x, y, t.w, t.h, t.bits);
}
static int phraseHeight(PhraseId p) {
  const Phrase &ph = PH(p); int h = 0;
  for (int i = 0; i < ph.n; i++) h += ph.lines[i].h + (i ? 1 : 0);
  return h;
}
static void drawPhraseAt(PhraseId p, int yTop) {          // وسط‌چین
  const Phrase &ph = PH(p); int y = yTop;
  for (int i = 0; i < ph.n; i++) {
    const TxtLine &l = ph.lines[i];
    drawTxt(l, (128 - l.w) / 2, y);
    y += l.h + 1;
  }
}

// ---------------- یک چشم ----------------
// side: -1 چشم چپ صفحه ، +1 چشم راست صفحه
static void drawEye(float cx, float cy, float w, float h, float r, const EyeP &e, int side) {
  if (h < 1) h = 1;
  if (w < 2) w = 2;
  float x0 = cx - w / 2, y0 = cy - h / 2;
  float rr = fminf(r, fminf(w, h) / 2 - 1); if (rr < 0) rr = 0;
  oled.setDrawColor(1);
  if (h <= 4) {                                  // چشم بسته = خط
    oled.drawRBox(x0, cy - 1, w, 3, 1);
    return;
  }
  if (e.happy > 0.5f) {                         // چشم خندان ^ ^ (هلال)
    float ry = h * 0.40f, th = fmaxf(3.0f, h * 0.24f);
    float ecy = cy + h * 0.12f;
    oled.drawFilledEllipse(cx, ecy, w / 2, ry);
    oled.setDrawColor(0);
    oled.drawFilledEllipse(cx, ecy + th, w / 2 + 1, ry);
    oled.drawBox(x0 - 2, ecy + 1, w + 4, h);
    oled.setDrawColor(1);
    return;
  }
  oled.drawRBox(x0, y0, w, h, rr);

  oled.setDrawColor(0);
  // پلک بالا (صاف / اخم / غم)
  if (e.lidTop > 0.02f || fabsf(e.lidAng) > 0.02f) {
    float base = y0 + e.lidTop * h;
    float inner = base + fmaxf(e.lidAng, 0) * h * 0.6f;   // اخم: گوشه‌ي داخلی پایین
    float outer = base + fmaxf(-e.lidAng, 0) * h * 0.6f;  // غم: گوشه‌ي بیرونی پایین
    float yl = (side < 0) ? outer : inner;
    float yr = (side < 0) ? inner : outer;
    float L = x0 - 2, R = x0 + w + 2, T = y0 - 2;
    fillTri(L, T, R, T, R, yr);
    fillTri(L, T, L, yl, R, yr);
  }
  oled.setDrawColor(1);
}

// ---------------- کل صورت ----------------
// scale: ۱ = تمام صفحه ؛ کوچک‌تر برای وقتی که زیرش متن هست
// t = زمان (میلی‌ثانیه) برای انیمیشن‌ها ؛ blink = ۰..۱
static void drawFace(Mood mood, const EyeP &e, float blink, float cy, float scale, uint32_t t) {
  float gap = EYE_GAP * scale;
  float sz = scale * EYE_SIZE;                   // اندازه‌ی چشم‌ها
  float jx = 0, jy = 0;
  if (mood == M_ANGRY) { jx = (int)((t / 40) % 3) - 1; jy = (int)((t / 55) % 3) - 1; }
  if (mood == M_LAUGH) { jy = (sinf(t * 0.025f) > 0 ? -2 : 1) * scale; }
  float lx = e.lookX * scale + jx, ly = e.lookY * scale + jy;
  float cxL = 64 - gap - 0 + lx, cxR = 64 + gap + lx, cyy = cy + ly;

  if (mood == M_LOVE) {                          // چشم قلبی
    float beat = 1.0f + 0.12f * fmaxf(0, sinf(t * 0.012f));
    drawHeart(cxL, cyy, 34 * sz * beat);
    drawHeart(cxR, cyy, 34 * sz * beat);
    return;
  }
  if (mood == M_DIZZY) {                         // چشم گیج
    drawSpiral(cxL, cyy, 15 * sz, t * 0.01f);
    drawSpiral(cxR, cyy, 15 * sz, t * 0.01f);
    return;
  }
  float hb = 1.0f - blink * 0.94f;
  float wL = e.w * e.ls * sz, hL = e.h * e.ls * sz * hb;
  float wR = e.w * e.rs * sz, hR = e.h * e.rs * sz * hb;
  drawEye(cxL, cyy, wL, hL, e.r * sz, e, -1);

  if (mood == M_WINK) {                          // چشمک: چشم راست ^
    float s = 13 * sz;
    for (int k = 0; k < 3; k++) {
      oled.drawLine(cxR - s, cyy + 4 + k, cxR, cyy - 6 + k);
      oled.drawLine(cxR, cyy - 6 + k, cxR + s, cyy + 4 + k);
    }
  } else {
    drawEye(cxR, cyy, wR, hR, e.r * sz, e, +1);
  }

  if (mood == M_SAD) {                           // اشک
    float ph = fmodf(t / 1400.0f, 1.0f);
    float tx = cxL - wL * 0.25f, ty = cyy + hL / 2 + 2 + ph * 14 * scale;
    if (ty < 62) {
      oled.drawDisc(tx, ty + 2, 2 * scale + 1);
      fillTri(tx - 2 * scale, ty + 1, tx + 2 * scale, ty + 1, tx, ty - 4 * scale);
    }
  }
  if (mood == M_SLEEP && scale > 0.8f) {         // zZz
    oled.setFont(u8g2_font_6x10_tr);
    for (int k = 0; k < 3; k++) {
      float ph = fmodf(t / 2600.0f + k / 3.0f, 1.0f);
      int zx = 102 + k * 6 + (int)(sinf(ph * 6.28f) * 2), zy = 30 - (int)(ph * 26);
      if (ph < 0.92f) oled.drawStr(zx, zy, (k == 1) ? "Z" : "z");
    }
  }
  if (mood == M_ANGRY && scale > 0.8f) {         // علامت عصبانیت گوشه‌ي بالا
    int ax = 112, ay = 9;
    for (int k = 0; k < 2; k++) {
      oled.drawLine(ax - 6, ay - 2 + k, ax - 2, ay - 2 + k); oled.drawLine(ax - 2 + k, ay - 6, ax - 2 + k, ay - 2);
      oled.drawLine(ax + 2, ay - 2 + k, ax + 6, ay - 2 + k); oled.drawLine(ax + 2 - k, ay - 6, ax + 2 - k, ay - 2);
      oled.drawLine(ax - 6, ay + 2 - k, ax - 2, ay + 2 - k); oled.drawLine(ax - 2 + k, ay + 2, ax - 2 + k, ay + 6);
      oled.drawLine(ax + 2, ay + 2 - k, ax + 6, ay + 2 - k); oled.drawLine(ax + 2 - k, ay + 2, ax + 2 - k, ay + 6);
    }
  }
}

// صورت + جمله زیرش (جمله می‌تواند P_COUNT = بدون جمله باشد)
static void drawFaceWithText(Mood mood, const EyeP &e, float blink, uint32_t t, PhraseId p) {
  if (p >= P_COUNT) { drawFace(mood, e, blink, 32, 1.0f, t); return; }
  int th = phraseHeight(p);
  int avail = 64 - th - 2;
  float scale = fminf(0.78f, (avail - 2) / (36.0f * EYE_SIZE));
  drawFace(mood, e, blink, avail / 2.0f + 1, scale, t);
  drawPhraseAt(p, 64 - th);
}

// ---------------- ساعت ----------------
static int numWidth(const TxtLine *dig, int v, int digits, int sp) {
  int w = 0;
  for (int i = 0; i < digits; i++) { w += dig[v % 10].w + (i ? sp : 0); v /= 10; }
  return w;
}
// عدد را از چپ به راست می‌نویسد (اعداد فارسی هم چپ‌به‌راست خوانده می‌شوند)
static int drawNum(const TxtLine *dig, int v, int digits, int x, int y, int sp) {
  int d[4]; for (int i = digits - 1; i >= 0; i--) { d[i] = v % 10; v /= 10; }
  for (int i = 0; i < digits; i++) { drawTxt(dig[d[i]], x, y); x += dig[d[i]].w + sp; }
  return x;
}

// ساعت درشت بالای صفحه + نوار ثانیه
static void drawClockTime(int hh, int mm, int ss, int y = 0, bool bar = true) {
  const int sp = 3;
  int wH = numWidth(DIG_BIG, hh, 2, sp), wM = numWidth(DIG_BIG, mm, 2, sp);
  int colonW = 10;
  int x = (128 - (wH + colonW + wM)) / 2;
  x = drawNum(DIG_BIG, hh, 2, x, y, sp);
  if (ss % 2 == 0) {                              // دو نقطه‌ي چشمک‌زن
    oled.drawBox(x + 3, y + 10, 4, 4);
    oled.drawBox(x + 3, y + 22, 4, 4);
  }
  x += colonW;
  drawNum(DIG_BIG, mm, 2, x, y, sp);
  if (bar) {
    int bw = (ss * 100) / 59;
    oled.drawHLine(14, y + 38, 100);
    oled.drawBox(14, y + 37, bw, 3);
  }
}
// تاریخ: از راست به چپ  «شنبه ۱۸ مهر» ؛ y = بالای خط
static void drawDateLine(int jd, int jm, int wd, int y) {
  const TxtLine &W = PH(P_WD0 + wd).lines[0];
  const TxtLine &M = PH(P_M1 + (jm - 1)).lines[0];
  int dd = jd >= 10 ? 2 : 1;
  int wN = numWidth(DIG_SMALL, jd, dd, 1);
  int gap = 5;
  int all = W.w + gap + wN + gap + M.w;
  int xr = 64 + all / 2;                          // لبه‌ي راست
  drawTxt(W, xr - W.w, y);            xr -= W.w + gap;
  drawNum(DIG_SMALL, jd, dd, xr - wN, y + 1, 1); xr -= wN + gap;
  drawTxt(M, xr - M.w, y);
}

// ---------------- تبدیل تاریخ میلادی به شمسی ----------------
static void g2j(int gy, int gm, int gd, int &jy, int &jm, int &jd) {
  static const int gdm[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
  long gy2 = (gm > 2) ? (gy + 1) : gy;
  long days = 355666 + (365L * gy) + ((gy2 + 3) / 4) - ((gy2 + 99) / 100) + ((gy2 + 399) / 400) + gd + gdm[gm - 1];
  long y = -1595 + (33 * (days / 12053));
  days %= 12053;
  y += 4 * (days / 1461);
  days %= 1461;
  if (days > 365) { y += (days - 1) / 365; days = (days - 1) % 365; }
  int m, d;
  if (days < 186) { m = 1 + days / 31; d = 1 + (days % 31); }
  else { m = 7 + (days - 186) / 30; d = 1 + ((days - 186) % 30); }
  jy = y; jm = m; jd = d;
}
// روز هفته به ترتیب ایرانی: ۰ = شنبه ... ۶ = جمعه
static int persianWeekday(int y, int m, int d) {
  static const int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
  if (m < 3) y -= 1;
  int dow = (y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7;   // ۰ = یکشنبه
  return (dow + 1) % 7;
}
