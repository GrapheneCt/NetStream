#include <kernel.h>
#include <appmgr.h>
#include <stdlib.h>
#include <string.h>
#include <libdbg.h>
#include <paf.h>

#include "common.h"
#include "utils.h"
#include "yt_utils.h"
#include "np_utils.h"
#include "dialog.h"
#include "tex_pool.h"
#include "hvdb.h"
#include <paf_file_ext.h>
#include "option_menu.h"
#include "menus/menu_generic.h"
#include "menus/menu_hvdb.h"
#include "menus/menu_hvdb_tracks.h"
#include "menus/menu_settings.h"

#undef SCE_DBG_LOG_COMPONENT
#define SCE_DBG_LOG_COMPONENT "[HVDB]"

using namespace paf;

menu::HVDB::Submenu::Submenu(HVDB *parentObj)
{
	m_currentPage = 0;
	m_baseParent = parentObj;
	m_interrupted = false;
	m_allJobsComplete = true;
}

menu::HVDB::Submenu::~Submenu()
{
	m_list->SetName(static_cast<uint32_t>(0));
	ReleaseCurrentPage();
	common::transition::DoReverse(0.0f, m_submenuRoot, common::transition::Type_FadeinFast, true, false);
}

void menu::HVDB::Submenu::ReleaseCurrentPage()
{
	m_interrupted = true;
	while (!m_allJobsComplete)
	{
		thread::RMutex::MainThreadMutex()->Unlock();
		thread::Sleep(10);
		thread::RMutex::MainThreadMutex()->Lock();
	}
	m_interrupted = false;

	if (m_list->GetCellNum(0) > 0)
	{
		m_list->DeleteCell(0, 0, m_list->GetCellNum(0));
	}

	m_baseParent->m_texPool->SetAlive(false);
	m_baseParent->m_texPool->AddAsyncCancelPending();
	m_baseParent->m_texPool->RemoveAll();
	m_baseParent->m_texPool->SetAlive(true);

	m_results.clear();
}

void menu::HVDB::Submenu::OnListButton(ui::Widget *wdg)
{
	uint32_t totalCount = m_results.size();
	uint32_t idhash = wdg->GetName().GetIDHash();

	if (m_currentPage == 0)
	{
		if (idhash == totalCount)
		{
			GoToNextPage();
			return;
		}
	}
	else
	{
		if (idhash == totalCount)
		{
			GoToPrevPage();
			return;
		}
		else if (idhash == totalCount + 1)
		{
			GoToNextPage();
			return;
		}
	}

	Submenu::Item item = m_results.at(idhash);

	menu::HvdbTracks *menu = new menu::HvdbTracks(item.workId.c_str(), item.bannerUrl.c_str());
	menu->PushBrowserPage(NULL);
}

void menu::HVDB::Submenu::OnTexPoolAdd(int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
{
	Item *workItem = static_cast<Item *>(userdata);
	ui::Button *button = static_cast<ui::Button *>(self);

	if (e->GetValue(0) == workItem->texId.GetIDHash())
	{
		button->SetTexture(workItem->texPool->Get(workItem->texId));
		button->DeleteEventCallback(ui::Handler::CB_STATE_READY_CACHEIMAGE, OnTexPoolAdd, userdata);
	}
}

ui::ListItem *menu::HVDB::Submenu::CreateListItem(ui::listview::ItemFactory::CreateParam& param)
{
	Plugin::TemplateOpenParam tmpParam;
	ui::Widget *item = NULL;
	ui::Widget *button = NULL;
	ui::Widget *subText = NULL;
	wstring text16;
	wchar_t numPageText[32];

	if (!param.list_view->GetName().GetIDHash())
	{
		return new ui::ListItem(param.parent, 0);
	}

	ui::Widget *targetRoot = param.parent;
	uint32_t totalCount = m_results.size();

	if (param.cell_index == totalCount || param.cell_index == totalCount + 1)
	{
		g_appPlugin->TemplateOpen(targetRoot, template_list_item_hvdb_aligned, tmpParam);
		item = targetRoot->GetChild(targetRoot->GetChildrenNum() - 1);
		button = item->FindChild(image_button_list_item_hvdb_aligned);
	}
	else
	{
		g_appPlugin->TemplateOpen(targetRoot, template_list_item_hvdb, tmpParam);
		item = targetRoot->GetChild(targetRoot->GetChildrenNum() - 1);
		button = item->FindChild(image_button_list_item_hvdb);
	}

	button->SetName(param.cell_index);

	intrusive_ptr<graph::Surface> tex;

	if (param.cell_index == totalCount)
	{
		if (m_currentPage == 0)
		{
			text16 = g_appPlugin->GetString(msg_next_page);
			sce_paf_swprintf(numPageText, sizeof(numPageText) / 2, L" (%d)", m_currentPage + 1 + 1);
			text16 += numPageText;
			button->SetString(text16);
			tex = g_appPlugin->GetTexture(tex_button_arrow_right);
			button->SetTexture(tex);
			goto serviceButton;
		}
		else
		{
			text16 = g_appPlugin->GetString(msg_previous_page);
			sce_paf_swprintf(numPageText, sizeof(numPageText) / 2, L" (%d)", m_currentPage + 1 - 1);
			text16 += numPageText;
			button->SetString(text16);
			tex = g_appPlugin->GetTexture(tex_button_arrow_left);
			button->SetTexture(tex);
			goto serviceButton;
		}
	}
	else if (param.cell_index == totalCount + 1)
	{
		text16 = g_appPlugin->GetString(msg_next_page);
		sce_paf_swprintf(numPageText, sizeof(numPageText) / 2, L" (%d)", m_currentPage + 1 + 1);
		text16 += numPageText;
		button->SetString(text16);
		tex = g_appPlugin->GetTexture(tex_button_arrow_right);
		button->SetTexture(tex);
		goto serviceButton;
	}

	Item *workItem = &m_results.at(param.cell_index);
	subText = button->FindChild(text_list_item_hvdb_subtext);
	subText->SetString(workItem->stat);
	button->SetString(workItem->name);
	if (m_baseParent->m_texPool->Exist(workItem->texId))
	{
		button->SetTexture(m_baseParent->m_texPool->Get(workItem->texId));
	}
	else
	{
		workItem->texPool = m_baseParent->m_texPool;
		button->AddEventCallback(ui::Handler::CB_STATE_READY_CACHEIMAGE, OnTexPoolAdd, workItem);
		m_baseParent->m_texPool->AddAsync(workItem->texId, workItem->texId.GetID().c_str());
	}

serviceButton:

	button->AddEventCallback(ui::Button::CB_BTN_DECIDE,
	[](int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
	{
		reinterpret_cast<Submenu *>(userdata)->OnListButton(static_cast<ui::Widget *>(self));
	}, this);

	return static_cast<ui::ListItem *>(item);
}

void menu::HVDB::FavouriteSubmenu::ParseFavourite()
{
	string text8;
	HVDBItem *hvdbItem = NULL;
	void *hvdbCtx = NULL;
	int32_t ret = -1;

	thread::RMutex::MainThreadMutex()->Lock();
	m_baseParent->m_loaderIndicator->Start();
	thread::RMutex::MainThreadMutex()->Unlock();

	SCE_DBG_LOG_INFO("[Favourite] Begin parse");

	HVDBSort sort;
	HVDBOrder order;
	HVDBFilter filter;
	sce::AppSettings *settings = menu::Settings::GetAppSetInstance();
	settings->GetInt("hvdb_search_sort", (int32_t *)&sort, 0);
	settings->GetInt("hvdb_search_sort_type", (int32_t *)&order, 0);
	settings->GetInt("hvdb_search_filter", (int32_t *)&filter, 0);

	ret = hvdbParseFavourites(&hvdbCtx, m_currentPage + 1, sort, order, filter, &m_pgi, &hvdbItem);
	SCE_DBG_LOG_INFO("[Favourite] hvdbParseFavourites(): 0x%08X, current page: %d", ret, m_currentPage);

	if (ret <= 0)
	{
		dialog::OpenError(g_appPlugin, ret, Framework::Instance()->GetCommonString("msg_error_connect_server_peer"));
		thread::RMutex::MainThreadMutex()->Lock();
		m_baseParent->m_loaderIndicator->Stop();
		thread::RMutex::MainThreadMutex()->Unlock();
		m_allJobsComplete = true;
		return;
	}

	m_pgi.currentPage--;

	do
	{
		Item item;
		item.workId = hvdbItem->workItem->id;
		item.bannerUrl = hvdbItem->workItem->bannerUrl;
		text8 = "\n";
		text8 += hvdbItem->workItem->title;
		common::Utf8ToUtf16(text8, &item.name);
		item.texId = hvdbItem->workItem->thmbUrl;
		text8 = hvdbItem->workItem->circle;
		text8 += "\n";
		text8 += hvdbItem->workItem->published;
		common::Utf8ToUtf16(text8, &item.stat);

		m_results.push_back(item);

		hvdbItem = hvdbItem->next;
	} while (hvdbItem);

	thread::RMutex::MainThreadMutex()->Lock();
	m_baseParent->m_loaderIndicator->Stop();
	if (m_currentPage == 0)
	{
		if (m_pgi.worksTotal > m_pgi.worksPerPage)
		{
			m_list->InsertCell(0, 0, m_results.size() + 1);
		}
		else
		{
			m_list->InsertCell(0, 0, m_results.size());
		}
	}
	else
	{
		if (ret <= m_pgi.worksPerPage)
		{
			m_list->InsertCell(0, 0, m_results.size() + 1);
		}
		else
		{
			m_list->InsertCell(0, 0, m_results.size() + 2);
		}
	}
	thread::RMutex::MainThreadMutex()->Unlock();

	hvdbCleanup(hvdbCtx);

	m_allJobsComplete = true;
}

void menu::HVDB::FavouriteSubmenu::GoToNextPage()
{
	ReleaseCurrentPage();
	m_currentPage++;

	FavouriteJob *job = new FavouriteJob(this);
	common::SharedPtr<job::JobItem> itemParam(job);
	m_allJobsComplete = false;
	utils::GetJobQueue()->Enqueue(itemParam);
}

void menu::HVDB::FavouriteSubmenu::GoToPrevPage()
{
	ReleaseCurrentPage();
	m_currentPage--;

	FavouriteJob *job = new FavouriteJob(this);
	common::SharedPtr<job::JobItem> itemParam(job);
	m_allJobsComplete = false;
	utils::GetJobQueue()->Enqueue(itemParam);
}

void menu::HVDB::FavouriteSubmenu::GoToBasePageForSync()
{
	ReleaseCurrentPage();
	m_currentPage = 0;

	FavouriteJob *job = new FavouriteJob(this);
	common::SharedPtr<job::JobItem> itemParam(job);
	m_allJobsComplete = false;
	utils::GetJobQueue()->Enqueue(itemParam);
}

menu::HVDB::FavouriteSubmenu::FavouriteSubmenu(HVDB *parentObj) : Submenu(parentObj)
{
	Plugin::TemplateOpenParam tmpParam;

	g_appPlugin->TemplateOpen(m_baseParent->m_browserRoot, template_list_view_hvdb_fav, tmpParam);
	m_submenuRoot = m_baseParent->m_browserRoot->FindChild(plane_list_view_hvdb_fav_root);
	m_list = static_cast<ui::ListView *>(m_submenuRoot->FindChild(list_view_hvdb));
	ListViewCb *lwCb = new ListViewCb(this);
	m_list->SetItemFactory(lwCb);
	m_list->InsertSegment(0, 1);
	math::v4 sz(960.0f, 100.0f);
	m_list->SetCellSizeDefault(0, sz);
	m_list->SetSegmentLayoutType(0, ui::ListView::LAYOUT_TYPE_LIST);

	common::transition::Do(0.0f, m_submenuRoot, common::transition::Type_FadeinFast, true, false);

	FavouriteJob *job = new FavouriteJob(this);
	common::SharedPtr<job::JobItem> itemParam(job);
	m_allJobsComplete = false;
	utils::GetJobQueue()->Enqueue(itemParam);
}

menu::HVDB::FavouriteSubmenu::~FavouriteSubmenu()
{

}

void menu::HVDB::OnSettingsButton()
{
	vector<OptionMenu::Button> buttons;
	OptionMenu::Button bt;
	bt.label = g_appPlugin->GetString(msg_settings);
	buttons.push_back(bt);
	bt.label = g_appPlugin->GetString(msg_settings_hvdb_refresh);
	buttons.push_back(bt);

	new OptionMenu(g_appPlugin, m_root, &buttons);
}

void menu::HVDB::OnOptionMenuEvent(int32_t type, int32_t subtype)
{
	if (menu::GetTopMenu()->GetMenuType() != menu::MenuType_Hvdb)
	{
		return;
	}

	if (type == OptionMenu::OptionMenuEvent_Close)
	{
		return;
	}

	switch (subtype)
	{
	case 0:
		menu::SettingsButtonCbFun(ui::Button::CB_BTN_DECIDE, NULL, 0, NULL);
		break;
	case 1:
		if (m_currentSubmenu && (m_currentSubmenu->GetType() == Submenu::SubmenuType_Favourites))
		{
			m_currentSubmenu->GoToBasePageForSync();
		}
		break;
	}
}

void menu::HVDB::OnBackButton()
{
	delete this;
}

bool menu::HVDB::AuthMe()
{
	char user[256];
	char password[256];
	int32_t ret = -1;

	sce::AppSettings *settings = menu::Settings::GetAppSetInstance();
	settings->GetString("hvdb_user", user, sizeof(user) - 1, "");
	settings->GetString("hvdb_password", password, sizeof(password) - 1, "");

	ret = hvdbLogin(user, password);
	if (ret != HVDB_OK)
	{
		SCE_DBG_LOG_ERROR("[AuthMe] hvdbLogin(): 0x%08X", ret);
		return false;
	}

	return true;
}

menu::HVDB::HVDB() :
	GenericMenu("page_hvdb",
	MenuOpenParam(false, 200.0f, Plugin::TransitionType_SlideFromBottom),
	MenuCloseParam(false, 200.0f, Plugin::TransitionType_SlideFromBottom))
{
	m_currentSubmenu = NULL;

	utils::SetRequestShare(ytutils::GetShare());

	m_root->AddEventCallback(OptionMenu::OptionMenuEvent,
	[](int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
	{
		reinterpret_cast<HVDB *>(userdata)->OnOptionMenuEvent(e->GetValue(0), e->GetValue(1));
	}, this);

	ui::Widget *settingsButton = m_root->FindChild(button_settings_page_hvdb);
	settingsButton->Show(common::transition::Type_Reset);
	settingsButton->AddEventCallback(ui::Button::CB_BTN_DECIDE,
	[](int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
	{
		reinterpret_cast<HVDB *>(userdata)->OnSettingsButton();
	}, this);

	ui::Widget *backButton = m_root->FindChild(button_back_page_hvdb);
	backButton->Show(common::transition::Type_Reset);
	backButton->AddEventCallback(ui::Button::CB_BTN_DECIDE,
	[](int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
	{
		reinterpret_cast<HVDB *>(userdata)->OnBackButton();
	}, this);

	m_browserRoot = m_root->FindChild(plane_browser_root_page_hvdb);
	m_loaderIndicator = static_cast<ui::BusyIndicator *>(m_root->FindChild(busyindicator_loader_page_hvdb));
	m_topText = static_cast<ui::Text *>(m_root->FindChild(text_top));

	m_texPool = new TexPool(g_appPlugin);
	m_texPool->SetShare(utils::GetShare());

	char host[256];
	sce::AppSettings *settings = menu::Settings::GetAppSetInstance();
	settings->GetString("hvdb_host", host, sizeof(host) - 1, "");
	hvdbSetInstance(host);

	if (!AuthMe())
	{
		return;
	}

	SwitchSubmenu(Submenu::SubmenuType_Favourites);
}

menu::HVDB::~HVDB()
{
	delete m_currentSubmenu;
	m_texPool->DestroyAsync();
	utils::SetGlobalProxy(NULL);
	utils::SetRequestShare(NULL);
}

void menu::HVDB::SwitchSubmenu(Submenu::SubmenuType type)
{
	if (m_currentSubmenu)
	{
		if (m_currentSubmenu->GetType() == type)
		{
			return;
		}
		delete m_currentSubmenu;
		m_currentSubmenu = NULL;
	}

	switch (type)
	{
	case Submenu::SubmenuType_Favourites:
		m_currentSubmenu = new FavouriteSubmenu(this);
		break;
	}
}