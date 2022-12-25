#include <kernel.h>
#include <power.h> 
#include <libdbg.h>
#include <paf.h>
#include <stdlib.h>

#include "utils.h"
#include "yt_utils.h"
#include "curl_file.h"
#include "common.h"

static SceUID s_lock = SCE_UID_INVALID_UID;
static job::JobQueue *s_utilsJobQueue = SCE_NULL;
static CurlFile::Share *s_curlShare = SCE_NULL;
static utils::PowerTick s_powerTickMode = utils::PowerTick_None;

namespace utils {
	SceVoid PowerTickTask(ScePVoid pUserData)
	{
		if (s_powerTickMode == PowerTick_All)
			sceKernelPowerTick(SCE_KERNEL_POWER_TICK_DEFAULT);
		else if (s_powerTickMode == PowerTick_Suspend)
			sceKernelPowerTick(SCE_KERNEL_POWER_TICK_DISABLE_AUTO_SUSPEND);
	}
}

SceUInt32 utils::GetHash(const char *name)
{
	rco::Element searchResult;

	searchResult.id = name;
	searchResult.hash = searchResult.GetHash(&searchResult.id);

	return searchResult.hash;
}

rco::Element utils::CreateElement(const char *name)
{
	rco::Element ret;
	ret.id = name;
	ret.hash = ret.GetHash(&ret.id);

	return ret;
}

wchar_t *utils::GetStringWithNum(const char *name, SceUInt32 num)
{
	rco::Element searchRequest;
	char fullName[128];

	sce_paf_snprintf(fullName, sizeof(fullName), "%s%u", name, num);

	searchRequest.hash = utils::GetHash(fullName);
	wchar_t *res = (wchar_t *)g_appPlugin->GetWString(&searchRequest);

	return res;
}

wchar_t *utils::GetString(const char *name)
{
	rco::Element searchRequest;

	searchRequest.hash = utils::GetHash(name);
	wchar_t *res = (wchar_t *)g_appPlugin->GetWString(&searchRequest);

	return res;
}

wchar_t *utils::GetString(SceUInt32 hash)
{
	rco::Element searchRequest;

	searchRequest.hash = hash;
	wchar_t *res = (wchar_t *)g_appPlugin->GetWString(&searchRequest);

	return res;
}

graph::Surface *utils::GetTexture(const char *name)
{
	graph::Surface *tex = SCE_NULL;
	rco::Element searchParam;

	searchParam.hash = utils::GetHash(name);
	Plugin::GetTexture(&tex, g_appPlugin, &searchParam);

	return tex;
}

graph::Surface *utils::GetTexture(SceUInt32 hash)
{
	graph::Surface *tex = SCE_NULL;
	rco::Element searchParam;

	searchParam.hash = hash;
	Plugin::GetTexture(&tex, g_appPlugin, &searchParam);

	return tex;
}

ui::Widget *utils::GetChild(ui::Widget *parent, const char *id)
{
	rco::Element searchRequest;

	searchRequest.hash = utils::GetHash(id);
	return parent->GetChild(&searchRequest, 0);
}

ui::Widget *utils::GetChild(ui::Widget *parent, SceUInt32 hash)
{
	rco::Element searchRequest;

	searchRequest.hash = hash;
	return parent->GetChild(&searchRequest, 0);
}

SceVoid utils::SetPowerTickTask(PowerTick mode)
{
	task::Unregister(utils::PowerTickTask, SCE_NULL);
	if (mode == PowerTick_None)
		return;

	s_powerTickMode = mode;
	task::Register(utils::PowerTickTask, SCE_NULL);
}

SceVoid utils::Lock(SceUInt32 flag)
{
	sceKernelClearEventFlag(s_lock, ~flag);
}

SceVoid utils::Unlock(SceUInt32 flag)
{
	sceKernelSetEventFlag(s_lock, flag);
}

SceVoid utils::Wait(SceUInt32 flag)
{
	sceKernelWaitEventFlag(s_lock, flag, SCE_KERNEL_EVF_WAITMODE_OR, SCE_NULL, SCE_NULL);
}

SceVoid utils::ConvertSecondsToString(string *string, SceUInt64 seconds, SceBool needSeparator)
{
	SceInt32 h = 0, m = 0, s = 0;
	h = (seconds / 3600);
	m = (seconds - (3600 * h)) / 60;
	s = (seconds - (3600 * h) - (m * 60));

	if (needSeparator) {
		if (h > 0) {
			string->clear();
			*string = ccc::Sprintf("%02d:%02d:%02d / ", h, m, s);
		}
		else {
			string->clear();
			*string = ccc::Sprintf("%02d:%02d / ", m, s);
		}
	}
	else {
		if (h > 0) {
			string->clear();
			*string = ccc::Sprintf("%02d:%02d:%02d", h, m, s);
		}
		else {
			string->clear();
			*string = ccc::Sprintf("%02d:%02d", m, s);
		}
	}
}

SceVoid utils::Init()
{
	s_lock = sceKernelCreateEventFlag("utils::Lock", SCE_KERNEL_ATTR_MULTI, 0, SCE_NULL);

	job::JobQueue::Option queueOpt;
	queueOpt.workerNum = 1;
	queueOpt.workerOpt = SCE_NULL;
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

SceBool utils::LoadNetworkSurface(const char *url, graph::Surface **surface)
{
	graph::Surface *tex = SCE_NULL;
	SharedPtr<CurlFile> fres;
	SceInt32 res = -1;

	fres = CurlFile::Open(url, &res, 0, s_curlShare);
	if (res < 0)
	{
		*surface = SCE_NULL;
		return SCE_FALSE;
	}

	graph::Surface::Create(&tex, g_appPlugin->memoryPool, (SharedPtr<File>*)&fres);
	if (tex == SCE_NULL)
	{
		*surface = SCE_NULL;
		return SCE_FALSE;
	}

	fres.reset();

	*surface = tex;

	return SCE_TRUE;
}

utils::AsyncNetworkSurfaceLoader::TargetDeleteEventCallback::~TargetDeleteEventCallback()
{
	delete workObj;
}

utils::AsyncNetworkSurfaceLoader::AsyncNetworkSurfaceLoader(const char *url, ui::Widget *target, graph::Surface **surf, SceBool autoLoad)
{
	item = new Job("AsyncNetworkSurfaceLoader");
	item->target = target;
	item->url = url;
	item->loadedSurface = surf;
	item->workObj = this;
	target->RegisterEventCallback(0x10000000, new TargetDeleteEventCallback(this));

	if (autoLoad)
	{
		SharedPtr<job::JobItem> itemParam(item);
		utils::GetJobQueue()->Enqueue(&itemParam);
	}
}

utils::AsyncNetworkSurfaceLoader::~AsyncNetworkSurfaceLoader()
{
	if (item)
	{
		item->workObj = SCE_NULL;
		item->Cancel();
		item = SCE_NULL;
	}
}

SceVoid utils::AsyncNetworkSurfaceLoader::Load()
{
	SharedPtr<job::JobItem> itemParam(item);
	utils::GetJobQueue()->Enqueue(&itemParam);
}

SceVoid utils::AsyncNetworkSurfaceLoader::Abort()
{
	if (item)
	{
		item->workObj = SCE_NULL;
		item->Cancel();
		item = SCE_NULL;
	}
}

SceVoid utils::AsyncNetworkSurfaceLoader::Job::Finish()
{
	if (workObj)
		workObj->item = SCE_NULL;
}

SceVoid utils::AsyncNetworkSurfaceLoader::Job::Run()
{
	graph::Surface *tex = SCE_NULL;
	SceInt32 res = -1;
	CurlFile::OpenArg oarg;
	oarg.SetUrl(url.c_str());
	oarg.SetShare(s_curlShare);
	oarg.SetOpt(3000, CurlFile::OpenArg::Opt_ConnectTimeOut);
	oarg.SetOpt(5000, CurlFile::OpenArg::Opt_RecvTimeOut);

	CurlFile *file = new CurlFile();
	res = file->Open(&oarg);
	if (res != 0)
	{
		delete file;
		return;
	}

	SharedPtr<CurlFile> fres(file);

	if (IsCanceled())
	{
		fres->Close();
		fres.reset();
		return;
	}

	graph::Surface::Create(&tex, g_appPlugin->memoryPool, (SharedPtr<File>*)&fres);
	fres.reset();
	if (tex == SCE_NULL)
	{
		return;
	}

	if (IsCanceled())
	{
		tex->Release();
		return;
	}

	thread::s_mainThreadMutex.Lock();
	target->SetSurfaceBase(&tex);
	thread::s_mainThreadMutex.Unlock();

	if (loadedSurface)
	{
		*loadedSurface = tex;
	}
	else
	{
		tex->Release();
	}
}