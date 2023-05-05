#include <kernel.h>
#include <power.h> 
#include <libdbg.h>
#include <paf.h>
#include <stdlib.h>

#include "utils.h"
#include "yt_utils.h"
#include <paf_file_ext.h>
#include "common.h"

static SceUID s_lock = SCE_UID_INVALID_UID;
static job::JobQueue *s_utilsJobQueue = NULL;
static CurlFile::Share *s_curlShare = NULL;
static utils::PowerTick s_powerTickMode = utils::PowerTick_None;

static void PowerTickTask(void *pUserData)
{
	if (s_powerTickMode == utils::PowerTick_All)
		sceKernelPowerTick(SCE_KERNEL_POWER_TICK_DEFAULT);
	else if (s_powerTickMode == utils::PowerTick_Suspend)
		sceKernelPowerTick(SCE_KERNEL_POWER_TICK_DISABLE_AUTO_SUSPEND);
}

uint32_t utils::GetHash(const char *name)
{
	IDParam param(name);
	return param.GetIDHash();
}

wchar_t *utils::GetStringWithNum(const char *name, uint32_t num)
{
	IDParam searchRequest;
	char fullName[128];
	sce_paf_snprintf(fullName, sizeof(fullName), "%s%u", name, num);
	searchRequest.SetID(fullName);
	return g_appPlugin->GetString(searchRequest);
}

void utils::SetPowerTickTask(PowerTick mode)
{
	common::MainThreadCallList::Unregister(PowerTickTask, NULL);
	if (mode == PowerTick_None)
		return;

	s_powerTickMode = mode;
	common::MainThreadCallList::Register(PowerTickTask, NULL);
}

void utils::Lock(uint32_t flag)
{
	sceKernelClearEventFlag(s_lock, ~flag);
}

void utils::Unlock(uint32_t flag)
{
	sceKernelSetEventFlag(s_lock, flag);
}

void utils::Wait(uint32_t flag)
{
	sceKernelWaitEventFlag(s_lock, flag, SCE_KERNEL_EVF_WAITMODE_OR, NULL, NULL);
}

void utils::ConvertSecondsToString(string& string, uint64_t seconds, bool needSeparator)
{
	int32_t h = 0, m = 0, s = 0;
	h = (seconds / 3600);
	m = (seconds - (3600 * h)) / 60;
	s = (seconds - (3600 * h) - (m * 60));

	string.clear();

	if (needSeparator) {
		if (h > 0) {
			string = common::FormatString("%02d:%02d:%02d / ", h, m, s);
		}
		else {
			string = common::FormatString("%02d:%02d / ", m, s);
		}
	}
	else {
		if (h > 0) {
			string = common::FormatString("%02d:%02d:%02d", h, m, s);
		}
		else {
			string = common::FormatString("%02d:%02d", m, s);
		}
	}
}

void utils::SetDisplayResolution(uint32_t resolution)
{
	if (SCE_PAF_IS_DOLCE)
	{
		ui::Environment *env = Framework::Instance()->GetEnvironmentInstance();
		switch (resolution)
		{
		case ui::EnvironmentParam::RESOLUTION_PSP2:
			env->SetResolution(960, 544);
			break;
		case ui::EnvironmentParam::RESOLUTION_HD_HALF:
			env->SetResolution(1280, 725);
			break;
		case ui::EnvironmentParam::RESOLUTION_HD_FULL:
			env->SetResolution(1920, 1088);
			break;
		}
	}
}

void utils::Init()
{
	s_lock = sceKernelCreateEventFlag("utils::Lock", SCE_KERNEL_ATTR_MULTI, 0, NULL);

	job::JobQueue::Option queueOpt;
	queueOpt.workerNum = 1;
	queueOpt.workerOpt = NULL;
	queueOpt.workerPriority = SCE_KERNEL_HIGHEST_PRIORITY_USER + 30;
	queueOpt.workerStackSize = SCE_KERNEL_256KiB;

	s_utilsJobQueue = new job::JobQueue("utils::JobQueue", &queueOpt);

	s_curlShare = CurlFile::Share::Create();
	s_curlShare->AddRef();
}

job::JobQueue *utils::GetJobQueue()
{
	return s_utilsJobQueue;
}

CurlFile::Share *utils::GetShare()
{
	return s_curlShare;
}

void utils::SetTimeout(TimeoutFunc func, float timeoutMs, void *userdata1, void *userdata2)
{
	Timer *t = new Timer(timeoutMs);
	TimeoutListener *listener = new TimeoutListener(t, func);
	TimerListenerList::ListenerParam lparam(listener, true, userdata1, userdata2);
	TimerListenerList::s_default_list->Register(lparam);
}