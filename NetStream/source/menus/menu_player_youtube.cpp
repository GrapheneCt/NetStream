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

void menu::PlayerYoutube::LoadSecondStageTask(void *pArgBlock)
{
	reinterpret_cast<PlayerYoutube *>(pArgBlock)->OnLoadSecondStage();
	common::MainThreadCallList::Unregister(LoadSecondStageTask, pArgBlock);
}

void menu::PlayerYoutube::Load()
{
	InvItemVideo *invItem = NULL;
	int32_t ret = 0;
	string text8;
	wstring text16;

	ret = invParseVideo(m_videoId.c_str(), &invItem);
	if (ret == true)
	{
		if (invItem->id)
		{
			if (!m_lastAttempt)
			{
				m_description = invItem->description;

				thread::RMutex::main_thread_mutex.Lock();

				OnDescButton();

				text8 = invItem->title;
				common::Utf8ToUtf16(text8, &text16);
				m_title->SetString(text16);

				text8 = "Uploaded by ";
				text8 += invItem->author;
				common::Utf8ToUtf16(text8, &text16);
				m_stat0->SetString(text16);

				text8 = invItem->subCount;
				text8 += " subscribers";
				common::Utf8ToUtf16(text8, &text16);
				m_stat1->SetString(text16);

				text8 = invItem->published;
				common::Utf8ToUtf16(text8, &text16);
				m_stat2->SetString(text16);

				thread::RMutex::main_thread_mutex.Unlock();
			}

			int32_t quality = 0;
			sce::AppSettings *settings = menu::Settings::GetAppSetInstance();

			if (invItem->isLive)
			{
				if (m_lastAttempt)
				{
					ret = -1;
					goto releaseLocks;
				}

				char *hls = new char[SCE_KERNEL_1KiB];
				hls[0] = 0;
				settings->GetInt("yt_hls_quality", static_cast<int32_t *>(&quality), 0);
				ret = invGetHlsUrl(invItem, static_cast<InvHlsQuality>(quality), hls, SCE_KERNEL_1KiB);
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
					invGetHlsUrl(invItem, static_cast<InvHlsQuality>(quality), hls, SCE_KERNEL_1KiB);
				}
				m_videoLink = hls;
				m_isHls = true;
				if (quality == 4)
				{
					m_isHighHls = true;
				}
				delete hls;

				ui::Widget *livePlane = m_root->FindChild(plane_youtube_live_now);
				intrusive_ptr<graph::Surface> liveTex = g_appPlugin->GetTexture(tex_yt_icon_live_now);
				thread::RMutex::main_thread_mutex.Lock();
				livePlane->SetTexture(liveTex);
				thread::RMutex::main_thread_mutex.Unlock();
			}
			else
			{
				settings->GetInt("yt_quality", static_cast<int32_t *>(&quality), 0);
				if (m_lastAttempt)
				{
					char lastResort[256];
					char lastResortAudio[256];
					lastResort[0] = 0;
					lastResortAudio[0] = 0;
					ret = invGetProxyUrl(invItem, static_cast<InvProxyType>(quality), lastResort, sizeof(lastResort));
					m_videoLink = lastResort;
					invGetProxyUrl(invItem, INV_PROXY_AUDIO_HQ, lastResortAudio, sizeof(lastResortAudio));
					m_audioLink = lastResortAudio;
				}
				else
				{
					switch (quality)
					{
					case 0:
						if (invItem->avcLqUrl)
							m_videoLink = invItem->avcLqUrl;
						else
							m_videoLink = invItem->avcHqUrl;
						break;
					case 1:
						if (invItem->avcHqUrl)
							m_videoLink = invItem->avcHqUrl;
						else
							m_videoLink = invItem->avcLqUrl;
						break;
					}
					m_audioLink = invItem->audioHqUrl;
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
		m_videoLink = "";
		m_audioLink = "";
	}

	common::MainThreadCallList::Register(menu::PlayerYoutube::LoadSecondStageTask, this);
}

ui::ListItem *menu::PlayerYoutube::CreateHlsCommentListItem(ui::listview::ItemFactory::CreateParam& param)
{
	Plugin::TemplateOpenParam tmpParam;
	ui::Widget *item = NULL;
	wstring text16;

	ui::Widget *targetRoot = param.parent;

	g_appPlugin->TemplateOpen(targetRoot, template_list_item_youtube_comment, tmpParam);
	item = targetRoot->GetChild(targetRoot->GetChildrenNum() - 1);

	ui::Widget *button = item->FindChild(button_yt_companel_comment_item);
	button->SetName(HLS_COMMENT_MAGIC + param.cell_index);

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

void menu::PlayerYoutube::ParseHlsComments(thread::Thread *thrd)
{
	ui::ListView *list = m_companelRoot;
	ui::Widget *button = NULL;
	InvItemComment *comment;
	int32_t commCount = 0;
	uint32_t realCommentNum = HLS_COMMENT_NUM;
	uint32_t lastHash = 0;
	string text8;
	wstring text16;
	wstring tmpText16;

	while (!thrd->IsCanceled())
	{
		realCommentNum = HLS_COMMENT_NUM;
		comment = NULL;

		commCount = invParseHlsComments(m_videoId.c_str(), &comment);
		if (commCount <= 0)
		{
			thread::Sleep(33);
			invCleanupHlsComments(comment);
			if (thrd->IsCanceled())
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
			if (thrd->IsCanceled())
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

	thrd->Cancel();
}

void menu::PlayerYoutube::ParseComments()
{
	InvItemComment *comments = NULL;
	InvCommentSort sort;
	string text8;
	wstring text16;
	int32_t ret = 0;
	uint32_t addOverhead = 0;
	uint32_t oldCommentCount = 0;

	sce::AppSettings *settings = menu::Settings::GetAppSetInstance();
	settings->GetInt("yt_comment_sort", reinterpret_cast<int32_t *>(&sort), 0);

	if (m_commentCont.empty())
	{
		ret = invParseComments(m_videoId.c_str(), NULL, sort, &comments);
	}
	else
	{
		ret = invParseComments(m_videoId.c_str(), m_commentCont.c_str(), sort, &comments);
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
		m_commentItems.push_back(item);
	}

	if (comments[ret - 1].continuation)
	{
		m_commentCont = comments[ret - 1].continuation;
		addOverhead = 1;
	}
	else
	{
		m_commentCont = "end";
	}

	ui::ListView *list = static_cast<ui::ListView *>(m_companelRoot);
	thread::RMutex::main_thread_mutex.Lock();
	list->InsertCell(0, oldCommentCount, m_commentItems.size() + addOverhead - oldCommentCount);
	thread::RMutex::main_thread_mutex.Unlock();

	invCleanupComments(comments);
}

void menu::PlayerYoutube::OnLoadSecondStage()
{
	m_player = new menu::PlayerSimple(m_videoLink.c_str());
	m_player->SetPosition(-1920.0f, -1080.0f);
	m_root->SetShowAlpha(1.0f);
	if (m_isHighHls)
	{
		m_player->GetPlayer()->LimitFPS(true);
	}
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

void menu::PlayerYoutube::OnExpandButton()
{
	m_player->SetScale(1.0f);
	m_player->SetPosition(0.0f, 0.0f);
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
		common::transition::DoReverse(0.0f, m_companelRoot, common::transition::Type_Fadein1, true, false);
		m_companelRoot = NULL;
	}

	g_appPlugin->TemplateOpen(m_companelBase, template_list_view_youtube_companel, tmpParam);
	m_companelRoot = static_cast<ui::ListView *>(m_companelBase->GetChild(m_companelBase->GetChildrenNum() - 1));
	m_companelRoot->Show(common::transition::Type_Fadein1);
	m_companelRoot->SetName("comment_root");

	ui::ListView *list = (ui::ListView *)m_companelRoot;

	list->InsertSegment(0, 1);
	math::v4 sz(414.0f, 70.0f);
	list->SetCellSizeDefault(0, sz);
	list->SetSegmentLayoutType(0, ui::ListView::LAYOUT_TYPE_LIST);

	m_commentItems.clear();

	if (m_isHls)
	{
		HlsCommentListViewCb *lwCb = new HlsCommentListViewCb(this);
		list->SetItemFactory(lwCb);
		list->InsertCell(0, 0, HLS_COMMENT_NUM);

		m_hlsCommentThread = new HlsCommentParseThread(this);
		m_hlsCommentThread->Start();
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
		if (m_hlsCommentThread)
		{
			m_hlsCommentThread->Cancel();
			thread::RMutex::main_thread_mutex.Unlock();
			m_hlsCommentThread->Join();
			thread::RMutex::main_thread_mutex.Lock();
			delete m_hlsCommentThread;
			m_hlsCommentThread = NULL;
		}
		common::transition::DoReverse(0.0f, m_companelRoot, common::transition::Type_Fadein1, true, false);
		m_companelRoot = NULL;
	}

	g_appPlugin->TemplateOpen(m_companelBase, template_scroll_view_youtube_companel, tmpParam);
	m_companelRoot = static_cast<ui::ListView *>(m_companelBase->GetChild(m_companelBase->GetChildrenNum() - 1));
	m_companelRoot->Show(common::transition::Type_Fadein1);
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
	if (!m_isHls)
	{
		bt.label = g_appPlugin->GetString(msg_settings_youtube_download);
		buttons.push_back(bt);
		bt.label = g_appPlugin->GetString(msg_settings_youtube_download_audio);
		buttons.push_back(bt);
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

		res = ytutils::EnqueueDownloadAsync(m_videoLink.c_str(), title8.c_str());
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

		loaderIndicator->Stop();
		common::transition::DoReverse(0.0f, loaderPlane, common::transition::Type_Fadein1, true, false);
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

		if (m_lastAttempt)
		{
			m_lastAttempt = false;

			ui::BusyIndicator *loaderIndicator = static_cast<ui::BusyIndicator *>(m_root->FindChild(busyindicator_youtube_loader));
			ui::Widget *loaderPlane = m_root->FindChild(plane_youtube_loader);

			loaderIndicator->Stop();
			common::transition::DoReverse(0.0f, loaderPlane, common::transition::Type_Fadein1, true, false);
			EnableInput();

			m_favButton->Disable(false);
			m_expandButton->Disable(false);

			dialog::OpenError(g_appPlugin, SCE_ERROR_ERRNO_EUNSUP, Framework::Instance()->GetCommonString("msg_error_connect_server_peer"));
		}
		else
		{
			utils::SetDisplayResolution(ui::EnvironmentParam::RESOLUTION_HD_FULL);
			m_lastAttempt = true;
			LoadJob *job = new LoadJob(this);
			common::SharedPtr<job::JobItem> itemParam(job);
			utils::GetJobQueue()->Enqueue(itemParam);
		}
		break;
	}
}

void menu::PlayerYoutube::OnSettingsEvent(int32_t type)
{
	if (type == Settings::SettingsEvent_Close)
	{
		menu::GetMenuAt(menu::GetMenuCount() - 2)->EnableInput();
	}
}

menu::PlayerYoutube::PlayerYoutube(const char *id, bool isFavourite) :
	GenericMenu("page_youtube_player",
	MenuOpenParam(false, 200.0f, Plugin::TransitionType_SlideFromBottom),
	MenuCloseParam(false, 200.0f, Plugin::TransitionType_SlideFromBottom))
{
	m_player = NULL;
	m_videoId = id;
	m_isHls = false;
	m_isHighHls = false;
	m_isFav = isFavourite;
	m_companelRoot = NULL;
	m_hlsCommentThread = NULL;
	m_lastAttempt = false;

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
	if (m_hlsCommentThread)
	{
		m_hlsCommentThread->Cancel();
		m_hlsCommentThread->Join();
		delete m_hlsCommentThread;
	}

	if (m_companelRoot)
	{
		m_companelRoot->SetName((uint32_t)0);
	}
}