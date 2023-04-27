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

			using job::JobItem::JobItem;

			~LoadJob() {}

			void Run();

			void Finish() {}

			PlayerYoutube *workObj;
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

			~HlsCommentListViewCb()
			{

			}

			ui::ListItem* Create(CreateParam& param);
		};

		class CommentListViewCb : public ui::listview::ItemFactory
		{
		public:

			~CommentListViewCb()
			{

			}

			ui::ListItem* Create(CreateParam& param);

			PlayerYoutube *workObj;
		};

		class HlsCommentParseThread : public thread::Thread
		{
		public:

			using thread::Thread::Thread;

			void EntryFunction();

			PlayerYoutube *workObj;
		};

		class CommentParseJob : public job::JobItem
		{
		public:

			using job::JobItem::JobItem;

			~CommentParseJob() {}

			void Run();

			void Finish() {}

			PlayerYoutube *workObj;
		};

		static void TaskLoadSecondStage(void *pArgBlock);
		static void DwAddCompleteCb(int32_t result);
		static void BackButtonCbFun(int32_t type, ui::Handler *self, ui::Event *e, void *userdata);
		static void SettingsButtonCbFun(int32_t type, ui::Handler *self, ui::Event *e, void *userdata);
		static void ExpandButtonCbFun(int32_t type, ui::Handler *self, ui::Event *e, void *userdata);
		static void FavButtonCbFun(int32_t type, ui::Handler *self, ui::Event *e, void *userdata);
		static void CommentButtonCbFun(int32_t type, ui::Handler *self, ui::Event *e, void *userdata);
		static void DescButtonCbFun(int32_t type, ui::Handler *self, ui::Event *e, void *userdata);
		static void CommentBodyButtonCbFun(int32_t type, ui::Handler *self, ui::Event *e, void *userdata);
		static void PlayerEventCbFun(int32_t type, ui::Handler *self, ui::Event *e, void *userdata);
		static void OptionMenuEventCbFun(int32_t type, ui::Handler *self, ui::Event *e, void *userdata);
		static void SettingsEventCbFun(int32_t type, ui::Handler *self, ui::Event *e, void *userdata);

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

		ui::Text *title;
		ui::Text *stat0;
		ui::Text *stat1;
		ui::Text *stat2;
		ui::Widget *expandButton;
		ui::Widget *favButton;
		ui::Widget *companelBase;
		ui::Widget *commentButton;
		ui::Widget *descButton;
		ui::Widget *companelRoot;
		menu::PlayerSimple *player;
		string videoLink;
		string videoId;
		bool isHls;
		bool isHighHls;
		bool isFav;
		string description;
		string commentCont;
		vector<CommentItem> commentItems;
		HlsCommentParseThread *hlsCommentThread;
		bool lastAttempt;

		const uint32_t k_settingsIdList[4] = {
			youtube_search_setting,
			youtube_comment_setting,
			youtube_quality_setting,
			youtube_player_setting
		};
	};
}

#endif
