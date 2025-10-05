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
#include "ftube.h"
#include <paf_file_ext.h>
#include "option_menu.h"
#include "players/player_beav.h"
#include "players/player_av.h"
#include "menus/menu_generic.h"
#include "menus/menu_player_youtube.h"
#include "menus/menu_player_simple.h"
#include "menus/menu_settings.h"

#undef SCE_DBG_LOG_COMPONENT
#define SCE_DBG_LOG_COMPONENT "[PlayerYoutube]"

using namespace paf;

void menu::PlayerYoutube::LoadSecondStageTask(void *pArgBlock)
{
	reinterpret_cast<PlayerYoutube *>(pArgBlock)->OnLoadSecondStage();
	common::MainThreadCallList::Unregister(LoadSecondStageTask, pArgBlock);
}

void menu::PlayerYoutube::Load()
{
	FTItem *ftItem = NULL;
	void *ftCtx = NULL;
	int32_t ret = 0;
	string text8;
	wstring text16;

	m_videoLink = "";
	m_videoLinkForDw = "";
	m_audioLink.clear();
	if (m_option)
	{
		delete m_option;
		m_option = NULL;
	}

	sce::AppSettings *settings = menu::Settings::GetAppSetInstance();

	int32_t vodVideoQuality;
	settings->GetInt("yt_quality_vod_video", reinterpret_cast<int32_t *>(&vodVideoQuality), 3);

	SCE_DBG_LOG_INFO("[Load] Video id: %s", m_videoId.c_str());

	ret = ftParseVideo(&ftCtx, m_videoId.c_str(), vodVideoQuality, FT_AUDIO_VOD_QUALITY_MEDIUM, &ftItem);
	if (ret == FT_OK)
	{
		if (ftItem->videoItem->id)
		{
			m_description = ftItem->videoItem->description;

			thread::RMutex::MainThreadMutex()->Lock();

			OnDescButton();

			text8 = ftItem->videoItem->title;
			common::Utf8ToUtf16(text8, &text16);
			m_title->SetString(text16);

			text8 = "Uploaded by ";
			text8 += ftItem->videoItem->author;
			common::Utf8ToUtf16(text8, &text16);
			m_stat0->SetString(text16);

			ytutils::FormatViews(text16, ftItem->videoItem->viewCount);
			m_stat1->SetString(text16);

			// TODO: do something with m_stat2?
			// text8 = "ERASE ME";
			// common::Utf8ToUtf16(text8, &text16);
			// m_stat2->SetString(text16);

			thread::RMutex::MainThreadMutex()->Unlock();

			if (ftItem->videoItem->isLive)
			{
				SCE_DBG_LOG_INFO("[Load] Video is LIVE");

				BEAVPlayer::Option *opt = new BEAVPlayer::Option();
				settings->GetInt("yt_quality_live", reinterpret_cast<int32_t *>(&opt->defaultRes), 3);
				m_option = opt;

				m_videoLink = ftItem->videoItem->hlsManifestUrl;
				m_isLive = true;

				ui::Widget *livePlane = m_root->FindChild(plane_youtube_live_now);
				intrusive_ptr<graph::Surface> liveTex = g_appPlugin->GetTexture(tex_yt_icon_live_now);
				thread::RMutex::MainThreadMutex()->Lock();
				livePlane->SetTexture(liveTex);
				thread::RMutex::MainThreadMutex()->Unlock();
			}
			else
			{
				SCE_DBG_LOG_INFO("[Load] Video is VOD");

				if (ftItem->videoItem->compositeVideoUrl)
				{
					m_videoLinkForDw = ftItem->videoItem->compositeVideoUrl;
				}

				common::SharedPtr<LocalFile> lmanifest;
				int32_t res = -1;

				lmanifest = LocalFile::Open("savedata0:lmanifest.mpd", SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0666, &res);
				if (res == SCE_OK)
				{
					lmanifest->Write(ftItem->videoItem->dashManifest, sce_paf_strlen(ftItem->videoItem->dashManifest));
					lmanifest->Close();

					m_videoLink = "savedata0:lmanifest.mpd";
					if (ftItem->videoItem->audioOnlyUrl)
					{
						m_audioLink = ftItem->videoItem->audioOnlyUrl;
					}

					SCE_DBG_LOG_INFO("[Load] DASH manifest ready at %s", m_videoLink.c_str());
				}
				else
				{
					SCE_DBG_LOG_ERROR("failed to create manifest at %s: 0x%08X", m_videoLink.c_str(), res);
					ret = res;
				}
			}
		}
		else
		{
			ret = -1;
		}

		ftCleanup(ftCtx);
	}
	else
	{
		SCE_DBG_LOG_ERROR("[Load] ftParseVideo(): 0x%08X", ret);
	}

	common::MainThreadCallList::Register(menu::PlayerYoutube::LoadSecondStageTask, this);
}

ui::ListItem *menu::PlayerYoutube::CreateLiveCommentListItem(ui::listview::ItemFactory::CreateParam& param)
{
	Plugin::TemplateOpenParam tmpParam;
	ui::Widget *item = NULL;
	wstring text16;

	ui::Widget *targetRoot = param.parent;

	g_appPlugin->TemplateOpen(targetRoot, template_list_item_youtube_comment, tmpParam);
	item = targetRoot->GetChild(targetRoot->GetChildrenNum() - 1);

	ui::Widget *button = item->FindChild(button_yt_companel_comment_item);
	button->SetName(LIVE_COMMENT_MAGIC + param.cell_index);

	return static_cast<ui::ListItem *>(item);
}

ui::ListItem *menu::PlayerYoutube::CreateCommentListItem(ui::listview::ItemFactory::CreateParam& param)
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

	button->AddEventCallback(ui::Button::CB_BTN_DECIDE,
	[](int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
	{
		reinterpret_cast<menu::PlayerYoutube *>(userdata)->OnCommentBodyButton(static_cast<ui::Widget *>(self));
	}, this);

	if (param.cell_index == m_commentItems.size())
	{
		text16 = g_appPlugin->GetString(msg_youtube_comment_more);
	}
	else
	{
		CommentItem entry = m_commentItems.at(param.cell_index);
		text16 = entry.author + L"  " + entry.date + L"\n" + entry.content;

		if (!entry.replyContinuation.empty())
		{
			//TODO: implement comment replies
		}
	}

	button->SetString(text16);

	return static_cast<ui::ListItem *>(item);
}

void menu::PlayerYoutube::ParseLiveComments(thread::Thread *thrd)
{
	ui::ListView *list = m_companelRoot;
	ui::Widget *button = NULL;
	FTItem *ftItem;
	void *ftCtx = NULL;
	int32_t commCount = 0;
#ifdef USE_LIVE_COMMENT_HASHING
	uint32_t lastHash = 0;
#endif
	string text8;
	wstring text16;
	wstring tmpText16;

	while (!thrd->IsCanceled())
	{
		if (m_player->GetScale() != 1.0f)
		{
			commCount = ftParseLiveComments(&ftCtx, m_videoId.c_str(), LIVE_COMMENT_NUM, &ftItem);
			if (commCount <= 0)
			{
				thread::Sleep(33);
				ftCleanup(ftCtx);
				if (thrd->IsCanceled())
				{
					break;
				}
				continue;
			}

#ifdef USE_LIVE_COMMENT_HASHING
			if (utils::GetHash(ftItem->commentItem->content) == lastHash)
			{
				thread::Sleep(33);
				ftCleanup(ftCtx);
				if (thrd->IsCanceled())
				{
					break;
				}
				continue;
			}
#endif

			uint32_t i = commCount - 1;
			do
			{
				text8 = ftItem->commentItem->author;
				common::Utf8ToUtf16(text8, &text16);
				text8 = ftItem->commentItem->content;
				common::Utf8ToUtf16(text8, &tmpText16);
				text16 += L"\n";
				text16 += tmpText16;

				button = list->FindChild(LIVE_COMMENT_MAGIC + i);
				thread::RMutex::MainThreadMutex()->Lock();
				button->SetString(text16);
				thread::RMutex::MainThreadMutex()->Unlock();

				ftItem = ftItem->next;

#ifdef USE_LIVE_COMMENT_HASHING
				if (i == commCount - 1)
				{
					lastHash = utils::GetHash(text8.c_str());
				}
#endif

				i--;
			} while (ftItem);

			ftCleanup(ftCtx);
		}
		else
		{
			thread::Sleep(33);
			if (thrd->IsCanceled())
			{
				break;
			}
		}

		thread::Sleep(33);
	}

	thrd->Cancel();
}

ui::ListItem *menu::PlayerYoutube::CreateSelectQualityListItem(ui::listview::ItemFactory::CreateParam& param)
{
	Plugin::TemplateOpenParam tmpParam;
	ui::Widget *item = NULL;
	wstring text16;

	ui::Widget *targetRoot = param.parent;

	g_appPlugin->TemplateOpen(targetRoot, template_list_item_youtube_quality_select, tmpParam);
	item = targetRoot->GetChild(targetRoot->GetChildrenNum() - 1);

	ui::Button *button = static_cast<ui::Button *>(item->FindChild(button_yt_quality_select_item));
	button->SetName(SELECT_QUALITY_MAGIC + param.cell_index);

	GenericPlayer::GenericRepresentationInfo repInfo;
	m_player->GetPlayer()->GetVideoRepresentationInfo(&repInfo, param.cell_index);

	button->SetString(repInfo.string);
	if (repInfo.currentlySelected || !repInfo.enabled)
	{
		button->Disable(false);
	}
	else
	{
		button->AddEventCallback(ui::Button::CB_BTN_DECIDE,
		[](int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
		{
			reinterpret_cast<menu::PlayerYoutube *>(userdata)->OnSelectQualityButton(static_cast<ui::Widget *>(self));
		}, this);
	}

	SCE_DBG_LOG_INFO("[CreateSelectQualityListItem] Created %ls, selected: %u, disabled: %u", repInfo.string.c_str(), repInfo.currentlySelected, !repInfo.enabled);

	return static_cast<ui::ListItem *>(item);
}

void menu::PlayerYoutube::ParseComments()
{
	FTItem *commentItem = NULL;
	void *ftCtx = NULL;
	const char *commentContTmp = NULL;
	FTCommentSort sort;
	string text8;
	wstring text16;
	int32_t ret = 0;
	uint32_t addOverhead = 0;
	uint32_t oldCommentCount = 0;

	sce::AppSettings *settings = menu::Settings::GetAppSetInstance();
	settings->GetInt("yt_comment_sort", reinterpret_cast<int32_t *>(&sort), 0);

	if (m_commentCont.empty())
	{
		ret = ftParseComments(&ftCtx, m_videoId.c_str(), NULL, sort, &commentItem, &commentContTmp);
	}
	else
	{
		ret = ftParseComments(&ftCtx, m_videoId.c_str(), m_commentCont.c_str(), sort, &commentItem, &commentContTmp);
	}

	if (ret <= 0)
	{
		m_commentCont = "end";
		return;
	}

	oldCommentCount = m_commentItems.size();

	for (int i = 0; i < ret; i++)
	{
		CommentItem item;
		text8 = commentItem->commentItem->author;
		common::Utf8ToUtf16(text8, &item.author);
		text8 = commentItem->commentItem->content;
		common::Utf8ToUtf16(text8, &item.content);
		text8 = commentItem->commentItem->published;
		common::Utf8ToUtf16(text8, &item.date);
		text8 = commentItem->commentItem->voteCount;
		common::Utf8ToUtf16(text8, &item.voteCount);
		item.likedByOwner = commentItem->commentItem->likedByOwner;
		if (commentItem->commentItem->replyContinuation)
		{
			item.replyContinuation = commentItem->commentItem->replyContinuation;
		}
		m_commentItems.push_back(item);
		commentItem = commentItem->next;
	}

	if (commentContTmp)
	{
		m_commentCont = commentContTmp;
		addOverhead = 1;
	}
	else
	{
		m_commentCont = "end";
	}

	ui::ListView *list = static_cast<ui::ListView *>(m_companelRoot);
	thread::RMutex::MainThreadMutex()->Lock();
	list->InsertCell(0, oldCommentCount, m_commentItems.size() + addOverhead - oldCommentCount);
	thread::RMutex::MainThreadMutex()->Unlock();

	ftCleanup(ftCtx);
}

void menu::PlayerYoutube::OnLoadSecondStage()
{
	m_player = new menu::PlayerSimple(m_videoLink.c_str(), m_option);
	m_player->SetPosition(-1920.0f, -1080.0f);
	m_root->SetShowAlpha(1.0f);
	m_player->SetSettingsOverride(menu::PlayerSimple::SettingsOverride_YouTube);
}

void menu::PlayerYoutube::OnCommentBodyButton(ui::Widget *wdg)
{
	Plugin::TemplateOpenParam tmpParam;
	wstring text16;

	if (wdg->GetName().GetIDHash() == m_commentItems.size())
	{
		ui::ListView *list = static_cast<ui::ListView *>(m_companelRoot);
		list->DeleteCell(0, m_commentItems.size(), 1);

		CommentParseJob *job = new CommentParseJob(this);
		common::SharedPtr<job::JobItem> itemParam(job);
		utils::GetJobQueue()->Enqueue(itemParam);
	}
	else
	{
		CommentItem entry = m_commentItems.at(wdg->GetName().GetIDHash());

		ui::ScrollView *commentView = dialog::OpenScrollView(g_appPlugin, g_appPlugin->GetString(msg_youtube_comment_detail));

		g_appPlugin->TemplateOpen(commentView->FindChild("dialog_view_box"), template_scroll_item_youtube_comment_detail, tmpParam);

		ui::Widget *detailText = commentView->FindChild(text_youtube_comment_detail);
		text16 = entry.author + L"\n" + entry.date + L"\n";
		text16 += entry.voteCount + g_appPlugin->GetString(msg_youtube_comment_like_count);
		detailText->SetString(text16);

		ui::Widget *bodyText = commentView->FindChild(text_youtube_comment_detail_body);
		text16 = entry.content;
		bodyText->SetString(text16);
	}
}

void menu::PlayerYoutube::OnExpandButton()
{
	m_player->SetScale(1.0f);
	m_player->SetPosition(0.0f, 0.0f);
}

void menu::PlayerYoutube::OnVideoSettingsButton()
{
	ui::ListView *root = dialog::OpenListView(g_appPlugin, g_appPlugin->GetString(msg_quality_select_dialog_title));

	root->InsertSegment(0, 1);
	math::v4 sz(580.0f, 70.0f);
	root->SetCellSizeDefault(0, sz);
	root->SetSegmentLayoutType(0, ui::ListView::LAYOUT_TYPE_LIST);

	SelectQualityDialogListViewCb *lwCb = new SelectQualityDialogListViewCb(this);
	root->SetItemFactory(lwCb);
	root->InsertCell(0, 0, m_player->GetPlayer()->GetVideoRepresentationNum());
}

void menu::PlayerYoutube::OnSelectQualityButton(ui::Widget *wdg)
{
	uint32_t idx = wdg->GetName().GetIDHash() - SELECT_QUALITY_MAGIC;
	SCE_DBG_LOG_INFO("[OnSelectQualityButton] Selected quality idx: %u", idx);
	m_player->GetPlayer()->SelectVideoRepresentation(idx);
	dialog::Close();
}

void menu::PlayerYoutube::OnFavButton()
{
	uint32_t texid = 0;

	if (m_isFav)
	{
		texid = tex_yt_icon_favourite_for_player;
		ytutils::GetFavLog()->Remove(m_videoId.c_str());
		m_isFav = false;
	}
	else
	{
		texid = tex_yt_icon_favourite_for_player_glow;
		ytutils::GetFavLog()->AddAsync(m_videoId.c_str());
		m_isFav = true;
	}

	intrusive_ptr<graph::Surface> favIcon = g_appPlugin->GetTexture(texid);
	if (favIcon.get())
	{
		m_favButton->SetTexture(favIcon);
	}
}

void menu::PlayerYoutube::OnCommentButton()
{
	Plugin::TemplateOpenParam tmpParam;

	if (m_companelRoot)
	{
		if (m_companelRoot->GetName().GetID() == "comment_root")
		{
			return;
		}
		common::transition::DoReverse(0.0f, m_companelRoot, common::transition::Type_FadeinFast, true, false);
		m_companelRoot = NULL;
	}

	g_appPlugin->TemplateOpen(m_companelBase, template_list_view_youtube_companel, tmpParam);
	m_companelRoot = static_cast<ui::ListView *>(m_companelBase->GetChild(m_companelBase->GetChildrenNum() - 1));
	m_companelRoot->Show(common::transition::Type_FadeinFast);
	m_companelRoot->SetName("comment_root");

	ui::ListView *list = (ui::ListView *)m_companelRoot;

	list->InsertSegment(0, 1);
	math::v4 sz(414.0f, 70.0f);
	list->SetCellSizeDefault(0, sz);
	list->SetSegmentLayoutType(0, ui::ListView::LAYOUT_TYPE_LIST);

	m_commentItems.clear();

	if (m_isLive)
	{
		LiveCommentListViewCb *lwCb = new LiveCommentListViewCb(this);
		list->SetItemFactory(lwCb);
		list->InsertCell(0, 0, LIVE_COMMENT_NUM);

		m_liveCommentThread = new LiveCommentParseThread(this);
		m_liveCommentThread->Start();
	}
	else
	{
		CommentListViewCb *lwCb = new CommentListViewCb(this);
		list->SetItemFactory(lwCb);

		CommentParseJob *job = new CommentParseJob(this);
		common::SharedPtr<job::JobItem> itemParam(job);
		utils::GetJobQueue()->Enqueue(itemParam);
	}
}

void menu::PlayerYoutube::OnDescButton()
{
	Plugin::TemplateOpenParam tmpParam;
	wstring text16;

	if (m_description.length() == 0)
	{
		return;
	}

	if (m_companelRoot)
	{
		if (m_companelRoot->GetName().GetID() == "description_root")
		{
			return;
		}
		if (m_liveCommentThread)
		{
			m_liveCommentThread->Cancel();
			thread::RMutex::MainThreadMutex()->Unlock();
			m_liveCommentThread->Join();
			thread::RMutex::MainThreadMutex()->Lock();
			delete m_liveCommentThread;
			m_liveCommentThread = NULL;
		}
		common::transition::DoReverse(0.0f, m_companelRoot, common::transition::Type_FadeinFast, true, false);
		m_companelRoot = NULL;
	}

	g_appPlugin->TemplateOpen(m_companelBase, template_scroll_view_youtube_companel, tmpParam);
	m_companelRoot = static_cast<ui::ListView *>(m_companelBase->GetChild(m_companelBase->GetChildrenNum() - 1));
	m_companelRoot->Show(common::transition::Type_FadeinFast);
	m_companelRoot->SetName("description_root");

	ui::Widget *descText = m_companelRoot->FindChild(text_youtube_companel);

	common::Utf8ToUtf16(m_description, &text16);
	text16 += L"\n\n";
	descText->SetString(text16);
}

void menu::PlayerYoutube::OnBackButton()
{
	delete m_player;
	delete this;
}

void menu::PlayerYoutube::OnDwAddComplete(int32_t result)
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

void menu::PlayerYoutube::OnSettingsButton()
{
	vector<OptionMenu::Button> buttons;
	OptionMenu::Button bt;
	bt.label = g_appPlugin->GetString(msg_settings);
	buttons.push_back(bt);
	if (!m_isLive)
	{
		bt.label = g_appPlugin->GetString(msg_settings_youtube_download);
		buttons.push_back(bt);
		if (!m_audioLink.empty())
		{
			bt.label = g_appPlugin->GetString(msg_settings_youtube_download_audio);
			buttons.push_back(bt);
		}
	}

	new OptionMenu(g_appPlugin, m_root, &buttons);
}

void menu::PlayerYoutube::OnOptionMenuEvent(int32_t type, int32_t subtype)
{
	if (type == OptionMenu::OptionMenuEvent_Close)
	{
		return;
	}

	wstring title16;
	string title8;
	int32_t res = SCE_OK;
	switch (subtype)
	{
	case 0:
		menu::GetMenuAt(menu::GetMenuCount() - 2)->DisableInput();
		menu::SettingsButtonCbFun(ui::Button::CB_BTN_DECIDE, NULL, 0, NULL);
		break;
	case 1:
		m_title->GetString(title16);
		common::Utf16ToUtf8(title16, &title8);
		title8 += ".mp4";

		res = ytutils::EnqueueDownloadAsync(m_videoLinkForDw.c_str(), title8.c_str());
		SCE_DBG_LOG_INFO("[OnOptionMenuEvent] Enqueue video download: %s", title8.c_str());
		if (res == SCE_OK)
		{
			dialog::OpenPleaseWait(g_appPlugin, NULL, Framework::Instance()->GetCommonString("msg_wait"));
		}
		else
		{
			dialog::OpenError(g_appPlugin, res);
		}
		break;
	case 2:
		m_title->GetString(title16);
		common::Utf16ToUtf8(title16, &title8);
		title8 += ".webm";

		res = ytutils::EnqueueDownloadAsync(m_audioLink.c_str(), title8.c_str());
		SCE_DBG_LOG_INFO("[OnOptionMenuEvent] Enqueue audio download: %s", title8.c_str());
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

void menu::PlayerYoutube::OnPlayerEvent(int32_t type)
{
	SCE_DBG_LOG_INFO("[OnPlayerEvent] Event: %d", type);

	switch (type)
	{
	case PlayerSimple::PlayerEvent_Back:
		if (m_player->GetScale() != 1.0f)
		{
			delete m_player;
		}
		else
		{
			m_player->SetScale(0.5f);
			if (SCE_PAF_IS_DOLCE)
			{
				m_player->SetPosition(26.0f, -158.0f);
			}
			else
			{
				m_player->SetPosition(13.0f, -79.0f);
			}
		}
		break;
	case PlayerSimple::PlayerEvent_InitOk:
		int32_t min = 0;

		ui::BusyIndicator *loaderIndicator = static_cast<ui::BusyIndicator *>(m_root->FindChild(busyindicator_youtube_loader));
		ui::Widget *loaderPlane = m_root->FindChild(plane_youtube_loader);

		if (m_player->GetPlayer()->GetVideoRepresentationNum() > 1)
		{
			m_videoSettingsButton->Show();
		}

		loaderIndicator->Stop();
		common::transition::DoReverse(0.0f, loaderPlane, common::transition::Type_FadeinFast, true, false);
		EnableInput();

		menu::Settings::GetAppSetInstance()->GetInt("yt_min", &min, 0);
		if (min)
		{
			m_player->SetScale(0.5f);
			if (SCE_PAF_IS_DOLCE)
			{
				m_player->SetPosition(26.0f, -158.0f);
			}
			else
			{
				m_player->SetPosition(13.0f, -79.0f);
			}
		}
		else
		{
			m_player->SetPosition(0.0f, 0.0f);
			m_root->Hide();
		}
		break;
	case PlayerSimple::PlayerEvent_InitFail:
		delete m_player;
		m_player = NULL;

		//if (m_lastAttempt)
		{
			//m_lastAttempt = false;

			ui::BusyIndicator *loaderIndicator = static_cast<ui::BusyIndicator *>(m_root->FindChild(busyindicator_youtube_loader));
			ui::Widget *loaderPlane = m_root->FindChild(plane_youtube_loader);

			loaderIndicator->Stop();
			common::transition::DoReverse(0.0f, loaderPlane, common::transition::Type_FadeinFast, true, false);
			EnableInput();

			m_favButton->Disable(false);
			m_expandButton->Disable(false);
			m_videoSettingsButton->Disable(false);

			dialog::OpenError(g_appPlugin, SCE_ERROR_ERRNO_EUNSUP, Framework::Instance()->GetCommonString("msg_error_connect_server_peer"));
		}
		//else
		//{
		//	utils::SetDisplayResolution(ui::EnvironmentParam::RESOLUTION_HD_FULL);
		//	m_lastAttempt = true;
		//	LoadJob *job = new LoadJob(this);
		//	common::SharedPtr<job::JobItem> itemParam(job);
		//	utils::GetJobQueue()->Enqueue(itemParam);
		//}
		break;
	}
}

void menu::PlayerYoutube::OnSettingsEvent(int32_t type)
{
	SCE_DBG_LOG_INFO("[OnSettingsEvent] Event: %d", type);

	if (type == Settings::SettingsEvent_Close)
	{
		menu::GetMenuAt(menu::GetMenuCount() - 2)->EnableInput();
	}
	else if (type == Settings::SettingsEvent_ValueChange)
	{
		
	}
}

menu::PlayerYoutube::PlayerYoutube(const char *id, bool isFavourite) :
	GenericMenu("page_youtube_player",
	MenuOpenParam(false, 200.0f, Plugin::TransitionType_SlideFromBottom),
	MenuCloseParam(false, 200.0f, Plugin::TransitionType_SlideFromBottom))
{
	m_player = NULL;
	m_videoId = id;
	m_isLive = false;
	m_isFav = isFavourite;
	m_companelRoot = NULL;
	m_liveCommentThread = NULL;
	m_option = NULL;

	m_root->AddEventCallback(OptionMenu::OptionMenuEvent,
	[](int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
	{
		reinterpret_cast<menu::PlayerYoutube *>(userdata)->OnOptionMenuEvent(e->GetValue(0), e->GetValue(1));
	}, this);

	m_root->AddEventCallback(Settings::SettingsEvent,
	[](int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
	{
		reinterpret_cast<menu::PlayerYoutube *>(userdata)->OnSettingsEvent(e->GetValue(0));
	}, this);

	m_root->AddEventCallback(PlayerSimple::PlayerSimpleEvent,
	[](int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
	{
		reinterpret_cast<menu::PlayerYoutube *>(userdata)->OnPlayerEvent(e->GetValue(0));
	}, this);

	m_root->AddEventCallback(Downloader::DownloaderEvent,
	[](int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
	{
		reinterpret_cast<menu::PlayerYoutube *>(userdata)->OnDwAddComplete(e->GetValue(0));
	}, this);

	ui::BusyIndicator *loaderIndicator = static_cast<ui::BusyIndicator *>(m_root->FindChild(busyindicator_youtube_loader));
	loaderIndicator->Start();

	ui::Widget *settingsButton = m_root->FindChild(button_settings_page_youtube_player);
	settingsButton->Show(common::transition::Type_Reset);
	settingsButton->AddEventCallback(ui::Button::CB_BTN_DECIDE,
	[](int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
	{
		reinterpret_cast<menu::PlayerYoutube *>(userdata)->OnSettingsButton();
	}, this);

	ui::Widget *backButton = m_root->FindChild(button_back_page_youtube_player);
	backButton->Show(common::transition::Type_Reset);
	backButton->AddEventCallback(ui::Button::CB_BTN_DECIDE,
	[](int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
	{
		reinterpret_cast<menu::PlayerYoutube *>(userdata)->OnBackButton();
	}, this);

	m_title = static_cast<ui::Text *>(m_root->FindChild(text_video_title));
	m_stat0 = static_cast<ui::Text *>(m_root->FindChild(text_video_stat_0));
	m_stat1 = static_cast<ui::Text *>(m_root->FindChild(text_video_stat_1));
	m_stat2 = static_cast<ui::Text *>(m_root->FindChild(text_video_stat_2));

	m_expandButton = static_cast<ui::Button *>(m_root->FindChild(button_youtube_expand));
	m_expandButton->AddEventCallback(ui::Button::CB_BTN_DECIDE,
	[](int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
	{
		reinterpret_cast<menu::PlayerYoutube *>(userdata)->OnExpandButton();
	}, this);

	m_videoSettingsButton = static_cast<ui::Button *>(m_root->FindChild(button_youtube_settings));
	m_videoSettingsButton->Hide();
	m_videoSettingsButton->AddEventCallback(ui::Button::CB_BTN_DECIDE,
		[](int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
	{
		reinterpret_cast<menu::PlayerYoutube *>(userdata)->OnVideoSettingsButton();
	}, this);

	m_favButton = static_cast<ui::Button *>(m_root->FindChild(button_youtube_fav));
	m_favButton->AddEventCallback(ui::Button::CB_BTN_DECIDE,
	[](int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
	{
		reinterpret_cast<menu::PlayerYoutube *>(userdata)->OnFavButton();
	}, this);

	if (m_isFav)
	{
		intrusive_ptr<graph::Surface> favIcon = g_appPlugin->GetTexture(tex_yt_icon_favourite_for_player_glow);
		m_favButton->SetTexture(favIcon);
	}
	m_companelBase = m_root->FindChild(plane_youtube_companel_base);
	m_commentButton = m_root->FindChild(button_yt_companel_comment);
	m_commentButton->AddEventCallback(ui::Button::CB_BTN_DECIDE,
	[](int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
	{
		reinterpret_cast<menu::PlayerYoutube *>(userdata)->OnCommentButton();
	}, this);

	m_descButton = m_root->FindChild(button_yt_companel_description);
	m_descButton->AddEventCallback(ui::Button::CB_BTN_DECIDE,
	[](int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
	{
		reinterpret_cast<menu::PlayerYoutube *>(userdata)->OnDescButton();
	}, this);

	DisableInput();

	LoadJob *job = new LoadJob(this);
	common::SharedPtr<job::JobItem> itemParam(job);
	utils::GetJobQueue()->Enqueue(itemParam);
}

menu::PlayerYoutube::~PlayerYoutube()
{
	if (m_liveCommentThread)
	{
		m_liveCommentThread->Cancel();
		m_liveCommentThread->Join();
		delete m_liveCommentThread;
	}

	if (m_companelRoot)
	{
		m_companelRoot->SetName((uint32_t)0);
	}

	if (m_option)
	{
		delete m_option;
	}
}