#include <kernel.h>
#include <paf.h>

#include "event.h"

static void PostEventRecursive(ui::Widget *target, int32_t type, ui::Event *ev)
{
	for (int i = 0; i < target->GetChildrenNum(); i++)
	{
		ui::Widget *subtarget = target->GetChild(i);
		if (subtarget)
		{
			PostEventRecursive(subtarget, type, ev);
		}
	}
	target->DoEvent(type, ev);
}

void event::BroadcastGlobalEvent(Plugin *workPlugin, uint32_t type, int32_t d0, int32_t d1, int32_t d2, int32_t d3)
{
	ui::Event ev(ui::EV_COMMAND, ui::Event::MODE_DISPATCH, ui::Event::ROUTE_FORWARD, 0, d0, d1, d2, d3);
	resource::ResourceObj *res = workPlugin->GetResource();
	if (res)
	{
		cxml::Element el = res->GetTableElement(resource::TableType_Page);
		el = el.GetFirstChild();
		while (el == true)
		{
			uint32_t pageId = 0;
			cxml::util::GetIDHash(el, "id", &pageId);
			ui::Widget *target = workPlugin->PageRoot(pageId);
			if (target)
			{
				PostEventRecursive(target, type, &ev);
			}
			el = el.GetNextSibling();
		}
	}
}