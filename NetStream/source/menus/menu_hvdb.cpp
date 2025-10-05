#include <kernel.h>
#include <appmgr.h>
#include <stdlib.h>
#include <string.h>
#include <libdbg.h>
#include <paf.h>

#include "common.h"
#include "utils.h"
#include "hvdb_utils.h"
#include "dialog.h"
#include "tex_pool.h"
#include "hvdb.h"
#include <paf_file_ext.h>
#include "option_menu.h"
#include "players/player_fmod.h"
#include "menus/menu_generic.h"
#include "menus/menu_hvdb.h"
#include "menus/menu_settings.h"
#include "menus/menu_player_simple.h"

using namespace paf;

void menu::HVDB::OnPlayerEvent(int32_t type)
{
	switch (type)
	{
	case PlayerSimple::PlayerEvent_Back:
		delete m_player;
		break;
	case PlayerSimple::PlayerEvent_InitOk:

		break;
	case PlayerSimple::PlayerEvent_InitFail:
		dialog::OpenError(g_appPlugin, SCE_ERROR_ERRNO_EUNSUP, Framework::Instance()->GetCommonString("msg_error_load_file"));
		delete m_player;
		break;
	}
}

void menu::HVDB::OnDeleteButton(ui::Widget *wdg)
{
	Plugin::TemplateOpenParam tmpParam;
	uint32_t idhash = wdg->GetName().GetIDHash();
	
	AudioItem *workItem = &m_entryResults.at(idhash);
	hvdbutils::GetEntryLog()->Remove(workItem->id.c_str());
	m_list->DeleteCell(0, idhash, 1);
}

void menu::HVDB::OnListButton(ui::Widget *wdg)
{
	Plugin::TemplateOpenParam tmpParam;
	uint32_t idhash = wdg->GetName().GetIDHash();

	if (m_trackResults.empty())
	{
		m_backButton->Hide(common::transition::Type_Reset);

		m_list->Show(common::transition::Type_3D_SlideToBack1);
		m_list->SetTransitionComplete(false);

		g_appPlugin->TemplateOpen(m_browserRoot, template_list_view_generic, tmpParam);
		m_trackList = static_cast<ui::ListView *>(m_browserRoot->GetChild(m_browserRoot->GetChildrenNum() - 1));
		ListViewTrackCb *lwCb = new ListViewTrackCb(this);
		m_trackList->SetItemFactory(lwCb);
		m_trackList->InsertSegment(0, 1);
		math::v4 sz(960.0f, 80.0f);
		m_trackList->SetCellSizeDefault(0, sz);
		m_trackList->SetSegmentLayoutType(0, ui::ListView::LAYOUT_TYPE_LIST);

		AudioItem *workItem = &m_entryResults.at(idhash);
		TrackParseJob *job = new TrackParseJob(this, workItem);
		common::SharedPtr<job::JobItem> itemParam(job);
		job::JobQueue::DefaultQueue()->Enqueue(itemParam);
	}
	else
	{
		TrackItem *workItem = &m_trackResults.at(idhash);
		FMODPlayer::Option opt;
		opt.coverUrl = workItem->audioItem->cover.c_str();
		m_player = new menu::PlayerSimple(workItem->url.c_str(), &opt);
	}
}

void menu::HVDB::OnAddEntryButton()
{
	wstring request;
	m_addEntryTextBox->GetString(request);

	if (request.empty())
	{
		dialog::Close();
		return;
	}

	EntryAddJob *job = new EntryAddJob(this);
	common::Utf16ToUtf8(request, &m_addEntryId);
	common::SharedPtr<job::JobItem> itemParam(job);
	job::JobQueue::DefaultQueue()->Enqueue(itemParam);
	dialog::Close();
}

void menu::HVDB::OnSettingsButton()
{
	vector<OptionMenu::Button> buttons;
	OptionMenu::Button bt;
	bt.label = g_appPlugin->GetString(msg_settings);
	buttons.push_back(bt);
	bt.label = g_appPlugin->GetString(msg_settings_hvdb_add_entry);
	buttons.push_back(bt);
	bt.label = g_appPlugin->GetString(msg_settings_hvdb_clean_all_entry);
	buttons.push_back(bt);

	new OptionMenu(g_appPlugin, m_root, &buttons);
}

void menu::HVDB::OnOptionMenuEvent(int32_t type, int32_t subtype)
{
	Plugin::TemplateOpenParam tmpParam;

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
		ui::ScrollView *root = dialog::OpenScrollView(g_appPlugin, g_appPlugin->GetString(msg_settings_hvdb_add_entry));
		g_appPlugin->TemplateOpen(root, template_add_entry_hvdb, tmpParam);
		ui::Widget *okButton = root->FindChild(button_hvdb_add_entry);
		m_addEntryTextBox = static_cast<ui::TextBox *>(root->FindChild(text_box_hvdb_add_entry));
		okButton->AddEventCallback(ui::Button::CB_BTN_DECIDE,
		[](int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
		{
			reinterpret_cast<HVDB *>(userdata)->OnAddEntryButton();
		}, this);
		break;
	case 2:
		dialog::OpenYesNo(g_appPlugin, NULL, g_appPlugin->GetString(msg_settings_hvdb_clean_all_entry_confirm));
		m_dialogIdx = subtype;
		break;
	}
}

void menu::HVDB::OnDialogEvent(int32_t type)
{
	if (menu::GetTopMenu()->GetMenuType() != menu::MenuType_Hvdb)
	{
		return;
	}

	if (type == dialog::ButtonCode_Yes)
	{
		m_list->DeleteSegment(0, 1);
		LogClearJob *job = new LogClearJob(this);
		common::SharedPtr<job::JobItem> itemParam(job);
		job::JobQueue::DefaultQueue()->Enqueue(itemParam);
	}
}

void menu::HVDB::OnBackButton()
{
	if (m_trackList)
	{
		common::transition::DoReverse(-100.0f, m_trackList, common::transition::Type_3D_SlideFromFront, true, false);
		m_list->Hide(common::transition::Type_3D_SlideToBack1);
		m_list->Show(common::transition::Type_Reset);
		m_list->SetTransitionComplete(false);
		m_trackList = NULL;
		m_trackResults.clear();
	}
	else
	{
		delete this;
	}
}

void menu::HVDB::OnTexPoolAdd(int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
{
	AudioItem *workItem = (AudioItem *)userdata;
	ui::Button *button = (ui::Button *)self;

	if (e->GetValue(0) == workItem->texId.GetIDHash())
	{
		button->SetTexture(workItem->texPool->Get(workItem->texId));
		button->DeleteEventCallback(ui::Handler::CB_STATE_READY_CACHEIMAGE, OnTexPoolAdd, userdata);
	}
}

ui::ListItem *menu::HVDB::CreateTrackListItem(ui::listview::ItemFactory::CreateParam& param)
{
	Plugin::TemplateOpenParam tmpParam;
	ui::Widget *item = NULL;
	ui::Widget *button = NULL;

	if (!param.list_view->GetName().GetIDHash())
	{
		return new ui::ListItem(param.parent, 0);
	}

	ui::Widget *targetRoot = param.parent;
	uint32_t totalCount = m_trackResults.size();

	g_appPlugin->TemplateOpen(targetRoot, template_list_item_generic, tmpParam);
	item = targetRoot->GetChild(targetRoot->GetChildrenNum() - 1);
	button = item->FindChild(image_button_list_item);

	button->SetName(param.cell_index);

	intrusive_ptr<graph::Surface> tex = g_appPlugin->GetTexture(tex_file_icon_music);
	if (tex.get())
	{
		button->SetTexture(tex);
	}

	TrackItem *workItem = &m_trackResults.at(param.cell_index);
	button->SetString(workItem->name);
	button->AddEventCallback(ui::Button::CB_BTN_DECIDE,
	[](int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
	{
		reinterpret_cast<HVDB *>(userdata)->OnListButton(static_cast<ui::Widget *>(self));
	}, this);

	return static_cast<ui::ListItem *>(item);
}

ui::ListItem *menu::HVDB::CreateListItem(ui::listview::ItemFactory::CreateParam& param)
{
	Plugin::TemplateOpenParam tmpParam;
	ui::Widget *item = NULL;
	ui::Widget *button = NULL;
	ui::Widget *deleteButton = NULL;
	ui::Widget *subText = NULL;

	if (!param.list_view->GetName().GetIDHash())
	{
		return new ui::ListItem(param.parent, 0);
	}

	ui::Widget *targetRoot = param.parent;
	uint32_t totalCount = m_entryResults.size();

	g_appPlugin->TemplateOpen(targetRoot, template_list_item_hvdb, tmpParam);
	item = targetRoot->GetChild(targetRoot->GetChildrenNum() - 1);
	button = item->FindChild(image_button_list_item_hvdb);
	deleteButton = button->FindChild(button_list_item_hvdb_delete);

	button->SetName(param.cell_index);
	deleteButton->SetName(param.cell_index);

	intrusive_ptr<graph::Surface> tex;

	AudioItem *workItem = &m_entryResults.at(param.cell_index);
	subText = button->FindChild(text_list_item_hvdb_subtext);
	subText->SetString(workItem->subtitle);
	button->SetString(workItem->name);
	if (m_texPool->Exist(workItem->texId))
	{
		button->SetTexture(m_texPool->Get(workItem->texId));
	}
	else
	{
		workItem->texPool = m_texPool;
		button->AddEventCallback(ui::Handler::CB_STATE_READY_CACHEIMAGE, OnTexPoolAdd, workItem);
		m_texPool->AddAsync(workItem->texId, workItem->texId.GetID().c_str());
	}

	button->AddEventCallback(ui::Button::CB_BTN_DECIDE,
	[](int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
	{
		reinterpret_cast<HVDB *>(userdata)->OnListButton(static_cast<ui::Widget *>(self));
	}, this);

	button->AddEventCallback(ui::Button::CB_BTN_DECIDE,
	[](int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
	{
		reinterpret_cast<HVDB *>(userdata)->OnDeleteButton(static_cast<ui::Widget *>(self));
	}, this);

	return static_cast<ui::ListItem *>(item);
}

void menu::HVDB::ParseTrack(AudioItem *audioItem)
{
	string text8;
	int32_t ret = 0;
	HvdbItemTrack *hvdbItem;

	thread::RMutex::MainThreadMutex()->Lock();
	m_loaderIndicator->Start();
	thread::RMutex::MainThreadMutex()->Unlock();

	ret = hvdbParseTrack(audioItem->id.c_str(), &hvdbItem);
	if (ret > 0)
	{
		for (int i = 0; i < ret; i++)
		{
			TrackItem item;
			item.audioItem = audioItem;
			text8 = hvdbItem[i].title;
			common::Utf8ToUtf16(text8, &item.name);
			item.url = hvdbItem[i].url;
			m_trackResults.push_back(item);
		}

		hvdbCleanupTrack(hvdbItem);

		thread::RMutex::MainThreadMutex()->Lock();
		m_loaderIndicator->Stop();
		m_trackList->InsertCell(0, 0, m_trackResults.size());
		m_backButton->Show(common::transition::Type_Reset);
		thread::RMutex::MainThreadMutex()->Unlock();
	}
	else
	{
		thread::RMutex::MainThreadMutex()->Lock();
		m_loaderIndicator->Stop();
		dialog::OpenError(g_appPlugin, ret, g_appPlugin->GetString(msg_hvdb_error_id_not_exist));
		m_backButton->Show(common::transition::Type_Reset);
		thread::RMutex::MainThreadMutex()->Unlock();
	}
}

void menu::HVDB::AddEntry()
{
	string text8;
	int32_t ret = 0;
	HvdbItemAudio *hvdbItem;

	thread::RMutex::MainThreadMutex()->Lock();
	m_loaderIndicator->Start();
	thread::RMutex::MainThreadMutex()->Unlock();

	if (m_addEntryId.empty())
	{
		int32_t totalNum = hvdbutils::GetEntryLog()->GetSize();

		char *entryData = static_cast<char *>(sce_paf_calloc(totalNum, SCE_INI_FILE_PROCESSOR_KEY_BUFFER_SIZE));

		hvdbutils::GetEntryLog()->Reset();

		for (int32_t i = 0; i < totalNum; i++)
		{
			hvdbutils::GetEntryLog()->GetNext(entryData + (i * SCE_INI_FILE_PROCESSOR_KEY_BUFFER_SIZE));
		}

		for (int32_t i = 0; i < totalNum; i++)
		{
			if (m_interrupted)
			{
				m_interrupted = false;
				break;
			}

			ret = hvdbParseAudio(entryData + (i * SCE_INI_FILE_PROCESSOR_KEY_BUFFER_SIZE), &hvdbItem);
			if (ret == true)
			{
				AudioItem item;
				item.id = entryData + (i * SCE_INI_FILE_PROCESSOR_KEY_BUFFER_SIZE);
				text8 = "\n";
				text8 += hvdbItem->title;
				common::Utf8ToUtf16(text8, &item.name);
				item.texId = hvdbItem->thmbUrl;
				text8 = "RJ";
				text8 += entryData + (i * SCE_INI_FILE_PROCESSOR_KEY_BUFFER_SIZE);
				common::Utf8ToUtf16(text8, &item.subtitle);
				item.cover = hvdbItem->thmbUrlHq;

				m_entryResults.push_back(item);

				hvdbCleanupAudio(hvdbItem);
			}
		}

		thread::RMutex::MainThreadMutex()->Lock();
		m_loaderIndicator->Stop();
		if (!m_interrupted)
		{
			m_list->InsertCell(0, 0, m_entryResults.size());
		}
		thread::RMutex::MainThreadMutex()->Unlock();
	}
	else
	{
		ret = hvdbParseAudio(m_addEntryId.c_str(), &hvdbItem);
		if (ret == true)
		{
			AudioItem item;
			item.id = m_addEntryId.c_str();
			text8 = "\n";
			text8 += hvdbItem->title;
			common::Utf8ToUtf16(text8, &item.name);
			item.texId = hvdbItem->thmbUrl;
			text8 = "RJ";
			text8 += m_addEntryId.c_str();
			common::Utf8ToUtf16(text8, &item.subtitle);
			item.cover = hvdbItem->thmbUrlHq;

			m_entryResults.push_back(item);

			hvdbCleanupAudio(hvdbItem);

			hvdbutils::GetEntryLog()->Add(m_addEntryId.c_str());

			thread::RMutex::MainThreadMutex()->Lock();
			m_loaderIndicator->Stop();
			m_list->InsertCell(0, m_entryResults.size(), 1);
			thread::RMutex::MainThreadMutex()->Unlock();
		}
		else
		{
			thread::RMutex::MainThreadMutex()->Lock();
			m_loaderIndicator->Stop();
			dialog::OpenError(g_appPlugin, ret, g_appPlugin->GetString(msg_hvdb_error_id_not_exist));
			thread::RMutex::MainThreadMutex()->Unlock();
		}
	}
}

int32_t menu::HVDB::LogClearJob::Run()
{
	dialog::OpenPleaseWait(g_appPlugin, NULL, Framework::Instance()->GetCommonString("msg_wait"));
	m_parent->ClearEntryResults();
	hvdbutils::EntryLog::Clean();
	dialog::Close();

	return SCE_PAF_OK;
}

menu::HVDB::HVDB() :
	GenericMenu("page_hvdb",
	MenuOpenParam(false, 200.0f, Plugin::TransitionType_SlideFromBottom),
	MenuCloseParam(false, 200.0f, Plugin::TransitionType_SlideFromBottom))
{
	Plugin::TemplateOpenParam tmpParam;

	m_interrupted = false;
	m_trackList = NULL;

	m_root->AddEventCallback(OptionMenu::OptionMenuEvent,
	[](int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
	{
		reinterpret_cast<HVDB *>(userdata)->OnOptionMenuEvent(e->GetValue(0), e->GetValue(1));
	}, this);

	m_root->AddEventCallback(dialog::DialogEvent,
	[](int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
	{
		reinterpret_cast<HVDB *>(userdata)->OnDialogEvent(e->GetValue(0));
	}, this);

	m_root->AddEventCallback(PlayerSimple::PlayerSimpleEvent,
	[](int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
	{
		reinterpret_cast<HVDB *>(userdata)->OnPlayerEvent(e->GetValue(0));
	}, this);

	ui::Widget *settingsButton = m_root->FindChild(button_settings_page_hvdb);
	settingsButton->Show(common::transition::Type_Reset);
	settingsButton->AddEventCallback(ui::Button::CB_BTN_DECIDE,
	[](int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
	{
		reinterpret_cast<HVDB *>(userdata)->OnSettingsButton();
	}, this);

	m_backButton = static_cast<ui::Button *>(m_root->FindChild(button_back_page_hvdb));
	m_backButton->Show(common::transition::Type_Reset);
	m_backButton->AddEventCallback(ui::Button::CB_BTN_DECIDE,
	[](int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
	{
		reinterpret_cast<HVDB *>(userdata)->OnBackButton();
	}, this);

	m_browserRoot = m_root->FindChild(plane_browser_root_page_hvdb);
	m_loaderIndicator = static_cast<ui::BusyIndicator *>(m_root->FindChild(busyindicator_loader_page_hvdb));
	m_topText = static_cast<ui::Text *>(m_root->FindChild(text_top));

	g_appPlugin->TemplateOpen(m_browserRoot, template_list_view_generic, tmpParam);
	m_list = static_cast<ui::ListView *>(m_browserRoot->GetChild(m_browserRoot->GetChildrenNum() - 1));
	ListViewCb *lwCb = new ListViewCb(this);
	m_list->SetItemFactory(lwCb);
	m_list->InsertSegment(0, 1);
	math::v4 sz(960.0f, 100.0f);
	m_list->SetCellSizeDefault(0, sz);
	m_list->SetSegmentLayoutType(0, ui::ListView::LAYOUT_TYPE_LIST);

	m_texPool = new TexPool(g_appPlugin, true);
	m_texPool->SetShare(utils::GetShare());

	EntryAddJob *job = new EntryAddJob(this);
	common::SharedPtr<job::JobItem> itemParam(job);
	job::JobQueue::DefaultQueue()->Enqueue(itemParam);
}

menu::HVDB::~HVDB()
{
	m_interrupted = true;
	job::JobQueue::DefaultQueue()->WaitEmpty();
	m_texPool->DestroyAsync();
}

void menu::HVDB::ClearEntryResults()
{
	m_entryResults.clear();
}