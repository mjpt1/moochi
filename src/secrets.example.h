// ===========================================================================
//  نمونه‌ی رمزها — این فایل را کپی کن به secrets.h و مقدارهای خودت را بنویس.
//  Example secrets — copy this file to secrets.h and put your own values in it.
//  secrets.h در .gitignore است و هیچ‌وقت روی گیت‌هاب نمی‌رود.
// ===========================================================================
#pragma once

#define WIFI_SSID        "your-wifi-name"
#define WIFI_PASS        "your-wifi-password"

// اگر وای‌فای پیدا نشد ، موچی یک وای‌فای به اسم «Moochi» با این رمز می‌سازد (حداقل ۸ حرف)
// If Wi-Fi is not found, Moochi opens its own hotspot "Moochi" with this password (8+ chars)
#define AP_PASS          "moochi1234"

// رمز آپلود بی‌سیم (OTA) — همین را در secrets.ini هم بنویس
// Over-the-air upload password — use the same value in secrets.ini
#define OTA_PASS         "moochi1234"

// کد وصل شدن به بات بله/تلگرام:  /pair 1234   — حتما عوضش کن
// Code to pair your Bale/Telegram bot:  /pair 1234   — change it!
#define PAIR_CODE        "1234"

// کانال ntfy.sh — هر کس اسمش را بداند می‌تواند پیام بفرستد ، پس طولانی و تصادفی بگذار
// ntfy.sh topic — anyone who knows it can message Moochi, so make it long and random
#define NTFY_TOPIC       "moochi-change-me-to-something-random"

// اسم صاحب ربات و دوستش (روی صفحه و در بات)
// Owner's and friend's names (shown on screen and in the bot)
// اسم‌های جمله‌های آماده و صدا در tools/names.json هستند — README را ببین
#define OWNER_NAME       "نیلا"
#define FRIEND_NAME      "آرش"
// تولد صاحب ربات (شمسی: ماه ، روز) — ۰ = تنظیم نشده (از صفحه‌ی تنظیمات هم می‌شود)
// Owner's birthday (Jalali month, day) — 0 = not set (also settable from the web page)
#define BDAY_JM          0
#define BDAY_JD          0
