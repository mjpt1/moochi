# -*- coding: utf-8 -*-
"""متن‌های خوانا (انگلیسی و فارسی UTF-8) داخل یک فایل .bin را بیرون می‌کشد."""
import re, sys
data = open(sys.argv[1], "rb").read()
seen = set()
for m in re.finditer(rb"(?:[\x20-\x7e]|[\xc0-\xf4][\x80-\xbf]{1,3}){6,}", data):
    s = m.group().decode("utf-8", "ignore").strip()
    if len(s) >= 6 and s not in seen:
        seen.add(s)
        sys.stdout.buffer.write(f"{m.start():08x}  {s}\n".encode("utf-8"))
