#include <kernel.h>
#include <paf.h>

#include "common.h"
#include "utils.h"
#include "dialog.h"
#include "player_beav.h"
#include "generic_server_browser.h"
#include "option_menu.h"
#include "menus/menu_server.h"
#include "menus/menu_settings.h"
#include "menus/menu_player_simple.h"

using namespace paf;

void menu::GenericServerMenu::PlayerEventCbFun(int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
{
	GenericServerMenu *workObj = (GenericServerMenu *)userdata;

	switch (e->GetValue(0))
	{
	case PlayerSimple::PlayerEvent_Back:
		delete workObj->player;
		break;
	case PlayerSimple::PlayerEvent_InitOk:

		break;
	case PlayerSimple::PlayerEvent_InitFail:
		workObj->playerFailed = true;
		dialog::OpenError(g_appPlugin, SCE_ERROR_ERRNO_EUNSUP, Framework::Instance()->GetCommonString("msg_error_load_file"));
		delete workObj->player;
		break;
	}
}

void menu::GenericServerMenu::BackButtonCbFun(int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
{
	GenericServerMenu *workObj = (GenericServerMenu *)userdata;

	workObj->PopBrowserPage();
	if (workObj->pageList.empty())
	{
		delete workObj;
	}
}

void menu::GenericServerMenu::PlayerCreateTimeoutFun(void *userdata1, void *userdata2)
{
	GenericServerMenu *workObj = (GenericServerMenu *)userdata1;
	GenericServerBrowser::Entry *entry = (GenericServerBrowser::Entry *)userdata2;
	workObj->player = new menu::PlayerSimple(workObj->browser->GetBEAVUrl(&entry->ref).c_str());
}

void menu::GenericServerMenu::ListButtonCbFun(int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
{
	GenericServerMenu *workObj = (GenericServerMenu *)userdata;
	ui::Widget *wdg = (ui::Widget*)self;
	BrowserPage *workPage = workObj->pageList.back();
	GenericServerBrowser::Entry *entry = workPage->itemList->at(wdg->GetName().GetIDHash());

	if (entry->type == GenericServerBrowser::Entry::Type_Folder || entry->type == GenericServerBrowser::Entry::Type_PlaylistFile)
	{
		workObj->PushBrowserPage(&entry->ref);
	}
	else if (entry->type == GenericServerBrowser::Entry::Type_SupportedFile)
	{
		if (SCE_PAF_IS_DOLCE)
		{
			utils::SetDisplayResolution(ui::EnvironmentParam::RESOLUTION_HD_FULL);
			utils::SetTimeout(PlayerCreateTimeoutFun, 10.0f, workObj, entry);
		}
		else
		{
			workObj->player = new menu::PlayerSimple(workObj->browser->GetBEAVUrl(&entry->ref).c_str());
		}
	}
}

void menu::GenericServerMenu::SettingsButtonCbFun(int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
{
	GenericServerMenu *workObj = (GenericServerMenu *)userdata;
	
	vector<OptionMenu::Button> buttons;
	OptionMenu::Button bt;
	bt.label = g_appPlugin->GetString(msg_settings);
	buttons.push_back(bt);

	new OptionMenu(g_appPlugin, workObj->root, &buttons);
}

void menu::GenericServerMenu::OptionMenuEventCbFun(int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
{
	if (e->GetValue(0) == OptionMenu::OptionMenuEvent_Close)
	{
		return;
	}

	menu::SettingsButtonCbFun(ui::Button::CB_BTN_DECIDE, NULL, 0, NULL);
}

ui::ListItem *menu::GenericServerMenu::ListViewCb::Create(CreateParam& param)
{
	Plugin::TemplateOpenParam tmpParam;
	ui::Widget *item = NULL;
	wstring text16;

	if (!param.list_view->GetName().GetIDHash())
	{
		return new ui::ListItem(param.parent, 0);
	}

	BrowserPage *workPage = workObj->pageList.back();

	GenericServerBrowser::Entry *entry = workPage->itemList->at(param.cell_index);
	ui::Widget *targetRoot = param.parent;

	g_appPlugin->TemplateOpen(targetRoot, template_list_item_generic, tmpParam);
	item = targetRoot->GetChild(targetRoot->GetChildrenNum() - 1);

	ui::Widget *button = item->FindChild(image_button_list_item);
	button->SetName(param.cell_index);

	common::Utf8ToUtf16(entry->ref, &text16);
	button->SetString(text16);
	button->AddEventCallback(ui::Button::CB_BTN_DECIDE, ListButtonCbFun, workObj);

	uint32_t texid = 0;
	switch (entry->type)
	{
	default:
	case GenericServerBrowser::Entry::Type_UnsupportedFile:
		texid = tex_file_icon_unsupported;
		break;
	case GenericServerBrowser::Entry::Type_SupportedFile:
		texid = tex_file_icon_video;
		break;
	case GenericServerBrowser::Entry::Type_Folder:
		texid = tex_file_icon_folder;
		break;
	case GenericServerBrowser::Entry::Type_PlaylistFile:
		texid = tex_file_icon_playlist;
		break;
	}

	intrusive_ptr<graph::Surface> tex = g_appPlugin->GetTexture(texid);
	if (tex.get())
	{
		button->SetTexture(tex);
	}

	return (ui::ListItem *)item;
}

void menu::GenericServerMenu::ConnectionFailedDialogHandler(int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
{
	GenericServerMenu *workObj = (GenericServerMenu *)userdata;
	if (workObj->playerFailed)
	{
		workObj->playerFailed = false;
		return;
	}

	workObj->PopBrowserPage();
	menu::SettingsButtonCbFun(ui::Button::CB_BTN_DECIDE, NULL, 0, NULL);
	if (workObj->pageList.empty())
	{
		delete workObj;
	}
	menu::GenericMenu *topMenu = menu::GetTopMenu();
	if (topMenu)
	{
		topMenu->DisableInput();
	}
}

void menu::GenericServerMenu::GoToJob::Run()
{
	int32_t ret = SCE_OK;
	string currentPath;
	wstring text16;

	thread::RMutex::main_thread_mutex.Lock();
	workObj->loaderIndicator->Start();
	thread::RMutex::main_thread_mutex.Unlock();

	BrowserPage *workPage = workObj->pageList.back();

	if (targetRef.length() == 0)
	{
		workPage->itemList = workObj->browser->GoTo(NULL, &ret);
	}
	else
	{
		workPage->itemList = workObj->browser->GoTo(targetRef.c_str(), &ret);
	}
	if (ret != SCE_OK)
	{
		thread::RMutex::main_thread_mutex.Lock();
		workObj->loaderIndicator->Stop();
		thread::RMutex::main_thread_mutex.Unlock();
		workPage->isLoaded = true;
		dialog::OpenError(g_appPlugin, ret, Framework::Instance()->GetCommonString("msg_error_connect_server_peer"));
		return;
	}
	if (workObj->firstBoot)
	{
		ui::Widget *backButton = workObj->root->FindChild(button_back_page_server_generic);
		ui::Widget *settingsButton = workObj->root->FindChild(button_settings_page_server_generic);
		thread::RMutex::main_thread_mutex.Lock();
		backButton->Show(common::transition::Type_Reset);
		settingsButton->Show(common::transition::Type_Reset);
		thread::RMutex::main_thread_mutex.Unlock();
		workObj->firstBoot = false;
	}

	currentPath = workObj->browser->GetPath();
	if (workObj->GetMenuType() == MenuType_Local)
	{
		utils::SafememWrite(currentPath);
	}
	common::Utf8ToUtf16(currentPath, &text16);

	thread::RMutex::main_thread_mutex.Lock();
	workObj->loaderIndicator->Stop();
	workPage->list->InsertCell(0, 0, workPage->itemList->size());
	workObj->topText->SetString(text16);
	thread::RMutex::main_thread_mutex.Unlock();

	workPage->isLoaded = true;
}

menu::GenericServerMenu::GenericServerMenu() :
	GenericMenu("page_server_generic",
	MenuOpenParam(false, 200.0f, Plugin::TransitionType_SlideFromBottom),
	MenuCloseParam(false, 200.0f, Plugin::TransitionType_SlideFromBottom))
{
	Plugin::TemplateOpenParam tmpParam;
	firstBoot = true;
	playerFailed = false;

	root->AddEventCallback(dialog::DialogEvent, ConnectionFailedDialogHandler, this);
	root->AddEventCallback(OptionMenu::OptionMenuEvent, OptionMenuEventCbFun, this);
	root->AddEventCallback(PlayerSimple::PlayerSimpleEvent, PlayerEventCbFun, this);

	ui::Widget *settingsButton = root->FindChild(button_settings_page_server_generic);
	settingsButton->Hide(common::transition::Type_Reset);
	settingsButton->AddEventCallback(ui::Button::CB_BTN_DECIDE, SettingsButtonCbFun, this);

	ui::Widget *backButton = root->FindChild(button_back_page_server_generic);
	backButton->Hide(common::transition::Type_Reset);
	backButton->AddEventCallback(ui::Button::CB_BTN_DECIDE, BackButtonCbFun, this);

	browserRoot = root->FindChild(plane_browser_root_page_server_generic);
	loaderIndicator = (ui::BusyIndicator *)root->FindChild(busyindicator_loader_page_server_generic);
	topText = (ui::Text *)root->FindChild(text_top);
}

menu::GenericServerMenu::~GenericServerMenu()
{
	while (PopBrowserPage())
	{

	}

	delete browser;
}

bool menu::GenericServerMenu::PushBrowserPage(string *ref)
{
	Plugin::TemplateOpenParam tmpParam;

	BrowserPage *page = new BrowserPage();

	g_appPlugin->TemplateOpen(browserRoot, template_list_view_generic, tmpParam);
	page->list = (ui::ListView *)browserRoot->GetChild(browserRoot->GetChildrenNum() - 1);
	ListViewCb *lwCb = new ListViewCb();
	lwCb->workObj = this;
	page->list->SetItemFactory(lwCb);
	page->list->InsertSegment(0, 1);
	math::v4 sz(960.0f, 80.0f);
	page->list->SetCellSizeDefault(0, sz);
	page->list->SetSegmentLayoutType(0, ui::ListView::LAYOUT_TYPE_LIST);

	BrowserPage *wp;
	if (!pageList.empty()) {
		if (pageList.size() > 1) {
			wp = pageList.rbegin()[1];
			wp->list->Hide(common::transition::Type_Reset);
			wp->list->SetTransitionComplete(false);
		}
		wp = pageList.back();
		wp->list->Show(common::transition::Type_3D_SlideToBack1);
		wp->list->SetTransitionComplete(false);
	}

	pageList.push_back(page);

	GoToJob *job = new GoToJob("Http::GoToJob");
	job->workObj = this;
	if (ref)
	{
		job->targetRef = *ref;
	}
	common::SharedPtr<job::JobItem> itemParam(job);
	utils::GetJobQueue()->Enqueue(itemParam);

	return true;
}

bool menu::GenericServerMenu::PopBrowserPage()
{
	BrowserPage *pageToPop;
	bool isLastPage = false;

	if (pageList.empty())
	{
		return false;
	}
	else if (pageList.size() == 1)
	{
		isLastPage = true;
	}

	pageToPop = pageList.back();
	while (!pageToPop->isLoaded)
	{
		thread::Sleep(10);
	}

	pageToPop->list->SetName((uint32_t)0);

	if (!isLastPage)
	{
		common::transition::DoReverse(-100.0f, pageToPop->list, common::transition::Type_3D_SlideFromFront, true, false);
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
			wp->list->Hide(common::transition::Type_3D_SlideToBack1);
			wp->list->Show(common::transition::Type_Reset);
			wp->list->SetTransitionComplete(false);
			if (pageList.size() > 1) {
				wp = pageList.rbegin()[1];
				wp->list->Show(common::transition::Type_Reset);
				wp->list->SetTransitionComplete(false);
			}
		}
	}

	if (!isLastPage)
	{
		browser->SetPath("..");
		string currentPath = browser->GetPath();
		wstring text16;
		common::Utf8ToUtf16(currentPath, &text16);
		topText->SetString(text16);
	}

	return true;
}