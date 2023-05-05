#include <kernel.h>
#include <appmgr.h>
#include <stdlib.h>
#include <string.h>
#include <libdbg.h>
#include <paf.h>

#include "common.h"
#include "utils.h"
#include "yt_utils.h"
#include "dialog.h"
#include "tex_pool.h"
#include "invidious.h"
#include <paf_file_ext.h>
#include "option_menu.h"
#include "menus/menu_generic.h"
#include "menus/menu_youtube.h"
#include "menus/menu_player_youtube.h"
#include "menus/menu_settings.h"

using namespace paf;

menu::YouTube::Submenu::Submenu(YouTube *parentObj)
{
	currentPage = 0;
	parent = parentObj;
	interrupted = false;
	allJobsComplete = true;
}

menu::YouTube::Submenu::~Submenu()
{
	list->SetName((uint32_t)0);
	ReleaseCurrentPage();
	common::transition::DoReverse(0.0f, submenuRoot, common::transition::Type_Fadein1, true, false);
}

void menu::YouTube::Submenu::ReleaseCurrentPage()
{
	interrupted = true;
	while (!allJobsComplete)
	{
		thread::RMutex::main_thread_mutex.Unlock();
		thread::Sleep(10);
		thread::RMutex::main_thread_mutex.Lock();
	}
	interrupted = false;

	if (list->GetCellNum(0) > 0)
	{
		list->DeleteCell(0, 0, list->GetCellNum(0));
	}

	parent->texPool->SetAlive(false);
	parent->texPool->AddAsyncWaitComplete();
	parent->texPool->RemoveAll();
	parent->texPool->SetAlive(true);

	results.clear();
}

void menu::YouTube::Submenu::ListViewCb::TexPoolAddCbFun(int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
{
	Item *workItem = (Item *)userdata;
	ui::Button *button = (ui::Button *)self;

	if (e->GetValue(0) == workItem->texId.GetIDHash())
	{
		button->SetTexture(workItem->texPool->Get(workItem->texId));
		button->DeleteEventCallback(ui::Handler::CB_STATE_READY_CACHEIMAGE, TexPoolAddCbFun, userdata);
	}
}

ui::ListItem *menu::YouTube::Submenu::ListViewCb::Create(CreateParam& param)
{
	Plugin::TemplateOpenParam tmpParam;
	ui::Widget *item = NULL;
	ui::Widget *button = NULL;
	ui::Text *timeText = NULL;
	ui::Widget *subText = NULL;
	wstring text16;
	wchar_t numPageText[32];

	if (!param.list_view->GetName().GetIDHash())
	{
		return new ui::ListItem(param.parent, 0);
	}

	ui::Widget *targetRoot = param.parent;
	uint32_t totalCount = workObj->results.size();

	if (param.cell_index == totalCount || param.cell_index == totalCount + 1)
	{
		g_appPlugin->TemplateOpen(targetRoot, template_list_item_youtube_aligned, tmpParam);
		item = targetRoot->GetChild(targetRoot->GetChildrenNum() - 1);
		button = item->FindChild(image_button_list_item_youtube_aligned);
	}
	else
	{
		g_appPlugin->TemplateOpen(targetRoot, template_list_item_youtube, tmpParam);
		item = targetRoot->GetChild(targetRoot->GetChildrenNum() - 1);
		button = item->FindChild(image_button_list_item_youtube);
	}

	button->SetName(param.cell_index);

	intrusive_ptr<graph::Surface> tex;

	if (param.cell_index == totalCount)
	{
		if (workObj->currentPage == 0)
		{
			text16 = g_appPlugin->GetString(msg_next_page);
			sce_paf_swprintf(numPageText, sizeof(numPageText) / 2, L" (%d)", workObj->currentPage + 1);
			text16 += numPageText;
			button->SetString(text16);
			tex = g_appPlugin->GetTexture(tex_button_arrow_right);
			button->SetTexture(tex);
			goto serviceButton;
		}
		else
		{
			text16 = g_appPlugin->GetString(msg_previous_page);
			sce_paf_swprintf(numPageText, sizeof(numPageText) / 2, L" (%d)", workObj->currentPage - 1);
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
		sce_paf_swprintf(numPageText, sizeof(numPageText) / 2, L" (%d)", workObj->currentPage + 1);
		text16 += numPageText;
		button->SetString(text16);
		tex = g_appPlugin->GetTexture(tex_button_arrow_right);
		button->SetTexture(tex);
		goto serviceButton;
	}

	Item *workItem = &workObj->results.at(param.cell_index);
	timeText = (ui::Text *)button->FindChild(text_list_item_youtube_time);
	subText = button->FindChild(text_list_item_youtube_subtext);
	if (workItem->time == L"LIVE")
	{
		math::v4 col(1.0f, 0.0f, 0.0f, 0.5f);
		timeText->SetStyleAttribute(graph::TextStyleAttribute_BackColor, 0, 0, col);
	}
	timeText->SetString(workItem->time);
	subText->SetString(workItem->stat);
	button->SetString(workItem->name);
	if (workObj->parent->texPool->Exist(workItem->texId))
	{
		button->SetTexture(workObj->parent->texPool->Get(workItem->texId));
	}
	else
	{
		workItem->texPool = workObj->parent->texPool;
		button->AddEventCallback(ui::Handler::CB_STATE_READY_CACHEIMAGE, TexPoolAddCbFun, workItem);
		workObj->parent->texPool->AddAsync(workItem->texId, workItem->texId.GetID().c_str());
	}

serviceButton:

	button->AddEventCallback(ui::Button::CB_BTN_DECIDE, ListButtonCbFun, workObj);

	return (ui::ListItem *)item;
}

void menu::YouTube::SearchSubmenu::SearchJob::Run()
{
	string text8;
	InvItem *items = NULL;
	int32_t ret = -1;

	thread::RMutex::main_thread_mutex.Lock();
	workObj->parent->loaderIndicator->Start();
	thread::RMutex::main_thread_mutex.Unlock();

	if (!isId)
	{
		InvSort sort;
		InvDate date;
		char region[3];
		sce::AppSettings *settings = menu::Settings::GetAppSetInstance();
		settings->GetInt("yt_search_sort", (int32_t *)&sort, 0);
		settings->GetInt("yt_search_date", (int32_t *)&date, 0);
		region[0] = 0;
		settings->GetString("yt_search_region", region, sizeof(region), "");
		ret = invParseSearch(workObj->request.c_str(), workObj->currentPage, INV_ITEM_TYPE_VIDEO, sort, date, region, &items);
	}
	else
	{
		items = new InvItem();
		ret = invParseVideo(workObj->request.c_str() + 3, &items->videoItem);
		if (!items->videoItem->id)
		{
			invCleanupVideo(items->videoItem);
			ret = -1;
		}
	}

	if (ret <= 0)
	{
		dialog::OpenError(g_appPlugin, ret, Framework::Instance()->GetCommonString("msg_error_connect_server_peer"));
		thread::RMutex::main_thread_mutex.Lock();
		workObj->parent->loaderIndicator->Stop();
		thread::RMutex::main_thread_mutex.Unlock();
		workObj->allJobsComplete = true;
		return;
	}

	for (int i = 0; i < ret; i++)
	{
		Item item;
		item.type = items[i].type;
		switch (item.type)
		{
		case INV_ITEM_TYPE_VIDEO:
			item.videoId = items[i].videoItem->id;
			text8 = "\n";
			text8 += items[i].videoItem->title;
			common::Utf8ToUtf16(text8, &item.name);
			item.texId = items[i].videoItem->thmbUrl;
			if (items[i].videoItem->isLive || items[i].videoItem->lengthSec == 0)
			{
				text8 = "LIVE";
			}
			else
			{
				utils::ConvertSecondsToString(text8, items[i].videoItem->lengthSec, false);
			}
			common::Utf8ToUtf16(text8, &item.time);
			text8 = "by ";
			text8 += items[i].videoItem->author;
			text8 += "\n";
			text8 += items[i].videoItem->published;
			common::Utf8ToUtf16(text8, &item.stat);
			break;
		}

		workObj->results.push_back(item);
	}

	thread::RMutex::main_thread_mutex.Lock();
	workObj->parent->loaderIndicator->Stop();
	if (workObj->currentPage == 0)
	{
		if (!isId)
		{
			workObj->list->InsertCell(0, 0, workObj->results.size() + 1);
		}
		else
		{
			workObj->list->InsertCell(0, 0, workObj->results.size());
		}
	}
	else
	{
		workObj->list->InsertCell(0, 0, workObj->results.size() + 2);
	}
	thread::RMutex::main_thread_mutex.Unlock();

	if (!isId)
	{
		invCleanupSearch(items);
	}
	else
	{
		invCleanupVideo(items->videoItem);
		delete items;
	}

	workObj->allJobsComplete = true;
}

void menu::YouTube::SearchSubmenu::SearchButtonCbFun(int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
{
	wstring text16;
	string text8;
	bool isId = false;

	menu::YouTube::SearchSubmenu *workObj = (menu::YouTube::SearchSubmenu *)userdata;
	workObj->ReleaseCurrentPage();
	workObj->searchBox->EndEdit();

	workObj->request.clear();
	workObj->searchBox->GetString(text16);
	common::Utf16ToUtf8(text16, &workObj->request);

	if (workObj->request.empty())
	{
		return;
	}
	else if ((sce_paf_strstr(workObj->request.c_str(), "id:") == workObj->request.c_str()) && workObj->request.length() == 14)
	{
		isId = true;
	}

	workObj->currentPage = 0;

	SearchJob *job = new SearchJob("YouTube::SearchJob");
	job->workObj = workObj;
	job->isId = isId;
	common::SharedPtr<job::JobItem> itemParam(job);
	workObj->allJobsComplete = false;
	utils::GetJobQueue()->Enqueue(itemParam);
}

void menu::YouTube::SearchSubmenu::GoToNextPage()
{
	ReleaseCurrentPage();
	currentPage++;

	SearchJob *job = new SearchJob("YouTube::SearchJob");
	job->workObj = this;
	job->isId = false;
	common::SharedPtr<job::JobItem> itemParam(job);
	allJobsComplete = false;
	utils::GetJobQueue()->Enqueue(itemParam);
}

void menu::YouTube::SearchSubmenu::GoToPrevPage()
{
	ReleaseCurrentPage();
	currentPage--;

	SearchJob *job = new SearchJob("YouTube::SearchJob");
	job->workObj = this;
	job->isId = false;
	common::SharedPtr<job::JobItem> itemParam(job);
	allJobsComplete = false;
	utils::GetJobQueue()->Enqueue(itemParam);
}

menu::YouTube::SearchSubmenu::SearchSubmenu(YouTube *parentObj) : Submenu(parentObj)
{
	Plugin::TemplateOpenParam tmpParam;

	g_appPlugin->TemplateOpen(parent->browserRoot, template_list_view_youtube_search, tmpParam);
	submenuRoot = parent->browserRoot->FindChild(plane_list_view_youtube_search_root);
	list = (ui::ListView *)submenuRoot->FindChild(list_view_youtube);
	ListViewCb *lwCb = new ListViewCb();
	lwCb->workObj = this;
	list->SetItemFactory(lwCb);
	list->InsertSegment(0, 1);
	math::v4 sz(960.0f, 100.0f);
	list->SetCellSizeDefault(0, sz);
	list->SetSegmentLayoutType(0, ui::ListView::LAYOUT_TYPE_LIST);

	searchBox = (ui::TextBox *)submenuRoot->FindChild(text_box_top_youtube_search);
	searchButton = submenuRoot->FindChild(button_top_youtube_search);

	searchBox->AddEventCallback(ui::TextBox::CB_TEXT_BOX_ENTER_PRESSED, SearchButtonCbFun, this);
	searchButton->AddEventCallback(ui::Button::CB_BTN_DECIDE, SearchButtonCbFun, this);

	common::transition::Do(0.0f, submenuRoot, common::transition::Type_Fadein1, true, false);
}

menu::YouTube::SearchSubmenu::~SearchSubmenu()
{

}

void menu::YouTube::HistorySubmenu::HistoryJob::Run()
{
	string text8;
	char *entryData;
	int32_t ret = 0;
	InvItemVideo *invItem;
	char key[SCE_INI_FILE_PROCESSOR_KEY_BUFFER_SIZE];

	thread::RMutex::main_thread_mutex.Lock();
	workObj->parent->loaderIndicator->Start();
	thread::RMutex::main_thread_mutex.Unlock();

	ytutils::GetHistLog()->Reset();
	int32_t totalNum = ytutils::GetHistLog()->GetSize();

	while (totalNum > 30)
	{
		ytutils::GetHistLog()->GetNext(key);
		ytutils::GetHistLog()->Remove(key);
		ytutils::GetHistLog()->Reset();
		totalNum = ytutils::GetHistLog()->GetSize();
	}

	entryData = (char *)sce_paf_calloc(totalNum, SCE_INI_FILE_PROCESSOR_KEY_BUFFER_SIZE);

	for (int32_t i = 0; i < totalNum; i++)
	{
		ytutils::GetHistLog()->GetNext(entryData + (i * SCE_INI_FILE_PROCESSOR_KEY_BUFFER_SIZE));
	}

	for (int32_t i = totalNum; i > -1; i--)
	{
		if (workObj->interrupted)
		{
			break;
		}

		ret = invParseVideo(entryData + (i * SCE_INI_FILE_PROCESSOR_KEY_BUFFER_SIZE), &invItem);
		if (ret == true)
		{
			if (invItem->id)
			{
				Item item;
				item.type = INV_ITEM_TYPE_VIDEO;
				item.videoId = invItem->id;
				text8 = "\n";
				text8 += invItem->title;
				common::Utf8ToUtf16(text8, &item.name);
				item.texId = invItem->thmbUrl;
				if (invItem->isLive || invItem->lengthSec == 0)
				{
					text8 = "LIVE";
				}
				else
				{
					utils::ConvertSecondsToString(text8, invItem->lengthSec, false);
				}
				common::Utf8ToUtf16(text8, &item.time);
				text8 = "by ";
				text8 += invItem->author;
				text8 += "\n";
				text8 += invItem->published;
				common::Utf8ToUtf16(text8, &item.stat);

				workObj->results.push_back(item);
			}

			invCleanupVideo(invItem);
		}
	}

	thread::RMutex::main_thread_mutex.Lock();
	workObj->parent->loaderIndicator->Stop();
	if (!workObj->interrupted)
	{
		workObj->list->InsertCell(0, 0, workObj->results.size());
	}
	thread::RMutex::main_thread_mutex.Unlock();

	sce_paf_free(entryData);

	workObj->allJobsComplete = true;
}

void menu::YouTube::HistorySubmenu::GoToNextPage()
{

}

void menu::YouTube::HistorySubmenu::GoToPrevPage()
{

}

menu::YouTube::HistorySubmenu::HistorySubmenu(YouTube *parentObj) : Submenu(parentObj)
{
	Plugin::TemplateOpenParam tmpParam;

	g_appPlugin->TemplateOpen(parent->browserRoot, template_list_view_youtube_history, tmpParam);
	submenuRoot = parent->browserRoot->FindChild(plane_list_view_youtube_history_root);
	list = (ui::ListView *)submenuRoot->FindChild(list_view_youtube);
	ListViewCb *lwCb = new ListViewCb();
	lwCb->workObj = this;
	list->SetItemFactory(lwCb);
	list->InsertSegment(0, 1);
	math::v4 sz(960.0f, 100.0f);
	list->SetCellSizeDefault(0, sz);
	list->SetSegmentLayoutType(0, ui::ListView::LAYOUT_TYPE_LIST);

	wstring title = g_appPlugin->GetString(msg_youtube_history);
	parent->topText->SetString(title);

	common::transition::Do(0.0f, submenuRoot, common::transition::Type_Fadein1, true, false);

	if (ytutils::GetHistLog()->GetSize() > 0)
	{
		HistoryJob *job = new HistoryJob("YouTube::HistoryJob");
		job->workObj = this;
		common::SharedPtr<job::JobItem> itemParam(job);
		allJobsComplete = false;
		utils::GetJobQueue()->Enqueue(itemParam);
	}
}

menu::YouTube::HistorySubmenu::~HistorySubmenu()
{
	wstring title;
	parent->topText->SetString(title);
}

void menu::YouTube::FavouriteSubmenu::FavouriteJob::Run()
{
	string text8;
	char *entryData;
	int32_t ret = 0;
	InvItemVideo *invItem = NULL;
	bool isLastPage = false;

	thread::RMutex::main_thread_mutex.Lock();
	workObj->parent->loaderIndicator->Start();
	thread::RMutex::main_thread_mutex.Unlock();

	ytutils::GetFavLog()->Reset();
	int32_t totalNum = ytutils::GetFavLog()->GetSize();

	if (!workObj->request.empty())
	{
		isLastPage = true;
		char key[SCE_INI_FILE_PROCESSOR_KEY_BUFFER_SIZE];

		for (int32_t i = 0; i < totalNum; i++)
		{
			if (workObj->interrupted)
			{
				break;
			}

			ytutils::GetFavLog()->GetNext(key);
			ret = invParseVideo(key, &invItem);
			if (ret == true)
			{
				if (invItem->id && sce_paf_strstr(invItem->title, workObj->request.c_str()) && workObj->results.size() < 30)
				{
					Item item;
					item.type = INV_ITEM_TYPE_VIDEO;
					item.videoId = invItem->id;
					text8 = "\n";
					text8 += invItem->title;
					common::Utf8ToUtf16(text8, &item.name);
					item.texId = invItem->thmbUrl;
					if (invItem->isLive || invItem->lengthSec == 0)
					{
						text8 = "LIVE";
					}
					else
					{
						utils::ConvertSecondsToString(text8, invItem->lengthSec, false);
					}
					common::Utf8ToUtf16(text8, &item.time);
					text8 = "by ";
					text8 += invItem->author;
					text8 += "\n";
					text8 += invItem->published;
					common::Utf8ToUtf16(text8, &item.stat);

					workObj->results.push_back(item);
				}

				invCleanupVideo(invItem);
			}
		}
	}
	else
	{
		int32_t startNum = workObj->currentPage * 30;

		int32_t realNum = 30;
		if ((totalNum - startNum) < 30)
		{
			realNum = totalNum - startNum;
			isLastPage = true;
		}

		entryData = (char *)sce_paf_calloc(realNum, SCE_INI_FILE_PROCESSOR_KEY_BUFFER_SIZE);

		for (int32_t i = 0; i < startNum; i++)
		{
			ytutils::GetFavLog()->GetNext(entryData);
		}

		for (int32_t i = 0; i < realNum; i++)
		{
			ytutils::GetFavLog()->GetNext(entryData + (i * SCE_INI_FILE_PROCESSOR_KEY_BUFFER_SIZE));
		}

		for (int32_t i = 0; i < realNum; i++)
		{
			if (workObj->interrupted)
			{
				break;
			}

			ret = invParseVideo(entryData + (i * SCE_INI_FILE_PROCESSOR_KEY_BUFFER_SIZE), &invItem);
			if (ret == true)
			{
				if (invItem->id)
				{
					Item item;
					item.type = INV_ITEM_TYPE_VIDEO;
					item.videoId = invItem->id;
					text8 = "\n";
					text8 += invItem->title;
					common::Utf8ToUtf16(text8, &item.name);
					item.texId = invItem->thmbUrl;
					if (invItem->isLive || invItem->lengthSec == 0)
					{
						text8 = "LIVE";
					}
					else
					{
						utils::ConvertSecondsToString(text8, invItem->lengthSec, false);
					}
					common::Utf8ToUtf16(text8, &item.time);
					text8 = "by ";
					text8 += invItem->author;
					text8 += "\n";
					text8 += invItem->published;
					common::Utf8ToUtf16(text8, &item.stat);
					workObj->results.push_back(item);
				}

				invCleanupVideo(invItem);
			}
		}

		sce_paf_free(entryData);
	}

	thread::RMutex::main_thread_mutex.Lock();
	workObj->parent->loaderIndicator->Stop();
	if (!workObj->interrupted)
	{
		if (workObj->currentPage == 0)
		{
			if (!isLastPage)
			{
				workObj->list->InsertCell(0, 0, workObj->results.size() + 1);
			}
			else
			{
				workObj->list->InsertCell(0, 0, workObj->results.size());
			}
		}
		else
		{
			if (!isLastPage)
			{
				workObj->list->InsertCell(0, 0, workObj->results.size() + 2);
			}
			else
			{
				workObj->list->InsertCell(0, 0, workObj->results.size() + 1);
			}
		}
	}
	thread::RMutex::main_thread_mutex.Unlock();

	workObj->allJobsComplete = true;
}

void menu::YouTube::FavouriteSubmenu::SearchButtonCbFun(int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
{
	wstring text16;
	string text8;

	menu::YouTube::FavouriteSubmenu *workObj = (menu::YouTube::FavouriteSubmenu *)userdata;
	workObj->ReleaseCurrentPage();
	workObj->searchBox->EndEdit();

	workObj->request.clear();
	workObj->searchBox->GetString(text16);
	common::Utf16ToUtf8(text16, &workObj->request);

	if (workObj->request.empty() || ytutils::GetFavLog()->GetSize() == 0)
	{
		return;
	}

	FavouriteJob *job = new FavouriteJob("YouTube::FavouriteJob");
	job->workObj = workObj;
	common::SharedPtr<job::JobItem> itemParam(job);
	workObj->allJobsComplete = false;
	utils::GetJobQueue()->Enqueue(itemParam);
}

void menu::YouTube::FavouriteSubmenu::GoToNextPage()
{
	ReleaseCurrentPage();
	currentPage++;

	FavouriteJob *job = new FavouriteJob("YouTube::FavouriteJob");
	job->workObj = this;
	common::SharedPtr<job::JobItem> itemParam(job);
	allJobsComplete = false;
	utils::GetJobQueue()->Enqueue(itemParam);
}

void menu::YouTube::FavouriteSubmenu::GoToPrevPage()
{
	ReleaseCurrentPage();
	currentPage--;

	FavouriteJob *job = new FavouriteJob("YouTube::FavouriteJob");
	job->workObj = this;
	common::SharedPtr<job::JobItem> itemParam(job);
	allJobsComplete = false;
	utils::GetJobQueue()->Enqueue(itemParam);
}

menu::YouTube::FavouriteSubmenu::FavouriteSubmenu(YouTube *parentObj) : Submenu(parentObj)
{
	Plugin::TemplateOpenParam tmpParam;

	g_appPlugin->TemplateOpen(parent->browserRoot, template_list_view_youtube_fav, tmpParam);
	submenuRoot = parent->browserRoot->FindChild(plane_list_view_youtube_fav_root);
	list = (ui::ListView *)submenuRoot->FindChild(list_view_youtube);
	ListViewCb *lwCb = new ListViewCb();
	lwCb->workObj = this;
	list->SetItemFactory(lwCb);
	list->InsertSegment(0, 1);
	math::v4 sz(960.0f, 100.0f);
	list->SetCellSizeDefault(0, sz);
	list->SetSegmentLayoutType(0, ui::ListView::LAYOUT_TYPE_LIST);

	searchBox = (ui::TextBox *)submenuRoot->FindChild(text_box_top_youtube_search);
	searchButton = submenuRoot->FindChild(button_top_youtube_search);

	searchBox->AddEventCallback(ui::TextBox::CB_TEXT_BOX_ENTER_PRESSED, SearchButtonCbFun, this);
	searchButton->AddEventCallback(ui::Button::CB_BTN_DECIDE, SearchButtonCbFun, this);

	common::transition::Do(0.0f, submenuRoot, common::transition::Type_Fadein1, true, false);

	if (ytutils::GetFavLog()->GetSize() > 0)
	{
		FavouriteJob *job = new FavouriteJob("YouTube::FavouriteJob");
		job->workObj = this;
		common::SharedPtr<job::JobItem> itemParam(job);
		allJobsComplete = false;
		utils::GetJobQueue()->Enqueue(itemParam);
	}
}

menu::YouTube::FavouriteSubmenu::~FavouriteSubmenu()
{

}

void menu::YouTube::ListButtonCbFun(int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
{
	Submenu *workObj = (Submenu *)userdata;
	ui::Widget *wdg = (ui::Widget *)self;
	uint32_t totalCount = workObj->results.size();
	uint32_t idhash = wdg->GetName().GetIDHash();

	if (workObj->currentPage == 0)
	{
		if (idhash == totalCount)
		{
			workObj->GoToNextPage();
			return;
		}
	}
	else
	{
		if (idhash == totalCount)
		{
			workObj->GoToPrevPage();
			return;
		}
		else if (idhash == totalCount + 1)
		{
			workObj->GoToNextPage();
			return;
		}
	}

	Submenu::Item item = workObj->results.at(idhash);

	switch (item.type)
	{
	case INV_ITEM_TYPE_VIDEO:
		ytutils::GetHistLog()->AddAsync(item.videoId.c_str());
		utils::SetDisplayResolution(ui::EnvironmentParam::RESOLUTION_HD_FULL);
		new menu::PlayerYoutube(item.videoId.c_str(), workObj->GetType() == Submenu::SubmenuType_Favourites);
		break;
	}
}

void menu::YouTube::SettingsButtonCbFun(int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
{
	YouTube *workObj = (YouTube *)userdata;

	vector<OptionMenu::Button> buttons;
	OptionMenu::Button bt;
	bt.label = g_appPlugin->GetString(msg_settings);
	buttons.push_back(bt);
	bt.label = g_appPlugin->GetString(msg_settings_youtube_clean_history);
	buttons.push_back(bt);
	bt.label = g_appPlugin->GetString(msg_settings_youtube_clean_fav);
	buttons.push_back(bt);

	new OptionMenu(g_appPlugin, workObj->root, &buttons);
}

void menu::YouTube::OptionMenuEventCbFun(int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
{
	if (menu::GetTopMenu()->GetMenuType() != menu::MenuType_Youtube)
	{
		return;
	}

	menu::YouTube *workObj = (menu::YouTube *)userdata;

	if (e->GetValue(0) == OptionMenu::OptionMenuEvent_Close)
	{
		return;
	}

	switch (e->GetValue(1))
	{
	case 0:
		menu::SettingsButtonCbFun(ui::Button::CB_BTN_DECIDE, NULL, 0, NULL);
		break;
	case 1:
		dialog::OpenYesNo(g_appPlugin, NULL, g_appPlugin->GetString(msg_settings_youtube_clean_history_confirm));
		workObj->dialogIdx = e->GetValue(1);
		break;
	case 2:
		dialog::OpenYesNo(g_appPlugin, NULL, g_appPlugin->GetString(msg_settings_youtube_clean_fav_confirm));
		workObj->dialogIdx = e->GetValue(1);
		break;
	}
}

void menu::YouTube::DialogHandlerCbFun(int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
{
	if (menu::GetTopMenu()->GetMenuType() != menu::MenuType_Youtube)
	{
		return;
	}

	menu::YouTube *workObj = (menu::YouTube *)userdata;

	if (e->GetValue(0) == dialog::ButtonCode_Yes)
	{
		switch (workObj->dialogIdx)
		{
		case 1:
			ytutils::HistLog::Clean();
			break;
		case 2:
			ytutils::FavLog::Clean();
			break;
		}
	}
}

void menu::YouTube::BackButtonCbFun(int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
{
	YouTube *workObj = (YouTube *)userdata;

	delete workObj;
}

void menu::YouTube::SubmenuButtonCbFun(int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
{
	YouTube *workObj = (YouTube *)userdata;
	ui::Widget *wdg = (ui::Widget *)self;
	workObj->SwitchSubmenu((Submenu::SubmenuType)wdg->GetName().GetIDHash());
}

menu::YouTube::YouTube() :
	GenericMenu("page_youtube",
	MenuOpenParam(false, 200.0f, Plugin::TransitionType_SlideFromBottom),
	MenuCloseParam(false, 200.0f, Plugin::TransitionType_SlideFromBottom))
{
	currentSubmenu = NULL;

	root->AddEventCallback(OptionMenu::OptionMenuEvent, OptionMenuEventCbFun, this);
	root->AddEventCallback(dialog::DialogEvent, DialogHandlerCbFun, this);

	ui::Widget *settingsButton = root->FindChild(button_settings_page_youtube);
	settingsButton->Show(common::transition::Type_Reset);
	settingsButton->AddEventCallback(ui::Button::CB_BTN_DECIDE, SettingsButtonCbFun, this);

	ui::Widget *backButton = root->FindChild(button_back_page_youtube);
	backButton->Show(common::transition::Type_Reset);
	backButton->AddEventCallback(ui::Button::CB_BTN_DECIDE, BackButtonCbFun, this);

	browserRoot = root->FindChild(plane_browser_root_page_youtube);
	loaderIndicator = (ui::BusyIndicator *)root->FindChild(busyindicator_loader_page_youtube);
	topText = (ui::Text *)root->FindChild(text_top);
	btMenu = (ui::Box *)root->FindChild(box_bottommenu_page_youtube);

	searchBt = btMenu->FindChild(button_yt_btmenu_search);
	histBt = btMenu->FindChild(button_yt_btmenu_history);
	favBt = btMenu->FindChild(button_yt_btmenu_favourite);
	searchBt->AddEventCallback(ui::Button::CB_BTN_DECIDE, SubmenuButtonCbFun, this);
	histBt->AddEventCallback(ui::Button::CB_BTN_DECIDE, SubmenuButtonCbFun, this);
	favBt->AddEventCallback(ui::Button::CB_BTN_DECIDE, SubmenuButtonCbFun, this);

	char instance[256];
	sce_paf_memset(instance, 0, sizeof(instance));
	menu::Settings::GetAppSetInstance()->GetString("inv_instance", instance, sizeof(instance), "");
	invSetInstanceUrl(instance);

	texPool = new TexPool(g_appPlugin);
	texPool->SetShare(utils::GetShare());

	SwitchSubmenu(Submenu::SubmenuType_Search);
}

menu::YouTube::~YouTube()
{
	delete currentSubmenu;
	texPool->DestroyAsync();
}

void menu::YouTube::SwitchSubmenu(Submenu::SubmenuType type)
{
	if (currentSubmenu)
	{
		if (currentSubmenu->GetType() == type)
			return;
		delete currentSubmenu;
		currentSubmenu = NULL;
	}

	switch (type)
	{
	case Submenu::SubmenuType_Search:
		currentSubmenu = new SearchSubmenu(this);
		break;
	case Submenu::SubmenuType_History:
		currentSubmenu = new HistorySubmenu(this);
		break;
	case Submenu::SubmenuType_Favourites:
		currentSubmenu = new FavouriteSubmenu(this);
		break;
	}
}