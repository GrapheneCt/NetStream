#ifndef _MENU_HTTP_H_
#define _MENU_HTTP_H_

#include <kernel.h>
#include <paf.h>

#include "dialog.h"
#include "menu_server.h"
#include "http_server_browser.h"
#include "menus/menu_player_simple.h"

using namespace paf;

namespace menu {
	class Http : public GenericServerMenu
	{
	public:

		Http()
		{
			sce::AppSettings *settings = menu::Settings::GetAppSetInstance();

			char host[256];
			char port[32];
			char user[256];
			char password[256];

			settings->GetString("http_host", host, sizeof(host), "");
			settings->GetString("http_port", port, sizeof(port), "");
			settings->GetString("http_user", user, sizeof(user), "");
			settings->GetString("http_password", password, sizeof(password), "");

			m_browser = new HttpServerBrowser(host, port, user, password);
		}

		MenuType GetMenuType()
		{
			return MenuType_Http;
		}

		const uint32_t *GetSupportedSettingsItems(int32_t *count)
		{
			*count = sizeof(k_settingsIdList) / sizeof(char*);
			return k_settingsIdList;
		}

	private:

		const uint32_t k_settingsIdList[1] = {
			http_setting
		};
	};
}

#endif