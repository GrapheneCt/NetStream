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

		static SceVoid PlayerBackCb(PlayerSimple *player, ScePVoid pUserArg);
		static SceVoid PlayerFailCb(PlayerSimple *player, ScePVoid pUserArg);
		static SceVoid OptionButtonCb(SceUInt32 index, ScePVoid pUserData);
		static SceVoid BackButtonCbFun(SceInt32 eventId, ui::Widget *self, SceInt32 a3, ScePVoid pUserData);
		static SceVoid ListButtonCbFun(SceInt32 eventId, ui::Widget *self, SceInt32 a3, ScePVoid pUserData);
		static SceVoid SettingsButtonCbFun(SceInt32 eventId, ui::Widget *self, SceInt32 a3, ScePVoid pUserData);

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

			GenericServerMenu *workObj;
		};

		class GoToJob : public job::JobItem
		{
		public:

			using job::JobItem::JobItem;

			~GoToJob() {}

			SceVoid Run();

			SceVoid Finish() {}

			static SceVoid ConnectionFailedDialogHandler(dialog::ButtonCode buttonCode, ScePVoid pUserArg);

			GenericServerMenu *workObj;
			string targetRef;
		};

		class BrowserPage
		{
		public:

			BrowserPage() : isLoaded(SCE_FALSE)
			{

			}

			~BrowserPage()
			{

			}

			vector<GenericServerBrowser::Entry *> *itemList;
			ui::ListView *list;
			SceBool isLoaded;
		};

		GenericServerMenu();

		virtual ~GenericServerMenu();

		virtual MenuType GetMenuType() = 0;

		virtual const SceUInt32 *GetSupportedSettingsItems(SceInt32 *count) = 0;

		SceBool PushBrowserPage(string *ref);

		SceBool PopBrowserPage();

		GenericServerBrowser *browser;
		ui::Widget *browserRoot;
		ui::BusyIndicator *loaderIndicator;
		ui::Text *topText;
		vector<BrowserPage *> pageList;
		SceBool firstBoot;
	};
}

#endif