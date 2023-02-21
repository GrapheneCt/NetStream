#include <appmgr.h>
#include <kernel.h>
#include <libsysmodule.h>
#include <libdbg.h>
#include <net.h>
#include <libnetctl.h>
#include <paf.h>
#include <power.h>
#include <taihen.h>
#include <stdlib.h>

#include "common.h"
#include "main.h"
#include "debug.h"
#include "dialog.h"
#include "utils.h"
#include "yt_utils.h"
#include "tw_utils.h"
#include "beav_player.h"
#include "invidious.h"
#include "menus/menu_generic.h"
#include "menus/menu_first.h"

extern "C"
{
	int curl_global_memmanager_set_np(void*(*)(unsigned int), void(*)(void *), void*(*)(void *, unsigned int));
}

using namespace paf;

Plugin *g_appPlugin;

graph::Surface *g_texCheckMark;
graph::Surface *g_texTransparent;

SceVoid menu::main::NetcheckJob::Run()
{
	SceInt32 ret = SCE_OK;
	SceUID shellPid = SCE_UID_INVALID_UID;
	char titleid[12];
	int sarg[2];
	SceNetInitParam param;

	param.memory = sce_paf_malloc(SCE_KERNEL_16KiB);
	param.size = SCE_KERNEL_16KiB;
	param.flags = 0;
	ret = sceNetInit(&param);
	while (ret != SCE_OK)
	{
		ret = sceNetInit(&param);
		thread::Sleep(100);
	}

	ret = sceNetCtlInit();
	while (ret != SCE_OK)
	{
		ret = sceNetCtlInit();
		thread::Sleep(100);
	}

	sceNetCtlInetGetState(&ret);
	while (ret != SCE_NET_CTL_STATE_IPOBTAINED)
	{
		sceNetCtlInetGetState(&ret);
		thread::Sleep(100);
	}

	sceAppMgrAppParamGetString(SCE_KERNEL_PROCESS_ID_SELF, 12, titleid, 12);
	if (sceAppMgrGetIdByName(&shellPid, "NPXS19999") == SCE_OK &&
		_vshKernelSearchModuleByName("taihen", sarg) > 0)
	{
		string pluginPath;
		common::string_util::setf(pluginPath, "ux0:app/%s/module/download_enabler_netstream.suprx", titleid);
		taiLoadStartModuleForPid(shellPid, pluginPath.c_str(), 0, SCE_NULL, 0);
	}

	ytutils::Init();

	dialog::Close();
}

SceVoid pluginLoadCB(Plugin *plugin)
{
	rco::Element searchParam;

	if (plugin == SCE_NULL) {
		SCE_DBG_LOG_ERROR("[NS_PLUGIN_BASE] Plugin load FAIL!\n");
		return;
	}

	g_appPlugin = plugin;

	dialog::OpenPleaseWait(g_appPlugin, SCE_NULL, utils::GetString("msg_wait"));

	menu::main::NetcheckJob *ncJob = new menu::main::NetcheckJob("NS::NetcheckJob");
	common::SharedPtr<job::JobItem> itemParam(ncJob);
	job::s_defaultJobQueue->Enqueue(itemParam);

	//menu::InitMenuSystem();
	menu::Settings::Init();

	Plugin *CRplugin = Plugin::Find("__system__common_resource");

	searchParam.hash = utils::GetHash("_common_texture_check_mark");
	Plugin::GetTexture(&g_texCheckMark, CRplugin, &searchParam);

	searchParam.hash = utils::GetHash("_common_texture_transparent");
	Plugin::GetTexture(&g_texTransparent, CRplugin, &searchParam);

	new menu::First();
}

int main()
{
	// PAF framework init
	Framework::InitParam fwParam;
	fwParam.LoadDefaultParams();
	fwParam.applicationMode = Framework::Mode_Game;
	fwParam.defaultSurfacePoolSize = SCE_KERNEL_32MiB;
	fwParam.textSurfaceCacheSize = SCE_KERNEL_4MiB;
	fwParam.graphMemSystemHeapSize = SCE_KERNEL_1MiB;
	fwParam.graphicsFlags = 7;

	if (SCE_PAF_IS_DOLCE)
	{
		scePowerSetConfigurationMode(SCE_POWER_CONFIGURATION_MODE_C);
		fwParam.uiCtxOpt = new ui::Context::Option(ui::Context::Option::Flag_ResolutionDefault);
		fwParam.screenWidth = 1920;
		fwParam.sceenHeight = 1088;
		//fwParam.optionalFeatureFlags = fwParam.optionalFeatureFlags | Framework::InitParam::FeatureFlag_UseDisplayAreaSettings;
	}

	Framework *fw = new Framework(fwParam);
	fw->LoadCommonResourceAsync();

#ifdef _DEBUG
	sceDbgSetMinimumLogLevel(SCE_DBG_LOG_LEVEL_TRACE);
	InitDebug();
#endif

	SceAppUtilInitParam init;
	SceAppUtilBootParam boot;
	sce_paf_memset(&init, 0, sizeof(SceAppUtilInitParam));
	sce_paf_memset(&boot, 0, sizeof(SceAppUtilBootParam));
	sceAppUtilInit(&init, &boot);

	// Module init
	sceSysmoduleLoadModule(SCE_SYSMODULE_FIBER);
	sceSysmoduleLoadModule(SCE_SYSMODULE_NET);
	sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_BXCE);
	sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_INI_FILE_PROCESSOR);
	sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_COMMON_GUI_DIALOG);
	new Module("app0:module/libInvidious.suprx", 0, 0, 0);
	new Module("app0:module/libLootkit.suprx", 0, 0, 0);
	new Module("app0:module/libcurl.suprx", 0, 0, 0);

	curl_global_memmanager_set_np(sce_paf_malloc, sce_paf_free, sce_paf_realloc);

	utils::Init();

	// Plugin init
	Plugin::InitParam pluginParam;
	pluginParam.pluginName = "netstream_plugin";
	pluginParam.resourcePath = "app0:netstream_plugin.rco";
	pluginParam.scopeName = "__main__";
#ifdef _DEBUG
	pluginParam.pluginFlags = Plugin::InitParam::PluginFlag_UseRcdDebug;
#endif
	pluginParam.pluginStartCB = pluginLoadCB;

	fw->LoadPluginAsync(pluginParam);

	BEAVPlayer::PreInit();

	fw->Run();

	sceKernelExitProcess(0);

	return 0;
}
