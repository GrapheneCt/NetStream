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

			SceVoid Run();

			SceVoid Finish() {}

			PlayerYoutube *workObj;
		};

		class CommentItem
		{
		public:

			wstring author;
			wstring content;
			wstring date;
			SceInt32 likeCount;
			SceBool likedByOwner;
			string replyContinuation;
		};

		class HlsCommentListViewCb : public ui::ListView::ItemCallback
		{
		public:

			~HlsCommentListViewCb()
			{

			}

			ui::ListItem *Create(Param *info);
		};

		class CommentListViewCb : public ui::ListView::ItemCallback
		{
		public:

			~CommentListViewCb()
			{

			}

			ui::ListItem *Create(Param *info);

			PlayerYoutube *workObj;
		};

		class HlsCommentParseThread : public thread::Thread
		{
		public:

			using thread::Thread::Thread;

			SceVoid EntryFunction();

			PlayerYoutube *workObj;
		};

		class CommentParseJob : public job::JobItem
		{
		public:

			using job::JobItem::JobItem;

			~CommentParseJob() {}

			SceVoid Run();

			SceVoid Finish() {}

			PlayerYoutube *workObj;
		};

		static SceVoid PlayerBackCb(PlayerSimple *player, ScePVoid pUserArg);
		static SceVoid PlayerOkCb(PlayerSimple *player, ScePVoid pUserArg);
		static SceVoid PlayerFailCb(PlayerSimple *player, ScePVoid pUserArg);
		static SceVoid TaskLoadSecondStage(void *pArgBlock);
		static SceVoid DwAddCompleteCb(SceInt32 result);
		static SceVoid OptionButtonCb(SceUInt32 index, ScePVoid pUserData);
		static SceVoid BackButtonCbFun(SceInt32 eventId, ui::Widget *self, SceInt32 a3, ScePVoid pUserData);
		static SceVoid SettingsButtonCbFun(SceInt32 eventId, ui::Widget *self, SceInt32 a3, ScePVoid pUserData);
		static SceVoid ExpandButtonCbFun(SceInt32 eventId, ui::Widget *self, SceInt32 a3, ScePVoid pUserData);
		static SceVoid FavButtonCbFun(SceInt32 eventId, ui::Widget *self, SceInt32 a3, ScePVoid pUserData);
		static SceVoid CommentButtonCbFun(SceInt32 eventId, ui::Widget *self, SceInt32 a3, ScePVoid pUserData);
		static SceVoid DescButtonCbFun(SceInt32 eventId, ui::Widget *self, SceInt32 a3, ScePVoid pUserData);
		static SceVoid CommentBodyButtonCbFun(SceInt32 eventId, ui::Widget *self, SceInt32 a3, ScePVoid pUserData);
		static SceVoid SettingsCloseCbFun(ScePVoid pUserData);

		PlayerYoutube(const char *id, SceBool isFavourite);

		~PlayerYoutube();

		MenuType GetMenuType()
		{
			return MenuType_PlayerYouTube;
		}

		const SceUInt32 *GetSupportedSettingsItems(SceInt32 *count)
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
		SceBool isHls;
		SceBool isHighHls;
		SceBool isFav;
		string description;
		string commentCont;
		vector<CommentItem> commentItems;
		HlsCommentParseThread *hlsCommentThread;
		SceBool lastAttempt;

		const SceUInt32 k_settingsIdList[4] = {
			youtube_search_setting,
			youtube_comment_setting,
			youtube_quality_setting,
			youtube_player_setting
		};
	};
}

#endif
