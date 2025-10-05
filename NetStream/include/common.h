#ifndef _COMMON_H_
#define _COMMON_H_

#include <paf.h>

#include "netstream_plugin.h"
#include "netstream_settings.h"
#include "netstream_locale.h"

#include "menus/menu_settings.h"

#define USER_AGENT "Mozilla/5.0 (PlayStation Vita 3.74) AppleWebKit/537.73 (KHTML, like Gecko) Silk/3.2"

#define NP_TUS_FAV_LOG_SLOT		(2)
#define NP_TUS_HIST_LOG_SLOT	(1)

using namespace paf;

extern Plugin *g_appPlugin;

extern intrusive_ptr<graph::Surface> g_texCheckMark;
extern intrusive_ptr<graph::Surface> g_texTransparent;

#endif
