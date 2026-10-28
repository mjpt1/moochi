# -*- coding: utf-8 -*-
"""ساعت موچی را با ساعت کامپیوتر تنظیم می‌کند.  اجرا: python set_time.py [COMx]"""
import sys, time, datetime
import serial, serial.tools.list_ports

port = sys.argv[1] if len(sys.argv) > 1 else None
if not port:
    for p in serial.tools.list_ports.comports():
        if p.vid == 0x303A:                      # USB خود ESP32-C3
            port = p.device
            break
if not port:
    sys.exit("برد پیدا نشد. شماره‌ي پورت را بده:  set_time.py COM5")
with serial.Serial(port, 115200, timeout=1) as s:
    time.sleep(1.5)
    now = datetime.datetime.now() + datetime.timedelta(seconds=1)
    cmd = now.strftime("time %Y-%m-%d %H:%M:%S\n")
    s.write(cmd.encode())
    time.sleep(0.8)
    print("ارسال شد:", cmd.strip())
    print(s.read(300).decode("utf-8", "ignore"))
