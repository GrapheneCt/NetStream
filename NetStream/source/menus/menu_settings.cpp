#include <kernel.h>
#include <appmgr.h>
#include <stdlib.h>
#include <string.h>
#include <paf.h>
#include <audioout.h>
#include <shellaudio.h>
#include <libsysmodule.h>
#include <libdbg.h>
#include <bxce.h>
#include <app_settings.h>
#include <ini_file_processor.h>

#include "common.h"
#include "menus/menu_settings.h"
#include "menus/menu_generic.h"
#include "utils.h"
#include "event.h"

#define WIDE2(x) L##x
#define WIDE(x) WIDE2(x)

using namespace paf;
using namespace sce;

const uint32_t k_safeMemIniLimit = 0x400;
const int32_t k_settingsVersion = 4;

static sce::AppSettings *s_appSet = NULL;
static menu::Settings *s_instance = NULL;
static wchar_t *s_verinfo = NULL;

void menu::Settings::Init()
{
	int32_t ret;
	size_t fsize = 0;
	const char *fmime = NULL;
	Plugin::InitParam pluginParam;
	AppSettings::InitParam sparam;

	pluginParam.name = "app_settings_plugin";
	pluginParam.resource_file = "vs0:vsh/common/app_settings_plugin.rco";
	pluginParam.module_file = "vs0:vsh/common/app_settings.suprx";
	pluginParam.caller_name = "__main__";
	pluginParam.set_param_func = AppSettings::PluginSetParamCB;
	pluginParam.init_func = AppSettings::PluginInitCB;
	pluginParam.start_func = AppSettings::PluginStartCB;
	pluginParam.stop_func = AppSettings::PluginStopCB;
	pluginParam.exit_func = AppSettings::PluginExitCB;
	pluginParam.draw_priority = 0x96;

	Plugin::LoadSync(pluginParam);

	sparam.xml_file = g_appPlugin->GetResource()->GetFile(file_netstream_settings, &fsize, &fmime);
	sparam.alloc_cb = sce_paf_malloc;
	sparam.free_cb = sce_paf_free;
	sparam.realloc_cb = sce_paf_realloc;
	sparam.safemem_offset = 0;
	sparam.safemem_size = k_safeMemIniLimit;

	sce::AppSettings::GetInstance(sparam, &s_appSet);

	ret = -1;
	s_appSet->GetInt("settings_version", &ret, 0);
	if (ret != k_settingsVersion) {
		ret = s_appSet->Initialize();
		s_appSet->SetInt("settings_version", k_settingsVersion);
	}

	wstring *verinfo = new wstring();

#ifdef _DEBUG
	*verinfo = L"DEBUG ";
#else
	*verinfo = L"RELEASE ";
#endif
	*verinfo += WIDE(__DATE__);
	*verinfo += L" v 3.00";
	s_verinfo = (wchar_t *)verinfo->c_str();
}

menu::Settings::~Settings()
{
	menu::GenericMenu *topMenu = menu::GetTopMenu();
	if (topMenu) {
		topMenu->Activate();
	}

	event::BroadcastGlobalEvent(g_appPlugin, SettingsEvent, SettingsEvent_Close);

	s_instance = NULL;
}

menu::Settings::Settings()
{
	if (s_instance)
	{
		SCE_DBG_LOG_ERROR("[MENU] Attempt to create second singleton instance\n");
		return;
	}

	menu::GenericMenu *topMenu = menu::GetTopMenu();
	if (topMenu) {
		topMenu->DisableInput();
	}

	AppSettings::InterfaceCallbacks ifCb;

	ifCb.onStartPageTransitionCb = CBOnStartPageTransition;
	ifCb.onPageActivateCb = CBOnPageActivate;
	ifCb.onPageDeactivateCb = CBOnPageDeactivate;
	ifCb.onCheckVisible = CBOnCheckVisible;
	ifCb.onPreCreateCb = CBOnPreCreate;
	ifCb.onPostCreateCb = CBOnPostCreate;
	ifCb.onPressCb = CBOnPress;
	ifCb.onPressCb2 = CBOnPress2;
	ifCb.onTermCb = CBOnTerm;
	ifCb.onGetStringCb = CBOnGetString;
	ifCb.onGetSurfaceCb = CBOnGetSurface;

	Plugin *appSetPlug = paf::Plugin::Find("app_settings_plugin");
	const AppSettings::Interface *appSetIf = (const sce::AppSettings::Interface *)appSetPlug->GetInterface(1);
	appSetIf->Show(&ifCb);

	s_instance = this;
}

void menu::Settings::CBOnStartPageTransition(const char *elementId, int32_t type)
{

}

void menu::Settings::CBOnPageActivate(const char *elementId, int32_t type)
{

}

void menu::Settings::CBOnPageDeactivate(const char *elementId, int32_t type)
{

}

int32_t menu::Settings::CBOnCheckVisible(const char *elementId, bool *pIsVisible)
{
	*pIsVisible = false;

	if (sce_paf_strstr(elementId, "_setting"))
	{
		if (!sce_paf_strcmp(elementId, "verinfo_setting"))
		{
			*pIsVisible = true;
			return SCE_OK;
		}

		int32_t supportedItemsCount = 0;
		uint32_t elementHash = utils::GetHash(elementId);
		const uint32_t *supportedItems = menu::GetTopMenu()->GetSupportedSettingsItems(&supportedItemsCount);

		for (int i = 0; i < supportedItemsCount; i++)
		{
			if (supportedItems[i] == elementHash)
			{
				*pIsVisible = true;
				return SCE_OK;
			}
		}
	}
	else
	{
		*pIsVisible = true;
	}

	return SCE_OK;
}

int32_t menu::Settings::CBOnPreCreate(const char *elementId, sce::AppSettings::Element *element)
{
	return SCE_OK;
}

int32_t menu::Settings::CBOnPostCreate(const char *elementId, paf::ui::Widget *widget)
{
	return SCE_OK;
}

int32_t menu::Settings::CBOnPress(const char *elementId, const char *newValue)
{
	int32_t ret = SCE_OK;
	IDParam elem(elementId);
	IDParam val(newValue);

	event::BroadcastGlobalEvent(g_appPlugin, SettingsEvent, SettingsEvent_ValueChange, elem.GetIDHash(), val.GetIDHash());

	return ret;
}

int32_t menu::Settings::CBOnPress2(const char *elementId, const char *newValue)
{
	return SCE_OK;
}

void menu::Settings::CBOnTerm(int32_t result)
{
	delete s_instance;
}

wchar_t *menu::Settings::CBOnGetString(const char *elementId)
{
	wchar_t *res = g_appPlugin->GetString(elementId);

	if (res[0] == 0)
	{
		if (!sce_paf_strcmp(elementId, "msg_verinfo"))
		{
			return s_verinfo;
		}
	}

	return res;
}

int32_t menu::Settings::CBOnGetSurface(graph::Surface **surf, const char *elementId)
{
	return SCE_OK;
}

menu::Settings *menu::Settings::GetInstance()
{
	return s_instance;
}

AppSettings *menu::Settings::GetAppSetInstance()
{
	return s_appSet;
}