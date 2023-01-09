#include <kernel.h>
#include <paf.h>

#include "common.h"
#include "utils.h"
#include "dialog.h"
#include "beav_player.h"
#include "generic_server_browser.h"
#include "option_menu.h"
#include "menus/menu_server.h"
#include "menus/menu_settings.h"
#include "menus/menu_player_simple.h"

using namespace paf;

SceVoid menu::GenericServerMenu::PlayerBackCb(PlayerSimple *player, ScePVoid pUserArg)
{
	delete player;
}

SceVoid menu::GenericServerMenu::PlayerFailCb(PlayerSimple *player, ScePVoid pUserArg)
{
	dialog::OpenError(g_appPlugin, SCE_ERROR_ERRNO_EUNSUP, utils::GetString("msg_error_load_file"));
	delete player;
}

SceVoid menu::GenericServerMenu::BackButtonCbFun(SceInt32 eventId, ui::Widget *self, SceInt32 a3, ScePVoid pUserData)
{
	GenericServerMenu *workObj = (GenericServerMenu *)pUserData;

	workObj->PopBrowserPage();
	if (workObj->pageList.empty())
	{
		delete workObj;
	}
}

SceVoid menu::GenericServerMenu::ListButtonCbFun(SceInt32 eventId, ui::Widget *self, SceInt32 a3, ScePVoid pUserData)
{
	GenericServerMenu *workObj = (GenericServerMenu *)pUserData;
	BrowserPage *workPage = workObj->pageList.back();
	GenericServerBrowser::Entry *entry = workPage->itemList->at(self->elem.hash);

	if (entry->type == GenericServerBrowser::Entry::Type_Folder)
	{
		workObj->PushBrowserPage(&entry->ref);
	}
	else if (entry->type == GenericServerBrowser::Entry::Type_SupportedFile)
	{
		new menu::PlayerSimple(workObj->browser->GetBEAVUrl(&entry->ref).c_str(), SCE_NULL, PlayerFailCb, PlayerBackCb, pUserData);
	}
}

SceVoid menu::GenericServerMenu::SettingsButtonCbFun(SceInt32 eventId, ui::Widget *self, SceInt32 a3, ScePVoid pUserData)
{
	GenericServerMenu *workObj = (GenericServerMenu *)pUserData;
	
	vector<OptionMenu::Button> buttons;
	OptionMenu::Button bt;
	bt.label = utils::GetString(msg_settings);
	buttons.push_back(bt);

	new OptionMenu(g_appPlugin, workObj->root, &buttons, OptionButtonCb, SCE_NULL);
}

SceVoid menu::GenericServerMenu::OptionButtonCb(SceUInt32 index, ScePVoid pUserData)
{
	menu::SettingsButtonCbFun(ui::EventMain_Decide, SCE_NULL, 0, SCE_NULL);
}

ui::ListItem *menu::GenericServerMenu::ListViewCb::Create(Param *info)
{
	rco::Element searchParam;
	Plugin::TemplateOpenParam tmpParam;
	ui::Widget *item = SCE_NULL;
	graph::Surface *tex = SCE_NULL;
	wstring text16;

	BrowserPage *workPage = workObj->pageList.back();

	GenericServerBrowser::Entry *entry = workPage->itemList->at(info->cellIndex);
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
	case GenericServerBrowser::Entry::Type_UnsupportedFile:
		tex = utils::GetTexture(tex_file_icon_unsupported);
		break;
	case GenericServerBrowser::Entry::Type_SupportedFile:
		tex = utils::GetTexture(tex_file_icon_video);
		break;
	case GenericServerBrowser::Entry::Type_Folder:
		tex = utils::GetTexture(tex_file_icon_folder);
		break;
	}

	button->SetSurfaceBase(&tex);
	tex->Release();

	return (ui::ListItem *)item;
}

SceVoid menu::GenericServerMenu::GoToJob::ConnectionFailedDialogHandler(dialog::ButtonCode buttonCode, ScePVoid pUserArg)
{
	GenericServerMenu *workObj = (GenericServerMenu *)pUserArg;
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

SceVoid menu::GenericServerMenu::GoToJob::Run()
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
		ui::Widget *backButton = utils::GetChild(workObj->root, button_back_page_server_generic);
		ui::Widget *settingsButton = utils::GetChild(workObj->root, button_settings_page_server_generic);
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

menu::GenericServerMenu::GenericServerMenu() :
	GenericMenu("page_server_generic",
	MenuOpenParam(false, 200.0f, Plugin::PageEffectType_SlideFromBottom),
	MenuCloseParam(false, 200.0f, Plugin::PageEffectType_SlideFromBottom))
{
	rco::Element searchParam;
	Plugin::TemplateOpenParam tmpParam;
	firstBoot = SCE_TRUE;

	ui::Widget *settingsButton = utils::GetChild(root, button_settings_page_server_generic);
	settingsButton->PlayEffectReverse(0.0f, effect::EffectType_Reset);
	settingsButton->RegisterEventCallback(ui::EventMain_Decide, new utils::SimpleEventCallback(SettingsButtonCbFun, this));

	ui::Widget *backButton = utils::GetChild(root, button_back_page_server_generic);
	backButton->PlayEffectReverse(0.0f, effect::EffectType_Reset);
	backButton->RegisterEventCallback(ui::EventMain_Decide, new utils::SimpleEventCallback(BackButtonCbFun, this));

	browserRoot = utils::GetChild(root, plane_browser_root_page_server_generic);
	loaderIndicator = (ui::BusyIndicator *)utils::GetChild(root, busyindicator_loader_page_server_generic);
	topText = (ui::Text *)utils::GetChild(root, text_top);
}

menu::GenericServerMenu::~GenericServerMenu()
{
	while (PopBrowserPage())
	{

	}

	delete browser;
}

SceBool menu::GenericServerMenu::PushBrowserPage(string *ref)
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

SceBool menu::GenericServerMenu::PopBrowserPage()
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