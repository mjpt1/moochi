// ===========================================================================
//  موچی ۲ — شکل‌های اضافه: پاکت نامه ، آب‌وهوا ، تاس ، کیک ، منو و ...
// ===========================================================================
#pragma once
#include "face.h"

static void drawTxtRight(const TxtLine &t, int xr, int y) { drawTxt(t, xr - t.w, y); }
static void drawTxtCenter(const TxtLine &t, int y) { drawTxt(t, (128 - t.w) / 2, y); }
static const TxtLine &line0(PhraseId p) { return PH(p).lines[0]; }

// عدد با رقم‌های ریز فارسی ؛ خروجی = عرض
static int smallNumWidth(int v) {
  if (v < 0) v = -v;
  int d = 1; for (int t = v; t >= 10; t /= 10) d++;
  return numWidth(DIG_SMALL, v, d, 1);
}
static int drawSmallNum(int v, int x, int y) {        // x = لبه‌ي چپ
  bool neg = v < 0; if (neg) { v = -v; oled.drawBox(x, y + 9, 5, 2); x += 7; }
  int d = 1; for (int t = v; t >= 10; t /= 10) d++;
  return drawNum(DIG_SMALL, v, d, x, y, 1);
}

// ---------------- پاکت نامه (پیام خوانده‌نشده) ----------------
static void drawEnvelope(int x, int y, int w = 16, int h = 11) {
  oled.setDrawColor(0); oled.drawBox(x - 1, y - 1, w + 2, h + 2); oled.setDrawColor(1);
  oled.drawFrame(x, y, w, h);
  oled.drawLine(x, y, x + w / 2, y + h / 2 + 1);
  oled.drawLine(x + w - 1, y, x + w / 2 - 1, y + h / 2 + 1);
}

// ---------------- آب‌وهوا ----------------
enum Wx : uint8_t { WX_NONE, WX_SUN, WX_PARTLY, WX_CLOUD, WX_FOG, WX_RAIN, WX_SNOW, WX_STORM };
static Wx wmoToWx(int c) {
  if (c < 0) return WX_NONE;
  if (c == 0) return WX_SUN;
  if (c <= 2) return WX_PARTLY;
  if (c == 3) return WX_CLOUD;
  if (c == 45 || c == 48) return WX_FOG;
  if ((c >= 71 && c <= 77) || c == 85 || c == 86) return WX_SNOW;
  if (c >= 95) return WX_STORM;
  return WX_RAIN;
}
static void drawCloud(int x, int y, int s) {          // x,y = گوشه‌ي چپ پایین
  oled.drawDisc(x + s * 0.30f, y - s * 0.28f, s * 0.26f);
  oled.drawDisc(x + s * 0.58f, y - s * 0.42f, s * 0.34f);
  oled.drawDisc(x + s * 0.82f, y - s * 0.25f, s * 0.22f);
  oled.drawBox(x + s * 0.28f, y - s * 0.30f, s * 0.56f, s * 0.30f + 1);
}
static void drawSun(int cx, int cy, int r, uint32_t t) {
  oled.drawDisc(cx, cy, r);
  float a0 = t * 0.0015f;
  for (int i = 0; i < 8; i++) {
    float a = a0 + i * 0.785f;
    oled.drawLine(cx + cosf(a) * (r + 2), cy + sinf(a) * (r + 2), cx + cosf(a) * (r + 5), cy + sinf(a) * (r + 5));
  }
}
static void drawMoon(int cx, int cy, int r) {
  oled.drawDisc(cx, cy, r);
  oled.setDrawColor(0); oled.drawDisc(cx + r * 0.55f, cy - r * 0.35f, r); oled.setDrawColor(1);
}
// آیکن کوچک ~۲۰ پیکسلی ؛ cx,cy = مرکز
static void drawWxIcon(Wx w, int cx, int cy, bool day, uint32_t t) {
  switch (w) {
    case WX_SUN:    if (day) drawSun(cx, cy, 5, t); else drawMoon(cx, cy, 7); break;
    case WX_PARTLY: if (day) drawSun(cx + 4, cy - 4, 4, t); else drawMoon(cx + 4, cy - 4, 5);
                    oled.setDrawColor(0); drawCloud(cx - 11, cy + 8, 20); oled.setDrawColor(1);
                    drawCloud(cx - 10, cy + 7, 18); break;
    case WX_CLOUD:  drawCloud(cx - 10, cy + 5, 20); break;
    case WX_FOG:    for (int i = 0; i < 4; i++) oled.drawHLine(cx - 9 + (i & 1) * 2, cy - 6 + i * 4, 16); break;
    case WX_RAIN:   drawCloud(cx - 10, cy + 2, 20);
                    for (int i = 0; i < 4; i++) { int ph = (t / 90 + i * 3) % 7; oled.drawLine(cx - 7 + i * 5, cy + 4 + ph, cx - 8 + i * 5, cy + 6 + ph); }
                    break;
    case WX_SNOW:   drawCloud(cx - 10, cy + 2, 20);
                    for (int i = 0; i < 4; i++) { int ph = (t / 160 + i * 2) % 6; oled.drawPixel(cx - 7 + i * 5, cy + 5 + ph); oled.drawPixel(cx - 6 + i * 5, cy + 5 + ph); }
                    break;
    case WX_STORM:  drawCloud(cx - 10, cy + 2, 20);
                    oled.drawLine(cx, cy + 3, cx - 3, cy + 8); oled.drawLine(cx - 3, cy + 8, cx + 1, cy + 8); oled.drawLine(cx + 1, cy + 8, cx - 2, cy + 13);
                    break;
    default: break;
  }
}
// «۱۸°» + آیکن ، وسط‌چین در یک خط
static void drawWeatherLine(int temp, Wx w, bool day, int y, uint32_t t) {
  int nw = smallNumWidth(temp) + (temp < 0 ? 7 : 0);
  int total = 22 + 6 + nw + 6;
  int x = (128 - total) / 2;
  drawWxIcon(w, x + 11, y + 9, day, t);
  x += 28;
  x = drawSmallNum(temp, x, y + 1);
  oled.drawCircle(x + 2, y + 4, 2);
}

// ---------------- تاس ----------------
static void drawDie(int face, int cx, int cy, int s) {
  oled.drawRFrame(cx - s / 2, cy - s / 2, s, s, 5);
  oled.drawRFrame(cx - s / 2 + 1, cy - s / 2 + 1, s - 2, s - 2, 4);
  int q = s / 4, r = s / 11 + 1;
  auto P = [&](int dx, int dy) { oled.drawDisc(cx + dx * q, cy + dy * q, r); };
  if (face & 1) P(0, 0);
  if (face >= 2) { P(-1, -1); P(1, 1); }
  if (face >= 4) { P(1, -1); P(-1, 1); }
  if (face == 6) { P(-1, 0); P(1, 0); }
}

// ---------------- کیک تولد ----------------
static void drawCake(int cx, int by, uint32_t t) {     // by = پایین کیک
  oled.drawRBox(cx - 18, by - 12, 36, 12, 2);
  oled.drawRBox(cx - 13, by - 21, 26, 10, 2);
  oled.setDrawColor(0);
  for (int i = 0; i < 6; i++) oled.drawDisc(cx - 15 + i * 6, by - 12, 2);
  oled.setDrawColor(1);
  for (int i = -1; i <= 1; i++) {
    int x = cx + i * 8;
    oled.drawBox(x, by - 27, 2, 6);
    int fl = (t / 120 + i) & 1;
    oled.drawDisc(x + (fl ? 1 : 0), by - 30, 2);
    oled.drawPixel(x + (fl ? 1 : 0), by - 33);
  }
}
// کاغذرنگی
static void drawConfetti(uint32_t t) {
  for (int i = 0; i < 18; i++) {
    uint32_t seed = i * 2654435761u;
    int x = (seed >> 8) % 128;
    int sp = 12 + (seed >> 20) % 20;
    int y = ((t * sp / 1000) + (seed >> 12) % 64) % 70 - 4;
    x += (int)(sinf((t + i * 300) * 0.004f) * 3);
    if (i & 1) oled.drawBox(x, y, 2, 2); else oled.drawPixel(x, y);
  }
}

// ---------------- نوار پیشرفت ----------------
static void drawBar(int x, int y, int w, int h, float k) {
  oled.drawRFrame(x, y, w, h, h / 2);
  int f = (int)((w - 4) * fmaxf(0, fminf(1, k)));
  if (f > 0) oled.drawRBox(x + 2, y + 2, f, h - 4, (h - 4) / 2);
}

// ---------------- بازی حافظه: سه دکمه ----------------
static void drawSimonPads(int lit) {   // ۰ چپ ، ۱ بالا ، ۲ راست ، -۱ هیچ
  struct { int x, y; } P[3] = {{18, 36}, {64, 14}, {110, 36}};
  for (int i = 0; i < 3; i++) {
    if (i == lit) oled.drawDisc(P[i].x, P[i].y, 11);
    else { oled.drawCircle(P[i].x, P[i].y, 11); oled.drawCircle(P[i].x, P[i].y, 10); }
  }
}

// ---------------- آیکن‌های منو ----------------
static void drawMenuIcon(uint8_t i, int cx, int cy, uint32_t t) {
  switch (i) {
    case 0: oled.drawDisc(cx, cy, 13); oled.setDrawColor(0); oled.drawDisc(cx, cy, 6); oled.setDrawColor(1);
            oled.setFont(u8g2_font_6x10_tr); oled.drawStr(cx - 2, cy + 4, "8"); break;              // توپ جادویی
    case 1: drawDie(5, cx, cy, 26); break;                                                          // تاس
    case 2: drawHeart(cx, cy, 24 + 4 * sinf(t * 0.01f)); break;                                     // سرعت
    case 3: oled.drawDisc(cx - 12, cy + 4, 5); oled.drawCircle(cx, cy - 7, 5); oled.drawCircle(cx + 12, cy + 4, 5); break; // حافظه
    case 4: oled.drawCircle(cx, cy, 12); oled.drawLine(cx, cy, cx, cy - 8);
            oled.drawLine(cx, cy, cx + cosf(t * 0.003f) * 9, cy + sinf(t * 0.003f) * 9); break;     // تایمر
    case 5: drawMoon(cx, cy, 12); break;                                                            // چراغ خواب
    case 6: drawHeart(cx, cy - 4, 20); drawBar(cx - 15, cy + 9, 30, 7, 0.7f); break;               // عشق‌سنج
    case 7: {                                                                                       // فال حافظ (کتاب باز)
      int y = cy - 6;
      oled.drawLine(cx, y + 2, cx - 13, y); oled.drawLine(cx - 13, y, cx - 13, y + 12); oled.drawLine(cx - 13, y + 12, cx, y + 14);
      oled.drawLine(cx, y + 2, cx + 13, y); oled.drawLine(cx + 13, y, cx + 13, y + 12); oled.drawLine(cx + 13, y + 12, cx, y + 14);
      oled.drawVLine(cx, y + 2, 13);
      for (int i = 0; i < 3; i++) { oled.drawLine(cx - 10, y + 3 + i * 3, cx - 3, y + 4 + i * 3); oled.drawLine(cx + 3, y + 4 + i * 3, cx + 10, y + 3 + i * 3); }
      break;
    }
    case 8: {                                                                                       // جعبه‌ي موسیقی (نُت)
      int b = (int)(sinf(t * 0.008f) * 2);
      oled.drawDisc(cx - 8, cy + 7 + b, 4); oled.drawDisc(cx + 8, cy + 4 - b, 4);
      oled.drawVLine(cx - 5, cy - 9 + b, 16); oled.drawVLine(cx + 11, cy - 12 - b, 16);
      oled.drawLine(cx - 5, cy - 9 + b, cx + 11, cy - 12 - b); oled.drawLine(cx - 5, cy - 8 + b, cx + 11, cy - 11 - b);
      oled.drawLine(cx - 5, cy - 7 + b, cx + 11, cy - 10 - b);
      break;
    }
    case 9:                                                                                         // جواب به دوست (حباب حرف)
      oled.drawRFrame(cx - 15, cy - 10, 30, 18, 5); oled.drawTriangle(cx - 8, cy + 7, cx - 2, cy + 7, cx - 10, cy + 12);
      drawHeart(cx, cy - 1, 10);
      break;
    default: oled.drawLine(cx - 8, cy - 8, cx + 8, cy + 8); oled.drawLine(cx + 8, cy - 8, cx - 8, cy + 8);
             oled.drawLine(cx - 7, cy - 8, cx + 9, cy + 8); oled.drawLine(cx + 9, cy - 8, cx - 7, cy + 8); break;
  }
}
