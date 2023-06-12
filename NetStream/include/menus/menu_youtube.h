#ifndef _MENU_YOUTUBE_H_
#define _MENU_YOUTUBE_H_

#include <kernel.h>
#include <paf.h>

#include "dialog.h"
#include "tex_pool.h"
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

			virtual void ReleaseCurrentPage();

			virtual void GoToNextPage() = 0;

			virtual void GoToPrevPage() = 0;

			virtual SubmenuType GetType() = 0;

			class ListViewCb : public ui::listview::ItemFactory
			{
			public:

				static void TexPoolAddCbFun(int32_t type, ui::Handler *self, ui::Event *e, void *userdata);

				~ListViewCb()
				{

				}

				ui::ListItem *Create(CreateParam& param);

				void Start(StartParam& param)
				{
					param.list_item->Show(common::transition::Type_Popup1);
				}

				Submenu *workObj;
			};

			class Item
			{
			public:

				Item()
				{

				}

				InvItemType type;
				wstring name;
				wstring time;
				wstring stat;
				string videoId;
				IDParam texId;
				TexPool *texPool;
			};

			YouTube *parent;
			ui::Widget *submenuRoot;
			ui::ListView *list;
			bool interrupted;
			bool allJobsComplete;
			vector<Item> results;
			uint32_t currentPage;
		};

		class SearchSubmenu : public Submenu
		{
		public:

			class SearchJob : public job::JobItem
			{
			public:

				using job::JobItem::JobItem;

				~SearchJob() {}

				void Run();

				void Finish() {}

				SearchSubmenu *workObj;
				bool isId;
			};

			static void SearchButtonCbFun(int32_t type, ui::Handler *self, ui::Event *e, void *userdata);

			SearchSubmenu(YouTube *parentObj);

			~SearchSubmenu();

			void GoToNextPage();

			void GoToPrevPage();

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

				void Run();

				void Finish() {}

				HistorySubmenu *workObj;
			};

			HistorySubmenu(YouTube *parentObj);

			~HistorySubmenu();

			void GoToNextPage();

			void GoToPrevPage();

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

				void Run();

				void Finish() {}

				FavouriteSubmenu *workObj;
			};

			static void SearchButtonCbFun(int32_t type, ui::Handler *self, ui::Event *e, void *userdata);

			FavouriteSubmenu(YouTube *parentObj);

			~FavouriteSubmenu();

			void GoToNextPage();

			void GoToPrevPage();

			SubmenuType GetType()
			{
				return SubmenuType_Favourites;
			}

			ui::TextBox *searchBox;
			ui::Widget *searchButton;
			string request;
		};

		class LogClearJob : public job::JobItem
		{
		public:

			enum Type
			{
				Type_Fav,
				Type_Hist
			};

			using job::JobItem::JobItem;

			~LogClearJob() {}

			void Run();

			void Finish() {}

			Type type;
		};

		static void BackButtonCbFun(int32_t type, ui::Handler *self, ui::Event *e, void *userdata);
		static void ListButtonCbFun(int32_t type, ui::Handler *self, ui::Event *e, void *userdata);
		static void SubmenuButtonCbFun(int32_t type, ui::Handler *self, ui::Event *e, void *userdata);
		static void SettingsButtonCbFun(int32_t type, ui::Handler *self, ui::Event *e, void *userdata);
		static void OptionMenuEventCbFun(int32_t type, ui::Handler *self, ui::Event *e, void *userdata);
		static void DialogHandlerCbFun(int32_t type, ui::Handler *self, ui::Event *e, void *userdata);

		YouTube();

		~YouTube();

		void SwitchSubmenu(Submenu::SubmenuType type);

		MenuType GetMenuType()
		{
			return MenuType_Youtube;
		}

		const uint32_t *GetSupportedSettingsItems(int32_t *count)
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
		int32_t dialogIdx;
		TexPool *texPool;

		const uint32_t k_settingsIdList[4] = {
			youtube_search_setting,
			youtube_comment_setting,
			youtube_quality_setting,
			youtube_player_setting
		};
	};

}

#endif
