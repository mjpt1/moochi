@echo off
chcp 65001 >nul
REM ============================================================
REM  بکاپ کامل برنامه‌ی فعلی روی ESP32-C3 (قبل از Upload موچی ۲)
REM  خروجی: backup\moochi-old-full.bin  (۴ مگابایت ، کل حافظه)
REM ============================================================
cd /d "%~dp0"
set "PY=%USERPROFILE%\.platformio\penv\Scripts\python.exe"
set "ESPTOOL=%USERPROFILE%\.platformio\packages\tool-esptoolpy\esptool.py"
if not exist "%ESPTOOL%" (
  echo esptool پیدا نشد. یک بار پروژه را در PlatformIO Build کن و دوباره اجرا کن.
  pause & exit /b 1
)
if not exist backup mkdir backup
echo در حال خواندن حافظه‌ي برد... (حدود ۱ دقیقه)
echo اگر وصل نشد: BOOT را نگه دار ، RST را بزن ، BOOT را ول کن و دوباره اجرا کن.
"%PY%" "%ESPTOOL%" --chip esp32c3 --baud 460800 read_flash 0 0x400000 backup\moochi-old-full.bin
if errorlevel 1 ( echo خطا! & pause & exit /b 1 )
"%PY%" tools\bin_strings.py backup\moochi-old-full.bin > backup\moochi-old-strings.txt
echo.
echo تمام شد:  backup\moochi-old-full.bin
echo متن‌های داخل برنامه‌ي قبلی:  backup\moochi-old-strings.txt
pause
