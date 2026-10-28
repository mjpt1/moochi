@echo off
chcp 65001 >nul
REM برگرداندن برنامه‌ي قدیمی (از بکاپ) روی برد
cd /d "%~dp0"
set "PY=%USERPROFILE%\.platformio\penv\Scripts\python.exe"
set "ESPTOOL=%USERPROFILE%\.platformio\packages\tool-esptoolpy\esptool.py"
if not exist backup\moochi-old-full.bin ( echo بکاپ پیدا نشد & pause & exit /b 1 )
"%PY%" "%ESPTOOL%" --chip esp32c3 --baud 460800 write_flash 0 backup\moochi-old-full.bin
pause
