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

#define WIDE2(x) L##x
#define WIDE(x) WIDE2(x)

using namespace paf;
using namespace sce;

const SceUInt32 k_safeMemIniLimit = 0x400;
const SceInt32 k_settingsVersion = 1;

static sce::AppSettings *s_appSet = SCE_NULL;
static menu::Settings *s_instance = SCE_NULL;
static wchar_t *s_verinfo = SCE_NULL;

static menu::Settings::ValueChangeCallback s_valChangeCb = SCE_NULL;
static ScePVoid s_valChangeCbUserArg = SCE_NULL;

SceVoid menu::Settings::Init()
{
	SceInt32 ret;
	SceSize fsize = 0;
	const char *fmime = SCE_NULL;
	Plugin::InitParam pluginParam;
	AppSettings::InitParam sparam;

	pluginParam.pluginName = "app_settings_plugin";
	pluginParam.resourcePath = "vs0:vsh/common/app_settings_plugin.rco";
	pluginParam.scopeName = "__main__";

	pluginParam.pluginSetParamCB = AppSettings::PluginCreateCB;
	pluginParam.pluginInitCB = AppSettings::PluginInitCB;
	pluginParam.pluginStartCB = AppSettings::PluginStartCB;
	pluginParam.pluginStopCB = AppSettings::PluginStopCB;
	pluginParam.pluginExitCB = AppSettings::PluginExitCB;
	pluginParam.pluginPath = "vs0:vsh/common/app_settings.suprx";
	pluginParam.unk_58 = 0x96;

	s_frameworkInstance->LoadPlugin(&pluginParam);

	sparam.xmlFile = g_appPlugin->resource->GetFile(file_netstream_settings, &fsize, &fmime);
	sparam.allocCB = sce_paf_malloc;
	sparam.freeCB = sce_paf_free;
	sparam.reallocCB = sce_paf_realloc;
	sparam.safeMemoryOffset = 0;
	sparam.safeMemorySize = k_safeMemIniLimit;

	sce::AppSettings::GetInstance(&sparam, &s_appSet);

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
	*verinfo += L" v 1.01";
	s_verinfo = (wchar_t *)verinfo->c_str();
}

menu::Settings::~Settings()
{
	menu::GenericMenu *topMenu = menu::GetTopMenu();
	if (topMenu) {
		ui::Widget::SetControlFlags(topMenu->root, 1);
	}

	if (closeCb) {
		closeCb(closeCbUserArg);
	}

	s_instance = SCE_NULL;
}

menu::Settings::Settings()
{
	closeCb = SCE_NULL;
	closeCbUserArg = SCE_NULL;

	if (s_instance)
	{
		SCE_DBG_LOG_ERROR("[MENU] Attempt to create second singleton instance\n");
		return;
	}

	menu::GenericMenu *topMenu = menu::GetTopMenu();
	if (topMenu) {
		ui::Widget::SetControlFlags(topMenu->root, 0);
	}

	AppSettings::InterfaceCallbacks ifCb;

	ifCb.listChangeCb = CBListChange;
	ifCb.listForwardChangeCb = CBListForwardChange;
	ifCb.listBackChangeCb = CBListBackChange;
	ifCb.isVisibleCb = CBIsVisible;
	ifCb.elemInitCb = CBElemInit;
	ifCb.elemAddCb = CBElemAdd;
	ifCb.valueChangeCb = CBValueChange;
	ifCb.valueChangeCb2 = CBValueChange2;
	ifCb.termCb = CBTerm;
	ifCb.getStringCb = CBGetString;
	ifCb.getTexCb = CBGetTex;

	Plugin *appSetPlug = paf::Plugin::Find("app_settings_plugin");
	AppSettings::Interface *appSetIf = (sce::AppSettings::Interface *)appSetPlug->GetInterface(1);
	appSetIf->Show(&ifCb);

	s_instance = this;
}

SceVoid menu::Settings::SetCloseCallback(CloseCallback cb, ScePVoid uarg)
{
	closeCb = cb;
	closeCbUserArg = uarg;
}

SceVoid menu::Settings::SetValueChangeCallback(ValueChangeCallback cb, ScePVoid uarg)
{
	s_valChangeCb = cb;
	s_valChangeCbUserArg = uarg;
}

SceVoid menu::Settings::CBListChange(const char *elementId)
{

}

SceVoid menu::Settings::CBListForwardChange(const char *elementId)
{

}

SceVoid menu::Settings::CBListBackChange(const char *elementId)
{

}

SceInt32 menu::Settings::CBIsVisible(const char *elementId, SceBool *pIsVisible)
{
	*pIsVisible = SCE_FALSE;

	if (sce_paf_strstr(elementId, "_setting"))
	{
		if (!sce_paf_strcmp(elementId, "verinfo_setting"))
		{
			*pIsVisible = SCE_TRUE;
			return SCE_OK;
		}

		SceInt32 supportedItemsCount = 0;
		SceUInt32 elementHash = utils::GetHash(elementId);
		const SceUInt32 *supportedItems = menu::GetTopMenu()->GetSupportedSettingsItems(&supportedItemsCount);

		for (int i = 0; i < supportedItemsCount; i++)
		{
			if (supportedItems[i] == elementHash)
			{
				*pIsVisible = SCE_TRUE;
				return SCE_OK;
			}
		}
	}
	else
	{
		*pIsVisible = SCE_TRUE;
	}

	return SCE_OK;
}

SceInt32 menu::Settings::CBElemInit(const char *elementId)
{
	return SCE_OK;
}

SceInt32 menu::Settings::CBElemAdd(const char *elementId, paf::ui::Widget *widget)
{
	return SCE_OK;
}

SceInt32 menu::Settings::CBValueChange(const char *elementId, const char *newValue)
{
	SceInt32 ret = SCE_OK;

	if (s_valChangeCb)
	{
		return s_valChangeCb(elementId, newValue, s_valChangeCbUserArg);
	}

	return ret;
}

SceInt32 menu::Settings::CBValueChange2(const char *elementId, const char *newValue)
{
	return SCE_OK;
}

SceVoid menu::Settings::CBTerm()
{
	delete s_instance;
}

wchar_t *menu::Settings::CBGetString(const char *elementId)
{
	rco::Element searchParam;
	wchar_t *res = SCE_NULL;

	searchParam.hash = utils::GetHash(elementId);

	res = g_appPlugin->GetWString(&searchParam);

	if (res[0] == 0)
	{
		if (!sce_paf_strcmp(elementId, "msg_verinfo"))
		{
			return s_verinfo;
		}
	}

	return res;
}

SceInt32 menu::Settings::CBGetTex(graph::Surface **tex, const char *elementId)
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