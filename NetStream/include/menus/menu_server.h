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

		static void BackButtonCbFun(int32_t type, ui::Handler *self, ui::Event *e, void *userdata);
		static void ListButtonCbFun(int32_t type, ui::Handler *self, ui::Event *e, void *userdata);
		static void SettingsButtonCbFun(int32_t type, ui::Handler *self, ui::Event *e, void *userdata);
		static void PlayerEventCbFun(int32_t type, ui::Handler *self, ui::Event *e, void *userdata);
		static void OptionMenuEventCbFun(int32_t type, ui::Handler *self, ui::Event *e, void *userdata);
		static void ConnectionFailedDialogHandler(int32_t type, ui::Handler *self, ui::Event *e, void *userdata);

		static void PlayerCreateTimeoutFun(void *userdata1, void *userdata2);

		class ListViewCb : public ui::listview::ItemFactory
		{
		public:

			~ListViewCb()
			{

			}

			ui::ListItem *Create(CreateParam& param);

			void Start(StartParam& param)
			{
				param.list_item->Show(common::transition::Type_Popup1);
			}

			GenericServerMenu *workObj;
		};

		class GoToJob : public job::JobItem
		{
		public:

			using job::JobItem::JobItem;

			~GoToJob() {}

			void Run();

			void Finish() {}

			GenericServerMenu *workObj;
			string targetRef;
		};

		class BrowserPage
		{
		public:

			BrowserPage() : isLoaded(false)
			{

			}

			~BrowserPage()
			{

			}

			vector<GenericServerBrowser::Entry *> *itemList;
			ui::ListView *list;
			bool isLoaded;
		};

		GenericServerMenu();

		virtual ~GenericServerMenu();

		virtual MenuType GetMenuType() = 0;

		virtual const uint32_t *GetSupportedSettingsItems(int32_t *count) = 0;

		bool PushBrowserPage(string *ref);

		bool PopBrowserPage();

		GenericServerBrowser *browser;
		ui::Widget *browserRoot;
		ui::BusyIndicator *loaderIndicator;
		ui::Text *topText;
		vector<BrowserPage *> pageList;
		menu::PlayerSimple *player;
		bool firstBoot;
		bool playerFailed;
	};
}

#endif