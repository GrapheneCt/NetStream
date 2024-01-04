#ifndef _MENU_SERVER_H_
#define _MENU_SERVER_H_

#include <kernel.h>
#include <paf.h>

#include "dialog.h"
#include "menu_generic.h"
#include "generic_server_browser.h"
#include "menus/menu_player_simple.h"

using namespace paf;

namespace menu {
	class GenericServerMenu : public GenericMenu
	{
	public:

		class ListViewCb : public ui::listview::ItemFactory
		{
		public:

			ListViewCb(GenericServerMenu *parent) : m_parent(parent)
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

			GenericServerMenu *m_parent;
		};

		class GoToJob : public job::JobItem
		{
		public:

			GoToJob(GenericServerMenu *parent) : job::JobItem("Server::GoToJob", NULL), m_parent(parent)
			{

			}

			~GoToJob() {}

			void Run()
			{
				m_parent->GoTo(m_targetRef);
			}

			void Finish() {}

			void SetRef(const string& ref)
			{
				m_targetRef = ref;
			}

		private:

			GenericServerMenu *m_parent;
			string m_targetRef;
		};

		class BrowserPage
		{
		public:

			BrowserPage() : m_isLoaded(false)
			{

			}

			~BrowserPage()
			{

			}

			vector<GenericServerBrowser::Entry *> *m_itemList;
			ui::ListView *m_list;
			bool m_isLoaded;
		};

		GenericServerMenu();

		virtual ~GenericServerMenu();

		virtual MenuType GetMenuType() = 0;

		virtual const uint32_t *GetSupportedSettingsItems(int32_t *count) = 0;

		ui::ListItem* CreateListItem(ui::listview::ItemFactory::CreateParam& param);

		void GoTo(const string& ref);

		bool PushBrowserPage(string *ref);

		bool PopBrowserPage();

	protected:

		GenericServerBrowser *m_browser;

	private:

		void OnBackButton();
		void OnListButton(ui::Widget *wdg);
		void OnSettingsButton();
		void OnPlayerEvent(int32_t type);
		void OnOptionMenuEvent(int32_t type);
		void OnConnectionFailedDialogEvent();
		void OnPlayerCreateTimeout(GenericServerBrowser::Entry *entry);

		ui::Widget *m_browserRoot;
		ui::BusyIndicator *m_loaderIndicator;
		ui::Text *m_topText;
		vector<BrowserPage *> m_pageList;
		menu::PlayerSimple *m_player;
		bool m_firstBoot;
		bool m_playerFailed;
	};
}

#endif