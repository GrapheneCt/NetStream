#include <kernel.h>
#include <paf.h>

#include "common.h"
#include "utils.h"
#include "dialog.h"
#include "beav_player.h"
#include "http_server_browser.h"
#include "menus/menu_http.h"
#include "menus/menu_settings.h"
#include "menus/menu_player_simple.h"

using namespace paf;

SceVoid menu::Http::PlayerBackCb(PlayerSimple *player, ScePVoid pUserArg)
{
	delete player;
}

SceVoid menu::Http::PlayerFailCb(PlayerSimple *player, ScePVoid pUserArg)
{
	dialog::OpenError(g_appPlugin, SCE_ERROR_ERRNO_EUNSUP, utils::GetString("msg_error_load_file"));
	delete player;
}

SceVoid menu::Http::BackButtonCbFun(SceInt32 eventId, ui::Widget *self, SceInt32 a3, ScePVoid pUserData)
{
	Http *workObj = (Http *)pUserData;

	workObj->PopBrowserPage();
	if (workObj->pageList.empty())
	{
		delete workObj;
	}
}

SceVoid menu::Http::ListButtonCbFun(SceInt32 eventId, ui::Widget *self, SceInt32 a3, ScePVoid pUserData)
{
	Http *workObj = (Http *)pUserData;
	BrowserPage *workPage = workObj->pageList.back();
	HttpServerBrowser::Entry *entry = workPage->itemList->at(self->elem.hash);

	if (entry->type == HttpServerBrowser::Entry::Type_Folder)
	{
		workObj->PushBrowserPage(&entry->ref);
	}
	else if (entry->type == HttpServerBrowser::Entry::Type_SupportedFile)
	{
		new menu::PlayerSimple(workObj->browser->GetBEAVUrl(&entry->ref).c_str(), SCE_NULL, PlayerFailCb, PlayerBackCb, pUserData);
	}
}

ui::ListItem *menu::Http::ListViewCb::Create(Param *info)
{
	rco::Element searchParam;
	Plugin::TemplateOpenParam tmpParam;
	ui::Widget *item = SCE_NULL;
	graph::Surface *tex = SCE_NULL;
	wstring text16;

	BrowserPage *workPage = workObj->pageList.back();

	HttpServerBrowser::Entry *entry = workPage->itemList->at(info->cellIndex);
	ui::Widget *targetRoot = info->parent;

	searchParam.hash = template_list_item_generic;
	g_appPlugin->TemplateOpen(targetRoot, &searchParam, &tmpParam);
	item = targetRoot->GetChild(targetRoot->childNum - 1);

	ui::Widget *button = utils::GetChild(item, image_button_list_item);
	button->elem.hash = info->cellIndex;

	ccc::UTF8toUTF16(&entry->ref, &text16);
	button->SetLabel(&text16);
	button->RegisterEventCallback(ui::EventMain_Decide, new utils::SimpleEventCallback(ListButtonCbFun, workObj));

	switch (entry->type)
	{
	case HttpServerBrowser::Entry::Type_UnsupportedFile:
		tex = utils::GetTexture(tex_file_icon_unsupported);
		break;
	case HttpServerBrowser::Entry::Type_SupportedFile:
		tex = utils::GetTexture(tex_file_icon_video);
		break;
	case HttpServerBrowser::Entry::Type_Folder:
		tex = utils::GetTexture(tex_file_icon_folder);
		break;
	}

	button->SetSurfaceBase(&tex);
	tex->Release();

	return (ui::ListItem *)item;
}

SceVoid menu::Http::GoToJob::ConnectionFailedDialogHandler(dialog::ButtonCode buttonCode, ScePVoid pUserArg)
{
	Http *workObj = (Http *)pUserArg;
	workObj->PopBrowserPage();
	menu::SettingsButtonCbFun(ui::EventMain_Decide, SCE_NULL, 0, SCE_NULL);
	if (workObj->pageList.empty())
	{
		delete workObj;
	}
	menu::GenericMenu *topMenu = menu::GetTopMenu();
	if (topMenu) {
		ui::Widget::SetControlFlags(topMenu->root, 0);
	}
}

SceVoid menu::Http::GoToJob::Run()
{
	SceInt32 ret = SCE_OK;
	string currentPath;
	wstring text16;

	thread::s_mainThreadMutex.Lock();
	workObj->loaderIndicator->Start();
	thread::s_mainThreadMutex.Unlock();

	BrowserPage *workPage = workObj->pageList.back();

	if (targetRef.length() == 0)
	{
		workPage->itemList = workObj->browser->GoTo(SCE_NULL, &ret);
	}
	else
	{
		workPage->itemList = workObj->browser->GoTo(targetRef.c_str(), &ret);
	}
	if (ret != SCE_OK)
	{
		thread::s_mainThreadMutex.Lock();
		workObj->loaderIndicator->Stop();
		thread::s_mainThreadMutex.Unlock();
		workPage->isLoaded = SCE_TRUE;
		dialog::OpenError(g_appPlugin, ret, utils::GetString("msg_error_connect_server_peer"), ConnectionFailedDialogHandler, workObj);
		return;
	}
	if (workObj->firstBoot)
	{
		ui::Widget *backButton = utils::GetChild(workObj->root, button_back_page_http);
		ui::Widget *settingsButton = utils::GetChild(workObj->root, button_settings_page_http);
		thread::s_mainThreadMutex.Lock();
		backButton->PlayEffect(0.0f, effect::EffectType_Reset);
		settingsButton->PlayEffect(0.0f, effect::EffectType_Reset);
		thread::s_mainThreadMutex.Unlock();
		workObj->firstBoot = SCE_FALSE;
	}

	currentPath = workObj->browser->GetPath();
	ccc::UTF8toUTF16(&currentPath, &text16);

	thread::s_mainThreadMutex.Lock();
	workObj->loaderIndicator->Stop();
	workPage->list->AddItem(0, 0, workPage->itemList->size());
	workObj->topText->SetLabel(&text16);
	thread::s_mainThreadMutex.Unlock();

	workPage->isLoaded = SCE_TRUE;
}

menu::Http::Http() :
	GenericMenu("page_http",
	MenuOpenParam(false, 200.0f, Plugin::PageEffectType_SlideFromBottom),
	MenuCloseParam(false, 200.0f, Plugin::PageEffectType_SlideFromBottom))
{
	rco::Element searchParam;
	Plugin::TemplateOpenParam tmpParam;
	firstBoot = SCE_TRUE;

	ui::Widget *settingsButton = utils::GetChild(root, button_settings_page_http);
	settingsButton->PlayEffectReverse(0.0f, effect::EffectType_Reset);
	settingsButton->RegisterEventCallback(ui::EventMain_Decide, new utils::SimpleEventCallback(menu::SettingsButtonCbFun));

	ui::Widget *backButton = utils::GetChild(root, button_back_page_http);
	backButton->PlayEffectReverse(0.0f, effect::EffectType_Reset);
	backButton->RegisterEventCallback(ui::EventMain_Decide, new utils::SimpleEventCallback(BackButtonCbFun, this));

	browserRoot = utils::GetChild(root, plane_browser_root_page_http);
	loaderIndicator = (ui::BusyIndicator *)utils::GetChild(root, busyindicator_loader_page_http);
	topText = (ui::Text *)utils::GetChild(root, text_top);

	sce::AppSettings *settings = menu::Settings::GetAppSetInstance();

	char host[256];
	char port[32];
	char user[256];
	char password[256];

	settings->GetString("http_host", host, sizeof(host), "");
	settings->GetString("http_port", port, sizeof(port), "");
	settings->GetString("http_user", user, sizeof(user), "");
	settings->GetString("http_password", password, sizeof(password), "");

	browser = new HttpServerBrowser(host, port, user, password);
}

menu::Http::~Http()
{
	while (PopBrowserPage())
	{

	}

	delete browser;
}

SceBool menu::Http::PushBrowserPage(string *ref)
{
	rco::Element searchParam;
	Plugin::TemplateOpenParam tmpParam;

	BrowserPage *page = new BrowserPage();

	searchParam.hash = template_list_view_generic;
	g_appPlugin->TemplateOpen(browserRoot, &searchParam, &tmpParam);
	page->list = (ui::ListView *)browserRoot->GetChild(browserRoot->childNum - 1);
	ListViewCb *lwCb = new ListViewCb();
	lwCb->workObj = this;
	page->list->RegisterItemCallback(lwCb);
	page->list->SetSegmentEnable(0, 1);
	Vector4 sz(960.0f, 80.0f);
	page->list->SetCellSize(0, &sz);
	page->list->SetConfigurationType(0, ui::ListView::ConfigurationType_Simple);

	BrowserPage *wp;
	if (!pageList.empty()) {
		if (pageList.size() > 1) {
			wp = pageList.rbegin()[1];
			wp->list->PlayEffectReverse(0.0f, effect::EffectType_Reset);
			if (wp->list->animationStatus & 0x80)
				wp->list->animationStatus &= ~0x80;
		}
		wp = pageList.back();
		wp->list->PlayEffect(0.0f, effect::EffectType_3D_SlideToBack1);
		if (wp->list->animationStatus & 0x80)
			wp->list->animationStatus &= ~0x80;
	}

	pageList.push_back(page);

	GoToJob *job = new GoToJob("Http::GoToJob");
	job->workObj = this;
	if (ref)
	{
		job->targetRef = *ref;
	}
	SharedPtr<job::JobItem> itemParam(job);
	utils::GetJobQueue()->Enqueue(&itemParam);

	return SCE_TRUE;
}

SceBool menu::Http::PopBrowserPage()
{
	BrowserPage *pageToPop;
	SceBool isLastPage = SCE_FALSE;

	if (pageList.empty())
	{
		return SCE_FALSE;
	}
	else if (pageList.size() == 1)
	{
		isLastPage = SCE_TRUE;
	}

	pageToPop = pageList.back();
	while (!pageToPop->isLoaded)
	{
		thread::Sleep(10);
	}

	if (!isLastPage)
	{
		effect::Play(-100.0f, pageToPop->list, effect::EffectType_3D_SlideFromFront, true, false);
	}

	for (int i = 0; i < pageToPop->itemList->size(); i++)
	{
		delete pageToPop->itemList->at(i);
	}

	delete pageToPop->itemList;
	delete pageToPop;
	pageList.pop_back();

	if (!isLastPage)
	{
		BrowserPage *wp;
		if (!pageList.empty()) {
			wp = pageList.back();
			wp->list->PlayEffectReverse(0.0f, effect::EffectType_3D_SlideToBack1);
			wp->list->PlayEffect(0.0f, effect::EffectType_Reset);
			if (wp->list->animationStatus & 0x80)
				wp->list->animationStatus &= ~0x80;
			if (pageList.size() > 1) {
				wp = pageList.rbegin()[1];
				wp->list->PlayEffect(0.0f, effect::EffectType_Reset);
				if (wp->list->animationStatus & 0x80)
					wp->list->animationStatus &= ~0x80;
			}
		}
	}

	if (!isLastPage)
	{
		browser->SetPath("..");
		string currentPath = browser->GetPath();
		wstring text16;
		ccc::UTF8toUTF16(&currentPath, &text16);
		topText->SetLabel(&text16);
	}

	return SCE_TRUE;
}