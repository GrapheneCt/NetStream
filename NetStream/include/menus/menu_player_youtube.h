#ifndef _MENU_PLAYER_YOUTUBE_H_
#define _MENU_PLAYER_YOUTUBE_H_

#include <kernel.h>
#include <paf.h>

#include "dialog.h"
#include "ftube.h"
#include "menu_generic.h"
#include "menus/menu_player_simple.h"

using namespace paf;

#define LIVE_COMMENT_NUM		6
#define LIVE_COMMENT_MAGIC		0x16865ddb
#define SELECT_QUALITY_MAGIC	0x3e9f36f4

#define USE_LIVE_COMMENT_HASHING

namespace menu {

	class PlayerYoutube : public GenericMenu
	{
	public:

		enum CompanelMode
		{
			CompanelMode_None,
			CompanelMode_Description,
			CompanelMode_Comment
		};

		class LoadJob : public job::JobItem
		{
		public:

			LoadJob(PlayerYoutube *parent) : job::JobItem("YouTube::LoadJob", NULL), m_parent(parent)
			{

			}

			~LoadJob() {}

			int32_t Run()
			{
				m_parent->Load();

				return SCE_PAF_OK;
			}

			void Finish(int32_t result) {}

		private:

			PlayerYoutube *m_parent;
		};

		class CommentItem
		{
		public:

			wstring author;
			wstring content;
			wstring date;
			wstring voteCount;
			bool likedByOwner;
			string replyContinuation;
		};

		class LiveCommentListViewCb : public ui::listview::ItemFactory
		{
		public:

			LiveCommentListViewCb(menu::PlayerYoutube *parent) : m_parent(parent)
			{

			}

			~LiveCommentListViewCb()
			{

			}

			ui::ListItem* Create(CreateParam& param)
			{
				return m_parent->CreateLiveCommentListItem(param);
			}

		private:

			menu::PlayerYoutube *m_parent;
		};

		class CommentListViewCb : public ui::listview::ItemFactory
		{
		public:

			CommentListViewCb(menu::PlayerYoutube *parent) : m_parent(parent)
			{

			}

			~CommentListViewCb()
			{

			}

			ui::ListItem* Create(CreateParam& param)
			{
				return m_parent->CreateCommentListItem(param);
			}

		private:

			PlayerYoutube *m_parent;
		};

		class SelectQualityDialogListViewCb : public ui::listview::ItemFactory
		{
		public:

			SelectQualityDialogListViewCb(menu::PlayerYoutube *parent) : m_parent(parent)
			{

			}

			~SelectQualityDialogListViewCb()
			{

			}

			ui::ListItem* Create(CreateParam& param)
			{
				return m_parent->CreateSelectQualityListItem(param);
			}

		private:

			menu::PlayerYoutube *m_parent;
		};

		class LiveCommentParseThread : public thread::Thread
		{
		public:

			LiveCommentParseThread(PlayerYoutube *parent) : thread::Thread(SCE_KERNEL_DEFAULT_PRIORITY_USER + 20, SCE_KERNEL_64KiB, "YouTube::CommentParseThread", NULL), m_parent(parent)
			{

			}

			void EntryFunction()
			{
				m_parent->ParseLiveComments(this);
			}

		private:

			PlayerYoutube *m_parent;
		};

		class CommentParseJob : public job::JobItem
		{
		public:

			CommentParseJob(PlayerYoutube *parent) : job::JobItem("YouTube::CommentParseJob", NULL), m_parent(parent)
			{

			}

			~CommentParseJob() {}

			int32_t Run()
			{
				m_parent->ParseComments();

				return SCE_PAF_OK;
			}

			void Finish(int32_t result) {}

		private:

			PlayerYoutube *m_parent;
		};

		PlayerYoutube(const char *id, bool isFavourite);

		~PlayerYoutube() override;

		MenuType GetMenuType() override
		{
			return MenuType_PlayerYouTube;
		}

		const uint32_t *GetSupportedSettingsItems(int32_t *count) override
		{
			*count = sizeof(k_settingsIdList) / sizeof(char*);
			return k_settingsIdList;
		}

	private:

		static void LoadSecondStageTask(void *pArgBlock);
		void OnLoadSecondStage();
		void OnDwAddComplete(int32_t result);
		void OnBackButton();
		void OnSettingsButton();
		void OnExpandButton();
		void OnVideoSettingsButton();
		void OnSelectQualityButton(ui::Widget *wdg);
		void OnFavButton();
		void OnCommentButton();
		void OnDescButton();
		void OnCommentBodyButton(ui::Widget *wdg);
		void OnPlayerEvent(int32_t type);
		void OnOptionMenuEvent(int32_t type, int32_t subtype);
		void OnSettingsEvent(int32_t type);

		void Load();
		void ParseComments();
		ui::ListItem *CreateLiveCommentListItem(ui::listview::ItemFactory::CreateParam& param);
		ui::ListItem *CreateCommentListItem(ui::listview::ItemFactory::CreateParam& param);
		void ParseLiveComments(thread::Thread *thrd);

		ui::ListItem *CreateSelectQualityListItem(ui::listview::ItemFactory::CreateParam& param);

		ui::Text *m_title;
		ui::Text *m_stat0;
		ui::Text *m_stat1;
		ui::Text *m_stat2;
		ui::Button *m_expandButton;
		ui::Button *m_videoSettingsButton;
		ui::Button *m_favButton;
		ui::Widget *m_companelBase;
		ui::Widget *m_commentButton;
		ui::Widget *m_descButton;
		ui::ListView *m_companelRoot;
		menu::PlayerSimple *m_player;
		string m_videoLink;
		string m_videoLinkForDw;
		string m_audioLink;
		string m_videoId;
		bool m_isLive;
		bool m_isFav;
		string m_description;
		string m_commentCont;
		vector<CommentItem> m_commentItems;
		LiveCommentParseThread *m_liveCommentThread;
		GenericPlayer::Option *m_option;

		const uint32_t k_settingsIdList[5] = {
			youtube_search_setting,
			youtube_comment_setting,
			youtube_quality_setting,
			youtube_player_setting,
			cloud_sync_setting
		};
	};
}

#endif
