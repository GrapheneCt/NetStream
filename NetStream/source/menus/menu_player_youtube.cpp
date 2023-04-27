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
#include "menus/menu_player_youtube.h"
#include "menus/menu_player_simple.h"
#include "menus/menu_settings.h"

using namespace paf;

void menu::PlayerYoutube::LoadJob::Run()
{
	InvItemVideo *invItem = NULL;
	int32_t ret = 0;
	string text8;
	wstring text16;

	ret = invParseVideo(workObj->videoId.c_str(), &invItem);
	if (ret == true)
	{
		if (invItem->id)
		{
			if (!workObj->lastAttempt)
			{
				workObj->description = invItem->description;

				thread::RMutex::main_thread_mutex.Lock();

				DescButtonCbFun(0, workObj->descButton, 0, workObj);

				text8 = invItem->title;
				common::Utf8ToUtf16(text8, &text16);
				workObj->title->SetString(text16);

				text8 = "Uploaded by ";
				text8 += invItem->author;
				common::Utf8ToUtf16(text8, &text16);
				workObj->stat0->SetString(text16);

				text8 = invItem->subCount;
				text8 += " subscribers";
				common::Utf8ToUtf16(text8, &text16);
				workObj->stat1->SetString(text16);

				text8 = invItem->published;
				common::Utf8ToUtf16(text8, &text16);
				workObj->stat2->SetString(text16);

				thread::RMutex::main_thread_mutex.Unlock();
			}

			int32_t quality = 0;
			sce::AppSettings *settings = menu::Settings::GetAppSetInstance();

			if (invItem->isLive)
			{
				if (workObj->lastAttempt)
				{
					ret = -1;
					goto releaseLocks;
				}

				char *hls = new char[SCE_KERNEL_1KiB];
				hls[0] = 0;
				settings->GetInt("yt_hls_quality", (int32_t *)&quality, 0);
				ret = invGetHlsUrl(workObj->videoId.c_str(), (InvHlsQuality)quality, hls, SCE_KERNEL_1KiB);
				if (ret != SCE_OK)
				{
					if (quality == 0)
					{
						quality++;
					}
					else
					{
						quality--;
					}
					invGetHlsUrl(workObj->videoId.c_str(), (InvHlsQuality)quality, hls, SCE_KERNEL_1KiB);
				}
				workObj->videoLink = hls;
				workObj->isHls = true;
				if (quality == 4)
				{
					workObj->isHighHls = true;
				}
				delete hls;

				ui::Widget *livePlane = workObj->root->FindChild(plane_youtube_live_now);
				intrusive_ptr<graph::Surface> liveTex = g_appPlugin->GetTexture(tex_yt_icon_live_now);
				thread::RMutex::main_thread_mutex.Lock();
				livePlane->SetTexture(liveTex);
				thread::RMutex::main_thread_mutex.Unlock();
			}
			else
			{
				settings->GetInt("yt_quality", (int32_t *)&quality, 0);
				if (workObj->lastAttempt)
				{
					char lastResort[256];
					lastResort[0] = 0;
					ret = invGetProxyUrl(workObj->videoId.c_str(), (InvVideoQuality)quality, lastResort, sizeof(lastResort));
					workObj->videoLink = lastResort;
				}
				else
				{
					switch (quality)
					{
					case 0:
						if (invItem->avcLqUrl)
							workObj->videoLink = invItem->avcLqUrl;
						else
							workObj->videoLink = invItem->avcHqUrl;
						break;
					case 1:
						if (invItem->avcHqUrl)
							workObj->videoLink = invItem->avcHqUrl;
						else
							workObj->videoLink = invItem->avcLqUrl;
						break;
					}
				}
			}
		}
		else
		{
			ret = -1;
		}

		invCleanupVideo(invItem);
	}
	else
	{
		ret = -1;
	}

releaseLocks:

	if (ret < 0)
	{
		workObj->videoLink = "";
	}

	common::MainThreadCallList::Register(menu::PlayerYoutube::TaskLoadSecondStage, workObj);
}

ui::ListItem *menu::PlayerYoutube::HlsCommentListViewCb::Create(CreateParam& param)
{
	Plugin::TemplateOpenParam tmpParam;
	ui::Widget *item = NULL;
	wstring text16;

	ui::Widget *targetRoot = param.parent;

	g_appPlugin->TemplateOpen(targetRoot, template_list_item_youtube_comment, tmpParam);
	item = targetRoot->GetChild(targetRoot->GetChildrenNum() - 1);

	ui::Widget *button = item->FindChild(button_yt_companel_comment_item);
	button->SetName(HLS_COMMENT_MAGIC + param.cell_index);

	return (ui::ListItem *)item;
}

ui::ListItem *menu::PlayerYoutube::CommentListViewCb::Create(CreateParam& param)
{
	Plugin::TemplateOpenParam tmpParam;
	ui::Widget *item = NULL;
	wstring text16;

	if (!param.list_view->GetName().GetIDHash())
	{
		return new ui::ListItem(param.parent, 0);
	}

	ui::Widget *targetRoot = param.parent;

	g_appPlugin->TemplateOpen(targetRoot, template_list_item_youtube_comment, tmpParam);
	item = targetRoot->GetChild(targetRoot->GetChildrenNum() - 1);

	ui::Widget *button = item->FindChild(button_yt_companel_comment_item);
	button->SetName(param.cell_index);

	button->AddEventCallback(ui::Button::CB_BTN_DECIDE, CommentBodyButtonCbFun, workObj);

	if (param.cell_index == workObj->commentItems.size())
	{
		text16 = g_appPlugin->GetString(msg_youtube_comment_more);
	}
	else
	{
		CommentItem entry = workObj->commentItems.at(param.cell_index);
		text16 = entry.author + L"  " + entry.date + L"\n" + entry.content;

		if (!entry.replyContinuation.empty())
		{
			//TODO: implement comment replies
		}
	}

	button->SetString(text16);

	return (ui::ListItem *)item;
}

void menu::PlayerYoutube::HlsCommentParseThread::EntryFunction()
{
	ui::ListView *list = (ui::ListView *)workObj->companelRoot;
	ui::Widget *button = NULL;
	InvItemComment *comment;
	int32_t commCount = 0;
	uint32_t realCommentNum = HLS_COMMENT_NUM;
	uint32_t lastHash = 0;
	string text8;
	wstring text16;
	wstring tmpText16;

	while (!IsCanceled())
	{
		realCommentNum = HLS_COMMENT_NUM;
		comment = NULL;

		commCount = invParseHlsComments(workObj->videoId.c_str(), &comment);
		if (commCount <= 0)
		{
			thread::Sleep(33);
			invCleanupHlsComments(comment);
			if (IsCanceled())
			{
				break;
			}
			continue;
		}

		if (commCount < HLS_COMMENT_NUM)
		{
			realCommentNum = commCount;
		}

		InvItemComment *startComment = &comment[commCount - realCommentNum];

		if (utils::GetHash(startComment[realCommentNum - 1].content) == lastHash)
		{
			thread::Sleep(33);
			invCleanupHlsComments(comment);
			if (IsCanceled())
			{
				break;
			}
			continue;
		}

		for (int i = 0; i < realCommentNum; i++)
		{
			text8 = startComment[i].author;
			common::Utf8ToUtf16(text8, &text16);
			text8 = startComment[i].content;
			common::Utf8ToUtf16(text8, &tmpText16);
			text16 += L"\n";
			text16 += tmpText16;

			button = list->FindChild(HLS_COMMENT_MAGIC + i);
			thread::RMutex::main_thread_mutex.Lock();
			button->SetString(text16);
			thread::RMutex::main_thread_mutex.Unlock();
		}

		lastHash = utils::GetHash(text8.c_str());

		invCleanupHlsComments(comment);

		thread::Sleep(33);
	}

	Cancel();
}

void menu::PlayerYoutube::CommentParseJob::Run()
{
	InvItemComment *comments = NULL;
	InvCommentSort sort;
	string text8;
	wstring text16;
	int32_t ret = 0;
	uint32_t addOverhead = 0;
	uint32_t oldCommentCount = 0;

	sce::AppSettings *settings = menu::Settings::GetAppSetInstance();
	settings->GetInt("yt_comment_sort", (int32_t *)&sort, 0);

	if (workObj->commentCont.empty())
	{
		ret = invParseComments(workObj->videoId.c_str(), NULL, sort, &comments);
	}
	else
	{
		ret = invParseComments(workObj->videoId.c_str(), workObj->commentCont.c_str(), sort, &comments);
	}

	if (ret <= 0)
	{
		workObj->commentCont = "end";
		return;
	}

	oldCommentCount = workObj->commentItems.size();

	for (int i = 0; i < ret; i++)
	{
		CommentItem item;
		text8 = comments[i].author;
		common::Utf8ToUtf16(text8, &item.author);
		text8 = comments[i].content;
		common::Utf8ToUtf16(text8, &item.content);
		text8 = comments[i].published;
		common::Utf8ToUtf16(text8, &item.date);
		item.likeCount = comments[i].likeCount;
		item.likedByOwner = comments[i].likedByOwner;
		if (comments[i].replyContinuation)
		{
			item.replyContinuation = comments[i].replyContinuation;
		}
		workObj->commentItems.push_back(item);
	}

	if (comments[ret - 1].continuation)
	{
		workObj->commentCont = comments[ret - 1].continuation;
		addOverhead = 1;
	}
	else
	{
		workObj->commentCont = "end";
	}

	ui::ListView *list = (ui::ListView *)workObj->companelRoot;
	thread::RMutex::main_thread_mutex.Lock();
	list->InsertCell(0, oldCommentCount, workObj->commentItems.size() + addOverhead - oldCommentCount);
	thread::RMutex::main_thread_mutex.Unlock();

	invCleanupComments(comments);
}

void menu::PlayerYoutube::CommentBodyButtonCbFun(int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
{
	Plugin::TemplateOpenParam tmpParam;
	wstring text16;
	ui::Widget *wdg = (ui::Widget*)self;

	PlayerYoutube *workObj = (PlayerYoutube *)userdata;

	if (wdg->GetName().GetIDHash() == workObj->commentItems.size())
	{
		ui::ListView *list = (ui::ListView *)workObj->companelRoot;
		list->DeleteCell(0, workObj->commentItems.size(), 1);

		CommentParseJob *job = new CommentParseJob("YouTube::CommentParseJob");
		job->workObj = workObj;
		common::SharedPtr<job::JobItem> itemParam(job);
		utils::GetJobQueue()->Enqueue(itemParam);
	}
	else
	{
		CommentItem entry = workObj->commentItems.at(wdg->GetName().GetIDHash());

		ui::ScrollView *commentView = dialog::OpenScrollView(g_appPlugin, g_appPlugin->GetString(msg_youtube_comment_detail));

		g_appPlugin->TemplateOpen(commentView->FindChild("dialog_view_box"), template_scroll_item_youtube_comment_detail, tmpParam);

		ui::Widget *detailText = commentView->FindChild(text_youtube_comment_detail);
		text16 = entry.author + L"\n" + entry.date + L"\n";
		wstring likeText16;
		string likeText8 = common::FormatString("%d", entry.likeCount);
		common::Utf8ToUtf16(likeText8, &likeText16);
		text16 += likeText16 + g_appPlugin->GetString(msg_youtube_comment_like_count);
		detailText->SetString(text16);

		ui::Widget *bodyText = commentView->FindChild(text_youtube_comment_detail_body);
		text16 = entry.content;
		bodyText->SetString(text16);
	}
}

void menu::PlayerYoutube::ExpandButtonCbFun(int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
{
	PlayerYoutube *workObj = (PlayerYoutube *)userdata;
	workObj->player->SetScale(1.0f);
	workObj->player->SetPosition(0.0f, 0.0f);
}

void menu::PlayerYoutube::FavButtonCbFun(int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
{
	uint32_t texid = 0;
	PlayerYoutube *workObj = (PlayerYoutube *)userdata;

	if (workObj->isFav)
	{
		texid = tex_yt_icon_favourite_for_player;
		ytutils::GetFavLog()->Remove(workObj->videoId.c_str());
		workObj->isFav = false;
	}
	else
	{
		texid = tex_yt_icon_favourite_for_player_glow;
		ytutils::GetFavLog()->AddAsync(workObj->videoId.c_str());
		workObj->isFav = true;
	}

	intrusive_ptr<graph::Surface> favIcon = g_appPlugin->GetTexture(texid);
	if (favIcon.get())
	{
		workObj->favButton->SetTexture(favIcon);
	}
}

void menu::PlayerYoutube::CommentButtonCbFun(int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
{
	Plugin::TemplateOpenParam tmpParam;
	PlayerYoutube *workObj = (PlayerYoutube *)userdata;

	if (workObj->companelRoot)
	{
		if (workObj->companelRoot->GetName().GetID() == "comment_root")
		{
			return;
		}
		common::transition::DoReverse(0.0f, workObj->companelRoot, common::transition::Type_Fadein1, true, false);
		workObj->companelRoot = NULL;
	}

	g_appPlugin->TemplateOpen(workObj->companelBase, template_list_view_youtube_companel, tmpParam);
	workObj->companelRoot = workObj->companelBase->GetChild(workObj->companelBase->GetChildrenNum() - 1);
	workObj->companelRoot->Show(common::transition::Type_Fadein1);
	workObj->companelRoot->SetName("comment_root");

	ui::ListView *list = (ui::ListView *)workObj->companelRoot;

	list->InsertSegment(0, 1);
	math::v4 sz(414.0f, 70.0f);
	list->SetCellSizeDefault(0, sz);
	list->SetSegmentLayoutType(0, ui::ListView::LAYOUT_TYPE_LIST);

	workObj->commentItems.clear();

	if (workObj->isHls)
	{
		HlsCommentListViewCb *lwCb = new HlsCommentListViewCb();
		list->SetItemFactory(lwCb);
		list->InsertCell(0, 0, HLS_COMMENT_NUM);

		workObj->hlsCommentThread = new HlsCommentParseThread(SCE_KERNEL_DEFAULT_PRIORITY_USER + 20, SCE_KERNEL_64KiB, "YouTube::CommentParseThread");
		workObj->hlsCommentThread->workObj = workObj;
		workObj->hlsCommentThread->Start();
	}
	else
	{
		CommentListViewCb *lwCb = new CommentListViewCb();
		lwCb->workObj = workObj;
		list->SetItemFactory(lwCb);

		CommentParseJob *job = new CommentParseJob("YouTube::CommentParseJob");
		job->workObj = workObj;
		common::SharedPtr<job::JobItem> itemParam(job);
		utils::GetJobQueue()->Enqueue(itemParam);
	}
}

void menu::PlayerYoutube::DescButtonCbFun(int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
{
	Plugin::TemplateOpenParam tmpParam;
	wstring text16;
	PlayerYoutube *workObj = (PlayerYoutube *)userdata;

	if (workObj->description.length() == 0)
	{
		return;
	}

	if (workObj->companelRoot)
	{
		if (workObj->companelRoot->GetName().GetID() == "description_root")
		{
			return;
		}
		if (workObj->hlsCommentThread)
		{
			workObj->hlsCommentThread->Cancel();
			workObj->hlsCommentThread->Join();
			delete workObj->hlsCommentThread;
			workObj->hlsCommentThread = NULL;
		}
		common::transition::DoReverse(0.0f, workObj->companelRoot, common::transition::Type_Fadein1, true, false);
		workObj->companelRoot = NULL;
	}

	g_appPlugin->TemplateOpen(workObj->companelBase, template_scroll_view_youtube_companel, tmpParam);
	workObj->companelRoot = workObj->companelBase->GetChild(workObj->companelBase->GetChildrenNum() - 1);
	workObj->companelRoot->Show(common::transition::Type_Fadein1);
	workObj->companelRoot->SetName("description_root");

	ui::Widget *descText = workObj->companelRoot->FindChild(text_youtube_companel);

	common::Utf8ToUtf16(workObj->description, &text16);
	text16 += L"\n\n";
	descText->SetString(text16);
}

void menu::PlayerYoutube::BackButtonCbFun(int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
{
	PlayerYoutube *workObj = (PlayerYoutube *)userdata;
	delete workObj->player;
	delete workObj;
}

void menu::PlayerYoutube::DwAddCompleteCb(int32_t result)
{
	dialog::Close();
	if (result == SCE_OK)
	{
		dialog::OpenOk(g_appPlugin, NULL, g_appPlugin->GetString(msg_settings_youtube_download_begin));
	}
	else
	{
		dialog::OpenError(g_appPlugin, result);
	}
}

void menu::PlayerYoutube::SettingsButtonCbFun(int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
{
	PlayerYoutube *workObj = (PlayerYoutube *)userdata;

	vector<OptionMenu::Button> buttons;
	OptionMenu::Button bt;
	bt.label = g_appPlugin->GetString(msg_settings);
	buttons.push_back(bt);
	if (!workObj->isHls)
	{
		bt.label = g_appPlugin->GetString(msg_settings_youtube_download);
		buttons.push_back(bt);
	}

	new OptionMenu(g_appPlugin, workObj->root, &buttons);
}

void menu::PlayerYoutube::OptionMenuEventCbFun(int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
{
	menu::PlayerYoutube *player = (menu::PlayerYoutube *)userdata;

	if (e->GetValue(0) == OptionMenu::OptionMenuEvent_Close)
	{
		return;
	}

	switch (e->GetValue(1))
	{
	case 0:
		menu::GetMenuAt(menu::GetMenuCount() - 2)->DisableInput();
		menu::SettingsButtonCbFun(ui::Button::CB_BTN_DECIDE, NULL, 0, NULL);
		break;
	case 1:
		wstring title16;
		string title8;
		player->title->GetString(title16);
		common::Utf16ToUtf8(title16, &title8);
		title8 += ".mp4";

		int32_t res = ytutils::EnqueueDownloadAsync(player->videoLink.c_str(), title8.c_str(), DwAddCompleteCb);
		if (res == SCE_OK)
		{
			dialog::OpenPleaseWait(g_appPlugin, NULL, Framework::Instance()->GetCommonString("msg_wait"));
		}
		else
		{
			dialog::OpenError(g_appPlugin, res);
		}
		break;
	}
}

void menu::PlayerYoutube::PlayerEventCbFun(int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
{
	menu::PlayerYoutube *workObj = (menu::PlayerYoutube *)userdata;

	switch (e->GetValue(0))
	{
	case PlayerSimple::PlayerEvent_Back:
		if (workObj->player->GetScale() != 1.0f)
		{
			delete workObj->player;
		}
		else
		{
			workObj->player->SetScale(0.5f);
			if (SCE_PAF_IS_DOLCE)
			{
				workObj->player->SetPosition(26.0f, -158.0f);
			}
			else
			{
				workObj->player->SetPosition(13.0f, -79.0f);
			}
		}
		break;
	case PlayerSimple::PlayerEvent_InitOk:
		int32_t min = 0;

		ui::BusyIndicator *loaderIndicator = (ui::BusyIndicator *)workObj->root->FindChild(busyindicator_youtube_loader);
		ui::Widget *loaderPlane = workObj->root->FindChild(plane_youtube_loader);

		loaderIndicator->Stop();
		common::transition::DoReverse(0.0f, loaderPlane, common::transition::Type_Fadein1, true, false);
		workObj->EnableInput();

		menu::Settings::GetAppSetInstance()->GetInt("yt_min", &min, 0);
		if (min)
		{
			workObj->player->SetScale(0.5f);
			if (SCE_PAF_IS_DOLCE)
			{
				workObj->player->SetPosition(26.0f, -158.0f);
			}
			else
			{
				workObj->player->SetPosition(13.0f, -79.0f);
			}
		}
		else
		{
			workObj->player->SetPosition(0.0f, 0.0f);
			workObj->root->Hide();
		}
		break;
	case PlayerSimple::PlayerEvent_InitFail:
		delete workObj->player;
		workObj->player = NULL;

		if (workObj->lastAttempt)
		{
			workObj->lastAttempt = false;

			ui::BusyIndicator *loaderIndicator = (ui::BusyIndicator *)workObj->root->FindChild(busyindicator_youtube_loader);
			ui::Widget *loaderPlane = workObj->root->FindChild(plane_youtube_loader);

			loaderIndicator->Stop();
			common::transition::DoReverse(0.0f, loaderPlane, common::transition::Type_Fadein1, true, false);
			workObj->EnableInput();

			dialog::OpenError(g_appPlugin, SCE_ERROR_ERRNO_EUNSUP, Framework::Instance()->GetCommonString("msg_error_connect_server_peer"));
		}
		else
		{
			workObj->lastAttempt = true;
			LoadJob *job = new LoadJob("YouTube::LoadJob");
			job->workObj = workObj;
			common::SharedPtr<job::JobItem> itemParam(job);
			utils::GetJobQueue()->Enqueue(itemParam);
		}
		break;
	}
}

void menu::PlayerYoutube::SettingsEventCbFun(int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
{
	if (e->GetValue(0) == Settings::SettingsEvent_Close)
	{
		menu::GetMenuAt(menu::GetMenuCount() - 2)->EnableInput();
	}
}

void menu::PlayerYoutube::TaskLoadSecondStage(void *pArgBlock)
{
	PlayerYoutube *workObj = (PlayerYoutube *)pArgBlock;

	workObj->player = new menu::PlayerSimple(workObj->videoLink.c_str());
	workObj->player->SetPosition(-1920.0f, -1080.0f);
	workObj->root->Show();
	workObj->DisableInput();
	if (workObj->isHighHls)
	{
		workObj->player->player->LimitFPS(true);
	}
	workObj->player->SetSettingsOverride(menu::PlayerSimple::SettingsOverride_YouTube);

	common::MainThreadCallList::Unregister(TaskLoadSecondStage, pArgBlock);
}

menu::PlayerYoutube::PlayerYoutube(const char *id, bool isFavourite) :
	GenericMenu("page_youtube_player",
	MenuOpenParam(false, 200.0f, Plugin::TransitionType_SlideFromBottom),
	MenuCloseParam(false, 200.0f, Plugin::TransitionType_SlideFromBottom))
{
	player = NULL;
	videoId = id;
	isHls = false;
	isHighHls = false;
	isFav = isFavourite;
	companelRoot = NULL;
	hlsCommentThread = NULL;
	lastAttempt = false;

	menu::GetMenuAt(0)->GetRoot()->SetEventCallback(OptionMenu::OptionMenuEvent, OptionMenuEventCbFun, this);
	root->AddEventCallback(Settings::SettingsEvent, SettingsEventCbFun, this);
	root->AddEventCallback(PlayerSimple::PlayerSimpleEvent, PlayerEventCbFun, this);

	ui::BusyIndicator *loaderIndicator = (ui::BusyIndicator *)root->FindChild(busyindicator_youtube_loader);
	loaderIndicator->Start();

	ui::Widget *settingsButton = root->FindChild(button_settings_page_youtube_player);
	settingsButton->Show(common::transition::Type_Reset);
	settingsButton->AddEventCallback(ui::Button::CB_BTN_DECIDE, SettingsButtonCbFun, this);

	ui::Widget *backButton = root->FindChild(button_back_page_youtube_player);
	backButton->Show(common::transition::Type_Reset);
	backButton->AddEventCallback(ui::Button::CB_BTN_DECIDE, BackButtonCbFun, this);

	title = (ui::Text *)root->FindChild(text_video_title);
	stat0 = (ui::Text *)root->FindChild(text_video_stat_0);
	stat1 = (ui::Text *)root->FindChild(text_video_stat_1);
	stat2 = (ui::Text *)root->FindChild(text_video_stat_2);
	expandButton = root->FindChild(button_youtube_expand);
	expandButton->AddEventCallback(ui::Button::CB_BTN_DECIDE, ExpandButtonCbFun, this);
	favButton = root->FindChild(button_youtube_fav);
	favButton->AddEventCallback(ui::Button::CB_BTN_DECIDE,FavButtonCbFun);
	if (isFav)
	{
		intrusive_ptr<graph::Surface> favIcon = g_appPlugin->GetTexture(tex_yt_icon_favourite_for_player_glow);
		favButton->SetTexture(favIcon);
	}
	companelBase = root->FindChild(plane_youtube_companel_base);
	commentButton = root->FindChild(button_yt_companel_comment);
	commentButton->AddEventCallback(ui::Button::CB_BTN_DECIDE, CommentButtonCbFun, this);
	descButton = root->FindChild(button_yt_companel_description);
	descButton->AddEventCallback(ui::Button::CB_BTN_DECIDE, DescButtonCbFun, this);

	DisableInput();

	LoadJob *job = new LoadJob("YouTube::LoadJob");
	job->workObj = this;
	common::SharedPtr<job::JobItem> itemParam(job);
	utils::GetJobQueue()->Enqueue(itemParam);
}

menu::PlayerYoutube::~PlayerYoutube()
{
	if (hlsCommentThread)
	{
		hlsCommentThread->Cancel();
		hlsCommentThread->Join();
		delete hlsCommentThread;
	}

	if (companelRoot)
	{
		companelRoot->SetName((uint32_t)0);
	}
}