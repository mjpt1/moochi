// ===========================================================================
//  موچی ۲  —  ربات کوچولوی عاشق   (نسخه‌ی ۳: اینترنت ، بازی‌ها ، پیام از راه دور)
//  برد: ESP32-C3 Super Mini
//  OLED ۱٫۳ اینچ + ماژول ساعت (I2C روی 6/7) ، WS2812 روی 4 ، بلندگو روی 5
//  تاچ: بالا 0 ، راست 2 ، چپ 1  —  کلید جیوه‌ای 10
//
//  کنترل‌ها:
//   تاچ بالا: یک بار = ساعت/آب‌وهوا ، دو بار = جمله‌ی عاشقانه ، نگه داشتن = ناز
//   چپ/راست: قلقلک (اگر پیام نخوانده باشد: نشان دادن پیام) ، نگه داشتن = ناز
//   چپ و راست با هم = بغل ؛ ۲ ثانیه نگه داشتن = منوی بازی‌ها
//   تکان شدید = عصبانی ، ضربه = غافلگیر ، کج = گیج
//   در منو: چپ/راست = قبلی/بعدی ، بالا = انتخاب ، نگه داشتن بالا = خروج (در همه‌ی بازی‌ها)
//
//  صفحه‌ی تنظیمات: http://moochi.local     دستورهای Serial (۱۱۵۲۰۰): help
// ===========================================================================
#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <Adafruit_NeoPixel.h>
#include <Preferences.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <time.h>
#include "driver/gpio.h"
#include "config.h"

#if OLED_IS_SSD1306
static U8G2_SSD1306_128X64_NONAME_F_HW_I2C oledDev(OLED_FLIP ? U8G2_R2 : U8G2_R0, U8X8_PIN_NONE);
#else
static U8G2_SH1106_128X64_NONAME_F_HW_I2C oledDev(OLED_FLIP ? U8G2_R2 : U8G2_R0, U8X8_PIN_NONE);
#endif
U8G2 &oled = oledDev;

#include "face.h"
#include "screens.h"
#include "persian_text.h"
#include "moochi_voice_sel.h"
#include "web_page.h"
#include "hafez.h"

static Adafruit_NeoPixel led(1, PIN_LED, (LED_IS_RGB_ORDER ? NEO_RGB : NEO_GRB) + NEO_KHZ800);
static Preferences prefs;
static WebServer server(80);

// ===========================================================================
//  تنظیمات ذخیره‌شده (بعد از خاموش/روشن شدن می‌مانند)
// ===========================================================================
struct Settings {
  uint8_t love = 70;
  uint8_t ah = 7, am = 30, aon = 0, adays = 0x7F;   // زنگ: ساعت ، دقیقه ، روشن ، روزها (بیت ۰ = شنبه)
  uint8_t bm = BDAY_JM, bd = BDAY_JD, anm = ANNIV_JM, anD = ANNIV_JD;
  char tz[48] = TZ_DEFAULT;
  float lat = CITY_LAT, lon = CITY_LON;
  uint8_t city = 0;
  char ssid[33] = WIFI_SSID, pass[65] = WIFI_PASS;
  uint16_t reactBest = 0; uint8_t simonBest = 0;
  char lastId[24] = "";
  char msg[300] = ""; bool unread = false;
  uint8_t msgKind = 0;                       // ۰ متن ، ۱ نقاشی
  char botTok[64] = BOT_TOKEN; uint8_t botApi = BOT_API; int64_t owner = 0;
  uint16_t xp = 0;                           // امتیاز کل محبت (برای کلاه و پاپیون!)
  int32_t meet = 0;                          // تاریخ دیدار yyyymmdd (میلادی)
  char rem[400] = "";                        // یادآورها: هر خط «08:30 متن»
  uint8_t moodDay = 0; char lastMood[40] = "";
  char oname[40] = OWNER_NAME, fname[40] = FRIEND_NAME;   // اسم صاحب و دوست (از صفحه‌ی وب عوض می‌شود)
} S;
static uint8_t doodle[16 * 48];              // نقاشی ۱۲۸×۴۸

static void loadSettings() {
  prefs.begin("moochi", true);
  S.love = prefs.getUChar("love", S.love);
  S.ah = prefs.getUChar("ah", S.ah); S.am = prefs.getUChar("am", S.am);
  S.aon = prefs.getUChar("aon", S.aon); S.adays = prefs.getUChar("adays", S.adays);
  S.bm = prefs.getUChar("bm", S.bm); S.bd = prefs.getUChar("bd", S.bd);
  S.anm = prefs.getUChar("anm", S.anm); S.anD = prefs.getUChar("and", S.anD);
  if (!prefs.getString("tz", S.tz, sizeof S.tz)) strcpy(S.tz, TZ_DEFAULT);
  S.lat = prefs.getFloat("lat", S.lat); S.lon = prefs.getFloat("lon", S.lon);
  S.city = prefs.getUChar("city", S.city);
  if (!prefs.getString("ssid", S.ssid, sizeof S.ssid)) strcpy(S.ssid, WIFI_SSID);
  if (!prefs.getString("pass", S.pass, sizeof S.pass)) strcpy(S.pass, WIFI_PASS);
  S.reactBest = prefs.getUShort("rbest", 0); S.simonBest = prefs.getUChar("sbest", 0);
  prefs.getString("lastId", S.lastId, sizeof S.lastId);
  prefs.getString("msg", S.msg, sizeof S.msg);
  S.unread = prefs.getBool("unread", false);
  S.msgKind = prefs.getUChar("mkind", 0);
  if (!prefs.getString("botTok", S.botTok, sizeof S.botTok)) strlcpy(S.botTok, BOT_TOKEN, sizeof S.botTok);
  S.botApi = prefs.getUChar("botApi", BOT_API); S.owner = prefs.getLong64("owner", 0);
  S.xp = prefs.getUShort("xp", 0); S.meet = prefs.getInt("meet", 0);
  prefs.getString("rem", S.rem, sizeof S.rem);
  S.moodDay = prefs.getUChar("moodDay", 0); prefs.getString("lastMood", S.lastMood, sizeof S.lastMood);
  prefs.getBytes("doodle", doodle, sizeof doodle);
  if (!prefs.getString("oname", S.oname, sizeof S.oname)) strlcpy(S.oname, OWNER_NAME, sizeof S.oname);
  if (!prefs.getString("fname", S.fname, sizeof S.fname)) strlcpy(S.fname, FRIEND_NAME, sizeof S.fname);
  prefs.end();
}
static void saveU8(const char *k, uint8_t v) { prefs.begin("moochi", false); prefs.putUChar(k, v); prefs.end(); }
static void saveStr(const char *k, const char *v) { prefs.begin("moochi", false); prefs.putString(k, v); prefs.end(); }
static void saveAll() {
  prefs.begin("moochi", false);
  prefs.putUChar("ah", S.ah); prefs.putUChar("am", S.am); prefs.putUChar("aon", S.aon); prefs.putUChar("adays", S.adays);
  prefs.putUChar("bm", S.bm); prefs.putUChar("bd", S.bd); prefs.putUChar("anm", S.anm); prefs.putUChar("and", S.anD);
  prefs.putString("tz", S.tz); prefs.putFloat("lat", S.lat); prefs.putFloat("lon", S.lon); prefs.putUChar("city", S.city);
  prefs.putString("ssid", S.ssid); prefs.putString("pass", S.pass);
  prefs.putString("botTok", S.botTok); prefs.putUChar("botApi", S.botApi);
  prefs.putInt("meet", S.meet); prefs.putString("rem", S.rem);
  prefs.putString("oname", S.oname); prefs.putString("fname", S.fname);
  prefs.end();
}

// ===========================================================================
//  صدا — بوق‌ها (نُت) و حرف زدن (صدای ضبط‌شده)
// ===========================================================================
struct Note { uint16_t f; uint16_t ms; };
#define REST 0
static Note sndBuf[128];
static uint8_t sndLen = 0, sndPos = 0;
static uint32_t sndNext = 0;
static bool sndActive = false;
static const uint8_t SPK_CH = 0;
static volatile bool vPlaying = false;

static void spkInit() {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttach(PIN_SPEAKER, 2000, 8);
#else
  ledcSetup(SPK_CH, 2000, 8);
  ledcAttachPin(PIN_SPEAKER, SPK_CH);
#endif
  gpio_set_drive_capability((gpio_num_t)PIN_SPEAKER, GPIO_DRIVE_CAP_3);   // بیشترین جریان = بلندتر
}
static inline void spkDuty(uint32_t d) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(PIN_SPEAKER, d);
#else
  ledcWrite(SPK_CH, d);
#endif
}
static void spkTone(uint16_t f) {
  if (!f || !SOUND_ON) { spkDuty(0); return; }
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  // صداهای بم (خرخر ، غرغر) با دقت ۸ بیت ساخته نمی‌شوند ← دقت بیشتر
  uint8_t bits = f < 400 ? 12 : 8;
  ledcChangeFrequency(PIN_SPEAKER, f, bits);
  spkDuty(((uint32_t)1 << (bits - 1)) * SOUND_VOLUME / 128);
#else
  ledcWriteTone(SPK_CH, f);                         // دقت ۱۰ بیت
  spkDuty((uint32_t)SOUND_VOLUME * 512 / 128);
#endif
}
static void play(const Note *n, uint8_t len) {
  if (vPlaying) return;
  if (len > 128) len = 128;
  memcpy(sndBuf, n, len * sizeof(Note));
  sndLen = len; sndPos = 0; sndActive = true; sndNext = 0;
}
#define PLAY(arr) play(arr, sizeof(arr) / sizeof(arr[0]))
static void soundStop() { sndActive = false; spkDuty(0); }
static void soundTick(uint32_t now) {
  if (vPlaying || !sndActive || now < sndNext) return;
  if (sndPos >= sndLen) { spkTone(0); sndActive = false; return; }
  spkTone(sndBuf[sndPos].f);
  sndNext = now + sndBuf[sndPos].ms;
  sndPos++;
}
static void beep(uint16_t f, uint16_t ms) { Note n[1] = {{f, ms}}; play(n, 1); }
static void babble(uint8_t syll, uint16_t lo, uint16_t hi) {
  Note n[40]; uint8_t k = 0;
  for (uint8_t i = 0; i < syll && k < 38; i++) {
    n[k++] = { (uint16_t)random(lo, hi), (uint16_t)random(40, 95) };
    n[k++] = { REST, (uint16_t)random(15, 45) };
  }
  play(n, k);
}

// ---------------- حرف زدن: PWM ۷۸ کیلوهرتز به‌جای DAC ، ۱۰ هزار نمونه در ثانیه ----------------
static const uint8_t *vData = nullptr;
static volatile uint32_t vLen = 0, vPos = 0;
static hw_timer_t *vTimer = nullptr;
static PhraseId vPending = P_COUNT;
static int32_t vPred = 0; static int vIdx = 0;
static int vPrevOut = 128, vCurOut = 128; static volatile bool vHalf = false;
#define VRAMP 200
static const int16_t ADPCM_STEP[89] = {7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60, 66,
  73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
  876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358, 5894,
  6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767};
static const int8_t ADPCM_IDX[8] = {-1, -1, -1, -1, 2, 4, 6, 8};

// وقفه‌ي سخت‌افزاری با ۲ برابر نرخ نمونه: هر نمونه دو بار ، بار دوم نرم‌شده (میانگین) پخش می‌شود
// (قبلا با esp_timer بود که وای‌فای عقبش می‌انداخت و صدا «پَرِش» داشت)
static void ARDUINO_ISR_ATTR vIsr() {
  if (!vPlaying) return;
  if (vHalf) { vHalf = false; spkDuty((vPrevOut + vCurOut) >> 1); return; }
  uint32_t i = vPos;
  if (i >= vLen) { vPlaying = false; spkDuty(0); return; }
  uint8_t b = vData[i >> 1]; uint8_t c = (i & 1) ? (b >> 4) : (b & 15);
  int st = ADPCM_STEP[vIdx], diff = st >> 3;
  if (c & 4) diff += st;
  if (c & 2) diff += st >> 1;
  if (c & 1) diff += st >> 2;
  vPred += (c & 8) ? -diff : diff;
  if (vPred > 32767) vPred = 32767; else if (vPred < -32768) vPred = -32768;
  vIdx += ADPCM_IDX[c & 7]; if (vIdx < 0) vIdx = 0; else if (vIdx > 88) vIdx = 88;
  int v = 128 + (vPred >> 8) * VOICE_VOLUME / 128;
  if (i < VRAMP) v = v * (int)i / VRAMP;
  else if (vLen - i < VRAMP) v = v * (int)(vLen - i) / VRAMP;
  v = v < 0 ? 0 : (v > 255 ? 255 : v);
  vPrevOut = vCurOut; vCurOut = v;
  spkDuty(vPrevOut);
  vHalf = true;
  vPos = i + 1;
}
// اگر اسم‌ها با اسم‌های داخل صدا فرق کنند ، جمله‌هایی که اسم دارند بی‌صدا می‌شوند
static bool voiceMuted(PhraseId p) {
  if (!strcmp(S.oname, VOICE_OWNER) && !strcmp(S.fname, VOICE_FRIEND)) return false;
  for (size_t i = 0; i < VOICE_NAMED_N; i++) if (VOICE_NAMED[i] == p) return true;
  return false;
}
static bool hasVoice(PhraseId p) { return VOICE_ON && SOUND_ON && p < P_COUNT && VOICES[p].n > 0 && !voiceMuted(p); }
static uint32_t voiceMs(PhraseId p) { return hasVoice(p) ? VOICES[p].n * 1000UL / VOICE_RATE : 0; }
static void voiceStop() {
  vPending = P_COUNT;
  if (vPlaying) { vPlaying = false; spkDuty(0); }
}
static bool vTimerOn = false;
static void voiceStart(PhraseId p) {
  sndActive = false;
  if (!vTimerOn) { timerStart(vTimer); vTimerOn = true; }
  vPlaying = false;
  vData = VOICES[p].d; vLen = VOICES[p].n; vPos = 0; vPred = 0; vIdx = 0; vPrevOut = vCurOut = 0; vHalf = false;
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcChangeFrequency(PIN_SPEAKER, 78125, 8);
#else
  ledcChangeFrequency(SPK_CH, 78125, 8);
#endif
  spkDuty(0);
  vPlaying = true;
}
static void voiceInit() {
  uint32_t us = 1000000UL / (VOICE_RATE * 2);
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  vTimer = timerBegin(1000000);
  timerAttachInterrupt(vTimer, &vIsr);
  timerAlarm(vTimer, us, true, 0);
  timerStop(vTimer);                        // فقط موقع حرف زدن روشن می‌شود
#else
  vTimer = timerBegin(0, 80, true);
  timerAttachInterrupt(vTimer, &vIsr, true);
  timerAlarmWrite(vTimer, us, true);
  timerAlarmEnable(vTimer);
  timerStop(vTimer);
#endif
}
static void voiceSay(PhraseId p) { if (hasVoice(p)) { voiceStop(); vPending = p; } }
static void voiceTick() {
  if (vTimerOn && !vPlaying && vPending == P_COUNT) { timerStop(vTimer); vTimerOn = false; spkDuty(0); }
  if (vPending != P_COUNT && !sndActive && !vPlaying) { PhraseId p = vPending; vPending = P_COUNT; voiceStart(p); }
}

static const Note S_BOOT[]  = {{523,90},{0,20},{659,90},{0,20},{784,90},{0,20},{1047,180},{0,60},{988,80},{1175,220}};
static const Note S_HAPPY[] = {{880,60},{0,15},{1175,60},{0,15},{1568,110}};
static const Note S_GIGGLE[]= {{1400,45},{0,30},{1700,45},{0,30},{1400,45},{0,30},{1900,45},{0,30},{1500,60}};
static const Note S_LOVE[]  = {{659,140},{0,30},{784,140},{0,30},{1047,260},{0,50},{988,120},{1047,300}};
static const Note S_SURP[]  = {{600,30},{800,30},{1000,30},{1300,30},{1700,30},{2200,90}};
static const Note S_ANGRY[] = {{140,70},{110,70},{150,70},{100,70},{160,70},{95,90},{0,40},{220,60},{180,140}};
static const Note S_SAD[]   = {{784,180},{0,20},{698,180},{0,20},{587,180},{0,20},{494,380}};
static const Note S_PURR[]  = {{80,60},{0,40},{90,60},{0,40},{80,60},{0,40},{95,70},{0,40},{80,60}};
static const Note S_CLICK[] = {{2600,15},{0,20},{3200,25}};
static const Note S_CLOCK[] = {{1319,60},{0,20},{1760,90}};
static const Note S_CHIME[] = {{1568,120},{0,40},{1319,120},{0,40},{1047,260}};
static const Note S_SNEEZE[] = {{700,110},{0,70},{820,150},{0,260},{1900,40},{1200,40},{500,140}};
static const Note S_DING[] = {{1319,90},{0,30},{1760,90},{0,30},{2093,200}};
static const Note S_YAWN[]  = {{700,120},{650,120},{560,160},{480,220},{420,320}};
static const Note S_WAKE[]  = {{500,60},{700,60},{900,60},{1300,140}};
static const Note S_ANNOY[] = {{300,90},{0,40},{260,90},{0,40},{220,180}};
static const Note S_DIZZY[] = {{900,50},{1100,50},{900,50},{700,50},{900,50},{1100,50},{900,50},{700,90}};
static const Note S_MSG[]   = {{1047,80},{0,30},{1319,80},{0,30},{1568,80},{0,30},{2093,200}};
static const Note S_ALARM[] = {{1760,90},{0,50},{1760,90},{0,50},{1760,90},{0,50},{1760,90},{0,400},
                               {2093,90},{0,50},{2093,90},{0,50},{2093,90},{0,50},{2093,90},{0,400}};
static const Note S_WIN[]   = {{784,90},{0,20},{988,90},{0,20},{1175,90},{0,20},{1568,250}};
static const Note S_LOSE[]  = {{392,200},{0,30},{330,200},{0,30},{262,450}};
static const Note S_BDAY[]  = {{392,220},{0,40},{392,110},{440,330},{392,330},{523,330},{494,600},{0,120},
                               {392,220},{0,40},{392,110},{440,330},{392,330},{587,330},{523,600},{0,120},
                               {392,220},{0,40},{392,110},{784,330},{659,330},{523,330},{494,330},{440,600},{0,120},
                               {698,220},{0,40},{698,110},{659,330},{523,330},{587,330},{523,700}};

// ===========================================================================
//  ماژول ساعت (DS3231 / DS1307) و زمان
// ===========================================================================
#define RTC_ADDR 0x68
struct DT { int y, mo, d, h, mi, s; };
static bool rtcOk = false, sysTimeOk = false;
static DT now_dt = {2026, 1, 1, 12, 0, 0};
static uint32_t rtcReadAt = 0, softBase = 0;
static uint8_t bcd2(uint8_t v) { return (v >> 4) * 10 + (v & 15); }
static uint8_t tobcd(uint8_t v) { return ((v / 10) << 4) | (v % 10); }

static bool rtcRead(DT &t) {
  Wire.beginTransmission(RTC_ADDR); Wire.write(0);
  if (Wire.endTransmission() != 0) return false;
  if (Wire.requestFrom(RTC_ADDR, 7) != 7) return false;
  uint8_t r[7]; for (int i = 0; i < 7; i++) r[i] = Wire.read();
  t.s = bcd2(r[0] & 0x7F); t.mi = bcd2(r[1] & 0x7F);
  if (r[2] & 0x40) { int h = bcd2(r[2] & 0x1F) % 12; if (r[2] & 0x20) h += 12; t.h = h; }
  else t.h = bcd2(r[2] & 0x3F);
  t.d = bcd2(r[4] & 0x3F); t.mo = bcd2(r[5] & 0x1F); t.y = 2000 + bcd2(r[6]);
  bool halted = r[0] & 0x80;
  return !halted && t.mo >= 1 && t.mo <= 12 && t.d >= 1 && t.d <= 31 && t.h < 24 && t.mi < 60 && t.s < 60;
}
static void rtcWrite(const DT &t) {
  Wire.beginTransmission(RTC_ADDR); Wire.write(0);
  Wire.write(tobcd(t.s)); Wire.write(tobcd(t.mi)); Wire.write(tobcd(t.h));
  Wire.write(1); Wire.write(tobcd(t.d)); Wire.write(tobcd(t.mo)); Wire.write(tobcd(t.y - 2000));
  Wire.endTransmission();
  Wire.beginTransmission(RTC_ADDR); Wire.write(0x0F); Wire.endTransmission();
  if (Wire.requestFrom(RTC_ADDR, 1) == 1) {
    uint8_t st = Wire.read();
    Wire.beginTransmission(RTC_ADDR); Wire.write(0x0F); Wire.write(st & 0x7F); Wire.endTransmission();
  }
}
static DT compileTime() {
  static const char *mons = "JanFebMarAprMayJunJulAugSepOctNovDec";
  char m[4] = {0}; int d, y, h, mi, s;
  sscanf(__DATE__, "%3s %d %d", m, &d, &y); sscanf(__TIME__, "%d:%d:%d", &h, &mi, &s);
  int mo = (strstr(mons, m) - mons) / 3 + 1;
  return {y, mo, d, h, mi, s};
}
static void timeTick(uint32_t now) {
  if (now - rtcReadAt < 400) return;
  rtcReadAt = now;
  if (rtcOk) { DT t; if (rtcRead(t)) { now_dt = t; return; } }
  if (sysTimeOk) {
    time_t tt = time(nullptr); struct tm tm; localtime_r(&tt, &tm);
    now_dt = {tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec};
    return;
  }
  uint32_t sec = (now - softBase) / 1000;
  DT c = compileTime();
  uint32_t tot = c.h * 3600UL + c.mi * 60 + c.s + sec;
  now_dt = c; now_dt.h = (tot / 3600) % 24; now_dt.mi = (tot / 60) % 60; now_dt.s = tot % 60;
}
static bool isNight() {
  return (NIGHT_FROM_H > NIGHT_TO_H) ? (now_dt.h >= NIGHT_FROM_H || now_dt.h < NIGHT_TO_H)
                                     : (now_dt.h >= NIGHT_FROM_H && now_dt.h < NIGHT_TO_H);
}
static void jalaliToday(int &jy, int &jm, int &jd) { g2j(now_dt.y, now_dt.mo, now_dt.d, jy, jm, jd); }
static int jdoy(int m, int d) { return m <= 6 ? (m - 1) * 31 + d : 186 + (m - 7) * 30 + d; }
static int daysUntil(int tm, int td) {
  if (!tm || !td) return -1;
  int jy, jm, jd; jalaliToday(jy, jm, jd);
  int diff = jdoy(tm, td) - jdoy(jm, jd);
  if (diff < 0) diff += 365;
  return diff;
}

// ===========================================================================
//  اینترنت — یک «تسک» جدا تا انیمیشن‌ها گیر نکنند
// ===========================================================================
static volatile bool netUp = false, apOn = false, netCfgChanged = true;
static char netIP[20] = "-";
static volatile int wxTemp = -999, wxCode = -1; static volatile bool wxDay = true;
static volatile bool ntpFresh = false;
static char inBuf[1200]; static char inId[24]; static volatile bool inReady = false;
static char outBuf[200]; static volatile bool outReady = false;
static char netTz[48]; static float netLat, netLon;

static String jsonStr(const String &s, int from) {     // متن JSON بعد از from (بعد از کوتیشن)
  String out; out.reserve(64);
  for (int i = from; i < (int)s.length(); i++) {
    char c = s[i];
    if (c == '"') break;
    if (c == '\\' && i + 1 < (int)s.length()) {
      char e = s[++i];
      if (e == 'n') out += '\n';
      else if (e == 'u' && i + 4 < (int)s.length()) {
        uint32_t cp = strtol(s.substring(i + 1, i + 5).c_str(), nullptr, 16); i += 4;
        if (cp >= 0xD800 && cp <= 0xDBFF) { cp = 0x1F496; if (i + 6 < (int)s.length() && s[i + 1] == '\\') i += 6; }
        if (cp < 0x80) out += (char)cp;
        else if (cp < 0x800) { out += (char)(0xC0 | cp >> 6); out += (char)(0x80 | (cp & 63)); }
        else if (cp < 0x10000) { out += (char)(0xE0 | cp >> 12); out += (char)(0x80 | ((cp >> 6) & 63)); out += (char)(0x80 | (cp & 63)); }
        else { out += (char)(0xF0 | cp >> 18); out += (char)(0x80 | ((cp >> 12) & 63)); out += (char)(0x80 | ((cp >> 6) & 63)); out += (char)(0x80 | (cp & 63)); }
      } else out += e;
    } else out += c;
  }
  return out;
}

static void netWeather(WiFiClientSecure &cli) {
  HTTPClient http;
  char url[200];
  snprintf(url, sizeof url, "https://api.open-meteo.com/v1/forecast?latitude=%.2f&longitude=%.2f&current=temperature_2m,weather_code,is_day&timezone=auto", netLat, netLon);
  http.setTimeout(8000);
  if (!http.begin(cli, url)) return;
  if (http.GET() == 200) {
    String b = http.getString();
    int c = b.indexOf("\"current\":{");
    if (c >= 0) {
      int t = b.indexOf("\"temperature_2m\":", c), w = b.indexOf("\"weather_code\":", c), d = b.indexOf("\"is_day\":", c);
      if (t > 0 && w > 0) {
        wxTemp = (int)lroundf(b.substring(t + 17).toFloat());
        wxCode = b.substring(w + 15).toInt();
        wxDay = d > 0 ? b.substring(d + 9).toInt() != 0 : true;
      }
    }
  }
  http.end();
}
static uint8_t pollFail = 0;
static void netPoll(WiFiClientSecure &cli) {
  if (inReady) return;
  HTTPClient http;
  String url = String(NTFY_SERVER) + "/" + NTFY_TOPIC + "/json?poll=1&since=" + (S.lastId[0] ? S.lastId : "12h");
  http.setTimeout(8000);
  if (!http.begin(cli, url)) return;
  int code = http.GET();
  if (code < 0) { if (pollFail < 4) pollFail++; if (pollFail == 1) Serial.println("ntfy.sh not reachable (filtered?) -> retry slower"); }
  else pollFail = 0;
  if (code == 200) {
    String b = http.getString();
    String lastMsg, lastId;
    int p = 0;
    while (p < (int)b.length()) {
      int e = b.indexOf('\n', p); if (e < 0) e = b.length();
      String ln = b.substring(p, e); p = e + 1;
      if (ln.indexOf("\"event\":\"message\"") < 0) continue;
      int i = ln.indexOf("\"id\":\""); if (i >= 0) lastId = jsonStr(ln, i + 6);
      int m = ln.indexOf("\"message\":\""); lastMsg = (m >= 0) ? jsonStr(ln, m + 11) : String("");
    }
    if (lastId.length()) {
      strlcpy(inBuf, lastMsg.c_str(), sizeof inBuf);
      strlcpy(inId, lastId.c_str(), sizeof inId);
      inReady = true;
    }
  }
  http.end();
}
static void netPublish(WiFiClientSecure &cli) {
  HTTPClient http;
  String url = String(NTFY_SERVER) + "/" + NTFY_TOPIC + "-back";
  http.setTimeout(8000);
  if (http.begin(cli, url)) {
    http.addHeader("Title", "Moochi");
    http.addHeader("Tags", "heart");
    http.POST((uint8_t *)outBuf, strlen(outBuf));
    http.end();
  }
  outReady = false;
}
static volatile int wifiReason = 0;
static const char *reasonText(int r) {
  switch (r) {
    case 201: return "SSID not found (5GHz? name wrong? too far?)";
    case 15: case 204: return "wrong password (handshake failed)";
    case 202: case 2: return "auth failed / rejected";
    case 203: return "association failed";
    case 200: return "beacon timeout (weak signal)";
    case 8: return "left network";
    default: return "";
  }
}
static void wifiEvt(WiFiEvent_t ev, WiFiEventInfo_t info) {
  if (ev == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
    int r = info.wifi_sta_disconnected.reason;
    if (r != wifiReason) Serial.printf("WiFi disconnected, reason %d %s\n", r, reasonText(r));
    wifiReason = r;
  } else if (ev == ARDUINO_EVENT_WIFI_STA_GOT_IP) {
    wifiReason = 0;
    Serial.printf("WiFi connected: %s\n", WiFi.localIP().toString().c_str());
  }
}
static void wifiScanPrint() {
  int n = WiFi.scanNetworks();
  Serial.printf("WiFi scan: %d networks (ESP32-C3 sees only 2.4GHz)\n", n);
  bool found = false;
  for (int i = 0; i < n; i++) {
    bool me = WiFi.SSID(i) == S.ssid; found |= me;
    Serial.printf("  %s%-24s ch%-2d %ddBm\n", me ? "> " : "  ", WiFi.SSID(i).c_str(), WiFi.channel(i), WiFi.RSSI(i));
  }
  if (!found) Serial.printf("!! '%s' NOT visible -> it is 5GHz only, or too far\n", S.ssid);
  WiFi.scanDelete();
}
static void wifiStart() {
  WiFi.begin(S.ssid, S.pass);
  // بردهای ESP32-C3 Super Mini آنتن ضعیفی دارند و با توان کامل اغلب وصل نمی‌شوند
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
}
// ---------------- بات بله / تلگرام ----------------
static volatile int64_t inChat = 0;          // پیام از کدام چت آمده (۰ = صفحه‌ي وب/ntfy)
struct BotOut { int64_t chat; char text[800]; };
static BotOut botOut[4]; static volatile uint8_t botHead = 0, botTail = 0;
static int64_t botOffset = 0; static uint8_t botFail = 0;
static void botQueue(int64_t chat, const char *t) {
  if (!chat || !S.botTok[0]) return;
  uint8_t n = (botHead + 1) % 4; if (n == botTail) return;
  botOut[botHead].chat = chat; strlcpy(botOut[botHead].text, t, sizeof botOut[0].text);
  botHead = n;
}
static String botBase() { return String(S.botApi ? "https://api.telegram.org/bot" : "https://tapi.bale.ai/bot") + S.botTok; }
static String jsonEsc(const char *t) {
  String o; o.reserve(strlen(t) + 16);
  for (; *t; t++) {
    char c = *t;
    if (c == '"' || c == '\\') { o += '\\'; o += c; }
    else if (c == '\n') o += "\\n";
    else if ((uint8_t)c < 0x20) o += ' ';
    else o += c;
  }
  return o;
}
static void botSend(WiFiClientSecure &cli, int64_t chat, const char *text) {
  HTTPClient http; http.setTimeout(10000);
  if (!http.begin(cli, botBase() + "/sendMessage")) return;
  http.addHeader("Content-Type", "application/json");
  String body = String("{\"chat_id\":") + String((long long)chat) + ",\"text\":\"" + jsonEsc(text) + "\"}";
  int c = http.POST(body);
  if (c != 200) Serial.printf("[bot] send failed %d\n", c);
  http.end();
}
static void botPoll(WiFiClientSecure &cli) {
  if (inReady) return;
  HTTPClient http; http.setTimeout(20000);
  String url = botBase() + "/getUpdates?limit=1&timeout=10&offset=" + String((long long)botOffset);
  if (!http.begin(cli, url)) return;
  int code = http.GET();
  if (code < 0) { if (botFail < 4) botFail++; if (botFail == 1) Serial.println("[bot] server not reachable"); http.end(); return; }
  botFail = 0;
  if (code != 200) { Serial.printf("[bot] getUpdates HTTP %d (token wrong?)\n", code); botFail = 3; http.end(); return; }
  String b = http.getString(); http.end();
  int u = b.indexOf("\"update_id\":");
  if (u < 0) return;
  botOffset = atoll(b.c_str() + u + 12) + 1;
  int ch = b.indexOf("\"chat\":{", u); int64_t chat = 0;
  if (ch >= 0) { int id = b.indexOf("\"id\":", ch); if (id >= 0) chat = atoll(b.c_str() + id + 5); }
  int t = b.lastIndexOf("\"text\":\"");     // آخرین = متن خود پیام (نه پیامی که به آن جواب داده)
  if (t < u) t = -1;
  String txt = (t >= 0) ? jsonStr(b, t + 8) : String("❤");   // استیکر و عکس = قلب
  if (chat) { strlcpy(inBuf, txt.c_str(), sizeof inBuf); inChat = chat; inReady = true; }
}

static void netTask(void *) {
  WiFi.onEvent(wifiEvt);
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(HOSTNAME);
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
  WiFi.setAutoReconnect(true);
  vTaskDelay(pdMS_TO_TICKS(300));
  wifiScanPrint();
  wifiStart();
  uint32_t t0 = millis(), lastTry = millis(), upSince = 0;
  uint32_t nextNtp = 0, nextWx = 0, nextPoll = 0, nextBot = 0;
  WiFiClientSecure cli; cli.setInsecure();
  for (;;) {
    uint32_t now = millis();
    if (WiFi.status() != WL_CONNECTED) {
      netUp = false; upSince = 0;
      if (!apOn && now - t0 > 30000) {
        WiFi.mode(WIFI_AP_STA); WiFi.softAP("Moochi", AP_PASS); WiFi.setTxPower(WIFI_POWER_8_5dBm); apOn = true;
        Serial.println("AP 'Moochi' started (pass " AP_PASS ") -> http://192.168.4.1");
        strlcpy(netIP, "192.168.4.1", sizeof netIP);
      }
      if (now - lastTry > 60000) {                    // هر یک دقیقه یک تلاش کامل از نو
        lastTry = now;
        WiFi.disconnect(false, false); vTaskDelay(pdMS_TO_TICKS(800));
        wifiStart();
      }
      vTaskDelay(pdMS_TO_TICKS(500));
      continue;
    }
    if (!netUp) { netUp = true; upSince = now; strlcpy(netIP, WiFi.localIP().toString().c_str(), sizeof netIP); nextNtp = nextWx = nextPoll = now; }
    if (apOn && now - upSince > 120000) { WiFi.softAPdisconnect(true); WiFi.mode(WIFI_STA); apOn = false; }
    if (netCfgChanged) {
      netCfgChanged = false;
      strlcpy(netTz, S.tz, sizeof netTz); netLat = S.lat; netLon = S.lon;
      nextNtp = nextWx = now;
    }
    if ((int32_t)(now - nextNtp) >= 0) {
      configTzTime(netTz, "pool.ntp.org", "time.google.com", "time.cloudflare.com");
      struct tm tm;
      if (getLocalTime(&tm, 10000) && tm.tm_year > 120) { sysTimeOk = true; ntpFresh = true; nextNtp = now + 6UL * 3600000; }
      else nextNtp = now + 60000;
    }
    if ((int32_t)(now - nextWx) >= 0) { netWeather(cli); nextWx = now + 30UL * 60000; }
    if (S.botTok[0]) {
      while (botTail != botHead) { botSend(cli, botOut[botTail].chat, botOut[botTail].text); botTail = (botTail + 1) % 4; }
      if ((int32_t)(millis() - nextBot) >= 0) { botPoll(cli); nextBot = millis() + (botFail ? (5000UL << botFail) : 300); }
    }
#if NTFY_ON
    if (outReady) netPublish(cli);
    if ((int32_t)(now - nextPoll) >= 0) { netPoll(cli); nextPoll = millis() + (20000UL << pollFail); }   // اگر در دسترس نبود: کمتر تلاش کن
#endif
    vTaskDelay(pdMS_TO_TICKS(300));
  }
}
static uint32_t lastPub[4];
static void notifyFriend(uint8_t kind, const char *text, uint32_t minGapMs) {
  uint32_t now0 = millis();
  if (S.botTok[0] && S.owner) {
    if (lastPub[kind] && now0 - lastPub[kind] < minGapMs) return;
    lastPub[kind] = now0; botQueue(S.owner, text); return;
  }
#if WIFI_ON && NTFY_ON
  uint32_t now = millis();
  if (lastPub[kind] && now - lastPub[kind] < minGapMs) return;
  if (outReady) return;
  lastPub[kind] = now;
  strlcpy(outBuf, text, sizeof outBuf);
  outReady = true;
#endif
}

// ===========================================================================
//  ورودی‌ها: تاچ‌ها
// ===========================================================================
enum Ev : uint8_t { EV_NONE, EV_TOP_TAP, EV_TOP_DOUBLE, EV_TOP_LONG, EV_LEFT_TAP, EV_RIGHT_TAP,
                    EV_SIDE_LONG, EV_HUG, EV_MENU, EV_ANNOY, EV_SHAKE, EV_BUMP, EV_TILT, EV_UNTILT };
struct Touch {
  uint8_t pin; bool raw = false, down = false; uint32_t tRaw = 0, tDown = 0;
  bool longFired = false, consumed = false;
  explicit Touch(uint8_t p) : pin(p) {}
};
static Touch tTop{PIN_TOUCH_TOP}, tLeft{PIN_TOUCH_LEFT}, tRight{PIN_TOUCH_RIGHT};
static uint32_t topFirstUp = 0; static bool topWaitDouble = false, fastTop = false;
static uint32_t pokeTimes[8]; static uint8_t pokeIdx = 0;
static uint32_t hugAt = 0; static bool menuFired = false;

static Ev evQ[8]; static uint8_t evN = 0;
static void pushEv(Ev e) { if (evN < 8) evQ[evN++] = e; }
static bool readTouch(uint8_t pin) { return digitalRead(pin) == (TOUCH_ACTIVE_HIGH ? HIGH : LOW); }

static bool touchUpdate(Touch &t, uint32_t now, bool &released) {
  bool r = readTouch(t.pin); released = false;
  if (r != t.raw) { t.raw = r; t.tRaw = now; }
  if (now - t.tRaw < 25 || t.raw == t.down) return false;
  t.down = t.raw;
  if (t.down) { t.tDown = now; t.longFired = false; t.consumed = false; return true; }
  released = true; return false;
}
static void poke(uint32_t now) {
  if (fastTop) return;                               // در بازی‌ها «بسه دیگه» نگوید
  pokeTimes[pokeIdx++ % 8] = now;
  uint8_t c = 0; for (auto p : pokeTimes) if (p && now - p < 3500) c++;
  if (c >= 7) { pushEv(EV_ANNOY); memset(pokeTimes, 0, sizeof(pokeTimes)); }
}
static void touchesTick(uint32_t now) {
  bool rel;
  if (touchUpdate(tTop, now, rel)) poke(now);
  if (tTop.down && !tTop.longFired && now - tTop.tDown > 900) { tTop.longFired = true; pushEv(EV_TOP_LONG); }
  if (rel && !tTop.longFired) {
    if (fastTop) pushEv(EV_TOP_TAP);
    else if (topWaitDouble && now - topFirstUp < 380) { topWaitDouble = false; pushEv(EV_TOP_DOUBLE); }
    else { topWaitDouble = true; topFirstUp = now; }
  }
  if (topWaitDouble && !tTop.down && now - topFirstUp >= 380) { topWaitDouble = false; pushEv(EV_TOP_TAP); }
  Touch *side[2] = {&tLeft, &tRight};
  for (int i = 0; i < 2; i++) {
    Touch &t = *side[i];
    if (touchUpdate(t, now, rel)) poke(now);
    if (t.down && !t.consumed && !t.longFired && now - t.tDown > 900) { t.longFired = true; pushEv(EV_SIDE_LONG); }
    if (rel && !t.consumed && !t.longFired) pushEv(i == 0 ? EV_LEFT_TAP : EV_RIGHT_TAP);
  }
  if (tLeft.down && tRight.down && !tLeft.consumed && !tRight.consumed) {
    tLeft.consumed = tRight.consumed = true; tLeft.longFired = tRight.longFired = true;
    hugAt = now; menuFired = false;
    pushEv(EV_HUG);
  }
  if (tLeft.down && tRight.down && tLeft.consumed && !menuFired && now - hugAt > 2000) { menuFired = true; pushEv(EV_MENU); }
}

// ===========================================================================
//  ورودی‌ها: کلید جیوه‌ای
// ===========================================================================
static volatile uint32_t tiltEdges[24]; static volatile uint8_t tiltEdgeIdx = 0;
static volatile uint32_t tiltLastUs = 0, tiltLastMs = 0;
static void IRAM_ATTR tiltIsr() {
  uint32_t us = micros();
  if (us - tiltLastUs < 2500) return;
  tiltLastUs = us; tiltLastMs = millis();
  uint8_t i = tiltEdgeIdx; tiltEdges[i % 24] = tiltLastMs; tiltEdgeIdx = i + 1;
}
static uint32_t tiltCooldown = 0, tiltEvalFrom = 0;
static int tiltBaseline = -1; static bool tiltedNow = false;
static int tiltCount(uint32_t now, uint32_t win, uint32_t from) {
  int c = 0;
  for (int i = 0; i < 24; i++) { uint32_t e = tiltEdges[i]; if (e && now - e < win && e >= from) c++; }
  return c;
}
static void tiltTick(uint32_t now) {
  if (now < tiltCooldown) return;
  int c = tiltCount(now, SHAKE_WINDOW_MS, tiltEvalFrom);
  if (c >= SHAKE_EDGES_HARD) { pushEv(EV_SHAKE); tiltEvalFrom = now; tiltCooldown = now + 1500; return; }
  if (c >= BUMP_EDGES_MIN && now - tiltLastMs > 450) { pushEv(EV_BUMP); tiltEvalFrom = now; tiltCooldown = now + 800; return; }
#if TILT_STEADY_ON
  int lvl = digitalRead(PIN_TILT);
  if (tiltBaseline < 0) tiltBaseline = lvl;
  bool off = (lvl != tiltBaseline);
  if (now - tiltLastMs > TILT_STEADY_MS) {
    if (off && !tiltedNow) { tiltedNow = true; pushEv(EV_TILT); }
    else if (!off && tiltedNow) { tiltedNow = false; pushEv(EV_UNTILT); }
  }
#endif
}

// ===========================================================================
//  حالت‌ها
// ===========================================================================
enum Mode : uint8_t { MODE_BOOT, MODE_FACE, MODE_CLOCK, MODE_DIAG, MODE_MSG, MODE_MENU, MODE_APP,
                      MODE_ALARM, MODE_PARTY, MODE_OTA, MODE_ASK, MODE_NOTE };
static Mode mode = MODE_BOOT;
static uint32_t bootAt = 0, modeAt = 0;
static uint8_t bootPhase = 0;
static const uint32_t BOOT_MIN[4] = {1400, 2000, 2500, 3300};
static const PhraseId BOOT_P[4] = {P_COUNT, P_HELLO, P_IAM, P_OWNER};
static const char *MODE_NAMES[] = {"boot", "face", "clock", "diag", "msg", "menu", "app", "alarm", "party", "ota", "ask", "note"};
static void setMode(Mode m) { if (m != mode) Serial.printf("[mode] %s -> %s\n", MODE_NAMES[mode], MODE_NAMES[m]); mode = m; modeAt = millis(); fastTop = (m == MODE_MENU || m == MODE_APP || m == MODE_ALARM || m == MODE_MSG || m == MODE_ASK || m == MODE_NOTE); }

static Mood baseMood = M_NEUTRAL, tmpMood = M_NEUTRAL;
static uint32_t tmpUntil = 0;
static Mood afterTmp = M_COUNT; static uint32_t afterTmpMs = 0;
static PhraseId msg = P_COUNT; static uint32_t msgUntil = 0;
static uint32_t lastActive = 0, nextLook = 0, nextBlink = 0, blinkAt = 0, nextLove = 0, nextLonely = 0, nextNewMsg = 0;
static uint32_t clockTempUntil = 0;
static float lookTX = 0, lookTY = 0;
static EyeP eye = moodTarget(M_NEUTRAL);
static uint8_t angerLevel = 0; static uint32_t angerAt = 0;
static int lastAngryIdx = -1, lastHour = -1, greetedDay = -1, partyDay = -1;
static bool sleepyAnnounced = false;
static uint32_t loveSaveAt = 0, loveDecayAt = 0; static bool loveDirty = false;

static Mood curMood(uint32_t now) { return (now < tmpUntil) ? tmpMood : baseMood; }
static void say(PhraseId p, uint32_t ms) {
  msg = p;
  uint32_t vm = voiceMs(p);
  msgUntil = millis() + (vm ? max(ms, vm + 900) : ms);
  if (vm) { voiceStop(); vPending = p; }
}
static PhraseId pick(const uint8_t *g, uint8_t n) { return (PhraseId)g[random(n)]; }
static void feel(Mood m, uint32_t ms) { tmpMood = m; tmpUntil = millis() + ms; afterTmp = M_COUNT; }
static bool xpDirty = false;
static void loveAdd(int d) {
  if (d > 0 && S.xp < 60000) { S.xp += d; xpDirty = true; }
  int v = (int)S.love + d; S.love = v < 0 ? 0 : (v > 100 ? 100 : v); loveDirty = true;
}

// ---------------- اسم‌ها (از صفحه‌ی وب یا بات عوض می‌شوند) ----------------
// جمله‌های آماده‌ای که اسم دارند (جدول NAMED در moochi_text*.h) اگر اسم با اسم داخل تصویرها فرق کند
// روی خود برد با فونت فارسی از نو نوشته می‌شوند و در RAM نگه داشته می‌شوند.
static TxtLine namedLines[NAMED_N][2]; static Phrase namedPh[NAMED_N];
static volatile bool namesDirty = false;
static void nameSubst(char *out, size_t n, const char *tpl) {
  size_t k = 0;
  for (const char *p = tpl; *p && k + 1 < n; ) {
    if (p[0] == '{' && (p[1] == 'O' || p[1] == 'F') && p[2] == '}') {
      for (const char *q = (p[1] == 'O') ? S.oname : S.fname; *q && k + 1 < n; ) out[k++] = *q++;
      p += 3;
    } else out[k++] = *p++;
  }
  out[k] = 0;
}
static bool renderNamedLine(const char *txt, bool big, TxtLine &L) {
  static uint16_t raw[PT_MAX], g[PT_MAX];
  int n = ptShape(raw, ptDecode(txt, raw, PT_MAX), g);
  oled.clearBuffer();
  ptDrawLine(g, 0, n, 127, 0);
  const uint8_t *buf = oled.getBufferPtr();
  int x0 = 128, x1 = -1, y0 = 64, y1 = -1;
  for (int y = 0; y < PF_HEIGHT + 2 && y < 64; y++)
    for (int x = 0; x < 128; x++)
      if ((buf[(y / 8) * 128 + x] >> (y % 8)) & 1) { if (x < x0) x0 = x; if (x > x1) x1 = x; if (y < y0) y0 = y; if (y > y1) y1 = y; }
  if (x1 < 0) return false;
  int sc = (big && (x1 - x0 + 1) * 2 <= 120) ? 2 : 1;
  int w = (x1 - x0 + 1) * sc, h = (y1 - y0 + 1) * sc, bw = (w + 7) / 8;
  uint8_t *bits = (uint8_t *)calloc(bw * h, 1);
  if (!bits) return false;
  for (int y = y0; y <= y1; y++)
    for (int x = x0; x <= x1; x++)
      if ((buf[(y / 8) * 128 + x] >> (y % 8)) & 1)
        for (int dy = 0; dy < sc; dy++) for (int dx = 0; dx < sc; dx++) {
          int xx = (x - x0) * sc + dx, yy = (y - y0) * sc + dy;
          bits[yy * bw + (xx >> 3)] |= 1 << (xx & 7);
        }
  L.w = w; L.h = h; L.bits = bits;
  return true;
}
static void rebuildNamed() {
  for (int i = 0; i < NAMED_N; i++) {
    phOverride[NAMED[i].id] = nullptr;
    for (int j = 0; j < 2; j++) { free((void *)namedLines[i][j].bits); namedLines[i][j].bits = nullptr; }
  }
  if (!strcmp(S.oname, TEXT_OWNER) && !strcmp(S.fname, TEXT_FRIEND)) return;   // همان تصویرهای آماده
  char t[160];
  for (int i = 0; i < NAMED_N; i++) {
    nameSubst(t, sizeof t, NAMED[i].tpl);
    char *bar = strchr(t, '|'); int cnt = 0;
    if (bar) *bar = 0;
    if (renderNamedLine(t, NAMED[i].big, namedLines[i][cnt])) cnt++;
    if (bar && renderNamedLine(bar + 1, NAMED[i].big, namedLines[i][cnt])) cnt++;
    if (!cnt) continue;
    namedPh[i].n = cnt; namedPh[i].lines = namedLines[i];
    phOverride[NAMED[i].id] = &namedPh[i];
  }
  oled.clearBuffer();
  Serial.printf("[names] %s / %s (redrawn on device)\n", S.oname, S.fname);
}
static void setNames(const char *o, const char *f) {
  if (o && *o) strlcpy(S.oname, o, sizeof S.oname);
  if (f && *f) strlcpy(S.fname, f, sizeof S.fname);
  prefs.begin("moochi", false); prefs.putString("oname", S.oname); prefs.putString("fname", S.fname); prefs.end();
  namesDirty = true;
}

// ---------------- روزهای خاص ----------------
static PhraseId specialToday() {
  int jy, jm, jd; jalaliToday(jy, jm, jd);
  if (jm == S.bm && jd == S.bd) return P_BDAY;
  if (S.anm && jm == S.anm && jd == S.anD) return P_ANNIV;
  if (jm == 1 && jd <= 2) return P_NOWRUZ;
  if (jm == 9 && jd == 30) return P_YALDA;
  if (jm == 12 && jd == 5) return P_LOVEDAY;
  return P_COUNT;
}
static PhraseId partyP = P_COUNT;
static void startParty(PhraseId p) {
  partyP = p; setMode(MODE_PARTY);
  voiceStop();
  if (p == P_BDAY) PLAY(S_BDAY); else PLAY(S_LOVE);
  voiceSay(p);
}

// ---------------- پیام‌ها ----------------
static PText msgT;
static uint32_t dumpAt = 0;
static uint8_t msgPage = 0; static uint32_t msgPageAt = 0;
static void showMessage() {
  Serial.printf("[msg] show: kind %d, %u bytes, heap %u\n", S.msgKind, (unsigned)strlen(S.msg), (unsigned)ESP.getFreeHeap());
  if (S.msgKind == 1) { msgT.n = 0; msgT.lines = 0; } else ptPrepare(msgT, S.msg);
  Serial.printf("[msg] %d glyphs, %d lines, first=%04X\n", msgT.n, msgT.lines, msgT.n ? msgT.g[0] : 0);
  // dumpAt = millis() + 1200;   // برای عیب‌یابی: تصویر صفحه در Serial
  msgPage = 0; msgPageAt = millis();
  setMode(MODE_MSG);
}
static void markRead(bool byOwner = true) {
  if (!S.unread) return;
  Serial.println("[msg] mark read");
  S.unread = false;
  prefs.begin("moochi", false); prefs.putBool("unread", false); prefs.end();
  if (byOwner) { char nb[96]; snprintf(nb, sizeof nb, "💌 %s پیامت رو دید", S.oname); notifyFriend(1, nb, 0); loveAdd(5); }
}
static bool isHeartMsg(const char *t) {
  String s(t); s.trim();
  return s == "<3" || s == "❤" || s == "❤️" || s == "♥" || s == "💖" || s == "💗" || s == "😘";
}
static void wakeQuiet() { baseMood = M_NEUTRAL; sleepyAnnounced = false; lastActive = millis(); }
static void onIncoming(const char *text) {
  if (!text || !*text) return;
  if (mode == MODE_OTA) return;
  if (mode == MODE_APP || mode == MODE_MENU) setMode(MODE_FACE);
  wakeQuiet();
  if (isHeartMsg(text)) {
    if (mode != MODE_ALARM) setMode(MODE_FACE);
    feel(M_LOVE, 4500); PLAY(S_LOVE); say(P_THINK, 4000);
    return;
  }
  strlcpy(S.msg, text, sizeof S.msg);
  S.unread = true; S.msgKind = 0;
  prefs.begin("moochi", false); prefs.putString("msg", S.msg); prefs.putBool("unread", true); prefs.putUChar("mkind", 0); prefs.end();
  if (mode == MODE_ALARM) return;
  PLAY(S_MSG); voiceSay(P_FROM);
  showMessage();
  nextNewMsg = millis() + 10UL * 60000;
}

// ---------------- نقاشی (۱۲۸×۴۸ ، ۷۶۸ بایت) ----------------
static int b64v(char c) {
  if (c >= 'A' && c <= 'Z') return c - 'A'; if (c >= 'a' && c <= 'z') return c - 'a' + 26;
  if (c >= '0' && c <= '9') return c - '0' + 52; if (c == '+' || c == '-') return 62; if (c == '/' || c == '_') return 63; return -1;
}
static size_t b64dec(const char *s, uint8_t *out, size_t max) {
  uint32_t acc = 0; int bits = 0; size_t n = 0;
  for (; *s && n < max; s++) { int v = b64v(*s); if (v < 0) continue; acc = (acc << 6) | v; bits += 6; if (bits >= 8) { bits -= 8; out[n++] = (acc >> bits) & 0xFF; } }
  return n;
}
static void onDoodle(const uint8_t *bits) {
  memcpy(doodle, bits, sizeof doodle);
  strlcpy(S.msg, "🎨", sizeof S.msg); S.msgKind = 1; S.unread = true;
  prefs.begin("moochi", false); prefs.putBytes("doodle", doodle, sizeof doodle); prefs.putString("msg", S.msg);
  prefs.putUChar("mkind", 1); prefs.putBool("unread", true); prefs.end();
  if (mode == MODE_APP || mode == MODE_MENU) setMode(MODE_FACE);
  wakeQuiet();
  if (mode == MODE_ALARM) return;
  PLAY(S_MSG); voiceSay(P_FROM);
  showMessage();
  nextNewMsg = millis() + 10UL * 60000;
}

// ---------------- یادداشت روی صفحه (یادآور ، فال ، ...) ----------------
static PText noteT; static uint8_t notePage = 0, noteIcon = 0; static uint32_t notePageAt = 0, noteFor = 20000;
static void showNote(const char *text, uint8_t icon, uint32_t ms) {
  ptPrepare(noteT, text); notePage = 0; notePageAt = millis(); noteIcon = icon; noteFor = ms;
  setMode(MODE_NOTE);
}
// اعداد فارسی
static void faNum(char *out, size_t n, const char *in) {
  size_t k = 0;
  for (; *in && k + 3 < n; in++) {
    if (*in >= '0' && *in <= '9') { out[k++] = 0xDB; out[k++] = 0xB0 + (*in - '0'); }
    else out[k++] = *in;
  }
  out[k] = 0;
}

// روز شمار میلادی (برای شمارش معکوس دیدار)
static int32_t civDays(int y, int m, int d) {
  y -= m <= 2; int era = (y >= 0 ? y : y - 399) / 400; unsigned yoe = y - era * 400;
  unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1; unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097 + (int32_t)doe - 719468;
}
static int meetDays() {
  if (!S.meet) return -1;
  return civDays(S.meet / 10000, S.meet / 100 % 100, S.meet % 100) - civDays(now_dt.y, now_dt.mo, now_dt.d);
}
static bool friendTime(int &h, int &m) {          // ساعت دوست (فقط اگر موچی در منطقه‌ي زمانی دیگری است)
  if (!sysTimeOk || !strcmp(S.tz, TZ_DEFAULT)) return false;
  time_t t = time(nullptr) + FRIEND_UTC_OFFSET; struct tm tm; gmtime_r(&t, &tm);
  h = tm.tm_hour; m = tm.tm_min; return true;
}
static const char *const MOOD_FA[M_COUNT] = {"آروم", "خوشحال", "خندون", "عاشق", "متعجب", "عصبانی", "غمگین",
  "خواب‌آلود", "خواب", "کلافه", "کنجکاو", "گیج", "شیطون", "قهر"};

// «امروز حالت چطوره؟»
static PText askT; static uint32_t askRetryAt = 0;
static const char *const FEEL_FA[3] = {"بد 😢", "معمولی 😐", "خوب 😊"};
static void startAsk() {
  voiceStop(); wakeQuiet();
  ptPrepare(askT, "امروز حالت چطوره؟");
  setMode(MODE_ASK); PLAY(S_DING);
}

// ===========================================================================
//  منو و بازی‌ها
// ===========================================================================
enum App : uint8_t { A_8BALL, A_DICE, A_REACT, A_SIMON, A_TIMER, A_NIGHT, A_LOVE, A_FAL, A_MUSIC, A_REPLY, A_EXIT };

// جعبه‌ي موسیقی (آهنگ‌های قدیمی و آزاد)
#define N_(f, d) {f, d}, {0, 25}
static const Note SONG_TWINKLE[] = {N_(523,330),N_(523,330),N_(784,330),N_(784,330),N_(880,330),N_(880,330),N_(784,660),
  N_(698,330),N_(698,330),N_(659,330),N_(659,330),N_(587,330),N_(587,330),N_(523,660),
  N_(784,330),N_(784,330),N_(698,330),N_(698,330),N_(659,330),N_(659,330),N_(587,660),
  N_(784,330),N_(784,330),N_(698,330),N_(698,330),N_(659,330),N_(659,330),N_(587,660),
  N_(523,330),N_(523,330),N_(784,330),N_(784,330),N_(880,330),N_(880,330),N_(784,660),
  N_(698,330),N_(698,330),N_(659,330),N_(659,330),N_(587,330),N_(587,330),N_(523,800)};
static const Note SONG_JOY[] = {N_(659,300),N_(659,300),N_(698,300),N_(784,300),N_(784,300),N_(698,300),N_(659,300),N_(587,300),
  N_(523,300),N_(523,300),N_(587,300),N_(659,300),N_(659,450),N_(587,150),N_(587,600),
  N_(659,300),N_(659,300),N_(698,300),N_(784,300),N_(784,300),N_(698,300),N_(659,300),N_(587,300),
  N_(523,300),N_(523,300),N_(587,300),N_(659,300),N_(587,450),N_(523,150),N_(523,700)};
static const Note SONG_LULLABY[] = {N_(659,250),N_(659,250),N_(784,750),N_(659,250),N_(659,250),N_(784,750),
  N_(659,250),N_(784,250),N_(1047,500),N_(988,500),N_(880,500),N_(880,500),N_(784,750),
  N_(587,250),N_(659,250),N_(698,500),N_(587,500),N_(587,250),N_(659,250),N_(698,750),
  N_(587,250),N_(698,250),N_(988,250),N_(880,250),N_(784,500),N_(988,500),N_(1047,1000)};
static const Note SONG_ELISE[] = {N_(659,180),N_(622,180),N_(659,180),N_(622,180),N_(659,180),N_(494,180),N_(587,180),N_(523,180),N_(440,450),
  N_(262,180),N_(330,180),N_(440,180),N_(494,450),N_(330,180),N_(415,180),N_(494,180),N_(523,450),
  N_(330,180),N_(659,180),N_(622,180),N_(659,180),N_(622,180),N_(659,180),N_(494,180),N_(587,180),N_(523,180),N_(440,450),
  N_(262,180),N_(330,180),N_(440,180),N_(494,450),N_(330,180),N_(523,180),N_(494,180),N_(440,700)};
static const Note SONG_JINGLE[] = {N_(659,250),N_(659,250),N_(659,500),N_(659,250),N_(659,250),N_(659,500),
  N_(659,250),N_(784,250),N_(523,350),N_(587,120),N_(659,900),
  N_(698,250),N_(698,250),N_(698,350),N_(698,120),N_(698,250),N_(659,250),N_(659,250),N_(659,120),N_(659,120),
  N_(659,250),N_(587,250),N_(587,250),N_(659,250),N_(587,500),N_(784,600)};
struct Song { const char *name; const Note *n; uint8_t len; };
static const Song SONGS[] = {
  {"ستاره‌ی کوچولو", SONG_TWINKLE, sizeof(SONG_TWINKLE) / sizeof(Note)},
  {"لالایی", SONG_LULLABY, sizeof(SONG_LULLABY) / sizeof(Note)},
  {"سرود شادی", SONG_JOY, sizeof(SONG_JOY) / sizeof(Note)},
  {"برای الیزه", SONG_ELISE, sizeof(SONG_ELISE) / sizeof(Note)},
  {"زنگوله‌ها", SONG_JINGLE, sizeof(SONG_JINGLE) / sizeof(Note)},
  {"تولدت مبارک", S_BDAY, sizeof(S_BDAY) / sizeof(Note)},
};
#define N_SONGS (sizeof(SONGS) / sizeof(SONGS[0]))

// جواب‌های آماده برای دوست (FRIEND_NAME)
static const char *const REPLIES[] = {"منم دوستت دارم ❤", "دلم برات تنگ شده", "بغل 🤗", "زنگ بزن 📞", "شب بخیر 🌙", "صبح بخیر ☀", "بوس 😘"};
#define N_REPLIES (sizeof(REPLIES) / sizeof(REPLIES[0]))
static PText appT;
static uint8_t menuIdx = 0;
static App app = A_8BALL;
static uint8_t aState = 0; static uint32_t aT = 0, aT2 = 0; static int aVal = 0, aVal2 = 0;
static uint8_t simonSeq[40]; static uint8_t simonLen = 0, simonPos = 0; static int simonLit = -1;
static PhraseId aPhrase = P_COUNT;
static const uint16_t SIMON_F[3] = {440, 554, 659};

static void openMenu() { setMode(MODE_MENU); menuIdx = 0; PLAY(S_CLICK); voiceSay((PhraseId)G_MENU[0]); }
static void openApp(uint8_t a) {
  if (a >= A_EXIT) { setMode(MODE_FACE); feel(M_HAPPY, 800); return; }
  app = (App)a; aState = 0; aT = aT2 = millis(); aVal = aVal2 = 0; aPhrase = P_COUNT;
  setMode(MODE_APP);
  switch (app) {
    case A_8BALL: voiceSay(P_ASK); break;
    case A_DICE:  aVal = 1 + random(6); voiceSay(P_SHAKEME); break;
    case A_REACT: voiceSay(P_RHINT); break;
    case A_SIMON: simonLen = 0; aState = 0; break;
    case A_TIMER: aVal = 5; break;                       // دقیقه
    case A_NIGHT: aVal = 3; break;                       // نور ۱..۵
    case A_LOVE:  break;
    case A_FAL:   ptPrepare(appT, "نیت کن و\nتاچ بالا رو بزن"); break;
    case A_MUSIC: aVal = 0; ptPrepare(appT, SONGS[0].name); break;
    case A_REPLY: aVal = 0; ptPrepare(appT, REPLIES[0]); break;
    default: break;
  }
}
static void exitApp() { setMode(MODE_FACE); soundStop(); feel(M_HAPPY, 900); PLAY(S_CLICK); }

static void simonNext() {
  simonSeq[simonLen++] = random(3);
  simonPos = 0; aState = 1; aT = millis() + (simonLen == 1 ? 1700 : 700); aVal = 0;   // نشان دادن
}
static void simonPress(uint8_t pad) {
  uint32_t now = millis();
  if (aState != 2) return;
  simonLit = pad; aT2 = now + 250; beep(SIMON_F[pad], 200);
  if (simonSeq[simonPos] != pad) {
    aState = 3; aT = now;
    soundStop(); PLAY(S_LOSE); voiceSay(P_LOSE);
    uint8_t score = simonLen - 1;
    if (score > S.simonBest) { S.simonBest = score; saveU8("sbest", score); aVal2 = 1; }
    return;
  }
  if (++simonPos >= simonLen) { aState = 4; aT = now + 700; }    // مرحله‌ی بعد
}

static void appEvent(Ev e) {
  uint32_t now = millis();
  if (e == EV_TOP_LONG) { exitApp(); return; }
  switch (app) {
    case A_8BALL:
      if (e == EV_SHAKE || e == EV_BUMP || e == EV_TOP_TAP) { aState = 1; aT = now; PLAY(S_DIZZY); aPhrase = pick(G_ANSWER, G_ANSWER_N); }
      break;
    case A_DICE:
      if (e == EV_SHAKE || e == EV_BUMP || e == EV_TOP_TAP || e == EV_LEFT_TAP || e == EV_RIGHT_TAP) { aState = 1; aT = now; aT2 = 0; }
      break;
    case A_REACT:
      if (e != EV_TOP_TAP && e != EV_LEFT_TAP && e != EV_RIGHT_TAP && e != EV_HUG) break;
      if (aState == 1) { aState = 4; aT = now; PLAY(S_ANNOY); voiceSay(P_EARLY); }        // زود زد
      else if (aState == 2) {
        aVal = now - aT2; aState = 3; aT = now; aVal2 = 0;
        if (!S.reactBest || aVal < S.reactBest) { S.reactBest = aVal; prefs.begin("moochi", false); prefs.putUShort("rbest", aVal); prefs.end(); aVal2 = 1; PLAY(S_WIN); voiceSay(P_RECORD); }
        else PLAY(S_HAPPY);
      } else if (aState == 3 || aState == 4) { aState = 1; aT = now; aT2 = now + random(1500, 4500); }
      else if (aState == 0) { aState = 1; aT = now; aT2 = now + random(1500, 4500); }
      break;
    case A_SIMON:
      if (aState == 0 || aState == 3) { if (e == EV_TOP_TAP || e == EV_LEFT_TAP || e == EV_RIGHT_TAP) { simonLen = 0; aVal2 = 0; simonNext(); voiceSay(P_WATCH); } break; }
      if (e == EV_LEFT_TAP) simonPress(0); else if (e == EV_TOP_TAP) simonPress(1); else if (e == EV_RIGHT_TAP) simonPress(2);
      break;
    case A_TIMER:
      if (aState == 0) {                                  // تنظیم
        if (e == EV_LEFT_TAP && aVal > 1) { aVal--; beep(1500, 25); }
        if (e == EV_RIGHT_TAP && aVal < 99) { aVal++; beep(1800, 25); }
        if (e == EV_SIDE_LONG) { aVal = (aVal == 25) ? 5 : 25; beep(2000, 40); }
        if (e == EV_TOP_TAP) { aState = 1; aT = now; aT2 = now + aVal * 60000UL; PLAY(S_CLOCK); }
      } else if (aState == 1) { if (e == EV_TOP_TAP) { aState = 2; aT = aT2 - now; PLAY(S_CLICK); } }        // مکث
      else if (aState == 2) { if (e == EV_TOP_TAP) { aState = 1; aT2 = now + aT; PLAY(S_CLICK); } }
      else if (aState == 3) { aState = 0; soundStop(); voiceStop(); }
      break;
    case A_NIGHT:
      if (e == EV_LEFT_TAP && aVal > 1) aVal--;
      if (e == EV_RIGHT_TAP && aVal < 5) aVal++;
      if (e == EV_TOP_TAP) exitApp();
      break;
    case A_LOVE:
      if (e == EV_SIDE_LONG || e == EV_HUG) { loveAdd(5); PLAY(S_PURR); }
      if (e == EV_TOP_TAP) exitApp();
      break;
    case A_FAL:
      if ((e == EV_TOP_TAP || e == EV_SHAKE || e == EV_BUMP) && aState != 1) { aState = 1; aT = now; PLAY(S_DIZZY); }
      else if (aState == 2 && (e == EV_LEFT_TAP || e == EV_RIGHT_TAP)) { int pg = (appT.lines + 2) / 3; aVal2 = (aVal2 + 1) % (pg ? pg : 1); aT2 = now; }
      break;
    case A_MUSIC:
      if (e == EV_LEFT_TAP || e == EV_RIGHT_TAP) {
        aVal = (aVal + (e == EV_RIGHT_TAP ? 1 : N_SONGS - 1)) % N_SONGS; soundStop(); aState = 0;
        ptPrepare(appT, SONGS[aVal].name); beep(1800, 25);
      }
      if (e == EV_TOP_TAP) { if (aState == 1) { soundStop(); aState = 0; } else { voiceStop(); play(SONGS[aVal].n, SONGS[aVal].len); aState = 1; } }
      break;
    case A_REPLY:
      if (aState == 0 && (e == EV_LEFT_TAP || e == EV_RIGHT_TAP)) {
        aVal = (aVal + (e == EV_RIGHT_TAP ? 1 : N_REPLIES - 1)) % N_REPLIES; ptPrepare(appT, REPLIES[aVal]); beep(1800, 25);
      }
      if (aState == 0 && e == EV_TOP_TAP) {
        char b[120]; snprintf(b, sizeof b, "💬 %s: %s", S.oname, REPLIES[aVal]);
        bool ok = S.botTok[0] && S.owner;
        notifyFriend(3, b, 0);
        ptPrepare(appT, ok ? "فرستاده شد ✓" : "بات تنظیم نشده!");
        aState = 1; aT = now; if (ok) PLAY(S_LOVE); else PLAY(S_SAD); loveAdd(2);
      }
      break;
    default: break;
  }
}
static void appTick(uint32_t now) {
  switch (app) {
    case A_8BALL:
      if (aState == 1 && (int32_t)(now - aT) > 1300) { aState = 2; voiceSay(aPhrase); }
      break;
    case A_DICE:
      if (aState == 1) {
        if (now >= aT2) { aVal = 1 + random(6); aT2 = now + 90 + (now - aT) / 6; beep(2400, 12); }
        if ((int32_t)(now - aT) > 1200) { aState = 2; PLAY(S_HAPPY); }
      }
      break;
    case A_REACT:
      if (aState == 0 && (int32_t)(now - aT) > 2600) { aState = 1; aT = now; aT2 = now + random(1500, 4500); }
      if (aState == 1 && now >= aT2) { aState = 2; aT2 = now; beep(1760, 60); }
      break;
    case A_SIMON:
      if (simonLit >= 0 && aState == 2 && now > aT2) simonLit = -1;
      if (aState == 1 && now >= aT) {                     // پخش دنباله
        int idx = aVal / 2;
        if (idx >= simonLen) { aState = 2; simonPos = 0; simonLit = -1; break; }
        if ((aVal & 1) == 0) { simonLit = simonSeq[idx]; beep(SIMON_F[simonLit], 380); aT = now + 420; }
        else { simonLit = -1; aT = now + 160; }
        aVal++;
      }
      if (aState == 4 && now >= aT) { PLAY(S_HAPPY); simonNext(); }
      break;
    case A_TIMER:
      if (aState == 1 && (int32_t)(now - aT2) >= 0) { aState = 3; aT = now; voiceSay(P_TDONE); }
      if (aState == 3) {
        if (!sndActive && !vPlaying && vPending == P_COUNT) PLAY(S_ALARM);
        if ((int32_t)(now - aT) > 60000) { aState = 0; soundStop(); }
      }
      break;
    case A_NIGHT:
      if ((int32_t)(now - modeAt) > (int32_t)(2UL * 3600000)) exitApp();
      break;
    case A_FAL:
      if (aState == 1 && (int32_t)(now - aT) > 1500) {
        aState = 2; aVal = random(HAFEZ_N); aVal2 = 0; aT2 = now;
        ptPrepare(appT, HAFEZ[aVal]); PLAY(S_CHIME);
        char b[240]; snprintf(b, sizeof b, "📜 فال %s:\n%s", S.oname, HAFEZ[aVal]); notifyFriend(2, b, 60000);
      }
      if (aState == 2) { int pg = (appT.lines + 2) / 3; if (pg > 1 && (int32_t)(now - aT2) > 4500) { aVal2 = (aVal2 + 1) % pg; aT2 = now; } }
      break;
    case A_MUSIC:
      if (aState == 1 && !sndActive) aState = 0;
      break;
    case A_REPLY:
      if (aState == 1 && (int32_t)(now - aT) > 1800) { exitApp(); feel(M_LOVE, 2000); }
      break;
    default: break;
  }
}
static void drawApp(uint32_t now) {
  switch (app) {
    case A_8BALL: {
      if (aState == 0) { EyeP e = moodTarget(M_CURIOUS); drawFaceWithText(M_CURIOUS, e, 0, now, P_ASK); }
      else if (aState == 1) { EyeP e = moodTarget(M_DIZZY); drawFace(M_DIZZY, e, 0, 32, 1, now); }
      else { EyeP e = moodTarget(M_HAPPY); drawFaceWithText(M_HAPPY, e, 0, now, aPhrase); }
      break;
    }
    case A_DICE: {
      int jx = (aState == 1) ? random(-3, 4) : 0, jy = (aState == 1) ? random(-3, 4) : 0;
      drawDie(aVal, 64 + jx, 26 + jy, 40);
      if (aState == 0) drawTxtCenter(line0(P_SHAKEME), 64 - line0(P_SHAKEME).h);
      break;
    }
    case A_REACT: {
      if (aState == 0) drawPhraseAt(P_RHINT, 10);
      else if (aState == 1) { EyeP e = moodTarget(M_NEUTRAL); e.lookX = sinf(now * 0.004f) * 10; drawFace(M_NEUTRAL, e, 0, 32, 1, now); }
      else if (aState == 2) drawHeart(64, 32, 50);
      else if (aState == 3) {
        int w = smallNumWidth(aVal); const TxtLine &ms = line0(P_MSEC);
        int all = w + 6 + ms.w, xr = 64 + all / 2;
        drawSmallNum(aVal, xr - w, 8); drawTxt(ms, xr - w - 6 - ms.w, 6);
        if (aVal2) drawTxtCenter(line0(P_RECORD), 30);
        else { drawHeart(30, 44, 10); drawSmallNum(S.reactBest, 40, 36); }
      } else drawPhraseAt(P_EARLY, 20);
      break;
    }
    case A_SIMON: {
      drawSimonPads(simonLit);
      const TxtLine &lv = line0(P_LEVEL);
      int sc = simonLen > 0 ? simonLen - 1 : 0;
      if (aState == 0) drawTxtCenter(line0(P_SHINT), 44);
      else if (aState == 3) { drawTxtCenter(line0(P_LOSE), 26); int w = smallNumWidth(simonLen - 1); drawSmallNum(simonLen - 1, 64 - w / 2, 46); }
      else { int w = smallNumWidth(sc); drawTxt(lv, 64 + 2, 64 - lv.h); drawSmallNum(sc, 64 - 4 - w, 64 - lv.h + 1); }
      break;
    }
    case A_TIMER: {
      uint32_t left = (aState == 1) ? (uint32_t)max<int32_t>(0, (int32_t)(aT2 - now)) : (aState == 2 ? aT : aVal * 60000UL);
      if (aState == 3) left = 0;
      uint32_t s = (left + 999) / 1000;
      bool show = !(aState == 2 && (now / 400) % 2) && !(aState == 3 && (now / 300) % 2);
      if (show) {
        int mm = s / 60, ss = s % 60;
        drawClockTime(mm > 99 ? 99 : mm, ss, 0, 0, false);
      }
      if (aState == 0) drawTxtCenter(line0(P_MN_TIMER), 40);
      else if (aState == 3) drawTxtCenter(line0(P_TDONE), 40);
      else drawBar(14, 44, 100, 9, 1.0f - (float)left / (aVal * 60000.0f));
      break;
    }
    case A_NIGHT: {
      drawMoon(100, 16, 10);
      EyeP e = moodTarget(M_SLEEP); drawFace(M_SLEEP, e, 0, 40, 0.8f, now);
      for (int i = 0; i < aVal; i++) oled.drawDisc(10 + i * 8, 58, 2);
      break;
    }
    case A_FAL: {
      if (aState == 0) { drawMenuIcon(7, 64, 14, now); ptDrawLines(appT, 0, 2, 30, 17); }
      else if (aState == 1) { drawMenuIcon(7, 64 + (int)(sinf(now * 0.03f) * 6), 30, now); }
      else ptDrawLines(appT, aVal2 * 3, 3, 4, 20);
      break;
    }
    case A_MUSIC: {
      drawMenuIcon(8, 64, 16, now);
      if (aState == 1) for (int i = 0; i < 3; i++) { int ph = (now / 12 + i * 40) % 120; int x = 20 + i * 44, y = 34 - ph / 4; oled.drawDisc(x, y, 3); oled.drawVLine(x + 3, y - 10, 10); }
      ptDrawLines(appT, 0, 1, 42, 17);
      oled.drawTriangle(4, 50, 10, 45, 10, 55); oled.drawTriangle(124, 50, 118, 45, 118, 55);
      break;
    }
    case A_REPLY: {
      drawMenuIcon(9, 64, 14, now);
      ptDrawLines(appT, 0, 2, 30, 17);
      if (aState == 0) { oled.drawTriangle(4, 40, 10, 35, 10, 45); oled.drawTriangle(124, 40, 118, 35, 118, 45); }
      break;
    }
    case A_LOVE: {
      { char b[40], f[60]; snprintf(b, sizeof b, "%u", (unsigned)S.xp); faNum(f, sizeof f, b); ptPrepare(appT, f); ptDrawLines(appT, 0, 1, 50, 17); }
      drawTxtCenter(line0(P_MN_LOVE), 0);
      float beat = 1 + 0.1f * fmaxf(0, sinf(now * 0.012f));
      drawHeart(20, 40, 22 * beat);
      drawBar(38, 34, 70, 12, S.love / 100.0f);
      drawSmallNum(S.love, 112, 33);
      break;
    }
    default: break;
  }
}

// ===========================================================================
//  زنگ ساعت
// ===========================================================================
static int alarmKey = -1; static uint32_t snoozeAt = 0, alarmVoiceAt = 0;
static void startAlarm() {
  voiceStop(); soundStop();
  setMode(MODE_ALARM); wakeQuiet();
  alarmVoiceAt = millis() + 2500;
}
static void stopAlarm(bool snooze) {
  soundStop(); voiceStop();
  setMode(MODE_FACE);
  if (snooze) { snoozeAt = millis() + 5UL * 60000; baseMood = M_SLEEPY; say(P_SNOOZE, 2500); }
  else { snoozeAt = 0; feel(M_HAPPY, 3000); say(P_MORNING, 3000); greetedDay = now_dt.d; }
}
static void alarmCheck(uint32_t now) {
  if (mode == MODE_ALARM || mode == MODE_OTA || mode == MODE_BOOT) return;
  if (snoozeAt && (int32_t)(now - snoozeAt) >= 0) { snoozeAt = 0; startAlarm(); return; }
  if (!S.aon) return;
  int wd = persianWeekday(now_dt.y, now_dt.mo, now_dt.d);
  int key = now_dt.d * 1440 + now_dt.h * 60 + now_dt.mi;
  if (now_dt.h == S.ah && now_dt.mi == S.am && (S.adays >> wd & 1) && key != alarmKey) { alarmKey = key; startAlarm(); }
}

// ===========================================================================
//  واکنش به رویدادها
// ===========================================================================
static void wakeUp(uint32_t now) {
  bool wasAsleep = (baseMood == M_SLEEP || baseMood == M_SLEEPY);
  baseMood = M_NEUTRAL; sleepyAnnounced = false;
  if (wasAsleep) {
    feel(M_SURPRISED, 900); PLAY(S_WAKE);
    if (now_dt.h >= 5 && now_dt.h < 12 && greetedDay != now_dt.d) { greetedDay = now_dt.d; say(P_MORNING, 2600); feel(M_HAPPY, 2600); }
    else say(P_WAKE, 1600);
  }
}

static void handle(Ev e, uint32_t now) {
  uint32_t idleBefore = now - lastActive;
  lastActive = now;
  if (mode == MODE_BOOT || mode == MODE_OTA) return;
  if (mode == MODE_DIAG) { if (e == EV_TOP_LONG) setMode(MODE_FACE); return; }

  if (e == EV_MENU && mode != MODE_ALARM) { openMenu(); return; }
  switch (mode) {
    case MODE_MENU:
      if (e == EV_RIGHT_TAP) { menuIdx = (menuIdx + 1) % G_MENU_N; PLAY(S_CLICK); voiceSay((PhraseId)G_MENU[menuIdx]); }
      else if (e == EV_LEFT_TAP) { menuIdx = (menuIdx + G_MENU_N - 1) % G_MENU_N; PLAY(S_CLICK); voiceSay((PhraseId)G_MENU[menuIdx]); }
      else if (e == EV_TOP_TAP) { PLAY(S_HAPPY); openApp(menuIdx); }
      else if (e == EV_TOP_LONG) exitApp();
      return;
    case MODE_APP: appEvent(e); return;
    case MODE_ALARM:
      if (e == EV_TOP_TAP) stopAlarm(true);
      else if (e == EV_HUG || e == EV_SIDE_LONG || e == EV_TOP_LONG || e == EV_SHAKE) stopAlarm(false);
      return;
    case MODE_MSG:
      markRead();
      if (e == EV_LEFT_TAP || e == EV_RIGHT_TAP) {
        if (msgT.lines > 3) { msgPage = (msgPage + 1) % ((msgT.lines + 2) / 3); }
        msgPageAt = now; modeAt = now;
      } else { setMode(MODE_FACE); msg = P_COUNT; feel(M_LOVE, 2500); PLAY(S_LOVE); }
      return;
    case MODE_PARTY: setMode(MODE_FACE); feel(M_LOVE, 2000); return;
    case MODE_ASK: {
      int k = (e == EV_LEFT_TAP) ? 0 : (e == EV_TOP_TAP) ? 1 : (e == EV_RIGHT_TAP) ? 2 : -1;
      if (k < 0) { if (e == EV_TOP_LONG) { setMode(MODE_FACE); askRetryAt = now + 30UL * 60000; } return; }
      S.moodDay = now_dt.d; strlcpy(S.lastMood, FEEL_FA[k], sizeof S.lastMood);
      prefs.begin("moochi", false); prefs.putUChar("moodDay", S.moodDay); prefs.putString("lastMood", S.lastMood); prefs.end();
      char b[120]; snprintf(b, sizeof b, "🌙 حال %s امروز: %s", S.oname, FEEL_FA[k]); notifyFriend(3, b, 0);
      setMode(MODE_FACE); loveAdd(2);
      if (k == 0) { feel(M_SAD, 1800); afterTmp = M_LOVE; afterTmpMs = 3500; say(P_HUG, 3500); PLAY(S_SAD); }
      else if (k == 1) { feel(M_LOVE, 3000); say(pick(G_LOVE, G_LOVE_N), 3000); PLAY(S_LOVE); }
      else { feel(M_LAUGH, 2500); afterTmp = M_LOVE; afterTmpMs = 2000; PLAY(S_HAPPY); }
      return;
    }
    case MODE_NOTE:
      if ((e == EV_LEFT_TAP || e == EV_RIGHT_TAP) && noteT.lines > 3) { notePage = (notePage + 1) % ((noteT.lines + 2) / 3); notePageAt = now; modeAt = now; }
      else { setMode(MODE_FACE); soundStop(); feel(M_HAPPY, 1500); PLAY(S_CLICK); }
      return;
    default: break;
  }

  bool wasAsleep = (baseMood == M_SLEEP);
  // روز خاص: اولین لمس روز = جشن!
  PhraseId sp = specialToday();
  if (sp != P_COUNT && partyDay != now_dt.d && now_dt.h >= 7) { partyDay = now_dt.d; wakeQuiet(); startParty(sp); loveAdd(5); return; }
  if (wasAsleep && e != EV_SHAKE) { wakeUp(now); return; }
  wakeUp(now);
  // دلتنگی: ۳ ساعت کسی سراغش نیامده
  if (idleBefore > 3UL * 3600000 && !isNight() && (e == EV_TOP_TAP || e == EV_LEFT_TAP || e == EV_RIGHT_TAP || e == EV_HUG || e == EV_TOP_LONG || e == EV_SIDE_LONG)) {
    setMode(MODE_FACE); feel(M_SAD, 2600); say(P_MISSED, 3200); PLAY(S_SAD); afterTmp = M_LOVE; afterTmpMs = 2000; return;
  }

  switch (e) {
    case EV_TOP_TAP:
      if (mode == MODE_CLOCK) { setMode(MODE_FACE); feel(M_HAPPY, 800); PLAY(S_CLICK); }
      else { setMode(MODE_CLOCK); clockTempUntil = 0; PLAY(S_CLOCK); }
      break;
    case EV_TOP_DOUBLE:
      setMode(MODE_FACE); feel(M_LOVE, 3200); say(pick(G_LOVE, G_LOVE_N), 3200); PLAY(S_LOVE); loveAdd(1);
      break;
    case EV_TOP_LONG:
    case EV_SIDE_LONG:
      if (S.unread) { showMessage(); markRead(); break; }   // پیام نخوانده اولویت دارد
      setMode(MODE_FACE);
      if (tmpMood == M_SULK && now < tmpUntil) { feel(M_HAPPY, 2500); say(P_SORRY, 2500); PLAY(S_HAPPY); angerLevel = 0; }
      else { feel(M_HAPPY, 2800); say(pick(G_PET, G_PET_N), 2400); PLAY(S_PURR); }
      loveAdd(4);
      break;
    case EV_LEFT_TAP:
    case EV_RIGHT_TAP:
      if (S.unread || (mode == MODE_CLOCK && S.msg[0])) { showMessage(); markRead(); break; }
      if (mode == MODE_CLOCK) setMode(MODE_FACE);
      if (tmpMood == M_SULK && now < tmpUntil) { lookTX = (e == EV_LEFT_TAP) ? 10 : -10; PLAY(S_ANNOY); break; }
      lookTX = (e == EV_LEFT_TAP) ? -12 : 12; lookTY = -2; nextLook = now + 1800;
      if (random(100) < 55) { feel(M_LAUGH, 1500); PLAY(S_GIGGLE); if (random(2)) say(pick(G_TICKLE, G_TICKLE_N), 1500); }
      else { feel(random(2) ? M_CURIOUS : M_WINK, 1200); PLAY(S_CLICK); }
      loveAdd(1);
      break;
    case EV_HUG:
      setMode(MODE_FACE); angerLevel = 0;
      feel(M_LOVE, 3500); say(random(2) ? P_HUG : pick(G_LOVE, G_LOVE_N), 3000); PLAY(S_LOVE);
      loveAdd(6);
      { char nb[96]; snprintf(nb, sizeof nb, "🤗 %s موچی رو بغل کرد", S.oname); notifyFriend(0, nb, 10UL * 60000); }
      break;
    case EV_ANNOY:
      setMode(MODE_FACE); feel(M_ANNOYED, 2500); say(pick(G_ANNOY, G_ANNOY_N), 2200); PLAY(S_ANNOY);
      break;
    case EV_SHAKE: {
      setMode(MODE_FACE); baseMood = M_NEUTRAL;
      if (now - angerAt > 25000) angerLevel = 0;
      angerLevel++; angerAt = now;
      int idx; do { idx = random(G_ANGRY_N); } while (idx == lastAngryIdx && G_ANGRY_N > 1);
      lastAngryIdx = idx;
      feel(M_ANGRY, 3200); say((PhraseId)G_ANGRY[idx], 3000); PLAY(S_ANGRY);
      if (angerLevel >= 3) { afterTmp = M_DIZZY; afterTmpMs = 2500; }
      else if (angerLevel >= 2) { afterTmp = M_SULK; afterTmpMs = 7000; }
      loveAdd(-3);
      break;
    }
    case EV_BUMP:
      if (mode == MODE_CLOCK) break;
      feel(M_SURPRISED, 1400); PLAY(S_SURP);
      if (random(100) < 60) say(pick(G_SURPRISE, G_SURPRISE_N), 1400);
      break;
    case EV_TILT: setMode(MODE_FACE); feel(M_DIZZY, 2600); say(P_TILT, 2400); PLAY(S_DIZZY); break;
    case EV_UNTILT: feel(M_HAPPY, 1200); PLAY(S_HAPPY); break;
    default: break;
  }
}

static uint32_t sneezeAt = 0;
static void brainTick(uint32_t now) {
  if (tmpUntil && now >= tmpUntil && afterTmp != M_COUNT) {
    Mood m = afterTmp; afterTmp = M_COUNT;
    tmpMood = m; tmpUntil = now + afterTmpMs;
    if (m == M_SULK) { say(P_SULK, 2500); PLAY(S_SAD); }
    if (m == M_DIZZY) { say(P_ANG5, 2200); PLAY(S_DIZZY); afterTmp = M_SULK; afterTmpMs = 7000; }
  }
  if (msg != P_COUNT && now >= msgUntil) msg = P_COUNT;

  bool night = isNight();
  uint32_t idle = now - lastActive;
  uint32_t tSleepy = night ? IDLE_SLEEPY_MS / 3 : IDLE_SLEEPY_MS;
  uint32_t tSleep  = night ? IDLE_SLEEP_MS / 4 : IDLE_SLEEP_MS;
  if (mode == MODE_FACE && now >= tmpUntil) {
    if (idle > tSleep && baseMood != M_SLEEP) { baseMood = M_SLEEP; PLAY(S_YAWN); if (night) say(P_NIGHT, 2200); }
    else if (idle > tSleepy && baseMood == M_NEUTRAL) {
      baseMood = M_SLEEPY;
      if (!sleepyAnnounced) { sleepyAnnounced = true; say(P_SLEEPY, 2200); PLAY(S_YAWN); }
    }
  }
  // برنامه‌ها/پیام/منو خودشان بعد از مدتی بسته شوند
  if (mode == MODE_MSG) {
    uint32_t pages = (msgT.lines + 2) / 3;
    if (pages > 1 && (int32_t)(now - msgPageAt) > 3500) { msgPage = (msgPage + 1) % pages; msgPageAt = now; }
    if ((int32_t)(now - modeAt) > (int32_t)(8000 + pages * 4000)) { setMode(MODE_FACE); msg = P_COUNT; }
  }
  if (mode == MODE_MENU && (int32_t)(now - modeAt) > 60000 && idle > 30000) setMode(MODE_FACE);
  if (mode == MODE_APP && app != A_NIGHT && app != A_TIMER && idle > 5UL * 60000) exitApp();
  if (mode == MODE_PARTY && (int32_t)(now - modeAt) > 9000) { setMode(MODE_FACE); feel(M_HAPPY, 1500); }
  if (mode == MODE_ALARM) {
    if (!sndActive && !vPlaying && vPending == P_COUNT) PLAY(S_ALARM);
    if (now >= alarmVoiceAt) { alarmVoiceAt = now + 9000; voiceSay(P_ALARM); }
    if ((int32_t)(now - modeAt) > (int32_t)(10UL * 60000)) stopAlarm(false);
  }
  if (mode == MODE_APP) appTick(now);
  if (mode == MODE_ASK && (int32_t)(now - modeAt) > 120000) { setMode(MODE_FACE); askRetryAt = now + 30UL * 60000; }
  if (mode == MODE_NOTE) {
    uint32_t pages = (noteT.lines + 2) / 3;
    if (pages > 1 && (int32_t)(now - notePageAt) > 4000) { notePage = (notePage + 1) % pages; notePageAt = now; }
    if (noteIcon == 0 && (int32_t)(now - modeAt) < 40000 && ((now - modeAt) / 10000) != ((now - modeAt - 40) / 10000) && !sndActive) PLAY(S_DING);
    if ((int32_t)(now - modeAt) > (int32_t)noteFor) setMode(MODE_FACE);
  }
  // یادآورها: هر خط «HH:MM متن»
  static int remKey = -1;
  int rk = now_dt.h * 60 + now_dt.mi;
  if (rk != remKey) {
    remKey = rk;
    for (const char *ln = S.rem; *ln; ) {
      const char *e = strchr(ln, '\n'); size_t L = e ? (size_t)(e - ln) : strlen(ln);
      int hh, mm;
      if (L > 5 && sscanf(ln, "%d:%d", &hh, &mm) == 2 && hh == now_dt.h && mm == now_dt.mi && mode != MODE_ALARM && mode != MODE_OTA && mode != MODE_BOOT) {
        char t[160]; const char *q = strchr(ln, ' '); size_t k = 0;
        if (q && q < ln + L) { q++; k = min((size_t)(ln + L - q), sizeof t - 1); memcpy(t, q, k); }
        t[k] = 0;
        wakeQuiet(); showNote(k ? t : "یادت نره! ⏰", 0, 60000); PLAY(S_DING); lastActive = now;
        break;
      }
      ln += L; if (*ln) ln++;
    }
  }
  // شب‌ها: «امروز حالت چطوره؟»
  if (MOOD_ASK_HOUR && now_dt.h >= MOOD_ASK_HOUR && now_dt.h < MOOD_ASK_HOUR + 3 && S.moodDay != now_dt.d &&
      (mode == MODE_FACE || mode == MODE_CLOCK) && (!askRetryAt || (int32_t)(now - askRetryAt) >= 0) && (int32_t)(now - lastActive) < 10UL * 60000 && now > 60000) {
    askRetryAt = 0; startAsk();
  }

  Mood m = curMood(now);
  if (now >= nextLook && m != M_SLEEP) {
    if (random(100) < 35) { lookTX = 0; lookTY = 0; }
    else { lookTX = random(-10, 11); lookTY = random(-4, 5); }
    nextLook = now + random(1400, 4500);
    if (m == M_NEUTRAL && mode == MODE_FACE && random(100) < 6) feel(random(2) ? M_CURIOUS : M_WINK, 1200);
    else if (m == M_NEUTRAL && mode == MODE_FACE && msg == P_COUNT && random(1000) < 6) {    // عطسه!
      feel(M_SURPRISED, 750); afterTmp = M_LAUGH; afterTmpMs = 900; sneezeAt = now + 650; PLAY(S_SNEEZE);
    }
  }
  if (now >= nextBlink) {
    blinkAt = now;
    nextBlink = now + ((m == M_SLEEPY) ? random(1200, 2500) : random(2200, 6000));
    if (random(100) < 15) nextBlink = now + 260;
  }

  bool calm = (mode == MODE_FACE && baseMood == M_NEUTRAL && now >= tmpUntil && now > 30000 && !night);
  // جمله‌ی عاشقانه‌ی تصادفی
  if (now >= nextLove) {
    nextLove = now + random(LOVE_MSG_MIN_MS, LOVE_MSG_MAX_MS);
    if (calm) { PhraseId lp = pick(G_LOVE, G_LOVE_N); feel(M_LOVE, 3000); say(lp, 3000); if (!hasVoice(lp)) babble(5, 900, 1800); }
  }
  // پیام نخوانده: یادآوری
  if (S.unread && now >= nextNewMsg && calm) { nextNewMsg = now + 10UL * 60000; PLAY(S_MSG); say(P_NEWMSG, 2500); }
  // عشق کم شده: «دلم گرفته، نازم کن»
  if (S.love < 30 && now >= nextLonely && calm && idle > 120000) { nextLonely = now + 15UL * 60000; feel(M_SAD, 4000); say(P_LONELY, 3500); PLAY(S_SAD); }
  // کم شدن آرام عشق (فقط روزها)
  if (now - loveDecayAt > 20UL * 60000) { loveDecayAt = now; if (!night && S.love > 20) loveAdd(-1); }
  if (loveDirty && now - loveSaveAt > 120000) {
    loveDirty = false; loveSaveAt = now; saveU8("love", S.love);
    if (xpDirty) { xpDirty = false; prefs.begin("moochi", false); prefs.putUShort("xp", S.xp); prefs.end(); }
  }

  // سر هر ساعت
  if (now_dt.h != lastHour) {
    if (lastHour >= 0 && HOURLY_CHIME && now_dt.h >= 8 && now_dt.h <= 22 && baseMood != M_SLEEP && (mode == MODE_FACE || mode == MODE_CLOCK)) {
      PhraseId sp = specialToday();
      if (sp != P_COUNT && now_dt.h % 3 == 0) startParty(sp);
      else { PLAY(S_CHIME); if (mode == MODE_FACE) { setMode(MODE_CLOCK); clockTempUntil = now + 6000; } }
    }
    lastHour = now_dt.h;
  }
  if (mode == MODE_CLOCK && clockTempUntil && now >= clockTempUntil) { setMode(MODE_FACE); clockTempUntil = 0; }

  static int contrast = -1;
  int want = (night || (mode == MODE_APP && app == A_NIGHT)) ? 5 : 200;
  if (want != contrast) { contrast = want; oled.setContrast(want); }
}

// ===========================================================================
//  LED قلب
// ===========================================================================
struct Heart { uint8_t r, g, b; uint16_t bpm; uint8_t style; };
static Heart heartFor(Mood m) {
  switch (m) {
    case M_ANGRY:     return {255, 0, 0, 170, 2};
    case M_LOVE:      return {255, 20, 90, 95, 0};
    case M_HAPPY: case M_LAUGH: return {255, 80, 20, 90, 0};
    case M_SURPRISED: return {255, 200, 0, 120, 0};
    case M_SAD: case M_SULK: return {0, 40, 255, 45, 1};
    case M_SLEEPY:    return {120, 0, 200, 30, 1};
    case M_SLEEP:     return {60, 0, 140, 12, 1};
    case M_DIZZY:     return {0, 255, 60, 140, 2};
    case M_ANNOYED:   return {255, 60, 0, 100, 0};
    default:          return {255, 30, 70, 62, 0};
  }
}
static void ledSet(float r, float g, float b) { led.setPixelColor(0, led.Color(r, g, b)); led.show(); }
static void ledTick(uint32_t now) {
  static uint32_t last = 0;
  if (now - last < 20) return;
  last = now;
  float mx = LED_MAX_BRIGHT / 255.0f * (isNight() ? 0.35f : 1.0f);
  if (mode == MODE_APP && app == A_NIGHT) { float k = 0.08f + aVal * 0.18f; ledSet(255 * k, 110 * k, 30 * k); return; }
  if (mode == MODE_NOTE && noteIcon == 0) { bool on = (now / 400) & 1; ledSet(0, on ? 200 * mx : 20 * mx, on ? 255 * mx : 30 * mx); return; }
  if (mode == MODE_ALARM) { bool on = (now / 250) & 1; ledSet(on ? 255 * mx : 0, on ? 200 * mx : 0, on ? 220 * mx : 0); return; }
  if (mode == MODE_PARTY) {
    float h = fmodf(now / 600.0f, 1.0f) * 6; int i = (int)h; float f = h - i, q = 1 - f;
    float r[6] = {1, q, 0, 0, f, 1}, g[6] = {f, 1, 1, q, 0, 0}, b[6] = {0, 0, f, 1, 1, q};
    ledSet(r[i] * 255 * mx, g[i] * 255 * mx, b[i] * 255 * mx); return;
  }
  if (mode == MODE_APP && app == A_SIMON && simonLit >= 0) {
    static const uint8_t C[3][3] = {{0, 120, 255}, {255, 40, 120}, {40, 255, 60}};
    ledSet(C[simonLit][0] * mx, C[simonLit][1] * mx, C[simonLit][2] * mx); return;
  }
  Heart h = (mode == MODE_CLOCK) ? Heart{0, 120, 140, 50, 1}
          : (mode == MODE_ASK) ? Heart{255, 150, 0, 40, 1}
          : (mode == MODE_NOTE) ? Heart{120, 60, 255, 50, 1}
          : (mode == MODE_MSG || mode == MODE_BOOT) ? Heart{255, 20, 90, 70, 0} : heartFor(curMood(now));
  if (S.unread && mode == MODE_FACE && (now / 3000) % 2) h = {255, 0, 160, 120, 2};
  float period = 60000.0f / h.bpm;
  float p = fmodf(now, period) / period, v;
  if (h.style == 1) v = 0.15f + 0.85f * (0.5f - 0.5f * cosf(p * 6.2832f));
  else if (h.style == 2) v = (p < 0.5f) ? 1.0f : 0.1f;
  else { float a = expf(-powf((p - 0.08f) / 0.045f, 2)), b = 0.7f * expf(-powf((p - 0.30f) / 0.05f, 2)); v = 0.08f + 0.92f * fmaxf(a, b); }
  float br = mx * v;
  ledSet(h.r * br, h.g * br, h.b * br);
}

// ===========================================================================
//  نقاشی
// ===========================================================================
static float blinkVal(uint32_t now) {
  uint32_t d = now - blinkAt;
  if (d > 160) return 0;
  return d < 80 ? d / 80.0f : (160 - d) / 80.0f;
}
static void drawBoot(uint32_t now) {
  uint32_t t = now - bootAt;
  EyeP e = moodTarget(M_NEUTRAL);
  if (bootPhase == 0) {
    float k = fminf(1.0f, t / 1100.0f);
    e.h = 3 + (36 - 3) * k * k;
    drawFace(M_NEUTRAL, e, 0, 32, 1.0f, now);
  } else if (bootPhase == 1) {
    EyeP h = moodTarget(M_HAPPY); drawFaceWithText(M_HAPPY, h, 0, now, P_HELLO);
  } else if (bootPhase == 2) {
    EyeP h = moodTarget(M_NEUTRAL); h.lookY = -2; drawFaceWithText(M_NEUTRAL, h, blinkVal(now), now, P_IAM);
  } else {
    const TxtLine &a = line0(P_OWNER), &b = line0(P_NAME);
    drawTxtCenter(a, 2); drawTxtCenter(b, 26);
    float beat = 1.0f + 0.15f * fmaxf(0, sinf(t * 0.011f));
    drawHeart(64 - b.w / 2 - 16, 26 + b.h / 2, 16 * beat);
    drawHeart(64 + b.w / 2 + 16, 26 + b.h / 2, 16 * beat);
  }
}
static PText clockT; static int clockTKey = -1;
static void drawClockScreen(uint32_t now) {
  drawClockTime(now_dt.h, now_dt.mi, now_dt.s, -1, true);
  // خط پایین به نوبت: تاریخ ، آب‌وهوا ، شمارش معکوس ، ساعت دوست ، روز دیدار
  int items[6], n = 0;
  int mh, mm; bool mt = friendTime(mh, mm);
  int md = meetDays();
  items[n++] = 0;
  if (wxTemp > -100) items[n++] = 1;
  int db = daysUntil(S.bm, S.bd), da = daysUntil(S.anm, S.anD);
  if (db > 0 && db <= 14) items[n++] = 2;
  if (da > 0 && da <= 14) items[n++] = 3;
  if (mt) items[n++] = 4;
  if (md >= 0 && md <= 400) items[n++] = 5;
  int it = items[(now / 4000) % n];
  int y = 43;
  if (it >= 4) {
    int key = it * 10000 + (it == 4 ? mh * 60 + mm : md);
    if (key != clockTKey) {
      clockTKey = key; char a[100], b[140];
      if (it == 4) snprintf(a, sizeof a, "ساعت %s %02d:%02d", S.fname, mh, mm);
      else if (md == 0) snprintf(a, sizeof a, "امروز روز دیداره ❤");
      else snprintf(a, sizeof a, "%d روز تا دیدار ❤", md);
      faNum(b, sizeof b, a); ptPrepare(clockT, b);
    }
    ptDrawLines(clockT, 0, 1, 45, 17);
    return;
  }
  if (it == 0) { int jy, jm, jd; jalaliToday(jy, jm, jd); drawDateLine(jd, jm, persianWeekday(now_dt.y, now_dt.mo, now_dt.d), y); }
  else if (it == 1) drawWeatherLine(wxTemp, wmoToWx(wxCode), wxDay, y, now);
  else {
    int d = (it == 2) ? db : da; const TxtLine &L = line0(it == 2 ? P_TO_BDAY : P_TO_ANNIV);
    int w = smallNumWidth(d), all = w + 5 + L.w, xr = 64 + all / 2;
    drawSmallNum(d, xr - w, y + 1); drawTxt(L, xr - w - 5 - L.w, y);
  }
}
static void drawMsgScreen(uint32_t now) {
  if (S.msgKind == 1) {                            // نقاشی
    oled.drawXBMP(0, 16, 128, 48, doodle);
    drawEnvelope(56, 1);
    if ((now / 500) % 2) { drawHeart(40, 7, 8); drawHeart(88, 7, 8); }
    return;
  }
  drawEnvelope(56, 0);
  if ((now / 500) % 2) { drawHeart(40, 6, 8); drawHeart(88, 6, 8); }
  ptDrawLines(msgT, msgPage * 3, 3, 13, 17);
  int pages = (msgT.lines + 2) / 3;
  if (pages > 1) for (int i = 0; i < pages; i++) {
    if (i == msgPage) oled.drawBox(2, 20 + i * 6, 3, 4); else oled.drawFrame(2, 20 + i * 6, 3, 4);
  }
}
static void drawMenu(uint32_t now) {
  uint8_t i = menuIdx;
  drawMenuIcon(i, 64, 18, now);
  drawTxtCenter(line0((PhraseId)G_MENU[i]), 64 - line0((PhraseId)G_MENU[i]).h);
  // فلش‌ها
  oled.drawTriangle(4, 24, 11, 17, 11, 31);
  oled.drawTriangle(124, 24, 117, 17, 117, 31);
  for (int k = 0; k < G_MENU_N; k++) {
    int x = 64 - G_MENU_N * 3 + k * 6;
    if (k == i) oled.drawBox(x, 37, 3, 3); else oled.drawPixel(x + 1, 38);
  }
}
static void drawParty(uint32_t now) {
  drawConfetti(now > modeAt ? now - modeAt : 0);
  int th = phraseHeight(partyP);
  if (partyP == P_BDAY && ((now > modeAt ? now - modeAt : 0) / 3000) % 2 == 0) drawCake(64, 54, now);
  else if (partyP == P_BDAY) drawPhraseAt(partyP, (64 - th) / 2);
  else {
    float b = 1 + 0.15f * fmaxf(0, sinf(now * 0.012f));
    drawHeart(40, 18, 22 * b); drawHeart(88, 18, 22 * b);
    drawPhraseAt(partyP, 64 - th);
  }
}
static void drawAlarm(uint32_t now) {
  drawClockTime(now_dt.h, now_dt.mi, now_dt.s, -1, false);
  if ((now / 600) % 2) drawTxtCenter(line0(P_ALARM), 64 - line0(P_ALARM).h);
  else { drawSun(18, 50, 5, now); drawSun(110, 50, 5, now); drawTxtCenter(line0(P_ALARM), 64 - line0(P_ALARM).h); }
}
static void drawDiag(uint32_t now) {
  char s[40];
  oled.setFont(u8g2_font_6x10_tr);
  snprintf(s, sizeof s, "T:%d L:%d R:%d TILT:%d", readTouch(PIN_TOUCH_TOP), readTouch(PIN_TOUCH_LEFT), readTouch(PIN_TOUCH_RIGHT), digitalRead(PIN_TILT));
  oled.drawStr(0, 10, s);
  snprintf(s, sizeof s, "shake/1s:%d love:%d", tiltCount(now, 1000, 0), S.love);
  oled.drawStr(0, 21, s);
  snprintf(s, sizeof s, "RTC:%s %02d:%02d:%02d", rtcOk ? "ok" : "--", now_dt.h, now_dt.mi, now_dt.s);
  oled.drawStr(0, 32, s);
  snprintf(s, sizeof s, "WiFi:%s %s", netUp ? "ok" : (apOn ? "AP" : "--"), netIP);
  oled.drawStr(0, 43, s);
  snprintf(s, sizeof s, "wx:%d %d  hold TOP=exit", wxTemp, wxCode);
  oled.drawStr(0, 54, s);
}
static void drawAsk(uint32_t now) {
  ptDrawLines(askT, 0, 1, 0, 17);
  for (int k = 0; k < 3; k++) {
    int x = 20 + k * 44, y = 42 + (int)(sinf(now * 0.006f + k * 2.1f) * 2);
    oled.drawCircle(x, y, 13); oled.drawCircle(x, y, 12);
    oled.drawDisc(x - 5, y - 4, 2); oled.drawDisc(x + 5, y - 4, 2);
    if (k == 0) oled.drawCircle(x, y + 10, 6, U8G2_DRAW_UPPER_LEFT | U8G2_DRAW_UPPER_RIGHT);
    else if (k == 1) oled.drawHLine(x - 5, y + 5, 11);
    else oled.drawCircle(x, y + 1, 7, U8G2_DRAW_LOWER_LEFT | U8G2_DRAW_LOWER_RIGHT);
  }
}
static void drawNote(uint32_t now) {
  if (noteIcon == 0) {                             // زنگوله
    int sw = (int)(sinf(now * 0.02f) * 2);
    oled.drawDisc(64 + sw, 7, 6, U8G2_DRAW_UPPER_LEFT | U8G2_DRAW_UPPER_RIGHT); oled.drawBox(58 + sw, 7, 13, 3);
    oled.drawHLine(56 + sw, 10, 17); oled.drawDisc(64 + sw, 12, 1);
  } else drawMenuIcon(7, 64, 7, now);
  ptDrawLines(noteT, notePage * 3, 3, 14, 17);
}
// کلاه و تزیین — با بیشتر شدن امتیاز محبت (xp)
static void drawAccessory(uint32_t now) {
  if (S.xp >= 400) {                               // تاج
    oled.drawBox(54, 6, 21, 4);
    oled.drawTriangle(54, 6, 58, 6, 54, 0); oled.drawTriangle(61, 6, 68, 6, 64, -1); oled.drawTriangle(71, 6, 75, 6, 75, 0);
    oled.setDrawColor(0); oled.drawPixel(64, 7); oled.setDrawColor(1);
  } else if (S.xp >= 150) {                        // گل
    for (int i = 0; i < 5; i++) { float a = i * 1.2566f + now * 0.001f; oled.drawDisc(64 + cosf(a) * 4, 5 + sinf(a) * 4, 2); }
    oled.setDrawColor(0); oled.drawDisc(64, 5, 1); oled.setDrawColor(1);
  } else if (S.xp >= 40) {                         // پاپیون
    oled.drawTriangle(64, 4, 55, 0, 55, 9); oled.drawTriangle(64, 4, 73, 0, 73, 9); oled.drawDisc(64, 4, 2);
  }
}
static void drawFloatHearts(uint32_t now) {
  for (int i = 0; i < 3; i++) {
    uint32_t ph = (now / 25 + i * 37) % 80;
    int x = (i == 1) ? 64 + (int)(sinf(now * 0.004f) * 4) : (i == 0 ? 6 : 122);
    int y = 70 - (int)ph;
    if (i == 1 && y > 14) continue;              // وسط فقط بالای صفحه (روی چشم‌ها نیفتد)
    drawHeart(x + (int)(sinf((now + i * 500) * 0.006f) * 2), y, 7);
  }
}
static uint8_t otaPct = 0;
static uint32_t ipShowUntil = 0;
static void drawOta() {
  oled.setFont(u8g2_font_6x10_tr);
  oled.drawStr(28, 20, "Updating...");
  drawBar(14, 30, 100, 12, otaPct / 100.0f);
}
// تصویر صفحه را با # و . در Serial چاپ می‌کند (برای عیب‌یابی)
static void dumpScreen() {
  uint8_t *buf = oled.getBufferPtr();
  Serial.printf("[screen] mode=%s\n", MODE_NAMES[mode]);
  char row[130];
  for (int y = 0; y < 64; y++) {
    for (int x = 0; x < 128; x++) row[x] = (buf[(y / 8) * 128 + x] >> (y % 8)) & 1 ? '#' : '.';
    row[128] = 0; Serial.println(row);
  }
}
static void render(uint32_t now) {
  Mood m = curMood(now);
  EyeP target = moodTarget(m);
  target.lookX += lookTX; target.lookY += lookTY;
  if (m == M_SLEEP) target.lookX = 0;
  eyeLerp(eye, target, 0.28f);

  oled.clearBuffer();
  switch (mode) {
    case MODE_BOOT:  drawBoot(now); break;
    case MODE_DIAG:  drawDiag(now); break;
    case MODE_CLOCK: drawClockScreen(now); break;
    case MODE_MSG:   drawMsgScreen(now); break;
    case MODE_MENU:  drawMenu(now); break;
    case MODE_APP:   drawApp(now); break;
    case MODE_ALARM: drawAlarm(now); break;
    case MODE_PARTY: drawParty(now); break;
    case MODE_OTA:   drawOta(); break;
    case MODE_ASK:   drawAsk(now); break;
    case MODE_NOTE:  drawNote(now); break;
    default: {
      float b = (m == M_SLEEP || m == M_LOVE || m == M_DIZZY) ? 0 : blinkVal(now);
      if (sneezeAt && now < sneezeAt + 500 && now >= sneezeAt) { b = 1; eye.lookY = 6; }   // هاپچه!
      drawFaceWithText(m, eye, b, now, msg);
      if (msg == P_COUNT) { drawAccessory(now); if (m == M_LOVE) drawFloatHearts(now); }
      if (S.unread && msg == P_COUNT && (now / 700) % 2) drawEnvelope(110, 1);
    }
  }
  // نشان دادن آدرس بعد از وصل شدن به وای‌فای
  if (now < ipShowUntil && mode != MODE_OTA) {
    char b[40];
    if (netUp) snprintf(b, sizeof b, "WiFi OK %s", netIP); else snprintf(b, sizeof b, "AP:Moochi 192.168.4.1");
    oled.setDrawColor(0); oled.drawBox(0, 52, 128, 12); oled.setDrawColor(1);
    oled.setFont(u8g2_font_6x10_tr); oled.drawStr((128 - oled.getStrWidth(b)) / 2, 62, b);
  }
  oled.sendBuffer();
}

// ===========================================================================
//  صفحه‌ی وب
// ===========================================================================
static bool webStarted = false, otaStarted = false, apShown = false;
static uint32_t rebootAt = 0;
static Mood moodByName(const String &n) { for (int i = 0; i < M_COUNT; i++) if (n == MOOD_NAMES[i]) return (Mood)i; return M_HAPPY; }
static void webSetup() {
  server.on("/", HTTP_GET, [] { server.send_P(200, "text/html; charset=utf-8", WEB_PAGE); });
  server.on("/api/status", HTTP_GET, [] {
    int jy, jm, jd; jalaliToday(jy, jm, jd);
    char b[640];
    snprintf(b, sizeof b,
      "{\"time\":\"%02d:%02d\",\"date\":\"%d/%d/%d\",\"temp\":%s,\"love\":%d,\"unread\":%s,\"wifi\":\"%s\","
      "\"ah\":%d,\"am\":%d,\"aon\":%d,\"adays\":%d,\"bm\":%d,\"bd\":%d,\"anm\":%d,\"and\":%d,\"city\":%d,\"ssid\":\"%s\",\"ntfy\":\"%s\"}",
      now_dt.h, now_dt.mi, jy, jm, jd, wxTemp > -100 ? String(wxTemp).c_str() : "null", S.love, S.unread ? "true" : "false",
      netUp ? netIP : (apOn ? "AP 192.168.4.1" : "-"), S.ah, S.am, S.aon, S.adays, S.bm, S.bd, S.anm, S.anD, S.city, S.ssid, NTFY_TOPIC);
    // متن آخرین پیام (با فرار دادن کاراکترهای JSON)
    String j(b); j.remove(j.length() - 1);
    j += ",\"msg\":\"";
    for (const char *p = S.msg; *p; p++) {
      char c = *p;
      if (c == '"' || c == '\\') { j += '\\'; j += c; }
      else if ((uint8_t)c < 0x20) j += ' ';
      else j += c;
    }
    char mt[16] = "";
    if (S.meet) snprintf(mt, sizeof mt, "%04d-%02d-%02d", (int)(S.meet / 10000), (int)(S.meet / 100 % 100), (int)(S.meet % 100));
    j += "\",\"kind\":"; j += S.msgKind;
    j += ",\"bot\":"; j += S.botTok[0] ? "true" : "false";
    j += ",\"botApi\":"; j += S.botApi;
    j += ",\"owner\":"; j += S.owner ? "true" : "false";
    j += ",\"pair\":\"" PAIR_CODE "\",\"xp\":"; j += S.xp;
    j += ",\"meet\":\""; j += mt;
    j += "\",\"meetDays\":"; j += meetDays();
    j += ",\"rem\":\""; j += jsonEsc(S.rem);
    j += "\",\"mood\":\""; j += jsonEsc(S.moodDay == now_dt.d ? S.lastMood : "");
    j += "\",\"oname\":\""; j += jsonEsc(S.oname);
    j += "\",\"fname\":\""; j += jsonEsc(S.fname);
    j += "\"}";
    server.send(200, "application/json; charset=utf-8", j);
  });
  server.on("/api/msg", HTTP_POST, [] {
    String t = server.arg("t");
    server.send(200, "text/plain", "ok");
    onIncoming(t.c_str());
  });
  server.on("/api/set", HTTP_POST, [] {
    if (server.hasArg("ah")) { S.ah = server.arg("ah").toInt(); S.am = server.arg("am2").toInt(); S.aon = server.arg("aon").toInt(); S.adays = server.arg("adays").toInt(); }
    if (server.hasArg("bm")) { S.bm = server.arg("bm").toInt(); S.bd = server.arg("bd").toInt(); S.anm = server.arg("am").toInt(); S.anD = server.arg("ad").toInt(); }
    if (server.hasArg("tz")) {
      strlcpy(S.tz, server.arg("tz").c_str(), sizeof S.tz); S.lat = server.arg("lat").toFloat(); S.lon = server.arg("lon").toFloat();
      S.city = server.arg("city").toInt(); wxTemp = -999; netCfgChanged = true;
    }
    if (server.hasArg("botTok")) {
      String tk = server.arg("botTok"); tk.trim();
      if (tk != "*") { strlcpy(S.botTok, tk.c_str(), sizeof S.botTok); S.owner = 0; prefs.begin("moochi", false); prefs.putLong64("owner", 0); prefs.end(); }
      S.botApi = server.arg("botApi").toInt();
    }
    if (server.hasArg("meet")) {
      int y = 0, mo = 0, d = 0;
      S.meet = (sscanf(server.arg("meet").c_str(), "%d-%d-%d", &y, &mo, &d) == 3) ? y * 10000 + mo * 100 + d : 0;
    }
    if (server.hasArg("oname")) setNames(server.arg("oname").c_str(), server.arg("fname").c_str());
    if (server.hasArg("rem")) { String r = server.arg("rem"); r.replace("\r", ""); r.trim(); strlcpy(S.rem, r.c_str(), sizeof S.rem); }
    bool reboot = false;
    if (server.hasArg("ssid") && server.arg("ssid").length()) {
      strlcpy(S.ssid, server.arg("ssid").c_str(), sizeof S.ssid); strlcpy(S.pass, server.arg("pass").c_str(), sizeof S.pass); reboot = true;
    }
    saveAll();
    server.send(200, "text/plain", "ok");
    if (reboot) rebootAt = millis() + 1500;
  });
  server.on("/api/do", HTTP_POST, [] {
    String a = server.arg("a"), x = server.arg("x");
    lastActive = millis(); wakeQuiet();
    if (a == "app") openApp(x.toInt());
    else if (a == "mood") { setMode(MODE_FACE); feel(moodByName(x), 5000); }
    else if (a == "boot") { setMode(MODE_BOOT); bootAt = millis(); bootPhase = 0; voiceStop(); PLAY(S_BOOT); }
    else if (a == "party") startParty(P_BDAY);
    else if (a == "showmsg") { if (S.msg[0]) showMessage(); }
    else if (a == "read") markRead(false);
    else if (a == "fal") { openApp(A_FAL); aState = 1; aT = millis(); PLAY(S_DIZZY); }
    else if (a == "ask") startAsk();
    else if (a == "music") { int k = x.length() ? x.toInt() : random(N_SONGS); if (k < 0 || k >= (int)N_SONGS) k = 0; openApp(A_MUSIC); aVal = k; ptPrepare(appT, SONGS[k].name); voiceStop(); play(SONGS[k].n, SONGS[k].len); aState = 1; }
    else if (a == "note") showNote(x.length() ? x.c_str() : "یادت نره! ⏰", 0, 60000);
    server.send(200, "text/plain", "ok");
  });
  server.on("/api/doodle", HTTP_POST, [] {
    static uint8_t tmp[sizeof doodle];
    size_t n = b64dec(server.arg("d").c_str(), tmp, sizeof tmp);
    if (n != sizeof doodle) { server.send(400, "text/plain", "bad"); return; }
    server.send(200, "text/plain", "ok");
    lastActive = millis(); onDoodle(tmp);
  });
  server.onNotFound([] { server.sendHeader("Location", "/"); server.send(302); });
  server.begin();
}
static const char BOT_HELP_FMT[] =
  "سلام! من موچی‌ام 🤖❤\n"
  "هر چی این‌جا بنویسی روی صفحه‌ی من نشون داده می‌شه و برای %s می‌خونمش.\n\n"
  "/status  حال و احوال موچی و %s\n"
  "/love  یه قلب بفرست\n"
  "/fal  فال حافظ برای %s\n"
  "/ask  بپرس امروز حالش چطوره\n"
  "/music  یه آهنگ براش بزن\n"
  "/remind 21:30 قرصت یادت نره\n"
  "/reminders  لیست یادآورها\n"
  "/clear  پاک کردن یادآورها\n"
  "/meet 2026-12-20  روز دیدار\n\n"
  "/names نیلا آرش  عوض کردن اسم‌ها\n\n"
  "🎨 نقاشی: توی صفحه‌ی «نقاشی برای موچی» بکش و متنی که با DOODLE: شروع می‌شه رو این‌جا بفرست.";
static const char *botHelp() {
  static char h[800];
  snprintf(h, sizeof h, BOT_HELP_FMT, S.oname, S.oname, S.oname);
  return h;
}
static void botHandle(int64_t chat, String t) {
  t.trim();
  char b[700];
  if (t.startsWith("/start") || t.startsWith("/pair")) {
    int sp = t.indexOf(' ');
    String code = sp > 0 ? t.substring(sp + 1) : String(""); code.trim();
    if (code == PAIR_CODE) {
      S.owner = chat; prefs.begin("moochi", false); prefs.putLong64("owner", chat); prefs.end();
      snprintf(b, sizeof b, "✅ وصل شدیم! از این به بعد پیام‌هات می‌رسه به موچی و خبرهای %s میاد این‌جا.", S.oname); botQueue(chat, b);
      botQueue(chat, botHelp());
      wakeQuiet(); setMode(MODE_FACE); feel(M_LOVE, 3000); PLAY(S_LOVE);
    } else if (S.owner == chat) botQueue(chat, botHelp());
    else botQueue(chat, "سلام! 🤖 من موچی‌ام. برای وصل شدن بنویس:\n/pair کد");
    return;
  }
  if (!S.owner || chat != S.owner) { botQueue(chat, "🙈 من فقط با صاحبم حرف می‌زنم. اول بنویس /pair و کد"); return; }
  lastActive = millis();
  if (t == "/help") { botQueue(chat, botHelp()); return; }
  if (t.startsWith("/names")) {
    String r = t.substring(6); r.trim(); int sp = r.indexOf(' ');
    if (r.length() && sp != 0) {
      String o = sp > 0 ? r.substring(0, sp) : r, f = sp > 0 ? r.substring(sp + 1) : String(S.fname); f.trim();
      setNames(o.c_str(), f.c_str());
      snprintf(b, sizeof b, "✅ اسم‌ها: %s و %s", S.oname, S.fname);
    } else snprintf(b, sizeof b, "اسم‌ها الان: %s و %s\nبرای عوض کردن: /names نیلا آرش", S.oname, S.fname);
    botQueue(chat, b); return;
  }
  if (t == "/status") {
    int mh = 0, mm = 0; bool mt = friendTime(mh, mm); int md = meetDays();
    char w[24] = "—"; if (wxTemp > -100) snprintf(w, sizeof w, "%d°", (int)wxTemp);
    snprintf(b, sizeof b, "🤖 موچی الان: %s\n🕒 ساعت %s: %02d:%02d\n🌡 هوا: %s\n❤ عشق‌سنج: %d٪\n⭐ امتیاز محبت: %u\n💌 پیام آخر: %s\n🙂 حال امروز %s: %s%s%s",
      MOOD_FA[curMood(millis())], S.oname, now_dt.h, now_dt.mi, w, S.love, (unsigned)S.xp,
      !S.msg[0] ? "—" : (S.unread ? "هنوز ندیده" : "دیده ✓"),
      S.oname, S.moodDay == now_dt.d && S.lastMood[0] ? S.lastMood : "هنوز معلوم نیست",
      md >= 0 ? "\n✈ روزهای مانده تا دیدار: " : "", md >= 0 ? String(md).c_str() : "");
    (void)mt; (void)mh; (void)mm;
    botQueue(chat, b); return;
  }
  if (t == "/love" || t == "/hug") { onIncoming("❤"); botQueue(chat, "❤ رسید! موچی چشم‌هاش قلبی شد"); return; }
  if (t == "/fal") { openApp(A_FAL); aState = 1; aT = millis(); PLAY(S_DIZZY); botQueue(chat, "📜 موچی داره فال می‌گیره... (فال برات فرستاده می‌شه)"); return; }
  if (t == "/ask") { startAsk(); botQueue(chat, "🙂 پرسیدم. جوابش رو برات می‌فرستم."); return; }
  if (t.startsWith("/music")) {
    int k = t.length() > 7 ? t.substring(7).toInt() - 1 : (int)random(N_SONGS);
    if (k < 0 || k >= (int)N_SONGS) k = 0;
    openApp(A_MUSIC); aVal = k; ptPrepare(appT, SONGS[k].name); voiceStop(); play(SONGS[k].n, SONGS[k].len); aState = 1;
    snprintf(b, sizeof b, "🎵 داره «%s» می‌زنه", SONGS[k].name); botQueue(chat, b); return;
  }
  if (t.startsWith("/remind")) {
    String r = t.substring(7); r.trim();
    int hh, mm;
    if (sscanf(r.c_str(), "%d:%d", &hh, &mm) != 2 || hh > 23 || mm > 59) { botQueue(chat, "مثال: /remind 21:30 قرصت یادت نره"); return; }
    char ln[200]; int sp = r.indexOf(' ');
    snprintf(ln, sizeof ln, "%02d:%02d %s", hh, mm, sp > 0 ? r.substring(sp + 1).c_str() : "");
    if (strlen(S.rem) + strlen(ln) + 2 >= sizeof S.rem) { botQueue(chat, "جا نیست! اول /clear بزن"); return; }
    if (S.rem[0]) strlcat(S.rem, "\n", sizeof S.rem);
    strlcat(S.rem, ln, sizeof S.rem); saveStr("rem", S.rem);
    snprintf(b, sizeof b, "⏰ هر روز %02d:%02d یادش می‌اندازم", hh, mm); botQueue(chat, b); return;
  }
  if (t == "/reminders") { snprintf(b, sizeof b, "⏰ یادآورها:\n%s", S.rem[0] ? S.rem : "— هیچی"); botQueue(chat, b); return; }
  if (t == "/clear") { S.rem[0] = 0; saveStr("rem", S.rem); botQueue(chat, "🧹 یادآورها پاک شد"); return; }
  if (t.startsWith("/meet")) {
    int y, mo, d;
    if (sscanf(t.c_str() + 5, " %d-%d-%d", &y, &mo, &d) == 3 && y > 2000) {
      S.meet = y * 10000 + mo * 100 + d; prefs.begin("moochi", false); prefs.putInt("meet", S.meet); prefs.end();
      snprintf(b, sizeof b, "✈ ثبت شد! %d روز مونده", meetDays()); botQueue(chat, b);
    } else { S.meet = 0; prefs.begin("moochi", false); prefs.putInt("meet", 0); prefs.end(); botQueue(chat, "مثال: /meet 2026-12-20 (روز دیدار پاک شد)"); }
    return;
  }
  if (t.startsWith("/mood ")) { setMode(MODE_FACE); feel(moodByName(t.substring(6)), 5000); botQueue(chat, "👀"); return; }
  if (t.startsWith("DOODLE:") || t.startsWith("doodle:")) {
    static uint8_t tmp[sizeof doodle];
    size_t n = b64dec(t.c_str() + 7, tmp, sizeof tmp);
    if (n == sizeof doodle) { onDoodle(tmp); botQueue(chat, "🎨 نقاشیت رسید به موچی!"); }
    else botQueue(chat, "🎨 نقاشی ناقص رسید، دوباره کپی کن و بفرست");
    return;
  }
  if (t.startsWith("/")) { botQueue(chat, "این دستور رو نمی‌شناسم 🤔 /help"); return; }
  bool heart = isHeartMsg(t.c_str());
  onIncoming(t.c_str());
  botQueue(chat, heart ? "❤ رسید! موچی چشم‌هاش قلبی شد" : "📩 رسید به موچی");
}
static void otaSetup() {
  ArduinoOTA.setHostname(HOSTNAME);
  ArduinoOTA.setPassword(OTA_PASS);
  ArduinoOTA.onStart([] { voiceStop(); soundStop(); setMode(MODE_OTA); otaPct = 0; render(millis()); });
  ArduinoOTA.onProgress([](unsigned p, unsigned t) {
    uint8_t pc = p * 100 / t;
    if (pc != otaPct) { otaPct = pc; oled.clearBuffer(); drawOta(); oled.sendBuffer(); ledSet(0, pc, 40); }
  });
  ArduinoOTA.onError([](ota_error_t) { setMode(MODE_FACE); feel(M_SAD, 3000); });
  ArduinoOTA.begin();
}
static void netLoop(uint32_t now) {
#if WIFI_ON
  if (!webStarted && (netUp || apOn)) { webStarted = true; webSetup(); }
  if (apOn && !apShown) { apShown = true; ipShowUntil = now + 15000; Serial.println("WiFi not found -> AP 'Moochi' 192.168.4.1"); }
  if (!otaStarted && netUp) { otaStarted = true; ipShowUntil = now + 10000; otaSetup(); MDNS.addService("http", "tcp", 80); Serial.printf("WiFi: %s  http://%s.local\n", netIP, HOSTNAME); }
  if (webStarted) server.handleClient();
  if (otaStarted) ArduinoOTA.handle();
  if (ntpFresh) {
    ntpFresh = false;
    time_t tt = time(nullptr); struct tm tm; localtime_r(&tt, &tm);
    DT t = {tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec};
    if (rtcOk) rtcWrite(t);
    rtcReadAt = 0;
    Serial.printf("NTP: %04d-%02d-%02d %02d:%02d\n", t.y, t.mo, t.d, t.h, t.mi);
  }
  if (inReady) {
    String text = inBuf; String id = inId; int64_t chat = inChat;
    inChat = 0; inReady = false;
    if (chat) botHandle(chat, text);
    else { strlcpy(S.lastId, id.c_str(), sizeof S.lastId); saveStr("lastId", S.lastId); onIncoming(text.c_str()); }
  }
  if (rebootAt && now > rebootAt) ESP.restart();
#endif
}

// ===========================================================================
//  Serial
// ===========================================================================
static void serialTick() {
  static char line[300]; static uint16_t n = 0;
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\r') continue;
    if (c != '\n' && n < sizeof(line) - 1) { line[n++] = c; continue; }
    line[n] = 0; n = 0;
    DT t;
    if (sscanf(line, "time %d-%d-%d %d:%d:%d", &t.y, &t.mo, &t.d, &t.h, &t.mi, &t.s) == 6) {
      rtcWrite(t); rtcOk = rtcRead(now_dt) || rtcOk;
      Serial.printf("ساعت تنظیم شد: %04d-%02d-%02d %02d:%02d:%02d\n", t.y, t.mo, t.d, t.h, t.mi, t.s);
    } else if (!strncmp(line, "mood ", 5)) { setMode(MODE_FACE); feel(moodByName(line + 5), 5000); }
    else if (!strcmp(line, "diag")) setMode(mode == MODE_DIAG ? MODE_FACE : MODE_DIAG);
    else if (!strcmp(line, "clock")) { setMode(MODE_CLOCK); clockTempUntil = 0; }
    else if (!strcmp(line, "shake")) pushEv(EV_SHAKE);
    else if (!strcmp(line, "hug")) pushEv(EV_HUG);
    else if (!strcmp(line, "menu")) pushEv(EV_MENU);
    else if (!strcmp(line, "alarm")) startAlarm();
    else if (!strcmp(line, "party")) startParty(P_BDAY);
    else if (!strncmp(line, "app ", 4)) openApp(atoi(line + 4));
    else if (!strncmp(line, "msg ", 4)) onIncoming(line + 4);
    else if (!strcmp(line, "boot")) { setMode(MODE_BOOT); bootAt = millis(); bootPhase = 0; voiceStop(); PLAY(S_BOOT); }
    else if (!strncmp(line, "say ", 4)) { int k = atoi(line + 4); if (k >= 0 && k < P_COUNT) { setMode(MODE_FACE); say((PhraseId)k, 2500); } }
    else if (!strcmp(line, "ip")) Serial.printf("WiFi:%s ip:%s ap:%d ssid:'%s' reason:%d %s\n", netUp ? "ok" : "--", netIP, apOn, S.ssid, wifiReason, reasonText(wifiReason));
    else if (!strcmp(line, "scan")) wifiScanPrint();
    else if (!strcmp(line, "dump")) dumpScreen();
    else if (!strcmp(line, "ask")) startAsk();
    else if (!strncmp(line, "names ", 6)) { char *sp = strchr(line + 6, ' '); if (sp) *sp++ = 0; setNames(line + 6, sp); }
    else if (!strcmp(line, "fal")) { openApp(A_FAL); aState = 1; aT = millis(); }
    else if (!strncmp(line, "note ", 5)) showNote(line + 5, 0, 60000);
    else if (!strcmp(line, "sneeze")) { setMode(MODE_FACE); feel(M_SURPRISED, 750); afterTmp = M_LAUGH; afterTmpMs = 900; sneezeAt = millis() + 650; PLAY(S_SNEEZE); }
    else if (!strncmp(line, "xp ", 3)) { S.xp = atoi(line + 3); xpDirty = loveDirty = true; }
    else {
      Serial.println(F("دستورها:\n  time 2026-10-10 18:30:00 | mood angry | clock | diag | shake | hug | menu | alarm | party\n"
                       "  app 0..9 | msg سلام عشقم | say 5 | boot | ip | ask | fal | note متن | sneeze | xp 200"));
    }
  }
}
static void i2cScan() {
  Serial.print("I2C:");
  for (uint8_t a = 1; a < 127; a++) { Wire.beginTransmission(a); if (Wire.endTransmission() == 0) Serial.printf(" 0x%02X", a); }
  Serial.println();
}

// ===========================================================================
void setup() {
  Serial.begin(115200);
  uint8_t tm = TOUCH_ACTIVE_HIGH ? INPUT_PULLDOWN : INPUT_PULLUP;
  pinMode(PIN_TOUCH_TOP, tm); pinMode(PIN_TOUCH_LEFT, tm); pinMode(PIN_TOUCH_RIGHT, tm);
  pinMode(PIN_TILT, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_TILT), tiltIsr, CHANGE);

  led.begin(); led.setBrightness(255); led.clear(); led.show();
  spkInit();
  voiceInit();
  loadSettings();

  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(400000);
  delay(50);
  i2cScan();
  Wire.beginTransmission(0x3D);
  if (Wire.endTransmission() == 0) oled.setI2CAddress(0x3D * 2);
  oled.begin();
  oled.setBusClock(400000);
  rebuildNamed();

  Wire.beginTransmission(RTC_ADDR);
  rtcOk = (Wire.endTransmission() == 0);
  if (rtcOk) { DT t; if (!rtcRead(t) || t.y < 2025) rtcWrite(compileTime()); }
  setenv("TZ", S.tz, 1); tzset();
  softBase = millis();
  timeTick(1000);
  Serial.printf("reset reason: %d\n", (int)esp_reset_reason());
  Serial.printf("RTC: %s  %04d-%02d-%02d %02d:%02d  love=%d\n", rtcOk ? "OK" : "NOT FOUND", now_dt.y, now_dt.mo, now_dt.d, now_dt.h, now_dt.mi, S.love);

  randomSeed(esp_random());
  bootAt = lastActive = millis();
  nextLove = millis() + LOVE_MSG_MIN_MS;
  nextBlink = millis() + 2500;
  nextNewMsg = millis() + 60000;

#if WIFI_ON
  xTaskCreate(netTask, "net", 12288, nullptr, 1, nullptr);
#endif

  delay(30);
  if (readTouch(PIN_TOUCH_TOP)) { setMode(MODE_DIAG); tTop.longFired = true; }
  else { setMode(MODE_BOOT); PLAY(S_BOOT); }
}

void loop() {
  static uint32_t lastFrame = 0;
  uint32_t now = millis();

  soundTick(now);
  voiceTick();
  touchesTick(now);
  tiltTick(now);
  serialTick();
  timeTick(now);
  netLoop(now);

  if (mode == MODE_BOOT) {
    uint32_t dur = max(BOOT_MIN[bootPhase], voiceMs(BOOT_P[bootPhase]) + 900);
    if (now - bootAt >= dur) {
      bootAt = now; bootPhase++;
      if (bootPhase >= 4) {
        setMode(MODE_FACE); lastActive = now; feel(M_HAPPY, 1000);
        PhraseId sp = specialToday();
        if (sp != P_COUNT) { partyDay = now_dt.d; startParty(sp); }
        else if (S.unread) { nextNewMsg = now + 3000; }
      } else if (hasVoice(BOOT_P[bootPhase])) { voiceStop(); vPending = BOOT_P[bootPhase]; }
    }
  }

  for (uint8_t i = 0; i < evN; i++) handle(evQ[i], now);
  evN = 0;
  now = millis();   // مهم: رویدادها/صفحه‌ي وب ممکن است زمان‌ها را «بعد از» now گذاشته باشند
  if (mode != MODE_BOOT && mode != MODE_DIAG && mode != MODE_OTA) { brainTick(now); alarmCheck(now); }

  if (namesDirty) { namesDirty = false; rebuildNamed(); }
  ledTick(now);
  if (now - lastFrame >= 33) { lastFrame = now; render(now); if (dumpAt && now > dumpAt) { dumpAt = 0; dumpScreen(); } }
  delay(1);
}
