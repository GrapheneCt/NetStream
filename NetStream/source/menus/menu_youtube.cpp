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
	interrupted = SCE_FALSE;
	allJobsComplete = SCE_TRUE;
}

menu::YouTube::Submenu::~Submenu()
{
	list->elem.hash = 0;
	ReleaseCurrentPage();
	effect::Play(0.0f, submenuRoot, effect::EffectType_Fadein1, true, false);
}

SceVoid menu::YouTube::Submenu::ReleaseCurrentPage()
{
	interrupted = SCE_TRUE;
	while (!allJobsComplete)
	{
		thread::s_mainThreadMutex.Unlock();
		thread::Sleep(10);
		thread::s_mainThreadMutex.Lock();
	}
	interrupted = SCE_FALSE;

	if (list->GetCellNum(0) > 0)
	{
		list->RemoveItem(0, 0, list->GetCellNum(0));
	}

	for (Item i : results)
	{
		if (i.surface)
		{
			i.surface->Release();
		}
	}

	results.clear();
}

ui::ListItem *menu::YouTube::Submenu::ListViewCb::Create(Param *info)
{
	rco::Element searchParam;
	Plugin::TemplateOpenParam tmpParam;
	ui::Widget *item = SCE_NULL;
	ui::Widget *button = SCE_NULL;
	ui::Text *timeText = SCE_NULL;
	ui::Widget *subText = SCE_NULL;
	graph::Surface *tex = SCE_NULL;
	wstring text16;
	wchar_t numPageText[32];

	if (!info->list->elem.hash)
	{
		return new ui::ListItem(info->parent, 0);
	}

	ui::Widget *targetRoot = info->parent;
	Item *workItem = &workObj->results.at(info->cellIndex);
	SceUInt32 totalCount = workObj->results.size();

	if (info->cellIndex == totalCount || info->cellIndex == totalCount + 1)
	{
		searchParam.hash = template_list_item_youtube_aligned;
		g_appPlugin->TemplateOpen(targetRoot, &searchParam, &tmpParam);
		item = targetRoot->GetChild(targetRoot->childNum - 1);
		button = utils::GetChild(item, image_button_list_item_youtube_aligned);
	}
	else
	{
		searchParam.hash = template_list_item_youtube;
		g_appPlugin->TemplateOpen(targetRoot, &searchParam, &tmpParam);
		item = targetRoot->GetChild(targetRoot->childNum - 1);
		button = utils::GetChild(item, image_button_list_item_youtube);
	}

	button->elem.hash = info->cellIndex;

	if (info->cellIndex == totalCount)
	{
		if (workObj->currentPage == 0)
		{
			text16 = utils::GetString(msg_next_page);
			sce_paf_swprintf(numPageText, sizeof(numPageText) / 2, L" (%d)", workObj->currentPage + 1);
			text16 += numPageText;
			button->SetLabel(&text16);
			tex = utils::GetTexture(tex_button_arrow_right);
			button->SetSurfaceBase(&tex);
			goto serviceButton;
		}
		else
		{
			text16 = utils::GetString(msg_previous_page);
			sce_paf_swprintf(numPageText, sizeof(numPageText) / 2, L" (%d)", workObj->currentPage - 1);
			text16 += numPageText;
			button->SetLabel(&text16);
			tex = utils::GetTexture(tex_button_arrow_left);
			button->SetSurfaceBase(&tex);
			goto serviceButton;
		}
	}
	else if (info->cellIndex == totalCount + 1)
	{
		text16 = utils::GetString(msg_next_page);
		sce_paf_swprintf(numPageText, sizeof(numPageText) / 2, L" (%d)", workObj->currentPage + 1);
		text16 += numPageText;
		button->SetLabel(&text16);
		tex = utils::GetTexture(tex_button_arrow_right);
		button->SetSurfaceBase(&tex);
		goto serviceButton;
	}

	timeText = (ui::Text *)utils::GetChild(button, text_list_item_youtube_time);
	subText = utils::GetChild(button, text_list_item_youtube_subtext);
	if (workItem->time == L"LIVE")
	{
		Rgba col(1.0f, 0.0f, 0.0f, 1.0f);
		timeText->SetColor(ui::Text::ColorType_Background, 0, 0, &col);
	}
	timeText->SetLabel(&workItem->time);
	subText->SetLabel(&workItem->stat);
	button->SetLabel(&workItem->name);
	if (!workItem->surface)
	{
		new utils::AsyncNetworkSurfaceLoader(workItem->surfacePath.c_str(), button, &workItem->surface);
	}
	else
	{
		button->SetSurfaceBase(&workItem->surface);
	}

serviceButton:

	button->RegisterEventCallback(ui::EventMain_Decide, new  utils::SimpleEventCallback(ListButtonCbFun, workObj));

	return (ui::ListItem *)item;
}

SceVoid menu::YouTube::SearchSubmenu::SearchJob::Run()
{
	string text8;
	InvItem *items = SCE_NULL;
	SceInt32 ret = -1;

	thread::s_mainThreadMutex.Lock();
	workObj->parent->loaderIndicator->Start();
	thread::s_mainThreadMutex.Unlock();

	if (!isId)
	{
		InvSort sort;
		InvDate date;
		sce::AppSettings *settings = menu::Settings::GetAppSetInstance();
		settings->GetInt("yt_search_sort", (SceInt32 *)&sort, 0);
		settings->GetInt("yt_search_date", (SceInt32 *)&date, 0);
		ret = invParseSearch(workObj->request.c_str(), workObj->currentPage, INV_ITEM_TYPE_VIDEO, sort, date, &items);
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
		dialog::OpenError(g_appPlugin, ret, utils::GetString("msg_error_connect_server_peer"));
		thread::s_mainThreadMutex.Lock();
		workObj->parent->loaderIndicator->Stop();
		thread::s_mainThreadMutex.Unlock();
		workObj->allJobsComplete = SCE_TRUE;
		return;
	}

	for (int i = 0; i < ret; i++)
	{
		Item item;
		item.type = items[i].type;
		switch (item.type)
		{
		case INV_ITEM_TYPE_VIDEO:
			item.id = items[i].videoItem->id;
			text8 = "\n";
			text8 += items[i].videoItem->title;
			common::Utf8ToUtf16(text8, &item.name);
			item.surfacePath = items[i].videoItem->thmbUrl;
			if (items[i].videoItem->isLive || items[i].videoItem->lengthSec == 0)
			{
				text8 = "LIVE";
			}
			else
			{
				utils::ConvertSecondsToString(text8, items[i].videoItem->lengthSec, SCE_FALSE);
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

	thread::s_mainThreadMutex.Lock();
	workObj->parent->loaderIndicator->Stop();
	if (workObj->currentPage == 0)
	{
		if (!isId)
		{
			workObj->list->AddItem(0, 0, workObj->results.size() + 1);
		}
		else
		{
			workObj->list->AddItem(0, 0, workObj->results.size());
		}
	}
	else
	{
		workObj->list->AddItem(0, 0, workObj->results.size() + 2);
	}
	thread::s_mainThreadMutex.Unlock();

	if (!isId)
	{
		invCleanupSearch(items);
	}
	else
	{
		invCleanupVideo(items->videoItem);
		delete items;
	}

	workObj->allJobsComplete = SCE_TRUE;
}

SceVoid menu::YouTube::SearchSubmenu::SearchButtonCbFun(SceInt32 eventId, ui::Widget *self, SceInt32 a3, ScePVoid pUserData)
{
	wstring text16;
	string text8;
	SceBool isId = SCE_FALSE;

	menu::YouTube::SearchSubmenu *workObj = (menu::YouTube::SearchSubmenu *)pUserData;
	workObj->ReleaseCurrentPage();
	workObj->searchBox->Hide();

	workObj->request.clear();
	workObj->searchBox->GetLabel(&text16);
	common::Utf16ToUtf8(text16, &workObj->request);

	if (workObj->request.empty())
	{
		return;
	}
	else if ((sce_paf_strstr(workObj->request.c_str(), "id:") == workObj->request.c_str()) && workObj->request.length() == 14)
	{
		isId = SCE_TRUE;
	}

	workObj->currentPage = 0;

	SearchJob *job = new SearchJob("YouTube::SearchJob");
	job->workObj = workObj;
	job->isId = isId;
	common::SharedPtr<job::JobItem> itemParam(job);
	workObj->allJobsComplete = SCE_FALSE;
	utils::GetJobQueue()->Enqueue(itemParam);
}

SceVoid menu::YouTube::SearchSubmenu::GoToNextPage()
{
	ReleaseCurrentPage();
	currentPage++;

	SearchJob *job = new SearchJob("YouTube::SearchJob");
	job->workObj = this;
	job->isId = SCE_FALSE;
	common::SharedPtr<job::JobItem> itemParam(job);
	allJobsComplete = SCE_FALSE;
	utils::GetJobQueue()->Enqueue(itemParam);
}

SceVoid menu::YouTube::SearchSubmenu::GoToPrevPage()
{
	ReleaseCurrentPage();
	currentPage--;

	SearchJob *job = new SearchJob("YouTube::SearchJob");
	job->workObj = this;
	job->isId = SCE_FALSE;
	common::SharedPtr<job::JobItem> itemParam(job);
	allJobsComplete = SCE_FALSE;
	utils::GetJobQueue()->Enqueue(itemParam);
}

menu::YouTube::SearchSubmenu::SearchSubmenu(YouTube *parentObj) : Submenu(parentObj)
{
	rco::Element searchParam;
	Plugin::TemplateOpenParam tmpParam;

	searchParam.hash = template_list_view_youtube_search;
	g_appPlugin->TemplateOpen(parent->browserRoot, &searchParam, &tmpParam);
	submenuRoot = utils::GetChild(parent->browserRoot, plane_list_view_youtube_search_root);
	list = (ui::ListView *)utils::GetChild(submenuRoot, list_view_youtube);
	ListViewCb *lwCb = new ListViewCb();
	lwCb->workObj = this;
	list->RegisterItemCallback(lwCb);
	list->SetSegmentEnable(0, 1);
	Vector4 sz(960.0f, 100.0f);
	list->SetCellSize(0, &sz);
	list->SetConfigurationType(0, ui::ListView::ConfigurationType_Simple);

	searchBox = (ui::TextBox *)utils::GetChild(submenuRoot, text_box_top_youtube_search);
	searchButton = utils::GetChild(submenuRoot, button_top_youtube_search);

	searchBox->RegisterEventCallback(0x1000000B, new utils::SimpleEventCallback(SearchButtonCbFun, this));
	searchButton->RegisterEventCallback(ui::EventMain_Decide, new utils::SimpleEventCallback(SearchButtonCbFun, this));

	effect::PlayReverse(0.0f, submenuRoot, effect::EffectType_Fadein1, true, false);
}

menu::YouTube::SearchSubmenu::~SearchSubmenu()
{

}

SceVoid menu::YouTube::HistorySubmenu::HistoryJob::Run()
{
	string text8;
	char *entryData;
	SceInt32 ret = 0;
	InvItemVideo *invItem;
	char key[SCE_INI_FILE_PROCESSOR_KEY_BUFFER_SIZE];

	thread::s_mainThreadMutex.Lock();
	workObj->parent->loaderIndicator->Start();
	thread::s_mainThreadMutex.Unlock();

	ytutils::GetHistLog()->Reset();
	SceInt32 totalNum = ytutils::GetHistLog()->GetSize();

	while (totalNum > 30)
	{
		ytutils::GetHistLog()->GetNext(key);
		ytutils::GetHistLog()->Remove(key);
		ytutils::GetHistLog()->Reset();
		totalNum = ytutils::GetHistLog()->GetSize();
	}

	entryData = (char *)sce_paf_calloc(totalNum, SCE_INI_FILE_PROCESSOR_KEY_BUFFER_SIZE);

	for (SceInt32 i = 0; i < totalNum; i++)
	{
		ytutils::GetHistLog()->GetNext(entryData + (i * SCE_INI_FILE_PROCESSOR_KEY_BUFFER_SIZE));
	}

	for (SceInt32 i = totalNum; i > -1; i--)
	{
		if (workObj->interrupted)
		{
			break;
		}

		ret = invParseVideo(entryData + (i * SCE_INI_FILE_PROCESSOR_KEY_BUFFER_SIZE), &invItem);
		if (ret == SCE_TRUE)
		{
			if (invItem->id)
			{
				Item item;
				item.type = INV_ITEM_TYPE_VIDEO;
				item.id = invItem->id;
				text8 = "\n";
				text8 += invItem->title;
				common::Utf8ToUtf16(text8, &item.name);
				item.surfacePath = invItem->thmbUrl;
				if (invItem->isLive || invItem->lengthSec == 0)
				{
					text8 = "LIVE";
				}
				else
				{
					utils::ConvertSecondsToString(text8, invItem->lengthSec, SCE_FALSE);
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

	thread::s_mainThreadMutex.Lock();
	workObj->parent->loaderIndicator->Stop();
	if (!workObj->interrupted)
	{
		workObj->list->AddItem(0, 0, workObj->results.size());
	}
	thread::s_mainThreadMutex.Unlock();

	sce_paf_free(entryData);

	workObj->allJobsComplete = SCE_TRUE;
}

SceVoid menu::YouTube::HistorySubmenu::GoToNextPage()
{

}

SceVoid menu::YouTube::HistorySubmenu::GoToPrevPage()
{

}

menu::YouTube::HistorySubmenu::HistorySubmenu(YouTube *parentObj) : Submenu(parentObj)
{
	rco::Element searchParam;
	Plugin::TemplateOpenParam tmpParam;

	searchParam.hash = template_list_view_youtube_history;
	g_appPlugin->TemplateOpen(parent->browserRoot, &searchParam, &tmpParam);
	submenuRoot = utils::GetChild(parent->browserRoot, plane_list_view_youtube_history_root);
	list = (ui::ListView *)utils::GetChild(submenuRoot, list_view_youtube);
	ListViewCb *lwCb = new ListViewCb();
	lwCb->workObj = this;
	list->RegisterItemCallback(lwCb);
	list->SetSegmentEnable(0, 1);
	Vector4 sz(960.0f, 100.0f);
	list->SetCellSize(0, &sz);
	list->SetConfigurationType(0, ui::ListView::ConfigurationType_Simple);

	wstring title = utils::GetString(msg_youtube_history);
	parent->topText->SetLabel(&title);

	effect::PlayReverse(0.0f, submenuRoot, effect::EffectType_Fadein1, true, false);

	if (ytutils::GetHistLog()->GetSize() > 0)
	{
		HistoryJob *job = new HistoryJob("YouTube::HistoryJob");
		job->workObj = this;
		common::SharedPtr<job::JobItem> itemParam(job);
		allJobsComplete = SCE_FALSE;
		utils::GetJobQueue()->Enqueue(itemParam);
	}
}

menu::YouTube::HistorySubmenu::~HistorySubmenu()
{
	wstring title;
	parent->topText->SetLabel(&title);
}

SceVoid menu::YouTube::FavouriteSubmenu::FavouriteJob::Run()
{
	string text8;
	char *entryData;
	SceInt32 ret = 0;
	InvItemVideo *invItem = SCE_NULL;
	SceBool isLastPage = SCE_FALSE;

	thread::s_mainThreadMutex.Lock();
	workObj->parent->loaderIndicator->Start();
	thread::s_mainThreadMutex.Unlock();

	ytutils::GetFavLog()->Reset();
	SceInt32 totalNum = ytutils::GetFavLog()->GetSize();

	if (!workObj->request.empty())
	{
		isLastPage = SCE_TRUE;
		char key[SCE_INI_FILE_PROCESSOR_KEY_BUFFER_SIZE];

		for (SceInt32 i = 0; i < totalNum; i++)
		{
			if (workObj->interrupted)
			{
				break;
			}

			ytutils::GetFavLog()->GetNext(key);
			ret = invParseVideo(key, &invItem);
			if (ret == SCE_TRUE)
			{
				if (invItem->id && sce_paf_strstr(invItem->title, workObj->request.c_str()) && workObj->results.size() < 30)
				{
					Item item;
					item.type = INV_ITEM_TYPE_VIDEO;
					item.id = invItem->id;
					text8 = "\n";
					text8 += invItem->title;
					common::Utf8ToUtf16(text8, &item.name);
					item.surfacePath = invItem->thmbUrl;
					if (invItem->isLive || invItem->lengthSec == 0)
					{
						text8 = "LIVE";
					}
					else
					{
						utils::ConvertSecondsToString(text8, invItem->lengthSec, SCE_FALSE);
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
		SceInt32 startNum = workObj->currentPage * 30;

		SceInt32 realNum = 30;
		if ((totalNum - startNum) < 30)
		{
			realNum = totalNum - startNum;
			isLastPage = SCE_TRUE;
		}

		entryData = (char *)sce_paf_calloc(realNum, SCE_INI_FILE_PROCESSOR_KEY_BUFFER_SIZE);

		for (SceInt32 i = 0; i < startNum; i++)
		{
			ytutils::GetFavLog()->GetNext(entryData);
		}

		for (SceInt32 i = 0; i < realNum; i++)
		{
			ytutils::GetFavLog()->GetNext(entryData + (i * SCE_INI_FILE_PROCESSOR_KEY_BUFFER_SIZE));
		}

		for (SceInt32 i = 0; i < realNum; i++)
		{
			if (workObj->interrupted)
			{
				break;
			}

			ret = invParseVideo(entryData + (i * SCE_INI_FILE_PROCESSOR_KEY_BUFFER_SIZE), &invItem);
			if (ret == SCE_TRUE)
			{
				if (invItem->id)
				{
					Item item;
					item.type = INV_ITEM_TYPE_VIDEO;
					item.id = invItem->id;
					text8 = "\n";
					text8 += invItem->title;
					common::Utf8ToUtf16(text8, &item.name);
					item.surfacePath = invItem->thmbUrl;
					if (invItem->isLive || invItem->lengthSec == 0)
					{
						text8 = "LIVE";
					}
					else
					{
						utils::ConvertSecondsToString(text8, invItem->lengthSec, SCE_FALSE);
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
	thread::s_mainThreadMutex.Lock();
	workObj->parent->loaderIndicator->Stop();
	if (!workObj->interrupted)
	{
		if (workObj->currentPage == 0)
		{
			if (!isLastPage)
			{
				workObj->list->AddItem(0, 0, workObj->results.size() + 1);
			}
			else
			{
				workObj->list->AddItem(0, 0, workObj->results.size());
			}
		}
		else
		{
			if (!isLastPage)
			{
				workObj->list->AddItem(0, 0, workObj->results.size() + 2);
			}
			else
			{
				workObj->list->AddItem(0, 0, workObj->results.size() + 1);
			}
		}
	}
	thread::s_mainThreadMutex.Unlock();

	workObj->allJobsComplete = SCE_TRUE;
}

SceVoid menu::YouTube::FavouriteSubmenu::SearchButtonCbFun(SceInt32 eventId, ui::Widget *self, SceInt32 a3, ScePVoid pUserData)
{
	wstring text16;
	string text8;

	menu::YouTube::FavouriteSubmenu *workObj = (menu::YouTube::FavouriteSubmenu *)pUserData;
	workObj->ReleaseCurrentPage();
	workObj->searchBox->Hide();

	workObj->request.clear();
	workObj->searchBox->GetLabel(&text16);
	common::Utf16ToUtf8(text16, &workObj->request);

	if (workObj->request.empty() || ytutils::GetFavLog()->GetSize() == 0)
	{
		return;
	}

	FavouriteJob *job = new FavouriteJob("YouTube::FavouriteJob");
	job->workObj = workObj;
	common::SharedPtr<job::JobItem> itemParam(job);
	workObj->allJobsComplete = SCE_FALSE;
	utils::GetJobQueue()->Enqueue(itemParam);
}

SceVoid menu::YouTube::FavouriteSubmenu::GoToNextPage()
{
	ReleaseCurrentPage();
	currentPage++;

	FavouriteJob *job = new FavouriteJob("YouTube::FavouriteJob");
	job->workObj = this;
	common::SharedPtr<job::JobItem> itemParam(job);
	allJobsComplete = SCE_FALSE;
	utils::GetJobQueue()->Enqueue(itemParam);
}

SceVoid menu::YouTube::FavouriteSubmenu::GoToPrevPage()
{
	ReleaseCurrentPage();
	currentPage--;

	FavouriteJob *job = new FavouriteJob("YouTube::FavouriteJob");
	job->workObj = this;
	common::SharedPtr<job::JobItem> itemParam(job);
	allJobsComplete = SCE_FALSE;
	utils::GetJobQueue()->Enqueue(itemParam);
}

menu::YouTube::FavouriteSubmenu::FavouriteSubmenu(YouTube *parentObj) : Submenu(parentObj)
{
	rco::Element searchParam;
	Plugin::TemplateOpenParam tmpParam;

	searchParam.hash = template_list_view_youtube_fav;
	g_appPlugin->TemplateOpen(parent->browserRoot, &searchParam, &tmpParam);
	submenuRoot = utils::GetChild(parent->browserRoot, plane_list_view_youtube_fav_root);
	list = (ui::ListView *)utils::GetChild(submenuRoot, list_view_youtube);
	ListViewCb *lwCb = new ListViewCb();
	lwCb->workObj = this;
	list->RegisterItemCallback(lwCb);
	list->SetSegmentEnable(0, 1);
	Vector4 sz(960.0f, 100.0f);
	list->SetCellSize(0, &sz);
	list->SetConfigurationType(0, ui::ListView::ConfigurationType_Simple);

	searchBox = (ui::TextBox *)utils::GetChild(submenuRoot, text_box_top_youtube_search);
	searchButton = utils::GetChild(submenuRoot, button_top_youtube_search);

	searchBox->RegisterEventCallback(0x1000000B, new utils::SimpleEventCallback(SearchButtonCbFun, this));
	searchButton->RegisterEventCallback(ui::EventMain_Decide, new utils::SimpleEventCallback(SearchButtonCbFun, this));

	effect::PlayReverse(0.0f, submenuRoot, effect::EffectType_Fadein1, true, false);

	if (ytutils::GetFavLog()->GetSize() > 0)
	{
		FavouriteJob *job = new FavouriteJob("YouTube::FavouriteJob");
		job->workObj = this;
		common::SharedPtr<job::JobItem> itemParam(job);
		allJobsComplete = SCE_FALSE;
		utils::GetJobQueue()->Enqueue(itemParam);
	}
}

menu::YouTube::FavouriteSubmenu::~FavouriteSubmenu()
{

}

SceVoid menu::YouTube::ListButtonCbFun(SceInt32 eventId, ui::Widget *self, SceInt32 a3, ScePVoid pUserData)
{
	Submenu *workObj = (Submenu *)pUserData;
	SceUInt32 totalCount = workObj->results.size();

	if (workObj->currentPage == 0)
	{
		if (self->elem.hash == totalCount)
		{
			workObj->GoToNextPage();
			return;
		}
	}
	else
	{
		if (self->elem.hash == totalCount)
		{
			workObj->GoToPrevPage();
			return;
		}
		else if (self->elem.hash == totalCount + 1)
		{
			workObj->GoToNextPage();
			return;
		}
	}

	SceInt32 idx = self->elem.hash;

	Submenu::Item item = workObj->results.at(idx);

	switch (item.type)
	{
	case INV_ITEM_TYPE_VIDEO:
		ytutils::GetHistLog()->AddAsync(item.id.c_str());
		if (workObj->GetType() == Submenu::SubmenuType_Favourites)
		{
			new menu::PlayerYoutube(item.id.c_str(), SCE_TRUE);
		}
		else
		{
			new menu::PlayerYoutube(item.id.c_str(), SCE_FALSE);
		}
		break;
	}
}

SceVoid menu::YouTube::SettingsButtonCbFun(SceInt32 eventId, ui::Widget *self, SceInt32 a3, ScePVoid pUserData)
{
	YouTube *workObj = (YouTube *)pUserData;

	vector<OptionMenu::Button> buttons;
	OptionMenu::Button bt;
	bt.label = utils::GetString(msg_settings);
	buttons.push_back(bt);
	bt.label = utils::GetString(msg_settings_youtube_clean_history);
	buttons.push_back(bt);
	bt.label = utils::GetString(msg_settings_youtube_clean_fav);
	buttons.push_back(bt);

	new OptionMenu(g_appPlugin, workObj->root, &buttons, OptionButtonCb, SCE_NULL, SCE_NULL);
}

SceVoid menu::YouTube::OptionButtonCb(SceUInt32 index, ScePVoid pUserData)
{
	switch (index)
	{
	case 0:
		menu::SettingsButtonCbFun(ui::EventMain_Decide, SCE_NULL, 0, SCE_NULL);
		break;
	case 1:
		dialog::OpenYesNo(g_appPlugin, SCE_NULL, utils::GetString(msg_settings_youtube_clean_history_confirm), DialogEventHandler, (ScePVoid)index);
		break;
	case 2:
		dialog::OpenYesNo(g_appPlugin, SCE_NULL, utils::GetString(msg_settings_youtube_clean_fav_confirm), DialogEventHandler, (ScePVoid)index);
		break;
	}
}

SceVoid menu::YouTube::DialogEventHandler(dialog::ButtonCode buttonCode, ScePVoid pUserArg)
{
	SceUInt32 index = (SceUInt32)pUserArg;

	if (buttonCode == dialog::ButtonCode_Yes)
	{
		switch (index)
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

SceVoid menu::YouTube::BackButtonCbFun(SceInt32 eventId, ui::Widget *self, SceInt32 a3, ScePVoid pUserData)
{
	YouTube *workObj = (YouTube *)pUserData;

	delete workObj;
}

SceVoid menu::YouTube::SubmenuButtonCbFun(SceInt32 eventId, ui::Widget *self, SceInt32 a3, ScePVoid pUserData)
{
	YouTube *workObj = (YouTube *)pUserData;
	workObj->SwitchSubmenu((Submenu::SubmenuType)self->elem.hash);
}

menu::YouTube::YouTube() :
	GenericMenu("page_youtube",
	MenuOpenParam(false, 200.0f, Plugin::PageEffectType_SlideFromBottom),
	MenuCloseParam(false, 200.0f, Plugin::PageEffectType_SlideFromBottom))
{
	currentSubmenu = SCE_NULL;

	ui::Widget *settingsButton = utils::GetChild(root, button_settings_page_youtube);
	settingsButton->PlayEffect(0.0f, effect::EffectType_Reset);
	settingsButton->RegisterEventCallback(ui::EventMain_Decide, new utils::SimpleEventCallback(SettingsButtonCbFun, this));

	ui::Widget *backButton = utils::GetChild(root, button_back_page_youtube);
	backButton->PlayEffect(0.0f, effect::EffectType_Reset);
	backButton->RegisterEventCallback(ui::EventMain_Decide, new utils::SimpleEventCallback(BackButtonCbFun, this));

	browserRoot = utils::GetChild(root, plane_browser_root_page_youtube);
	loaderIndicator = (ui::BusyIndicator *)utils::GetChild(root, busyindicator_loader_page_youtube);
	topText = (ui::Text *)utils::GetChild(root, text_top);
	btMenu = (ui::Box *)utils::GetChild(root, box_bottommenu_page_youtube);

	searchBt = utils::GetChild(btMenu, button_yt_btmenu_search);
	histBt = utils::GetChild(btMenu, button_yt_btmenu_history);
	favBt = utils::GetChild(btMenu, button_yt_btmenu_favourite);
	searchBt->RegisterEventCallback(ui::EventMain_Decide, new utils::SimpleEventCallback(SubmenuButtonCbFun, this));
	histBt->RegisterEventCallback(ui::EventMain_Decide, new utils::SimpleEventCallback(SubmenuButtonCbFun, this));
	favBt->RegisterEventCallback(ui::EventMain_Decide, new utils::SimpleEventCallback(SubmenuButtonCbFun, this));

	char instance[256];
	sce_paf_memset(instance, 0, sizeof(instance));
	menu::Settings::GetAppSetInstance()->GetString("inv_instance", instance, sizeof(instance), "");
	invSetInstanceUrl(instance);

	SwitchSubmenu(Submenu::SubmenuType_Search);
}

menu::YouTube::~YouTube()
{
	delete currentSubmenu;
}

SceVoid menu::YouTube::SwitchSubmenu(Submenu::SubmenuType type)
{
	if (currentSubmenu)
	{
		if (currentSubmenu->GetType() == type)
			return;
		delete currentSubmenu;
		currentSubmenu = SCE_NULL;
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