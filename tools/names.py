# -*- coding: utf-8 -*-
"""
اسم صاحب ربات و دوستش — برای جمله‌ها و صدا
Names of the robot's owner and her friend — used in sentences and voice.

  tools/names.json  (شخصی ، روی گیت‌هاب نمی‌رود / personal, git-ignored)
  tools/names.example.json  (نمونه / example)

اگر names.json باشد ، خروجی در src/*_personal.h ساخته می‌شود (آن هم روی گیت‌هاب نمی‌رود).
If names.json exists, output goes to src/*_personal.h (also git-ignored).
"""
import json, os

HERE = os.path.dirname(os.path.abspath(__file__))
_P = os.path.join(HERE, "names.json")
PERSONAL = os.path.exists(_P)
NAMES = json.load(open(_P if PERSONAL else os.path.join(HERE, "names.example.json"), encoding="utf-8"))


def fill(s):
    return s.replace("{OWNER}", NAMES["owner"]).replace("{FRIEND}", NAMES["friend"])


def out_path(base):
    """base = 'moochi_text' / 'moochi_voice'"""
    return os.path.join(HERE, "..", "src", base + ("_personal.h" if PERSONAL else ".h"))
