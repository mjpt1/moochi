// ===========================================================================
//  موچی ۲ — نوشتن هر متن فارسی/انگلیسی روی OLED
//  (چسباندن حروف فارسی + راست‌به‌چپ + اعداد و انگلیسی چپ‌به‌راست + شکستن خط)
// ===========================================================================
#pragma once
#include "moochi_font.h"

extern U8G2 &oled;
#define PT_MAX 320

static const PGlyph *pfFind(uint16_t cp) {
  int lo = 0, hi = PF_NGLYPH - 1;
  while (lo <= hi) { int m = (lo + hi) / 2; if (PF_GLYPHS[m].cp == cp) return &PF_GLYPHS[m]; if (PF_GLYPHS[m].cp < cp) lo = m + 1; else hi = m - 1; }
  return nullptr;
}
static const PShape *pfShape(uint16_t cp) {
  int lo = 0, hi = PF_NSHAPE - 1;
  while (lo <= hi) { int m = (lo + hi) / 2; if (PF_SHAPES[m].base == cp) return &PF_SHAPES[m]; if (PF_SHAPES[m].base < cp) lo = m + 1; else hi = m - 1; }
  return nullptr;
}
#define PT_HEART 0x2764
static bool ptIsMark(uint16_t c) { return (c >= 0x64B && c <= 0x652) || c == 0x670 || c == 0xFE0F || c == 0x200D; }

// UTF-8 → کدها (ایموجی‌ها همه قلب می‌شوند!)
static int ptDecode(const char *s, uint16_t *out, int max) {
  int n = 0;
  const uint8_t *p = (const uint8_t *)s;
  while (*p && n < max) {
    uint32_t c; int k;
    if (*p < 0x80) { c = *p; k = 1; }
    else if ((*p & 0xE0) == 0xC0) { c = *p & 0x1F; k = 2; }
    else if ((*p & 0xF0) == 0xE0) { c = *p & 0x0F; k = 3; }
    else { c = *p & 0x07; k = 4; }
    for (int i = 1; i < k; i++) { if ((p[i] & 0xC0) != 0x80) { k = i; break; } c = (c << 6) | (p[i] & 0x3F); }
    p += k;
    if (c == '\r') continue;
    if (c == '\t') c = ' ';             // '\n' = شکستن خط اجباری
    if (c >= 0x1F000 || c == 0x2665 || c == 0x2764 || c == 0x2763) c = PT_HEART;
    if (c > 0xFFFF) continue;
    out[n++] = (uint16_t)c;
  }
  return n;
}

enum { JN_NONE = 0, JN_RIGHT = 1, JN_DUAL = 2 };
static uint8_t ptJoin(uint16_t c) {
  if (c == 0x640) return JN_DUAL;
  const PShape *s = pfShape(c);
  if (!s) return JN_NONE;
  if (s->ini) return JN_DUAL;
  if (s->fin) return JN_RIGHT;
  return JN_NONE;
}

// شکل‌دهی: ورودی به ترتیب نوشتن ، خروجی کد شکل‌های چسبان
static int ptShape(const uint16_t *in, int n, uint16_t *out) {
  uint16_t c[PT_MAX]; int m = 0;
  for (int i = 0; i < n; i++) if (!ptIsMark(in[i])) c[m++] = in[i];
  int k = 0;
  for (int i = 0; i < m; i++) {
    uint16_t ch = c[i];
    if (ch == 0x200C) continue;                     // نیم‌فاصله: فقط جدا می‌کند
    const PShape *s = pfShape(ch);
    if (!s) { out[k++] = ch; continue; }
    bool prev = (i > 0 && ptJoin(c[i - 1]) == JN_DUAL);
    // لا
    if (ch == 0x644 && i + 1 < m) {
      for (auto &la : PF_LAMALEF) if (la[0] == c[i + 1]) {
        uint16_t g = prev ? la[2] : la[1];
        if (g) { out[k++] = g; i++; goto next; }
      }
    }
    {
      bool nxt = (ptJoin(ch) == JN_DUAL) && (i + 1 < m) && ptJoin(c[i + 1]) != JN_NONE;
      uint16_t g = 0;
      if (prev && nxt) g = s->med ? s->med : s->fin;
      else if (prev)   g = s->fin;
      else if (nxt)    g = s->ini;
      if (!g) g = s->iso ? s->iso : ch;
      out[k++] = g;
    }
    next:;
  }
  return k;
}

static bool ptLTR(uint16_t c) {
  return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
         (c >= 0x6F0 && c <= 0x6F9) || (c >= 0x660 && c <= 0x669);
}
static int ptAdv(uint16_t c) {
  if (c == PT_HEART) return 12;
  const PGlyph *g = pfFind(c);
  return g ? g->adv : 0;
}
static void ptGlyph(uint16_t c, int x, int y) {
  if (c == PT_HEART) { drawHeart(x + 6, y + PF_HEIGHT / 2, 10); return; }
  const PGlyph *g = pfFind(c);
  if (g && g->w) oled.drawXBMP(x + g->xoff, y, g->w, PF_HEIGHT, PF_BITS + g->off);
}
static int ptWidth(const uint16_t *g, int a, int b) { int w = 0; for (int i = a; i < b; i++) w += ptAdv(g[i]); return w; }

// یک خط را از راست به چپ می‌کشد ؛ xr = لبه‌ي راست
static void ptDrawLine(const uint16_t *g, int a, int b, int xr, int y) {
  while (b > a && g[b - 1] == ' ') b--;
  while (a < b && g[a] == ' ') a++;
  int pen = xr;
  oled.setBitmapMode(1);
  for (int i = a; i < b;) {
    if (ptLTR(g[i])) {                                // تکه‌ي چپ‌به‌راست (عدد / انگلیسی)
      int j = i + 1;
      while (j < b) {
        if (ptLTR(g[j])) { j++; continue; }
        int k = j; while (k < b && !ptLTR(g[k]) && g[k] < 0x600 && g[k] != PT_HEART) k++;
        if (k < b && ptLTR(g[k])) { j = k; continue; }
        break;
      }
      int w = ptWidth(g, i, j), x = pen - w;
      for (int t = i; t < j; t++) { ptGlyph(g[t], x, y); x += ptAdv(g[t]); }
      pen -= w; i = j;
    } else {
      uint16_t c = g[i];                               // آینه‌ای کردن پرانتزها در متن راست‌به‌چپ
      if (c == '(') c = ')'; else if (c == ')') c = '('; else if (c == 0xAB) c = 0xBB; else if (c == 0xBB) c = 0xAB;
      else if (c == '[') c = ']'; else if (c == ']') c = '['; else if (c == '<') c = '>'; else if (c == '>') c = '<';
      int w = ptAdv(c);
      pen -= w; ptGlyph(c, pen, y); i++;
    }
  }
  oled.setBitmapMode(0);
}

// شکستن به خط‌ها ؛ خروجی: شروع هر خط در starts و تعداد خط
static int ptWrap(const uint16_t *g, int n, int width, uint16_t *starts, int maxLines) {
  int lines = 0, i = 0;
  while (i < n && lines < maxLines) {
    while (i < n && g[i] == ' ') i++;
    if (i >= n) break;
    starts[lines++] = i;
    int w = 0, lastSpace = -1, j = i;
    bool hard = false;
    for (; j < n; j++) {
      if (g[j] == '\n') { hard = true; break; }
      if (g[j] == ' ') lastSpace = j;
      w += ptAdv(g[j]);
      if (w > width) break;
    }
    if (hard) { i = j + 1; continue; }
    if (j >= n) { i = n; break; }
    if (j == starts[lines - 1]) j++;                 // یک حرف خیلی پهن
    i = (lastSpace > starts[lines - 1]) ? lastSpace + 1 : j;
  }
  starts[lines] = n;
  return lines;
}

// کل متن آماده‌ي کشیدن
struct PText { uint16_t g[PT_MAX]; int n; uint16_t st[16]; int lines; };
static void ptPrepare(PText &t, const char *utf8, int width = 124) {
  uint16_t raw[PT_MAX];
  int n = ptDecode(utf8, raw, PT_MAX);
  t.n = ptShape(raw, n, t.g);
  t.lines = ptWrap(t.g, t.n, width, t.st, 15);
  // پایان هر خط = شروع خط بعد (ptWrap آخر را n گذاشته)
  for (int i = 0; i < t.lines; i++) if (i + 1 < t.lines) {}
}
static int ptLineEnd(const PText &t, int i) {
  int e = (i + 1 < t.lines) ? t.st[i + 1] : t.n;
  while (e > t.st[i] && (t.g[e - 1] == '\n' || t.g[e - 1] == ' ')) e--;   // پایان خط بدون فاصله/شکست
  return e;
}
static void ptDrawLines(const PText &t, int first, int count, int y, int pitch, bool center = true) {
  for (int i = 0; i < count && first + i < t.lines; i++) {
    int a = t.st[first + i], b = ptLineEnd(t, first + i);
    int w = ptWidth(t.g, a, b);
    // فاصله‌های اول و آخر را حساب نکن
    while (b > a && t.g[b - 1] == ' ') { b--; w -= ptAdv(' '); }
    int xr = center ? 64 + w / 2 : 126;
    ptDrawLine(t.g, a, b, xr, y + i * pitch);
  }
}
