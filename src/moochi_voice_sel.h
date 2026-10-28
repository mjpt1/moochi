// صدا: نسخه‌ی شخصی (روی گیت‌هاب نمی‌رود) یا نسخه‌ی نمونه
// Voice: personal build (git-ignored) if present, otherwise the example one
#pragma once
#if __has_include("moochi_voice_personal.h")
#include "moochi_voice_personal.h"
#else
#include "moochi_voice.h"
#endif
