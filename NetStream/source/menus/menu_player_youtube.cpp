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
#include "curl_file.h"
#include "option_menu.h"
#include "menus/menu_generic.h"
#include "menus/menu_player_youtube.h"
#include "menus/menu_player_simple.h"
#include "menus/menu_settings.h"

using namespace paf;

SceVoid menu::PlayerYoutube::LoadJob::Run()
{
	InvItemVideo *invItem = SCE_NULL;
	SceInt32 ret = 0;
	string text8;
	wstring text16;

	ret = invParseVideo(workObj->videoId.c_str(), &invItem);
	if (ret == SCE_TRUE)
	{
		if (invItem->id)
		{
			if (!workObj->lastAttempt)
			{
				workObj->description = invItem->description;

				thread::s_mainThreadMutex.Lock();

				DescButtonCbFun(0, workObj->descButton, 0, workObj);

				text8 = invItem->title;
				ccc::UTF8toUTF16(&text8, &text16);
				workObj->title->SetLabel(&text16);

				text8 = "Uploaded by ";
				text8 += invItem->author;
				ccc::UTF8toUTF16(&text8, &text16);
				workObj->stat0->SetLabel(&text16);

				text8 = invItem->subCount;
				text8 += " subscribers";
				ccc::UTF8toUTF16(&text8, &text16);
				workObj->stat1->SetLabel(&text16);

				text8 = invItem->published;
				ccc::UTF8toUTF16(&text8, &text16);
				workObj->stat2->SetLabel(&text16);

				thread::s_mainThreadMutex.Unlock();
			}

			SceInt32 quality = 0;
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
				settings->GetInt("yt_hls_quality", (SceInt32 *)&quality, 0);
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
				workObj->isHls = SCE_TRUE;
				if (quality == 4)
				{
					workObj->isHighHls = SCE_TRUE;
				}
				delete hls;

				rco::Element searchParam;
				graph::Surface *liveTex;
				ui::Widget *livePlane = utils::GetChild(workObj->root, plane_youtube_live_now);
				searchParam.hash = tex_yt_icon_live_now;
				Plugin::GetTexture(&liveTex, g_appPlugin, &searchParam);
				thread::s_mainThreadMutex.Lock();
				livePlane->SetSurfaceBase(&liveTex);
				thread::s_mainThreadMutex.Unlock();
			}
			else
			{
				settings->GetInt("yt_quality", (SceInt32 *)&quality, 0);
				if (workObj->lastAttempt)
				{
					sceClibPrintf("using last resort...\n");
					char lastResort[256];
					lastResort[0] = 0;
					ret = invGetProxyUrl(workObj->videoId.c_str(), (InvVideoQuality)quality, lastResort, sizeof(lastResort));
					sceClibPrintf("last resort url: %s\n", lastResort);
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

	task::Register(menu::PlayerYoutube::TaskLoadSecondStage, workObj);
}

ui::ListItem *menu::PlayerYoutube::HlsCommentListViewCb::Create(Param *info)
{
	rco::Element searchParam;
	Plugin::TemplateOpenParam tmpParam;
	ui::Widget *item = SCE_NULL;
	wstring text16;

	ui::Widget *targetRoot = info->parent;

	searchParam.hash = template_list_item_youtube_comment;
	g_appPlugin->TemplateOpen(targetRoot, &searchParam, &tmpParam);
	item = targetRoot->GetChild(targetRoot->childNum - 1);

	ui::Widget *button = utils::GetChild(item, button_yt_companel_comment_item);
	button->elem.hash = HLS_COMMENT_MAGIC + info->cellIndex;

	return (ui::ListItem *)item;
}

ui::ListItem *menu::PlayerYoutube::CommentListViewCb::Create(Param *info)
{
	rco::Element searchParam;
	Plugin::TemplateOpenParam tmpParam;
	ui::Widget *item = SCE_NULL;
	wstring text16;

	ui::Widget *targetRoot = info->parent;

	searchParam.hash = template_list_item_youtube_comment;
	g_appPlugin->TemplateOpen(targetRoot, &searchParam, &tmpParam);
	item = targetRoot->GetChild(targetRoot->childNum - 1);

	ui::Widget *button = utils::GetChild(item, button_yt_companel_comment_item);
	button->elem.hash = info->cellIndex;

	button->RegisterEventCallback(ui::EventMain_Decide, new utils::SimpleEventCallback(CommentBodyButtonCbFun, workObj));

	if (info->cellIndex == workObj->commentItems.size())
	{
		text16 = utils::GetString(msg_youtube_comment_more);
	}
	else
	{
		CommentItem entry = workObj->commentItems.at(info->cellIndex);
		text16 = entry.author + L"  " + entry.date + L"\n" + entry.content;

		if (!entry.replyContinuation.empty())
		{
			//TODO: implement comment replies
		}
	}

	button->SetLabel(&text16);

	return (ui::ListItem *)item;
}

SceVoid menu::PlayerYoutube::HlsCommentParseThread::EntryFunction()
{
	rco::Element searchParam;
	ui::ListView *list = (ui::ListView *)workObj->companelRoot;
	ui::Widget *button = SCE_NULL;
	InvItemComment *comment;
	SceInt32 commCount = 0;
	SceUInt32 realCommentNum = HLS_COMMENT_NUM;
	SceUInt32 lastHash = 0;
	string text8;
	wstring text16;
	wstring tmpText16;

	while (!IsCanceled())
	{
		realCommentNum = HLS_COMMENT_NUM;
		comment = SCE_NULL;

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
			ccc::UTF8toUTF16(&text8, &text16);
			text8 = startComment[i].content;
			ccc::UTF8toUTF16(&text8, &tmpText16);
			text16 += L"\n";
			text16 += tmpText16;

			searchParam.hash = HLS_COMMENT_MAGIC + i;
			button = list->GetChild(&searchParam, 0);
			thread::s_mainThreadMutex.Lock();
			button->SetLabel(&text16);
			thread::s_mainThreadMutex.Unlock();
		}

		lastHash = utils::GetHash(text8.c_str());

		invCleanupHlsComments(comment);

		thread::Sleep(33);
	}

	Cancel();
}

SceVoid menu::PlayerYoutube::CommentParseJob::Run()
{
	InvItemComment *comments = SCE_NULL;
	InvCommentSort sort;
	string text8;
	wstring text16;
	SceInt32 ret = 0;
	SceUInt32 addOverhead = 0;
	SceUInt32 oldCommentCount = 0;

	sce::AppSettings *settings = menu::Settings::GetAppSetInstance();
	settings->GetInt("yt_comment_sort", (SceInt32 *)&sort, 0);

	if (workObj->commentCont.empty())
	{
		ret = invParseComments(workObj->videoId.c_str(), SCE_NULL, sort, &comments);
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
		ccc::UTF8toUTF16(&text8, &item.author);
		text8 = comments[i].content;
		ccc::UTF8toUTF16(&text8, &item.content);
		text8 = comments[i].published;
		ccc::UTF8toUTF16(&text8, &item.date);
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
	thread::s_mainThreadMutex.Lock();
	list->AddItem(0, oldCommentCount, workObj->commentItems.size() + addOverhead - oldCommentCount);
	thread::s_mainThreadMutex.Unlock();

	invCleanupComments(comments);
}

SceVoid menu::PlayerYoutube::CommentBodyButtonCbFun(SceInt32 eventId, ui::Widget *self, SceInt32 a3, ScePVoid pUserData)
{
	rco::Element searchParam;
	Plugin::TemplateOpenParam tmpParam;
	wstring text16;

	PlayerYoutube *workObj = (PlayerYoutube *)pUserData;

	if (self->elem.hash == workObj->commentItems.size())
	{
		ui::ListView *list = (ui::ListView *)workObj->companelRoot;
		list->RemoveItem(0, workObj->commentItems.size(), 1);

		CommentParseJob *job = new CommentParseJob("YouTube::CommentParseJob");
		job->workObj = workObj;
		SharedPtr<job::JobItem> itemParam(job);
		utils::GetJobQueue()->Enqueue(&itemParam);
	}
	else
	{
		CommentItem entry = workObj->commentItems.at(self->elem.hash);

		ui::ScrollView *commentView = dialog::OpenScrollView(g_appPlugin, utils::GetString(msg_youtube_comment_detail));

		searchParam.hash = template_scroll_item_youtube_comment_detail;
		g_appPlugin->TemplateOpen(utils::GetChild(commentView, "dialog_view_box"), &searchParam, &tmpParam);

		ui::Widget *detailText = utils::GetChild(commentView, text_youtube_comment_detail);
		text16 = entry.author + L"\n" + entry.date + L"\n";
		wstring likeText16;
		string likeText8 = ccc::Sprintf("%d", entry.likeCount);
		ccc::UTF8toUTF16(&likeText8, &likeText16);
		text16 += likeText16 + utils::GetString(msg_youtube_comment_like_count);
		detailText->SetLabel(&text16);

		ui::Widget *bodyText = utils::GetChild(commentView, text_youtube_comment_detail_body);
		text16 = entry.content;
		bodyText->SetLabel(&text16);
	}
}

SceVoid menu::PlayerYoutube::ExpandButtonCbFun(SceInt32 eventId, ui::Widget *self, SceInt32 a3, ScePVoid pUserData)
{
	PlayerYoutube *workObj = (PlayerYoutube *)pUserData;
	workObj->player->SetScale(1.0f);
	workObj->player->SetPosition(0.0f, 0.0f);
}

SceVoid menu::PlayerYoutube::FavButtonCbFun(SceInt32 eventId, ui::Widget *self, SceInt32 a3, ScePVoid pUserData)
{
	graph::Surface *favIcon = SCE_NULL;
	PlayerYoutube *workObj = (PlayerYoutube *)pUserData;

	if (workObj->isFav)
	{
		favIcon = utils::GetTexture(tex_yt_icon_favourite_for_player);
		workObj->favButton->SetSurfaceBase(&favIcon);
		ytutils::GetFavLog()->Remove(workObj->videoId.c_str());
		workObj->isFav = SCE_FALSE;
	}
	else
	{
		favIcon = utils::GetTexture(tex_yt_icon_favourite_for_player_glow);
		workObj->favButton->SetSurfaceBase(&favIcon);
		ytutils::GetFavLog()->AddAsync(workObj->videoId.c_str());
		workObj->isFav = SCE_TRUE;
	}
}

SceVoid menu::PlayerYoutube::CommentButtonCbFun(SceInt32 eventId, ui::Widget *self, SceInt32 a3, ScePVoid pUserData)
{
	rco::Element searchParam;
	Plugin::TemplateOpenParam tmpParam;
	PlayerYoutube *workObj = (PlayerYoutube *)pUserData;

	if (workObj->companelRoot)
	{
		if (workObj->companelRoot->elem.id == "comment_root")
		{
			return;
		}
		effect::Play(0.0f, workObj->companelRoot, effect::EffectType_Fadein1, true, false);
		workObj->companelRoot = SCE_NULL;
	}

	searchParam.hash = template_list_view_youtube_companel;
	g_appPlugin->TemplateOpen(workObj->companelBase, &searchParam, &tmpParam);
	workObj->companelRoot = workObj->companelBase->GetChild(workObj->companelBase->childNum - 1);
	workObj->companelRoot->PlayEffect(0.0f, effect::EffectType_Fadein1);
	workObj->companelRoot->elem.id = "comment_root";

	ui::ListView *list = (ui::ListView *)workObj->companelRoot;
	list->SetSegmentEnable(0, 1);
	Vector4 sz(414.0f, 70.0f);
	list->SetCellSize(0, &sz);
	list->SetConfigurationType(0, ui::ListView::ConfigurationType_Simple);

	workObj->commentItems.clear();

	if (workObj->isHls)
	{
		HlsCommentListViewCb *lwCb = new HlsCommentListViewCb();
		list->RegisterItemCallback(lwCb);
		list->AddItem(0, 0, HLS_COMMENT_NUM);

		workObj->hlsCommentThread = new HlsCommentParseThread(SCE_KERNEL_DEFAULT_PRIORITY_USER + 20, SCE_KERNEL_64KiB, "YouTube::CommentParseThread");
		workObj->hlsCommentThread->workObj = workObj;
		workObj->hlsCommentThread->Start();
	}
	else
	{
		CommentListViewCb *lwCb = new CommentListViewCb();
		lwCb->workObj = workObj;
		list->RegisterItemCallback(lwCb);

		CommentParseJob *job = new CommentParseJob("YouTube::CommentParseJob");
		job->workObj = workObj;
		SharedPtr<job::JobItem> itemParam(job);
		utils::GetJobQueue()->Enqueue(&itemParam);
	}
}

SceVoid menu::PlayerYoutube::DescButtonCbFun(SceInt32 eventId, ui::Widget *self, SceInt32 a3, ScePVoid pUserData)
{
	rco::Element searchParam;
	Plugin::TemplateOpenParam tmpParam;
	wstring text16;
	PlayerYoutube *workObj = (PlayerYoutube *)pUserData;

	if (workObj->description.length() == 0)
	{
		return;
	}

	if (workObj->companelRoot)
	{
		if (workObj->companelRoot->elem.id == "description_root")
		{
			return;
		}
		if (workObj->hlsCommentThread)
		{
			workObj->hlsCommentThread->Cancel();
			workObj->hlsCommentThread->Join();
			delete workObj->hlsCommentThread;
			workObj->hlsCommentThread = SCE_NULL;
		}
		effect::Play(0.0f, workObj->companelRoot, effect::EffectType_Fadein1, true, false);
		workObj->companelRoot = SCE_NULL;
	}

	searchParam.hash = template_scroll_view_youtube_companel;
	g_appPlugin->TemplateOpen(workObj->companelBase, &searchParam, &tmpParam);
	workObj->companelRoot = workObj->companelBase->GetChild(workObj->companelBase->childNum - 1);
	workObj->companelRoot->PlayEffect(0.0f, effect::EffectType_Fadein1);
	workObj->companelRoot->elem.id = "description_root";

	searchParam.hash = text_youtube_companel;
	ui::Widget *descText = workObj->companelRoot->GetChild(&searchParam, 0);

	ccc::UTF8toUTF16(&workObj->description, &text16);
	text16 += L"\n\n";
	descText->SetLabel(&text16);
}

SceVoid menu::PlayerYoutube::BackButtonCbFun(SceInt32 eventId, ui::Widget *self, SceInt32 a3, ScePVoid pUserData)
{
	PlayerYoutube *workObj = (PlayerYoutube *)pUserData;
	delete workObj->player;
	delete workObj;
}

SceVoid menu::PlayerYoutube::DwAddCompleteCb(SceInt32 result)
{
	dialog::Close();
	if (result == SCE_OK)
	{
		dialog::OpenOk(g_appPlugin, SCE_NULL, utils::GetString(msg_settings_youtube_download_begin));
	}
	else
	{
		dialog::OpenError(g_appPlugin, result);
	}
}

SceVoid menu::PlayerYoutube::SettingsButtonCbFun(SceInt32 eventId, ui::Widget *self, SceInt32 a3, ScePVoid pUserData)
{
	PlayerYoutube *workObj = (PlayerYoutube *)pUserData;

	vector<OptionMenu::Button> buttons;
	OptionMenu::Button bt;
	bt.label = utils::GetString(msg_settings);
	buttons.push_back(bt);
	bt.label = utils::GetString(msg_settings_youtube_download);
	buttons.push_back(bt);

	new OptionMenu(g_appPlugin, workObj->root, &buttons, OptionButtonCb, pUserData);
}

SceVoid menu::PlayerYoutube::OptionButtonCb(SceUInt32 index, ScePVoid pUserData)
{
	menu::PlayerYoutube *player = (menu::PlayerYoutube *)pUserData;

	switch (index)
	{
	case 0:
		menu::GenericMenu *baseMenu = menu::GetMenuAt(menu::GetMenuCount() - 2);
		ui::Widget::SetControlFlags(baseMenu->root, 0);
		SceUInt32 clCbArg[2];
		clCbArg[0] = (SceUInt32)SettingsCloseCbFun;
		clCbArg[1] = (SceUInt32)pUserData;
		menu::SettingsButtonCbFun(ui::EventMain_Decide, SCE_NULL, 0, clCbArg);
		break;
	case 1:
		wstring title16;
		string title8;
		player->title->GetLabel(&title16);
		ccc::UTF16toUTF8(&title16, &title8);
		title8 += ".mp4";

		SceInt32 res = ytutils::EnqueueDownloadAsync(player->videoLink.c_str(), title8.c_str(), DwAddCompleteCb);
		if (res == SCE_OK)
		{
			dialog::OpenPleaseWait(g_appPlugin, SCE_NULL, utils::GetString("msg_wait"));
		}
		else
		{
			dialog::OpenError(g_appPlugin, res);
		}
		break;
	}
}

SceVoid menu::PlayerYoutube::SettingsCloseCbFun(ScePVoid pUserData)
{
	menu::GenericMenu *baseMenu = menu::GetMenuAt(menu::GetMenuCount() - 2);
	ui::Widget::SetControlFlags(baseMenu->root, 1);
}

SceVoid menu::PlayerYoutube::PlayerBackCb(PlayerSimple *player, ScePVoid pUserArg)
{
	if (player->GetScale() != 1.0f)
	{
		delete player;
	}
	else
	{
		player->SetScale(0.5f);
		if (SCE_PAF_IS_DOLCE)
		{
			player->SetPosition(26.0f, -158.0f);
		}
		else
		{
			player->SetPosition(13.0f, -79.0f);
		}
	}
}

SceVoid menu::PlayerYoutube::PlayerOkCb(PlayerSimple *player, ScePVoid pUserArg)
{
	PlayerYoutube *workObj = (PlayerYoutube *)pUserArg;
	SceInt32 min = 0;

	ui::BusyIndicator *loaderIndicator = (ui::BusyIndicator *)utils::GetChild(workObj->root, busyindicator_youtube_loader);
	ui::Widget *loaderPlane = utils::GetChild(workObj->root, plane_youtube_loader);

	thread::s_mainThreadMutex.Lock();
	loaderIndicator->Stop();
	effect::Play(0.0f, loaderPlane, effect::EffectType_Fadein1, true, false);
	ui::Widget::SetControlFlags(workObj->root, 1);
	thread::s_mainThreadMutex.Unlock();

	menu::Settings::GetAppSetInstance()->GetInt("yt_min", &min, 0);
	if (min)
	{
		player->SetScale(0.5f);
		if (SCE_PAF_IS_DOLCE)
		{
			player->SetPosition(26.0f, -158.0f);
		}
		else
		{
			player->SetPosition(13.0f, -79.0f);
		}
	}
	else
	{
		workObj->player->SetPosition(0.0f, 0.0f);
		workObj->root->SetGraphicsDisabled(true);
	}
}

SceVoid menu::PlayerYoutube::PlayerFailCb(PlayerSimple *player, ScePVoid pUserArg)
{
	PlayerYoutube *workObj = (PlayerYoutube *)pUserArg;

	delete workObj->player;
	workObj->player = SCE_NULL;

	if (workObj->lastAttempt)
	{
		workObj->lastAttempt = SCE_FALSE;

		ui::BusyIndicator *loaderIndicator = (ui::BusyIndicator *)utils::GetChild(workObj->root, busyindicator_youtube_loader);
		ui::Widget *loaderPlane = utils::GetChild(workObj->root, plane_youtube_loader);

		thread::s_mainThreadMutex.Lock();
		loaderIndicator->Stop();
		effect::Play(0.0f, loaderPlane, effect::EffectType_Fadein1, true, false);
		ui::Widget::SetControlFlags(workObj->root, 1);
		thread::s_mainThreadMutex.Unlock();

		dialog::OpenError(g_appPlugin, SCE_ERROR_ERRNO_EUNSUP, utils::GetString("msg_error_connect_server_peer"));
	}
	else
	{
		workObj->lastAttempt = SCE_TRUE;
		LoadJob *job = new LoadJob("YouTube::LoadJob");
		job->workObj = workObj;
		SharedPtr<job::JobItem> itemParam(job);
		utils::GetJobQueue()->Enqueue(&itemParam);
	}
}

SceVoid menu::PlayerYoutube::TaskLoadSecondStage(void *pArgBlock)
{
	PlayerYoutube *workObj = (PlayerYoutube *)pArgBlock;

	workObj->player = new menu::PlayerSimple(workObj->videoLink.c_str(), PlayerOkCb, PlayerFailCb, PlayerBackCb, pArgBlock);
	workObj->player->SetPosition(-1920.0f, -1080.0f);
	workObj->root->SetGraphicsDisabled(false);
	if (workObj->isHighHls)
	{
		workObj->player->player->LimitFPS(SCE_TRUE);
	}
	workObj->player->SetSettingsOverride(menu::PlayerSimple::SettingsOverride_YouTube);

	task::Unregister(TaskLoadSecondStage, pArgBlock);
}

menu::PlayerYoutube::PlayerYoutube(const char *id, SceBool isFavourite) :
	GenericMenu("page_youtube_player",
	MenuOpenParam(false, 200.0f, Plugin::PageEffectType_SlideFromBottom),
	MenuCloseParam(false, 200.0f, Plugin::PageEffectType_SlideFromBottom))
{
	player = SCE_NULL;
	videoId = id;
	isHls = SCE_FALSE;
	isHighHls = SCE_FALSE;
	isFav = isFavourite;
	companelRoot = SCE_NULL;
	hlsCommentThread = SCE_NULL;
	lastAttempt = SCE_FALSE;

	ui::BusyIndicator *loaderIndicator = (ui::BusyIndicator *)utils::GetChild(root, busyindicator_youtube_loader);
	loaderIndicator->Start();

	ui::Widget *settingsButton = utils::GetChild(root, button_settings_page_youtube_player);
	settingsButton->PlayEffect(0.0f, effect::EffectType_Reset);
	settingsButton->RegisterEventCallback(ui::EventMain_Decide, new utils::SimpleEventCallback(SettingsButtonCbFun, this));

	ui::Widget *backButton = utils::GetChild(root, button_back_page_youtube_player);
	backButton->PlayEffect(0.0f, effect::EffectType_Reset);
	backButton->RegisterEventCallback(ui::EventMain_Decide, new utils::SimpleEventCallback(BackButtonCbFun, this));

	title = (ui::Text *)utils::GetChild(root, "text_video_title");
	stat0 = (ui::Text *)utils::GetChild(root, "text_video_stat_0");
	stat1 = (ui::Text *)utils::GetChild(root, "text_video_stat_1");
	stat2 = (ui::Text *)utils::GetChild(root, "text_video_stat_2");
	expandButton = utils::GetChild(root, button_youtube_expand);
	expandButton->RegisterEventCallback(ui::EventMain_Decide, new utils::SimpleEventCallback(ExpandButtonCbFun, this));
	favButton = utils::GetChild(root, button_youtube_fav);
	favButton->RegisterEventCallback(ui::EventMain_Decide, new utils::SimpleEventCallback(FavButtonCbFun, this));
	if (isFav)
	{
		graph::Surface *favIcon = utils::GetTexture(tex_yt_icon_favourite_for_player_glow);
		favButton->SetSurfaceBase(&favIcon);
	}
	companelBase = utils::GetChild(root, plane_youtube_companel_base);
	commentButton = utils::GetChild(root, button_yt_companel_comment);
	commentButton->RegisterEventCallback(ui::EventMain_Decide, new utils::SimpleEventCallback(CommentButtonCbFun, this));
	descButton = utils::GetChild(root, button_yt_companel_description);
	descButton->RegisterEventCallback(ui::EventMain_Decide, new utils::SimpleEventCallback(DescButtonCbFun, this));

	ui::Widget::SetControlFlags(root, 0);

	LoadJob *job = new LoadJob("YouTube::LoadJob");
	job->workObj = this;
	SharedPtr<job::JobItem> itemParam(job);
	utils::GetJobQueue()->Enqueue(&itemParam);
}

menu::PlayerYoutube::~PlayerYoutube()
{
	if (hlsCommentThread)
	{
		hlsCommentThread->Cancel();
		hlsCommentThread->Join();
		delete hlsCommentThread;
	}
}