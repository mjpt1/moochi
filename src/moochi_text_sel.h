// جمله‌ها: نسخه‌ی شخصی (با اسم‌های واقعی ، روی گیت‌هاب نمی‌رود) یا نسخه‌ی نمونه
// Sentences: personal build (real names, git-ignored) if present, otherwise the example one
#pragma once
#if __has_include("moochi_text_personal.h")
#include "moochi_text_personal.h"
#else
#include "moochi_text.h"
#endif
