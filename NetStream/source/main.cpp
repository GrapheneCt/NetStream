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

intrusive_ptr<graph::Surface> g_texCheckMark;
intrusive_ptr<graph::Surface> g_texTransparent;

void menu::main::NetcheckJob::Run()
{
	int32_t ret = SCE_OK;
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
		string pluginPath = common::FormatString("ux0:app/%s/module/download_enabler_netstream.suprx", titleid);
		taiLoadStartModuleForPid(shellPid, pluginPath.c_str(), 0, NULL, 0);
	}

	ytutils::Init();

	dialog::Close();
}

void pluginLoadCB(Plugin *plugin)
{
	if (plugin == NULL) {
		SCE_DBG_LOG_ERROR("[NS_PLUGIN_BASE] Plugin load FAIL!\n");
		return;
	}

	g_appPlugin = plugin;

	Framework *fw = Framework::Instance();

	dialog::OpenPleaseWait(g_appPlugin, NULL, fw->GetCommonString("msg_wait"));

	menu::main::NetcheckJob *ncJob = new menu::main::NetcheckJob("NS::NetcheckJob");
	common::SharedPtr<job::JobItem> itemParam(ncJob);
	job::JobQueue::default_queue->Enqueue(itemParam);

	menu::InitMenuSystem();
	menu::Settings::Init();

	g_texCheckMark = fw->GetCommonTexture("_common_texture_check_mark");
	g_texTransparent = fw->GetCommonTexture("_common_texture_transparent");

	new menu::First();
}

int main()
{
	// PAF framework init
	Framework::InitParam fwParam;
	Framework::SampleInit(&fwParam);
	fwParam.mode = Framework::Mode_Normal;
	fwParam.surface_pool_size = SCE_KERNEL_32MiB;
	fwParam.text_surface_pool_size = SCE_KERNEL_4MiB;
	fwParam.graph_heap_size_on_main_memory = 0;
	fwParam.graph_heap_size_on_video_memory = SCE_KERNEL_1MiB;
	fwParam.graphics_option = 7;

	if (SCE_PAF_IS_DOLCE)
	{
		scePowerSetConfigurationMode(SCE_POWER_CONFIGURATION_MODE_C);
		fwParam.env_param = new ui::EnvironmentParam(ui::EnvironmentParam::RESOLUTION_PSP2);
		fwParam.screen_width = 1920;
		fwParam.screen_height = 1088;
		//fwParam.option |= Framework::FrameworkOption_UseDisplayAreaSettings;
	}

	Framework *fw = new Framework(fwParam);
	fw->LoadCommonResourceAsync();

	if (SCE_PAF_IS_DOLCE)
	{
		fw->GetEnvironmentInstance()->SetResolution(960, 544);
	}

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
	new Module("app0:module/libInvidious.suprx");
	new Module("app0:module/libLootkit.suprx");
	new Module("app0:module/libcurl.suprx");

	curl_global_memmanager_set_np(sce_paf_malloc, sce_paf_free, sce_paf_realloc);

	utils::Init();

	// Plugin init
	Plugin::InitParam pluginParam;
	pluginParam.name = "netstream_plugin";
	pluginParam.resource_file = "app0:netstream_plugin.rco";
	pluginParam.caller_name = "__main__";
#ifdef _DEBUG
	pluginParam.option |= Plugin::Option_ResourceLoadWithDebugSymbol;
#endif
	pluginParam.start_func = pluginLoadCB;

	Plugin::LoadAsync(pluginParam);

	BEAVPlayer::PreInit();

	fw->Run();

	sceKernelExitProcess(0);

	return 0;
}
