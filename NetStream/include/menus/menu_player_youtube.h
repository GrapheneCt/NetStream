#ifndef _MENU_PLAYER_YOUTUBE_H_
#define _MENU_PLAYER_YOUTUBE_H_

#include <kernel.h>
#include <paf.h>

#include "dialog.h"
#include "invidious.h"
#include "menu_generic.h"
#include "menus/menu_player_simple.h"

using namespace paf;

#define HLS_COMMENT_NUM		6
#define HLS_COMMENT_MAGIC	0x16865ddb

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

			void Run()
			{
				m_parent->Load();
			}

			void Finish() {}

		private:

			PlayerYoutube *m_parent;
		};

		class CommentItem
		{
		public:

			wstring author;
			wstring content;
			wstring date;
			int32_t likeCount;
			bool likedByOwner;
			string replyContinuation;
		};

		class HlsCommentListViewCb : public ui::listview::ItemFactory
		{
		public:

			HlsCommentListViewCb(menu::PlayerYoutube *parent) : m_parent(parent)
			{

			}

			~HlsCommentListViewCb()
			{

			}

			ui::ListItem* Create(CreateParam& param)
			{
				return m_parent->CreateHlsCommentListItem(param);
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

		class HlsCommentParseThread : public thread::Thread
		{
		public:

			HlsCommentParseThread(PlayerYoutube *parent) : thread::Thread(SCE_KERNEL_DEFAULT_PRIORITY_USER + 20, SCE_KERNEL_64KiB, "YouTube::CommentParseThread", NULL), m_parent(parent)
			{

			}

			void EntryFunction()
			{
				m_parent->ParseHlsComments(this);
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

			void Run()
			{
				m_parent->ParseComments();
			}

			void Finish() {}

		private:

			PlayerYoutube *m_parent;
		};

		PlayerYoutube(const char *id, bool isFavourite);

		~PlayerYoutube();

		MenuType GetMenuType()
		{
			return MenuType_PlayerYouTube;
		}

		const uint32_t *GetSupportedSettingsItems(int32_t *count)
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
		void OnFavButton();
		void OnCommentButton();
		void OnDescButton();
		void OnCommentBodyButton(ui::Widget *wdg);
		void OnPlayerEvent(int32_t type);
		void OnOptionMenuEvent(int32_t type, int32_t subtype);
		void OnSettingsEvent(int32_t type);

		void Load();
		void ParseComments();
		ui::ListItem *CreateHlsCommentListItem(ui::listview::ItemFactory::CreateParam& param);
		ui::ListItem *CreateCommentListItem(ui::listview::ItemFactory::CreateParam& param);
		void ParseHlsComments(thread::Thread *thrd);

		ui::Text *m_title;
		ui::Text *m_stat0;
		ui::Text *m_stat1;
		ui::Text *m_stat2;
		ui::Button *m_expandButton;
		ui::Button *m_favButton;
		ui::Widget *m_companelBase;
		ui::Widget *m_commentButton;
		ui::Widget *m_descButton;
		ui::ListView *m_companelRoot;
		menu::PlayerSimple *m_player;
		string m_videoLink;
		string m_audioLink;
		string m_videoId;
		bool m_isHls;
		bool m_isHighHls;
		bool m_isFav;
		string m_description;
		string m_commentCont;
		vector<CommentItem> m_commentItems;
		HlsCommentParseThread *m_hlsCommentThread;
		bool m_lastAttempt;

		const uint32_t k_settingsIdList[4] = {
			youtube_search_setting,
			youtube_comment_setting,
			youtube_quality_setting,
			youtube_player_setting
		};
	};
}

#endif
