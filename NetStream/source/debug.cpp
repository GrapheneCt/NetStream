#ifdef _DEBUG

#include <kernel.h>
#include <libdbg.h>
#include <paf.h>
#include <libdbg.h>

#include "common.h"
#include "debug.h"

static SceInt32 s_oldMemSize = 0;
static SceInt32 s_oldGraphMemSize = 0;
static SceUInt32 s_iter = 0;

SceVoid leakTestTask(ScePVoid pUserData)
{
	SceInt32 sz = 0;
	SceInt32 delta = 0;
	string *str;

#ifdef DEBUG_MEM_HEAP
	memory::HeapAllocator *glAlloc = memory::HeapAllocator::GetGlobalHeapAllocator();
	sz = glAlloc->GetFreeSize();

	str = new string();

	*str = ccc::MemsizeFormat(sz);
	sceClibPrintf("[NS_DEBUG] Free heap memory: %s\n", str->c_str());
	delta = s_oldMemSize - sz;
	delta = -delta;
	if (delta) {
		sceClibPrintf("[NS_DEBUG] Memory delta: %d bytes\n", delta);
	}
	s_oldMemSize = sz;
	delete str;
#endif

#ifdef DEBUG_SURFACE_HEAP
	str = new string();

	sz = s_frameworkInstance->GetDefaultSurfaceMemoryPool()->GetFreeSize();
	*str = ccc::MemsizeFormat(sz);

	sceClibPrintf("[NS_DEBUG] Free graphics pool: %s\n", str->c_str());

	delta = s_oldGraphMemSize - sz;
	delta = -delta;
	if (delta) {
		sceClibPrintf("[NS_DEBUG] Graphics pool delta: %d bytes\n", delta);
	}
	s_oldGraphMemSize = sz;
	delete str;
#endif
}

SceVoid JobQueueTestTask(ScePVoid pUserData)
{

}

void InitDebug()
{
#ifdef DEBUG_EXTRA_TTY
	SCE_PAF_AUTO_TEST_SET_EXTRA_TTY(sceIoOpen("tty0:", SCE_O_WRONLY, 0));
	SCE_PAF_AUTO_TEST_DUMP("\n\n-----PAF EXTRA TTY DEBUG MODE ON-----\n\n");
#endif

	task::Register(leakTestTask, SCE_NULL);
#ifdef DEBUG_JOB_QUEUE
	task::Register(JobQueueTestTask, SCE_NULL);
#endif
}

/*
SceUInt32 oldButtons = 0;
ui::Widget *widg = SCE_NULL;

SceVoid DirectInputCallback(input::GamePad::GamePadData *pData)
{
	Vector4 opos = widg->GetPosition();

	if (pData->buttons & SCE_PAF_CTRL_UP)
	{
		opos.y += 1;
	}
	else if (pData->buttons & SCE_PAF_CTRL_DOWN)
	{
		opos.y -= 1;
	}
	else if (pData->buttons & SCE_PAF_CTRL_RIGHT)
	{
		opos.x += 1;
	}
	else if (pData->buttons & SCE_PAF_CTRL_LEFT)
	{
		opos.x -= 1;
	}
	else if ((pData->buttons & SCE_PAF_CTRL_CROSS) && !(oldButtons & SCE_PAF_CTRL_CROSS))
	{
		sceClibPrintf("\nx: %f\ny: %f\n", opos.x, opos.y);
	}

	widg->SetPosition(&opos);

	oldButtons = pData->buttons;
}
*/

#endif