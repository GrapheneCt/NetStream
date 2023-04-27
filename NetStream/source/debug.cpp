#ifdef _DEBUG

#include <kernel.h>
#include <libdbg.h>
#include <paf.h>
#include <libdbg.h>

#include "common.h"
#include "debug.h"

static int32_t s_oldMemSize = 0;
static int32_t s_oldGraphMemSize = 0;
static uint32_t s_iter = 0;

void leakTestTask(void *pUserData)
{
	int32_t sz = 0;
	int32_t delta = 0;
	string str;

#ifdef DEBUG_MEM_HEAP
	memory::HeapAllocator *glAlloc = memory::GetGlobalHeapAllocator();
	sz = glAlloc->GetFreeSize();

	str.clear();
	str = common::FormatBytesize(sz, 0);
	sceClibPrintf("[NS_DEBUG] Free heap memory: %s\n", str.c_str());

	delta = s_oldMemSize - sz;
	delta = -delta;
	if (delta) {
		sceClibPrintf("[NS_DEBUG] Memory delta: %d bytes\n", delta);
	}
	s_oldMemSize = sz;
#endif

#ifdef DEBUG_SURFACE_HEAP

	sz = gutil::GetDefaultSurfacePool()->GetFreeSize();

	str.clear();
	str = common::FormatBytesize(sz, 0);
	sceClibPrintf("[NS_DEBUG] Free graphics pool: %s\n", str.c_str());

	delta = s_oldGraphMemSize - sz;
	delta = -delta;
	if (delta) {
		sceClibPrintf("[NS_DEBUG] Graphics pool delta: %d bytes\n", delta);
	}
	s_oldGraphMemSize = sz;
#endif
}

void JobQueueTestTask(void *pUserData)
{

}

void InitDebug()
{
#ifdef DEBUG_EXTRA_TTY
	SCE_PAF_AUTO_TEST_SET_EXTRA_TTY(sceIoOpen("tty0:", SCE_O_WRONLY, 0));
	SCE_PAF_AUTO_TEST_DUMP("\n\n-----PAF EXTRA TTY DEBUG MODE ON-----\n\n");
#endif

	common::MainThreadCallList::Register(leakTestTask, NULL);
#ifdef DEBUG_JOB_QUEUE
	MainThreadCallList::Register(JobQueueTestTask, NULL);
#endif
}

/*
uint32_t oldButtons = 0;
ui::Widget *widg = NULL;

void DirectInputCallback(input::GamePad::GamePadData *pData)
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