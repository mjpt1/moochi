@echo off
chcp 65001 >nul
REM ساعت ربات را با ساعت همین کامپیوتر تنظیم می‌کند (Serial Monitor باید بسته باشد)
cd /d "%~dp0"
"%USERPROFILE%\.platformio\penv\Scripts\python.exe" tools\set_time.py %1
pause
