#ifndef _MENU_HVDB_H_
#define _MENU_HVDB_H_

#include <kernel.h>
#include <paf.h>

#include "dialog.h"
#include "tex_pool.h"
#include "hvdb.h"
#include "menu_generic.h"

using namespace paf;

namespace menu {
	class HVDB : public GenericMenu
	{
	public:

		class Submenu
		{
		public:

			enum SubmenuType
			{
				SubmenuType_Favourites = 0x947652c2
			};

			Submenu(HVDB *parentObj);

			virtual ~Submenu();

			virtual void ReleaseCurrentPage();

			virtual void GoToNextPage() = 0;

			virtual void GoToPrevPage() = 0;

			virtual void GoToBasePageForSync() = 0;

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

				wstring name;
				wstring stat;
				string workId;
				string bannerUrl;
				IDParam texId;
				TexPool *texPool;
			};

		protected:

			HVDB *m_baseParent;
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

		class FavouriteSubmenu : public Submenu
		{
		public:

			class FavouriteJob : public job::JobItem
			{
			public:

				FavouriteJob(FavouriteSubmenu *parent) : job::JobItem("HVDB::FavouriteJob", NULL), m_parent(parent)
				{

				}

				~FavouriteJob() {}

				int32_t Run()
				{
					m_parent->ParseFavourite();

					return SCE_PAF_OK;
				}

				void Finish(int32_t result) {}

			private:

				FavouriteSubmenu *m_parent;
			};

			FavouriteSubmenu(HVDB *parentObj);

			~FavouriteSubmenu();

			void ParseFavourite();

			void GoToNextPage();

			void GoToPrevPage();

			void GoToBasePageForSync();

			SubmenuType GetType()
			{
				return SubmenuType_Favourites;
			}

		private:

			HVDBPaginationInfo m_pgi;
		};

		HVDB();

		~HVDB()  override;

		void SwitchSubmenu(Submenu::SubmenuType type);

		void Refresh();

		MenuType GetMenuType() override
		{
			return MenuType_Hvdb;
		}

		const uint32_t *GetSupportedSettingsItems(int32_t *count) override
		{
			*count = sizeof(k_settingsIdList) / sizeof(char*);
			return k_settingsIdList;
		}

	private:

		void OnBackButton();
		void OnSubmenuButton(ui::Widget *wdg);
		void OnSettingsButton();
		void OnOptionMenuEvent(int32_t type, int32_t subtype);

		bool AuthMe();

		ui::Widget *m_browserRoot;
		ui::BusyIndicator *m_loaderIndicator;
		ui::Text *m_topText;
		Submenu *m_currentSubmenu;
		int32_t m_dialogIdx;
		TexPool *m_texPool;

		const uint32_t k_settingsIdList[3] = {
			hvdb_account_setting,
			hvdb_search_setting,
			hvdb_player_setting
		};
	};

}

#endif
