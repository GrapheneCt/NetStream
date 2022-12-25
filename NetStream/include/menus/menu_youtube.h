#ifndef _MENU_YOUTUBE_H_
#define _MENU_YOUTUBE_H_

#include <kernel.h>
#include <paf.h>

#include "dialog.h"
#include "invidious.h"
#include "menu_generic.h"

using namespace paf;

namespace menu {
	class YouTube : public GenericMenu
	{
	public:

		class Submenu
		{
		public:

			enum SubmenuType
			{
				SubmenuType_Search = 0xbe840a1f,
				SubmenuType_History = 0x2bc652ba,
				SubmenuType_Favourites = 0x947652c2
			};

			Submenu(YouTube *parentObj);

			virtual ~Submenu();

			virtual SceVoid ReleaseCurrentPage();

			virtual SceVoid GoToNextPage() = 0;

			virtual SceVoid GoToPrevPage() = 0;

			virtual SubmenuType GetType() = 0;

			class ListViewCb : public ui::ListView::ItemCallback
			{
			public:

				~ListViewCb()
				{

				}

				ui::ListItem *Create(Param *info);

				SceVoid Start(Param *info)
				{
					info->parent->PlayEffect(0.0f, effect::EffectType_Popup1);
				}

				Submenu *workObj;
			};

			class Item
			{
			public:

				Item() : surface(SCE_NULL)
				{

				}

				InvItemType type;
				wstring name;
				wstring time;
				wstring stat;
				string id;
				string surfacePath;
				graph::Surface *surface;
			};

			YouTube *parent;
			ui::Widget *submenuRoot;
			ui::ListView *list;
			SceBool interrupted;
			SceBool allJobsComplete;

			vector<Item> results;
			SceUInt32 currentPage;
		};

		class SearchSubmenu : public Submenu
		{
		public:

			class SearchJob : public job::JobItem
			{
			public:

				using job::JobItem::JobItem;

				~SearchJob() {}

				SceVoid Run();

				SceVoid Finish() {}

				SearchSubmenu *workObj;
				SceBool isId;
			};

			static SceVoid SearchButtonCbFun(SceInt32 eventId, ui::Widget *self, SceInt32 a3, ScePVoid pUserData);

			SearchSubmenu(YouTube *parentObj);

			~SearchSubmenu();

			SceVoid GoToNextPage();

			SceVoid GoToPrevPage();

			SubmenuType GetType()
			{
				return SubmenuType_Search;
			}

			ui::TextBox *searchBox;
			ui::Widget *searchButton;
			string request;
		};

		class HistorySubmenu : public Submenu
		{
		public:

			class HistoryJob : public job::JobItem
			{
			public:

				using job::JobItem::JobItem;

				~HistoryJob() {}

				SceVoid Run();

				SceVoid Finish() {}

				HistorySubmenu *workObj;
			};

			HistorySubmenu(YouTube *parentObj);

			~HistorySubmenu();

			SceVoid GoToNextPage();

			SceVoid GoToPrevPage();

			SubmenuType GetType()
			{
				return SubmenuType_History;
			}
		};

		class FavouriteSubmenu : public Submenu
		{
		public:

			class FavouriteJob : public job::JobItem
			{
			public:

				using job::JobItem::JobItem;

				~FavouriteJob() {}

				SceVoid Run();

				SceVoid Finish() {}

				FavouriteSubmenu *workObj;
			};

			static SceVoid SearchButtonCbFun(SceInt32 eventId, ui::Widget *self, SceInt32 a3, ScePVoid pUserData);

			FavouriteSubmenu(YouTube *parentObj);

			~FavouriteSubmenu();

			SceVoid GoToNextPage();

			SceVoid GoToPrevPage();

			SubmenuType GetType()
			{
				return SubmenuType_Favourites;
			}

			ui::TextBox *searchBox;
			ui::Widget *searchButton;
			string request;
		};

		static SceVoid BackButtonCbFun(SceInt32 eventId, ui::Widget *self, SceInt32 a3, ScePVoid pUserData);
		static SceVoid ListButtonCbFun(SceInt32 eventId, ui::Widget *self, SceInt32 a3, ScePVoid pUserData);
		static SceVoid SubmenuButtonCbFun(SceInt32 eventId, ui::Widget *self, SceInt32 a3, ScePVoid pUserData);
		static SceInt32 SettingsValueChangeCb(const char *id, const char *newValue, ScePVoid pUserArg);

		YouTube();

		~YouTube();

		SceVoid SwitchSubmenu(Submenu::SubmenuType type);

		MenuType GetMenuType()
		{
			return MenuType_Youtube;
		}

		const SceUInt32 *GetSupportedSettingsItems(SceInt32 *count)
		{
			*count = sizeof(k_settingsIdList) / sizeof(char*);
			return k_settingsIdList;
		}

		ui::Widget *browserRoot;
		ui::BusyIndicator *loaderIndicator;
		ui::Text *topText;
		ui::Box *btMenu;
		ui::Widget *searchBt;
		ui::Widget *histBt;
		ui::Widget *favBt;
		Submenu *currentSubmenu;

		const SceUInt32 k_settingsIdList[6] = {
			youtube_search_setting,
			youtube_comment_setting,
			youtube_quality_setting,
			youtube_player_setting,
			button_youtube_clean_history,
			button_youtube_clean_fav,
		};
	};

}

#endif
