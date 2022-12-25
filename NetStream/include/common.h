#ifndef _COMMON_H_
#define _COMMON_H_

#include <paf.h>

#include "netstream_plugin.h"
#include "netstream_settings.h"
#include "netstream_locale.h"

#include "menus/menu_settings.h"

#define USER_AGENT "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/108.0.0.0 Safari/537.36"

using namespace paf;

extern Plugin *g_appPlugin;

extern graph::Surface *g_texCheckMark;
extern graph::Surface *g_texTransparent;

#endif
