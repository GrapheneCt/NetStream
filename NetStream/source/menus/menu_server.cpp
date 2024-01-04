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

void menu::GenericServerMenu::OnPlayerEvent(int32_t type)
{
	switch (type)
	{
	case PlayerSimple::PlayerEvent_Back:
		delete m_player;
		break;
	case PlayerSimple::PlayerEvent_InitOk:

		break;
	case PlayerSimple::PlayerEvent_InitFail:
		m_playerFailed = true;
		dialog::OpenError(g_appPlugin, SCE_ERROR_ERRNO_EUNSUP, Framework::Instance()->GetCommonString("msg_error_load_file"));
		delete m_player;
		break;
	}
}

void menu::GenericServerMenu::OnBackButton()
{
	PopBrowserPage();
	if (m_pageList.empty())
	{
		delete this;
	}
}

void menu::GenericServerMenu::OnPlayerCreateTimeout(GenericServerBrowser::Entry *entry)
{
	m_player = new menu::PlayerSimple(m_browser->GetBEAVUrl(entry->ref).c_str());
}

void menu::GenericServerMenu::OnListButton(ui::Widget *wdg)
{
	BrowserPage *workPage = m_pageList.back();
	GenericServerBrowser::Entry *entry = workPage->m_itemList->at(wdg->GetName().GetIDHash());

	if (entry->type == GenericServerBrowser::Entry::Type_Folder || entry->type == GenericServerBrowser::Entry::Type_PlaylistFile)
	{
		PushBrowserPage(&entry->ref);
	}
	else if (entry->type == GenericServerBrowser::Entry::Type_SupportedFile)
	{
		if (SCE_PAF_IS_DOLCE)
		{
			utils::SetDisplayResolution(ui::EnvironmentParam::RESOLUTION_HD_FULL);
			utils::SetTimeout(
			[](void *userdata1, void *userdata2)
			{
				reinterpret_cast<menu::GenericServerMenu *>(userdata1)->OnPlayerCreateTimeout(reinterpret_cast<GenericServerBrowser::Entry *>(userdata2));
			}, 10.0f, this, entry);
		}
		else
		{
			m_player = new menu::PlayerSimple(m_browser->GetBEAVUrl(entry->ref).c_str());
		}
	}
}

void menu::GenericServerMenu::OnSettingsButton()
{
	vector<OptionMenu::Button> buttons;
	OptionMenu::Button bt;
	bt.label = g_appPlugin->GetString(msg_settings);
	buttons.push_back(bt);

	new OptionMenu(g_appPlugin, m_root, &buttons);
}

void menu::GenericServerMenu::OnOptionMenuEvent(int32_t type)
{
	if (type == OptionMenu::OptionMenuEvent_Close)
	{
		return;
	}

	menu::SettingsButtonCbFun(ui::Button::CB_BTN_DECIDE, NULL, 0, NULL);
}

ui::ListItem *menu::GenericServerMenu::CreateListItem(ui::listview::ItemFactory::CreateParam& param)
{
	Plugin::TemplateOpenParam tmpParam;
	ui::Widget *item = NULL;
	wstring text16;

	if (!param.list_view->GetName().GetIDHash())
	{
		return new ui::ListItem(param.parent, 0);
	}

	BrowserPage *workPage = m_pageList.back();

	GenericServerBrowser::Entry *entry = workPage->m_itemList->at(param.cell_index);
	ui::Widget *targetRoot = param.parent;

	g_appPlugin->TemplateOpen(targetRoot, template_list_item_generic, tmpParam);
	item = targetRoot->GetChild(targetRoot->GetChildrenNum() - 1);

	ui::Widget *button = item->FindChild(image_button_list_item);
	button->SetName(param.cell_index);

	common::Utf8ToUtf16(entry->ref, &text16);
	button->SetString(text16);
	button->AddEventCallback(ui::Button::CB_BTN_DECIDE,
	[](int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
	{
		reinterpret_cast<menu::GenericServerMenu *>(userdata)->OnListButton(static_cast<ui::Widget *>(self));
	}, this);

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

	return static_cast<ui::ListItem *>(item);
}

void menu::GenericServerMenu::OnConnectionFailedDialogEvent()
{
	if (m_playerFailed)
	{
		m_playerFailed = false;
		return;
	}

	PopBrowserPage();
	menu::SettingsButtonCbFun(ui::Button::CB_BTN_DECIDE, NULL, 0, NULL);
	if (m_pageList.empty())
	{
		delete this;
	}
	menu::GenericMenu *topMenu = menu::GetTopMenu();
	if (topMenu)
	{
		topMenu->DisableInput();
	}
}

void menu::GenericServerMenu::GoTo(const string& ref)
{
	int32_t ret = SCE_OK;
	string currentPath;
	wstring text16;

	thread::RMutex::main_thread_mutex.Lock();
	m_loaderIndicator->Start();
	thread::RMutex::main_thread_mutex.Unlock();

	BrowserPage *workPage = m_pageList.back();

	if (ref.length() == 0)
	{
		workPage->m_itemList = m_browser->GoTo(NULL, &ret);
	}
	else
	{
		workPage->m_itemList = m_browser->GoTo(ref.c_str(), &ret);
	}
	if (ret != SCE_OK)
	{
		thread::RMutex::main_thread_mutex.Lock();
		m_loaderIndicator->Stop();
		thread::RMutex::main_thread_mutex.Unlock();
		workPage->m_isLoaded = true;
		dialog::OpenError(g_appPlugin, ret, Framework::Instance()->GetCommonString("msg_error_connect_server_peer"));
		return;
	}
	if (m_firstBoot)
	{
		ui::Widget *backButton = m_root->FindChild(button_back_page_server_generic);
		ui::Widget *settingsButton = m_root->FindChild(button_settings_page_server_generic);
		thread::RMutex::main_thread_mutex.Lock();
		backButton->Show(common::transition::Type_Reset);
		settingsButton->Show(common::transition::Type_Reset);
		thread::RMutex::main_thread_mutex.Unlock();
		m_firstBoot = false;
	}

	currentPath = m_browser->GetPath();
	if (GetMenuType() == MenuType_Local)
	{
		utils::SafememWrite(currentPath);
	}
	common::Utf8ToUtf16(currentPath, &text16);

	thread::RMutex::main_thread_mutex.Lock();
	m_loaderIndicator->Stop();
	workPage->m_list->InsertCell(0, 0, workPage->m_itemList->size());
	m_topText->SetString(text16);
	thread::RMutex::main_thread_mutex.Unlock();

	workPage->m_isLoaded = true;
}

menu::GenericServerMenu::GenericServerMenu() :
	GenericMenu("page_server_generic",
	MenuOpenParam(false, 200.0f, Plugin::TransitionType_SlideFromBottom),
	MenuCloseParam(false, 200.0f, Plugin::TransitionType_SlideFromBottom))
{
	Plugin::TemplateOpenParam tmpParam;
	m_firstBoot = true;
	m_playerFailed = false;

	m_root->AddEventCallback(dialog::DialogEvent,
	[](int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
	{
		reinterpret_cast<menu::GenericServerMenu *>(userdata)->OnConnectionFailedDialogEvent();
	}, this);

	m_root->AddEventCallback(OptionMenu::OptionMenuEvent,
	[](int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
	{
		reinterpret_cast<menu::GenericServerMenu *>(userdata)->OnOptionMenuEvent(e->GetValue(0));
	}, this);

	m_root->AddEventCallback(PlayerSimple::PlayerSimpleEvent,
	[](int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
	{
		reinterpret_cast<menu::GenericServerMenu *>(userdata)->OnPlayerEvent(e->GetValue(0));
	}, this);

	ui::Widget *settingsButton = m_root->FindChild(button_settings_page_server_generic);
	settingsButton->Hide(common::transition::Type_Reset);
	settingsButton->AddEventCallback(ui::Button::CB_BTN_DECIDE,
	[](int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
	{
		reinterpret_cast<menu::GenericServerMenu *>(userdata)->OnSettingsButton();
	}, this);

	ui::Widget *backButton = m_root->FindChild(button_back_page_server_generic);
	backButton->Hide(common::transition::Type_Reset);
	backButton->AddEventCallback(ui::Button::CB_BTN_DECIDE,
	[](int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
	{
		reinterpret_cast<menu::GenericServerMenu *>(userdata)->OnBackButton();
	}, this);

	m_browserRoot = m_root->FindChild(plane_browser_root_page_server_generic);
	m_loaderIndicator = static_cast<ui::BusyIndicator *>(m_root->FindChild(busyindicator_loader_page_server_generic));
	m_topText = static_cast<ui::Text *>(m_root->FindChild(text_top));
}

menu::GenericServerMenu::~GenericServerMenu()
{
	while (PopBrowserPage())
	{

	}

	delete m_browser;
}

bool menu::GenericServerMenu::PushBrowserPage(string *ref)
{
	Plugin::TemplateOpenParam tmpParam;

	BrowserPage *page = new BrowserPage();

	g_appPlugin->TemplateOpen(m_browserRoot, template_list_view_generic, tmpParam);
	page->m_list = static_cast<ui::ListView *>(m_browserRoot->GetChild(m_browserRoot->GetChildrenNum() - 1));
	ListViewCb *lwCb = new ListViewCb(this);
	page->m_list->SetItemFactory(lwCb);
	page->m_list->InsertSegment(0, 1);
	math::v4 sz(960.0f, 80.0f);
	page->m_list->SetCellSizeDefault(0, sz);
	page->m_list->SetSegmentLayoutType(0, ui::ListView::LAYOUT_TYPE_LIST);

	BrowserPage *wp;
	if (!m_pageList.empty()) {
		if (m_pageList.size() > 1) {
			wp = m_pageList.rbegin()[1];
			wp->m_list->Hide(common::transition::Type_Reset);
			wp->m_list->SetTransitionComplete(false);
		}
		wp = m_pageList.back();
		wp->m_list->Show(common::transition::Type_3D_SlideToBack1);
		wp->m_list->SetTransitionComplete(false);
	}

	m_pageList.push_back(page);

	GoToJob *job = new GoToJob(this);
	if (ref)
	{
		job->SetRef(*ref);
	}
	common::SharedPtr<job::JobItem> itemParam(job);
	utils::GetJobQueue()->Enqueue(itemParam);

	return true;
}

bool menu::GenericServerMenu::PopBrowserPage()
{
	BrowserPage *pageToPop;
	bool isLastPage = false;

	if (m_pageList.empty())
	{
		return false;
	}
	else if (m_pageList.size() == 1)
	{
		isLastPage = true;
	}

	pageToPop = m_pageList.back();
	while (!pageToPop->m_isLoaded)
	{
		thread::Sleep(10);
	}

	pageToPop->m_list->SetName(static_cast<uint32_t>(0));

	if (!isLastPage)
	{
		common::transition::DoReverse(-100.0f, pageToPop->m_list, common::transition::Type_3D_SlideFromFront, true, false);
	}

	for (int i = 0; i < pageToPop->m_itemList->size(); i++)
	{
		delete pageToPop->m_itemList->at(i);
	}

	delete pageToPop->m_itemList;
	delete pageToPop;
	m_pageList.pop_back();

	if (!isLastPage)
	{
		BrowserPage *wp;
		if (!m_pageList.empty()) {
			wp = m_pageList.back();
			wp->m_list->Hide(common::transition::Type_3D_SlideToBack1);
			wp->m_list->Show(common::transition::Type_Reset);
			wp->m_list->SetTransitionComplete(false);
			if (m_pageList.size() > 1) {
				wp = m_pageList.rbegin()[1];
				wp->m_list->Show(common::transition::Type_Reset);
				wp->m_list->SetTransitionComplete(false);
			}
		}
	}

	if (!isLastPage)
	{
		m_browser->SetPath("..");
		string currentPath = m_browser->GetPath();
		wstring text16;
		common::Utf8ToUtf16(currentPath, &text16);
		m_topText->SetString(text16);
	}

	return true;
}