#ifndef _DEBUG_H_
#define _DEBUG_H_

#include <kernel.h>
#include <paf.h>

#ifdef _DEBUG

//#define DEBUG_MEM_HEAP
//#define DEBUG_SURFACE_HEAP
//#define DEBUG_JOB_QUEUE
//#define DEBUG_EXTRA_TTY

void InitDebug();
void SetCurrentDebugParam(paf::Plugin *plugin, paf::ui::Scene *page);
void SetPageDebugMode(paf::Plugin *plugin, paf::IDParam const& id, paf::Plugin::PageDebugMode mode, bool on);

#endif

#endif
