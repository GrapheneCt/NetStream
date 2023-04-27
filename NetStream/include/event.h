#ifndef _EVENT_UTIL_H_
#define _EVENT_UTIL_H_

#include <kernel.h>
#include <paf.h>

using namespace paf;

namespace event
{
	void BroadcastGlobalEvent(Plugin *workPlugin, uint32_t type, int32_t d0 = 0, int32_t d1 = 0, int32_t d2 = 0, int32_t d3 = 0);
};


#endif
