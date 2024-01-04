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

			ui::ListItem* CreateListItem(ui::listview::ItemFactory::CreateParam& param);

			class ListViewCb : public ui::listview::ItemFactory
			{
			public:

				ListViewCb(Submenu *parent) : m_parent(parent)
				{

				}

				~ListViewCb()
				{

				}

				ui::ListItem *Create(CreateParam& param)
				{
					return m_parent->CreateListItem(param);
				}

				void Start(StartParam& param)
				{
					param.list_item->Show(common::transition::Type_Popup1);
				}

			private:

				Submenu *m_parent;
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

		protected:

			YouTube *m_baseParent;
			ui::Widget *m_submenuRoot;
			ui::ListView *m_list;
			bool m_interrupted;
			bool m_allJobsComplete;
			vector<Item> m_results;
			uint32_t m_currentPage;

		private:

			static void OnTexPoolAdd(int32_t type, ui::Handler *self, ui::Event *e, void *userdata);

			void OnListButton(ui::Widget *wdg);
		};

		class SearchSubmenu : public Submenu
		{
		public:

			class SearchJob : public job::JobItem
			{
			public:

				SearchJob(SearchSubmenu *parent) : job::JobItem("YouTube::SearchJob", NULL), m_parent(parent)
				{

				}

				~SearchJob() {}

				void Run()
				{
					m_parent->Search();
				}

				void Finish() {}

			private:

				SearchSubmenu *m_parent;
			};

			SearchSubmenu(YouTube *parentObj);

			~SearchSubmenu();

			void Search();

			void GoToNextPage();

			void GoToPrevPage();

			SubmenuType GetType()
			{
				return SubmenuType_Search;
			}

		private:

			void OnSearchButton();

			ui::TextBox *m_searchBox;
			ui::Widget *m_searchButton;
			string m_request;
		};

		class HistorySubmenu : public Submenu
		{
		public:

			class HistoryJob : public job::JobItem
			{
			public:

				HistoryJob(HistorySubmenu *parent) : job::JobItem("YouTube::HistoryJob", NULL), m_parent(parent)
				{

				}

				~HistoryJob() {}

				void Run()
				{
					m_parent->Parse();
				}

				void Finish() {}

			private:

				HistorySubmenu *m_parent;
			};

			HistorySubmenu(YouTube *parentObj);

			~HistorySubmenu();

			void Parse();

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

				FavouriteJob(FavouriteSubmenu *parent) : job::JobItem("YouTube::FavouriteJob", NULL), m_parent(parent)
				{

				}

				~FavouriteJob() {}

				void Run()
				{
					m_parent->Parse();
				}

				void Finish() {}

			private:

				FavouriteSubmenu *m_parent;
			};

			FavouriteSubmenu(YouTube *parentObj);

			~FavouriteSubmenu();

			void Parse();

			void GoToNextPage();

			void GoToPrevPage();

			SubmenuType GetType()
			{
				return SubmenuType_Favourites;
			}

		private:

			void OnSearchButton();

			ui::TextBox *m_searchBox;
			ui::Widget *m_searchButton;
			string m_request;
		};

		class LogClearJob : public job::JobItem
		{
		public:

			enum Type
			{
				Type_Fav,
				Type_Hist
			};

			LogClearJob(Type type) : job::JobItem("YouTube::LogClearJob", NULL), m_type(type)
			{

			}

			~LogClearJob() {}

			void Run();

			void Finish() {}

		private:

			Type m_type;
		};

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

	private:

		void OnBackButton();
		void OnSubmenuButton(ui::Widget *wdg);
		void OnSettingsButton();
		void OnOptionMenuEvent(int32_t type, int32_t subtype);
		void OnDialogEvent(int32_t type);
		void OnSettingsEvent(int32_t type);

		ui::Widget *m_browserRoot;
		ui::BusyIndicator *m_loaderIndicator;
		ui::Text *m_topText;
		ui::Box *m_btMenu;
		ui::Widget *m_searchBt;
		ui::Widget *m_histBt;
		ui::Widget *m_favBt;
		Submenu *m_currentSubmenu;
		int32_t m_dialogIdx;
		TexPool *m_texPool;

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
